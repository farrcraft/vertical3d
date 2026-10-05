# The offline renderer

This document is for anyone changing `moya` or the library under it, `api/render/offline`. It
covers how to run moya, how a RIB scene becomes an image, and the rules each stage follows.

moya is the only offline renderer in the tree. It reads a RenderMan scene file and writes an
image file. It shares no code with the realtime Vulkan renderer: nothing it links names Vulkan
or SDL, and it opens no window. Its test suites therefore run in CI on machines with no GPU.

- [Running moya](#running-moya)
- [How a frame is made](#how-a-frame-is-made)
- [Framebuffers and raster space](#framebuffers-and-raster-space)
- [Sampling and the film](#sampling-and-the-film)
- [Cameras and projections](#cameras-and-projections)
- [RIB](#rib)
- [The shading language](#the-shading-language)
- [Normals](#normals)
- [The reyes hider](#the-reyes-hider)
- [Ray tracing](#ray-tracing)
- [Depth of field](#depth-of-field)
- [Motion blur](#motion-blur)
- [Adaptive sampling](#adaptive-sampling)
- [What is not built](#what-is-not-built)
- [Building and testing](#building-and-testing)

## Running moya

moya is three CMake targets, and it is built on one api library:

| Directory | Target | Contents |
|---|---|---|
| `moya/libmoya` | `v3dlib_moya` | The renderer: graphics state, both hiders, the RI C API |
| `moya/moya` | `moya` | The command-line driver |
| `moya/tests` | `v3dtest_moya` | The test suite, ctest name `moya` |
| `api/render/offline` | `v3dlib_render_offline` | The RIB reader, the shading language, sampling and the film, textures and the ray tracer. Namespace `v3d::render::offline` |

Run the driver from its own build directory:

```
cd out/build/x64-Debug/moya/moya
moya.exe --file data/raytrace-scene.rib --output scene.png
```

| Option | Meaning |
|---|---|
| `--file <path>` | The RIB scene to render, relative to the working directory. Without it, moya prints the help and exits |
| `--output <path>` | The image to write. It replaces whatever file the scene's `Display` names |
| `--grid <n>` | Micropolygons per grid. Default 256 |
| `--bucket <n>` | Bucket size, n by n pixels. Default 16 |
| `--help` | Print the options |
| `--version` | Print the version and Pixar's RenderMan copyright notice |

- **Run moya from its own directory.** Started anywhere else, Windows cannot find its DLLs and
  shows a modal dialog. The process then waits on the dialog and uses no CPU.
- **Two demo scenes are copied beside the executable** at build time, from `moya/moya/data/`:
  `test-scene-01.rib` (two quads under the reyes hider) and `raytrace-scene.rib` (a metal and a
  glass sphere under the ray hider). Neither names a `Display`, so pass `--output` to get a file.
- **moya writes a file only when one is named**, by `--output` or by
  `Display "name.png" "file" "rgb"`. A `Display` of any other type writes nothing.
- **The output format comes from the file extension**: bmp, jpeg, png or tga, written through
  `image::Factory`. The image is always three colour channels, whatever mode `Display` names.
- **The picture is written when `WorldEnd` is read**, as the RI standard specifies.
- **`--grid` and `--bucket` are applied before the scene is read.** An `Option "limits"` in the
  scene replaces them.
- A scene that does not parse prints `error reading rib file` with the reason and exits with a
  failure code. Any exception is printed to stderr.
- A Debug build is slow. A 256 by 192 ray traced scene takes about a minute.

moya is not RenderMan compliant, and does not aim to be. `moya/libmoya/Renderer.h` explains
why: a second API beside the RI C API breaks the standard's "one true API" clause.

## How a frame is made

### Glossary

- **RI** (RenderMan Interface): the C API a RenderMan renderer exposes, such as `RiPolygon`
  and `RiSurface`. moya declares all of it in `moya/libmoya/RenderMan.h`.
- **RIB** (RenderMan Interface Bytestream): a text file of RI calls, one **request** per call,
  such as `Polygon "P" [...]`. It is the scene format moya reads.
- **Parameter list**: the name and value pairs that end a request, such as `"Cs" [1 0 0]`. Each
  name has a type, set by a declaration.
- **RIB reader**: `offline::rib::Reader`. It tokenises a RIB file and calls one handler method
  per request.
- **Handler**: `offline::rib::Handler`, a C++ interface with one virtual method per request.
  moya implements it as `moya::RIBHandler`.
- **Graphics state**: the current transformation, colour, opacity, surface shader and lights,
  plus the frame's options. `moya::RenderContext` holds it.
- **Primitive**: one piece of geometry, a polygon or a sphere.
- **Hider**: the stage that decides what is visible at each sample. moya has two, selected by
  the `Hider` request: `"hidden"` (the reyes hider, and RI's default) and `"raytrace"`.
- **Bucket**: a rectangle of pixels, 16 by 16 by default. The reyes hider sorts primitives into
  buckets and renders one bucket at a time.
- **Split**: cut a primitive that is too large into smaller primitives.
- **Dice**: turn a primitive into a grid.
- **Grid**: a regular mesh of shading points made from one primitive.
- **Micropolygon**: one cell of a grid, about one pixel across or smaller.
- **Shading language (SL)**: RenderMan's C-like language for shaders. moya compiles a subset of
  it at run time and interprets the result.
- **Shader**: a program written in SL. moya runs surface, light and imager shaders.
- **Batch**: the set of shading points one shader run covers: a grid's vertices, one ray hit,
  or a row of pixels.
- **Mask**: one flag per point in a batch, saying which points execute the current statement.
- **Uniform and varying**: a uniform value is one value for the whole batch. A varying value has
  one value per point.
- **Sample**: a point inside a pixel where visibility is decided. It also carries a time within
  the shutter and a point on the lens.
- **Film**: `offline::Film`. It weights samples into pixels with a pixel filter.
- **Pixel filter**: the weighting function: box, triangle, catmull-rom, gaussian or sinc.
- **Imager**: a shader run over the finished picture, one row of pixels at a time.
- **Raster space**: pixel coordinates, with the origin at the image's upper left.

### The pipeline

```
scene.rib -> rib::Reader -> moya::RIBHandler -> moya::RenderContext (graphics state)
                                                      |  WorldEnd
                                         ReyesHider ("hidden") or RayHider ("raytrace")
                                                      |  samples
                                   offline::Film -> planes -> imager -> image file
```

1. The reader parses each request and calls the matching `RIBHandler` method, with its
   parameter list already typed.
2. The handler updates the graphics state in `RenderContext`. At `WorldBegin` the context saves
   the camera and freezes the options. Each primitive is added to the traced scene, and under
   the reyes hider it is also filed in a bucket.
3. At `WorldEnd` the hider renders. The reyes hider splits, dices, shades and hides each
   bucket's primitives into a frame of samples. The ray hider casts a primary ray through each
   sample.
4. The film filters the samples into the framebuffer's planes.
5. The imager shader, if the scene names one, runs over the planes.
6. The colour planes are written to the output file.

**There are two ways into a `RenderContext`**: the RI C entry points in `RenderMan.cxx`, and
`RIBHandler`. Neither calls the other. The reader cannot use the C API, because most RI
functions take varargs and a `va_list` cannot be built at run time. Every C entry point has a
definition, but many are empty, including every `V` form (`RiPolygonV`, `RiSphereV` and the
rest). The RI token table in `RenderMan.cxx` is `RtToken`, that is, pointers, and most of it is
uninitialised. A null token reaches the context as an empty string, not as an error.

### The two hiders

**A hider is a `moya::Hider`.** It reports whether it sees only the traced scene
(`traces()`), and it renders what the context gathered (`render()`). `RenderContext` holds one.
`"hidden"` is `moya::ReyesHider` and `"raytrace"` is `moya::RayHider`. A hider name moya does
not recognise is logged, and the current hider stays.

Both hiders read one graphics state and one camera, write one framebuffer, and share the film,
the shading language and the ray tracer. They differ in these ways:

| | Reyes (`"hidden"`) | Ray (`"raytrace"`) |
|---|---|---|
| Default | Yes | No |
| Spheres | Not drawn. The first in a scene is logged | Intersected exactly |
| A shader's `Oi` | Ignored: every sample is opaque | Composited front to back |
| A varying `"Cs"` | Interpolated across the grid | Ignored: the primitive's colour is used |
| `PixelVariance` | Ignored | Adds samples where a pixel is noisy |
| Time of shadow and traced rays | Shutter open | Each sample's own time |
| Storage of a primitive | Twice: in a bucket and in the traced scene | Once, in the traced scene |

Background: [ADR-0078](adr/0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md)

## Framebuffers and raster space

- **`offline::FrameBuffer` is a stack of float planes**, each the size of the image.
  `image(channels)` takes the leading planes as the picture and ignores the rest. The plane
  count is not the channel count.
- **moya's planes are red, green, blue, depth and coverage**, named by
  `moya::FrameBuffer::Plane`, under either hider.
- **Coverage is the filtered fraction of a pixel's samples that hit something.** An imager
  reads it as `alpha`. It separates a pixel nothing was drawn into from a black one. It is
  exactly zero or one only at one sample per pixel under a one-pixel box filter.
- **A value outside [0, 1] saturates** when the planes become an image.
- **Raster y runs downward from the upper left.** This is RI's convention and `image::Image`'s
  row order.
- **Raster space is `raster * screen` applied to a camera space point**: the projection first,
  then the scale into pixels. A matrix applies to the point on its right. Reversing either the
  y direction or the order writes a correct render upside down, or in eye units.
- **A depth is raster space z under either hider**: the projection, then the raster matrix. A
  ray hit is carried back into camera space and projected the same way.

## Sampling and the film

A pixel's colour is a filtered set of samples. Three classes divide the work:

- `offline::Sampling` holds what `PixelSamples`, `PixelFilter`, `PixelVariance`, `Shutter` and
  `DepthOfField` set. Every field starts at the RI default.
- `offline::Sampler` gives a pixel's samples: a raster position, a time and a lens point.
- `offline::Film` filters samples into pixels and writes them into a renderer's planes.

Both hiders render through them.

- **The RI defaults are two by two samples per pixel under a gaussian filter two pixels wide.**
  A scene that names neither is antialiased, and four times slower than one sample per pixel.
- **Samples are stratified and jittered**, over a grid the size of `PixelSamples`.
- **A pixel's samples are seeded by its column and row**, through a `type::Random`. A frame is
  the same on every run, every standard library, and any order of pixels or buckets.
- **An axis with one stratum is sampled at the pixel centre**, not jittered. One sample under a
  one-pixel box therefore reproduces a render taken at pixel centres exactly.
- **A `PixelSamples` rate below one still takes one sample.**
- **A sample is filtered into every pixel whose centre lies within the filter's width**, as it
  arrives. The film holds a weighted sum per pixel, not the samples.
- **The filters use RI's formulas**, cut off at the width the scene gives. Catmull-rom peaks at
  two and has a negative lobe, so a pixel's weights can sum to nearly zero. The film writes
  black there instead of dividing.
- **A filtered colour is premultiplied by opacity**, as `Ci` is.
- **A miss carries a colour into the film.** Under the ray hider it is
  `trace::Scene::background()`, which is black unless something sets it. Coverage counts only
  hits.
- **A depth is not filtered.** It is the nearest hit among the samples inside the pixel itself,
  because a blend of two surfaces' depths lies on neither surface. A pixel no sample hit keeps
  the value its depth plane held.
- **A reference image that pins hiding or shading names `PixelSamples 1 1` and
  `PixelFilter "box" 1 1`**, in its test code and in its `.rib`. `reference-sampled.png` and
  `raytrace-sampled.png` pin the defaults.

Background: [ADR-0076](adr/0076-offline-seeded-samples-resolved-by-one-shared-film.md)

## Cameras and projections

- **The defaults are moya's**: a 320 by 240 image, pixel aspect 1, an orthographic projection,
  a 90 degree field of view for a perspective one, and clipping from `1e-10` to `1e38`.
- **The frame aspect follows `Format` and the screen window follows the frame aspect**, unless
  the scene names either. A scene can set `FrameAspectRatio` or `ScreenWindow` without the other
  reverting it. The default screen window for an image wider than tall is `[-a, a]` by
  `[-1, 1]`, where `a` is the frame aspect.
- **The world to camera transformation applies as it stands.** `RenderContext::prepareWorld()`
  saves the current transformation as the `"camera"` coordinate system. By the RI standard,
  that transformation is the world to camera one. A transpose or an inverse of it is correct
  only when it is a pure rotation.
- **Both hiders project through the same coordinate systems**, `"camera"`, `"screen"` and
  `"raster"`. Any camera one hider accepts, the other accepts: an off-centre `ScreenWindow`, a
  matrix that scales, and a matrix that reverses handedness. RI's camera basis for a general
  look-at reverses handedness.
- **The ray hider finds a primary ray by inverting the camera to raster transformation.** It
  solves for camera x and y at a chosen depth, instead of inverting the whole matrix, because
  RI's default clipping range ruins the depth terms of a full inverse. Rays start on the near
  plane.
- **The frustum cull uses `type::geometry::Frustum` with `Depth::MinusOneToOne`**, the depth
  range moya's projection produces.

## RIB

The RIB reader is in `api/render/offline/rib`. It calls `offline::rib::Handler`, a C++
interface with typed parameter lists, not the RI C API. moya implements the handler as
`moya::RIBHandler`. The editor exports its scenes to RIB, one way; see [Editor.md](Editor.md).

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
  which it is.
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

Background: [ADR-0023](adr/0023-offline-rib-is-the-scene-format.md),
[ADR-0025](adr/0025-offline-rib-reader-calls-a-typed-handler-interface.md)

## The shading language

Shading is a language, not a set of built-in models. A scene names a shader with `Surface`,
`LightSource` or `Imager`, and moya compiles that shader from SL source the first time it is
named. The language is in `api/render/offline/sl`.

### From source to a run

| Stage | Class |
|---|---|
| Lexing | `sl::Lexer` |
| Parsing into a syntax tree | `sl::Parser`, building `sl::syntax` nodes |
| Type checking | `sl::Types`, `sl::Compiler` |
| Uniform and varying inference | `sl::Inference` |
| Flattening into instructions | `sl::Emitter`, producing a `sl::runtime::Program` |
| Running over a batch | `sl::runtime::Machine` |
| Finding a shader by name | `sl::ShaderLibrary` |
| Binding a scene's parameters | `sl::Instance` |
| Writing the globals into a batch | `sl::Globals`, filled from an `sl::Point` per shading point |

- **The SL lexer matches the RIB lexer.** Both are a `peek` and `next` lexer over an
  `std::istream`, with an `error()` that ends the stream and a line and column on every token.
  The shared parts are in `api/render/offline`: `offline::Characters` reads the stream with its
  position and decodes string escapes, and each language's `Token` is an `offline::Lexeme` over
  its own kinds.
- **The keywords are a closed set**: the five shader types, the eight data types, the two
  storage classes, the control flow keywords and the three lighting constructs. Any other name
  is an identifier, so a shader may declare a variable called `output` or define its own
  `noise`.
- **`.` is a dot product and `^` is a cross product**, and both bind tighter than `*`. Check
  this before assuming an expression means what it looks like.
- **All five shader types parse, and three run.** A `displacement` or `volume` shader parses and
  returns false from `sl::syntax::Shader::supported()`. A scene that names one is told the
  shader is unsupported, not that it is malformed.
- **The C preprocessor is not run.** A `#` produces a diagnostic naming the missing
  preprocessor. There is no compiled-shader file: a shader is source, compiled on first use.
- **The compiler annotates the syntax tree in place.** Every expression gets a type and a
  storage class, and every variable gets a symbol index. `sl::Compiler::symbols()` is the list
  a machine allocates registers against. A local declared twice in nested scopes is two
  symbols.
- **`syntax::forEachChild` lists a node's children in source order.** A pass that treats most
  node kinds alike handles only the kinds it needs. A new kind of node needs one case there.
- **The varying inference runs to a fixed point**, because a loop can carry a varying value back
  to a name that was read before it was written. Anything assigned under a varying condition is
  varying. A value declared `uniform` that a varying value reaches is an error, not a silent
  widening. Inferring uniform where varying was correct gives a whole grid one point's answer.
  It looks like a shading bug and is a compiler bug.
- **A run covers a batch, and a batch of one is not a special case.** The machine runs a flat
  program under a stack of execution masks. A condition all points agree on compiles to a jump.
  A condition they disagree on runs both branches, each with the points that took it. `break`,
  `continue` and `return` clear mask bits instead of jumping. A grid's batch is its vertices, a
  traced hit's batch is one point, and an imager's batch is a row of pixels.
- **A function call is inlined.** A run has no call stack and no register file per call. A
  shader's own function is pasted in at each call, bracketed so that a `return` inside it ends
  the function for those points, not the shader. Recursion is rejected at the call graph, so
  inlining terminates.

### Shaders, lights and spaces

- **A shader is found by name.** A `.sl` file on `Option "searchpath" "shader"` wins over a
  built-in shader of the same name, so a scene can replace one. A file holding exactly one
  shader may have any file name: RI identifies a shader by the name in its source.
- **A search path is colon-separated, and `&` stands for the previous path.** A single letter
  before a colon is a Windows drive letter, not the end of a directory.
- **A shader that fails to parse or compile is logged by name and position**, and the failure
  is cached, so a broken shader on many primitives is reported once. A failed surface is
  replaced by `matte`. A failed light is left out. A shader named as the wrong type, such as a
  light given to `Surface`, is reported and replaced the same way.
- **A surface with no `Surface` request uses `constant`**, its own colour unlit.
- **`L` points from the shaded point toward the light**, in a light shader's `illuminate` and in
  a surface shader's `illuminance` body alike. A cosine falloff is `L . N`, and nothing in the
  library negates `L`. A `.sl` file written for the opposite reading renders its lights inside
  out.
- **A shader's `"shader"` space is where the scene instanced it.** `sl::Placed` is an instance
  and that transformation. Declared defaults are evaluated through the renderer's space table,
  not stored as numbers, so `point "shader" (0, 0, 1)` in the standard lights aims where the
  scene placed the light. A position a scene binds is in the same space.
- **A light shader runs in its own space.** While a light runs, `"shader"` is the light's
  placement, not the surface's. This places a `pointlight`'s default `from` where the scene put
  the light.
- **Both hiders bind the same globals.** A hider fills one `sl::Point` per shading point.
  `sl::Globals`, resolved once per program, writes the points into the batch and reads `Ci` and
  `Oi` back. `Globals::shine` runs a light over a batch. `du` and `dv` are zero for a traced
  hit, because a single point has no neighbour to difference.
- **A grid and a traced hit have different current spaces.** A grid is shaded in camera space
  and a traced hit in world space. The space table is therefore a callback each renderer
  implements (`sl::runtime::Renderer::space`), not a constant in the library. A grid has no
  answer for `"object"`, because that is the transformation at the primitive, and a primitive
  does not carry it.

### Built-in shaders

The standard shaders are SL source compiled into the library, so `Surface "matte"` works with no
files on disk.

| Kind | Shaders |
|---|---|
| Surface | `constant`, `matte`, `metal`, `plastic`, `paintedplastic`, `shinymetal`, `glass` |
| Light | `ambientlight`, `distantlight`, `pointlight`, `spotlight` |
| Imager | `background` |

- **`paintedplastic`** multiplies `Cs` by the texture its `texturename` names. It tests whether
  it was given a texture by comparing strings, which compare by text.
- **`shinymetal` traces a reflection ray** where RI's version reads an environment map.
- **`glass` is this tree's own**; RI has no refracting shader. It sets `Oi` to one, because it
  shows what lies behind it by refraction. Its `Os` is what a shadow ray through it reads.
- **The three directional lights** (`distantlight`, `pointlight`, `spotlight`) call
  `transmission()`, so they cast shadows. A `spotlight` tests its cone against `-L`, the
  direction its light travels.
- **`background`** adds its colour behind each pixel in proportion to `1 - alpha`, then sets
  `alpha` to one, so a pixel it painted is no longer empty. A scene can use it instead of a
  backdrop polygon.

### Built-in functions

- **Each built-in's `Signature` names its `Body`**, and the machine dispatches on the body, not
  the name. A function cannot be declared without saying what implements it.
  - `SOURCE` is written in SL and inlined.
  - `STUB` returns its default value and is reported once. `shadow` and `calculatenormal` are
    the two stubs.
  - Every other body is implemented in C++ in the machine.
- **`diffuse`, `specular` and `phong` are SL source** in `sl::Builtins`, written over
  `illuminance`, as the standard defines them. A shader that calls one has it copied into its
  own function list before anything else runs, so later stages see one kind of function. A
  shader's own definition of the name wins.
- **`ambient()` is C++.** An ambient light uses neither `illuminate` nor `solar`, so it has no
  direction, and an `illuminance` loop cannot reach it.
- **A built-in may return results through its arguments.** `Signature::outputs` names the first
  argument it writes. The compiler requires a variable there and propagates the storage class
  of the call's inputs into it, as an assignment would. `fresnel` is the one that does: it
  returns the unpolarised reflectance of a dielectric, with `refract`'s conventions, and writes
  the reflected and refracted directions.
- **A cast chooses between built-ins that differ only in their result type.** The compiler takes
  the first signature that accepts a call, unless the call is the operand of a cast and a later
  signature returns the cast's type. `color noise(P)` is three patterns, not one grey one, and
  `float texture(name)` is the first channel. Without a cast, `noise` returns a float and
  `texture` a colour.
- **`texture()` reads an image held for the frame.** `offline::Textures` reads each name once,
  looking on `Option "searchpath" "texture"` and then at the name as given. A name that fails
  to read is remembered as missing. The machine returns black for it and logs it once.
  `offline::Texture` samples bilinearly between texel centres and wraps periodically, RI's
  defaults, with `t` running down the image. A grey image fills all three channels and an alpha
  channel is dropped. Called with only a name, `texture()` reads at the shader's `s` and `t`.
- **`noise()` is Perlin's improved noise in SL's range**: `[0, 1]`, and `0.5` on every lattice
  point. Its permutation is shuffled by a `type::Random` with a fixed seed, so a pattern is the
  same on every machine. Its float, pair and point forms read a line, a plane and a volume of
  it.

Background: [ADR-0026](adr/0026-offline-shaders-run-over-batches-of-points.md)

## Normals

Both hiders carry two normals, because SL's `faceforward` and `calculatenormal` are defined in
terms of the pair:

- **`Ng` is the geometric normal**: the plane the primitive lies in, one value across it, wound
  the way its vertices are.
- **`N` is the shading normal.** A scene sets it per vertex with a varying `"N"`. Without one,
  `N` equals `Ng`, and the surface is faceted.

Rules:

- **A normal transforms by the inverse transpose**, never by the matrix that moves the points.
  The two agree under a rotation and a uniform scale, so the fault stays hidden until a scene
  scales one axis.
- **moya's `Vertex` holds both.** `RenderContext::addPolygon` fills them from
  `Polygon::geometricNormal()` before transforming anything. The diceable branch moves both into
  eye space. Dicing interpolates `N` and renormalises it. `Ng` is copied, since there is one.
- **A split does not carry per-vertex normals or colours.** Its pieces are built from
  intersection points, which have neither. Each piece takes the whole primitive's plane through
  `ReyesPrimitive::place()`, so a surface large enough to split is faceted per piece.
- **`trace::Triangle` has one constructor per case**: with and without per-corner normals.
  `shadingNormal(u, v)` interpolates over the barycentric coordinates `type::Ray::intersects`
  reports: `u` weights corner `b`, `v` weights corner `c`, and `a` takes the rest. That overload
  of `intersects` exists because Moller-Trumbore computes the coordinates on its way to the
  distance, and the four-argument form discards them.

## The reyes hider

The reyes hider renders in two passes. The first pass runs as each primitive arrives. The second
runs at `WorldEnd`.

### First pass: bound, cull and bucket

`RenderContext::addPolygon` does the following for each polygon:

1. Adds the polygon to the traced scene, then stores on it the state it was submitted under:
   the object to eye transformation, the colour, the geometric normal and the shading state.
2. Gives each vertex without a `"Cs"` the current colour, and each vertex without an `"N"` the
   geometric normal.
3. Bounds the polygon in object space and moves the bound to eye space. A moving primitive's
   bound covers both ends of the shutter.
4. Culls the polygon if its bound is entirely beyond the far clipping plane or entirely before
   the near one.
5. Marks it undiceable if it crosses the near plane and reaches behind the eye (z below zero).
6. Culls it against the view frustum, testing the eye space bound against the projection.
7. Projects the bound into raster space. If it is wider or taller than
   `sqrt(gridsize) * ShadingRate` pixels, marks it undiceable.
8. If it is diceable, moves its vertices and normals into eye space.
9. Files it in the bucket that holds the upper left corner of its raster bound. A corner off
   the image is clamped to the nearest bucket.

Only `Polygon` and `PointsPolygons` reach this pass. Spheres are not diced.

### Second pass: split, dice, shade and hide

`moya::FrameBuffer::render` sweeps the buckets. For each primitive in a bucket:

- **An undiceable primitive is split** into up to four pieces, by two planes through the centre
  of its bound, perpendicular to its plane and to each other. Each piece goes back through the
  first pass, which bounds, culls and buckets it again. A piece with fewer than three vertices,
  or one no smaller than its parent on any axis, is dropped, so splitting terminates.
- **A diceable primitive is diced into one grid** of `sqrt(gridsize)` by `sqrt(gridsize)`
  micropolygons, 16 by 16 at the default. Dicing interpolates position, colour, shading normal
  and `"st"` bilinearly over the first four vertices. A triangle's fourth corner is its third. A
  polygon with more than four vertices loses the rest, which gives a wrong grid for a concave
  polygon.
- **The grid is shaded**: `moya::GridShader` runs the surface shader once over all of the
  grid's vertices and leaves `Ci` on each.
- **The grid is hidden into the samples.** Each micropolygon takes the colour shaded at its
  first corner. It is tested against each sample in the pixels its bound touches, as two
  triangles, and its depth is interpolated at the sample. The nearest micropolygon wins the
  sample.

Rules:

- **A `ReyesPrimitive` carries the transformation, colour, normal and shading state it was
  submitted under.** A split resubmits pieces through the first pass during the second, when
  none of that state is current.
- **The sweep repeats while any bucket split something.** A piece is bucketed where it lands,
  which can be a bucket the sweep has already passed. A primitive already diced yields no more
  grids, so a repeated sweep costs one pass over the buckets.
- **The samples are one store for the whole frame**, `moya::Samples`, filtered through the film
  once the last bucket is done. A sample is not finished until every grid that could reach it
  is hidden, and the sweep can return to any bucket, so no bucket is finished early. Bucket
  edges never show in the image.
- **Every sample is opaque.** The reyes hider keeps the nearest surface and sets its opacity to
  one, ignoring the shader's `Oi`.
- **A grid is shaded once for all its samples**, so its shadow and traced rays see the scene at
  the shutter's open time.
- **`--grid` and `--bucket` on the command line, and `Option "limits"`**, set the grid size and
  bucket size. `ShadingRate` scales the split threshold.

## Ray tracing

The ray tracer is in `api/render/offline/trace`, namespace `offline::trace`. Both hiders use it.
The ray hider casts every primary ray into it. Under either hider, a shader's `trace()` and
`transmission()` calls are answered from it.

### The traced scene

- **`trace::Scene` holds primitives in world space, as one list.** A primitive is anything that
  derives from `trace::Primitive` and implements `intersect` and `describe`. `trace::Triangle`
  and `trace::Sphere` are the two kinds. A new kind is a new class, and the scene does not
  change.
- **Every primitive a scene gives moya is added to the traced scene as it arrives**, in world
  space, before any split or dice. A polygon becomes a fan of triangles, since RI defines a
  polygon as planar and convex. The reyes hider reads the scene for `trace()` and
  `transmission()`. The ray hider reads it for everything.
- **A traced primitive carries its colour, opacity, surface shader and lights** from the moment
  it was made. It uses the primitive's colour, not a varying `"Cs"`.
- **A traced primitive carries the lights that were on when it was made**, as one set shared by
  every primitive made until the lights change (`trace::Lights`). A primitive given no lights,
  as in a scene built in code, is lit by the scene's own list, `Scene::lights()`.
- **A sphere is intersected where it is defined**, cut to its slab of heights and its sweep,
  with RI's outward normal and its `u` and `v`. Its silhouette is exact at any size.
  `Orientation` is not read, so a sphere cannot be turned inside out.
- **A moving primitive is stored where its motion's open end put it.** `Scene::nearest()` takes
  the poses at a time: a ray is carried back into the stored pose, and its hit is carried
  forward again.
- **`Scene::nearest()` tests every primitive.** There is no acceleration structure.

### The tracer

- **`trace::Tracer` traces rays through a scene and shades what they hit.** `see()` is what a
  primary ray sees. `transmitted()` and `traced()` are `transmission()` and `trace()` in world
  space, for a shading point that is not a hit, such as a grid vertex.
- **Each hit is shaded by a `trace::HitShader`** made on the stack for it. The `HitShader`
  answers the shader's callbacks for that hit. A hit a shader traces into gets its own
  `HitShader`, so nothing is saved and restored around a traced ray.
- **The `Tracer` keeps one machine per program per trace depth.** A surface tracing into another
  surface with the same shader is still part way through its run when the second run starts.
- **The `Tracer` computes the poses once for a sample's time** (`Tracer::time()`), so a
  sample's shadow and traced rays see the same moment without inverting a motion per ray.
- **A primary ray composites what it passes through**, front to back by each surface's `Oi`,
  with the background behind what remains. A shader that never writes `Oi` is as opaque as its
  primitive's `Os`. A ray continuing through a surface is not a traced ray and does not count
  against the trace depth.
- **A primitive with no surface shader is its own flat colour.**
- **One shader run per hit is slow**, and accepted. The references are 64 by 48 pixels.

### Shadows and traced rays

- **`transmission()` is how a shader casts a shadow.** The three standard directional lights call
  it. It multiplies the light by every occluder's `Os`, read off the primitive without running
  its shader, so a shadow ray never shades anything. A renderer that cannot answer
  `transmission()` lets all the light through.
- **A ray leaving a surface is offset along the geometric normal, toward its target.** A ray
  started exactly on the surface hits that surface, and every lit pixel turns black in a
  pattern that looks like a normal fault.
- **`trace()` goes as deep as `Option "trace" "maxdepth"`**, two by default. Past that depth it
  returns the background, which stops two surfaces that trace into each other.
- **A grid traces through the same scene.** `GridShader` carries a shading point's ray from
  camera space into world space, starting it off the grid's plane. A ray from a grid of a warped
  quad may hit the exact polygon the grid was diced from, because the two agree only when the
  polygon is planar.

Background: [ADR-0077](adr/0077-offline-one-shared-ray-tracer.md)

## Depth of field

- **`DepthOfField fstop focallength focaldistance` sets the lens.**
  `Sampling::lensRadius()` is `focalLength / (2 * fstop)`, in camera space units. An infinite
  fstop, an fstop of zero or a focal length of zero is a pinhole.
- **Only a perspective camera blurs.** An orthographic camera has no lens to move and ignores
  `DepthOfField`.
- **The lens is at the eye**, for both hiders.
- **The ray hider moves a ray's origin across the lens** and aims it at the point the pinhole ray
  would reach on the plane of focus.
- **The reyes hider moves the micropolygon instead.** A sample's lens point `L` shifts an eye
  space point by `L * radius * (1 - z / focalDistance)` in x and y. Depth is unchanged, so the
  perspective divide is unchanged, and a corner's raster position is linear in `L`. Each
  micropolygon is therefore projected three times, at the lens centre and one unit along each
  lens axis, and every sample's corners are a sum of those. Its bound is grown to cover the
  lens's four extremes, so every sample it can reach is tested.

## Motion blur

- **Only a transformation moves.** `offline::MovingTransform` is the current transformation at
  a motion block's two ends. Inside a block, each transform request applies to its own copy of
  the transformation the block started with, the first to the open end and the last to the
  close end. A block naming more than two times keeps its first and last. Outside a block, a
  request applies to both ends, so what follows a block moves with it.
- **Between the ends, translation and scale are interpolated linearly and rotation by a
  quaternion.** That is exact for a rigid motion with uniform scale. A shear, or a non-uniform
  scale under a rotation, is approximate.
- **A sample's time** lies between the `Shutter` open and close times. The default shutter is
  `0 0`, which means no blur.
- **The ray hider carries each ray into the pose at its sample's time.** Its shadow and traced
  rays see that same time.
- **The reyes hider places a moving micropolygon per sample.** A primitive carries its moving
  object to eye transformation. The hider moves the eye space corners from the open end to the
  sample's time. The micropolygon's bound is the union of where it is at eight slices of the
  shutter, grown by the furthest a corner moves in one slice. A sample is rejected by its own
  slice's bound before anything is placed.
- **A moving primitive is culled by its bound at both ends**, and measured for splitting at the
  open end only, because a split shrinks a primitive but not the distance it travels.
- **A moving occluder's shadow is sharp under the reyes hider**, because a grid's shadow rays
  are cast at shutter open.
- **A pixel's seed is its position**, so two frames of an animation share their sample
  patterns. A moving scene shows this as fixed-pattern noise. The fix is to fold the frame
  number into the seed, in `offline::Sampler` alone.

## Adaptive sampling

- **Only the ray hider adapts its sample count.** With `PixelVariance` above zero, it takes
  another seeded set of samples wherever the variance of a pixel's mean, in its worst channel,
  is above the bound. It stops at four times the first set.
- **`RenderContext::samplesTaken()` returns how many samples a pixel took.** It returns zero
  under the reyes hider.
- **The reyes hider takes the count `PixelSamples` names.** It samples a whole bucket at once,
  and adapting per pixel would split a grid's hiding in two.
- **A scene moved between hiders can therefore change its noise.** The hider is named in the
  file, so the change is never silent.

## What is not built

[TODO.md](TODO.md#offline-rendering) holds the open work and when each item becomes due:

- Area lights render as point lights.
- Displacement shaders are refused and `calculatenormal` is a stub, so neither displacement nor
  bump mapping is possible.
- The reyes hider ignores `Oi`, so translucent surfaces render opaque under `"hidden"`.
- `trace::Scene::nearest` tests every primitive; there is no acceleration structure.

[TODO.md](TODO.md#rirotates-sign) also holds an open question: whether moya applies `Rotate`
with the sign RI intends.

Also not built, with no open item:

- Volume shaders, the `shadow()` built-in and shadow maps.
- Deforming primitives inside a motion block.
- Any geometry beyond `Polygon`, `PointsPolygons` and `Sphere`, and spheres under the reyes
  hider.
- `CropWindow`, `Orientation`, `Attribute`, binary and gzipped RIB, and `Display` modes other
  than RGB.
- The RI C API's `V` entry points.

## Building and testing

- **`api/render/offline` is added by `api/CMakeLists.txt`** from the library manifest, like
  every api library. It inherits nothing from the `api/render` directory above it, so it sets
  `/utf-8` in its own CMakeLists. spdlog's bundled fmt has a `static_assert` that fails without
  that flag.
- **`v3dlib_render_offline` links `v3dlib_log`, `v3dlib_image`, `v3dlib_type` and glm**, and
  never Vulkan or SDL. A realtime dependency arriving through one of those libraries breaks its
  build, and its CI suites, first.
- **Two suites cover the offline code**: `render_offline` tests the library alone, and `moya`
  tests the renderer. Run either with `ctest -R`.
- **The moya suite renders against committed PNGs** in `moya/tests/data/`: `reference-*` under
  the reyes hider and `raytrace-*` under the ray hider. Each PNG has a `.rib` beside it
  describing the same scene, so the file route and the code route are pinned to one picture.
- **A failing case writes what it rendered to `data_out/`** beside the test executable. That is
  also how a reference is regenerated when a change is meant to alter the picture.
- **Fixtures are copied only when the test target relinks.** Editing a `.rib`, `.sl` or `.png`
  alone leaves the old copy in place, and the suite tests the old file. Touch a source file of
  the suite, or copy the fixture into `out/build/x64-Debug/moya/tests/data/` by hand.
- **To look at a reference at a useful size**, copy its `.rib` to a scratch directory, raise its
  `Format`, give it an absolute shader search path, and render it with the driver from the
  driver's directory.

[Testing.md](contributing/Testing.md) covers the test framework, image comparison and the rest
of the suites.
