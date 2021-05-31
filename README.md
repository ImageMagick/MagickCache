# Magick Cache: an Efficient Image Cache

`The Magick cache is a work in progress. Do not use the cache in production services until the version is at least 1.0.0. It is currently 0.9.2. The Magick Cache requires ImageMagick version 7.0.11-14 or above.`

The Magick Cache stores and retrieves images (and other content) efficiently within milliseconds with a small memory footprint. In addition to images, you can store video, audio, and associated properties. The cache supports virtually unlimited content upwards of billions of images making it suitable as a web image service.

The Magick Cache works in concert with [ImageMagick](https://imagemagick.org). Download the [MagickCache](https://github.com/ImageMagick/MagickCache) and install. You'll now want to create the cache and populate it with images, video, and associated metadata.

## Create a Magick Cache

You'll need a place to store and retrieve your content.  Let's create a cache on our local filesystem:

```
$ magick-cache create /opt/magick-cache
```

Once its created, you will want to populate it with content that includes images, video, audio, or metadata.

## Put content in the Magick Cache

Let's add a movie cast image to the cache to our newly created cache:</p>

```
$ magick-cache put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.jpg
```

Note, the image identifier is an IRI composed of `project/type/resource-path`. In this example, the project is `movies`, type is `image`, and the resource path is `mission-impossible/cast/rebecca-ferguson`. The path uniquely identifies a resource. Two different images cannot be stored with the same resource path. Instead use something like `mission-impossible/cast/200508-rebecca-ferguson` and `mission-impossible/cast/200513-rebecca-ferguson`.

Now, set a cache key and the time to live to 2 days. Anytime after 1 day, the image will automatically expire with the `expire` function. To get, expire, or delete the image, you will need to use the same cache key.

```
$ magick-cache -key s5hPjbxEwS -ttl "2 days" put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.jpg
```

Don't forget your cache key. Without it, you will not be able to get, list, delete or expire your content.

## Get content from the Magick Cache

Eventually you will want retrieve your content, let's get our cast image from the cache:

```
$ magick-cache -key s5hPjbxEwS get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

Notice the original image was put in the Magick Cache in the JPEG format. Here we conveniently convert it to the PNG image format.

## Delete content from the Magick Cache

We can explicitedly delete content:

```
$ magick-cache -key s5hPjbxEwS delete /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 
```

or we can delete cast images that have expired (exceeded their respective time to live), try this comand:

```
$ magick-cache -key s5hPjbxEwS expire /opt/magick-cache movies/image/mission-impossible/cast
```

## List the Magick Cache content

Perhaps you want to audit all the content you own:

```
$ magick-cache -key s5hPjbxEwS list /opt/magick-cache movies/image/mission-impossible/cast
movies/image/mission-impossible/cast/rebecca-ferguson 86400s 2021-05-30T17:41:42Z
listed 1 resources
```

Others can store content in the cache along side your content.  However, their content is unavailable to you.  You cannot get it, delete it, or list it.

## Magick Cache is not just for images

In addition to a type of image, you can store the image content in its original form, video, or audio as content type of `blob` or metadata with a content type of `meta`:

```
$ magick-cache -key D4HiNCZeRn put /opt/magick-cache movies/blob/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.mp4
```

or

```
$ magick-cache -key D4HiNCZeRn put /opt/magick-cache movies/meta/mission-impossible/cast/rebecca-ferguson 0200508-rebecca-ferguson.txt
```

Images must be in a format that ImageMagick understands.  Metadata must be text.  Blobs can be any content including images, video, audio, or binary files.
