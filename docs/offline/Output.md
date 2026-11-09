# Output

This page is for anyone changing how moya's new core writes its pictures to files and shows
them on a display. The code is
host code, in `moya/libmoya/output/` and the namespace `v3d::moya::output`. It writes a film's
snapshot, which [The film](Film.md) describes.

## EXR files

`writeExr(path, snapshot, parts, metadata)` writes a snapshot to an OpenEXR file. The path is
UTF-8.

- **A file holds one or more parts.** Each `ExrPart` names the outputs it holds, in order, how
  they are stored and how they are compressed. Every part covers the snapshot's whole
  resolution.
- **One part is written as a plain scanline file**, which every EXR reader opens. Several are
  written as a multi-part file, and each then needs a name of its own.
- **An output is the beauty or a channel.** `kBeauty`, "beauty", is the beauty. Any other output
  is the name of one of the film's channels.
- **Precision is per part**: `Precision::Half` stores 16-bit floats and `Precision::Float` 32-bit
  ones. A half part converts the snapshot's floats to halves before it writes them, because
  OpenEXR converts between pixel types only when it reads a file. To store the beauty as halves
  and depth as floats, put them in two parts.
- **Compression is per part**, any of OpenEXR's, with `Compression::Zip` the default.

## Channel names

- **The beauty is R, G and B.** The film has no coverage yet, so there is no A.
- **A channel of one float** keeps the channel's name, such as `depth`.
- **A channel of several floats** is the channel's name followed by `.R`, `.G`, `.B` and `.A` in
  order, such as `albedo.R`, `albedo.G` and `albedo.B`.

## The header

Every part's header holds:

- **`colorInteropID`**, the Color Interop Forum ID that `ExrMetadata::colourSpace` gives. It
  is `lin_ap1_scene` unless set, since the film holds ACEScg. An output converted into the working
  space by [`colour::Config::output()`](Colour.md) names the working space's ID instead. An
  empty ID writes nothing.
- **The chromaticities of the space**, when its ID is one of the linear spaces with standard
  primaries: AP0, AP1, Rec. 709, P3-D65, Rec. 2020 and Adobe RGB. The ID of any other space is
  written without them.
- **`worldToCamera` and `worldToNDC`**, OpenEXR's standard matrices, when `ExrMetadata` gives
  them. A glm matrix is written with each float at the same indices. glm multiplies column vectors
  and Imath multiplies row vectors, so the matrix reads back in Imath as the same transform.
- **`moya/renderTime`**, a float of seconds, when `ExrMetadata` gives one.

## 8-bit pictures

`displayed(snapshot, display)` gives a snapshot's beauty as a display shows it, for a PNG or the
editor's render view. `writePng(path, snapshot, display, logger)` writes it to a PNG file.

- **The display is a `colour::Transform`** from [`colour::Config::display()`](Colour.md), from the
  film's ACEScg through a display's view.
- **Each channel is 8 bits.** A value is clamped to [0, 1] and rounded to the nearest of 256 steps,
  and a NaN is zero.
- **The bytes are red, green and blue** for each pixel, row by row from the top left.
- **`writePng` throws `std::runtime_error`** when the picture does not encode or the file cannot
  be written.

## Errors

`writeExr` throws `std::invalid_argument` when:

- there are no parts;
- a part holds no outputs, or one output twice;
- an output is neither the beauty nor one of the snapshot's channels;
- two of a part's channels have the same name in the file, such as a channel named `R` beside the
  beauty, or a channel named `albedo.R` beside a three-float `albedo`;
- a file of several parts has a part with no name, or two parts with the same name;
- the snapshot's values do not fill its resolution.

It checks all of these before it opens the file, so a refused call writes nothing. OpenEXR's own
exceptions, which derive from `std::exception`, report a file that cannot be written.
