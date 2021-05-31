# Magick Cache: an Efficient Image Cache

The Magick image cache is a work in progress. Do not use the cache in production services until the API is at least version 1.0.0. It is currently 0.9.2. The Magick Cache requires ImageMagick version 7.0.11-14 or above.

The Magick Cache stores and retrieves images efficiently within milliseconds with a small memory footprint. In addition to images, you can store video, audio, and associated properties. The cache supports virtually unlimited content upwards of billions of images making it suitable as a web image service.

The Magick Cache works in concert with ImageMagick. Download the MagickCache and install. You'll now want to create the cache and populate it with images, video, and associated metadata.

## Create a Magick Cache

```
$ magick-cache create /opt/magick-cache
```

Once you create the magick cache, you will want to populate it with resources including images, video, audio, or metadata.

## Put an image in the Magick Cache

```
$ magick-cache put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.jpg
```

Note, image identifier is an IRI composed of project/type/resource-path. In this example, the project is movies, type is image, and the resource path is `mission-impossible/cast/rebecca-ferguson`. The path uniquely identifies a resource. Two different images cannot be stored with the same resource path. Instead use something like `mission-impossible/cast/200508-rebecca-ferguson` and `mission-impossible/cast/200513-rebecca-ferguson`.

Set a cache key and the time to live to 2 days. Anytime after 1 day, the image will automatically expire with the expire function. To get, expire, or delete the image, you will need to use the same cache key.

```
$ magick-cache -key s5hPjbxEwS -ttl "2 days" put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.jpg
```

Don't forget your cache key. Without it, you will not be able to get, list, delete or expire your content.

## Get an image from the Magick Cache

```
$ magick-cache -key s5hPjbxEwS get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

Notice the original image was put in the Magick Cache in the JPEG format. Here we the the image and convert it to the PNG image format.

## Delete an image from the Magick Cache
$ magick-cache -key s5hPjbxEwS delete /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 
To delete any cast images that have expired (exceeded their respective time to live), try this comand:

```
$ magick-cache -key s5hPjbxEwS expire /opt/magick-cache movies/image/mission-impossible/cast
```

## List the Magick Cache Content

```
$ magick-cache -key s5hPjbxEwS list /opt/magick-cache movies/image/mission-impossible/cast
```

In addition to a type of image, you can store the image content in its original form, video, or audio as content type of blob or metadata with a content type of meta:

```
$ magick-cache -key D4HiNCZeRn put /opt/magick-cache movies/blob/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.mp4
```

or

```
$ magick-cache -key D4HiNCZeRn put /opt/magick-cache movies/meta/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.txt
```
