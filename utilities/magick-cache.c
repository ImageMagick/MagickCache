/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                 M   M   AAA    GGGG  IIIII   CCCC  K   K                    %
%                 MM MM  A   A  G        I    C      K  K                     %
%                 M M M  AAAAA  G GGG    I    C      KKK                      %
%                 M   M  A   A  G   G    I    C      K  K                     %
%                 M   M  A   A   GGGG  IIIII   CCCC  K   K                    %
%                                                                             %
%                      CCCC   AAA    CCCC  H   H  EEEEE                       %
%                     C      A   A  C      H   H  E                           %
%                     C      AAAAA  C      HHHHH  EEE                         %
%                     C      A   A  C      H   H  E                           %
%                      CCCC  A   A   CCCC  H   H  EEEEE                       %
%                                                                             %
%                 CLI Interface to the MagickCache Repository                 %
%                                                                             %
%                             Software Design                                 %
%                                 Cristy                                      %
%                               March 2021                                    %
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

#include "MagickCore/studio.h"
#include "MagickCache/MagickCache.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  M a i n                                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

static MagickBooleanType DeleteResources(MagickCache *cache,
  MagickCacheResource *resource,const void *context)
{
  MagickBooleanType
    status;

  ssize_t
    *count = (ssize_t *) context;

  status=DeleteMagickCacheResource(cache,resource);
  if (status != MagickFalse)
    (*count)++;
  return(status);
}

static MagickBooleanType ExpireResources(MagickCache *cache,
  MagickCacheResource *resource,const void *context)
{
  MagickBooleanType
    status;

  ssize_t
    *count = (ssize_t *) context;

  status=IsMagickCacheResourceExpired(cache,resource);
  if (status != MagickFalse)
    {
      (*count)++;
      return(DeleteMagickCacheResource(cache,resource));
    }
  return(MagickTrue);
}

static MagickBooleanType IdentifyResources(MagickCache *cache,
  MagickCacheResource *resource,const void *context)
{
  MagickBooleanType
    status;

  ssize_t
    *count = (ssize_t *) context;

  status=IdentifyMagickCacheResource(cache,resource,stdout);
  if (status != MagickFalse)
    (*count)++;
  return(status);
}

static void MagickCacheUsage(int argc,char **argv)
{
  (void) fprintf(stdout,"Version: %s\n",GetMagickCacheVersion((size_t *) NULL));
  (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCacheCopyright());
  (void) fprintf(stdout,"Usage: %s [-passkey filename] create path\n",*argv);
  (void) fprintf(stdout,"Usage: %s [-passkey filename] "
    "[delete | expire | identify] path iri\n",*argv);
  (void) fprintf(stdout,"Usage: %s [-passkey filename] [-passphrase filename]"
    " [-extract geometry] [-ttl seconds] get path iri filename\n",*argv);
  (void) fprintf(stdout,"Usage: %s [-passkey filename] [-passphrase filename]"
    " [-ttl seconds] put path iri filename\n",*argv);
  exit(0);
}

static MagickBooleanType MagickCacheCLI(int argc,char **argv,
  ExceptionInfo *exception)
{
#define MagickCacheExit(exception) \
{ \
  CatchException(exception); \
  if (passphrase != (StringInfo *) NULL) \
    passphrase=DestroyStringInfo(passphrase); \
  if (passkey != (StringInfo *) NULL) \
    passkey=DestroyStringInfo(passkey); \
  if (resource != (MagickCacheResource *) NULL) \
    resource=DestroyMagickCacheResource(resource); \
  if (cache != (MagickCache *) NULL) \
    cache=DestroyMagickCache(cache); \
  if (message != (char *) NULL) \
    message=DestroyString(message); \
  return(-1); \
}
#define ThrowMagickCacheException(cache) \
{ \
  description=GetMagickCacheException(cache,&severity); \
  (void) FormatLocaleFile(stderr,"%s %s %lu %s\n",GetMagickModule(), \
    description); \
  description=(char *) DestroyString(description); \
  MagickCacheExit(exception); \
}
#define ThrowMagickCacheResourceException(cache,resource) \
{ \
  ThrowMagickCacheException(cache); \
  description=GetMagickCacheResourceException(resource,&severity); \
  (void) FormatLocaleFile(stderr,"%s %s %lu %s\n",GetMagickModule(), \
    description); \
  description=(char *) DestroyString(description); \
  MagickCacheExit(exception); \
}

  char
    *description = (char *) NULL,
    *extract = (char *) NULL,
    *filename = (char *) NULL,
    *function,
    *iri,
    *message = (char *) NULL,
    *path;

  const char
    *meta;

  const Image
    *image;

  const void
    *blob;

  ExceptionType
    severity = UndefinedException;

  ImageInfo
    *image_info;

  int
    i = 1;

  MagickBooleanType
    status;

  MagickCache
    *cache = (MagickCache *) NULL;

  MagickCacheResource
    *resource = (MagickCacheResource *) NULL;

  MagickCacheResourceType
    type;

  size_t
    extent,
    ttl = 0;

  StringInfo
    *passkey = (StringInfo *) NULL,
    *passphrase = (StringInfo *) NULL;

  if (argc < 2)
    MagickCacheUsage(argc,argv);
  for ( ; i < (argc-1); i++)
  {
    if (*argv[i] != '-')
      break;
    if (LocaleCompare(argv[i],"-passkey") == 0)
      {
        /*
          Protect your cache repository and its resources with a passkey.
        */
        passkey=FileToStringInfo(argv[++i],~0UL,exception);
        if (passkey == (StringInfo *) NULL)
          {
            message=GetExceptionMessage(errno);
            (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
              "unable to read passkey","`%s': %s",argv[i],message);
            MagickCacheExit(exception);
          }
      }
    if (LocaleCompare(argv[i],"-passphrase") == 0)
      {
        /*
          Protect your image by scrambling its content.
        */
        passphrase=FileToStringInfo(argv[++i],~0UL,exception);
        if (passphrase == (StringInfo *) NULL)
          {
            message=GetExceptionMessage(errno);
            (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
              "unable to read passphrase","`%s': %s",argv[i],message);
            MagickCacheExit(exception);
          }
      }
    if (LocaleCompare(argv[i],"-ttl") == 0)
      {
        char
          *q;

        /*
          Time to live, absolute or relative, e.g. 1440, 2 hours, 3 days, ...
        */
        ttl=(size_t) InterpretLocaleValue(argv[++i],&q);
        if (q != argv[i])
          {
            while (isspace((int) ((unsigned char) *q)) != 0)
              q++;
            if (LocaleNCompare(q,"second",6) == 0)
              ttl*=1;
            if (LocaleNCompare(q,"minute",6) == 0)
              ttl*=60;
            if (LocaleNCompare(q,"hour",4) == 0)
              ttl*=3600;
            if (LocaleNCompare(q,"day",3) == 0)
              ttl*=86400;
            if (LocaleNCompare(q,"week",4) == 0)
              ttl*=604800;
            if (LocaleNCompare(q,"month",5) == 0)
              ttl*=2628000;
            if (LocaleNCompare(q,"year",4) == 0)
              ttl*=31536000;
          }
      }
    if (LocaleCompare(argv[i],"-extract") == 0)
      extract=argv[++i];
  }
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  function=argv[i];
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  path=argv[++i];
  if (LocaleCompare(function,"create") == 0)
    {
      /*
        Create a new cache repository.
      */
      status=CreateMagickCache(path,passkey);
      if (status == MagickFalse)
        {
          message=GetExceptionMessage(errno);
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "unable to create magick cache","`%s': %s",path,message);
          MagickCacheExit(exception);
        }
      return(0);
    }
  cache=AcquireMagickCache(path,passkey);
  if (cache == (MagickCache *) NULL)
    {
      message=GetExceptionMessage(errno);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unable to open magick cache","`%s': %s",path,message);
      MagickCacheExit(exception);
    }
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  iri=argv[++i];
  resource=AcquireMagickCacheResource(cache,iri);
  SetMagickCacheResourceTTL(resource,ttl);
  type=GetMagickCacheResourceType(resource);
  if ((LocaleCompare(function,"delete") != 0) &&
      (LocaleCompare(function,"expire") != 0) &&
      (LocaleCompare(function,"identify") != 0))
    {
      if (type == UndefinedResourceType)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "unrecognized resource type","`%s'",iri);
          ThrowMagickCacheResourceException(cache,resource);
        }
      if (i == (argc-1))
        MagickCacheUsage(argc,argv);
      filename=argv[++i];
    }
  status=MagickTrue;
  switch (*function)
  {
    case 'd':
    {
      if (LocaleCompare(function,"delete") == 0)
        {
          /*
            Delete one or more resources in the cache repository.
          */
          ssize_t count = 0;
          status=IterateMagickCacheResources(cache,iri,&count,DeleteResources);
          (void) fprintf(stderr,"deleted %g resources\n",(double) count);
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheResourceException(cache,resource);
    }
    case 'e':
    {
      if (LocaleCompare(function,"expire") == 0)
        {
          /*
            Expire one or more resources in the cache repository.
          */
          ssize_t count = 0;
          status=IterateMagickCacheResources(cache,iri,&count,ExpireResources);
          (void) fprintf(stderr,"expired %g resources\n",(double) count);
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheResourceException(cache,resource);
    }
    case 'g':
    {
      if (LocaleCompare(function,"get") == 0)
        {
          /*
            Get a resource from the cache repository.
          */
          switch (type)
          {
            case BlobResourceType:
            {
              blob=GetMagickCacheResourceBlob(cache,resource);
              if (blob == (void *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              status=BlobToFile(filename,blob,GetMagickCacheResourceExtent(
                resource),exception);
              break;
            }
            case ImageResourceType:
            {
              Image
                *write_image;

              image=GetMagickCacheResourceImage(cache,resource,extract);
              if (image == (Image *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              image_info=AcquireImageInfo();
              write_image=CloneImageList(image,exception);
              if (passphrase != (StringInfo *) NULL)
                status=PasskeyDecipherImage(write_image,passphrase,exception);
              status=WriteImages(image_info,write_image,filename,exception);
              write_image=DestroyImageList(write_image);
              image_info=DestroyImageInfo(image_info);
              break;
            }
            case MetaResourceType:
            {
              meta=GetMagickCacheResourceMeta(cache,resource);
              if (meta == (void *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              status=BlobToFile(filename,meta,strlen(meta),exception);
              break;
            }
            default:
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"unrecognized IRI type","`%s'",iri);
              break;
            }
          }
          if (status == MagickFalse)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"unable to get resource","`%s'",filename);
              ThrowMagickCacheResourceException(cache,resource);
            }
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      MagickCacheExit(exception);
    }
    case 'i':
    {
      if (LocaleCompare(function,"identify") == 0)
        {
          /*
            Identify one or more resources in the cache repository.
          */
          ssize_t count = 0;
          status=IterateMagickCacheResources(cache,iri,&count,
            IdentifyResources);
          (void) fprintf(stderr,"identified %g resources\n",(double) count);
          if (status == MagickFalse)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"unable to identify resource","`%s'",filename);
              ThrowMagickCacheResourceException(cache,resource);
            }
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      MagickCacheExit(exception);
    }
    case 'p':
    {
      if (LocaleCompare(function,"put") == 0)
        {
          /*
            Put a resource in the cache repository.
          */
          switch (type)
          {
            case BlobResourceType:
            {
              blob=FileToBlob(filename,~0UL,&extent,exception);
              if (blob == (void *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              status=PutMagickCacheResourceBlob(cache,resource,extent,blob);
              break;
            }
            case ImageResourceType:
            {
              Image
                *read_image;

              image_info=AcquireImageInfo();
              (void) CopyMagickString(image_info->filename,filename,
                MagickPathExtent);
              image=ReadImage(image_info,exception);
              image_info=DestroyImageInfo(image_info);
              if (image == (Image *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              read_image=CloneImageList(image,exception);
              if (passphrase != (StringInfo *) NULL)
                status=PasskeyEncipherImage(read_image,passphrase,exception);
              status=PutMagickCacheResourceImage(cache,resource,read_image);
              read_image=DestroyImageList(read_image);
              break;
            }
            case MetaResourceType:
            {
              meta=(char *) FileToBlob(filename,~0UL,&extent,exception);
              if (meta == (char *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              status=PutMagickCacheResourceMeta(cache,resource,meta);
              break;
            }
            default:
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"unrecognized IRI type","`%s'",iri);
              break;
            }
          }
          if (status == MagickFalse)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                OptionError,"unable to put resource","`%s'",filename);
              ThrowMagickCacheResourceException(cache,resource);
            }
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
         "unrecognized magick cache function","`%s'",function);
      MagickCacheExit(exception);
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
         "unrecognized magick cache function","`%s'",function);
      MagickCacheExit(exception);
    }
  }
  if (passphrase != (StringInfo *) NULL)
    passphrase=DestroyStringInfo(passphrase);
  if (passkey != (StringInfo *) NULL)
    passkey=DestroyStringInfo(passkey);
  resource=DestroyMagickCacheResource(resource);
  cache=DestroyMagickCache(cache);
  return(0);
}

static int MagickCacheMain(int argc,char **argv)
{
  ExceptionInfo
    *exception;

  MagickBooleanType
    status;

  MagickCoreGenesis(*argv,MagickTrue);
  exception=AcquireExceptionInfo();
  status=MagickCacheCLI(argc,argv,exception);
  exception=DestroyExceptionInfo(exception);
  MagickCoreTerminus();
  return(status != MagickFalse ? 0 : 1);
}

#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__) || defined(__MINGW32__)
int main(int argc,char **argv)
{
  return(MagickCacheMain(argc,argv));
}
#else
int wmain(int argc,wchar_t *argv[])
{
  char
    **utf8;

  int
    status;

  int
    i;

  utf8=NTArgvToUTF8(argc,argv);
  status=MagickCacheMain(argc,utf8);
  for (i=0; i < argc; i++)
    utf8[i]=DestroyString(utf8[i]);
  utf8=(char **) RelinquishMagickMemory(utf8);
  return(status);
}
#endif
