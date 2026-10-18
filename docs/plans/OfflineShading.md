# Offline Shading — Area Lights, Opacity, Displacement and a Faster Ray Tracer

Drafted 2026-10-17 against `261fc9e0`. This plan takes up every section of
[the offline shading roadmap](../roadmap/OfflineShading.md). Its steps fall in `api/render/offline`,
`moya` and the offline documents. When it closes, moya renders soft shadows from area lights,
translucent surfaces under the reyes hider, and bumped and displaced surfaces. Its rotations are
checked against a worked example in the RenderMan Interface specification.

moya is the one offline renderer. It has two hiders: the reyes hider (`"hidden"`), which dices
primitives into grids of micropolygons and shades a grid at a time, and the ray hider
(`"raytrace"`), which shades one hit at a time. Both trace shadow and reflection rays through
`offline::trace`. [The offline renderer](../offline/README.md) describes the pipeline.

The plan keeps the roadmap's order, with four changes:

* **A step of corrections goes first.** The survey found the roadmap wrong about the tree in several
  places, and a later step would trip on each.
  [Step 1](#step-1--what-the-roadmap-got-wrong).
* **Derivatives are a step of their own, before bump mapping and displacement.** The machine has
  no `Du`, `Dv`, `Deriv` or `area` at all, not only a stub `calculatenormal`. Both features need
  them, and displacement cannot recompute a normal without them.
  [Steps 6 to 8](#step-6--the-record-how-a-hider-supplies-derivatives).
* **Batching traced hits no longer carries bump mapping.** A scanline's hits lie on different
  primitives and are not a grid, so differences across them are not derivatives. The ray hider
  gets derivatives another way, and batching for speed is held until a timing asks for it.
  [Step 9](#step-9--held-traced-hits-shaded-in-batches).
* **Area light samples are seeded per shading point, not from the film.** The reyes hider shades
  grid vertices before any film sample exists.
  [Step 10](#step-10--the-record-how-an-area-light-is-sampled).

## What the survey found

The tree was read at `261fc9e0`. Line numbers are from that commit.

**Transformations:**

* **`RiRotate`'s sign is decided in one place**, `RenderContext::rotate`
  (`moya/libmoya/RenderContext.cxx:463-466`), which passes the angle to `glm::rotate`.
  `RenderContextTest.cxx:116-128` pins the present sign: 90 degrees about z takes x to y.
* **The specification pins the sign, and moya agrees with it.** `RiRotate`'s own entry gives no
  direction. Appendix D's "RIB Entity File example" (RISpec 3.1, page 175) does: it draws one
  polygon at z = +0.5, the far face in the left-handed camera space, and names each rotated copy.
  `Rotate 90 0 1 0` gives the right face, so +z goes to +x. `Rotate 90 1 0 0` gives the bottom
  face, so +z goes to −y. `glm::rotate` gives both. The sign needs a test, not a fix.
* **`sl::ptransform` drops w** (`sl/Types.cxx:101-103`). The transform built-ins, `depth()` and
  the cast in `Machine::transform` (`sl/runtime/Machine.cxx:305-340`) all go through it.
* **A correct divide already exists**: `moya::project()` (`RenderContext.cxx:44-50`), with a
  smallest w. The sl runtime cannot reach it, because it lives in moya.
* **The ray hider refuses `"screen"`, `"raster"` and `"NDC"`** (`trace/HitShader.cxx:27-47`), so
  a shader that reads a projected position renders differently under the two hiders.

**The ray tracer:**

* **`trace::Scene` is a flat list of primitives**, and `nearest()` tests each one
  (`trace/Scene.cxx:90-130`). Shadows call `nearest()` once for each surface they pass through
  (`Tracer::transmitted`, `Tracer.cxx:64-97`). There is no any-hit query.
* **`trace::Primitive` has no bound.** `Triangle` stores world-space corners. `Sphere` stores a
  transform into object space. A moving primitive is stored at its reference pose, with its poses
  beside it.
* **`type::geometry::AABBox` and `Ray::intersects(const AABBox&, float*)` exist** in `api/type`,
  which `v3dlib_render_offline` already links.
* **The ray hider casts one sample at a time** (`moya/libmoya/RayHider.cxx:119-178`), and
  adaptive sampling decides per pixel whether to cast another pass.

**Lights:**

* **`AreaLightSource` reaches the handler as an ordinary light.** `rib::Handler::areaLightSource`
  forwards to `lightSource`, moya does not override it, and `RiAreaLightSourceV` calls
  `RiLightSourceV` (`RenderMan.cxx:511-515`).
* **A light runs at the origin of its placement** (`Globals::shine`, `sl/Globals.cxx:92-115`).
* **The lens disc sampler is private to `Sampler.cxx`** (`Sampler.cxx:20-45`), in an anonymous
  namespace.
* **`solar` drops its angle.** The illuminance axis and angle are known to `Machine::inside()` and
  are not passed to `Renderer::light` (`Machine.cxx:342-367`).

**The reyes hider:**

* **A sample keeps one surface.** `Bucket::hide()` sets `sample.opacity` to one
  (`reyes/Bucket.cxx:302-325`). The grid shader does not read `Oi` (`reyes/GridShader.cxx:194`),
  and `Vertex` has no opacity.
* **The film resolves once after every bucket** (`Samples::resolve`, `reyes/Samples.cxx:53-68`).
  That is where compositing a sample's surfaces fits.
* **A grid is shaded and then hidden** in `Bucket::render` (`Bucket.cxx:408-413`).
* **Every polygon is added to the traced scene before culling** (`RenderContext::addPolygon`,
  `RenderContext.cxx:733-741`).

**Displacement and derivatives:**

* **A displacement shader is refused** by `sl::syntax::Shader::supported()` and by the compiler
  (`sl/Compiler.cxx:106-108`). The RIB reader does not recognise `Displacement`, `RiDisplacement`
  is empty, and `RiAttribute` is empty, so `displacementbound` reaches nothing.
* **`calculatenormal` is a stub**, and `Du`, `Dv`, `Deriv` and `area` are not declared. The
  machine has no access to a neighbouring point in a batch.
* **`du` and `dv` are set by the reyes hider** to one over the grid's span, and left at zero by
  the ray hider.

**The command line:**

* **`--grid` and `--bucket` are applied before the scene is read** (`moya/moya/moya.cxx:26-60`),
  so a scene's `Option "limits"` replaces them. The reader renders at `WorldEnd`, and the bucket
  size is fixed when the world begins (`RenderContext.cxx:151`). Applying the command line after
  the read, as the roadmap says, is too late.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0092 | The acceleration structure | [4](#step-4--the-record-the-acceleration-structure) |
| 0093 | How each hider supplies derivatives | [6](#step-6--the-record-how-a-hider-supplies-derivatives) |
| 0094 | How an area light is sampled, and from what seed | [10](#step-10--the-record-how-an-area-light-is-sampled) |
| 0095 | Displacement under the ray hider | [16](#step-16--the-record-displacement-under-the-ray-hider) |

Each goes in as `proposed` and is accepted when the step that builds it lands.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--what-the-roadmap-got-wrong) | The roadmap and TODO.md corrected | `docs` | — | done |
| [2](#step-2--rirotates-sign-checked-against-the-specification) | `RiRotate`'s sign checked against the specification's cube | `moya` | — | done |
| [3](#step-3--projected-spaces-divide-by-w) | Projected spaces divide by w, and the ray hider answers them | `api/render/offline` | — | done |
| [4](#step-4--the-record-the-acceleration-structure) | The record: the acceleration structure | `docs/adr` | **0092** | done |
| [5](#step-5--a-bounding-volume-hierarchy-in-the-traced-scene) | Primitive bounds, and the structure in `trace::Scene` | `api/render/offline` | 0092 | done |
| [6](#step-6--the-record-how-a-hider-supplies-derivatives) | The record: how a hider supplies derivatives | `docs/adr` | **0093** | done |
| [7](#step-7--derivatives-under-the-reyes-hider) | `Du`, `Dv`, `Deriv`, `area` and `calculatenormal`, under the reyes hider | `api/render/offline`, `moya` | 0093 | done |
| [8](#step-8--derivatives-under-the-ray-hider) | The same built-ins under the ray hider | `api/render/offline` | 0093 | done |
| [9](#step-9--held-traced-hits-shaded-in-batches) | Traced hits shaded in batches | `api/render/offline`, `moya` | — | held |
| [10](#step-10--the-record-how-an-area-light-is-sampled) | The record: how an area light is sampled | `docs/adr` | **0094** | done |
| [11](#step-11--an-area-light-keeps-its-geometry) | An area light keeps its geometry | `api/render/offline`, `moya` | 0094 | done |
| [12](#step-12--area-lights-shine-from-their-surface) | Area lights shine from points on their surface | `api/render/offline`, `moya` | 0094 | done |
| [13](#step-13--opacity-under-the-reyes-hider) | Opacity under the reyes hider | `moya` | — | open |
| [14](#step-14--a-displacement-shader-reaches-the-renderer) | A displacement shader compiles and reaches the renderer | `api/render/offline`, `moya` | — | open |
| [15](#step-15--displacement-under-the-reyes-hider) | Displacement under the reyes hider | `moya` | — | open |
| [16](#step-16--the-record-displacement-under-the-ray-hider) | The record: displacement under the ray hider | `docs/adr` | **0095** | open |
| [17](#step-17--displacement-under-the-ray-hider) | Displacement under the ray hider, as 0095 decides | `api/render/offline`, `moya` | 0095 | open |
| [18](#step-18--solar-with-an-angle) | `solar` with an angle | `api/render/offline`, `moya` | — | open |
| [19](#step-19--the-traced-scene-only-when-something-traces) | The reyes hider fills the traced scene only when something traces | `api/render/offline`, `moya` | — | open |
| [20](#step-20--the-command-line-wins-over-option-limits) | The command line wins over `Option "limits"` | `moya` | — | open |
| [21](#step-21--the-documents) | The offline documents state the new rules | `docs` | — | open |

## What blocks what

```
1 Corrections
2 RiRotate                  (no order)
3 Divide by w ──> every step that adds a reference: 7, 8, 12, 13, 15, 17
4 ADR ──> 5 BVH ──┬──> 9 Batched hits (held)
                  └──> 12 Area lights
6 ADR ──> 7 Reyes derivatives ──> 8 Ray derivatives
10 ADR ──> 11 Geometry kept ──> 12 Area lights
13 Reyes opacity            (after 2)
14 Displacement request ──> 15 Reyes displacement (after 7) ──> 16 ADR ──> 17 Ray displacement (after 8)
18, 19, 20                  (no order)
21 Documents                (last; each step also updates its own section)
```

Steps 1 and 2 depend on nothing. Step 2 changes no picture, because moya already rotates as the
specification does. Step 3 comes before any step that adds a reference picture, because a wrong
projection would be baked into it. Step 12 needs steps 5 and 11, because an
area light multiplies the shadow rays. Step 15 needs steps 7 and 14. Step 17 needs steps 8 and 16.
Steps 18 to 20 can land at any point.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze` and
  clang-tidy stay clean.
* **The suites.** The offline suite and moya's suites pass. Each step with a CPU half adds cases,
  and `failsfirst.ts` is run on the change to show that each new case fails without it.
* **The references.** moya's references are compared at a tolerance of one 8-bit step
  ([Testing.md](../contributing/Testing.md#moya-references)). A step that must not change a
  picture leaves every committed PNG untouched. A step that adds a feature adds a reference scene
  for each hider that supports it, reached from code and from RIB like the others.

---

### Step 1 — What the roadmap got wrong

**Done.** The roadmap and TODO.md state each point below, and TODO.md's offline section names
this plan.

**Documents only.** Nothing changes a picture.

* The roadmap's step 1 and its verification say `RiRotate`'s sign needs a picture from outside
  the tree. The specification's own worked example settles it.
  [Step 2](#step-2--rirotates-sign-checked-against-the-specification).
* The roadmap's step 7 says to apply `--grid` and `--bucket` after the scene is read. The read
  renders, so the command line has to win while the scene is read. [Step 20](#step-20--the-command-line-wins-over-option-limits).
* The roadmap's step 3 says batching a scanline's hits gives bump mapping its derivatives. It
  does not. The hits lie on different primitives, and adaptive sampling decides per pixel.
* The roadmap's step 6 says displacement moves a grid after it is shaded. RenderMan runs the
  displacement shader after dicing and before the surface shader, so the surface shader sees the
  moved `P` and `N`.
* The roadmap's step 4 says an area light is sampled through the film's seeded samples. Under the
  reyes hider a grid is shaded before any film sample reaches it.
* The roadmap and TODO.md name `calculatenormal` as the missing derivative. `Du`, `Dv`, `Deriv`
  and `area` are missing too.
* The roadmap links this plan, and [plans/README.md](README.md) lists it.

**Tests:** none. The suites pass and the references are untouched.

### Step 2 — `RiRotate`'s sign, checked against the specification

**Done.** `moya_ribhandler_rotations_build_the_specification_cube_test` reads
`spec-cube.rib` and finds every face where the specification names it. With the angle negated in
`RenderContext::rotate`, the right, left, bottom and top faces fail, and the far and near faces
pass, as they should. The rule is stated on `RenderContext::rotate` and in
[Rib.md](../offline/Rib.md). The TODO entry is deleted. No picture changed.

**The specification's unit cube is the test.** Its RIB goes into `moya/tests/data/` as
`spec-cube.rib`, with a camera and a `Format` added. Each face is the same polygon at z = +0.5
under a different rotation, so the sign decides where each face lands.

* **A moya case reads it through the RIB reader**, and checks the centre of each face in camera
  space against the face the specification names. Those are far +z, right +x, near −z, left −x,
  bottom −y and top +y.
* **A comment on `RenderContext::rotate` states the rule**: a positive angle turns +z towards +x
  about y, and +z towards −y about x, as the specification's example requires.
* **The TODO entry for `RiRotate`'s sign is deleted.** It is the only TODO entry this step
  closes.

**Tests:** the cube case. The step changes no code for the case to fail against, so it is checked
by hand: with the angle negated in `RenderContext::rotate`, the case must fail. No reference
picture changes.

### Step 3 — Projected spaces divide by w

**Done.** `sl::project` holds the divide and the smallest w, and moya's `project()` calls it.
`Renderer::projected()` answers for the three spaces, and the machine divides for them in
`ptransform`, `depth()` and a point cast. `trace::Scene::projection()` carries the camera's
screen, raster and NDC matrices. moya sets them when the world begins, and both `GridShader` and
`HitShader` answer from them, so the NDC rule now lives in one place. The four cases are in
`ProjectedSpaceTest.cxx`, and two machine cases with a stand-in projection are in
`SlLibraryTest.cxx`. No reference changed. The TODO entry is deleted.

* **`sl::ptransform` gains a projected form** that divides by w, with the same smallest w as
  `moya::project()`. That rule moves into `api/render/offline`, and moya's `project()` calls it.
* **The renderer says which spaces are projected.** `sl::runtime::Renderer` gains a query,
  answered true for `"screen"`, `"raster"` and `"NDC"`. The transform built-ins, `depth()` and the
  cast use the projected form for those spaces, in either direction.
* **The ray hider answers the projected spaces** from the camera it already uses to build its
  rays (`RayHider.cxx:122`), so a shader that reads `depth()` gives the same answer under both
  hiders.

**Tests**, headless:

* `transform("raster", P)` under a perspective camera lands on the pixel the hider samples, for a
  point off the axis;
* `depth()` under a perspective camera is the NDC z of the divided point;
* a round trip `transform("raster", "camera", transform("camera", "raster", P))` returns P;
* the same three under the ray hider give the same values as under the reyes hider.

No reference changes. No shipped shader reads a projected space, and the fake NDC in
`SlLibraryTest` has a w of one.

### Step 4 — The record: the acceleration structure

**Done.** ADR-0092 is written as the recommendation below, and the decision was agreed. It is
accepted when step 5 lands.

**ADR-0092.** The recommendation is a **bounding volume hierarchy built with a binned surface area
heuristic**.

* A scene here mixes scales: a ground quad several hundred units wide beside small triangles. A
  hierarchy adapts to that. A uniform grid does not.
* A moving primitive's bound is the union of its poses' bounds. A hierarchy holds that bound like
  any other. A grid would place a fast-moving primitive in many cells.
* The structure is built once, when the world ends and before the first ray. Neither hider changes.

**Alternatives the record weighs:**

* **A uniform grid.** Simpler to build and to step through. It is poor on mixed scales, and its
  resolution is one more limit to tune.
* **A kd-tree.** Traversal is often faster. A primitive can sit in several leaves, and a build
  with a good split heuristic costs more.
* **A hierarchy of moving primitives kept apart.** The still primitives go in the hierarchy and
  the moving ones stay in a list. It is simpler, and a motion-blurred scene stays slow.

### Step 5 — A bounding volume hierarchy in the traced scene

**Done.** `trace::Hierarchy` holds the boxes, and `Scene::nearest` walks it. `Primitive::bound()`
returns false by default, so a primitive with no bound, like the test suite's infinite floor, is
tested against every ray. A tie between two hits at one distance goes to the primitive added
first, which keeps every answer equal to the linear search. The oracle in
`TraceHierarchyTest.cxx` checks that for still and moving scenes. ADR-0092 is accepted.
The new cases fail to build without the change, which proves little, so the hierarchy was also
broken by hand twice. With moving primitives boxed at their stored pose, the moving case fails
at two of its three times. With a tie given to the primitive added last, the tie case fails.

No committed picture changed. A Release render of a 70 by 70 heightfield, 9800 triangles at
160 by 120 with a shadowing distant light, wrote the same bytes before and after:

| | Before | After |
|---|---|---|
| Ray hider, Release | 31.0 s | 0.4 to 0.6 s |
| Reyes hider, Release | 503 s | 4.8 s |
| moya suite, Debug | 63 s | 46 s |
| Offline suite, Debug | 2.7 s | 15 s |

The offline suite is slower because the new oracle cases search every primitive for every ray
in Debug. The Release build is `out/build/x64-Release`, configured over x64-Debug's vcpkg
install with the manifest install off, as Build.md configures the starter.

* **`trace::Primitive::bound()`** returns an `AABBox` in world space at the reference pose.
  `Triangle` bounds its corners. `Sphere` bounds its eight object-space corners under the inverse
  of its transform.
* **A moving primitive's bound is swept** across its poses. The hierarchy tests the swept bound,
  and the primitive tests the ray at the ray's time, as it does now.
* **`trace::Scene` builds the hierarchy** the first time it is asked for a hit after a primitive
  was added. `nearest()` walks it, nearest child first, and stops when the next box lies beyond
  the nearest hit.
* **The linear search stays, private to the tests**, as the oracle the hierarchy is checked
  against.

**Tests**, headless:

* for a few thousand random triangles and spheres and a few thousand seeded rays, the hierarchy's
  nearest hit is the oracle's, primitive and distance;
* the same for a moving scene at three times across the shutter;
* an empty scene, a scene of one primitive, and a ray that misses every box.

**No committed picture changes.** The time each suite takes is recorded before and after, in the
step's state note. A Release render of a scene of about ten thousand triangles is timed as well,
because the suites' scenes are too small to show the gain.

### Step 6 — The record: how a hider supplies derivatives

**Done.** ADR-0093 is written as the recommendation below, and the decision was agreed. It is
accepted when step 7 lands.

**ADR-0093.** SL's `Du`, `Dv`, `Deriv` and `area` are differences across neighbouring shading
points. `calculatenormal` is the cross product of `P`'s two derivatives.

* **The reyes hider uses its grid.** A derivative is the difference to the next vertex along u or
  v, divided by `du` or `dv`. On the last row or column it is the difference to the previous one.
  This is what RenderMan does.
* **The recommendation for the ray hider is a stencil on the primitive.** Each hit is shaded as a
  batch of three points: the hit, and the points a small step away along the primitive's u and v.
  The step is the size of a pixel at the hit's distance. The machine then computes derivatives the
  same way under both hiders.
* **The machine learns the batch's shape** from the renderer: a grid of a given width and height,
  or a hit with its two neighbours. `Du` of a value reads the neighbouring lane that shape names.

**Alternatives the record weighs:**

* **Ray differentials.** Each ray carries its rate of change across the image. Derivatives are
  exact for the ray's footprint, which suits texture filtering too. It means writing every
  built-in's derivative by hand rather than differencing the shader's own values.
* **A scanline's hits as the batch**, as the roadmap proposed. Neighbouring hits can lie on
  different primitives, and adaptive sampling casts a pixel's passes at different times.
* **Derivatives under the reyes hider only.** It is simpler, and bump mapping then differs between
  the hiders.

### Step 7 — Derivatives under the reyes hider

**Done.** `Renderer::Shape` names a batch's layout: lone points, a grid, or the stencil step 8
uses. The library implements the five built-ins over it, and `GridShader` answers with its
grid. The derivative cases are in `SlDerivativeTest.cxx`, over a stand-in renderer of each
layout, so the stencil is tested before the ray hider uses it. `GridShaderTest.cxx` checks the
grid's shape and a bumped plane against normals worked out by hand. `reference-bump.png` is a
plane bumped by noise, from code and from `reference-bump.rib`, through the `bumpy.sl` fixture.
`calculatenormal` points the way a diced polygon's `Ng` does, so it needs no flip. ADR-0093 is
accepted. Most new cases fail to build without the change, so the grid's backward difference was
also broken by hand: with the last row given no neighbour, three cases fail.

* **`Du`, `Dv`, `Deriv` and `area`** are declared in `sl/Builtins.cxx` and implemented in the
  library, for floats, colours, points and vectors.
* **`calculatenormal`** is no longer a stub. The set of stubs in `SlLibraryTest.cxx:722` shrinks
  to `shadow`.
* **`sl::runtime::Renderer` gives the batch's shape.** `GridShader` answers with its grid's size.
* **A uniform value has a derivative of zero.**

**Tests**, headless:

* on a grid over a plane, `Du(P)` and `Dv(P)` are its edge vectors divided by the grid's span, and
  `calculatenormal(P)` is the plane's normal;
* on a grid over a curved patch, the last row and column use the backward difference;
* `Deriv(f, u)` is `Du(f) / Du(u)`, and `area(P)` is the length of the cross product;
* a shader that bumps a plane with a sine of `s` shades differently from the same plane unbumped,
  and matches a value worked out by hand at three vertices.

**A reference for bump mapping**: `reference-bump.png`, a plane bumped by a seeded noise.

### Step 8 — Derivatives under the ray hider

**Done.** `HitShader` shades a hit and its two neighbours as one batch, and answers `shape()`
with the stencil. `Primitive::at()` places a neighbour on a triangle and on a sphere, and
`Scene::around()` carries it into the hit's pose. The step comes from `Scene::footprint()`, a
pixel's width through the camera projection that step 3 put on the traced scene. Only the hit
traces. The derivatives of a triangle are its own edges, against which `TraceStencilTest.cxx`
checks them, and a sphere's against its analytic derivatives. Comparing with the grid directly
would mix two parameterisations of the same polygon. `raytrace-bump.png` is the bumped plane
under the ray hider, from code and from RIB. Every committed `raytrace-` picture is unchanged,
because lane zero is shaded as before.
With the stencil switched off by hand, four of the five new cases fail. The fifth checks
`Scene::footprint()` alone.

* **`HitShader` shades a hit as the stencil ADR-0093 decides.** `trace::Primitive` gives the
  point and the normal at a (u, v) near a hit, and the step's size comes from the ray's distance.
* **`du` and `dv` are set** to the stencil's step, rather than zero.
* **A trace from inside a stencil point is not repeated.** Only the hit itself traces. Its two
  neighbours take its results for `trace()` and `transmission()`.

**Tests:**

* step 7's plane and patch cases, rerun under `HitShader`, give the same derivatives as the grid
  to a small tolerance;
* a stencil on a triangle's edge stays on the triangle's plane;
* `raytrace-bump.png`, the scene of `reference-bump.png` under the ray hider. The two pictures are
  not compared with each other, because the hiders sample differently.

### Step 9 — Held: traced hits shaded in batches

**Held until a timing asks for it.** The roadmap batched a scanline's hits that share a shader
into one run of the machine. After step 8, batching is about speed alone.

* **It is due when a profile of a Release render, after step 5, shows that preparing and running
  the machine for one hit outweighs intersection.**
* If it is taken up, a pass of a scanline batches its first samples of every pixel, and the later
  adaptive passes batch what remains. A pixel's samples are seeded per pixel, so the order does
  not change the picture.
* When the plan closes with this step still held, it moves to TODO.md with its trigger.

### Step 10 — The record: how an area light is sampled

**Done.** ADR-0094 is written as the recommendation below, and the decision was agreed. It is
accepted when step 12 lands.

**ADR-0094.**

* **An area light is the geometry declared after it, in the same attribute block.** That is the
  RI rule. The geometry renders as a surface and also shines.
* **The light shader runs at points sampled on the geometry.** Each run gives one point's colour
  and direction, and the illuminance loop sees each sample as a separate light.
* **The samples are seeded per shading point and per light.** The seed is a hash of the light's
  handle and the shading point's position. A grid vertex and a ray hit then get the same samples
  for the same point, and the order of buckets or pixels never changes a picture.
* **The number of samples is an attribute**, `Attribute "light" "integer nsamples"`, with a
  default of 16.

**Alternatives the record weighs:**

* **Seeded from the film's sample.** It is stratified against the pixel, which reduces noise under
  the ray hider. The reyes hider shades before film samples exist, so the two hiders would sample
  differently.
* **A fixed pattern for every point.** It needs no seed. It produces banded shadows that a viewer
  sees as structure rather than noise.
* **An area light as many point lights**, placed once when the world ends. It is simple and
  deterministic, and it bands the same way.

### Step 11 — An area light keeps its geometry

**Done.** `offline::Area` holds an area light's triangles and spheres in world space, samples a
point on them evenly by area, and keys its samples on the light's handle. `sl::Placed` carries
it, so both hiders' light lists reach it. moya's handler and `RiAreaLightSourceV` both call
`RenderContext::areaLightSource`, and the attribute block saves and restores which area light
is open. The concentric disc map is now `uniform::disc`, beside `uniform::triangle` and
`uniform::sphere`. No other primitive is drawn by moya, so the report for one under an area
light has nothing to report. The cases showed that `AttributeEnd` switches an area light off,
as RI does for any light made in a block. `Illuminate` switches it on again.
Picking an area light's pieces evenly rather than by area fails the weighting case, and leaving
the area light open past its block fails both geometry cases.

* **moya's handler overrides `areaLightSource`.** The geometry that follows it in the attribute
  block is added to the light as well as to the scene. `RiAreaLightSourceV` does the same.
* **The shapes are polygons and spheres.** Another primitive under an area light is reported once
  and lights as a point light, as it does now.
* **A sampling function is shared.** The lens disc's concentric map moves out of `Sampler.cxx`'s
  anonymous namespace, beside a uniform point on a triangle and on a sphere.

**Tests:** the area light reaches the context with its geometry, from code and from RIB. Geometry
after the attribute block does not join it. A point sampled on a quad lies on the quad, and the
mean of many samples is near its centre.

### Step 12 — Area lights shine from their surface

**Done.** `Globals::shine` takes the point an area light runs at for each shading point, and both
hiders pass `Area::point()` for it. An area light counts as one light for each of its samples,
and each carries its share of the light. `Attribute "light" "integer nsamples"` sets the count,
through the RIB handler and through `RiAttribute`. ADR-0094 is accepted.

Two things the plan did not foresee:

* **The standard lights ignore P.** `pointlight` shines from its `from` parameter, so moving P
  changed nothing. A built-in `arealight` shines from P, as RenderMan area light shaders do.
* **An area light shadowed itself.** A shadow ray to a sample ends on the light's own quad. The
  ray's start was offset along the normal while its direction was not, so it reached the
  quad's plane short of its end. A shadow ray is now aimed from its offset start at the light
  and stops an offset short. That moved `raytrace-sampled.png` at four of its 3072 pixels, by
  at most three levels at a shadow's edge, and it was regenerated.

`reference-area.png` and `raytrace-area.png` are a box's soft shadow on a wall, from code and
from RIB. The code route, the RIB route and the moya driver give the same bytes.
Seeding the samples from a running count instead of the shading point fails the case that
shades in two orders, and both reyes area pictures.

* **`Globals::shine` takes the point the light runs at**, rather than its placement's origin. Both
  hiders' `light()` pass a sampled point for an area light, and the origin for any other light.
* **Shadows come from the shared ray tracer**, one ray to each sample.

**Tests:**

* a light of one sample on a tiny quad gives what a point light at the quad gives;
* the same light at 16 samples gives the same picture on two runs, and with the buckets in another
  order;
* `reference-area.png` and `raytrace-area.png`: a quad light over a ground plane and a box, with a
  soft shadow.

### Step 13 — Opacity under the reyes hider

* **A vertex carries `Oi`.** The grid shader writes it back with `Ci`.
* **A sample keeps every surface nearer than its nearest opaque one**, sorted by depth. A surface
  behind an opaque one is dropped when it arrives.
* **`Samples::resolve` composites front to back** before filtering, as `Tracer::see` does for a
  ray. Depth is the nearest surface's depth.

**Tests:**

* one sample under a surface of opacity one half over an opaque red one resolves to the mix the RI
  formula gives;
* surfaces that arrive far first and near first give the same result;
* an opaque surface in front drops everything behind it;
* `reference-glass.png`: a translucent quad over a checked plane. The same scene under the ray
  hider is compared with it by eye in the state note. It is not committed as a second reference,
  because `raytrace` already renders opacity.

### Step 14 — A displacement shader reaches the renderer

* **The compiler accepts `displacement` shaders.** `P` and `N` are writable in one.
  `SlCompilerTest.cxx:595-600` and `SlParserTest.cxx:229-236` change to say they compile.
* **The RIB reader dispatches `Displacement`**, and `rib::Handler` gains `displacement`.
  `RiDisplacementV` stops being empty.
* **`Attribute "displacementbound" "float sphere"`** reaches the context, with the
  `"coordinatesystem"` form. Any other attribute is reported once, as now.

**Tests:** the request reaches a recording handler, from RIB and from the C API. A displacement
shader that moves `P` along `N` compiles and runs on a batch.

### Step 15 — Displacement under the reyes hider

* **A grid is displaced after dicing and before the surface shader**, in `Bucket::render`. The
  surface shader sees the moved `P` and the `N` the displacement shader left.
* **A primitive's bound grows by its `displacementbound`** before it is culled and before the
  split test (`RenderContext::addPolygon`). A primitive with no bound is not grown, and a
  displacement that leaves it is clipped. That is RenderMan's rule too.
* **The traced scene holds the undisplaced primitive.** A shadow of a displaced surface follows its
  undisplaced shape until step 17 decides otherwise.

**Tests:**

* a displacement of a constant amount moves a plane's grid by that amount;
* a primitive whose displaced bound reaches the view is not culled;
* `reference-displaced.png`: a plane displaced by a sine, lit from the side.

### Step 16 — The record: displacement under the ray hider

**ADR-0095.** The ray tracer intersects triangles and spheres. It does not tessellate.

**The options the record weighs:**

* **The ray hider dices a displaced primitive** into a grid with the reyes hider's dicing, runs the
  displacement shader on it, and adds the grid's triangles to the traced scene. The traced scene
  then holds displaced geometry for both hiders' shadows. It costs memory, and needs step 5.
* **The ray hider refuses displacement**, and reports it once. Bump mapping through
  `calculatenormal` still works.
* **The ray hider bumps instead of displacing.** It runs the displacement shader for its normal
  only. The silhouette is wrong.

The recommendation is the first, because the dicing code already exists and the shadows of both
hiders then agree.

### Step 17 — Displacement under the ray hider

Built as ADR-0095 decides. If the ADR decides a refusal, this step is the report and its test.

**Tests**, if the ray hider dices:

* `raytrace-displaced.png`, the scene of step 15 under the ray hider;
* a shadow cast by a displaced surface follows the displaced shape, under both hiders.

### Step 18 — `solar` with an angle

* **`Renderer::light` receives the surface's illuminance axis and angle**, and `Globals::shine`
  passes them to the light shader.
* **`solar` with an angle chooses the direction inside its cone** nearest the surface's
  illuminance cone, as RenderMan does. `solar_angle_is_reported` changes to assert the direction.

**Tests:** a cone that holds the surface's axis gives the axis. A cone that does not gives the
cone's nearest edge.

### Step 19 — The traced scene only when something traces

* **A compiled program says whether it calls `trace()` or `transmission()`.** The compiler sets
  this from the built-ins it calls.
* **The reyes hider adds polygons to the traced scene** only when a surface, light or displacement
  shader bound in the world traces. Area light geometry is always added, because its shadows trace.

**Tests:** a scene of `matte` surfaces and a point light leaves the traced scene empty. Adding one
shadowing light shader fills it. The committed pictures are unchanged.

### Step 20 — The command line wins over `Option "limits"`

* **The context holds pinned limits.** `--grid` and `--bucket` set the size and pin it. A scene's
  `Option "limits"` then leaves a pinned size alone, and reports nothing.
* The help text and the comment in `moya.cxx:40-41` change to say the command line wins.

**Tests:** a pinned grid size survives a scene that names its own. An unpinned one is replaced.

### Step 21 — The documents

Each step updates the document it moves, as it lands. This step checks the whole set at the end.

* [Reyes.md](../offline/Reyes.md): displacement, opacity, derivatives across a grid.
* [RayTracing.md](../offline/RayTracing.md): the hierarchy, the stencil, area lights,
  displacement.
* [ShadingLanguage.md](../offline/ShadingLanguage.md): the derivative built-ins, displacement
  shaders, `solar` with an angle, the projected spaces.
* [Rib.md](../offline/Rib.md): `AreaLightSource`, `Displacement`, the attributes.
* [RunningAndTesting.md](../offline/RunningAndTesting.md): the new references, and the command
  line's limits.
* TODO.md loses its offline entries and `RiRotate`'s sign, and gains step 9 if it is still held.
* The roadmap moves to [roadmap/completed/](../roadmap/completed/), and this plan moves to
  [completed/](completed/) with an Outcome section.
