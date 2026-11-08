# Moya Scene — A Retained Scene, Change Tracking, and the RIB and Editor Clients

Drafted 2026-11-07 against `50ae0f16`. This plan takes up section 2 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core holds a
retained scene of geometry prototypes and instances, materials, lights, cameras and render
outputs, each created, modified and deleted by handle. Transformations and geometry carry
samples over the shutter, and every change is tracked so that a later structure rebuilds only
what it touched. RIB and the editor are both clients of the scene. Nothing in it renders yet.
Section 3 of the roadmap builds the BVH and intersection over it, and section 4 the path tracer.
The reyes hider and the ray hider keep rendering every RIB scene and every editor render, and no
picture they make changes.

The rebuild's decisions are ADR-0186 to ADR-0194, and the foundation's are ADR-0195 to ADR-0199,
all accepted. This plan follows four of them:

* [ADR-0188](../adr/offline/0188-offline-a-retained-scene-interface-moya-owns.md): a retained
  scene interface that moya owns, with RIB, the editor and Hydra as its clients.
* [ADR-0190](../adr/offline/0190-offline-materials-are-a-bsdf-library-and-materialx-graphs.md):
  materials as MaterialX graphs feeding moya's BSDF library, with OSL as a second pattern
  language.
* [ADR-0193](../adr/offline/0193-offline-moya-owns-its-ray-acceleration-structure.md): moya's
  own wide BVH, with motion over several segments and nested instances.
* [ADR-0195](../adr/offline/0195-offline-the-renderers-core-lives-in-libmoya.md): the new core
  lives in `moya/libmoya`.

The plan keeps the roadmap's order, with four changes:

* **Four decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are the scene's schema, how time is sampled, what a material holds before
  section 5 compiles it, and how changes are recorded and reach a render.
  [Decisions](#decisions).
* **Change tracking is built with the store, not after it.** Every setter of every object
  reports a change, so the record of changes has to exist before the first object kind does.
  [Step 5](#step-5--the-store-handles-parameters-and-changes).
* **The RIB reader's new requests are this plan's.** The roadmap's RIB client reads "the geometry
  and instancing that section 3 adds". Sections 2 and 3 run beside each other, so this plan adds
  the requests to the reader and the objects to the scene, and section 3 makes them intersect.
  The old hiders ignore the new requests as they ignore any request they do not support.
  [Step 10](#step-10--the-rib-readers-new-requests).
* **The editor's delegate builds the scene but keeps its picture from RIB.** Nothing renders the
  new scene until section 4, so `MoyaDelegate` keeps a scene in step with the editor's while its
  picture still comes from the reyes or ray hider. Section 4 switches the picture over.
  [Step 13](#step-13--the-editor-client).

## What the survey found

The tree was read at `50ae0f16`. Line numbers are from that commit.

**The RIB reader and its handler interface:**

* **`rib::Handler` is a stream of requests with no handles**
  (`api/render/offline/rib/Handler.h:33`). Each method has an empty body, so a handler that
  ignores a request needs no override.
* **The reader recognises 50 requests** (`rib/Reader.cxx:250-762`). Its geometry is `Polygon`,
  `PointsPolygons`, `Sphere` and `Curves`. It has no `ObjectBegin`, `ObjectEnd` or
  `ObjectInstance`, no `Points`, no quadric but the sphere, and no `SubdivisionMesh`.
* **Deformation motion is already carried by the reader.** A primitive after the first in a
  motion block goes to the handler's `deformation()`, and is dropped and reported when that is
  null (`rib/Reader.cxx:794-805`).
* **`rib::Recorder` is a handler that records requests and replays them** into another
  (`rib/Recorder.h:34`). The editor records a shot on its own thread and replays it on the
  render's.
* **The reader is fuzzed**, with the offline suite's RIB fixtures as its corpus
  (`api/render/offline/tests/CMakeLists.txt:49`).

**libmoya's RI state:**

* **`ri::RIBHandler` drives a `state::RenderContext`** (`moya/libmoya/ri/RIBHandler.h:36`, `:181`).
  The C interface in `ri/RenderMan.cxx` calls the same handler, so each RI rule is written once.
* **The render context is RI's graphics state and the old renderer together**
  (`state/RenderContext.h`). Its attribute and transformation stacks, coordinate systems,
  lights and shaders feed the two hiders, the old film and `sl`. None of it carries over.
* **Motion is two ends and an interpolation** (`api/render/offline/MovingTransform.h:18-31`):
  translation and scale linearly, rotation by a quaternion. A motion block of more than two
  times keeps its ends.
* **moya.exe reads a file into one `RIBHandler`** (`moya/moya.cxx:113`, `:126`).

**The editor:**

* **`MoyaDelegate` records a shot and replays it into a `RIBHandler`** on a thread of its own
  (`vertical3d/src/render/MoyaDelegate.cxx:140`, `:161`). The picture stays in the handler's
  framebuffer.
* **`RIBExportVisitor` turns the editor's scene into RIB requests**
  (`vertical3d/src/render/RIBExportVisitor.h:28-58`). Every surface is RI's `matte`, every light
  is RI's `pointlight`, `distantlight` or `spotlight`, and each face is one `Polygon`. A scene
  with no light gets a distant light from the camera.
* **Every item has a revision** that each setter moves (`vertical3d/src/scene/item/Item.h:100`).
  The delegate compares the revisions, the evaluated poses, the camera and the settings to decide
  whether to render again (`render/Delegate.h:277-291`). An edit restarts the whole render.
* **The editor's items are meshes, cameras, lights, groups and curves**
  (`scene/item/Item.h:38`). Its lights are point, distant and spot (`scene/item/Light.h:27`). It
  has no materials and no instancing.

**The tests:** moya's suite has 22 RIB scenes in `moya/tests/data`, 21 of them with reference
PNGs, read by the reference tests and the `RIB*Test.cxx` suites.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0200 | The scene's schema: how objects and their parameters are typed | [1](#step-1--the-record-the-scenes-schema) |
| 0201 | How time is sampled over the shutter | [2](#step-2--the-record-time-samples) |
| 0202 | What a material holds before it is compiled | [3](#step-3--the-record-a-material-is-a-network) |
| 0203 | How changes are recorded and reach a render | [4](#step-4--the-record-changes-and-commits) |

Each record step researched how production renderers make the choice, and the owner chose
among the options before the record was drafted. All four are accepted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-the-scenes-schema) | The record: the scene's schema | `docs/adr` | **0200** | done |
| [2](#step-2--the-record-time-samples) | The record: time samples | `docs/adr` | **0201** | done |
| [3](#step-3--the-record-a-material-is-a-network) | The record: what a material holds | `docs/adr` | **0202** | done |
| [4](#step-4--the-record-changes-and-commits) | The record: changes and commits | `docs/adr` | **0203** | done |
| [5](#step-5--the-store-handles-parameters-and-changes) | The store: handles, parameters and changes | `moya/libmoya` | 0200, 0203 | done |
| [6](#step-6--the-scene-description) | The scene description, for tests and `moya --scene` | `moya/libmoya`, `moya` | — | done |
| [7](#step-7--cameras-options-and-render-outputs) | Cameras, options and render outputs | `moya/libmoya` | 0201 | done |
| [8](#step-8--geometry-prototypes-and-instances) | Geometry prototypes and instances, with motion | `moya/libmoya` | 0201 | done |
| [9](#step-9--materials-and-lights) | Materials, lights and their bindings | `moya/libmoya` | 0202 | done |
| [10](#step-10--the-rib-readers-new-requests) | The RIB reader's new requests | `api/render/offline/rib` | — | done |
| [11](#step-11--the-rib-client-the-frame) | The RIB client: options, camera and outputs | `moya/libmoya` | — | open |
| [12](#step-12--the-rib-client-the-world) | The RIB client: the world | `moya/libmoya` | — | open |
| [13](#step-13--the-editor-client) | The editor client | `vertical3d/src/render` | 0203 | open |
| [14](#step-14--the-documents) | The reference pages for the scene | `docs` | — | open |

## What blocks what

```
1 ADR ──┬──> 5 Store ──> 6 Description ──┬──> 7 Camera, outputs ──> 11 RIB frame ──┐
4 ADR ──┘                                │                                        │
2 ADR ───────────────────────> 7, 8      ├──> 8 Geometry ──┐                      ├──> 14 Documents
3 ADR ───────────────────────> 9         └──> 9 Materials ─┴──> 12 RIB world ─────┤
10 Reader ──────────────────────────────────────────────────> 12                  │
                                         (after 7, 8, 9) ──> 13 Editor client ────┘
```

Steps 1 to 4 depend on nothing, and step 5 needs the records of steps 1 and 4. Step 10 touches
only the reader, and can start at any point. Steps 7, 8 and 9 can run beside each other once
step 6 lands. Step 12 needs the reader's new requests and every object kind. Step 13 needs the
object kinds, not the RIB client. Each step updates its own section of the reference documents,
and step 14 checks that they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build. The scene is host code, so neither build
  changes it.
* **No picture changes.** The old hiders read RIB as before, so every committed PNG in
  `moya/tests/data/` stays as it is, and the editor's render window shows the same picture.

---

### Step 1 — The record: the scene's schema

**The question.** How the scene's objects and their parameters are typed. Each client fills the
same objects: RIB from parameter lists typed by declarations, the editor from its items, and
later Hydra from prims and their attributes. Section 5's materials, section 4's lights and
section 9's edits all extend the schema, so it has to take new parameters without breaking a
client.

**The options:**

* **Typed objects with a typed parameter list each**, as Riley has. A function per object kind
  creates it from a list of named, typed values, and returns a handle of that kind. A new
  parameter is a new name, and no signature changes.
* **A reflected node schema**, as Cycles' `Node` and `SocketType` are. Each object kind declares
  its sockets once, with a type and a default, and every get, set, change flag and description
  goes through the declaration. A client sets a socket by name, and the description of step 6
  comes from the declarations alone.
* **Plain C++ structs per object kind**, set whole. The simplest, and every new parameter
  changes a struct that every client compiles against.

The record also settles what a handle is: an index with a generation, which a deleted object's
handle no longer matches, or a shared pointer.

**Drafted** as
[ADR-0200](../adr/offline/0200-offline-scene-objects-are-nodes-of-declared-parameters.md),
accepted: each kind declares its parameters once, and clients set them by name. Arnold, Cycles
and MoonRay, the renderers here that take edits while rendering, all work this way. A handle is
an index and a generation.

### Step 2 — The record: time samples

**The question.** How a transformation and a geometry's points carry samples over the shutter.
Section 3's BVH holds motion "over several segments", and section 4's camera rays draw a time.

**The options:**

* **Each object carries its own sample times**, as Riley and USD do. A RIB motion block keeps
  its times as written, and a client resamples nothing. The BVH builds bounds at each object's
  own times, or resamples them to its own.
* **The scene fixes one set of times**, evenly spread over the shutter, as Cycles' motion steps
  and Embree's time segments are. A client resamples every motion to those times, and the BVH's
  segments are the scene's.

The record also settles how a transformation is interpolated between samples: as the old
`MovingTransform` does, by decomposing into translation, rotation and scale, or by blending
matrices.

**Drafted** as
[ADR-0201](../adr/offline/0201-offline-each-scene-object-keeps-its-own-sample-times.md),
accepted: each object's own times, as RIB, USD and Riley give them. A consumer that needs even
keys, as OptiX and Embree do, resamples when it builds. A transformation is interpolated by
decomposition, as Cycles, pbrt and OptiX do.

### Step 3 — The record: a material is a network

**The question.** What a material object holds before section 5 builds the program that runs
it. RIB's surface and displacement shaders arrive as a name and parameters. MaterialX arrives as
a document of nodes. Section 13's OSL arrives as shaders and connections.

**The options:**

* **A network of shading nodes**, as Riley's materials are: each node names its shader and its
  parameter values, and connections join one node's output to another's input. A MaterialX
  document, an OSL network and a RIB shader of one node are all networks.
* **A MaterialX document**, held whole, with RIB's shaders translated into MaterialX when they
  arrive. One input form, and an OSL network would also have to become a MaterialX document.

**Drafted** as
[ADR-0202](../adr/offline/0202-offline-a-material-is-a-network-of-shading-nodes.md), accepted: a
network of nodes with named terminals, shaped as Hydra's `HdMaterialNetwork2` and Riley's shading
networks are. None of the renderers surveyed stores a MaterialX document as its material.

### Step 4 — The record: changes and commits

**The question.** How a change is recorded, and when it reaches a render. Section 3's BVH,
section 4's light tree, section 5's materials and each device's copy of the scene each rebuild
what a change touched, and section 9's edits arrive while sampling runs.

**The options:**

* **Dirty flags on each object**, set by a setter and cleared when the renderer syncs, as
  Cycles and Hydra's dirty bits work. One consumer at a time.
* **A version per change**, with each object and each socket stamped by the version that last
  changed it. Each consumer keeps the last version it built from, and asks for what is newer.
* **Edits applied as they arrive** under a lock, or **gathered and committed** as one change
  that a render takes between passes.

**Drafted** as
[ADR-0203](../adr/offline/0203-offline-scene-changes-are-versioned-and-committed.md), accepted:
a version per change, stamped on each object and parameter, and edits committed between passes.
Cycles' flags suit one sync of every structure, and moya's structures rebuild independently.
Riley, Arnold and Embree all take edits in a batch, between renders or on commit.

### Step 5 — The store: handles, parameters and changes

**Done.** `scene::Schema` holds `Kind`s, each a name and its `Declaration`s, and `scene::Scene`
holds objects of them. A `Value` is a typed array, and a handle is a kind, an index and a
generation. Clients create, set, declare and remove from any thread, each edit checked when it is
made, and `commit()` closes a batch. The owner's `apply()` makes each batch one version, and
`changes(since)` reports what each consumer has not seen. Three things the list below did not
settle:

* **Samples over the shutter are the store's.** A parameter declared for motion holds a value at
  each of its own times, which steps 7 and 8 use for transformations and points.
* **The store holds no kinds of its own.** A schema is passed in, so the tests declare kinds of
  their own, and steps 7 to 9 add moya's.
* **A handle parameter is checked for its kind when it is set.** Whether the object it names
  still exists is step 9's check, at commit.

[Scene.md](../offline/Scene.md) states the rules.

* **`moya/libmoya/scene/`**, in `v3d::moya::scene`. Host code, not kernel code: no device sees
  the scene. Section 3 and section 4 build device structures from it.
* **Handles** of each object kind, as step 1 decides, created, modified and deleted through the
  scene. A handle of a deleted object is refused, not reused silently.
* **Parameter values**: integers, floats, strings, colours with their colour space, points,
  vectors, normals, matrices, and arrays of each. A colour names its space as step 14 of the
  foundation reads one, and an empty name is the working space.
* **Primvars**: an array on a geometry with its interpolation class: constant, uniform,
  varying, vertex or face-varying. RI and USD name the same five.
* **Changes**: each set is recorded as step 4 decides, and a commit makes a batch of them one
  change. A consumer asks what changed since a version, by object and by parameter.

**Tests:** a deleted handle is refused. Setting a parameter records a change of it alone. Two
consumers each see every change since their own last version. A commit of many sets is one
version. A parameter of the wrong type is refused.

### Step 6 — The scene description

**Done.** `scene::describe()` writes the applied scene as text, in name order. Every object now
has a name, unique within its kind, given to `create(kind, name)`: an index depends on the order
objects were made in, and a name does not. A handle is described by its object's kind and name,
so two scenes describe alike whatever order they were built in. The RIB client takes a name from
RI's `Attribute "identifier" "name"`, so step 13's two paths through the editor name objects
alike. `moya --scene` is step 11's, since nothing reads RIB into the scene before it.
[Scene.md](../offline/Scene.md#the-description) states the format.

* **A text description of a scene**: every object, its handle, its parameters and their values,
  in an order that does not depend on the order objects were created in. Every later step's
  tests compare a scene with a committed description.
* **`moya --scene`** reads a RIB file into the new scene and prints its description, once step
  11 lands. A scene that reads differently from the old hiders' picture can then be inspected.

**Tests:** two scenes built in different orders describe alike. A description names every
parameter step 5 stores.

### Step 7 — Cameras, options and render outputs

**Done.** `scene::kinds()` gives moya's schema, which holds `camera`, `options`, `channel` and
`output` so far. A declaration now gives an element count, one, a fixed number or any, in place
of the flag for an array, and the strings a string parameter may hold. Three changes from the
list below:

* **A channel is an object of its own.** An output names its channels by handle, as Riley's
  displays name their render outputs, so two outputs can share one. Its precision and compression
  are the output's, as an EXR part's are.
* **The camera is the options', not each output's.** Every output of a render comes from one film,
  and a film is seen through one camera.
* **An output with no channels writes the beauty alone**, as RI's `"rgb"` mode does, rather
  than being refused.

[Scene.md](../offline/Scene.md#moyas-kinds) lists the kinds.

* **A camera**: perspective or orthographic, a screen window, clipping, depth of field by f-stop,
  focal length and focal distance, a shutter's open and close, and a transformation over the
  shutter.
* **Options**: the resolution and pixel aspect, a crop window, samples per pixel, the pixel
  filter and its widths, and the frame number the sampler is seeded by.
* **A render output**: a file, a format, a list of channels, each a name and a precision, and
  the camera it renders. Several outputs share a film. The foundation's film and EXR writer take
  them as they are.

**Tests:** each object's defaults are RI's defaults, which `CameraOptions` states today. A
render output with no channels is refused.

### Step 8 — Geometry prototypes and instances

**Done.** moya's schema adds `mesh`, `subdivision`, `curves`, `points`, RI's seven quadrics,
`group` and `instance`. A handle parameter now names several kinds it may hold, since an
instance's prototype may be any geometry or a group. Four changes from the list below:

* **An instance names the group it belongs to**, rather than a group listing its instances. An
  instance added to a group then changes that instance alone, and an instance in no group is in
  the world.
* **There is no single world transformation of a nested instance.** An instance inside a group has
  one placement for each instance of the group. Flattening the placements is section 3's BVH,
  which also stops at a group that contains itself.
* **A handle to a removed object names nothing**, and the structure built from it treats it as
  absent, as Hydra and Cycles do. Refusing it at commit would have the owner's `apply()` reject a
  batch a client had already committed.
* **Topology cannot change between times by construction.** Points over the shutter keep one
  count at every time, and the topology's arrays are not declared for motion.

A quadric's dimensions may move over the shutter, and each quadric is a unit shape until a client
sets it, since RI gives no defaults. The material an instance names is step 9's, since the
`material` kind comes with it. [Scene.md](../offline/Scene.md#moyas-kinds) lists the kinds.

* **Geometry prototypes**: polygon meshes, curves in each of RI's bases with a width per point,
  points with a width each, the six RI quadrics and the sphere, and subdivision meshes with
  creases and corners. Each holds its points and primvars, at the times step 2 decides.
* **Instances**: a prototype, a transformation over the shutter, a material, and visibility to
  the camera, to shadows and to indirect rays. An instance of a group of instances is an
  instance, so instancing nests as section 3's BVH holds it.
* **Deformation**: a prototype given at several times holds its points at each, with the same
  topology at every time. A prototype whose topology differs between times is refused.

**Tests:** each prototype kind holds what it was given, and its description matches. A mesh at
three times keeps three. A deformation that changes topology is refused. A nested instance's
world transformation is its own times its parent's.

### Step 9 — Materials and lights

**Done.** moya's schema adds `shader`, `connection` and `material`, an instance's `material`, and
seven lights. Three changes from the list below:

* **A network is objects.** A `shader` node names its shader and language and declares its
  inputs as primvars of constant interpolation. A `connection` feeds one node's output to another
  node's input, and a `material` names the node and output of each terminal. This is the shape of
  Hydra's `HdMaterialNetwork2`, so section 12's delegate copies one across.
* **Each light is a kind of its own**, as UsdLux's are, with UsdLux's parameters and defaults. A
  point light is a sphere light of radius zero and a spot a light with a cone, as the RIB client
  maps them at step 12.
* **A handle to a removed object names nothing**, as step 8 settled, so a mesh light whose
  prototype is removed names none, and so does an instance whose material is removed.

A primvar of constant interpolation is described without its class, so a shader's inputs read as
its parameters do. [Scene.md](../offline/Scene.md#moyas-kinds) lists the kinds.

* **A material**: a network as step 3 decides, with a surface and a displacement terminal. A
  material whose network names an unknown shader is kept, and section 5 reports it.
* **Lights**: rect, disk, sphere, cylinder, distant with an angle, mesh, and dome with an image,
  as section 4 samples them. Each has a colour, an intensity, an exposure and a transformation
  over the shutter. A point light is a sphere of radius zero, and a spot is a light with a cone,
  as RenderMan's `PxrSphereLight` and its shaping do.
* **A mesh light** names a geometry prototype, so an area light keeps its shape.
* **Bindings**: an instance names its material, and a light names the instances it lights,
  empty for all of them. Section 4 builds light linking on the second.

**Tests:** a material's network describes as it was built. A mesh light whose prototype is
removed names none. A removed material leaves its instances with none.

### Step 10 — The RIB reader's new requests

**Done.** The reader takes `ObjectBegin`, `ObjectEnd` and `ObjectInstance`, `Points`, the six
quadrics besides the sphere, and `SubdivisionMesh`, with two or three argument counts per tag.
The handler, the writer and the recorder carry each, and the geometry readers moved to
`rib/Primitives.cxx`, since `Reader.cxx` would have passed the file length limit. Two things the
list below did not say:

* **`RIBHandler` leaves out an object's geometry**, as RI leaves it out where it is declared,
  since its hiders do not instance. Before, the reader skipped `ObjectBegin` and the hiders drew
  the object where it was declared. No committed scene uses an object, so no picture changed.
* **RI's C interface reaches the handler for the quadrics, `RiPoints` and
  `RiSubdivisionMesh`**, so each still reaches the same method as its RIB request.
  `RiObjectBegin` returns no handle yet, because RI's handle is a pointer and the handler's is a
  string. The RIB client of step 12 decides how the C interface names an object.

The fuzz corpus gains `geometry.rib`, which a reader case reads whole.
[Rib.md](../offline/Rib.md) states the rules, and the manual's
[Rib.md](../manual/moya/Rib.md) lists the requests.

* **`ObjectBegin`, `ObjectEnd` and `ObjectInstance`**, with RI's rule that an object holds
  geometry alone and takes the attributes in force where it is instanced.
* **`Points`**, and **`Cone`, `Cylinder`, `Hyperboloid`, `Paraboloid`, `Disk` and `Torus`**,
  with RI's parameters.
* **`SubdivisionMesh`**, with its tags: crease, corner, hole and interpolate boundary.
* **The writer and the recorder** take each, so the editor's export and its shots can carry
  them.
* **`RIBHandler` reports each as unsupported**, as it does any request it does not draw, so the
  old hiders' pictures do not change.

**Tests:** each request reads into a recording with RIB's arguments, and writes back to the same
text. A malformed one is an error, not a crash. The fuzz corpus gains a fixture of each.

### Step 11 — The RIB client: the frame

* **A RIB client in `moya/libmoya/ri/`**, beside `RIBHandler`, that keeps RI's graphics state
  and creates scene objects. `RIBHandler` keeps driving the old hiders until section 7 removes
  them.
* **Options, the camera and the displays.** `Format`, `FrameAspectRatio`, `ScreenWindow`,
  `CropWindow`, `Projection`, `Clipping`, `DepthOfField`, `Shutter`, `PixelSamples`,
  `PixelFilter` and `Display` become the objects of step 7. `WorldBegin` fixes the camera, as RI
  does. A second `Display` with a `+` adds an output.
* **Frames.** Each `FrameBegin` block is a commit, so a file of several frames is a scene that
  changes between commits.
* **Names.** An object takes its name from RI's `Attribute "identifier" "name"`. One with none
  is named after its request and its place in the file.
* **`moya --scene`** reads a RIB file through the client and prints the scene's description, in
  place of rendering it.

**Tests:** the camera and displays of each of moya's RIB scenes describe as committed.

### Step 12 — The RIB client: the world

* **The attribute and transformation stacks**, coordinate systems and motion blocks, as
  `RIBHandler` keeps them today. A motion block's times are kept as step 2 decides.
* **Geometry.** Each primitive, the old four and step 10's new ones, is a prototype and an
  instance at the current transformation. An `ObjectBegin` block is a prototype, and each
  `ObjectInstance` an instance of it. A primitive repeated in a motion block deforms.
* **Shaders.** `Surface` and `Displacement` become a material of one node, named after the
  shader with its parameters. Instances under one attribute state share one material.
* **Lights.** `LightSource` of RI's `distantlight`, `pointlight` and `spotlight` become the
  lights of step 9, with RI's intensity and falloff. `AreaLightSource` makes the geometry after
  it a mesh light. `Illuminate` sets which instances a light reaches. `ambientlight` and a light
  of another shader are reported, and make no light.
* **Every RIB scene moya reads today reads through the client.** Each of the 22 in
  `moya/tests/data` has a committed description.

**Tests:** the committed descriptions. A light that `Illuminate` turns off lights none of the
instances that follow it. An object instanced twice is one prototype and two instances.

### Step 13 — The editor client

* **A translator in `vertical3d/src/render/`** that builds the editor's scene in the new scene.
  Each mesh is a prototype and an instance, and each curve that renders is a curves prototype.
  Each light is one of step 9's, and the render camera and settings are step 7's objects.
  Every surface gets one shared material of RI's `matte`, as the export writes today. A scene
  with no light gets the distant light the export adds.
* **Kept in step.** Each render and each update modifies only the objects whose items' revisions
  moved, deletes those whose items went, and commits once.
* **`MoyaDelegate` holds the scene** and keeps it in step on every render. Its picture still
  comes from the shot it replays into `RIBHandler`, until section 4 renders the scene.

**Tests:** the editor's scene through the translator and through the RIB export and the RIB
client describe alike. Moving one item modifies one instance, and the commit records that
instance alone. Deleting an item deletes its objects.

### Step 14 — The documents

* **A new reference page**, `docs/offline/Scene.md`, for the scene's objects, handles,
  parameters, time samples and changes. The docs site's summary lists it.
* **[Rib.md](../offline/Rib.md)** names the new requests and the RIB client.
* **[The offline README](../offline/README.md)**'s section on the new core names the scene and
  its page.
* **[The editor's rendering page](../editor/Rendering.md)** says that the delegate keeps a scene
  in step.
* **Each record's `Documented in` line** names the reference page that states its rule, and
  ADR-0188's names `Scene.md` in place of the roadmap.
* **The roadmap's section 2** links here, and says so when this plan closes.
