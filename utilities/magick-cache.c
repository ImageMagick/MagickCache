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
%               CLI Interface to the Magick Image Cache CRUD                  %
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
%  The Magick image cache stores and retrieves images efficiently within
%  milliseconds with a small memory footprint making it suitable as a web
%  image service.
%
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
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

static MagickBooleanType ExpireMyResources(MagickCache *cache,
  MagickCacheResource *resource,const void *context)
{
  ssize_t *count = (ssize_t *) context;
  (*count)++;
  return(ExpireMagickCacheResource(cache,resource));
}

static MagickBooleanType ListMyResources(MagickCache *cache,
  MagickCacheResource *resource,const void *context)
{
  ssize_t *count = (ssize_t *) context;
  (*count)++;
  (void) fprintf(stderr,"%s %g\n",GetMagickCacheResourceIRI(resource),
    (double) GetMagickCacheResourceTTL(resource));
  return(MagickTrue);
}

static void MagickCacheUsage(int argc,char **argv)
{
  (void) fprintf(stdout,"Version: %s\n",GetMagickCacheVersion((size_t *) NULL));
  (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCacheCopyright());
  (void) fprintf(stdout,"Usage: %s create path\n",*argv);
  (void) fprintf(stdout,"Usage: %s [-key passphrase] "
    "[delete | expire | list] path iri\n",*argv);
  (void) fprintf(stdout,"Usage: %s [-key passphrase -ttl seconds] "
    "[get | put] path iri filename\n",*argv);
  exit(0);
}

static inline unsigned long StringToUnsignedLong(
  const char *magick_restrict value)
{
  return(strtoul(value,(char **) NULL,10));
}

static MagickBooleanType MagickCacheCLI(int argc,char **argv,
  ExceptionInfo *exception)
{
#define ThrowMagickCacheException(exception) \
{ \
  CatchException(exception); \
  if (resource != (MagickCacheResource *) NULL) \
    resource=DestroyMagickCacheResource(resource); \
  if (cache != (MagickCache *) NULL) \
    cache=DestroyMagickCache(cache); \
  if (message != (char *) NULL) \
    message=DestroyString(message); \
  return(-1); \
}

  char
    *filename = (char *) NULL,
    *function,
    *iri,
    *key = (char *) NULL,
    *message = (char *) NULL,
    *meta,
    *path;

  Image
    *image;

  ImageInfo
    *image_info;

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

  ssize_t
    i = 1;

  void
    *blob;

  if (argc < 2)
    MagickCacheUsage(argc,argv);
  if (LocaleCompare(argv[i],"-key") == 0)
    {
      if (i == (argc-1))
        MagickCacheUsage(argc,argv);
      key=argv[++i];
      i++;
    }
  if (LocaleCompare(argv[i],"-ttl") == 0)
    {
      char
        *q;

      /*
        Time to live, absolute or relative, e.g. 1440, 2 hours, 3 days, ...
      */
      if (i == (argc-1))
        MagickCacheUsage(argc,argv);
      ttl=InterpretLocaleValue(argv[++i],&q);
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
      i++;
    }
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  function=argv[i];
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  path=argv[++i];
  if (LocaleCompare(function,"create") == 0)
    {
      status=CreateMagickCache(path);
      if (status == MagickFalse)
        {
          message=GetExceptionMessage(errno);
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "unable to create magick cache","`%s': %s",path,message);
          ThrowMagickCacheException(exception);
        }
      return(0);
    }
  cache=AcquireMagickCache(path);
  if (cache == (MagickCache *) NULL)
    {
      message=GetExceptionMessage(errno);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unable to open magick cache","`%s': %s",path,message);
      ThrowMagickCacheException(exception);
    }
  SetMagickCacheKey(cache,key);
  if (i == (argc-1))
    MagickCacheUsage(argc,argv);
  iri=argv[++i];
  resource=AcquireMagickCacheResource(cache,iri);
  SetMagickCacheResourceTTL(resource,ttl);
  type=GetMagickCacheResourceType(resource);
  if (type == UndefinedResourceType)
    ThrowMagickCacheException(GetMagickCacheResourceException(resource));
  if ((LocaleCompare(function,"delete") != 0) &&
      (LocaleCompare(function,"expire") != 0) &&
      (LocaleCompare(function,"list") != 0))
    {
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
          status=DeleteMagickCacheResource(cache,resource);
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheException(exception);
    }
    case 'e':
    {
      if (LocaleCompare(function,"expire") == 0)
        {
          ssize_t count = 0;
          status=IterateMagickCacheResources(cache,iri,&count,
            ExpireMyResources);
          (void) fprintf(stdout,"%g resources expired\n",(double) count);
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheException(exception);
    }
    case 'g':
    {
      if (LocaleCompare(function,"get") == 0)
        {
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
              blob=RelinquishMagickMemory(blob);
              break;
            }
            case ImageResourceType:
            {
              image=GetMagickCacheResourceImage(cache,resource);
              if (image == (Image *) NULL)
                {
                  status=MagickFalse;
                  break;
                }
              image_info=AcquireImageInfo();
              status=WriteImages(image_info,image,filename,exception);
              image_info=DestroyImageInfo(image_info);
              image=DestroyImage(image);
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
              meta=(char *) RelinquishMagickMemory(meta);
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
            ThrowMagickCacheException(GetMagickCacheResourceException(
              resource));
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheException(exception);
    }
    case 'l':
    {
      if (LocaleCompare(function,"list") == 0)
        {
          ssize_t count = 0;
          status=IterateMagickCacheResources(cache,iri,&count,
            ListMyResources);
          (void) fprintf(stdout,"%g resources listed\n",(double) count);
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "unrecognized magick cache function","`%s'",filename);
      ThrowMagickCacheException(exception);
    }
    case 'p':
    {
      if (LocaleCompare(function,"put") == 0)
        {
          switch (type)
          {
            case BlobResourceType:
            {
              blob=FileToBlob(filename,~0UL,&extent,exception);
              if (blob == (void *) NULL)
                break;
              status=PutMagickCacheResourceBlob(cache,resource,extent,blob);
              blob=RelinquishMagickMemory(blob);
              break;
            }
            case ImageResourceType:
            {
              image_info=AcquireImageInfo();
              (void) CopyMagickString(image_info->filename,filename,
                MagickPathExtent);
              image=ReadImage(image_info,exception);
              image_info=DestroyImageInfo(image_info);
              status=PutMagickCacheResourceImage(cache,resource,image);
              image=DestroyImage(image);
              break;
            }
            case MetaResourceType:
            {
              meta=(char *) FileToBlob(filename,~0UL,&extent,exception);
              if (meta == (char *) NULL)
                break;
              status=PutMagickCacheResourceMeta(cache,resource,meta);
              meta=(char *) RelinquishMagickMemory(meta);
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
            ThrowMagickCacheException(GetMagickCacheResourceException(
              resource));
          break;
        }
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
         "unrecognized magick cache function","`%s'",function);
      ThrowMagickCacheException(exception);
    }
    default:
    {
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
         "unrecognized magick cache function","`%s'",function);
      ThrowMagickCacheException(exception);
    }
  }
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