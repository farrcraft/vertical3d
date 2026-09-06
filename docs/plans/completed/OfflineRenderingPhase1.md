# Offline Rendering, Phase 1 — Each Renderer Computes A Pixel

Drafted 2026-09-05, **closed 2026-09-05**. Took up phase 1 of
[the offline rendering roadmap](../../roadmap/OfflineRendering.md), which stays the account of
where both renderers stand and what the later phases are; this plan does not repeat it.

**All six steps landed.** Both renderers produce a picture whose pixels came from geometry,
each is compared against a committed reference in ctest, and neither needs a GPU. Three things
turned up that this plan did not anticipate, and are recorded in the step notes below: the
raster transform did not flip y, the display type and mode tokens were null, and a bucket sweep
dropped the pieces a split handed back behind it.

The phase exists for one reason, and it is a verification argument rather than a feature one:
**a renderer that computes nothing and a renderer that works are indistinguishable from the
outside today.** talyn's black PNG is what a correct render of an empty scene looks like, and
moya's clean exit is what a correct render of a scene it culled entirely looks like. The moment
either produces a pixel whose value came from geometry, a rendered image can be compared against
a committed reference in ctest on every push with no GPU — the first render regression test the
repo has had, since everything below the recorder in `api/render` still waits on
[ADR-0007](../../adr/0007-ci-rendering-tests.md).

So the deliverable is not a good picture. It is a picture that would change if the code broke.

## Decisions

Recorded in [docs/adr/](../../adr/), not here. The ones that shape this plan:

| ADR | Decision |
|---|---|
| [0022](../../adr/0022-offline-rendering-shares-an-api-library.md) | Shared offline code is `api/render/offline`; each renderer is a library with a driver |
| [0023](../../adr/0023-rib-is-the-offline-scene-description.md) | RIB is what both renderers read; the editor exports to it |
| [0024](../../adr/0024-api-type-serves-both-renderers.md) | `api/type` serves both, and a convention is a parameter rather than a fork |
| [0007](../../adr/0007-ci-rendering-tests.md) | Render tests on a Windows runner — which these two do not need |

## What blocks what

Steps 1 to 3 are structure. None of them renders anything, and all of them have to exist before
either renderer can be tested at all: there is nowhere to put shared code, talyn has no library
to link a test against, and there is no way to assert that an image is the right image.

Steps 4 and 5 are the actual rendering, one per renderer, and are independent of each other.
Step 6 closes the phase and is the reason for the first three.

```
1 offline library ─┬─> 4 talyn renders ─┬─> 6 reference images
2 talyn splits   ──┘                    │
3 image compare  ───> 5 moya renders  ──┘
```

## Steps

### Step 1 — `api/render/offline`, and the framebuffer both renderers share

`api/render/offline/` with its own `CMakeLists.txt`, building `v3dlib_render_offline` in
namespace `v3d::render::offline`, linking `v3dlib_type`, `v3dlib_image` and `v3dlib_log` and
naming neither Vulkan nor SDL. `api/render/CMakeLists.txt` adds it as a subdirectory **after**
its `set(CMAKE_CXX_FLAGS "/utf-8")` line, where `tests` already is — a subdirectory added above
that line does not inherit the flag, and spdlog's bundled fmt has a `static_assert` that fails
without it.

The first resident is the float-plane framebuffer, which exists twice today with the same design
and the same doc comment.

- Width, height, a stack of `float` planes, a per-pixel write, and a conversion to
  `image::Image` with row 0 at the top, which is what both buffers already assume.
- **The channel count is not the plane count.** talyn's four planes are RGBA; moya needs RGB
  plus a depth plane, and a depth plane written into alpha is a wrong picture that looks like a
  shading bug. The conversion takes how many leading planes are image channels; the rest are the
  renderer's own.
- **The conversion clamps.** `static_cast<unsigned char>(value * 255.0f)` on a value above one
  wraps rather than saturating, which turns a bright pixel dark — latent today because every
  plane is zero, and not latent the moment anything shades.
- talyn's conversion is called `render()`, on a class that renders nothing. It is `image()` on
  the shared one.
- `moya::FrameBuffer` keeps its bucket grid and holds one of these for the planes, which is what
  its `planes_` member was declared for and has never had allocated. talyn's `RenderContext`
  holds one directly.

`api/render/offline/tests` covers the plane writes and the conversion: a written plane reads
back, row 0 lands at the top of the image, 1.0 is 255, an out-of-range value saturates rather
than wrapping, and a depth plane is not an image channel.

**Done when**: both renderers build against one framebuffer, and `v3dtest_render_offline` runs
in ctest.

**Landed.** The depth plane is addressed by name rather than by position: `moya::FrameBuffer`
carries a `Plane` enum and a `CHANNELS` count, and clears its depth plane to the largest float
rather than to zero. `clear(plane, value)` on the shared buffer is what makes that possible.

### Step 2 — talyn becomes a library, a driver and a suite

Mirroring moya, which is already `libmoya` + `moya` + `tests`:

- `talyn/libtalyn` builds `v3dlib_talyn` — `RenderContext`, `RIBReader`, and the `Scene` step 4
  adds. `FrameBuffer` is gone from it, to step 1.
- `talyn/talyn/talyn.cxx` is the driver: option parsing, the extension dispatch, and the write
  through `image::Factory`. It keeps everything it does today.
- `talyn/tests` with `v3d_add_test(talyn ...)`, and the fixture copy `api/image/tests` uses, so
  a scene file resolves beside the executable.
- `talyn/CMakeLists.txt` becomes the three `add_subdirectory` lines moya's is.

The 35-item wish list and the design note at the top of `talyn.cxx` do not move with it. The
list is the roadmap's phases 3 to 5 and the note is [ADR-0022](../../adr/0022-offline-rendering-shares-an-api-library.md),
[ADR-0023](../../adr/0023-rib-is-the-offline-scene-description.md) and the roadmap's phase 6 — a
comment that repeats a plan or a record goes stale where nobody is looking.

**Done when**: `ctest -N` lists a talyn suite, and `talyn --file data/test-scene-01.rib
--outfile out.png` still writes the image it writes today.

**Landed.** The demo scene moved to `talyn/talyn/data`, which `v3d_add_app_data` copies beside
the driver - it was not copied at all before, so the command above only worked from the source
tree. The suite keeps its own fixtures in `talyn/tests/data`.

### Step 3 — a tolerance image comparison in `api/image`

Three suites need it — `image`'s own, moya's and talyn's — and a helper in a test translation
unit cannot be shared across three binaries, so it is a function in `v3dlib_image` rather than
test scaffolding.

- Equal dimensions and equal format first, then every channel within a tolerance.
- It reports **how far off** the worst pixel was and where, not just that it differed. A
  reference test that says "images differ" costs an afternoon the first time it fires.
- The tolerance exists for float rounding across compilers, not for "close enough". A test that
  needs a loose one is testing something it should not be.

**Done when**: `api/image`'s suite covers it, including that a one-channel difference of exactly
the tolerance passes and one above it fails.

**Landed.** `image::compare` returns a `Difference` that records the worst pixel whether or not
the images matched, so a comparison that passed still says how close it came.

### Step 4 — talyn renders a triangle

A scene of one camera and one triangle, primary rays through it, flat colour on hit and a
background colour on miss.

- `talyn::Scene` holds a `type::Camera`, a list of triangles each with a flat colour, and the
  background. Built in code; RIB is phase 2.
- `RenderContext::render()` walks the pixels, asks the camera for the ray through each pixel
  centre, takes the nearest hit with `type::Ray::intersects(a, b, c, &distance)`, and writes the
  colour or the background. The 30-line algorithm comment describing shadow, reflection and
  refraction rays stays as the description of phases 3 and 5, not of this.
- `Camera::ray()` reads the cached matrices, so `createProjection()` and `createView()` have to
  have been called for the viewport being rendered.
- **No `api/type` change.** The camera measures y downward from the top of the viewport and
  `image::Image` row 0 is the top, so the pixel loop maps straight across with the convention
  ADR-0012 already set. The parameter [ADR-0024](../../adr/0024-api-type-serves-both-renderers.md)
  calls for is needed when moya routes a projection through `Camera`, which this phase does not
  do.
- **Something has to write the alpha plane.** Four planes are RGBA and nothing writes the
  fourth, so today's "black PNG" is a fully transparent one, and so is a rendered one until this
  is fixed.

**Done when**: a suite case renders a triangle against a background and asserts the colour of a
pixel inside it, a pixel outside it, and one either side of an edge.

**Landed** as written, with no `api/type` change. `RenderContext::render()` sets the camera's
viewport size and rebuilds its matrices, since `Camera::ray()` reads the cached ones; the pixel
aspect stays the caller's, because only the scene knows what the frame is meant to look like.

### Step 5 — moya dices, shades flat, and writes the image

The second reyes pass, as far as one flat colour per micropolygon, then output.

Three things block `dice` before any dicing gets written:

- `dice(boost::shared_ptr<MicroPolygonGrid> grid, RenderContext &)` takes the pointer **by
  value**, and `Bucket::render` passes a null one. `MicroPolygonGrid` has no default constructor,
  so the caller cannot make the grid either — as it stands no grid can reach the caller at all.
  The parameter becomes a reference.
- `while (prim->dice(grid, rc))` loops forever unless `dice` answers false when the primitive
  has no more grids. One grid per polygon here: true once, false after.
- The grid extent comes from `rc.gridSize()`, which is 256 and means 256 *micropolygons* — so a
  grid of 17 vertices a side, since the class documents `size` vertices as `size - 1`
  micropolygons.

Then the pass itself:

- Fill the grid by bilinear interpolation over the polygon's first four vertices, degenerating a
  triangle's fourth onto its third. A polygon with more than four vertices has the rest dropped,
  and this step says so rather than pretending otherwise.
- Shade flat, from a constant. `RiSurface` is empty and materials are phase 3.
- Hide at one sample per pixel: bound each micropolygon in raster space, and for every pixel
  centre inside it write the colour where its z beats what the depth plane holds. That is the
  last three lines of the pseudocode comment in `Bucket::render`, at the coarsest sampling that
  is still a hider.
- Output through the interface rather than around it. `RiDisplay` is declared with an empty
  body: give it one that records the name and mode on the context, and have
  `RenderContext::render()` write the image through `image::Factory` once the buckets are done,
  which is what its own doc comment already says happens there. The driver's `--output` then
  reaches it as `RiDisplay(name, RI_FILE, RI_RGB, RI_NULL)` instead of being parsed and dropped.

Six things the first picture will run into. They are listed to be expected rather than
rediscovered, and none is a defect while nothing renders:

- `RiProjection("perspective")` builds an identity matrix. The first scene is orthographic,
  which is the RI default and what `prepareWorld` asks for anyway.
- The orthographic screen window is hard-coded to [-1, 1] rather than read from `screen_` and
  `frameAspect_`, so a 4:3 image stretches a square window across it.
- `addPolygon` and `dice` both compose `screen * raster`, which applies the raster transform
  before the projection. Confirm it against the first picture and reverse it.
- The camera transform is applied as `glm::transpose(coordinateSystems_["camera"])`, which is an
  inverse only for a pure rotation.
- `near_` and `far_` default to `1.0e-10` and `1.0e38`, so the orthographic depth scale is about
  `2e-38` and every z collapses onto the near plane. The scene has to call `RiClipping`.
- The sample quad in `moya.cxx` has x = 0 at all four of its vertices. It is edge-on to the
  default camera and projects to a line.

**Done when**: `moya --output out.png` writes an image with a shaded polygon in it, and a suite
case asserts a pixel inside the polygon and one outside it.

**Landed**, and three things beyond the six above turned up on the way to the first picture:

- **The raster transform did not flip y.** RI raster space has its origin at the upper left
  corner and counts downward, and so does `image::Image`; without the flip a correctly computed
  picture is written upside down. `prepareWorld` negates the y scale and translate.
- **`RI_FILE` and the mode tokens were uninitialised.** They are `RtToken` — pointers — so
  `RiDisplay(name, RI_FILE, RI_RGB, RI_NULL)` reached the context as two nulls and selected no
  output. They now carry the strings the standard gives them.
- **A bucket sweep dropped what a split handed back behind it.** The first pass buckets a piece
  by where it lands rather than by where the sweep has reached, so a piece landing in a bucket
  already visited was never rendered — holes in any picture large enough to split.
  `FrameBuffer::render` sweeps until a sweep splits nothing; a primitive already diced yields no
  further grid, so a repeated sweep costs a pass over the grid and nothing else.

The composition order was reversed as this step anticipated: `raster * screen`, not
`screen * raster`.

### Step 6 — the reference images

One scene and one committed PNG per renderer, copied beside the suite the way `api/image` copies
its fixtures, compared with step 3's function.

- The scenes are the smallest ones that fail informatively: a triangle filling a known part of
  the frame, on a background that is not black, at a size small enough to read a diff by eye.
- A failing case writes what it rendered next to the reference. Chasing a CI failure without the
  actual image is most of the cost of a reference test.
- **These references get regenerated through phases 3 to 5**, every time shading or sampling
  changes the picture on purpose. That is expected and is not a weakness of the test: what it
  buys is that a change which was *not* meant to alter the picture says so.

**Done when**: `ctest -R moya` and `ctest -R talyn` each render a scene and match a committed
reference, on a machine with no GPU.

**Landed.** `talyn/tests/data/reference-triangle.png` and `moya/tests/data/reference-polygon.png`,
both 64 by 48. Each scene is asymmetric about both axes, so a flipped picture is a failing one.
A missing reference writes the render too, not only a failing comparison — regenerating one on
purpose starts from the same file a failure hands you.

moya's background is black because nothing clears its colour planes; the renderer has no
background colour to set, and giving it one is phase 3's `RiImager` rather than this.

## Verification

The four gates in [sdlc.md](../../sdlc.md) §4, with nothing special about this phase except the
last: `ninja -C out/build/x64-Debug`, cpplint per `CLAUDE.md`, and
`ctest --test-dir out/build/x64-Debug --output-on-failure`.

Render verification is the point of the phase rather than a step in it. Neither renderer touches
a window, a device or a swapchain, so unlike every other rendering change in this tree, "run it
and read the validation log" is not the method here — ctest is.

## What this phase does not do

- **No RIB.** The tokenizer, the declaration table and the geometry requests are phase 2 and
  [ADR-0023](../../adr/0023-rib-is-the-offline-scene-description.md). A consequence worth stating
  plainly: **talyn's driver still writes a background-only image at the end of this phase**,
  because its only route to a scene is a file it cannot yet read. talyn's picture comes from its
  suite, moya's from its driver, and the asymmetry closes in phase 2. A built-in demo scene
  behind a flag would close it sooner and would be a fake renderer for a real one.
- **No shading.** One constant colour per renderer. Lights, materials and the RI standard
  shaders are phase 3, and the fixed-function-or-language question is the one thing the roadmap
  still has open.
- **No sampling.** One sample per pixel centre, no pixel filter, no supersampling. The five
  `Ri*Filter` functions keep returning `0.0` until phase 4.
- **No `api/type` change.** See step 4 — ADR-0024 is honoured by not forking `Camera`, and its
  parameter arrives when moya needs it.
- **No perspective projection in moya.** The first scene is orthographic; the identity matrix
  `RiProjection("perspective")` builds is phase 2's problem, alongside the screen window.

## Open questions

Small enough to settle in the code with a comment rather than in a record, but worth naming so
they are settled deliberately. All three are settled:

- Whether `moya::FrameBuffer` keeps its name once the planes move out of it. **It keeps it** —
  the planes did not move out, they moved *into* it: the class owns the bucket grid and holds
  the shared buffer, which is what it was already named for.
- Where `talyn::Scene` lives. **`libtalyn`.** ADR-0022's rule for promoting it to
  `api/render/offline` is a second consumer, and moya has no scene of its own to be one.
- How the depth plane is addressed. **By name** — `moya::FrameBuffer::Plane` names the three
  colour channels and the depth, and `CHANNELS` says how many of them are the picture. A
  position-only convention reads the same at the call site whether it is right or wrong.
