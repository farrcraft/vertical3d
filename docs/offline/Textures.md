# Textures

moya reads a texture from a tiled, mipmapped OpenEXR file, which the texture maker writes
from images. `moya --maketexture` and RIB's `MakeTexture` both run the maker, and a render makes
any image a material reads that is not yet a texture. A render holds the tiles it reads in a
cache, within a limit of memory, and a material's image node looks a texture up over the shading
point's footprint. The code is `moya/libmoya/texture/` on the host, and
`moya/libmoya/kernel/texture/` for the lookup.

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

## Finding a texture

`texture::Textures` turns the images every material's program reads into textures, when the
surfaces are made for a render. Each image node names an image by its file, the colour space its
file input names, and whether its output is a colour.

- **A file is found** as given when it is absolute. A relative file is looked for in each
  directory of the options' `textureSearchPath` in turn, or in the folder moya runs from when the
  path is empty. Directories are separated by a colon or a semicolon, and a colon after a single
  letter at a directory's start belongs to a drive, as in `C:/maps`.
- **A UDIM image** holds `<UDIM>` in its file. Each tile from 1001 to 1100 that is found is read,
  ten to a row of v, so tile 1012 is the second column of the second row. The image's grid has as
  many rows as the last row with a tile, and a tile not found reads as nothing. A UDIM image of no
  tile is logged.
- **A file that is a texture already is read as it is**: a tiled EXR file of every level rounded
  down, read as data, or with ACEScg's chromaticities. Any other image is made into one, by the
  maker with a box of width 1, black wrap modes, and the image node's colour space, or as data.
- **A texture an image is made into lies in the options' `textureDirectory`**, or in
  `moya-textures` in the system's temporary directory. Its name is a hash of the image's bytes,
  its colour space and whether it is data. A texture made before from the same image is read
  again, and a changed image is made afresh.
- **An image not found, or one that cannot be made or opened, is logged**, and reads as nothing,
  so its node gives its default.
- **Images of one file read in one way share a texture.**

## The cache

`texture::Cache` holds the tiles a render reads, within the options' `textureMemory`, which RI's
`Option "limits" "texturememory"` gives in kilobytes, 1 GiB until a client sets it.

- **A texel's tile is loaded when the cache does not hold it**, and the tile used longest ago is
  dropped when the tiles held pass the limit. The tile just loaded is never dropped, so a limit
  below one tile holds one tile.
- **A tile is held as four floats for each texel**: grey as red, green and blue, an alpha of one
  where the file has none, and two channels as grey and alpha. The limit counts those floats.
- **A texel's value never depends on what the cache holds**, so a render gives the same picture
  with any limit.
- **Any number of threads read at once.** A thread loading a tile holds only its file's lock while
  it reads, and the cache's own lock while it finds or adds a tile.
- **A texel that cannot be read is zero**, and the render logs each file that failed once it
  ends.

## Looking up a texture

`kernel::texture::lookup()` reads an image at a texture coordinate, over the shading point's
footprint in `st`, with the image node's address modes and filter.

- **v runs up the texture**, as MaterialX places it, so a v of zero reads the bottom row.
- **A UDIM image reads the tile its coordinate's whole parts name**, at the coordinate's
  fractions. A coordinate outside the grid, or on a tile not found, gives the node's default.
- **The address modes are MaterialX's.** `constant` gives the default for a coordinate outside
  zero to one along its axis, and clamps a texel that a filter reaches past the edge. `clamp`
  reads the nearest edge texel, `periodic` repeats the texture, and `mirror` repeats it with
  every other copy reflected. They replace the file's `wrapmodes`.
- **`closest` reads the first level's texel the coordinate lies in.** `linear` and `cubic` both
  filter over the footprint.
- **The footprint's minor axis chooses the level**: its length in texels of the first level,
  as a power of two, between the two levels it falls between. Probes spread evenly along the
  major axis, as many as the axes' ratio, at most eight, and each is the two levels' bilinear
  values mixed. A footprint more than eight times as long as it is wide is read at the coarser
  level its major axis over eight gives. A point with no footprint reads the first level.
- **The footprint is the shading point's in `st`**, whatever coordinate the node reads, so a
  node whose coordinate is scaled, as a tiledimage's is, filters over the footprint of the
  unscaled coordinate.

## The texture access

Each stage that runs a material's program takes the device's texture access as a template
parameter, beside its traversal. The access holds the catalog of images, each image's grid of
textures and each texture's size, and gives one texel of a texture's level. The concept
`kernel::texture::Texels` names what it gives.

- **The CPU's access is `device::cpu::Textures`.** It reads each texel through a host function in
  libmoya, `texture::Cache::fetch()`, which loads the tile when the cache does not hold it. The
  CPU's libraries call it by its address and compile none of it.
- **`kernel::texture::Resident` holds every level of every texture in device memory.** It is the
  oracle every device's access is checked against.
- **`kernel::texture::Untextured` holds no image**, so every lookup gives its default. A caller
  that runs a program without textures uses it.

Background: [ADR-0225](../adr/offline/0225-offline-textures-are-tiled-exr-cached-by-tile.md),
[ADR-0229](../adr/offline/0229-offline-stages-take-their-texture-access-as-a-type.md)
