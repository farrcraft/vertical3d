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

Background: [ADR-0078](../adr/0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md)

## What is not built

[TODO.md](../TODO.md#offline-rendering) holds the open work and when each item becomes due:

- Area lights render as point lights.
- Displacement shaders are refused and `calculatenormal` is a stub, so neither displacement nor
  bump mapping is possible.
- The reyes hider ignores `Oi`, so translucent surfaces render opaque under `"hidden"`.
- `trace::Scene::nearest` tests every primitive; there is no acceleration structure.

[TODO.md](../TODO.md#rirotates-sign) also holds an open question: whether moya applies `Rotate`
with the sign RI intends.

Also not built, with no open item:

- Volume shaders, the `shadow()` built-in and shadow maps.
- Deforming primitives inside a motion block.
- Any geometry beyond `Polygon`, `PointsPolygons` and `Sphere`, and spheres under the reyes
  hider.
- `CropWindow`, `Orientation`, `Attribute`, binary and gzipped RIB, and `Display` modes other
  than RGB.
- The RI C API's `V` entry points.
