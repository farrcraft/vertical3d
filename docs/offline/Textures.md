# Textures

The new core reads a texture from a tiled, mipmapped OpenEXR file, which the texture maker writes
from an image. `moya --maketexture` and RIB's `MakeTexture` both run the maker. The code is
`moya/libmoya/texture/Maker.h`.

## The texture file

- **Tiled, of every level.** Tiles are 64 by 64 texels. The file holds every mip level, each half
  the one above along each side, rounded down and never below one texel, as OpenEXR's
  `MIPMAP_LEVELS` with `ROUND_DOWN` lays them out. The last level is one texel.
- **ZIP compressed.** Values are 32-bit floats when the image's are, and halves otherwise.
- **The channels.** A colour texture holds `R`, `G` and `B`, and `A` when the image has an alpha.
  A data texture keeps the image's channels: `Y` for one, `Y` and `A` for two, and `R`, `G`, `B`
  and `A` for three or four.
- **The wrap modes** are the string attribute `wrapmodes`, the s mode then the t mode, as
  `"periodic,clamp"`. Each is `black`, `clamp` or `periodic`. OpenImageIO reads the same
  attribute.
- **A colour texture's chromaticities are ACEScg's.** A data texture has none.
- **A texture is written beside its path first**, to the path with `.partial` after it, and moved
  into place, so a reader never sees half of one. A texture that fails to be made leaves no file.

## Reading the image

- **An OpenEXR file** gives its `R`, `G`, `B` and `A`, or its `Y` and `A`. A channel it lacks reads
  as zero. Its data window is read from its top left.
- **A PNG, JPEG, TGA or BMP file** gives the channels its reader gives, each of 8 bits, as values
  from 0 to 1. The PNG reader gives a grey image as `R`, `G` and `B`.
- **An image of neither kind**, or one that does not read, makes no texture.

## The colour space

A colour image is converted into ACEScg before its levels are filtered, so each level averages
light rather than encoded values. A data image is never converted.

- **A colour space that is named wins.** It is a name or role in the colour configuration, the
  built-in ACES one. A space that holds data, as `Raw` does, makes a data texture.
- **Otherwise an EXR file is in the linear space its chromaticities name**: AP0, AP1, Rec. 709,
  P3-D65, Rec. 2020 or Adobe RGB, each within 0.001. A file with none is in linear Rec. 709.
  Chromaticities of any other space are read as linear Rec. 709, and the log says so.
- **Otherwise an image is in sRGB**, the space of Color Interop Forum ID `srgb_rec709_scene`.
- **A grey colour image becomes `R`, `G` and `B`**, since its grey is converted as a colour.
- **An alpha is never converted.**

## Filtering the levels

Each level is made from the level above it, not from the image.

- **A texel is the filtered sum of the level above** about the texel's centre, divided by the sum
  of the filter's weights. The filter's widths are in texels of the level being made, so a box of
  width 1 averages the two by two texels it covers. Each width is above zero and at most 64.
- **The filters are RI's**: `box`, `triangle`, `catmull-rom`, `gaussian` and `sinc`, each with
  RI's formula and zero outside its width. A texel of the level above is weighed at its centre's
  offset.
- **A texel beyond the edge is found by the axis's wrap mode.** `periodic` reads the far edge,
  `clamp` the nearest edge texel, and `black` adds its weight and no value, so a black edge
  darkens.
- **A sum whose weights total zero takes the nearest texel of the level above**, as a filter
  narrower than a texel of the level above does.
- **A level's rows are made in parallel**, and each texel's sum is taken in one order, so a
  texture is the same file on every run.

## Making a texture

- **`moya --maketexture <image> <texture>`** makes one. `--swrap` and `--twrap` give the wrap
  modes, `black` when not given. `--filter` gives the filter, the box when not given, and
  `--swidth` and `--twidth` its widths, 1 when not given. `--colorspace` names the image's colour
  space. The moya manual's [CommandLine.md](../manual/moya/CommandLine.md) has the refusals.
- **RIB's `MakeTexture`** makes one at once, through the scene client, with the request's wrap
  modes, filter and widths. Its parameter `"string colorspace"` names the image's colour space. A
  wrap mode or filter RI does not name is logged, and the texture is made with `black` or the box.
  A texture that cannot be made is logged, and the file reads on.
- **A path is read as given**, relative to the folder moya runs from.
