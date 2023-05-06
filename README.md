# MagickCache: an Efficient Digital Media Repository

`The MagickCache is a work in progress. Do not use the cache in production services until the version is at least 1.0.0. It is currently 0.9.2. The MagickCache requires ImageMagick version 7.1.0-0 or above.`

MagickCache is an advanced toolset that guarantees secure caching of images, image sequences, videos, audios, or metadata within a local folder. The content is memory-mapped to ensure fast and efficient retrieval, and retrieving a portion of an image further enhances its efficiency. You have the flexibility to choose whether the content should persist or have a specified time-to-live (TTL) to automatically expire when the TTL is exceeded. MagickCache has the ability to support virtually an unlimited amount of content, up to billions of images, videos, metadata, or blobs, making it ideal for use as a digital media repository.

The MagickCache works in concert with [ImageMagick](https://imagemagick.org). Download the [MagickCache](https://github.com/ImageMagick/MagickCache) and install. You will now want to create the cache and populate it with images, video, audio, and any associated metadata.

## Create a Digital Media Repository

You will require a place to store and retrieve your content.  Let's create a digital media repository on your local filesystem:

```
$ magick-cache -passkey ~/.passkey create /opt/dmr
```

Where `~/.passkey` contains your cache passkey. The passkey can be any binary content, from a simple password or phrase, or an image, or even gibberish.  Note, the passkey is sensitive to any control characters you include in the file.  Here is one method to create a passkey without control characters:

```
$ echo -n "myPasskey" > ~/.passkey
```

To be effective, make your passkey at least 8 characters in length.  Don't lose your passkey. Without it, you will be unable to identify, delete, or expire content in your cache.

To reduce latency and increase efficiency, we recommend you create your digital media repository on a solid-state drive (SSD).

You only need to create a MagickCache once to store upwards of billions of images, video, audio, and metadata.  You can, however, create more than one MagickCache with different paths.

Once the MagickCache is created, you will want to populate the cache with content that includes images, video, audio, or metadata.

## Put content in the Digital Media Repository

Let's add a movie cast image to our newly created digital media repository:</p>

```
$ magick-cache put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

Note that the image identifier is an IRI comprising the project/type/resource-path components. In the given example, the project is movies, the type is image, and the resource path is mission-impossible/cast/rebecca-ferguson, which serves as a unique identifier for the cached resource. It is important to ensure that no two different images are stored using the same resource path. If you need to store multiple versions of an image, consider using a distinct identifier such as mission-impossible/cast/20210508-rebecca-ferguson-1 and mission-impossible/cast/20210508-rebecca-ferguson-2.

Now, let's set a resource passkey and the time to live to 2 days. Anytime after the second day, the image is automatically deleted with the `expire` function. To get, delete, or expire the image, you will need to use the same resource passkey:

```
$ magick-cache -passkey ~/.passkey -ttl "2 days" put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

Where `~/.passkey` contains your resource key. Don't lose your resource passkey. Without it, you will be unable to get, identify, delete, or expire resources you created.

The resource passkey ensures only you and the cache owner can access your image.  To prevent the cache owner from viewing its content, scramble it with a passphrase:

```
$ magick-cache -passkey ~/.passkey -passphrase ~/.passphrase -ttl "2 days" put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.jpg
```

You will need the same passphrase when you retrieve the image to restore it back to its original form.

Note, only images are scrambled.  Blobs and metadata are stored in the cache in plaintext. To prevent snooping, scramble any blobs or metadata *before* you store it in the cache.

## Get content from the Digital Media Repository

Eventually you will want retrieve your content from the cache. As an example, let's get our original cast image from the cache:

```
$ magick-cache -passkey ~/.passkey get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

Notice the original image was put in the cache in the JPEG format. Here we conveniently convert it to the PNG format as we extract the image.

The `-extract` option is useful when retrieving an image if you want to extract just a portion of the image. Specify the tile width, height, and offset as follows:

```
$ magick-cache -passkey ~/.passkey -extract 100x100+0+0 get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

To resize instead, do not specify the offset:

```
$ magick-cache -passkey ~/.passkey -extract 100x100 get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

If your image is scrambled, provide the passphrase to descramble it first:

```
$ magick-cache -passkey ~/.passkey -passphrase ~/.passphrase get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

## Delete content from the Digital Media Repository

We can explicitedly delete content:

```
$ magick-cache -passkey ~/.passkey delete /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 
```

or we can delete all cast images that have expired (exceeded their respective time to live), try this comand:

```
$ magick-cache -passkey ~/.passkey expire /opt/dmr movies/image/mission-impossible/cast
```

## Identify the Digital Media Repository Content

Perhaps you want to identify all the content you own:

```
$ magick-cache -passkey ~/.passkey identify /opt/dmr movies/image/mission-impossible/cast
movies/image/mission-impossible/cast/rebecca-ferguson[1368x912] 406B  1:0:0:0 2021-05-30T17:41:42Z
identified 1 resources
```

Each entry includes the IRI, image dimensions for images, the content extent in bytes, time to live, whether the resource is expired (denoted with a `*`), and the creation date.

MagickCache supports a wild resource type as in this example:

```
$ magick-cache -passkey ~/.passkey identify /opt/dmr movies/*/mission-impossible/cast
movies/image/mission-impossible/cast/rebecca-ferguson[1368x912] 889B 0:0:0:0  2023-05-06T00:49:27Z
movies/blob/mission-impossible/cast/rebecca-ferguson 1.14476MiB 0:0:0:0  2023-05-06T00:49:27Z
movies/meta/mission-impossible/cast/rebecca-ferguson 11B 0:0:0:0  2023-05-06T00:49:27Z
```

Others can store content in the cache along side your content.  However, their content is unavailable to you.  You cannot get, identify, delete, or expire content that you did not create or does not match your secret passkey.

The MagickCache creator can get, identify, delete, or expire all the content, including content you own, with this command, for example:

```
$ magick-cache -passkey ~/.passkey identify /opt/dmr /
```

Note, expired resources are annotated with an asterisks.

## MagickCache is not just for Images

In addition to a type of image, you can store the image content in its original form, video, or audio as content type of `blob` or metadata with a content type of `meta`:

```
$ magick-cache -passkey ~/.passkey put /opt/dmr movies/blob/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.mp4
```

or

```
$ magick-cache -passkey ~/.passkey put /opt/dmr movies/meta/mission-impossible/cast/rebecca-ferguson 20210508-rebecca-ferguson.txt
```

Images must be in a format that ImageMagick [supports](https://imagemagick.org/script/formats.php).  Metadata should be text.  Blobs can be any content, text or binary, including metadata or images, video, audio, or binary files.

## Delete a Digital Media Repository

The MagickCache owner can completely delete all the content within a cache:

```
$ magick-cache -passkey ~/.passkey delete /opt/dmr /
```

Be careful. After this command, your cache content is irrevocably lost.

## Digital Media Repository Security

It's important to note that the security measures employed by MagickCache are not based on cryptographic strength. Instead, the system generates a unique hash of appropriate quality for each resource to ensure that the resource ID remains unknown. Access to a resource is granted to both the cache user and the owner, provided they can present their respective passkeys. Furthermore, anyone with adequate privileges to access the MagickCache path directly on disk will also be able to access the resources stored therein.

## Portable Digital Media Repository

The digital asset repository you created is engineered to be portable and fully self-contained. This means that you can effortlessly transfer or duplicate the repository to any storage location within your current host or even to a different host altogether. You can then retrieve or upload resources to the repository as long as you use the same key that was used during the repository's initial creation.

## MagickCache API

You have seen how to create, put, get, identify, delete, or expire content to and from the MagickCache with the <samp>magick-cache</samp> command-line utility.  All these functions are also available from the [MagickCache API](https://github.com/ImageMagick/MagickCache) to conveniently include MagickCache functionality directly in your projects.

## ImageMagick Digital Media Repository Access

You can get media from, or put media to, the repository with [ImageMagick](https://imagemagick.org).  To convert a digital media resource to PNG, try:

```
convert -define dmr:path=/opt/dmr -define dmr:passkey=/dmr/.passkey \
  dmr:movies/image/mission-impossible/cast/rebecca-ferguson \
  rebecca-ferguson.png
```

To put or replace a resource in the repository, try:

```
convert rebecca-ferguson.png \
  -define dmr:path=/opt/dmr -define dmr:passkey=/dmr/.passkey \
  dmr:movies/image/mission-impossible/cast/rebecca-ferguson
```

You can also store an image as a blob instead.  To store meta data, set the `dmr:meta` property:

```
convert -define dmr:path=/opt/dmr -define dmr:passkey=/home/cristy/.passkey \
  -define dmr:meta="Ilsa Faust" xc: \
  dmr:movies/meta/mission-impossible/cast/rebecca-ferguson
```
