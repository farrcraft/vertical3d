# Output

This page is for anyone changing how moya's new core writes its pictures to files and shows
them on a display. The code is
host code, in `moya/libmoya/output/` and the namespace `v3d::moya::output`. It writes a film's
snapshot, which [The film](Film.md) describes.

## A scene's outputs

`output/Outputs.h` reads what a scene's `output` and `channel` objects ask for, which
[Scene.md](Scene.md) describes, and writes it.

- **`channelsOf(scene)` gives every channel an output names**, each once, in the order the outputs
  name them. A channel two outputs name is one film channel. A channel no output names is not
  rendered.
- **The render's film holds those channels.** `integrate::render()` makes its film from
  `filmChannelsOf(scene)`. A channel whose source the path tracer does not fill is reported, and
  holds zero, as [Integrator.md](Integrator.md#values-at-the-first-hit) says.
- **`outputsOf(scene)` gives the outputs**, in the order the scene holds them, with each one's
  file, format, channels, precision and compression.
- **`write(output, snapshot, config, logger)` writes one.** An EXR output is one part of the
  beauty followed by its channels, at its precision and compression. A PNG output is the beauty
  through the configuration's default display and view. A framebuffer is logged and not written,
  since moya has no window.
- **`moya --paths` writes every output after each frame.** Its `--output` replaces the first
  output's file, and the file's name gives its format. A scene with no output writes its beauty
  to `--output` or `paths.exr`, in floats.

## EXR files

`writeExr(path, snapshot, parts, metadata)` writes a snapshot to an OpenEXR file. The path is
UTF-8.

- **A file holds one or more parts.** Each `ExrPart` names the outputs it holds, in order, how
  they are stored and how they are compressed. Every part covers the snapshot's whole
  resolution.
- **One part is written as a plain scanline file**, which every EXR reader opens. Several are
  written as a multi-part file, and each then needs a name of its own.
- **An output is the beauty, the count of samples or a channel.** `kBeauty`, "beauty", is the
  beauty, and `kSampleCount`, "sampleCount", each pixel's count of samples. A snapshot that holds
  no count for each pixel refuses it. Any other output is the name of one of the film's channels.
- **Precision is per part**: `Precision::Half` stores 16-bit floats and `Precision::Float` 32-bit
  ones. A half part converts the snapshot's floats to halves before it writes them, because
  OpenEXR converts between pixel types only when it reads a file. To store the beauty as halves
  and depth as floats, put them in two parts.
- **Compression is per part**, any of OpenEXR's, with `Compression::Zip` the default.
- **A part's `floats` are written as 32-bit floats whatever its precision.** A Cryptomatte
  layer's ranks are, since an ID read as a half loses its hash.

## Channel names

- **The beauty is R, G, B and A**, its alpha the share of its samples that met something, as
  [Film.md](Film.md#the-films-pixels) says.
- **The count of samples is `sampleCount`**, as a float.
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
- **Each string of `ExrMetadata::strings`**, by its name.

## Cryptomatte

An output of a Cryptomatte layer writes it as the Cryptomatte specification lays it out.

- **Its ranks are the channels `<layer>00.R` to `.A`, `<layer>01` and on**, of 32-bit floats,
  beside a beauty of the output's precision.
- **An ID is a name's 32-bit MurmurHash3**, `output::idOf()`, with the lowest bit of its exponent
  flipped when the exponent is all zeros or all ones. Its float is never a NaN, an infinity or a
  denormal.
- **The header names each layer under its key**, the first seven hex digits of the layer name's
  hash. It writes `cryptomatte/<key>/name`, `hash` as `MurmurHash3_32` and `conversion` as
  `uint32_to_float32`. Its `manifest` is a JSON object of each name in the picture and its ID, as
  eight hex digits.

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
