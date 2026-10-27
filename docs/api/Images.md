# Images

This page is for someone writing an app against the api. It covers `api/image`: the image
type, reading, writing and comparing images, the texture atlas, and packing a sprite sheet.
Terms are defined in the [glossary](README.md#glossary).

`api/image` (`v3d::image`) reads, writes and compares images. It depends on libpng and libjpeg
and on no device.

## Image

`image::Image` ([api/image/Image.h](../../api/image/Image.h)) is a buffer of pixels with a width,
a height and a bit depth.

- **Row 0 is the top of the picture.** Every reader produces this order, and the canvas, the
  texture upload and the atlas packer all expect it.
- **`format()` is the channel count:** `Format::Grey` (1), `RGB` (3) or `RGBA` (4). It follows
  the depth.
- **An `Image` cannot be copied.** It owns its buffer. Hold one through
  `boost::shared_ptr<Image>`, and use `image::crop()` to make a copy of all or part of one.

## Reading and writing

`image::Factory` ([api/image/Factory.h](../../api/image/Factory.h)) reads and encodes by
format: `png`, `jpg`, `jpeg`, `bmp` and `tga`. **It writes no file.** It encodes an image to
bytes, and the caller writes them through `asset::writeFile`, which replaces a file atomically.

```cpp
v3d::image::Factory factory(logger);
boost::shared_ptr<v3d::image::Image> picture = factory.read("sprite.png");
const std::optional<std::string> bytes = factory.encode(picture, v3d::image::Factory::kind("out.tga"));
if (bytes) {
    v3d::asset::writeFile("out.tga", *bytes);
}
```

- `read(data, size, kind)` decodes from memory. Pass the format key, such as `"png"`, because a
  buffer has no extension. A glTF file's embedded images are read this way.
- Every reader returns rows top to bottom. A bmp is read at 8, 16, 24 or 32 bits, uncompressed
  or, at 16 and 32 bits, packed by the masks its header gives. 16 bits uncompressed is five bits
  a channel. 32 bits reads as RGBA, and as opaque when its alpha is zero everywhere, since most
  writers leave it unused. Every other depth comes back as RGB.
- Each read returns null when the file will not open or does not decode.
- `kind(filename)` is the format key a file name gives: its extension, lower case, without the
  dot.
- `encode` returns nothing when no writer has the key or the image does not encode. An error
  from libpng or libjpeg is logged, and the encode returns nothing rather than ending the
  process. A TGA is encoded with the 26-byte TGA 2.0 footer.
- Through the asset manager, an image file loads as `asset::media::kind::Image`, and `image()`
  returns the `image::Image`. The loader reads the file through `asset::readFile` and decodes it
  from memory.

**The readers do not agree on greyscale.** Know which you are reading:

| Format | A greyscale file reads as |
|---|---|
| png | RGB. The reader has libpng expand grey to RGB. |
| bmp | RGB. An 8-bit file's indices are resolved through its palette. |
| jpeg | RGB. The reader always has the decoder output RGB. |
| tga | Grey, at the file's own depth. |

**Every writer can write greyscale**: png as a grey image, tga as type 3, jpeg as
`JCS_GRAYSCALE`, and bmp as 8-bit indices into a 256-entry grey ramp.

## Other functions

- `crop(source, x, y, width, height)` cuts a rectangle into a new image at the same depth. It
  returns null when the rectangle is empty or not wholly inside the source. Use it to cut a
  sprite back out of a packed sheet.
- `swapRedBlue(from, to, {.pixels, .channels})` swaps the first and third bytes of each pixel, for
  converting between RGB and the BGR order BMP and TGA store. The buffers may be the same.
- `compare(a, b, tolerance)` compares two images channel by channel and returns a
  `Difference`. It records the worst pixel even when the images match. `description()` gives
  one line saying what differed and where. Keep the tolerance for float rounding between
  compilers, not for "close enough".

## Texture atlas

`image::TextureAtlas` ([api/image/TextureAtlas.h](../../api/image/TextureAtlas.h)) packs many
small images into one larger image, so they can share one texture.

```cpp
v3d::image::TextureAtlas atlas(512, 512, 4, logger);     // width, height, bytes per texel
glm::ivec4 r = atlas.region(32, 32);                     // the size you will blit
if (r.x < 0) { /* the atlas is full */ }
atlas.region(r.x, r.y, r.z, r.w, pixels, 32 * 4);        // blit into it
```

- **Ask for the size you will blit, and blit into exactly the rectangle you get back.** The
  atlas reserves a one-texel gutter on all four sides of every region itself. Do not add the
  gutter to what you ask for or subtract it from what you get.
- `region(width, height)` returns `x`, `y`, width and height of the usable rectangle, or an `x`
  and `y` of -1 when there is no room left.
- The sheet also keeps a one-texel border around its edge.
- **The gutter is left as zero:** black at depth 3, transparent black at depth 4. Edge texels
  are not copied into it. A linear sampler reading exactly at a region's edge therefore fades
  slightly towards the gutter. If that shows, inset the uvs by half a texel, as tetris does.
- Each region costs two texels more per axis than its size. The largest region a 16 × 16 atlas
  can hold is 12 × 12.
- `image()` returns the packed image, for upload or to encode.

Without a gutter, a linear sampler at a region's edge reads the neighbouring region's texels.
The visible result is a thin line of the wrong colour along a sprite's edge, with nothing in the
log.

### Packing a sprite sheet

`image::pack()` and `image::unpack()` ([api/image/Pack.h](../../api/image/Pack.h)) move named
images into one sheet and back out of it. They work in memory: the caller reads and writes the
files, and the sheet's document is `asset::media::SpriteSheets`.

```cpp
v3d::image::Packing packing = v3d::image::pack(sprites, 4096, logger);   // named images
if (!packing.sheet) { /* packing.refusal says why */ }
v3d::image::Unpacking cut = v3d::image::unpack(*packing.sheet, packing.placements);
```

- **`pack()` makes the smallest square sheet that holds every sprite.** It tries 16 pixels a
  side, doubles until the sprites fit, and tries the largest side last if that is not a power
  of two. Each sprite gets the atlas's one-texel gutter.
- The placements come back in the order the sprites were given. The tallest sprites are placed
  first, and sprites of one height keep their order, so one set of sprites always packs the same
  sheet.
- `pack()` refuses some sprites, with a reason in `refusal` and no sheet. It refuses no sprites,
  a sprite with no pixels, and two sprites of one name. It refuses sprites of different depths,
  or of a depth other than 8, 24 or 32 bits. It refuses a largest side under 16, and sprites
  that do not fit in the largest side.
- `unpack()` crops each placement out of the sheet. A placement that is empty or not wholly
  inside the sheet refuses the whole call, with a reason, and nothing is cut.
