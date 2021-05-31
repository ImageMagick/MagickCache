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
%                       Magick Image Cache CRUD Methods                       %
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
%  The Magick image cache stores and retrieves images efficiently within
%  milliseconds with a small memory footprint making it suitable as a web
%  image service.
%
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <MagickCore/MagickCore.h>
#include "MagickCache/MagickCache.h"
#include "MagickCache/magick-cache-private.h"

/*
  MagickCache defines.
*/
#define MagickCacheSentinel  ".magick-cache"
#define MagickCacheAPIVersion  1
#define MagickCacheMax(x,y)  (((x) > (y)) ? (x) : (y))
#define MagickCacheMin(x,y)  (((x) < (y)) ? (x) : (y))
#define MagickCacheNonce  "MagickCache"
#define MagickCacheNonceExtent  8
#define MagickCacheSignature  0xabacadabU
#define MagickCacheResourceSentinel  ".magick-cache-resource"
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

  time_t
    timestamp;

  RandomInfo
    *random_info;

  MagickBooleanType
    debug;

  ExceptionInfo
    *exception;

  StringInfo
    *nonce,
    *key;

  size_t
    signature;
};

struct _MagickCacheResource
{
  char
    *iri,
    *project,
    *type,
    *id;

  MagickCacheResourceType
    resource_type;

  time_t
    timestamp,
    ttl;

  size_t
    extent,
    version;

  StringInfo
    *nonce;

  MagickBooleanType
    debug;

  ExceptionInfo
    *exception;

  size_t
    signature;
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
%  AcquireMagickCache() creates a cache structure for getting or putting
%  resources from or to the magick cache repository.  NULL is returned if
%  the magick cache repo is not found or if the repo is not compatible with
%  the API version.
%
%  The format of the AcquireMagickCache method is:
%
%      MagickCache *AcquireMagickCache(const char *path)
%
%  A description of each parameter follows:
%
%    o path: the magick cache path, absolute (e.g. /myrepo) or relative (e.g.
%      ./myrepo).
%
*/

static void GetMagickCacheSentinel(MagickCache *cache,unsigned char *sentinel)
{
  unsigned char
    *p;

  unsigned int
    signature;

  p=sentinel;
  p+=sizeof(signature);
  (void) memcpy(GetStringInfoDatum(cache->nonce),p,
    GetStringInfoLength(cache->nonce));
  p+=GetStringInfoLength(cache->nonce);
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
    Create a magick cache signature.
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

MagickExport MagickCache *AcquireMagickCache(const char *path)
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
    Basic sanity check passes, allocate a magick cache struture.
  */
  cache=(MagickCache *) AcquireCriticalMemory(sizeof(*cache));
  (void) memset(cache,0,sizeof(*cache));
  cache->path=ConstantString(path);
  cache->timestamp=(time_t) attributes.st_ctime;
  cache->random_info=AcquireRandomInfo();
  cache->nonce=GetRandomKey(cache->random_info,MagickCacheNonceExtent);
  cache->key=AcquireStringInfo(0);
  cache->exception=AcquireExceptionInfo();
  cache->debug=IsEventLogging();
  cache->signature=MagickCacheSignature;
  /*
    Validate the magick cache sentinel.
  */
  sentinel_path=AcquireString(path);
  (void) ConcatenateString(&sentinel_path,"/");
  (void) ConcatenateString(&sentinel_path,MagickCacheSentinel);
  sentinel=FileToBlob(sentinel_path,~0UL,&extent,cache->exception);
  if (sentinel == NULL)
    {
      cache=DestroyMagickCache(cache);
      return((MagickCache *) NULL);
    }
  GetMagickCacheSentinel(cache,sentinel);
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
%  It is required to get and set metadata associated with resource content.
%
%  The format of the AcquireMagickCacheResource method is:
%
%      MagickCacheResource *AcquireMagickCacheResource(MagickCache *,
%        const char *iri)
%
%  A description of each parameter follows:
%
%    o iri: the IRI.
%
*/
MagickExport MagickCacheResource *AcquireMagickCacheResource(
  MagickCache *cache,const char *iri)
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
%    o cache: the magick cache.
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
%   C r e a t e M a g i c k C a c h e                                         %
%                                                                             %
%                                                                             % %                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CreateMagickCache() creates a magick cache repository suitably prepared for
%  storing and retrieving blob, image, and metadata resources.
%
%  The format of the CreateMagickCache method is:
%
%      MagickBooleanType CreateMagickCache(const char *path)
%
%  A description of each parameter follows:
%
%    o path: the magick cache directory path, absolute (e.g. /myrepo) or
%      relative (e.g. ./myrepo).
%
*/

static StringInfo *SetMagickCacheSentinel(void)
{
  RandomInfo
    *random_info;

  StringInfo
    *key_info,
    *meta;

  unsigned char
    *p;

  unsigned int
    signature;

  meta=AcquireStringInfo(MagickPathExtent);
  random_info=AcquireRandomInfo();
  key_info=GetRandomKey(random_info,MagickCacheNonceExtent);
  p=GetStringInfoDatum(meta);
  signature=GetMagickCacheSignature(key_info);
  (void) memcpy(p,&signature,sizeof(signature));
  p+=sizeof(signature);
  (void) memcpy(p,GetStringInfoDatum(key_info),MagickCacheNonceExtent);
  p+=MagickCacheNonceExtent;
  SetStringInfoLength(meta,(size_t) (p-GetStringInfoDatum(meta)));
  key_info=DestroyStringInfo(key_info);
  random_info=DestroyRandomInfo(random_info);
  return(meta);
}

MagickExport MagickBooleanType CreateMagickCache(const char *path)
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
    Create the magick cache path.
  */
  if (MagickCreatePath(path) == MagickFalse)
    return(MagickFalse);
  /*
    Create the magick cache sentinel.
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
  meta=SetMagickCacheSentinel();
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
%  DeleteMagickCacheResourceException() deletes a resource within the magick
%  cache.
%
%  The format of the DeleteMagickCacheResource method is:
%
%      MagickBooleanType DeleteMagickCacheResource(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o resource: the magick cache resource.
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
    Check that resource id exists in magick cache.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  status=GetMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(MagickFalse);
  /*
    Delete resource id in magick cache.
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
    Delete resource id sentinel in magick cache.
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
    Delete resource id IRI in magick cache.
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
%    o cache: the magick cache.
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
  if (cache->random_info != (RandomInfo *) NULL)
    cache->random_info=DestroyRandomInfo(cache->random_info);
  if (cache->key != (StringInfo *) NULL )
    cache->key=DestroyStringInfo(cache->key);
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
%  DestroyMagickCacheResource() deallocates memory associated with an
%  MagickCacheResource structure.
%
%  The format of the DestroyMagickCacheResource method is:
%
%      MagickCacheResource *DestroyMagickCacheResource(
%        MagickCacheResource *resource_info)
%
%  A description of each parameter follows:
%
%    o resource_info: the image info.
%
*/
MagickExport MagickCacheResource *DestroyMagickCacheResource(
  MagickCacheResource *resource)
{
  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  if (resource->nonce != (StringInfo *) NULL )
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
%   E x p i r e M a g i c k C a c h e R e s o u r c e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpireMagickCacheResource() deletes a resource within the magick
%  cache if it has expired, i.e. the resource creation date plus the time
%  to live is less than the current time.  Note, a resource with a time to
%  live of zero never expires.
%
%  The format of the ExpireMagickCacheResource method is:
%
%      MagickBooleanType ExpireMagickCacheResource(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType ExpireMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  MagickBooleanType
    status;

  /*
    If the resource ID exists and has expired, delete it.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  path=AcquireString(cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  status=GetMagickCacheResource(cache,resource);
  if ((status != MagickFalse) && (resource->ttl != 0) &&
      ((resource->timestamp+resource->ttl) < time(0)))
    status=DeleteMagickCacheResource(cache,resource);
  path=DestroyString(path);
  return(status);
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
%  GetMagickCacheException() return the exception associated with the image
%  cache.
%
%  The format of the GetMagickCacheException method is:
%
%      ExceptionInfo *GetMagickCacheException(const MagickCache *cache)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
*/
MagickExport ExceptionInfo *GetMagickCacheException(const MagickCache *cache)
{
  assert(cache != (const MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  return(cache->exception);
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
%    o cache: the magick cache.
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

  p=sentinel;
  p+=sizeof(signature);
  (void) memcpy(GetStringInfoDatum(resource->nonce),p,
    GetStringInfoLength(resource->nonce));
  p+=GetStringInfoLength(resource->nonce);
  (void) memcpy(&resource->ttl,p,sizeof(resource->ttl));
  p+=sizeof(resource->ttl);
}

static void SetMagickCacheResourceID(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *digest;

  StringInfo
    *signature;

  signature=StringToStringInfo(resource->iri);
  ConcatenateStringInfo(signature,resource->nonce);
  ConcatenateStringInfo(signature,cache->key);
  ConcatenateStringInfo(signature,cache->nonce);
  digest=StringInfoToDigest(signature);
  signature=DestroyStringInfo(signature);
  if (digest != (char *) NULL)
    {
      resource->id=ConstantString(digest);
      digest=DestroyString(digest);
    }
}

MagickExport MagickBooleanType GetMagickCacheResource(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  size_t
    extent;

  struct stat
    attributes;

  unsigned int
    signature;

  void
    *sentinel;

  /*
    Validate the magick cache resource sentinel.
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
  GetMagickCacheResourceSentinel(resource,sentinel);
  SetMagickCacheResourceID(cache,resource);
  signature=GetMagickCacheSignature(resource->nonce);
  if (memcmp(&signature,sentinel,sizeof(signature)) != 0)
    {
      sentinel=RelinquishMagickMemory(sentinel);
      (void) ThrowMagickException(resource->exception,GetMagickModule(),
        CacheError,"resource sentinel signature mismatch","`%s'",path);
      return(MagickFalse);
    }
  sentinel=RelinquishMagickMemory(sentinel);
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
  path=DestroyString(path);
  resource->timestamp=(time_t) attributes.st_ctime;
  resource->extent=(size_t) attributes.st_size;
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
%  GetMagickCacheResourceBlob() gets a resource identified by its IRI in the
%  MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceBlob method is:
%
%      void *GetMagickCacheResourceBlob(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o resource: the resource.
%
*/
MagickExport void *GetMagickCacheResourceBlob(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  MagickBooleanType
    status;

  void
    *blob;

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
  blob=FileToBlob(path,resource->extent,&resource->extent,resource->exception);
  path=DestroyString(path);
  return(blob);
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
%  GetMagickCacheResourceException() returns the exception associated with
%  the magick cache resource.
%
%  The format of the GetMagickCacheResourceException method is:
%
%      ExceptionInfo *GetMagickCacheResourceException(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: the magick cache resource.
%
*/
MagickExport ExceptionInfo *GetMagickCacheResourceException(
  const MagickCacheResource *resource)
{
  assert(resource != (const MagickCacheResource *) NULL);
  assert(resource->signature == MagickCacheSignature);
  return(resource->exception);
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
%  GetMagickCacheResourceExtent() returns the extent of the resource blob,
%  image, or meta associated with the MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceExtent method is:
%
%      const size_t GetMagickCacheResourceExtent(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const size_t GetMagickCacheResourceExtent(
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
%    o cache: the magick cache.
%
%    o length: the length of the ID.
%
%    o id: a buffer that is at least `length` bytes long.
%
*/

static inline MagickBooleanType IsUTFValid(int code)
{
  int
    mask;

  mask=(int) 0x7fffffff;
  if (((code & ~mask) != 0) && ((code < 0xd800) || (code > 0xdfff)) &&
      (code != 0xfffe) && (code != 0xffff))
    return(MagickFalse);
  return(MagickTrue);
}

MagickExport MagickBooleanType GetMagickCacheResourceID(MagickCache *cache,
  const size_t length,char *id)
{
  ssize_t
    i,
    j;

  StringInfo
    *key;

  unsigned char
    *code;

  assert(cache != (const MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  for (j=0; j < (ssize_t) length; j++)
  {
    key=GetRandomKey(cache->random_info,length);
    if (key == (StringInfo *) NULL)
      return(MagickFalse);
    code=GetStringInfoDatum(key);
    for (i=0; i < (ssize_t) length; i++)
    {
      if ((code[i] <= 32) || ((code[i] <= 0x9f && code[i] > 0x7F)))
        continue;
      id[j++]=(int) code[i];
      if (j == (ssize_t) length)
        break;
    }
    key=DestroyStringInfo(key);
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
%  GetMagickCacheResourceImage() gets a resource identified by its IRI in the
%  MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceImage method is:
%
%      Image *GetMagickCacheResourceImage(MagickCache *cache,
%        MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o resource: the resource.
%
*/
MagickExport Image *GetMagickCacheResourceImage(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *path;

  ExceptionInfo
    *exception;

  Image
    *image;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

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
  if (strlen(path) > (MagickPathExtent-2))
    {
      path=DestroyString(path);
      errno=ENAMETOOLONG;
      return((Image *) NULL);
    }
  image_info=AcquireImageInfo();
  (void) strcpy(image_info->filename,path);
  (void) strcpy(image_info->magick,"MPC");
  exception=AcquireExceptionInfo();
  image=ReadImage(image_info,exception);
  exception=DestroyExceptionInfo(exception);
  if (image == (Image *) NULL)
    (void) ThrowMagickException(resource->exception,GetMagickModule(),
      CacheError,"cannot get resource","`%s'",resource->iri);
  path=DestroyString(path);
  image_info=DestroyImageInfo(image_info);
  return(image);
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
%  MagickCacheResource structure.
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
MagickExport const char *GetMagickCacheResourceIRI(
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
%  GetMagickCacheResourceMeta() gets any metadata associated with the resource.
%
%  The format of the GetMagickCacheResourceMeta method is:
%
%      char *GetMagickCacheResourceMeta(MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o resource: the resource.
%
*/
MagickExport char *GetMagickCacheResourceMeta(MagickCache *cache,
  MagickCacheResource *resource)
{
  char
    *meta,
    *path;

  MagickBooleanType
    status;

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
  meta=(char *) FileToBlob(path,resource->extent,&resource->extent,
    resource->exception);
  path=DestroyString(path);
  return(meta);
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
%  GetMagickCacheResourceTimestamp() returns the timestamp associated with the
%  resource.
%
%  The format of the GetMagickCacheResourceTimestamp method is:
%
%      const time_t GetMagickCacheResourceTimestamp(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const time_t GetMagickCacheResourceTimestamp(
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
%  GetMagickCacheResourceTTL() returns the time-to-live associated with the
%  MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceTTL method is:
%
%      const size_t GetMagickCacheResourceTTL(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const size_t GetMagickCacheResourceTTL(
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
%  GetMagickCacheResourceType() returns the type associated with the
%  MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceType method is:
%
%      const MagickCacheResourceType GetMagickCacheResourceType(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const MagickCacheResourceType GetMagickCacheResourceType(
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
%  GetMagickCacheResourceVersion() returns the version associated with the
%  MagickCacheResource structure.
%
%  The format of the GetMagickCacheResourceVersion method is:
%
%      const size_t GetMagickCacheResourceVersion(
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const size_t GetMagickCacheResourceVersion(
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
%  GetMagickCacheTimestamp() returns the timestamp associated with the magick
%  cache.
%
%  The format of the GetMagickCacheResource method is:
%
%      const time_t GetMagickCacheResourceTimestamp(const MagickCache *cache)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
*/
MagickExport const time_t GetMagickCacheTimestamp(const MagickCache *cache)
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
%   I t e r a t e M a g i c k C a c h e R e s o u r c e s                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IterateMagickCacheResources() iterates over all the resources in the magick
%  cache and calls callback() once for each resource with three arguments: the
%  magick cache, the current resource, and a user context.  Use the resource
%  `get` methods to retrieve any associate metadata such as the IRI or
%  time-to-live.  To terminate the iterating, callback() can return MagickFalse.
%
%  The format of the IterateMagickCacheResources method is:
%
%      MagickBooleanrType IterateMagickCacheResources(MagickCache *cache,
%        const char *iri,MagickBooleanType (*callback)(MagickCache *cache,
%        MagickCacheResource *resource,const void *context))
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType IterateMagickCacheResources(MagickCache *cache,
  const char *iri,const void *context,MagickBooleanType (*callback)(
    MagickCache *cache,MagickCacheResource *resource,const void *context))
{
  struct ResourceNode
  {
    char
      *path;

    struct ResourceNode
      *next;
  };

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
    Check that resource id exists in magick cache.
  */
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCacheSignature);
  status=MagickTrue;
  head=AcquireCriticalMemory(sizeof(struct ResourceNode));
  head->path=AcquireString(cache->path);
  (void) ConcatenateString(&head->path,"/");
  (void) ConcatenateString(&head->path,iri);
  head->next=(struct ResourceNode *) NULL;
  q=head;
  for (p=head; p != (struct ResourceNode *) NULL; p=p->next)
  {
    directory=opendir(p->path);
    if (directory == (DIR *) NULL)
      return(-1);
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
        continue;
      if (S_ISDIR(attributes.st_mode) != 0)
        {
          node=AcquireCriticalMemory(sizeof(struct ResourceNode));
          node->path=ConstantString(path);
          node->next=(struct ResourceNode *) NULL;
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
                  status=callback(cache,resource,context);
                resource=DestroyMagickCacheResource(resource);
                if (status == MagickFalse)
                  {
                    sentinel=DestroyString(sentinel);
                    break;
                  }
              }
            sentinel=DestroyString(sentinel);
          }
      path=DestroyString(path);
    }
    (void) closedir(directory);
  }
  /*
    Free resources.
  */
  for (p=head; p != (struct ResourceNode *) NULL; )
  {
    node=p;
    p=p->next;
    if (node->path != (char *) NULL)
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
%  PutMagickCacheResource() puts a resource in the magick cache identified by
%  its IRI.  If the IRI already exists, the previous one is replaced.
%
%  The format of the PutMagickCacheResource method is:
%
%      MagickBooleanType PutMagickCacheResource(MagickCache *cache,
%        const MagickCacheResource *resource)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
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
    reate the path as defined by the IRI.
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
    Export magick cache resource meta info.
  */
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,MagickCacheResourceSentinel);
  if (IsPathAccessible(path) != MagickFalse)
    {
      path=DestroyString(path);
      errno=EEXIST;
      return(MagickFalse);
    }
  meta=SetMagickCacheResourceSentinel(resource);
  SetMagickCacheResourceID(cache,resource);
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
%  PutMagickCacheResource() puts a blob resource in the magick cache identified
%  by its IRI.  If the IRI already exists, the previous one is replaced.
%
%  The format of the PutMagickCacheResource method is:
%
%      MagickBooleanType PutMagickCacheResourceBlob(MagickCache *cache,
%        const MagickCacheResource *resource,const size_t extent,
%        const void blob)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
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
    Puts a blob resource in the magick cache identified by its IRI.
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
%  PutMagickCacheResourceImage() puts an image resource in the magick cache
%  identified by its IRI.  If the IRI already exists, the previous one is
%  replaced.
%
%  The format of the PutMagickCacheResourceImage method is:
%
%      MagickBooleanType PutMagickCacheResourceImage(MagickCache *cache,
%        const MagickCacheResource *resource,const Image *image)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
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
    Puts an image resource in the magick cache identified by its IRI.
  */
  status=PutMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    return(status);
  path=AcquireString("mpc:");
  (void) ConcatenateString(&path,cache->path);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->iri);
  if (MagickCreatePath(path) == MagickFalse)
    return(MagickFalse);
  image_info=AcquireImageInfo();
  images=CloneImageList(image,resource->exception);
  (void) ConcatenateString(&path,"/");
  (void) ConcatenateString(&path,resource->id);
  (void) strcpy(images->filename,path);
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
%  PutMagickCacheResourceMeta() puts resource meta in the magick cache
%  identified by its IRI.  If the IRI already exists, the previous one is
%  replaced.
%
%  The format of the PutMagickCacheResourceMeta method is:
%
%      MagickBooleanType PutMagickCacheResourceMeta(MagickCache *cache,
%        const MagickCacheResource *resource,const char *properties)
%
%  A description of each parameter follows:
%
%    o cache: the magick cache.
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
    Puts resource meta in the magick cache identified by its IRI.
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
%   S e t M a g i c k C a c h e K e y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickCacheKey() associates an API key with the magick-cache.
%
%  The format of the SetMagickCacheKey method is:
%
%      SetMagickCacheKey(MagickCache *cache,const char *key)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
%    o key: the API key.
%
*/
MagickExport void SetMagickCacheKey(MagickCache *cache,const char *key)
{
  assert(cache != (MagickCache *) NULL);
  assert(cache->signature == MagickCoreSignature);
  if (key == (const char *) NULL)
    return;
  if (cache->key != (StringInfo *) NULL )
    cache->key=DestroyStringInfo(cache->key);
  cache->key=StringToStringInfo(key);
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
%  SetMagickCacheResourceIRI() associates the IRI with the resource.
%
%  The format of the SetMagickCacheResourceIRI method is:
%
%      MagickBooleanType SetMagickCacheResourceIRI(MagickCache *cache,
%        MagickCacheResource *resource,const char *iri)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
%
%    o iri: the IRI.
%
*/
MagickExport MagickBooleanType SetMagickCacheResourceIRI(MagickCache *cache,
  MagickCacheResource *resource,const char *iri)
{
  char
    *p;

  assert(resource != (MagickCacheResource *) NULL);
  assert(resource->signature == MagickCoreSignature);
  resource->iri=ConstantString(iri);
  resource->project=ConstantString(iri);
  p=strtok(resource->project,"/");
  if (p == (char *) NULL)
    return(MagickFalse);
  resource->project=ConstantString(p);
  p=strtok(NULL,"/");
  if (p == (char *) NULL)
    return(MagickFalse);
  resource->id=ConstantString("");
  resource->type=ConstantString(p);
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
%  SetMagickCacheResourceTTL() associates the time to live with the resource.
%
%  The format of the SetMagickCacheResourceTTL method is:
%
%      SetMagickCacheResourceTTL(MagickCacheResource *resource,const size_t ttl)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
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
%  SetMagickCacheResourceVersion() associates the version with the resource.
%
%  The format of the SetMagickCacheResourceVersion method is:
%
%      MagickBooleanType SetMagickCacheResourceVersion(
%        MagickCacheResource *resource,const size_t version)
%
%  A description of each parameter follows:
%
%    o resource: a pointer to a MagickCacheResource structure.
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