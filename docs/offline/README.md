# The offline renderer

These pages are for anyone changing `moya` or the library it reads RIB through, `api/rib`.
Someone who renders with moya reads the [moya manual](../manual/moya/README.md) instead.

moya is the only offline renderer in the tree. It is a spectral path tracer. It reads a
RenderMan scene file and writes image files. It shares no code with the realtime Vulkan
renderer: nothing it links names Vulkan or SDL, and it opens no window. Its test suites
therefore run in CI on machines with no GPU.

| Page | Read it to |
|---|---|
| This page | Learn the terms, and how a RIB scene becomes a picture |
| [RunningAndTesting.md](RunningAndTesting.md) | See how moya is laid out, and build and test it |
| [Rib.md](Rib.md) | Change the RIB reader, writer or recorder, or the scene client that reads RIB into the scene |
| [Scene.md](Scene.md) | Change the scene: its kinds, handles, samples over the shutter, primvars and versions |
| [Kernel.md](Kernel.md) | Write kernel code, which every device compiles from one source |
| [Devices.md](Devices.md) | Change how moya holds memory on a device and launches kernel code |
| [Spectra.md](Spectra.md) | Change how moya carries light as wavelengths and turns it into colour |
| [Sampler.md](Sampler.md) | Change how moya draws the random numbers a path uses |
| [Film.md](Film.md) | Change how moya places camera samples and adds them to a picture |
| [Traversal.md](Traversal.md) | Change rays, hits, the camera's rays, the hierarchies and how a ray finds what it meets |
| [Geometry.md](Geometry.md) | Change how meshes, subdivision surfaces and curves become what a ray meets |
| [Surfaces.md](Surfaces.md) | Change how a hit becomes the point a path shades |
| [Materials.md](Materials.md) | Change how moya compiles a material into a program, and the BSDF library it feeds |
| [Patterns.md](Patterns.md) | Change the noises, ramps, splits and rotations a material's program runs |
| [Textures.md](Textures.md) | Change the texture files moya reads, and the maker that writes them from images |
| [Integrator.md](Integrator.md) | Change how moya builds paths, samples lights, stops a pixel and resumes a render |
| [Lights.md](Lights.md) | Change moya's lights, how a path meets and samples them, the light tree and linking |
| [Output.md](Output.md) | Change how moya writes its pictures to EXR and PNG files |
| [Compositing.md](Compositing.md) | Change the channels, light groups, Cryptomatte layers and deep files moya writes beside the beauty |
| [Colour.md](Colour.md) | Change how moya converts between colour spaces and shows a picture on a display |

## How a frame is made

### Glossary

- **RI** (RenderMan Interface): the calls a RenderMan renderer takes, such as `RiPolygon` and
  `RiSurface`. moya takes them as RIB requests alone, and has no C interface.
- **RIB** (RenderMan Interface Bytestream): a text file of RI calls, one **request** per call,
  such as `Polygon "P" [...]`. It is the scene format moya reads.
- **Parameter list**: the name and value pairs that end a request, such as `"Cs" [1 0 0]`. Each
  name has a type, set by a declaration.
- **RIB reader**: `rib::Reader`. It tokenises a RIB file and calls one handler method per
  request.
- **Handler**: `rib::Handler`, a C++ interface with one virtual method per request.
- **Scene client**: `moya::ri::SceneClient`, the handler moya implements. It keeps RI's
  graphics state, the current transformation and attributes, and fills the scene from it.
- **Scene**: `moya::scene::Scene`, the retained scene moya renders. It holds objects of declared
  kinds by handle, and records each change as a version.
- **Prototype and instance**: a prototype is a geometry or a group. An instance places a
  prototype in the world, or in a group, with a transformation and a material.
- **Options**: the scene object that holds the frame's settings, such as the resolution, the
  samples per pixel and the pixel filter. The **camera** is the object the options name.
- **Kernel code**: the code every device compiles from one source, in `moya/libmoya/kernel/`.
  Everything else is host code.
- **Device**: what holds the memory kernel code reads and launches it over the pixels of an
  image. The CPU device is the only one.
- **Path**: the rays a camera sample follows from the camera through each bounce. The
  **integrator** builds paths and gathers the light they carry.
- **Sample**: one path's contribution to one pixel. It carries a place in the pixel, a time
  within the shutter and a point on the lens. A **pass** adds a fixed number of samples to each
  pixel.
- **Pixel filter**: the weighting of a pixel's samples: box, triangle, catmull-rom, gaussian or
  sinc.
- **Film**: `film::Film`, which holds each pixel's weighted sums of its samples in a device's
  memory. A **snapshot** is the picture the film holds, resolved into each pixel's means.
- **Channel**: a value the film holds per pixel beside the beauty, such as a depth or the light
  of a light path expression. The **beauty** is the picture's colour.
- **Output**: a file a render writes, with the channels it names.
- **Raster space**: pixel coordinates, with the origin at the image's upper left.

### The pipeline

```
scene.rib -> rib::Reader -> ri::SceneClient -> scene::Scene
                                                   |  WorldEnd: commit
              gather the prototypes, build the hierarchies, compile the materials, build the lights
                                                   |
             integrate::render: paths on a device -> film -> film::Snapshot
                                                   |
                                     output::write -> EXR, deep EXR or PNG files
```

1. The reader parses each request and calls the matching `SceneClient` method, with its
   parameter list already typed.
2. The scene client updates its graphics state, and creates and changes scene objects as the
   requests arrive. The options, the camera, the outputs and their channels are objects. A
   prototype and an instance for each primitive, its material, and the lights are objects too.
3. At `WorldEnd` the client commits the frame and calls the function its `frame()` was given.
   The scene's owner applies the batch there and renders.
4. `integrate::render()` gathers the prototypes the world's instances reach, and builds a
   hierarchy over each and one over the world. It compiles each material into a program and
   builds the lights.
5. The integrator runs on the CPU device in passes. For each sample, the sampler draws its
   numbers, the pixel filter places it, and the camera gives its ray. The path's light, and the
   channels the outputs name, are added to the sample's own pixel in the film.
6. The render returns the film's snapshot. Each output writes the beauty and its channels, as
   EXR, as a deep EXR file, or through a display as an 8-bit PNG.

Background: [ADR-0186](../adr/offline/0186-offline-moya-renders-by-path-tracing-alone.md)

### The parts

- **moya lives in `moya/libmoya`**, in the namespace `v3d::moya`, not in an api library. Every
  client of it is built in this tree.
- **Kernel code** is in `moya/libmoya/kernel/`, compiled under the rules a GPU compiler imposes
  as the target `v3dlib_moya_kernel`. The CPU and every later device run it from one source.
  [Kernel.md](Kernel.md)
- **A device** holds the memory kernel code reads and launches it over the pixels of an image.
  The CPU device runs tiles on oneTBB's threads. [Devices.md](Devices.md)
- **Light is four sampled wavelengths.** An RGB colour becomes a spectrum through a table fitted
  for ACEScg when moya is built, and a spectrum becomes XYZ through the CIE observer.
  [Spectra.md](Spectra.md)
- **The ZSobol sampler** gives each sample's random numbers from its pixel, index and
  dimension, with no state. [Sampler.md](Sampler.md)
- **The film** holds each pixel's weighted sums: the beauty in ACEScg and named channels beside
  it. A camera sample's place is drawn from the pixel filter, and the sample is added to its own
  pixel alone. [Film.md](Film.md)
- **Colour management** goes through OpenColorIO, with its ACES 2.0 configuration built in.
  [Colour.md](Colour.md)
- **Output** is OpenEXR in the working space, or 8 bits through a display. [Output.md](Output.md)
- **Outputs for compositing** sit beside the beauty: channels of light path expressions, light
  groups, Cryptomatte layers of objects and materials, and deep files. OIDN denoises the beauty
  and any channel marked for it. [Compositing.md](Compositing.md)
- **The scene** holds objects of declared kinds by handle, with samples over the shutter, and
  records every change as a version, so what is built from it rebuilds only what changed.
  `ri::SceneClient` fills it from RIB, and the editor keeps one in step with its own.
  `moya --scene` prints the scene a file makes. [Scene.md](Scene.md)
- **Rays and hits** are kernel code. A triangle is met watertight, a ray leaving a surface starts
  off it, and a stage takes its traversal as a type, so each device brings its own. Camera rays
  follow RI's camera, and `moya --hits` writes what they hit. Rays are traced through the
  hierarchy unless the brute-force traversal is asked for. The brute-force traversal is the
  reference every other traversal is checked against. [Traversal.md](Traversal.md)
- **The hierarchy** is a bounding volume hierarchy of eight children a node, built by the surface
  area heuristic with spatial splits. Each geometry and each group of a scene has one, and the
  world has one over its instances. [Traversal.md](Traversal.md#the-hierarchy)
- **The CPU's traversal** tests a node's eight children at once. It is built once for each of
  SSE4.2, AVX2 and AVX-512, and the CPU device loads the newest build its processor runs.
  [Traversal.md](Traversal.md#the-cpus-traversal)
- **Instances place prototypes**, and an instance's transformation and a mesh's points can move
  over the shutter. A ray is traced at its own time, through a hierarchy built over the motion.
  [Traversal.md](Traversal.md#motion)
- **Meshes become triangles** by ear clipping, each keeping its face.
  [Geometry.md](Geometry.md)
- **Subdivision surfaces** are refined by OpenSubdiv, cut from their limit surface to an edge
  length in pixels with no cracks, and displaced within a bound.
  [Geometry.md](Geometry.md#tessellation-and-displacement)
- **Curves, points and RI's quadrics** are intersected by moya's own code: a round curve as the
  surface a sphere sweeps, a flat one as a ribbon that faces the ray.
  [Traversal.md](Traversal.md#curves)
- **A change to the scene rebuilds only the hierarchies it touched.** `accel::Assembler` keeps
  each geometry's hierarchy until its geometry changes. [Traversal.md](Traversal.md#rebuilding-what-changed)
- **A hit becomes a shading point** through the scene's surfaces, which are held apart from any
  traversal: its position, normals and frame at the hit's time, with the primvars shading reads.
  [Surfaces.md](Surfaces.md)
- **A surface is shaded by its material.** A material is a MaterialX graph, compiled into a node
  program every device runs, which feeds a BSDF library arranged as OpenPBR Surface arranges its
  layers. Standard Surface, UsdPreviewSurface and glTF PBR are translated onto OpenPBR, and so
  are RI's built-in surfaces. A material's displacement moves a subdivision surface's points, and
  its opacity cuts a surface out. [Materials.md](Materials.md)
- **Patterns read textures** through a cache of tiles of mipmapped EXR files, filtered by the
  footprint of the cone a ray carries. The texture maker writes those files from images.
  [Textures.md](Textures.md)
- **The integrator builds paths in stages** over a path state declared once, with depth limits
  and Russian roulette. Each bounce samples a light and the BSDF, and weighs the two by multiple
  importance sampling. `moya` renders a RIB file through it. [Integrator.md](Integrator.md)
- **Every UsdLux light lights the scene.** A rect, disk, sphere or cylinder light is an instance
  in the world's hierarchy, which a path meets as it meets geometry. A point and a distant light
  have no shape. A mesh light makes a geometry emit. A dome light is the environment, and its
  image and a rect light's texture are drawn by their luminance. A cone and an IES profile shape
  what a light emits. [Lights.md](Lights.md)
- **Each light is sampled by solid angle** where a method exists, and a light tree picks the
  light a shading point samples by its expected contribution. A light lights only the instances
  it links, and casts shadows only from those its shadow link names.
  [Lights.md](Lights.md#the-light-tree)
- **RIB names the lights**: RI's standard, ambient and area lights, and RenderMan's
  `PxrRectLight` and its kin through `Light`. [Rib.md](Rib.md#the-scene-client)
- **A pixel stops at a noise threshold**, measured from two halves of its samples. A render keeps
  a checkpoint it resumes from, and gives the same picture as one that never stopped.
  [Integrator.md](Integrator.md#adaptive-sampling)
- **`moya_bench` times the hierarchies** over fixed scenes, and the integrator over a lit room,
  a textured one and one with outputs, against a committed baseline. [Traversal.md](Traversal.md#speed)

A picture is the same on one thread and on every thread, since a pixel's samples are added in
order by the one work item that owns it. `FoundationTest.cxx` renders a colour chart through
all of it. `PathsReferenceTest.cxx` holds every lit scene, material scene and scene written for
RI's surfaces to its committed render, and `CompositingReferenceTest.cxx` holds every scene of
outputs to its own.

Background: [ADR-0195](../adr/offline/0195-offline-the-renderers-core-lives-in-libmoya.md)

## What is not built

[TODO.md](../TODO.md#offline-rendering) holds the offline renderer's open items. These are not
built:

- `st` on a quadric, so a pattern or a texture that reads `st` on a sphere reads nothing.
- Emission that varies over a surface. An emitting material's emission is a constant.
- A mesh light whose material cuts it out. A light sample counts the cut-away area too.
- A split's step smoothed over a pixel, and a texture filtered over the footprint of a scaled
  coordinate.
- A device other than the CPU.
- Displacement of any geometry but a subdivision surface, and shaders written in RenderMan's
  shading language. A surface shader moya cannot find shades as `matte`.
- Volumes, and any request the reader does not recognise, such as `Orientation`, `ReadArchive`,
  `Patch` and `NuPatch`.
- Binary and gzipped RIB.
