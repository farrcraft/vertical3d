# RIB

How moya reads a RenderMan scene file. The terms used here are defined in
[README.md](README.md#glossary).

## RIB

The RIB reader is in `api/render/offline/rib`. It calls `offline::rib::Handler`, a C++
interface with typed parameter lists, not the RI C API. moya implements the handler as
`moya::RIBHandler`. The editor exports its scenes to RIB, one way; see [editor/](../editor/README.md).

### What is read

The reader recognises these requests:

| Group | Requests |
|---|---|
| Options | `version`, `Declare`, `Option`, `Hider`, `Format`, `FrameAspectRatio`, `ScreenWindow`, `CropWindow`, `Projection`, `Clipping`, `Display`, `Imager` |
| Sampling and lens | `PixelSamples`, `PixelFilter`, `PixelVariance`, `DepthOfField`, `Shutter` |
| Blocks | `FrameBegin`, `FrameEnd`, `WorldBegin`, `WorldEnd`, `AttributeBegin`, `AttributeEnd`, `TransformBegin`, `TransformEnd`, `MotionBegin`, `MotionEnd` |
| Transforms | `Identity`, `Transform`, `ConcatTransform`, `Translate`, `Rotate`, `Scale` |
| Attributes | `Color`, `Opacity`, `ShadingRate`, `Attribute` |
| Shaders and lights | `Surface`, `LightSource`, `AreaLightSource`, `Illuminate`, `MakeTexture` |
| Geometry | `Polygon`, `PointsPolygons`, `Sphere` |

moya acts on all of them except `CropWindow`, `Attribute`, `FrameBegin` and `FrameEnd`, which it
accepts and ignores. `Option` acts on `"limits"` (`bucketsize`, `gridsize`), `"searchpath"`
(`shader`, `texture`) and `"trace"` (`maxdepth`).

### Rules

- **Only ASCII RIB is read.** A binary or gzipped stream is rejected with a message naming
  which it is. A UTF-8 byte order mark at the start is skipped, and a stream is read from where
  it stands rather than from its beginning.
- **A size that is not a size is skipped with a warning.** A `Format` whose resolution is not
  between 1 and 65536 or whose aspect is not a number, a `FrameBegin` that is not a finite
  frame number, and a `PixelFilter` whose width is not positive are skipped, and the rest of
  the file reads. A `Format` aspect of zero or less asks for the device's own, which is square
  pixels.
- **An unrecognised request is reported once per name, and its arguments are skipped.**
  `Reader::unrecognised()` lists them. `Reader::unsupported()` lists what was read but not
  built, such as a deforming primitive.
- **Every handler method has an empty body, not a pure virtual one.** The RI standard requires
  a renderer to accept a request it does not support. A misspelled override then compiles
  silently, so every override carries `override`. A method that does not match an RI request
  does not belong on the interface.
- **A matrix crosses the RIB boundary by being read in order, not transposed.** RIB writes row
  major under a row vector convention. glm stores column major under a column vector
  convention. `glm::make_mat4` over the sixteen floats is therefore the whole conversion. This
  applies to `Transform`, `ConcatTransform`, `RtMatrix` and the editor's export alike.
- **A parameter is typed by a declaration, or it is dropped.** The reader declares RI's
  standard names, including `Option "trace"`'s `maxdepth` and `Option "searchpath"`'s `shader`
  and `texture`, and nothing else. A shader parameter outside that set needs `Declare` or an
  inline type, such as `"uniform float size" [0.5]`. Otherwise the shader runs with its default.
- **A `Polygon` carries no vertex count.** The count is the length of `"P"`, which the reader
  divides out. A parameter list is therefore parsed before the count is known, and an
  unbracketed varying or vertex parameter ends the parse instead of being guessed at.
- **A light handle may be a number or a string.** RIB 3.03 writes a number and later RIB
  writes a string.
- **A `PixelFilter` name the reader does not recognise is reported and never reaches the
  handler**, so the renderer keeps the filter it had. A `PixelSamples` rate reaches the handler
  as a count of at least one. `DepthOfField` with no arguments is a pinhole, sent as an
  infinite fstop.
- **`MakeTexture` is understood and makes nothing.** There is no `txmake`. The image a shader
  names is the texture, read through `image::Factory`, so a scene should name the image itself,
  not a converted file.
- **`AreaLightSource` becomes an ordinary light** unless a handler overrides
  `Handler::areaLightSource`. moya does not.
- **A primitive repeated inside a motion block is a deformation, which moya does not build.**
  The reader hands the later poses to `Handler::deformation()`, which returns null unless a
  renderer overrides it, and lists them in `Reader::unsupported()`. The primitive is drawn at
  the block's first time.
- **A polygon's `"st"` is its texture coordinates**, two floats per vertex. A traced triangle
  weights its corners' values by its barycentric coordinates. A triangle given none has `s` and
  `t` equal to those coordinates. The reyes hider interpolates `"st"` across a grid, as it does
  `"Cs"`, and carries it through a split. A grid whose corners have none uses its own
  parameters.
- **A `PointsPolygons` face with fewer than three vertices is skipped.** The first face whose
  indices run past the end of the index list ends the request.

Background: [ADR-0023](../adr/0023-offline-rib-is-the-scene-format.md),
[ADR-0025](../adr/0025-offline-rib-reader-calls-a-typed-handler-interface.md)
