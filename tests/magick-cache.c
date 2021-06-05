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
  const char
    *path = "./magick-cache-tests";

  MagickCache
    *cache;

  MagickBooleanType
    status;

  size_t
    fail = 0,
    tests = 0;

  StringInfo
    *cache_key = StringToStringInfo("5u[Jz,3!");

  tests++;
  status=CreateMagickCache(path);
  if (status == MagickFalse)
    fail++;

  tests++;
  cache=AcquireMagickCache(path,cache_key);
  if (cache == (MagickCache *) NULL)
    fail++;

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
