# RIB

How moya reads a RenderMan scene file. The terms used here are defined in
[README.md](README.md#glossary).

## RIB

The RIB reader is in `api/render/offline/rib`. It calls `offline::rib::Handler`, a C++
interface with typed parameter lists, not the RI C API. A request that takes several values
of one type passes them as one struct named after the request, from `rib/Requests.h`, so a call
names each value: `handler->clipping({.hither = 0.1F, .yon = 100.0F})`. moya implements the
handler as `moya::RIBHandler`. The editor exports its scenes to RIB, one way; see
[editor/](../editor/README.md).

### What is read

The reader recognises these requests:

| Group | Requests |
|---|---|
| Options | `version`, `Declare`, `Option`, `Hider`, `Format`, `FrameAspectRatio`, `ScreenWindow`, `CropWindow`, `Projection`, `Clipping`, `Display`, `Imager` |
| Sampling and lens | `PixelSamples`, `PixelFilter`, `PixelVariance`, `DepthOfField`, `Shutter` |
| Blocks | `FrameBegin`, `FrameEnd`, `WorldBegin`, `WorldEnd`, `AttributeBegin`, `AttributeEnd`, `TransformBegin`, `TransformEnd`, `MotionBegin`, `MotionEnd` |
| Transforms | `Identity`, `Transform`, `ConcatTransform`, `Translate`, `Rotate`, `Scale` |
| Attributes | `Color`, `Opacity`, `ShadingRate`, `Attribute` |
| Shaders and lights | `Surface`, `Displacement`, `LightSource`, `AreaLightSource`, `Illuminate`, `MakeTexture` |
| Geometry | `Polygon`, `PointsPolygons`, `Sphere` |

moya acts on all of them except `FrameBegin` and `FrameEnd`, which it accepts and ignores.
`Option` acts on `"limits"` (`bucketsize`, `gridsize`), `"searchpath"` (`shader`, `texture`)
and `"trace"` (`maxdepth`). `Attribute` acts on `"light"` (`nsamples`) and
`"displacementbound"` (`sphere`, `coordinatesystem`), and reports any other attribute once by
name.

### Rules

- **Only ASCII RIB is read.** A binary or gzipped stream is rejected with a message naming
  which it is. A UTF-8 byte order mark at the start is skipped, and a stream is read from where
  it stands rather than from its beginning.
- **A request whose number cannot be used is skipped with a warning.** This covers a `Format`
  with a side that [Cameras and projections](CamerasAndSampling.md#cameras-and-projections)
  refuses. It also covers a `FrameBegin` whose frame number is not finite, and a `PixelFilter`
  whose width is not positive. Reading continues with the next request. `RiFormat` and
  `RiPixelFilter` in the C interface refuse the same values, and the context keeps what it had.
- **`Option "trace" "maxdepth"` is a depth from 0 to 16.** A larger depth is taken as 16, with
  a warning. A depth that is negative or not finite is not used, with a warning, and the depth
  stays as it was. A fraction is rounded down.
- **A count or a light handle that is too large is an error.** A count above 4294967040 or a
  numeric light handle beyond two billion stops the read, as any other malformed request does.
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
- **`Rotate` takes degrees, and a positive angle turns +z towards +x about y and towards -y
  about x.** This is the sign the unit cube in the RenderMan Interface specification's RIB entity
  file example needs to put each face where it names it. A moya case reads that cube and checks
  each face's centre.
- **A parameter is typed by a declaration, or it is dropped.** The reader declares RI's
  standard names, including `Option "trace"`'s `maxdepth` and `Option "searchpath"`'s `shader`
  and `texture`, and nothing else. A shader parameter outside that set needs `Declare` or an
  inline type, such as `"uniform float size" [0.5]`. Otherwise the shader runs with its default.
- **The C interface reads a parameter's values by its declared type.** An `integer` parameter's
  array holds `RtInt`s and a `string` parameter's holds `RtString`s. Every other type's holds
  `RtFloat`s.
- **A variadic C entry point forwards to its `V` form**, which does the work, so the two forms
  of a request cannot differ. `RiPolygon` and the RIB `Polygon` build their polygon through one
  function, and both read `"P"`, `"Cs"`, `"N"` and `"st"`.
- **No `Display` parameter is read.** Each one a scene gives is logged as not used.
- **`RiGetContext` returns a handle that lasts until `RiEnd` ends its context.** `RiContext`
  makes that context active again, and a handle to a context that has ended is ignored. `RiEnd`
  with no context begun does nothing.
- **A polygon sent before `WorldBegin` is not drawn.** The camera is fixed at `WorldBegin`, so
  the polygon cannot be placed. The first one is logged.
- **A coordinate system that is neither predefined nor saved is refused.** `CoordSysTransform`
  logs it and keeps the current transformation, and a lookup of it logs and gives the identity.
- **A `Sphere`'s varying parameters hold four values**, one at each corner of its parameter
  space, as RI defines them for a quadric. A uniform one holds one. moya reads no sphere
  parameter.
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
- **`Attribute "displacementbound"` is a distance in object space**, or in the space its
  `"string coordinatesystem"` names: `"object"`, `"world"`, `"camera"`, or one the scene saved
  with `RiCoordinateSystem`. moya keeps it in camera space, scaled by the largest stretch of the
  space's axes. A space the scene never named is reported, and object space is used. The RIB
  reader does not read `CoordinateSystem`, so a file can name only the three predefined spaces.
- **`AreaLightSource` becomes an ordinary light** unless a handler overrides
  `Handler::areaLightSource`. moya overrides it, and `RiAreaLightSource` does the same.
- **An area light keeps the geometry declared after it in the same attribute block.** Every
  polygon and sphere there joins it, in world space, whichever hider draws them, and also
  renders as a surface. A sphere joins whole, whatever slab and sweep it was cut to. Geometry
  after the block does not join. The block's end also switches the light off, as it does any
  light made inside one. `Illuminate` switches it on again for what follows.
- **A primitive repeated inside a motion block is a deformation, which moya does not build.**
  The reader hands the later poses to `Handler::deformation()`, which returns null unless a
  renderer overrides it, and lists them in `Reader::unsupported()`. The primitive is drawn at
  the block's first time.
- **A polygon's `"st"` is its texture coordinates**, two floats per vertex. A traced triangle
  weights its corners' values by its barycentric coordinates. A triangle given none has `s` and
  `t` equal to those coordinates. The reyes hider interpolates `"st"` across a grid, as it does
  `"Cs"`, and carries both through a split. A grid whose corners have none uses its own
  parameters.
- **A `PointsPolygons` face with fewer than three vertices is skipped.** The first face whose
  indices run past the end of the index list ends the request.

Background: [ADR-0023](../adr/0023-offline-rib-is-the-scene-format.md),
[ADR-0025](../adr/0025-offline-rib-reader-calls-a-typed-handler-interface.md)
