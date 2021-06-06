# Install Magick Cache

## Prerequisites

(ImageMagick)[https://imagemagick.org] version 7.0.11-14 or newer is required before you can download, configure, build, install, and utilize the magick cache.

## Download

```
$ git clone -b main https://github.com/ImageMagick/MagickCache.git MagickCache-0.9.2
```

## Configure

```
$ cd MagickCache-0.9.2
$ ./configuee
```

## Build, Verify, and Install

```
$ make
$ make check
$ make install
```

## Utilize the Cache

See README.MD to utilize the put, get, and identify resources within the cache.
