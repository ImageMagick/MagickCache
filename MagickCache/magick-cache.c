/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M   AAA    GGGG  IIIII   CCCC  K   K                   %
%                  MM MM  A   A  G        I    C      K  K                    %
%                  M M M  AAAAA  G GGG    I    C      KKK                     %
%                  M   M  A   A  G   G    I    C      K  K                    %
%                  M   M  A   A   GGGG  IIIII   CCCC  K   K                   %
%                                                                             %
%                      CCCC   AAA    CCCC  H   H  EEEEE                       %
%                     C      A   A  C      H   H  E                           %
%                     C      AAAAA  C      HHHHH  EEE                         %
%                     C      A   A  C      H   H  E                           %
%                      CCCC  A   A   CCCC  H   H  EEEEE                       %
%                                                                             %
%                         MagickCache CRUD Methods                            %
%                                                                             %
%                            Software Design                                  %
%                                Cristy                                       %
%                              March 2021                                     %
%                                                                             %
%                                                                             %
%  Copyright 1999-2021 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/script/license.php                               %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The MagickCache provides secure methods and tools to cache images, image
%  sequences, video, audio or metadata in a local folder. Any content is
%  memory-mapped for efficient retrieval.  Additional efficiences are possible
%  by retrieving a portion of an image.  Content can persist or you can assign
%  a time-to-live (TTL) to automatically expire content when the TTL is
%  exceeded. MagickCache supports virtually unlimited content upwards of
%  billions of images making it suitable as a web image service.
%
*/

#include <MagickCore/studio.h>
#include <MagickCore/MagickCore.h>
#include "MagickCache/MagickCache.h"
#include "MagickCache/magick-cache-private.h"

/*
  MagickCache defines.
*/
#define MagickCacheAPIVersion  1
#define MagickCacheMax(x,y)  (((x) > (y)) ? (x) : (y))
#define MagickCacheMin(x,y)  (((x) < (y)) ? (x) : (y))
#define MagickCacheDigestExtent  64
#define MagickCacheNonce  "MagickCache"
#define MagickCacheNonceExtent  8
#define MagickCacheSignature  0xabacadabU
#define ThrowMagickCacheException(severity,tag,context) \
{ \
  (void) ThrowMagickException(cache->exception,GetMagickModule(),severity,tag, \
    "`%s'",context); \
  CatchException(cache->exception); \
}

/*
  MagickCache structures.
*/
struct _MagickCache
{
  char
    *path;

  StringInfo
    *nonce,
    *passkey;

  char
    *digest;

  time_t
    timestamp;

  ExceptionInfo
    *exception;

  RandomInfo
    *random_info;

  MagickBooleanType
    debug;

  size_t
    signature;
};

struct _MagickCacheResource
{
  MagickCacheResourceType
    resource_type;

  char
    *iri,
    *project,
    *type,
    *id;

  size_t
    columns,
    rows,
    extent,
    version;

  StringInfo
    *nonce;

  time_t
    timestamp,
    ttl;

  void
    *blob;

  MagickBooleanType
    memory_mapped;

  ExceptionInfo
    *exception;

  MagickBooleanType
    debug;

  size_t
    signature;
};

struct ResourceNode
{
  char
    *path;

  struct ResourceNode
    *previous,
    *next;
};

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k C a c h e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickCache() creates a MagickCache structure for getting or putting
%  resources, from or to the MagickCache repository.  NULL is returned if the
%  MagickCache repo is not found or if the repo is not compatible with the
%  current API version.
%
%  The format of the AcquireMagickCache method is:
%
%      MagickCache *AcquireMagickCache(const char *path,
%        const StringInfo *passkey)
%
%  A description of each parameter follows:
%
%    o path: the MagickCache path, absolute (e.g. /myrepo) or relative (e.g.
%      ./myrepo).
%
%    o passkey: the MagickCache passkey.
%
*/

static void GetMagickCacheSentinel(MagickCache *cache,unsigned char *sentinel)
{
  unsigned char
    *p;

  unsigned int
    signature;

  /*
    Retrieve the MagickCache sentinel.
  */
  p=sentinel;
  p+=sizeof(signature);
  (void) memcpy(GetStringInfoDatum(cache->nonce),p,
    GetStringInfoLength(cache->nonce));
  p+=GetStringInfoLength(cache->nonce);
  (void) memcpy(cache->digest,p,strlen(cache->digest));
  p+=strlen(cache->digest);
}

static inline unsigned int GetMagickCacheSignature(const StringInfo *nonce)
{
  StringInfo
    *version;

  unsigned char
    *p;

  unsigned int
    signature;

  /*
    Generate a MagickCache signature based on the MagickCache properties.
  */
  version=AcquireStringInfo(MagickPathExtent);
  p=GetStringInfoDatum(version);
  (void) memcpy(p,MagickCachePackageName,strlen(MagickCachePackageName));
  p+=strlen(MagickCachePackageName);
  signature=MagickCacheAPIVersion;
  (void) memcpy(p,&signature,sizeof(signature));
  p+=sizeof(signature);
  signature=MagickCacheSignature;
  (void) memcpy(p,&signature,sizeof(signature));
  p+=sizeof(signature);
  SetStringInfoLength(version,(size_t) (p-GetStringInfoDatum(version)));
  ConcatenateStringInfo(version,nonce);
  signature=CRC32(GetStringInfoDatum(version),GetStringInfoLength(version));
  version=DestroyStringInfo(version);
  return(signature);
}

MagickExport MagickCache *AcquireMagickCache(const char *path,
  const StringInfo *passkey)
{
  char
    *sentinel_path;

  MagickCache
    *cache;

  size_t
    extent;

  struct stat
    attributes;

  unsigned int
    signature;

  void
    *sentinel;

  /*
    Confirm the path exists.
  */
  if (path == (const char *) NULL)
    return((MagickCache *) NULL);
  if (GetPathAttributes(path,&attributes) <= 0)
    return((MagickCache *) NULL);
  /*
    Basic sanity check passes, allocate & initialize a MagickCache struture.
  */
  cache=(MagickCache *) AcquireCriticalMemory(sizeof(*cache));
  (void) memset(cache,0,sizeof(*cache));
  cache->path=ConstantString(path);
  cache->timestamp=(time_t) attributes.st_ctime;
  cache->random_info=AcquireRandomInfo();
  cache->nonce=AcquireStringInfo(MagickCacheNonceExtent);
  if (passkey == (StringInfo *) NULL)
    cache->passkey=AcquireStringInfo(0);
  else
    cache->passkey=CloneStringInfo(passkey);
  cache->digest=StringInfoToDigest(cache->passkey);
  cache->exception=AcquireExceptionInfo();
  cache->debug=IsEventLogging();
  cache->signature=MagickCacheSignature;
  /*
    Validate the MagickCache sentinel.
  */
  sentinel_path=AcquireString(path);
  (void) ConcatenateString(&sentinel_path,"/");
  (void) ConcatenateString(&sentinel_path,MagickCacheSentinel);
  sentinel=FileToBlob(sentinel_path,~0UL,&extent,cache->exception);
  sentinel_path=DestroyString(sentinel_path);
  if (sentinel == NULL)
    {
      cache=DestroyMagickCache(cache);
      return((MagickCache *) NULL);
    }
  GetMagickCacheSentinel(cache,(unsigned char *) sentinel);
  signature=GetMagickCacheSignature(cache->nonce);
  if (memcmp(&signature,sentinel,sizeof(signature)) != 0)
    {
      sentinel=RelinquishMagickMemory(sentinel);
      cache=DestroyMagickCache(cache);
      return((MagickCache *) NULL);
    }
  sentinel=RelinquishMagickMemory(sentinel);
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k C a c h e R e s o u r c e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickCacheResource() allocates the MagickCacheResource structure.
%  It is required before you can get or put metadata associated with resource
%  content.
%
%  The format of the AcquireMagickCacheResource method is:
%
%      MagickCacheResource *AcquireMagickCacheResource(MagickCache *cache,
%        const char *iri)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o iri: the IRI.
%
*/
MagickExport MagickCacheResource *AcquireMagickCacheResource(MagickCache *cache,
  const char *iri)
{
  MagickCacheResource
    *resource;

  resource=(MagickCacheResource *) AcquireCriticalMemory(sizeof(*resource));
  (void) memset(resource,0,sizeof(*resource));
  resource->nonce=GetRandomKey(cache->random_info,MagickCacheNonceExtent);
  resource->version=MagickCacheAPIVersion;
  resource->exception=AcquireExceptionInfo();
  resource->signature=MagickCacheSignature;
  (void) SetMagickCacheResourceIRI(cache,resource,iri);
  return(resource);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l e a r M a g i c k C a c h e E x c e p t i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClearMagickCacheException() clears any exceptions associated with the cache.
%
%  The format of the ClearMagickCacheException method is:
%
%      MagickBooleanType ClearMagickCacheException(MagickCache *cache)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
*/
MagickExport MagickBooleanType ClearMagickCacheException(MagickCache *cache)
{
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  if (cache->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
      cache->path != (char *) NULL ? cache->path : "");
  ClearMagickException(cache->exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l e a r M a g i c k C a c h e R e s o u r c e E x c e p t i o n         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClearMagickCacheResourceException() clears any exceptions associated with
%  the resource.
%
%  The format of the ClearMagickCacheResourceException method is:
%
%      MagickBooleanType ClearMagickCacheResourceException(
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport MagickBooleanType ClearMagickCacheResourceException(
  MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  if (resource->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
      resource->iri != (char *) NULL ? resource->iri : "");
  ClearMagickException(resource->exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e E x t e n t                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceExtent() returns the resource extent associated with
%  the resource.  That is, the number of bytes the resource consumes on disk.
%
%  The format of the GetMagickCacheResourceTTL method is:
%
%      const size_t GetMagickCacheResourceExtent(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport size_t GetMagickCacheResourceExtent(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->extent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C r e a t e M a g i c k C a c h e                                         %
%                                                                             %
%                                                                             % %                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CreateMagickCache() creates a MagickCache repository suitably prepared for
%  storing and retrieving images, image sequences, video, and metadata
%  resources.
%
%  The format of the CreateMagickCache method is:
%
%      MagickBooleanType CreateMagickCache(const char *path,
%        const StringInfo *passkey)
%
%  A description of each parameter follows:
%
%    o path: the MagickCache directory path, absolute (e.g. /myrepo) or
%      relative (e.g. ./myrepo).
%
%    o passkey: the MagickCache passkey.
*/

static StringInfo *SetMagickCacheSentinel(const char *path,
  const StringInfo *passkey)
{
  char
    *digest;

  RandomInfo
    *random_info;

  StringInfo
    *key_info,
    *cache_key,
    *sentinel;

  unsigned char
    *p;

  unsigned int
    signature;

  /*
    Create a MagickCache sentiinel.
  */
  sentinel=AcquireStringInfo(MagickPathExtent);
  random_info=AcquireRandomInfo();
  key_info=GetRandomKey(random_info,MagickCacheNonceExtent);
  p=GetStringInfoDatum(sentinel);
  signature=GetMagickCacheSignature(key_info);
  (void) memcpy(p,&signature,sizeof(signature));
  p+=sizeof(signature);
  (void) memcpy(p,GetStringInfoDatum(key_info),MagickCacheNonceExtent);
  p+=MagickCacheNonceExtent;
  cache_key=StringToStringInfo(path);
  if (passkey != (const StringInfo *) NULL)
    ConcatenateStringInfo(cache_key,passkey);
  ConcatenateStringInfo(cache_key,key_info);
  digest=StringInfoToDigest(cache_key);
  cache_key=DestroyStringInfo(cache_key);
  (void) memcpy(p,digest,strlen(digest));
  p+=strlen(digest);
  SetStringInfoLength(sentinel,(size_t) (p-GetStringInfoDatum(sentinel)));
  digest=DestroyString(digest);
  key_info=DestroyStringInfo(key_info);
  random_info=DestroyRandomInfo(random_info);
  return(sentinel);
}

MagickExport MagickBooleanType CreateMagickCache(const char *path,
  const StringInfo *passkey)
{
  char
    *sentinel_path;

  ExceptionInfo
    *exception;

  MagickBooleanType
    status;

  StringInfo
    *meta;

  /*
    Create the MagickCache path.
  */
  if (MagickCreatePath(path) == MagickFalse)
    return(MagickFalse);
  /*
    Create the MagickCache sentinel.
  */
  sentinel_path=AcquireString(path);
  (void) ConcatenateString(&sentinel_path,"/");
  (void) ConcatenateString(&sentinel_path,MagickCacheSentinel);
  if (IsPathAccessible(sentinel_path) != MagickFalse)
    {
      sentinel_path=DestroyString(sentinel_path);
      errno=EEXIST;
      return(MagickFalse);
    }
  meta=SetMagickCacheSentinel(path,passkey);
  exception=AcquireExceptionInfo();
  status=BlobToFile(sentinel_path,GetStringInfoDatum(meta),
    GetStringInfoLength(meta),exception);
  exception=DestroyExceptionInfo(exception);
  meta=DestroyStringInfo(meta);
  sentinel_path=DestroyString(sentinel_path);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e M a g i c k C a c h e R e s o u r c e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteMagickCacheResource() deletes a resource within the cache repository.
%
%  The format of the DeleteMagickCacheResource method is:
%
%      MagickBooleanType DeleteMagickCacheResource(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the MagickCache resource.
%
*/
MagickExport MagickBooleanType DeleteMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *iri,
    *path;

  MagickBooleanType
    status;

  /*
    Check that resource id exists in MagickCache.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(MagickFalse);
  /*
    Delete resource ID in MagickCache.
  */
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  if (remove_utf8(path) != 0)
    {
      path=DestroyString(path);
      return(MagickFalse);
    }
  if (resource->resource_type == ImageResourceType)
    {
      /*
        Delete image cache file.
      */
      (void) ConcatenateString(&path,".cache");
      (void) remove_utf8(path);
    }
  path=DestroyString(path);
  /*
    Delete resource sentinel in MagickCache.
  */
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,MagickCacheResourceSentinel);
  if (remove_utf8(path) != 0)
    {
      path=DestroyString(path);
      return(MagickFalse);
    }
  path=DestroyString(path);
  /*
    Delete resource IRI in MagickCache.
  */
  iri=AcquireString(resource->iri);
  for ( ; *iri != '\0'; GetPathComponent(iri,HeadPath,iri))
  {
    path=AcquireString(cache->path);
    (void) ConcatenateString(&path,"/");
    (void) ConcatenateString(&path,iri);
    (void) remove_utf8(path);
    path=DestroyString(path);
  }
  iri=DestroyString(iri);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M a g i c k C a c h e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickCache() deallocates memory associated with a MagickCache.
%
%  The format of the CloseMagickCache method is:
%
%      MagickCache *CloseMagickCache(MagickCache *cache)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
*/
MagickExport MagickCache *DestroyMagickCache(MagickCache *cache)
{
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  if (cache->path != (char *) NULL )
    cache->path=DestroyString(cache->path);
  if (cache->nonce != (StringInfo *) NULL )
    cache->nonce=DestroyStringInfo(cache->nonce);
  if (cache->digest != (char *) NULL )
    cache->digest=DestroyString(cache->digest);
  if (cache->random_info != (RandomInfo *) NULL)
    cache->random_info=DestroyRandomInfo(cache->random_info);
  if (cache->passkey != (StringInfo *) NULL )
    cache->passkey=DestroyStringInfo(cache->passkey);
  if (cache->exception != (ExceptionInfo *) NULL)
    cache->exception=DestroyExceptionInfo(cache->exception);
  cache->signature=(~MagickCacheSignature);
  cache=(MagickCache *) RelinquishMagickMemory(cache);
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M a g i c k C a c h e R e s o u r c e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickCacheResource() deallocates memory associated with a resource.
%
%  The format of the DestroyMagickCacheResource method is:
%
%      MagickCacheResource *DestroyMagickCacheResource(
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/

static MagickBooleanType UnmapResourceBlob(void *map,const size_t length)
{
#if defined(MAGICKCORE_HAVE_MMAP)
  int
    status;

  status=munmap(map,length);
  return(status == -1 ? MagickFalse : MagickTrue);
#else
  (void) map;
  (void) length;
  return(MagickFalse);
#endif
}

static void DestroyMagickCacheResourceBlob(MagickCacheResource *resource)
{
  if (resource->resource_type == ImageResourceType)
    resource->blob=DestroyImageList((Image *) resource->blob);
  else
    if (resource->memory_mapped == MagickFalse)
      resource->blob=RelinquishMagickMemory(resource->blob);
    else
      {
        (void) UnmapResourceBlob(resource->blob,resource->extent);
        resource->memory_mapped=MagickFalse;
      }
}

MagickExport MagickCacheResource *DestroyMagickCacheResource(
  MagickCacheResource *resource)
{
  /*
   Deallocate memory associated with a resource.
  */
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  if (resource->blob != NULL)
    DestroyMagickCacheResourceBlob(resource);
  if (resource->iri != (char *) NULL)
    resource->iri=DestroyString(resource->iri);
  if (resource->project != (char *) NULL)
    resource->project=DestroyString(resource->project);
  if (resource->type != (char *) NULL)
    resource->type=DestroyString(resource->type);
  if (resource->id != (char *) NULL)
    resource->id=DestroyString(resource->id);
  if (resource->nonce != (StringInfo *) NULL)
    resource->nonce=DestroyStringInfo(resource->nonce);
  if (resource->exception != (ExceptionInfo *) NULL)
    resource->exception=DestroyExceptionInfo(resource->exception);
  resource->signature=(~MagickCacheSignature);
  resource=(MagickCacheResource *) RelinquishMagickMemory(resource);
  return(resource);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e E x c e p t i o n                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheException() returns the severity, reason, and description of
%  any exception that occurs associated with the cache repository.
%
%  The format of the GetMagickCacheException method is:
%
%      char *GetMagickCacheException(const MagickCache *cache,
%        ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o severity: the severity of the error is returned here.
%
*/
MagickExport char *GetMagickCacheException(const MagickCache *cache,
  ExceptionType *severity)
{
  char
    *description;

  assert(cache != (const MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  if (cache->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",cache->path);
  assert(severity != (ExceptionType *) NULL);
  *severity=cache->exception->severity;
  description=(char *) AcquireQuantumMemory(2UL*MagickPathExtent,
    sizeof(*description));
  if (description == (char *) NULL)
    {
      (void) ThrowMagickException(cache->exception,GetMagickModule(),CacheError,
        "MemoryAllocationFailed","`%s'",cache->path);
      return((char *) NULL);
    }
  *description='\0';
  if (cache->exception->reason != (char *) NULL)
    (void) CopyMagickString(description,GetLocaleExceptionMessage(
      cache->exception->severity,cache->exception->reason),MagickPathExtent);
  if (cache->exception->description != (char *) NULL)
    {
      (void) ConcatenateMagickString(description," (",MagickPathExtent);
      (void) ConcatenateMagickString(description,GetLocaleExceptionMessage(
        cache->exception->severity,cache->exception->description),
        MagickPathExtent);
      (void) ConcatenateMagickString(description,")",MagickPathExtent);
    }
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResource() get meta content associated with a resource
%  identified by its IRI.  MagickTrue is returned if the resource exists
%  otherwise MagickFalse.
%
%  The format of the GetMagickCacheResource method is:
%
%      MagickBooleanType GetMagickCacheResource(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
*/

static void GetMagickCacheResourceSentinel(MagickCacheResource *resource,
  unsigned char *sentinel)
{
  unsigned char
    *p;

  unsigned int
    signature;

  /*
    Get the MagickCache resource sentinel.
  */
  p=sentinel;
  p+=sizeof(signature);
  (void) memcpy(GetStringInfoDatum(resource->nonce),p,
    GetStringInfoLength(resource->nonce));
  p+=GetStringInfoLength(resource->nonce);
  (void) memcpy(&resource->ttl,p,sizeof(resource->ttl));
  p+=sizeof(resource->ttl);
  (void) memcpy(&resource->columns,p,sizeof(resource->columns));
  p+=sizeof(resource->columns);
  (void) memcpy(&resource->rows,p,sizeof(resource->rows));
  p+=sizeof(resource->rows);
  if (resource->id != (char *) NULL)
    resource->id=DestroyString(resource->id);
  resource->id=StringInfoToDigest(resource->nonce);
  (void) memcpy(resource->id,p,strlen(resource->id));
  p+=strlen(resource->id);
}

static void SetMagickCacheResourceID(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *digest;

  StringInfo
    *signature;

  /*
    Set a MagickCache resource ID.
  */
  signature=StringToStringInfo(resource->iri);
  ConcatenateStringInfo(signature,resource->nonce);
  ConcatenateStringInfo(signature,cache->passkey);
  ConcatenateStringInfo(signature,cache->nonce);
  digest=StringInfoToDigest(signature);
  signature=DestroyStringInfo(signature);
  if (resource->id != (char *) NULL)
    resource->id=DestroyString(resource->id);
  resource->id=digest;
}

MagickExport MagickBooleanType GetMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *digest,
    *path;

  size_t
    extent;

  StringInfo
    *passkey;

  struct stat
    attributes;

  unsigned int
    signature;

  void
    *sentinel;

  /*
    Validate the MagickCache resource sentinel.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,MagickCacheResourceSentinel);
  sentinel=FileToBlob(path,~0UL,&extent,resource->exception);
  path=DestroyString(path);
  if (sentinel == NULL)
    return(MagickFalse);
  GetMagickCacheResourceSentinel(resource,(unsigned char *) sentinel);
  signature=GetMagickCacheSignature(resource->nonce);
  /*
    If no cache passkey, generate the resource ID.
  */
  if (memcmp(&signature,sentinel,sizeof(signature)) != 0)
    {
      sentinel=RelinquishMagickMemory(sentinel);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"resource sentinel signature mismatch","`%s'",path);
      return(MagickFalse);
    }
  sentinel=RelinquishMagickMemory(sentinel);
  passkey=StringToStringInfo(cache->path);
  ConcatenateStringInfo(passkey,cache->passkey);
  ConcatenateStringInfo(passkey,cache->nonce);
  digest=StringInfoToDigest(passkey);
  passkey=DestroyStringInfo(passkey);
  if (strcmp(cache->digest,digest) != 0)
    SetMagickCacheResourceID(cache,resource);
  digest=DestroyString(digest);
  /*
    Verify resource exists.
  */
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  if (GetPathAttributes(path,&attributes) <= 0)
    {
      path=DestroyString(path);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot access resource sentinel","`%s'",path);
      return(MagickFalse);
    }
  resource->timestamp=(time_t) attributes.st_ctime;
  resource->extent=(size_t) attributes.st_size;
  path=DestroyString(path);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e B l o b                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceBlob() gets a blob associated with a resource
%  identified by its IRI.
%
%  The format of the GetMagickCacheResourceBlob method is:
%
%      const void *GetMagickCacheResourceBlob(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
*/

static void *MapResourceBlob(int file,const MapMode mode,
  const MagickOffsetType offset,const size_t length)
{
#if defined(MAGICKCORE_HAVE_MMAP)
  int
    flags,
    protection;

  void
    *map;

  /*
    Map file.
  */
  flags=0;
  if (file == -1)
#if defined(MAP_ANONYMOUS)
    flags|=MAP_ANONYMOUS;
#else
    return(NULL);
#endif
  switch (mode)
  {
    case ReadMode:
    default:
    {
      protection=PROT_READ;
      flags|=MAP_PRIVATE;
      break;
    }
    case WriteMode:
    {
      protection=PROT_WRITE;
      flags|=MAP_SHARED;
      break;
    }
    case IOMode:
    {
      protection=PROT_READ | PROT_WRITE;
      flags|=MAP_SHARED;
      break;
    }
  }
#if !defined(MAGICKCORE_HAVE_HUGEPAGES) || !defined(MAP_HUGETLB)
  map=mmap((char *) NULL,length,protection,flags,file,offset);
#else
  map=mmap((char *) NULL,length,protection,flags | MAP_HUGETLB,file,offset);
  if (map == MAP_FAILED)
    map=mmap((char *) NULL,length,protection,flags,file,offset);
#endif
  if (map == MAP_FAILED)
    return(NULL);
  return(map);
#else
  (void) file;
  (void) mode;
  (void) offset;
  (void) length;
  return(NULL);
#endif
}

static MagickBooleanType ResourceToBlob(MagickCacheResource *resource,
  const char *path)
{
  int
    file;

  ssize_t
    count = 0,
    i;

  struct stat
    attributes;

  /*
    Convert the resource identified by its IRI to a blob.
  */
  if (GetPathAttributes(path,&attributes) == MagickFalse)
    {
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot get resource","`%s'",resource->iri);
      return(MagickFalse);
    }
  resource->extent=(size_t) attributes.st_size;
  file=open_utf8(path,O_RDONLY | O_BINARY,0);
  if (resource->blob != NULL)
    DestroyMagickCacheResourceBlob(resource);
  resource->blob=MapResourceBlob(file,ReadMode,0,resource->extent);
  if (resource->blob != NULL)
    {
      resource->memory_mapped=MagickTrue;
      file=close(file)-1;
      return(MagickTrue);
    }
  resource->blob=AcquireMagickMemory(resource->extent);
  if (resource->blob == NULL)
    return(MagickFalse);
  for (i=0; i < (ssize_t) resource->extent; i+=count)
  {
    ssize_t
      count;

    count=read(file,(unsigned char *) resource->blob+i,(size_t)
      MagickCacheMin(resource->extent-i,(size_t) SSIZE_MAX));
    if (count <= 0)
      {
        count=0;
        if (errno != EINTR)
          break;
      }
  }
  if (i < (ssize_t) resource->extent)
    {
      file=close(file)-1;
      resource->blob=RelinquishMagickMemory(resource->blob);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot get resource","`%s'",resource->iri);
      return(MagickFalse);
    }
  if (close(file) == -1)
    {
      resource->blob=RelinquishMagickMemory(resource->blob);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot get resource","`%s'",resource->iri);
      return(MagickFalse);
    }
  return(MagickTrue);
}

MagickExport void *GetMagickCacheResourceBlob(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  MagickBooleanType
    status;

  /*
    Get the blob associated with a resource identified by its IRI.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(NULL);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  status=ResourceToBlob(resource,path);
  path=DestroyString(path);
  if (status == MagickFalse)
    return((void *) NULL);
  return((void *) resource->blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e E x c e p t i o n             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceException() returns the severity, reason, and
%  description of any exception that occurs assoicated with a resource.
%
%  The format of the GetMagickCacheException method is:
%
%      char *GetMagickCacheResourceException(
%        const MagickCacheResource  *resource,ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o resource: the magick resource.
%
%    o severity: the severity of the error is returned here.
%
*/
MagickExport char *GetMagickCacheResourceException(
  const MagickCacheResource *resource,ExceptionType *severity)
{
  char
    *description;

  /*
    Get a MagickCache resource exception.
  */
  assert(resource != (const MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  if (resource->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",resource->iri);
  assert(severity != (ExceptionType *) NULL);
  *severity=resource->exception->severity;
  description=(char *) AcquireQuantumMemory(2UL*MagickPathExtent,
    sizeof(*description));
  if (description == (char *) NULL)
    {
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"MemoryAllocationFailed","`%s'",resource->iri);
      return((char *) NULL);
    }
  *description='\0';
  if (resource->exception->reason != (char *) NULL)
    (void) CopyMagickString(description,GetLocaleExceptionMessage(
      resource->exception->severity,resource->exception->reason),
      MagickPathExtent);
  if (resource->exception->description != (char *) NULL)
    {
      (void) ConcatenateMagickString(description," (",MagickPathExtent);
      (void) ConcatenateMagickString(description,GetLocaleExceptionMessage(
        resource->exception->severity,resource->exception->description),
        MagickPathExtent);
      (void) ConcatenateMagickString(description,")",MagickPathExtent);
    }
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e I D                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceID() returns a unique resource ID in the ID parameter.
%
%  The format of the GetMagickCacheResourceID method is:
%
%      MagickBooleanType GetMagickCacheResourceID(MagickCache *cache,
%        const size_t length,char *id)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o length: the length of the ID.
%
%    o id: a buffer that is at least `length` bytes long.
%
*/
MagickExport MagickBooleanType GetMagickCacheResourceID(MagickCache *cache,
  const size_t length,char *id)
{
  ssize_t
    i,
    j;

  StringInfo
    *passkey;

  unsigned char
    *code;

  /*
    Get the MagickCache resource ID.
  */
  assert(cache != (const MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  for (j=0; j < (ssize_t) length; j++)
  {
    passkey=GetRandomKey(cache->random_info,length);
    if (passkey == (StringInfo *) NULL)
      return(MagickFalse);
    code=GetStringInfoDatum(passkey);
    for (i=0; i < (ssize_t) length; i++)
    {
      if ((code[i] <= 32) || ((code[i] <= 0x9f && code[i] > 0x7F)))
        continue;
      id[j++]=(int) code[i];
      if (j == (ssize_t) length)
        break;
    }
    passkey=DestroyStringInfo(passkey);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceImage() gets an image associated with a resource
%  identified by its IRI from the cache repository. To retrieve the entire
%  image, set the extract parameter to NULL.  Otherwise specify the size and
%  offset of a portion of the image, e.g. 100x100+0+1.  If you do not specify
%  the offset, the image is instead resized, e.g. 100x100 returns the image
%  resized while still retaining the original aspect ratio.
%
%  The format of the GetMagickCacheResourceImage method is:
%
%      const Image *GetMagickCacheResourceImage(MagickCache *cache,
%        MagickCacheResource *resource,const char *extract)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
%    o extract: the extract geometry.
%
*/
MagickExport Image *GetMagickCacheResourceImage(MagickCache *cache,
  MagickCacheResource *resource,const char *extract)
{
  char
    *path;

  ExceptionInfo
    *exception;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

  /*
    Return the resource identified by its IRI as an image.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return((Image *) NULL);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  if (extract != (const char *) NULL)
    {
      (void) ConcatenateString(&path,"[");
      (void) ConcatenateString(&path,extract);
      (void) ConcatenateString(&path,"]");
    }
  if (strlen(path) > (MagickPathExtent-2))
    {
      path=DestroyString(path);
      errno=ENAMETOOLONG;
      return((Image *) NULL);
    }
  image_info=AcquireImageInfo();
  (void) CopyMagickString(image_info->filename,path,MagickPathExtent);
  (void) CopyMagickString(image_info->magick,"MPC",MagickPathExtent);
  exception=AcquireExceptionInfo();
  if (resource->blob != NULL)
    DestroyMagickCacheResourceBlob(resource);
  resource->blob=(void *) ReadImage(image_info,exception);
  exception=DestroyExceptionInfo(exception);
  if (resource->blob == (void *) NULL)
    (void) ThrowMagickException(resource->exception,GetMagickModule(),
      CacheError,"cannot get resource","`%s'",resource->iri);
  else
    {
      const Image *image = (const Image *) resource->blob;
      resource->columns=image->columns;
      resource->rows=image->rows;
    }
  path=DestroyString(path);
  image_info=DestroyImageInfo(image_info);
  return((Image *) resource->blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e I R I                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceIRI() gets a resource identified by its IRI in the
%  cache repository.
%
%  The format of the GetMagickCacheResourceIRI method is:
%
%      const char *GetMagickCacheResourceIRI(MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport char *GetMagickCacheResourceIRI(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->iri);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e M e t a                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceMeta() gets any metadata associated with the resource
%  in the cache repository..
%
%  The format of the GetMagickCacheResourceMeta method is:
%
%      const char *GetMagickCacheResourceMeta(MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
*/
MagickExport char *GetMagickCacheResourceMeta(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  MagickBooleanType
    status;

  /*
    Return the resource identified by its IRI as metadata.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return((char *) NULL);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  if (strlen(path) > (MagickPathExtent-2))
    {
      path=DestroyString(path);
      errno=ENAMETOOLONG;
      return((char *) NULL);
    }
  status=ResourceToBlob(resource,path);
  path=DestroyString(path);
  if (status == MagickFalse)
    return((char *) NULL);
  return((char *) resource->blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e S i z e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceSize() returns the size of the image associated with
%  the resource in the cache repository.
%
%  The format of the GetMagickCacheResourceSize method is:
%
%      const void GetMagickCacheResourceSize(
%        const MagickCacheResource *resource,size_t *columns,size_t *rows)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport void GetMagickCacheResourceSize(
  const MagickCacheResource *resource,size_t *columns,size_t *rows)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  *columns=resource->columns;
  *rows=resource->rows;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e T i m e s t a m p             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceTimestamp() returns the timestamp associated with a
%  resource in the cache repository.
%
%  The format of the GetMagickCacheResourceTimestamp method is:
%
%      const time_t GetMagickCacheResourceTimestamp(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport time_t GetMagickCacheResourceTimestamp(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->timestamp);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e T T L                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceTTL() returns the time-to-live associated with a
%  resource in the cache repository.
%
%  The format of the GetMagickCacheResourceTTL method is:
%
%      const size_t GetMagickCacheResourceTTL(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport size_t GetMagickCacheResourceTTL(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->ttl);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e T y p e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceType() returns the type associated with a resource in
%  the cache respository.
%
%  The format of the GetMagickCacheResourceType method is:
%
%      const MagickCacheResourceType GetMagickCacheResourceType(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport MagickCacheResourceType GetMagickCacheResourceType(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->resource_type);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e R e s o u r c e V e r s i o n                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheResourceVersion() returns the version associated with a
%  resource in the cache repository.
%
%  The format of the GetMagickCacheResourceVersion method is:
%
%      const size_t GetMagickCacheResourceVersion(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport size_t GetMagickCacheResourceVersion(
  const MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->version);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k C a c h e T i m e s t a m p                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickCacheTimestamp() returns the timestamp associated with the cache
%  repository.
%
%  The format of the GetMagickCacheResource method is:
%
%      const time_t GetMagickCacheResourceTimestamp(const MagickCache *cache)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
*/
MagickExport time_t GetMagickCacheTimestamp(const MagickCache *cache)
{
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  return(cache->timestamp);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i f y M a g i c k C a c h e R e s o u r c e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyMagickCacheResource() identifies a resource within the cache
%  repository.
%
%  The format of the IdentifyMagickCacheResource method is:
%
%      MagickBooleanType IdentifyMagickCacheResource(MagickCache *cache,
%        MagickCacheResource *resource,FILE *file)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o iri: the IRI.
%
%    o file: the file.
%
*/
MagickExport MagickBooleanType IdentifyMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource,FILE *file)
{
  char
    extent[MagickPathExtent],
    iso8601[sizeof("9999-99-99T99:99:99Z")],
    *path,
    size[MagickPathExtent];

  int
    expired;

  MagickBooleanType
    status;

  struct tm
    timestamp;

  /*
    If the resource ID exists, identify it.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  status=GetMagickCacheResource(cache,resource);
  *size='\0';
  if (resource->resource_type == ImageResourceType)
    (void) snprintf(size,MagickPathExtent,"[%gx%g]",(double) resource->columns,
      (double) resource->rows);
  (void) FormatMagickSize(GetMagickCacheResourceExtent(resource),MagickTrue,
    "B",MagickPathExtent,extent);
  (void) GetMagickUTCTime(&resource->timestamp,&timestamp);
  (void) strftime(iso8601,sizeof(iso8601),"%FT%TZ",&timestamp);
  expired=' ';
  if ((resource->ttl != 0) && ((resource->timestamp+resource->ttl) < time(0)))
    expired='*';
  (void) fprintf(file,"%s%s %s %g:%g:%g:%g%c %s\n",GetMagickCacheResourceIRI(
    resource),size,extent,(double) (resource->ttl/(3600*24)),(double)
    ((resource->ttl % (24*3600))/3600),(double) ((resource->ttl % 3600)/60),
    (double) ((resource->ttl % 3600) % 60),expired,iso8601);
  path=DestroyString(path);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M a g i c k C a c h e R e s o u r c e E x p i r e d                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickCacheResourceExpired() returns MagickTrue if the resource creation
%  date plus the time to live is greater than or equal to the current time.
%  Note, a resource with a time to live of zero, never expires.
%
%  The format of the IsMagickCacheResourceExpired method is:
%
%      MagickBooleanType IsMagickCacheResourceExpired(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType IsMagickCacheResourceExpired(MagickCache *cache,
  MagickCacheResource *resource)
{
  MagickBooleanType
    status;

  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(status);
  if ((resource->ttl != 0) && ((resource->timestamp+resource->ttl) < time(0)))
    {
      errno=ESTALE;
      return(MagickTrue);
    }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I t e r a t e M a g i c k C a c h e R e s o u r c e s                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IterateMagickCacheResources() iterates over all the resources in the
%  MagickCache and calls the callback() method once for each resource
%  discovered with three arguments: the MagickCache, the current resource, and
%  a user context.  Use the resource `get` methods to retrieve any associated
%  metadata such as the IRI or time-to-live.  To terminate the iteration, have
%  callback() return MagickFalse.
%
%  The format of the IterateMagickCacheResources method is:
%
%      MagickBooleanType IterateMagickCacheResources(MagickCache *cache,
%        const char *iri,MagickBooleanType (*callback)(MagickCache *cache,
%        MagickCacheResource *resource,const void *context))
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType IterateMagickCacheResources(MagickCache *cache,
  const char *iri,const void *context,MagickBooleanType (*callback)(
  MagickCache *cache,MagickCacheResource *resource,const void *context))
{
  char
    *path;

  DIR
    *directory;

  MagickBooleanType
    status;

  struct dirent
    *entry;

  struct ResourceNode
    *head,
    *node,
    *p,
    *q;

  struct stat
    attributes;

  /*
    Check that resource id exists in MagickCache.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  status=MagickTrue;
  head=(struct ResourceNode *) AcquireCriticalMemory(sizeof(*node));
  (void) memset(head,0,sizeof(*head));
  head->path=AcquireString(cache->path);
  (void) ConcatenateString(&head->path,"/");
  (void) ConcatenateString(&head->path,iri);
  head->next=(struct ResourceNode *) NULL;
  head->previous=(struct ResourceNode *) NULL;
  q=head;
  for (p=head; p != (struct ResourceNode *) NULL; p=p->next)
  {
    directory=opendir(p->path);
    if (directory == (DIR *) NULL)
      {
        status=MagickFalse;
        break;
      }
    while ((entry=readdir(directory)) != (struct dirent *) NULL)
    {
      path=AcquireString(p->path);
      (void) ConcatenateString(&path,"/");
      (void) ConcatenateString(&path,entry->d_name);
      if (GetPathAttributes(path,&attributes) <= 0)
        {
          path=DestroyString(path);
          break;
        }
      if ((strcmp(entry->d_name, ".") == 0) ||
          (strcmp(entry->d_name, "..") == 0))
        {
          path=DestroyString(path);
          continue;
        }
      if (S_ISDIR(attributes.st_mode) != 0)
        {
          node=(struct ResourceNode *) AcquireCriticalMemory(sizeof(*node));
          (void) memset(node,0,sizeof(*node));
          node->path=path;
          node->next=(struct ResourceNode *) NULL;
          node->previous=q;
          q->next=node;
          q=q->next;
        }
      else
        if (S_ISREG(attributes.st_mode) != 0)
          {
            char *sentinel = ConstantString(path);
            GetPathComponent(path,TailPath,sentinel);
            if (strcmp(sentinel,MagickCacheResourceSentinel) == 0)
              {
                MagickCacheResource
                  *resource;

                GetPathComponent(path,HeadPath,path);
                resource=AcquireMagickCacheResource(cache,path+
                  strlen(cache->path)+1);
                status=GetMagickCacheResource(cache,resource);
                if (status != MagickFalse)
                  {
                    status=callback(cache,resource,context);
                    if (status == MagickFalse)
                      {
                        path=DestroyString(path);
                        resource=DestroyMagickCacheResource(resource);
                        sentinel=DestroyString(sentinel);
                        break;
                      }
                  }
                resource=DestroyMagickCacheResource(resource);
              }
            path=DestroyString(path);
            sentinel=DestroyString(sentinel);
          }
    }
    (void) closedir(directory);
  }
  /*
    Free resources.
  */
  for ( ; q != (struct ResourceNode *) NULL; )
  {
    node=q;
    q=q->previous;
    node->path=DestroyString(node->path);
    node=(struct ResourceNode *) RelinquishMagickMemory(node);
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P u t M a g i c k C a c h e R e s o u r c e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PutMagickCacheResource() puts an image, image sequence, video, or metadata
%  resource in the MagickCache identified by its IRI.  If the IRI already
%  exists, an exception is returned.
%
%  The format of the PutMagickCacheResource method is:
%
%      MagickBooleanType PutMagickCacheResource(MagickCache *cache,
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
%    o blob: the blob.
%
*/

static StringInfo *SetMagickCacheResourceSentinel(
  const MagickCacheResource *resource)
{
  StringInfo
    *meta;

  unsigned char
    *p;

  unsigned int
    signature;

  /*
    Set the MagickCache resource sentinel.
  */
  meta=AcquireStringInfo(MagickPathExtent);
  p=GetStringInfoDatum(meta);
  signature=GetMagickCacheSignature(resource->nonce);
  (void) memcpy(p,&signature,sizeof(signature));
  p+=sizeof(signature);
  (void) memcpy(p,GetStringInfoDatum(resource->nonce),
    GetStringInfoLength(resource->nonce));
  p+=GetStringInfoLength(resource->nonce);
  (void) memcpy(p,&resource->ttl,sizeof(resource->ttl));
  p+=sizeof(resource->ttl);
  (void) memcpy(p,&resource->columns,sizeof(resource->columns));
  p+=sizeof(resource->columns);
  (void) memcpy(p,&resource->rows,sizeof(resource->rows));
  p+=sizeof(resource->rows);
  (void) memcpy(p,resource->id,MagickCacheDigestExtent);
  p+=MagickCacheDigestExtent;
  SetStringInfoLength(meta,(size_t) (p-GetStringInfoDatum(meta)));
  return(meta);
}

MagickExport MagickBooleanType PutMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  MagickBooleanType
    status;

  StringInfo
    *meta;

  /*
    Create the resource path as defined by the IRI.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status != MagickFalse)
    {
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot overwrite resource","`%s'",resource->iri);
      return(MagickFalse);
    }
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  if (MagickCreatePath(path) == MagickFalse)
    {
      path=DestroyString(path);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"cannot put resource","`%s'",path);
      return(MagickFalse);
    }
  /*
    Export the MagickCache resource metadata.
  */
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,MagickCacheResourceSentinel);
  if (IsPathAccessible(path) != MagickFalse)
    {
      path=DestroyString(path);
      errno=EEXIST;
      return(MagickFalse);
    }
  SetMagickCacheResourceID(cache,resource);
  meta=SetMagickCacheResourceSentinel(resource);
  status=BlobToFile(path,GetStringInfoDatum(meta),GetStringInfoLength(meta),
    resource->exception);
  meta=DestroyStringInfo(meta);
  path=DestroyString(path);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P u t M a g i c k C a c h e R e s o u r c e B l o b                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PutMagickCacheResource() puts a blob resource in the MagickCache identified
%  by its IRI.  If the IRI already exists, an exception is returned.
%
%  The format of the PutMagickCacheResource method is:
%
%      MagickBooleanType PutMagickCacheResourceBlob(MagickCache *cache,
%        const MagickCacheResource *resource,const size_t extent,
%        const void blob)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
%    o blob: the blob.
%
*/
MagickExport MagickBooleanType PutMagickCacheResourceBlob(MagickCache *cache,
  MagickCacheResource *resource,const size_t extent,const void *blob)
{
  char
    *path;

  MagickBooleanType
    status;

  /*
    Puts a blob resource in the MagickCache identified by its IRI.
  */
  status=PutMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(MagickFalse);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  status=BlobToFile(path,blob,extent,cache->exception);
  path=DestroyString(path);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P u t M a g i c k C a c h e R e s o u r c e I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PutMagickCacheResourceImage() puts an image resource in the MagickCache
%  identified by its IRI.  If the IRI already exists, an exception is returned.
%
%  The format of the PutMagickCacheResourceImage method is:
%
%      MagickBooleanType PutMagickCacheResourceImage(MagickCache *cache,
%        const MagickCacheResource *resource,const Image *image)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
%    o image: the image.
%
*/
MagickExport MagickBooleanType PutMagickCacheResourceImage(MagickCache *cache,
  MagickCacheResource *resource,const Image *image)
{
  char
    *path;

  Image
    *images;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

  /*
    Puts an image resource in the MagickCache identified by its IRI.
  */
  resource->columns=image->columns;
  resource->rows=image->rows;
  status=PutMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(status);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  if (MagickCreatePath(path) == MagickFalse)
    return(MagickFalse);
  image_info=AcquireImageInfo();
  images=CloneImageList(image,resource->exception);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  (void) FormatLocaleString(images->filename,MagickPathExtent,"mpc:%s",path);
  path=DestroyString(path);
  status=WriteImages(image_info,images,images->filename,resource->exception);
  images=DestroyImageList(images);
  image_info=DestroyImageInfo(image_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P u t M a g i c k C a c h e R e s o u r c e M e t a                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PutMagickCacheResourceMeta() puts metadata in the MagickCache identified by
%  its IRI.  If the IRI already exists, an exception is returned.
%
%  The format of the PutMagickCacheResourceMeta method is:
%
%      MagickBooleanType PutMagickCacheResourceMeta(MagickCache *cache,
%        const MagickCacheResource *resource,const char *properties)
%
%  A description of each parameter follows:
%
%    o cache: the cache repository.
%
%    o resource: the resource.
%
%    o properties: the properties.
%
*/
MagickExport MagickBooleanType PutMagickCacheResourceMeta(MagickCache *cache,
  MagickCacheResource *resource,const char *properties)
{
  char
    *path;

  MagickBooleanType
    status;

  /*
    Puts resource meta in the MagickCache identified by its IRI.
  */
  status=PutMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(MagickFalse);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  status=BlobToFile(path,properties,strlen(properties)+1,cache->exception);
  path=DestroyString(path);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k C a c h e R e s o u r c e I R I                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickCacheResourceIRI() associates an IRI with a resource.
%
%  The format of the SetMagickCacheResourceIRI method is:
%
%      MagickBooleanType SetMagickCacheResourceIRI(MagickCache *cache,
%        MagickCacheResource *resource,const char *iri)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType SetMagickCacheResourceIRI(MagickCache *cache,
  MagickCacheResource *resource,const char *iri)
{
  char
    *path,
    *p;

  /*
    Parse the IRI into its components: project / type / resource-path.
  */
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  if (resource->iri != (char *) NULL)
    resource->iri=DestroyString(resource->iri);
  resource->iri=ConstantString(iri);
  path=ConstantString(iri);
  p=strtok(path,"/");
  if (p == (char *) NULL)
    return(MagickFalse);
  if (resource->project != (char *) NULL)
    resource->project=DestroyString(resource->project);
  resource->project=ConstantString(p);
  p=strtok(NULL,"/");
  if (p == (char *) NULL)
    return(MagickFalse);
  if (resource->type != (char *) NULL)
    resource->type=DestroyString(resource->type);
  resource->type=ConstantString(p);
  path=DestroyString(path);
  if (resource->id != (char *) NULL)
    resource->id=DestroyString(resource->id);
  resource->id=ConstantString("");
  if (LocaleCompare(resource->type,"*") == 0)
    resource->resource_type=WildResourceType;
  else
    if (LocaleCompare(resource->type,"blob") == 0)
      resource->resource_type=BlobResourceType;
    else
      if (LocaleCompare(resource->type,"image") == 0)
        resource->resource_type=ImageResourceType;
      else
        if (LocaleCompare(resource->type,"meta") == 0)
          resource->resource_type=MetaResourceType;
        else
          {
            (void) ThrowMagickException(resource->exception,GetMagickModule(),
              CacheError,"unknown resource type","`%s'",resource->type);
            return(MagickFalse);
          }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k C a c h e R e s o u r c e T T L                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickCacheResourceTTL() associates the time to live with a resource in
%  the cache respository.
%
%  The format of the SetMagickCacheResourceTTL method is:
%
%      void SetMagickCacheResourceTTL(MagickCacheResource *resource,
%        const size_t ttl)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
%    o ttl: the time to live.
%
*/
MagickExport void SetMagickCacheResourceTTL(MagickCacheResource *resource,
  const size_t ttl)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  resource->ttl=ttl;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k C a c h e R e s o u r c e V e r s i o n                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickCacheResourceVersion() associates the API version with a resource.
%
%  The format of the SetMagickCacheResourceVersion method is:
%
%      MagickBooleanType SetMagickCacheResourceVersion(
%        MagickCacheResource *resource,const size_t version)
%
%  A description of each parameter follows:
%
%    o resource: the resource.
%
%    o version: the version.
%
*/
MagickExport MagickBooleanType SetMagickCacheResourceVersion(
  MagickCacheResource *resource,const size_t version)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  resource->version=version;
  return(MagickTrue);
}
