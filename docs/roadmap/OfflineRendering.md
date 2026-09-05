# Offline Rendering

Two of the applications in this tree are offline renderers. `talyn` is an intended raytracer;
`moya` is a reyes renderer behind the RenderMan interface. Between them they hold about 4,500
lines, they both build and both run, and **neither has ever produced a picture of anything**.
talyn writes a black PNG of the size the scene file asked for. moya writes no file at all.

That is the shape of the problem: in both, the scaffolding around the renderer is further
along than the renderer. talyn has a command line, a scene file reader and an image writer
with a comment where the ray tracing goes. moya has the whole RenderMan API surface, a context
stack, a coordinate system stack and the entire first reyes pass, with a comment where the
second pass goes. Each is a few hundred lines of actual rendering away from being a renderer,
and each has a few hundred lines of scaffolding that will need reworking before those lines
can be written.

State as of 2026-09-04. Neither renderer is scheduled; per
[the modernization plan's conclusion](../plans/completed/Modernization.md) they are deliberately
kept out of the realtime work, and this roadmap does not change that.

## What exists

### moya — the reyes renderer

`moya/libmoya` builds as `v3dlib_moya`, with `moya/moya/moya.cxx` as a driver that calls the RI
C API directly and `moya/tests/` as a suite of 39 cases that pass.

The **RenderMan interface** is declared in full — every entry point in
[RenderMan.h](../../moya/libmoya/RenderMan.h) has a definition, which is what RI compliance
asks for even from a renderer that supports nothing. About thirty of them have a body. The ones
that matter are `RiBegin`/`RiEnd` (push and pop a context), `RiWorldBegin`/`RiWorldEnd` (the two
reyes passes), `RiFormat`, `RiProjection`, `RiClipping`, the transform stack, and `RiPolygon`,
which walks a varargs parameter list, reads `RI_P` and builds a `Polygon`. `RiSphere` is empty,
`RiSurface` is empty, and `RiLightSource` returns 0.

The **first reyes pass** is real and is the most finished code in either renderer.
[`RenderContext::addPolygon`](../../moya/libmoya/RenderContext.cxx) bounds a polygon in object
space, moves the bound to eye space, culls it against hither and yon, marks it undiceable if it
crosses the eye plane, moves the bound to raster space, culls it against the view frustum, marks
it undiceable if it is larger than a grid, transforms a diceable polygon's vertices to eye space,
and files it in the bucket its upper left corner lands in. The supporting maths — `Plane`,
`Frustum` plane extraction, `AABBox` classification, the Sutherland-Hodgman clip — is written.

The **second pass is half a comment**. [`Bucket::render`](../../moya/libmoya/Bucket.cxx)
splits: a primitive too large for one grid is subdivided and its pieces go back through the
first pass until each fits, which as of 2026-09-04 works and is tested. Everything after that
is fourteen lines of pseudocode describing dicing, shading, bounding, culling, bucket overlap,
sampling and hiding, and none of it is code. `Polygon::dice` reaches the point of having the
polygon's raster bound and returns `false`. `MicroPolygon` and `MicroPolygonGrid` are
containers with no producer. `FrameBuffer::planes_` — the float image planes the whole design
is built around — is declared, never allocated and never written. Nothing converts a
framebuffer to an image, `v3dlib_moya` links no `v3dlib_image`, and the driver's `--output` is
parsed and dropped.

So the driver runs, prints Pixar's copyright, builds a context, buckets a polygon, subdivides
whatever is too big, finds nothing it can dice, and exits 0.

### talyn — the raytracer

`talyn/src` builds one executable. There is no library, no `tests/` directory and no ctest
entry.

`main` parses eight options with `program_options`, dispatches on the file extension, and
drives a `RenderContext` and a `FrameBuffer` to an image written through `image::Factory` —
`--outfile foo.png` works, and the format comes from the extension, so bmp, jpeg, png and tga
are all reachable. The [RIBReader](../../talyn/src/RIBReader.cxx) reads whitespace-separated
tokens and switches on twenty-odd of them. **Exactly one has a body**: `Format` reads three
more tokens and sizes the framebuffer. Every other request, geometry included, is recognised
and discarded. The `FrameBuffer` is a stack of float planes with a conversion to
`image::Image`.

[`RenderContext::render`](../../talyn/src/RenderContext.cxx) is a 30-line comment giving the
recursive ray tracing algorithm — primary rays, nearest hit, shadow rays with attenuation,
reflection and refraction at depth. There is no scene, no camera, no primitive, no ray and no
intersection anywhere in the tree's talyn.

The header of [talyn.cxx](../../talyn/src/talyn.cxx) carries a 35-item wish list running from
"base rendering algorithm" to radiosity and photon mapping, and a design note proposing that
talyn eventually become moya's raytracing component, reached through a shading language's
`trace()`. None of that list is started. The note is a live question and is picked up below.

### What the api already provides

Neither renderer needs to write intersection maths or camera maths from scratch.

* **[`type::Ray`](../../api/type/Ray.h)** has an origin, a direction, `transformed()`, a slab
  test against `AABBox` and Möller-Trumbore against a triangle, all with tests. It was written
  for the editor's picker ([ADR-0014](../adr/0014-picking-is-a-cpu-ray-cast.md)) and it is
  exactly what a primary ray and a triangle-mesh raytracer need.
* **[`type::Camera`](../../api/type/Camera.h)** builds the matrices and `project()`/`unproject()`
  are inverses ([ADR-0012](../adr/0012-camera-builds-vulkan-clip-space.md)), so a primary ray
  through a pixel is an unproject and a subtract — with the caveat that it builds *Vulkan* clip
  space, which is a decision an offline renderer inherits rather than needs.
* **`api/image`** reads and writes bmp, jpeg, png and tga, and `Image` row 0 is the top of the
  picture, which is what both framebuffers assume.
* **`api/brep`, `api/dag`, `api/asset`, `api/log`, `api/config`** are all free of realtime
  dependencies. The offline renderers are consumers of exactly that set, and are a useful
  forcing function for keeping it that way.

## Defects found reviewing this

**All of these are fixed, on 2026-09-04.** They are kept here because several were latent only
because the path they sat on was unreachable, and the shape of each is worth knowing before
writing the code that will run over it again. The first three change behaviour rather than
merely removing a fault.

| Where | What was wrong |
|---|---|
| `Polygon::dice` | Took three parameters where the virtual it meant to override took one, so it overrode nothing: `Bucket::render` called through `ReyesPrimitive*`, got the base, and **no polygon could ever be diced**. Both now take `(grid, RenderContext &)`, and the context is threaded down through `FrameBuffer::render` and `Bucket::render` to reach it |
| `Polygon::split` | Computed four sub-polygons and the code returning them was commented out, so `Bucket::render` erased the primitive it had just split and the geometry was dropped. It now takes the context and submits each piece back through `addPolygon`, which is what re-bounds, re-culls and re-measures it. A piece with fewer than three vertices, or one no smaller than what it came from, is not submitted — **a split that does not shrink its input would be split again without end** |
| `Frustum::intersect` | Returned `OUTSIDE` only when all six planes excluded the box, so the cull in the first pass kept everything. One plane excluding it is now enough |
| `Frustum::extract` | Indexed the matrix as if it were row major, extracting the planes of the transpose, and took the near plane as the z row alone — the form for a clip volume whose depth runs `[0, 1]`, where the far plane beside it was already written for `[-1, 1]` |
| `RenderContext::projection` | The orthographic depth scale was negated, which is the right handed form; the interface looks down +z, so the whole clip range sat behind the near plane. `[2][2]` was also assigned twice, the second overwriting that scale with the depth translate that belongs in `[3][2]`, and the matrix was left uninitialised on the branch that composes it for an unsupported projection |
| `RenderContext::addPolygon` | Tested the raster space bound against the planes of the raster matrix — the planes of a matrix bound the space it *reads*, so this asked whether pixel coordinates fall inside a volume measured in eye units. It now tests the eye space bound against the projection |
| `Plane` | Carried a normal and distance alongside the plane equation, and of the four writers each set only one of the pair — so a plane built by `Frustum::extract` had a normal of whatever was on the stack, and `intersectEdge` read it. The equation is now the only state, and the normal and distance are derived |
| `Plane::clip` | Assigned the clipped polygon to its own by-value parameter, so the clip was computed and discarded, and it opened on the vertex before the first one of an empty polygon. It rewrites the polygon in place and leaves one with fewer than three vertices alone |
| `MicroPolygonGrid` | Was built empty and indexed into regardless. It now takes its extent on construction, has no default constructor, and asserts its bounds |
| `RenderContext`, `ReyesPrimitive`, `Vertex`, `Bucket` | Left members indeterminate: `near_` and `far_` in neither of the context's two constructor initialiser lists, along with the crop, frame aspect, screen window and depth of field options; `_diceable`; a vertex's point, colour and normal; a bucket's origin. Every default now sits on the member, which is what the two constructor lists disagreeing about them caused |
| `FrameBuffer` | Divided image size by bucket size, losing the right and bottom edges of an image that was not a whole number of buckets across, and converted a raster bound's negative corner straight to `unsigned int`. The counts round up and are stored once; a corner past either edge is clamped to the bucket that covers it |
| talyn's `main` | Stored `--height` into `width`, so `--height` overwrote the width and, `height` staying 0, the guard below never fired and neither override reached the scene. A lone `--width` or `--height` now says it needs the other rather than being ignored |
| talyn's `FrameBuffer` | Wrote `channels` bytes from a pixel *index*, so each pixel's channels landed on its neighbours. Invisible while every plane is zero |
| `talyn/CMakeLists.txt` | Named `src/Framebuffer.cxx` for a file called `FrameBuffer.cxx`, which builds only because the filesystem is case insensitive |

The suite grew from 18 cases to 39 covering them, including `Plane`, `Frustum`,
`MicroPolygonGrid` and `FrameBuffer`, which had none. The one that pins the split asserts that
a polygon 288 raster pixels across ends as 4^5 pieces — five rounds of four way subdivision to
get under a 16 pixel grid — which is both the termination proof and the arithmetic.

Three points of house style went the same day. talyn was in namespace `Talyn` and is now
`v3d::talyn`, which is what the `api/` path convention gives it. Half of libmoya named its
members `_leading` while its own `Polygon`, `RenderContext`, `Plane` and `Vertex` named theirs
`trailing_`; all of it is the latter now. And `RenderMan.cxx` held its renderer in a global
called `_renderer` — **a leading underscore at namespace scope is reserved to the
implementation** — which is now a plainly named object in an anonymous namespace, since no
other translation unit refers to it. That file was also the one thing in the tree that was not
valid UTF-8: 47 cp1252 smart quotes in its comments, in a build that passes `/utf-8`
unconditionally.

## The ordering

### Phase 1 — each renderer computes a pixel

Everything else is blocked on this, for a reason that is not obvious: **a renderer that
computes nothing and a renderer that works are indistinguishable from the outside today**.
talyn's black PNG is what a correct render of an empty scene looks like. moya's clean exit is
what a correct render of a scene it culled entirely looks like. Until each produces a pixel
whose value came from geometry, there is nothing to write a test against and no way to tell a
regression from the status quo.

The smallest thing that ends that, per renderer:

**talyn** — a scene of one camera and one triangle, primary rays through it, flat colour on
hit and a background colour on miss. `type::Ray::intersects` is the intersection, `type::Camera`
is the camera, and the framebuffer and writer already exist, so the new code is a scene class,
a loop and the shading constant. The framebuffer's channel indexing has to be fixed first or
the picture will not be the one that was computed.

**moya** — dicing, then output. `dice` dispatches and the grid takes an extent, so what is
left is filling that grid from a polygon, then allocating `planes_`, writing a flat colour per
micropolygon, and adding the framebuffer-to-`image::Image` conversion talyn already has. That
last piece is what makes `--output` mean something, and means `v3dlib_moya` grows a
`v3dlib_image` link.

### Phase 2 — a scene worth rendering

Blocked by phase 1: parsing a scene format nobody can render is unverifiable work.

The RIB reader is in the wrong tree. talyn has a reader that recognises requests and acts on
one of them; moya has the RI entry points the reader should be calling. One reader driving the
RI API serves both, and it needs a real tokenizer first — quoted strings, bracketed arrays and
typed parameter lists, none of which the current whitespace split handles.

The other route in is [the editor's project file](../adr/0018-a-project-is-json-and-stores-topology-verbatim.md).
It is the only actual scene in the tree and the only source of meshes that is not hand-written
RIB, and `api/asset` already parses JSON. It carries topology and a placement per mesh and
**nothing else** — the editor's `Scene` has no lights and no materials, and `SceneVisitor` is
written in anticipation of them rather than for them. So this route renders grey until the
editor's scene model grows the things phase 3 needs, which makes it the slower of the two to
first picture and the more valuable afterwards.

### Phase 3 — light and surface

Blocked by phase 2 for moya (a scene has to be able to *say* "light"), by phase 1 only for
talyn (a hard-coded light in a hard-coded scene is a fine start).

moya's `RiLightSource` returns 0, its `RiSurface` is empty, and neither renderer has a material
of any kind. The RI standard shaders — matte, metal, plastic, paintedplastic — are the obvious
target because the tokens are already declared. Shadow rays are talyn's version of the same
step, and are cheap once primary rays work.

The large question sitting underneath this phase is whether shading is fixed-function C++ or a
shading language. It is the difference between a weekend and a subsystem, it decides whether
talyn's raytracing is reached through `trace()` from a shader, and it should not be answered by
starting to write either one.

### Phase 4 — sampling and quality

Blocked by phase 3: antialiasing a flat-shaded scene measures nothing.

The five `Ri*Filter` functions — box, triangle, gaussian, catmull-rom, sinc — all return `0.0`
today, and they are the pixel filter half of this. The sampling half is supersampling, then
adaptive supersampling. `RenderContext` already declares `fStop_`, `focalLength_`,
`focalDistance_`, `shutterOpen_` and `shutterClose_`, which nothing writes and nothing reads:
depth of field and motion blur are fields waiting for an implementation.

### Phase 5 — talyn's own list

Blocked by phase 3. Reflection and refraction are the recursion the algorithm comment already
describes and are a day's work once shading exists; index of refraction and transparency come
with them. Texture and bump mapping ride on `api/image`. An acceleration structure is worth
nothing until there is a scene large enough to be slow, and should wait for one rather than be
built on principle.

### Phase 6 — whether they unify

The question [talyn.cxx](../../talyn/src/talyn.cxx) asks: one renderer with two algorithms
behind a common interface, or two renderers that share libraries. It cannot be answered before
phase 3, because the answer depends on how much the two turn out to share once both actually
shade something, and on whether shading is a language. Deliberately last.

## What this needs decided

Each of these is hard to reverse and would otherwise be settled by whoever writes the code
first. None is decided here — see [sdlc.md](../sdlc.md).

* **Where the shared offline code lives.** Two `FrameBuffer` classes exist with the same design
  and the same doc comment, and two `RenderContext` classes. The candidates are an `api/`
  library both consume, `v3dlib_moya` as the shared one with talyn depending on it, or leaving
  them separate. talyn.cxx's own note proposes a third name, `libv3drender`.
* **Which scene description is primary** — RIB, the editor's project JSON, or both through one
  intermediate. This decides where the reader lives and what phase 2 costs.
* **Whether the offline renderers use `type::Camera`.** It builds Vulkan clip space by
  [ADR-0012](../adr/0012-camera-builds-vulkan-clip-space.md), and reyes has its own
  well-specified screen and raster spaces that moya's `RenderContext` already half implements.
  Reusing it inherits a convention chosen for the swapchain.
* **Fixed-function shading or a shading language.** Phase 3, and it reaches into phase 6.
* **Whether talyn stays an executable.** It has no library and no tests, and it cannot get
  tests without one.

## Verification

This is the part worth knowing before any of the above starts.

**The offline renderers are the only renderers in this tree CI can run.** Everything below the
recorder in `api/render` needs a window and a GPU and is waiting on
[ADR-0007](../adr/0007-ci-rendering-tests.md); a rendering change is verified today by running
an app and reading the validation log. Neither of these renderers touches a window, a device or
a swapchain — `moya/tests/CMakeLists.txt` already says so, and its suite runs in CI now.

So the moment phase 1 lands, a rendered image can be compared against a committed reference in
ctest, on every push, with no GPU. That is the first real render regression test the repo has
had, and it is the reason phase 1 is phase 1 rather than a detail of phase 2.

Two things stand in the way of collecting on that. talyn has no `tests/` directory, no library
to link and no ctest entry, so its half of the work starts by giving it one. And a reference
image comparison needs a tolerance rather than an equality — the tree has no image-comparison
helper, and `api/image`'s suite is the natural place for one.

## Not on this roadmap

**Integration with the realtime stack.** Settled in
[the modernization plan](../plans/completed/Modernization.md): the offline renderers consume the
non-realtime `api/` libraries and nothing else, they are not scheduled into the Vulkan work, and
trying to make them fit the realtime pipeline is the wrong goal. Nothing found in this review
changes that.

**RenderMan compliance as a goal.** moya declares the full interface, which is what compliance
asks for, but [Renderer.h](../../moya/libmoya/Renderer.h) records that having a second API
violates the one-true-API clause and that moya is therefore not compliant. It prints Pixar's
copyright, which is what a modelling program using the standard is required to do. Chasing the
label is not on the list; implementing the requests is.
