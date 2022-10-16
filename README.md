# MagickCache: an Efficient Image Cache

`The MagickCache is a work in progress. Do not use the cache in production services until the version is at least 1.0.0. It is currently 0.9.2. The MagickCache requires ImageMagick version 7.1.0-0 or above.`

The MagickCache provides secure methods and tools to cache images, image
sequences, video, audio or metadata in a local folder. Any content is
memory-mapped for efficient retrieval.  Additional efficiencies are possible by
retrieving a portion of an image.  Content can persist or you can assign a
time-to-live (TTL) to automatically expire content when the TTL is exceeded.
MagickCache supports virtually unlimited content upwards of billions of images,
videos, metadata, or blobs making it suitable as a web image service.

The MagickCache works in concert with [ImageMagick](https://imagemagick.org). Download the [MagickCache](https://github.com/ImageMagick/MagickCache) and install. You will now want to create the cache and populate it with images, video, and associated metadata.

## Create a MagickCache
You will require a place to store and retrieve your content.  Let's create a magick-cache on our local filesystem:

```
$ magick-cache -passkey passkey.txt create /opt/magick-cache
```

Where `passkey.txt` contains your cache passkey. The passkey can be any binary content, from a simple password or phrase, or an image, or even gibberish.  Note, the passkey is sensitive to any control characters you include in the file.  Here is one method to create a passkey without control characters:

```
$ echo -n "myPasskey" > passkey.txt
```

To be effective, make your passkey at least 8 characters in length.  Don't lose your passkey. Without it, you will be unable to identify, expire, or delete content in your cache.

You only need to create a MagickCache once.  You can, however, create more than one MagickCache with different paths.

Once the MagickCache is created, you will want to populate the cache with content that includes images, video, audio, or metadata.

## Put content in the MagickCache

Let's add a movie cast image to our newly created cache:</p>

```
$ magick-cache put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

Note, the image identifier is an IRI composed of `project/type/resource-path`. In this example, the project is `movies`, type is `image`, and the resource path is `mission-impossible/cast/rebecca-ferguson`. The path uniquely identifies a cache resource. Two different images cannot be stored with the same resource path. Instead use something like `mission-impossible/cast/20210508-rebecca-ferguson-1` and `mission-impossible/cast/20210508-rebecca-ferguson-2`.

Now, let's set a resource passkey and the time to live to 2 days. Anytime after the second day, the image is automatically deleted with the `expire` function. To get, expire, or delete the image, you will need to use the same resource passkey:

```
$ magick-cache -passkey passkey.txt -ttl "2 days" put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

Where `passkey.txt` contains your resource key. Don't lose your resource passkey. Without it, you will be unable to get, identify, expire, or delete resource you own.

The resource passkey ensures only you and the cache owner can access your image.  To prevent the cache owner from viewing its content, scramble it with a passphrase:

```
$ magick-cache -passkey passkey.txt -passphrase passphrase.txt -ttl "2 days" put /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

Note, only images are scrambled.  Blobs and metadata are stored in the cache in plaintext. To prevent snooping, scramble any blobs or metadata *before* you store it in the cache.

## Get content from the MagickCache

Eventually you will want retrieve your content, let's get our original cast image from the cache:

```
$ magick-cache -passkey passkey.txt get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

Notice the original image was put in the cache in the JPEG format. Here we conveniently convert it to the PNG format as we extract the image.

The `-extract` option is useful when retrieving an image if you want to extract just a portion of the image. Specify the tile width, height, and offset as follows:

```
$ magick-cache -passkey passkey.txt -extract 100x100+0+0 get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

To resize instead, do not specify the offset:

```
$ magick-cache -passkey passkey.txt -extract 100x100 get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

If your image is scrambled, provide the passphrase to descramble it first:

```
$ magick-cache -passkey passkey.txt -passphrase passphrase.txt get /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

## Delete content from the MagickCache

We can explicitedly delete content:

```
$ magick-cache -passkey passkey.txt delete /opt/magick-cache movies/image/mission-impossible/cast/rebecca-ferguson 
```

or we can delete all cast images that have expired (exceeded their respective time to live), try this comand:

```
$ magick-cache -passkey passkey.txt expire /opt/magick-cache movies/image/mission-impossible/cast
```

## Identify the MagickCache content

Perhaps you want to identify all the content you own:

```
$ magick-cache -passkey passkey.txt identify /opt/magick-cache movies/image/mission-impossible/cast
movies/image/mission-impossible/cast/rebecca-ferguson[1368x912] 406B  1:0:0:0 2021-05-30T17:41:42Z
identified 1 resources
```

Each entry includes the IRI, image dimensions for images, the content extent in bytes, time to live, whether the resource is expired (denoted with a `*`), and the creation date.

Others can store content in the cache along side your content.  However, their content is unavailable to you.  You cannot get, identify, delete, or expire content that you did not create or does not match your secret passkey.

The MagickCache owner can get, identify, delete, or expire all the content, including content you own, with this command, for example:

```
$ magick-cache -passkey passkey.txt identify /opt/magick-cache /
```

Note, expired resources are annotated with an asterisks.

## MagickCache is not just for Images

In addition to a type of image, you can store the image content in its original form, video, or audio as content type of `blob` or metadata with a content type of `meta`:

```
$ magick-cache -passkey passkey.txt put /opt/magick-cache movies/blob/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.mp4
```

or

```
$ magick-cache -passkey passkey.txt put /opt/magick-cache movies/meta/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.txt
```

Images must be in a format that ImageMagick [supports](https://imagemagick.org/script/formats.php).  Metadata should be text.  Blobs can be any content including images, video, audio, or binary files.

## Delete a MagickCache

The MagickCache owner can completely delete all the content within a cache:

```
$ magick-cache -passkey passkey.txt delete /opt/magick-cache /
```

Be careful. After this command, your cache content is irrevocably lost.

## Security

MagickCache security is *not* crytographically strong.  Instead it generates a unique hash of sufficient quality for each resource to ensure the resource ID cannot be discovered.  A resource is accessible to both the user of the cache and the cache owner provided they can present their respective passkeys.  They are also accessible to anyone with sufficient privileges to directly access the MagickCache path on disk.

## API

You have seen how to create, put, get, identify, delete, or expire content to and from the MagickCache with the <samp>magick-cache</samp> command-line utility.  All these functions are also available from the [MagickCache API](https://github.com/ImageMagick/MagickCache) to conveniently MagickCache functionality directly in your projects.
