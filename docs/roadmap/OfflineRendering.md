# Offline Rendering

Two of the applications in this tree are offline renderers. `talyn` is a raytracer; `moya` is a
reyes renderer behind the RenderMan interface. **As of 2026-09-05 both read a scene from a RIB
file and draw it**, each compared against a committed reference in ctest, and the editor exports
to the same format.

Before that, neither had ever rendered anything: talyn wrote a black PNG of the size the scene
file asked for and moya wrote no file at all. That was the shape of the problem — in both, the
scaffolding around the renderer was further along than the renderer. What is left of it is one
flat colour per surface with no light and no material behind it, and one sample per pixel.

State as of 2026-09-05, with phases 1 and 2 closed the same day as their own plans
([one](../plans/completed/OfflineRenderingPhase1.md),
[two](../plans/completed/OfflineRenderingPhase2.md)) and phase 3 taken up by
[a plan of its own](../plans/OfflineRenderingPhase3.md). Nothing beyond that is scheduled; per
[the modernization plan's conclusion](../plans/completed/Modernization.md) both renderers are
deliberately kept out of the realtime work, and this roadmap does not change that.

## What exists

### moya — the reyes renderer

`moya/libmoya` builds as `v3dlib_moya`, with `moya/moya/moya.cxx` as a driver that reads a RIB
file through the shared reader and `moya/tests/` as a suite of 55 cases that pass.

There are **two ways into a render context**, and by
[ADR-0025](../adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md) neither goes
through the other: the RI C entry points, and
[`moya::RIBHandler`](../../moya/libmoya/RIBHandler.cxx). Both drive `RenderContext`, which is
where the behaviour is. A reader cannot use the C API, because a `va_list` cannot be built at
runtime.

The **RenderMan interface** is declared in full — every entry point in
[RenderMan.h](../../moya/libmoya/RenderMan.h) has a definition, which is what RI compliance
asks for even from a renderer that supports nothing. About forty of them have a body. `RiSphere`
is empty, `RiSurface` is empty, and `RiLightSource` returns 0. **The `V` forms are the ones
still worth having** — `RiPolygonV` and its siblings are the standard's own answer to a caller
holding a runtime parameter list, and implementing them on the handler would give the C API and
the reader one path rather than two.

The **first reyes pass** is real and is the most finished code in either renderer.
[`RenderContext::addPolygon`](../../moya/libmoya/RenderContext.cxx) bounds a polygon in object
space, moves the bound to eye space, culls it against hither and yon, marks it undiceable if it
crosses the eye plane, moves the bound to raster space, culls it against the view frustum, marks
it undiceable if it is larger than a grid, transforms a diceable polygon's vertices to eye space,
and files it in the bucket its upper left corner lands in. The supporting maths — `Plane`,
`Frustum` plane extraction, `AABBox` classification, the Sutherland-Hodgman clip — is written.

The **second pass** dices, shades and hides.
[`Bucket::render`](../../moya/libmoya/Bucket.cxx) splits a primitive too large for one grid and
its pieces go back through the first pass until each fits; a primitive that fits is diced into
one `MicroPolygonGrid` by bilinear interpolation over its first four vertices, and sampled at
one pixel centre per micropolygon against a depth plane. Dicing interpolates the primitive's
colour along with its position, which is the whole of shading: there is no separate shade step
and no constant written over the top of one. `RiDisplay` records a name and a mode, and
`RenderContext::render` writes the colour planes through `image::Factory` once the buckets are
done.

So the driver reads a file, prints Pixar's copyright, builds a context, buckets each polygon,
subdivides whatever is too big, dices the rest, and writes the picture the scene named.

**A primitive carries the graphics state it was submitted under** — the object to eye transform
and the colour, on `ReyesPrimitive`. Splitting resubmits pieces through the first pass during
the second one, when neither is current any more, and a split builds its pieces out of
intersection points that carry no colour of their own.

What the second pass does not do: no material and no light behind that colour, no pixel filter,
no supersampling, no bucket overlap — a grid is sampled wherever it lands rather than being
handed to each bucket it touches — and no more than four vertices per polygon.

### talyn — the raytracer

`talyn/libtalyn` builds `v3dlib_talyn`, with `talyn/talyn/talyn.cxx` as a driver and
`talyn/tests/` as a suite, mirroring moya per
[ADR-0022](../adr/0022-offline-rendering-shares-an-api-library.md).

`main` parses eight options with `program_options`, dispatches on the file extension, and
drives a `RenderContext` to an image written through `image::Factory` — `--outfile foo.png`
works, and the format comes from the extension, so bmp, jpeg, png and tga are all reachable.
`talyn --file scene.rib` reads that scene through the shared reader and
[`talyn::RIBHandler`](../../talyn/libtalyn/RIBHandler.cxx), which fans a polygon into triangles
through the current transformation and builds the camera at `WorldBegin`.

**The camera is the one thing talyn cannot take from an arbitrary RIB file.** `CameraProfile`
holds an eye and a rotation, and `Camera::createView()` composes them, so a world to camera
matrix that reverses handedness — which is what RI's camera basis is for any general lookat —
cannot be expressed. Such a scene is refused with a message rather than rendered mirrored. It
is the shape [ADR-0024](../adr/0024-api-type-serves-both-renderers.md) anticipated and the
first thing that would make it concrete.

[`RenderContext::render`](../../talyn/libtalyn/RenderContext.cxx) casts a primary ray through
every pixel centre, takes the nearest triangle hit and writes that triangle's flat colour or
the scene background. The 30-line comment above it gives the recursive algorithm the later
phases fill in — shadow rays with attenuation, reflection and refraction at depth.

`talyn::Scene` is a camera, a list of flat coloured triangles and a background. The background
is the one thing a file cannot set — RIB says it with `RiImager`, which is phase 3 — so a scene
that wants one draws a backdrop polygon, which is what the reference fixture does.

### What the api already provides

Neither renderer needs to write intersection maths or camera maths from scratch.

* **[`type::Ray`](../../api/type/Ray.h)** has an origin, a direction, `transformed()`, a slab
  test against `AABBox` and Möller-Trumbore against a triangle, all with tests. It was written
  for the editor's picker ([ADR-0014](../adr/0014-picking-is-a-cpu-ray-cast.md)) and it is
  exactly what a primary ray and a triangle-mesh raytracer need.
* **[`type::Camera`](../../api/type/Camera.h)** builds the matrices and `project()`/`unproject()`
  are inverses ([ADR-0012](../adr/0012-camera-builds-vulkan-clip-space.md)), so a primary ray
  through a pixel is an unproject and a subtract. It builds *Vulkan* clip space, which an
  offline renderer has no reason to want; by
  [ADR-0024](../adr/0024-api-type-serves-both-renderers.md) that convention becomes a parameter
  rather than a reason for a second camera.
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

**Done, 2026-09-05.** [OfflineRenderingPhase1.md](../plans/completed/OfflineRenderingPhase1.md) is
the plan, and carries the step ordering and what landed; what follows is why the phase was first.

Everything else was blocked on this, for a reason that is not obvious: **a renderer that
computes nothing and a renderer that works are indistinguishable from the outside**.
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
last piece is what makes `--output` mean something, and by
[ADR-0022](../adr/0022-offline-rendering-shares-an-api-library.md) it is written once in
`api/render/offline` rather than copied across.

### Phase 2 — a scene worth rendering

**Done, 2026-09-05.** [OfflineRenderingPhase2.md](../plans/completed/OfflineRenderingPhase2.md)
is the plan, and carries the step ordering and what landed; what follows is why the phase was
second.

Blocked by phase 1, which is done: parsing a scene format nobody can render is unverifiable work.

RIB is the format both renderers read, by
[ADR-0023](../adr/0023-rib-is-the-offline-scene-description.md), and the reader is in the wrong
tree for it: talyn has one that recognises requests and acts on one of them, and moya has the RI
entry points it should be calling. The reader moves to `api/render/offline` and needs a real
tokenizer first — quoted strings, bracketed arrays and typed parameter lists, none of which the
current whitespace split handles, and a declaration table without which the first parameter that
is not `P` has no type.

The editor is the other end of the same decision. Its project file
([ADR-0018](../adr/0018-a-project-is-json-and-stores-topology-verbatim.md)) stays the editor's
own and gains a RIB export, one way. That export carries topology and a placement per mesh and
**nothing else** — the editor's `Scene` has no lights and no materials, and `SceneVisitor` is
written in anticipation of them rather than for them — so a scene out of the editor renders grey
until the editor's scene model grows the things phase 3 needs. Hand-written RIB is what the
renderers are fed until then, and is what a test fixture is either way.

### Phase 3 — light and surface

**Open**, blocked by phase 2 which is done.
[OfflineRenderingPhase3.md](../plans/OfflineRenderingPhase3.md) is the plan, and carries the step
ordering; what follows is why the phase is third.

A scene can now *say* "light" — `LightSource` and `Surface` reach both handlers, with their
parameters typed by the declaration table, and both drop them.

moya's `RiLightSource` returns 0, its `RiSurface` is empty, and neither renderer has a material
of any kind. Neither has a surface normal either, which is the geometry half of the same gap.
Shadow rays are talyn's version of the step, and are cheap once primary rays work.

The large question sitting underneath this phase was whether shading is fixed-function C++ or a
shading language. **It is answered as a language**, and the plan's first step is the record that
weighs it. That answer is what makes this phase a subsystem rather than a weekend, and it is why
phases 4 and 5 sit behind it rather than beside it.

### Phase 4 — sampling and quality

Blocked by phase 3, which is open: antialiasing a flat-shaded scene measures nothing.

The five `Ri*Filter` functions — box, triangle, gaussian, catmull-rom, sinc — all return `0.0`
today, and they are the pixel filter half of this. The sampling half is supersampling, then
adaptive supersampling. `RenderContext` already declares `fStop_`, `focalLength_`,
`focalDistance_`, `shutterOpen_` and `shutterClose_`, which nothing writes and nothing reads:
depth of field and motion blur are fields waiting for an implementation.

### Phase 5 — talyn's own list

Blocked by phase 3, which is open. Reflection and refraction are the recursion the algorithm comment already
describes and are a day's work once shading exists; index of refraction and transparency come
with them. Texture and bump mapping ride on `api/image`. An acceleration structure is worth
nothing until there is a scene large enough to be slow, and should wait for one rather than be
built on principle.

### Phase 6 — whether they unify

The question talyn's driver used to ask in a comment at the top of it, until phase 1 removed the
comment: one renderer with two algorithms behind a common interface, or two renderers that share
libraries. Sharing libraries is settled —
[ADR-0022](../adr/0022-offline-rendering-shares-an-api-library.md) gives them one, and
[ADR-0023](../adr/0023-rib-is-the-offline-scene-description.md) gives them one way in — so what
is left is whether talyn becomes moya's raytracing component, reached from a shader's `trace()`.
That cannot be answered before phase 3, because it depends on how much the two turn out to share
once both actually shade something, and on whether shading is a language. Deliberately last.

## What is decided

Four of the five questions this roadmap opened are settled, on 2026-09-04, in three records —
where the shared code lives and whether talyn stays an executable turned out to be one decision.
The records hold the reasoning; these are pointers, not summaries.

* **Where the shared offline code lives** — `api/render/offline`, a second library beside
  `v3dlib_render` that links neither Vulkan nor SDL, and talyn splits into a library, a driver
  and a suite the way moya already is:
  [ADR-0022](../adr/0022-offline-rendering-shares-an-api-library.md).
* **Which scene description is primary** — RIB, read by one reader in that library; the editor's
  project file stays the editor's and gains a one-way export:
  [ADR-0023](../adr/0023-rib-is-the-offline-scene-description.md).
* **Whether the offline renderers use `api/type`** — yes, and a convention only one renderer
  needs becomes a parameter of the type rather than a second copy of it, starting with the clip
  space `Camera` builds: [ADR-0024](../adr/0024-api-type-serves-both-renderers.md), which
  narrows [ADR-0012](../adr/0012-camera-builds-vulkan-clip-space.md) without reversing it.

## What this still needs decided

**Nothing this roadmap opened is still open.** The fifth question — fixed-function shading or a
shading language — is answered as a language, and phase 3's plan writes the record for it. What
that answer does *not* settle is phase 6: a shader can call `trace()`, which is what makes the
question of whether moya's raytracing is talyn answerable, and answering it is still that phase's
own work.

## Verification

This is the part worth knowing before any of the above starts.

**The offline renderers are the only renderers in this tree CI can run.** Everything below the
recorder in `api/render` needs a window and a GPU and is waiting on
[ADR-0007](../adr/0007-ci-rendering-tests.md); a rendering change is verified today by running
an app and reading the validation log. Neither of these renderers touches a window, a device or
a swapchain — `moya/tests/CMakeLists.txt` already says so, and its suite runs in CI now.

Phase 1 collected on that. `talyn/tests/data/reference-triangle.png` and
`moya/tests/data/reference-polygon.png` are committed, and `ctest -R talyn` and `ctest -R moya`
each render a scene and compare it against one with `image::compare` — a tolerance rather than
an equality, for float rounding across compilers. That is the first real render regression test
the repo has had, and it is the reason phase 1 was phase 1 rather than a detail of phase 2.

**Each reference is reached by two routes.** Phase 2 added a `.rib` beside each `.png`
describing the same scene, so the file path and the code path are pinned to one picture apiece:
if they ever disagree, a case says which. That is what makes the reader a rendering change
rather than a parsing one.

**Both references are expected to be regenerated** as phases 3 to 5 change shading and sampling
on purpose. What they buy is that a change which was *not* meant to alter the picture says so. A
failing case, or a missing reference, writes what it rendered to `data_out/` beside the
executable — chasing a CI failure without the actual image is most of the cost of a reference
test.

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
