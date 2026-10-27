# The offline renderer

These pages are for anyone changing `moya` or the library under it, `api/render/offline`.

moya is the only offline renderer in the tree. It reads a RenderMan scene file and writes an
image file. It shares no code with the realtime Vulkan renderer: nothing it links names Vulkan
or SDL, and it opens no window. Its test suites therefore run in CI on machines with no GPU.

| Page | Read it to |
|---|---|
| This page | Learn the terms, and how a RIB scene becomes an image |
| [RunningAndTesting.md](RunningAndTesting.md) | Run moya, and build and test it |
| [Rib.md](Rib.md) | See which RIB requests are read, and the rules for reading them |
| [CamerasAndSampling.md](CamerasAndSampling.md) | Change raster space, sampling, the film, cameras, depth of field or motion blur |
| [ShadingLanguage.md](ShadingLanguage.md) | Change the shading language compiler, runtime, built-ins or normals |
| [Reyes.md](Reyes.md) | Change the reyes hider: bucketing, splitting, dicing and hiding |
| [RayTracing.md](RayTracing.md) | Change the ray tracer or the ray-trace hider |

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
  plus the frame's options. `moya::RenderContext` holds it, as the classes below.
- **Primitive**: one piece of geometry: a polygon, a sphere, or one segment of a curve.
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
- **Batch**: the set of shading points one shader run covers: a grid's vertices, up to 64 ray
  hits that share a shader, or a row of pixels.
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
                                     reyes::Hider ("hidden") or RayHider ("raytrace")
                                                      |  samples
                                   offline::Film -> planes -> imager -> image file
```

1. The reader parses each request and calls the matching `RIBHandler` method, with its
   parameter list already typed.
2. The handler updates the graphics state in `RenderContext`. At `WorldBegin` the context saves
   the camera and builds the frame from the options as they stand. An option set after it is
   not refused. Each primitive is added to the traced scene, and under the reyes hider it is
   also filed in a bucket.
3. At `WorldEnd` the hider renders. The reyes hider splits, dices, shades and hides each
   bucket's primitives into a frame of samples. The ray hider casts a primary ray through each
   sample.
4. The film filters the samples into the framebuffer's planes.
5. The imager shader, if the scene names one, runs over the planes.
6. The colour planes are written to the output file.

**The RI C entry points in `RenderMan.cxx` are an adapter over `RIBHandler`.** Each converts
its arguments into the request the handler takes and passes it on, so a C call and a file's
request follow the same rules. The conversion is all an entry point does: RI_NULL becomes the
empty name, a filter function becomes a `Filter`, and the untyped parameter arrays become a list
typed by the handler's declarations. The reader cannot call the C entry points instead, because
most take varargs and a `va_list` cannot be built at run time.

- `moya::Renderer` is the C interface's stack of handlers. `RiBegin` pushes one and `RiEnd`
  pops it. The active handler receives every call, and `RiGetContext`'s handle is the handler.
- `RIBHandler` ignores a request whose shader, light or space name is empty.
- Every C entry point has a definition, but many are empty: every quadric but the sphere, and
  `RiMakeTexture`, among them.
- `RiCurves` reads only `"P"` and `"constantwidth"`. A varying value's count depends on the
  basis's step, and a C array carries no length, so any other parameter is skipped and logged
  rather than read past.
- A varargs form collects its parameters and calls its `V` form. `RiSphere` is the exception.
  RI ends its named arguments on a float, which a `va_list` cannot be started from, so it passes
  no parameter. moya reads none of a sphere's.
- Each token in the token table holds RI's string for it, and each basis matrix holds RI's
  values.

### The render context

**`RenderContext` has one method per RI request**, and holds the world from `WorldBegin` to
`WorldEnd`. Each part of the graphics state is a class of its own in `moya/libmoya/`:

| Class | Holds |
|---|---|
| `CameraOptions`, `Camera` | The camera options, and the camera they freeze into at `WorldBegin`. Both are in `api/render/offline` |
| `Transforms` | The current transformation, the `TransformBegin` stack, and the coordinate systems a scene named |
| `Attributes` | Everything `AttributeBegin` saves but the transformation. A block saves and restores the whole value |
| `Lights` | The lights a scene created, which last for the frame. Which are on is an attribute |
| `Shaders` | The shader library, the textures, the imager, and whether any bound shader traces |
| `TracedScene` | The scene a ray meets, which every primitive is added to |
| `Display` | `Display` and `CropWindow`, and the finishing of a frame |
| `Progress` | The callback a render reports to, and whether it was asked to stop |

The planes a frame is drawn into are `context().picture()`, indexed by `moya::Planes`.

### The two hiders

**A hider is a `moya::Hider`.** It reports whether it sees only the traced scene
(`traces()`), and it renders what the context gathered (`render()`). `RenderContext` holds one.
`"hidden"` is `moya::reyes::Hider` and `"raytrace"` is `moya::RayHider`. A hider name moya does
not recognise is logged, and the current hider stays.

Both hiders read one graphics state and one camera, write one framebuffer, and share the film,
the shading language and the ray tracer. They differ in these ways:

| | Reyes (`"hidden"`) | Ray (`"raytrace"`) |
|---|---|---|
| Default | Yes | No |
| Spheres | Not drawn. The first in a scene is logged | Intersected exactly |
| Curves | Split, then diced into ribbons facing the eye | Intersected as ribbons facing each ray |
| A shader's `Oi` | Ignored: every sample is opaque | Composited front to back |
| A varying `"Cs"` | Interpolated across the grid | Blended across each triangle by the hit's weights |
| `PixelVariance` | Ignored | Adds samples where a pixel is noisy |
| Time of shadow and traced rays | Shutter open | Each sample's own time |
| Storage of a primitive | In a bucket, and in the traced scene until rendering starts if no shader traces | Once, in the traced scene |

Background: [ADR-0078](../adr/0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md)

## What is not built

[TODO.md](../TODO.md#offline-rendering) holds the open work:

- A shader's traced rays include the hits that did not call `trace()`.

Also not built, with no open item:

- Volume shaders, the `shadow()` built-in and shadow maps.
- Deforming primitives inside a motion block.
- Any geometry beyond `Polygon`, `PointsPolygons`, `Sphere` and `Curves`, and spheres under
  the reyes hider.
- A curve's `"N"`, which would turn its ribbon to face a given way, its varying `"Cs"`, round
  curves, a displaced traced curve, and a curve joining an area light.
- `Orientation`, any `Attribute` but `"light"` and `"displacementbound"`, binary and gzipped
  RIB, and `Display` modes other than RGB.
