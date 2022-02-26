/*
  Copyright 1999-2021 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    https://imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCache's Toolkit version and copyright.
*/
#ifndef _MAGICKCACHE_VERSION_H
#define _MAGICKCACHE_VERSION_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#define MagickCachePackageName "MagickCache"
#define MagickCacheCopyright  "Copyright (C) 1999-2021 ImageMagick Studio LLC"
#define MagickCacheLibVersion  0x10A
#define MagickCacheLibVersionText  "0.9.2"
#define MagickCacheLibVersionNumber  0,0,0
#define MagickCacheLibAddendum  "-1"
#define MagickCacheLibInterface  0
#define MagickCacheLibMinInterface  0
#define MagickCacheReleaseDate  ""
#define MagickCacheAuthoritativeURL  "http://www.imagemagick.org"
#define MagickCacheVersion MagickCachePackageName " " MagickCacheLibVersionText \
  MagickCacheLibAddendum " " MagickCacheReleaseDate " " MagickCacheAuthoritativeURL

/*
  Method declarations.
*/
extern MagickExport const char
  *GetMagickCacheCopyright(void),
  *GetMagickCacheVersion(size_t *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
