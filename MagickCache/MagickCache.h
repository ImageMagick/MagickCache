/*
  Copyright 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
  The MagickCache provides secure methods and tools to cache images, image
  sequences, video, audio or metadata in a local folder. Any content is
  memory-mapped for efficient retrieval.  Additional efficiences are possible
  by retrieving a portion of an image.  Content can persist or you can assign
  a time-to-live (TTL) to automatically expire content when the TTL is
  exceeded. MagickCache supports virtually unlimited content upwards of
  billions of images making it suitable as a web image service.
*/

#ifndef MAGICKCACHE_MAGICKCACHE_H
#define MAGICKCACHE_MAGICKCACHE_H

#include <limits.h>
#include <MagickCore/MagickCore.h>
#include "MagickCache/version.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum
{
  UndefinedResourceType,
  BlobResourceType,
  ImageResourceType,
  MetaResourceType,
  WildResourceType
} MagickCacheResourceType;

typedef struct _MagickCache
  MagickCache;

typedef struct _MagickCacheResource
  MagickCacheResource;

extern MagickExport char
  *GetMagickCacheException(const MagickCache *,ExceptionType *),
  *GetMagickCacheResourceException(const MagickCacheResource *,ExceptionType *),
  *GetMagickCacheResourceIRI(const MagickCacheResource *),
  *GetMagickCacheResourceMeta(MagickCache *,MagickCacheResource *);

extern MagickExport Image
  *GetMagickCacheResourceImage(MagickCache *cache,MagickCacheResource *,
    const char *);

extern MagickExport MagickBooleanType
  ClearMagickCacheException(MagickCache *),
  ClearMagickCacheResource(MagickCacheResource *),
  CreateMagickCache(const char *,const StringInfo *),
  DeleteMagickCacheResource(MagickCache *,MagickCacheResource *),
  GetMagickCacheResource(MagickCache *,MagickCacheResource *),
  GetMagickCacheResourceID(MagickCache *,const size_t,char *),
  IdentifyMagickCacheResource(MagickCache *,MagickCacheResource *,FILE *),
  IsMagickCacheResourceExpired(MagickCache *,MagickCacheResource *),
  IterateMagickCacheResources(MagickCache *,const char *,const void *,
    MagickBooleanType (*callback)(MagickCache *,MagickCacheResource *,
    const void *)),
  PutMagickCacheResource(MagickCache *,MagickCacheResource *),
  PutMagickCacheResourceBlob(MagickCache *,MagickCacheResource *,const size_t,
    const void *),
  PutMagickCacheResourceImage(MagickCache *,MagickCacheResource *,
    const Image *),
  PutMagickCacheResourceMeta(MagickCache *,MagickCacheResource *,const char *),
  SetMagickCacheResourceIRI(MagickCache *,MagickCacheResource *,const char *),
  SetMagickCacheResourceVersion(MagickCacheResource *,const size_t);

extern MagickExport MagickCache
  *AcquireMagickCache(const char *,const StringInfo *),
  *DestroyMagickCache(MagickCache *);

extern MagickExport MagickCacheResource
  *AcquireMagickCacheResource(MagickCache *,const char *),
  *DestroyMagickCacheResource(MagickCacheResource *);

extern MagickExport MagickCacheResourceType
  GetMagickCacheResourceType(const MagickCacheResource *);

extern MagickExport size_t
  GetMagickCacheResourceExtent(const MagickCacheResource *),
  GetMagickCacheResourceVersion(const MagickCacheResource *);

extern MagickExport time_t
  GetMagickCacheTimestamp(const MagickCache *),
  GetMagickCacheResourceTimestamp(const MagickCacheResource *),
  GetMagickCacheResourceTTL(const MagickCacheResource *);

extern MagickExport void
  *GetMagickCacheResourceBlob(MagickCache *,MagickCacheResource *),
  GetMagickCacheResourceSize(const MagickCacheResource *,size_t *,size_t *),
  SetMagickCacheResourceTTL(MagickCacheResource *,const time_t);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
