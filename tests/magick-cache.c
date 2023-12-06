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
%                     MagickCache Repository Unit Tests                       %
%                                                                             %
%                             Software Design                                 %
%                                 Cristy                                      %
%                               March 2021                                    %
%                                                                             %
%                                                                             %
%  Copyright 1999 ImageMagick Studio LLC, a non-profit organization           %
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
#include "MagickCache/MagickCache.h"
#include "MagickCache/magick-cache-private.h"

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

static MagickBooleanType MagickCacheCLI(int argc,char **argv,
  ExceptionInfo *exception)
{
#define MagickCacheKey  "5u[Jz,3!"
#define MagickCacheRepo  "./magick-cache-repo"
#define MagickCacheResourceIRI  "tests"
#define MagickCacheResourceBlobIRI  "tests/blob/rose"
#define MagickCacheResourceImageIRI  "tests/image/rose"
#define MagickCacheResourceTTL  75
#define MagickCacheResourceMeta  "a woody perennial flowering plant of the " \
  "genus Rosa, in the family Rosaceae, or the flower it bears"
#define MagickCacheResourceMetaIRI  "tests/meta/rose"
#define ThrowMagickCacheException(cache) \
{ \
  description=GetMagickCacheException(cache,&severity); \
  (void) FormatLocaleFile(stderr,"%s %s %lu %s\n",GetMagickModule(), \
    description); \
  description=(char *) DestroyString(description); \
}
#define ThrowMagickCacheResourceException(resource) \
{ \
  description=GetMagickCacheResourceException(resource,&severity); \
  (void) FormatLocaleFile(stderr,"%s %s %lu %s\n",GetMagickModule(), \
    description); \
  description=(char *) DestroyString(description); \
}

  char
    *description;

  const char
    *meta = (const char *) NULL,
    *path = MagickCacheRepo;

  const Image
    *image = (const Image *) NULL;

  const void
    *blob = (const void *) NULL;

  double
    distortion = 0.0;

  ExceptionType
    severity = UndefinedException;

  Image
    *difference = (Image *) NULL,
    *rose = (Image *) NULL;

  ImageInfo
    *image_info = AcquireImageInfo();

  MagickBooleanType
    status;

  MagickCache
    *cache = (MagickCache *) NULL;

  MagickCacheResource
    *blob_resource = (MagickCacheResource *) NULL,
    *image_resource = (MagickCacheResource *) NULL,
    *meta_resource = (MagickCacheResource *) NULL;

  size_t
    columns = 0,
    count = 0,
    extent = 0,
    fail = 0,
    rows = 0,
    tests = 0;

  StringInfo
    *passkey = StringToStringInfo(MagickCacheKey);

  unsigned long
    signature = MagickCoreSignature;

  /*
    Run the unit tests to exercise the MagickCache repository methods.
  */
  (void) argv;
  (void) argc;
  (void) FormatLocaleFile(stdout,"%g: create magick cache\n",(double) tests);
  tests++;
  status=CreateMagickCache(path,passkey);
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: acquire magick cache\n",(double) tests);
  tests++;
  cache=AcquireMagickCache(path,passkey);
  if (cache == (MagickCache *) NULL)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: acquire magick cache resource\n",(double)
    tests);
  tests++;
  if (cache != (MagickCache *) NULL)
    {
      blob_resource=AcquireMagickCacheResource(cache,
        MagickCacheResourceBlobIRI);
      image_resource=AcquireMagickCacheResource(cache,
        MagickCacheResourceImageIRI);
      meta_resource=AcquireMagickCacheResource(cache,
        MagickCacheResourceMetaIRI);
    }
  if ((blob_resource == (MagickCacheResource *) NULL) &&
      (image_resource == (MagickCacheResource *) NULL) &&
      (meta_resource == (MagickCacheResource *) NULL))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: put magick cache (image)\n",(double)
    tests);
  tests++;
  (void) strcpy(image_info->filename,"rose:");
  if (cache != (MagickCache *) NULL)
    rose=ReadImage(image_info,exception);
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL) &&
      (rose != (Image *) NULL))
    {
      SetMagickCacheResourceTTL(image_resource,1);
      status=PutMagickCacheResourceImage(cache,image_resource,rose);
    }
  else
    status=MagickFalse;
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(image_resource);
      fail++;
    }
  (void) sleep(1);

  (void) FormatLocaleFile(stdout,"%g: put/get magick cache (blob)\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    {
      SetMagickCacheResourceTTL(blob_resource,MagickCacheResourceTTL);
      status=PutMagickCacheResourceBlob(cache,blob_resource,sizeof(signature),
        &signature);
    }
  if ((cache != (MagickCache *) NULL) &&
      (blob_resource != (MagickCacheResource *) NULL) &&
      (status != MagickFalse))
    {
      blob=GetMagickCacheResourceBlob(cache,blob_resource);
      extent=GetMagickCacheResourceExtent(blob_resource);
    }
  if ((status == MagickFalse) || (blob == (const void *) NULL) ||
      (extent != sizeof(signature)) || (memcmp(blob,&signature,extent)))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(blob_resource);
      fail++;
    }
  (void) sleep(1);

  (void) FormatLocaleFile(stdout,"%g: put/get magick cache (meta)\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (meta_resource != (MagickCacheResource *) NULL))
    {
      SetMagickCacheResourceTTL(meta_resource,MagickCacheResourceTTL);
      status=PutMagickCacheResourceMeta(cache,meta_resource,
        MagickCacheResourceMeta);
    }
  if ((cache != (MagickCache *) NULL) &&
      (meta_resource != (MagickCacheResource *) NULL) &&
      (status != MagickFalse))
    meta=GetMagickCacheResourceMeta(cache,meta_resource);
  if ((status == MagickFalse) || (meta == (const char *) NULL) ||
      (strlen(meta) != strlen(MagickCacheResourceMeta)) ||
      (memcmp(meta,MagickCacheResourceMeta,strlen(meta))))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(meta_resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: identify magick cache resources\n",
    (double) tests);
  tests++;
  if (cache != (MagickCache *) NULL)
    {
      count=0;
      status=IterateMagickCacheResources(cache,MagickCacheResourceIRI,&count,
        IdentifyResources);
      (void) fprintf(stderr,"identified %g resources\n",(double) count);
    }
  if ((status == MagickFalse) || (count != 3))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: get magick cache (image)\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,image_resource,(const char *) NULL);
  if ((image != (const Image *) NULL) && (rose != (Image *) NULL))
    difference=CompareImages(rose,image,RootMeanSquaredErrorMetric,
      &distortion,exception);
  if ((image == (Image *) NULL) || (distortion >= MagickEpsilon))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(image_resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: get magick cache (image tile)\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,image_resource,"35x23+0+0");
  if (image != (const Image *) NULL)
    GetMagickCacheResourceSize(image_resource,&columns,&rows);
  if ((image == (Image *) NULL) || (columns != 35) || (rows != 23))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(image_resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: get magick cache (image resize)\n",
    (double) tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,image_resource,"35x23");
  if (image != (const Image *) NULL)
    GetMagickCacheResourceSize(image_resource,&columns,&rows);
  if ((image == (Image *) NULL) || (columns != 35) || (rows != 23))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(image_resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: expire magick cache resource\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    {
      count=0;
      status=IterateMagickCacheResources(cache,MagickCacheResourceIRI,&count,
        ExpireResources);
      (void) fprintf(stderr,"expired %g resources\n",(double) count);
    }
  if ((status == MagickFalse) || (count != 1))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }
  (void) FormatLocaleFile(stdout,"%g: delete magick cache resource\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (image_resource != (MagickCacheResource *) NULL))
    {
      count=0;
      status=IterateMagickCacheResources(cache,MagickCacheResourceIRI,&count,
        DeleteResources);
      (void) fprintf(stderr,"deleted %g resources\n",(double) count);
    }
  if ((status == MagickFalse) || (count != 2))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: delete magick cache\n",(double) tests);
  tests++;
  if (cache != (MagickCache *) NULL)
    {
      const char *path = MagickCacheRepo "/" MagickCacheSentinel;
      if (remove_utf8(path) == -1)
        status=MagickFalse;
      if (remove_utf8(MagickCacheRepo) == -1)
        status=MagickFalse;
    }
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }

  /*
    Free memory.
  */
  if (difference != (Image *) NULL)
    difference=DestroyImageList(difference);
  if (rose != (Image *) NULL)
    rose=DestroyImageList(rose);
  if (image_info != (ImageInfo *) NULL)
    image_info=DestroyImageInfo(image_info);
  if (meta_resource != (MagickCacheResource *) NULL)
    meta_resource=DestroyMagickCacheResource(meta_resource);
  if (image_resource != (MagickCacheResource *) NULL)
    image_resource=DestroyMagickCacheResource(image_resource);
  if (blob_resource != (MagickCacheResource *) NULL)
    blob_resource=DestroyMagickCacheResource(blob_resource);
  if (cache != (MagickCache *) NULL)
    cache=DestroyMagickCache(cache);

  /*
    Unit tests accounting.
  */
  (void) FormatLocaleFile(stdout,
    "validation suite: %.20g tests; %.20g passed; %.20g failed.\n",(double)
     tests,(double) (tests-fail),(double) fail);
  return(fail == 0 ? MagickTrue : MagickFalse);
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
