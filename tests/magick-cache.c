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
%                     Magick Image Cache CRUD Unit Tests                      %
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
#include <math.h>
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

static MagickBooleanType MagickCacheCLI(int argc,char **argv,
  ExceptionInfo *exception)
{
#define MagickCacheKey  "5u[Jz,3!"
#define MagickCacheRepo  "./magick-cache-repo"
#define MagickCacheResourceBlobIRI  "tests/blob/rose"
#define MagickCacheResourceImageIRI  "tests/image/rose"
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
    *resource = (MagickCacheResource *) NULL;

  size_t
    columns = 0,
    extent = 0,
    fail = 0,
    rows = 0,
    tests = 0;

  StringInfo
    *cache_key = StringToStringInfo(MagickCacheKey);

  unsigned int
    signature = MagickCoreSignature;

  (void) FormatLocaleFile(stdout,"%g: create magick cache\n",(double) tests);
  tests++;
  status=CreateMagickCache(path,cache_key);
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: acquire magick cache\n",(double) tests);
  tests++;
  cache=AcquireMagickCache(path,cache_key);
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
    resource=AcquireMagickCacheResource(cache,MagickCacheResourceImageIRI);
  if (resource == (MagickCacheResource *) NULL)
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
      (resource != (MagickCacheResource *) NULL) && (rose != (Image *) NULL))
    status=PutMagickCacheResourceImage(cache,resource,rose);
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: identify magick cache resource\n",
    (double) tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    status=IdentifyMagickCacheResource(cache,resource,stdout);
  if (status == MagickFalse)
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
      (resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,resource,(const char *) NULL);
  if ((image != (const Image *) NULL) && (rose != (Image *) NULL))
    difference=CompareImages(rose,image,RootMeanSquaredErrorMetric,
      &distortion,exception);
  if ((image == (Image *) NULL) || (distortion >= MagickEpsilon))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: get magick cache (image tile)\n",(double)
     tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,resource,"35x23+0+0");
  if (image != (const Image *) NULL)
    GetMagickCacheResourceSize(resource,&columns,&rows);
  if ((image == (Image *) NULL) || (columns != 35) || (rows != 23))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: get magick cache (image resize)\n",
    (double) tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    image=GetMagickCacheResourceImage(cache,resource,"35x23");
  if (image != (const Image *) NULL)
    GetMagickCacheResourceSize(resource,&columns,&rows);
  if ((image == (Image *) NULL) || (columns != 35) || (rows != 23))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: expire magick cache resource\n",(double)
    tests);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    status=ExpireMagickCacheResource(cache,resource);
  if (status == MagickFalse)
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
      (resource != (MagickCacheResource *) NULL))
    status=DeleteMagickCacheResource(cache,resource);
  if (status == MagickFalse)
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheException(cache);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: put/get magick cache (blob)\n",(double)
     tests);
  if (resource != (MagickCacheResource *) NULL)
    resource=DestroyMagickCacheResource(resource);
  if (cache != (MagickCache *) NULL)
    resource=AcquireMagickCacheResource(cache,MagickCacheResourceBlobIRI);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    status=PutMagickCacheResourceBlob(cache,resource,sizeof(signature),
      &signature);
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL) && (status != MagickFalse))
    {
      blob=GetMagickCacheResourceBlob(cache,resource);
      extent=GetMagickCacheResourceExtent(resource);
    }
  if ((status == MagickFalse) || (blob == (const void *) NULL) ||
      (extent != sizeof(signature)) || (memcmp(blob,&signature,extent)))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: put/get magick cache (meta)\n",(double)
     tests);
  if (resource != (MagickCacheResource *) NULL)
    resource=DestroyMagickCacheResource(resource);
  if (cache != (MagickCache *) NULL)
    resource=AcquireMagickCacheResource(cache,MagickCacheResourceMetaIRI);
  tests++;
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL))
    status=PutMagickCacheResourceMeta(cache,resource,MagickCacheResourceMeta);
  if ((cache != (MagickCache *) NULL) &&
      (resource != (MagickCacheResource *) NULL) && (status != MagickFalse))
    meta=GetMagickCacheResourceMeta(cache,resource);
  if ((status == MagickFalse) || (meta == (const char *) NULL) ||
      (strlen(meta) != strlen(MagickCacheResourceMeta)) ||
      (memcmp(meta,MagickCacheResourceMeta,strlen(meta))))
    {
      (void) FormatLocaleFile(stdout,"... fail @ %s/%s/%lu.\n",
        GetMagickModule());
      ThrowMagickCacheResourceException(resource);
      fail++;
    }

  (void) FormatLocaleFile(stdout,"%g: delete magick cache\n",(double) tests);
  tests++;
  if (cache != (MagickCache *) NULL)
    status=DeleteMagickCache(cache);
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
  if (resource != (MagickCacheResource *) NULL)
    resource=DestroyMagickCacheResource(resource);
  if (cache != (MagickCache *) NULL)
    cache=DestroyMagickCache(cache);

  /*
    Unit tests accounting.
  */
  (void) FormatLocaleFile(stdout,
    "validation suite: %.20g tests; %.20g passed; %.20g failed.\n",(double)
     tests,(double) (tests-fail),(double) fail);
  return(fail == 0 ? 1 : 0);
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
