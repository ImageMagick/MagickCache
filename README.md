# MagickCache: An Efficient Digital Media Repository

[![Build Status](https://github.com/ImageMagick/MagickCache/workflows/main/badge.svg)](https://github.com/ImageMagick/MagickCache/actions)
[![Donate](https://img.shields.io/badge/%24-donate-ff00ff.svg)](https://github.com/sponsors/ImageMagick)

![MagickCache](MagickCache.png)

MagickCache is a free, [open-source](https://imagemagick.org/script/license.php), high-performance repository for securely storing images, image sequences, video, audio, metadata, and arbitrary binary content. Resources are memory-mapped for fast access, and image retrieval can be limited to a specific region for additional efficiency.

Resources can be stored permanently or assigned a time-to-live (TTL), after which they expire and can be removed. A single MagickCache repository can scale to billions of resources, making it well suited to digital media archiving, asset management, and content delivery workloads.

MagickCache works in concert with [ImageMagick](https://imagemagick.org). Download and install [MagickCache](https://github.com/ImageMagick/MagickCache), then create and populate a repository with your media and metadata. MagickCache is available as both a command-line utility (`magick-cache`) and a C API.

## Contents

- [Key Concepts](#key-concepts)
- [Create a Digital Media Repository](#create-a-digital-media-repository)
- [Put Content in the Repository](#put-content-in-the-repository)
- [Get Content from the Repository](#get-content-from-the-repository)
- [Identify Repository Content](#identify-repository-content)
- [Delete and Expire Content](#delete-and-expire-content)
- [MagickCache Is Not Just for Images](#magickcache-is-not-just-for-images)
- [Delete a Repository](#delete-a-repository)
- [Security](#security)
- [Portability](#portability)
- [MagickCache API](#magickcache-api)
- [ImageMagick Access](#imagemagick-access)

## Key Concepts

| Term | Meaning |
| --- | --- |
| **Repository** | A directory on your filesystem that holds resources (for example, `/opt/dmr`). |
| **Resource** | A single stored item: an image, a blob, or a piece of metadata. |
| **IRI** | The identifier for a resource, in the form `project/type/resource-path`. |
| **Passkey** | A secret that identifies you as the owner of a resource (or of the repository). |
| **Passphrase** | An optional secret used to scramble image content before it is stored. |
| **TTL** | An optional time-to-live after which a resource is considered expired. |

Resource types are `image`, `blob`, and `meta`.

## Create a Digital Media Repository

You need a location to store and retrieve your content. Create a repository on your local filesystem:

```sh
magick-cache -passkey ~/.passkey create /opt/dmr
```

Here, `~/.passkey` is a file containing your repository passkey. The passkey can contain any binary content, including a password, phrase, image, or random data. It is sensitive to every character, including control characters and trailing newlines.

One way to create a passkey without stray control characters is:

```sh
echo -n "myPasskey" > ~/.passkey
chmod 600 ~/.passkey
```

For best results, use a passkey of at least eight characters, and restrict access to the file as shown above. Keep it safe: without the passkey, you cannot identify, delete, or expire resources in the repository.

For the lowest latency and best performance, store your repository on an SSD.

You only need to create a repository once. A single repository can hold billions of images, videos, audio files, blobs, and metadata resources, but you may also create multiple repositories if you prefer to keep content separate.

## Put Content in the Repository

Add a cast photo to the repository:

```sh
magick-cache -passkey ~/.passkey put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20240508-rebecca-ferguson.jpg
```

The resource identifier is an IRI composed of `project/type/resource-path`. In this example, the project is `movies`, the type is `image`, and the resource path is `mission-impossible/cast/rebecca-ferguson`.

Each resource path should uniquely identify a resource. If you need multiple versions of the same image, give each version a distinct identifier.

### Assign a time-to-live

To have a resource expire after two days, add the `-ttl` option:

```sh
magick-cache -passkey ~/.passkey -ttl "2 days" put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20240508-rebecca-ferguson.jpg
```

Once the TTL has elapsed, the resource is marked as expired and can be removed with the `expire` command (see [Delete and Expire Content](#delete-and-expire-content)).

### Scramble image content

To prevent the repository owner from viewing image content, scramble the image with a passphrase:

```sh
magick-cache -passkey ~/.passkey -passphrase ~/.passphrase -ttl "2 days" put /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson 20240508-rebecca-ferguson.jpg
```

The same passphrase is required when retrieving the image.

Only image resources are scrambled. Blob and metadata resources are stored as-is. If you need additional privacy for those, encrypt or obfuscate them before storing them.

## Get Content from the Repository

To retrieve a resource:

```sh
magick-cache -passkey ~/.passkey get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

The original image was stored as a JPEG; it is converted to PNG on retrieval because of the output file extension.

### Extract a region

To retrieve only a portion of an image, provide a geometry with an offset:

```sh
magick-cache -passkey ~/.passkey -extract 100x100+0+0 get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

### Resize on retrieval

To resize the image instead, omit the offset:

```sh
magick-cache -passkey ~/.passkey -extract 100x100 get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

### Retrieve a scrambled image

If the image was scrambled, supply the same passphrase that was used to store it:

```sh
magick-cache -passkey ~/.passkey -passphrase ~/.passphrase get /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson rebecca-ferguson.png
```

## Identify Repository Content

List the resources under a path:

```sh
magick-cache -passkey ~/.passkey identify /opt/dmr movies/image/mission-impossible/cast
```

Each entry includes the IRI, image dimensions (for images), content size, TTL, expiration status, and creation date. Expired resources are marked with an asterisk (`*`).

MagickCache also supports wildcard resource types. Quote the IRI so your shell does not expand the asterisk:

```sh
magick-cache -passkey ~/.passkey identify /opt/dmr 'movies/*/mission-impossible/cast'
```

Other users may store content alongside yours in the same repository. However, you cannot get, identify, delete, or expire resources that were created with a different passkey.

The repository owner can access all resources:

```sh
magick-cache -passkey ~/.passkey identify /opt/dmr /
```

## Delete and Expire Content

Delete a specific resource:

```sh
magick-cache -passkey ~/.passkey delete /opt/dmr movies/image/mission-impossible/cast/rebecca-ferguson
```

Delete all expired resources under a path:

```sh
magick-cache -passkey ~/.passkey expire /opt/dmr movies/image/mission-impossible/cast
```

Expiration is not automatic: a resource whose TTL has elapsed remains in the repository, flagged as expired, until `expire` is run. Consider running it periodically, for example from a scheduled job.

## MagickCache Is Not Just for Images

You can store video, audio, metadata, or arbitrary files using the `blob` and `meta` resource types:

```sh
magick-cache -passkey ~/.passkey put /opt/dmr movies/blob/mission-impossible/cast/rebecca-ferguson 20240508-rebecca-ferguson.mp4
```

```sh
magick-cache -passkey ~/.passkey put /opt/dmr movies/meta/mission-impossible/cast/rebecca-ferguson 20240508-rebecca-ferguson.txt
```

| Type | Accepts |
| --- | --- |
| `image` | Any format supported by [ImageMagick](https://imagemagick.org/script/formats.php). |
| `meta` | Text metadata. |
| `blob` | Any text or binary content (video, audio, documents, and so on). |

## Delete a Repository

The repository owner can delete all content:

```sh
magick-cache -passkey ~/.passkey delete /opt/dmr /
```

**Use caution.** This operation permanently removes all repository content and cannot be undone.

## Security

MagickCache is not intended to provide cryptographic security. Instead, it generates resource identifiers from secure-quality hashes so that resource locations are difficult to predict.

- A resource can be accessed by the repository owner or by a user who holds the passkey it was stored with.
- Anyone with sufficient privileges to read the repository files directly on disk may also be able to access stored resources.
- For additional privacy, image content can be scrambled with a passphrase before storage. The same passphrase is required to retrieve the image.
- Blob and metadata resources are never scrambled. Encrypt them yourself if they are sensitive.
- Protect your passkey and passphrase files with restrictive file permissions, and keep backups: a lost passkey cannot be recovered.

## Portability

A MagickCache repository is self-contained and portable. You can move or copy it to another location or host and continue using it, provided you supply the same repository passkey.

## MagickCache API

Every command-line operation (create, put, get, identify, delete, and expire) is also available through the [MagickCache C API](https://github.com/ImageMagick/MagickCache).

## ImageMagick Access

ImageMagick can read and write MagickCache resources directly using the `dmr:` prefix. The examples below use the ImageMagick 7 `magick` command; with ImageMagick 6, substitute `convert`.

Retrieve a resource:

```sh
magick -define dmr:path=/opt/dmr -define dmr:passkey="$HOME/.passkey" \
  dmr:movies/image/mission-impossible/cast/rebecca-ferguson \
  rebecca-ferguson.png
```

Store or replace a resource:

```sh
magick rebecca-ferguson.png \
  -define dmr:path=/opt/dmr -define dmr:passkey="$HOME/.passkey" \
  dmr:movies/image/mission-impossible/cast/rebecca-ferguson
```

Store metadata by setting the `dmr:meta` define:

```sh
magick -define dmr:path=/opt/dmr -define dmr:passkey="$HOME/.passkey" \
  -define dmr:meta="Ilsa Faust" xc: \
  dmr:movies/meta/mission-impossible/cast/rebecca-ferguson
```
