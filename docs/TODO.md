# TODO

This file lists open work that no open plan in [plans/](plans/) covers. It is for anyone looking
for something to pick up.

An entry is something **missing or unfinished** in the code. When an entry is done, delete it.
Do not tick it or mark it done. Everything here is open.

Two kinds of item do not belong here:

- **A finished feature that nothing calls yet.** It is not missing anything. Describe it in the
  document that owns the subject.
- **An option that was considered and rejected.** It is not unfinished. Record it in the ADR
  that made the decision.

Each entry states the gap in the code, not who is waiting for it. An entry may name a trigger:
the condition that makes the work due.

An open plan or roadmap may order entries from here. The entry stays here until the work lands,
and its section names the plan or roadmap that orders it.

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables the bugprone, performance, misc and readability families
and disables 20 checks by name. The tree is clean at the 186 checks left enabled.

Seven of the disabled checks are disabled for good, and `.clang-tidy` says why; they are not
listed here. The other thirteen are in the table below, with the number of distinct sites each
reports over a full `-DV3D_CLANG_TIDY=ON` build.
[ToolingDebt](plans/ToolingDebt.md) clears them in order.

- To remove a row, fix every site it reports. Never widen an exclusion instead.
- `voxel/src/noise` is not counted. It is vendored code, kept unchanged, and is skipped by
  clang-tidy, `/analyze` and cpplint.

| Check | Sites |
|---|---|
| `readability-implicit-bool-conversion` | 69 |
| `bugprone-narrowing-conversions` | 111 |
| `readability-braces-around-statements` | 111 |
| `readability-math-missing-parentheses` | 131 |
| `bugprone-easily-swappable-parameters` | 233 |
| `performance-enum-size` | 303 |
| `misc-use-internal-linkage` | 526 |
| `misc-const-correctness` | 939 |
| `misc-non-private-member-variables-in-classes` | 1303 |
| `readability-magic-numbers` | 1883 |
| `readability-identifier-length` | 2483 |
| `misc-include-cleaner` | 3346 |
| `readability-uppercase-literal-suffix` | 4156 |

## RiRotate's sign

[] The sign of `RiRotate` is unverified.

- The RenderMan Interface (RI) states rotations in a left-handed coordinate system.
- moya passes the angle straight to `glm::rotate`, which rotates counter-clockwise by the
  right-hand rule.
- Nothing in the tree can tell whether this is right. Both hiders read the same transformation,
  so they always agree with each other. A reference picture made by the tree only agrees with
  itself.
- A scene lit by a rotated light did not reveal the difference either.
- To settle it, render a scene whose correct picture is known from outside this tree.

Background: [OfflineRenderingPhase3](plans/completed/OfflineRenderingPhase3.md#open-questions).

## Offline rendering

Each of these waits until a scene needs it. [OfflineShading](roadmap/OfflineShading.md) orders
them, along with RiRotate's sign above.

[] **A transform into a projected space does not divide by w.** `ptransform` drops w, so
`transform("screen", P)`, `"raster"`, `"NDC"` and `depth()` are right under an orthographic
projection and not under a perspective one.

- The fix divides by w for the projected spaces, in the transform built-ins and in a cast.
- Due when a shader reads a projected position under a perspective camera.

[] **Area lights render as point lights.**

- A RenderMan area light runs its light shader at points on a primitive.
- `AreaLightSource` binds a shader to geometry, but neither hider keeps geometry as a light.
- Sampling a light's area would work like sampling the film's lens disc.
- Due when a scene needs soft shadows. Both hiders can already cast shadows through the shared
  ray tracer.

[] **Displacement and bump mapping are not possible.** Displacement shaders are refused, and
`calculatenormal` is a stub.

- Under the reyes hider, displacement reaches into moya's dicing. It moves a grid after the grid
  is shaded, and needs the grid's bound grown by `displacementbound`.
- Under the ray hider, displacement needs tessellation, which the ray tracer does not otherwise
  do.
- Bump mapping needs derivatives across a batch of points. A traced hit is a batch of one.
- Due when a scene needs surface detail that a texture cannot give.

[] **The reyes hider ignores opacity (`Oi`).**

- A sample keeps only the nearest surface and sets its opacity to one. A translucent shader is
  opaque under `"hidden"` and translucent under `"raytrace"`.
- The fix: each sample keeps every surface it meets, and composites them front to back once the
  bucket is done. This is the ray hider's `see()` loop, applied to a sample's list of surfaces
  instead of a ray's hits.
- Due when a scene needs glass or smoke drawn by the reyes hider.

[] **The reyes hider adds every polygon to the traced scene.**

- moya fans each polygon into the traced scene as it arrives, before culling, and shades its
  lights once for it. This is what lets an off-screen caster cast a shadow.
- A scene that never calls `trace()` or `transmission()` pays the memory and the time anyway.
- The fix adds geometry only when a surface or light shader in the scene traces.
- Due when a large reyes-only scene's memory or load time shows it.

[] **`solar` with an angle lights along its axis only.**

- A non-zero angle lets L be any direction inside a cone around the axis. RenderMan chooses the
  one nearest the surface's illuminance cone.
- A light shader is run with the surface's position only, so the machine cannot choose. It
  reports the angle once and uses the axis.
- The fix passes the surface's illuminance axis and angle to the light along with its position.
- Due when a scene needs a sky or another wide distant light.

[] **`offline::trace::Scene::nearest` tests every primitive.**

- moya traces shadow rays, so every shadow ray from every grid point pays for the whole scene.
- An acceleration structure goes inside `offline::trace`. Neither renderer needs to change.
- Due when a scene takes a second to render. The test suites' timings would show it.

[] **`offline::trace::Tracer` runs a shader once per hit.**

- Each hit gets its own `HitShader` and its own run of the machine, so a ray-hidden image pays
  the machine's per-run cost once per pixel sample.
- The fix batches the hits of a scanline that share a shader into one run, as the reyes hider
  shades a grid.
- Due when a ray-hidden scene's shading time shows it.

[] **moya's `--grid` and `--bucket` do not override a scene's `Option "limits"`.** The driver
applies them before reading the scene, so a scene that names its own sizes replaces them.
Applying them again after the read would make the command line win.

Background: [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration).

## moya's front end and reyes hider

Defects in `moya/libmoya` that its comments described wrongly. Each is due when a scene or a
caller reaches it.

[] **Five RI C entry points are unsafe or empty.**

- `RiProjection` reads the value after `"fov"` as a token and builds a string from it, then
  reads a `double` where RI passes a pointer to an `RtFloat`. The behaviour is undefined.
- `RiEnd` with no context open calls `pop_back` on an empty stack.
- `RiGetContext` returns a pointer into a `std::vector<RenderContext>`, so a later `RiBegin`
  that grows the vector leaves the handle dangling. `RiContext` ignores the handle.
- `RiDisplayV`, `RiPolygonV` and `RiProjectionV` are empty, and `RiDisplay` ignores its
  parameter list.
- `RtErrorHandler`, `RtProcSubdivFunc`, `RtProcFreeFunc` and `RtArchiveCallback` return
  `RtFloat` where RI has `RtVoid`, so `RiErrorIgnore` and its kin cannot be passed to
  `RiErrorHandler`.

[] **A bad name is accepted silently.**

- `RiProjection(RI_NULL)` gives an orthographic projection and an unknown name the identity.
  RI has the reverse, and nothing logs the unknown name.
- `setCoordinateSystem()` and `coordinateSystem()` with an unknown name insert an all-zero
  matrix and make it current, which flattens everything drawn after it.

[] **`RenderContext` assumes its caller's order.**

- `addPolygon()` under the reyes hider dereferences `frameBuffer_`, which exists only after
  `prepareWorld()`. A polygon sent before `RiWorldBegin` crashes.
- `render()` ignores the result of `image::Factory::write`, so an unknown extension writes no
  file and reports nothing. It also makes a new `Logger` instead of using `logger_`.
- `RiCropWindow` has no effect. `crop_` is stored by nothing and read by nothing.

[] **The reyes hider departs from RI in four places.**

- `GridShader` sets I to P for every camera. That is right for a perspective camera only; an
  orthographic camera's I is (0, 0, 1) in camera space. The ray hider is right.
- `GridShader::space("object")` fails, although the primitive's placement is that transform.
  `shade()` is given the `Shading`, not the primitive.
- ShadingRate is read as a side length in pixels: the first pass splits at
  `sqrt(gridSize) * shadingRate` pixels. RI defines it as an area.
- `Polygon::dice()` uses the first four vertices, so a polygon with more is diced wrongly.

## Tile grids

`api/grid` is a library of its own, and odyssey uses it. The grid owns a map's picture and
terrain legend; the rest of a map belongs to the game. [Grid.md](api/Grid.md) describes the
library.

[] **A route and sight stop at a region's edge.**

- A region is a grid placed by its corner, and [Grid.md](api/Grid.md#regions) states the rules.
  odyssey loads its regions as the player approaches and crosses between them by stepping.
- `findPath` and `hasLineOfSight` each take one grid, so odyssey's clicked routes and its sight
  stay within the region the player stands in. The next region is dark until the player crosses.
- Joining them needs a search over several grids, or a game that stitches one together.
- Due when a game needs a route, or sight, that crosses a seam.

[] **Remembered sight belongs in `api/grid`.**

- Fog of war that keeps seen ground revealed is in odyssey's
  [tile/Sight.h](../odyssey/tile/Sight.h), but nothing about it is specific to odyssey.
- Move it to `api/grid` when a second game needs it.
- Background: [LargeWorlds, step 8](plans/completed/LargeWorlds.md#step-8--remembered-sight).

## Voxel

[] **Mouselook turns far too fast and rolls the view.**

- Ten pixels of horizontal mouse motion turns the view most of the way round and tips it over.
- The code reads one pixel as one unit of `Player::look()`'s heading and pitch. That scale was
  set before relative mouse mode existed.
- Background: [ShellAndShipping, step 3](plans/completed/ShellAndShipping.md#step-3--a-relative-mouse).

## Sprite sheets

The api has every piece of a sprite sheet round trip: `image::TextureAtlas` places regions,
`config::SpriteSheets` reads and writes the document that names them, and `image::crop` cuts a
region back out of a sheet.

[] **Nothing packs a sheet, and nothing unpacks a whole sheet.**

- `imagetool --crop` cuts out one region, so a caller that already knows where the sprites are
  can split a sheet one rectangle at a time.
- Nothing reads a `sprites.json` and cuts out every region it names.
- Undecided: whether this belongs in `imagetool`, in a separate `spritetool`, or with whoever
  needs it first.

## Models

`api/asset` reads glTF 2.0 into a `v3d::type::Model`. That is the only geometry the api loads
from a file. No app in this tree loads one: voxel builds its terrain procedurally and the editor
models with `brep::BRep`. The gap below therefore comes from the library's own tests, not from
an app. [Assets.md](api/Assets.md) describes the loader.

[] **External buffers are resolved by cgltf, not by the asset manager.** A `.gltf` file with
external buffers has them resolved relative to the file, which is cgltf's own behaviour. It
matches the asset manager's path handling only because the manager passes cgltf a full path.

## Lit scenes

[ToolingDebt](plans/ToolingDebt.md) takes the grade tolerance.

[] **There are no cascaded shadow maps.**

- `lighting::fitShadowToView` fits one map to the camera's view out to a distance. Over a long
  view, one map spreads its texels thinly, and near shadows lose detail.
- Cascades split the view into slices by distance, each with its own map, and the lit shader
  picks the slice a fragment falls in.
- `lighting::fitShadow` covers each caster's position and a margin, not its extent or scale. A
  caller with large casters passes a margin big enough for them.
- Due when a game's view is long enough for near shadows to look coarse.
- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

[] **The grade tests allow a one-step tolerance that may not be needed.**

- The identity grade and the inverting grade were exact on the authoring GPU (a Radeon), from a
  UNORM scene and from an sRGB one.
- [PostTest.cpp](../api/render/tests/device/PostTest.cpp) still allows a difference of one step,
  because lavapipe, the software Vulkan driver CI uses, had not been checked.
- If CI shows lavapipe is exact too, reduce the tolerance to zero. If it is not, add a comment
  beside the case saying lavapipe is the reason.
- Background: [LitScene, step 10](plans/completed/LitScene.md#step-10--the-chain-after-the-scene).

## Loading

[] **A texture has no request of its own.**

- `MeshRegistry::request` returns a mesh handle at once and fills it later. A texture handle
  cannot do the same, because `pipeline::Resources` would have to swap the image behind a live
  handle and rewrite its descriptor set.
- A worker job that decodes the image, and a completion that calls `Textures::texture(image)`,
  covers the need meanwhile.
- Due when a game wants a texture handle before its pixels arrive.

[] **An upload still waits for its copy.**

- `memory::Uploader::oneShot` waits on its own fence, so the caller can free its staging buffer
  when it returns. Frames in flight keep running, and the main thread waits only for the copy.
- An upload that returned at once would retire its staging buffer through the frame ring.
- Due when a profile shows the fence wait itself.

[] **The upload budget's default is not measured.** `MeshRegistry::DEFAULT_UPLOAD_BUDGET` is
4 MiB, chosen without measuring a region's upload, because no game in the tree requests meshes.
Due when one does.

## Audio

`audio::Engine` plays a voice on a named bus, with a fade, a gain and a pan.

[] **A voice has no position.**

- A pan places a sound between the speakers. Nothing places it in a 3D scene, or makes it
  quieter with distance.
- SDL_mixer 3.2's `MIX_SetTrack3DPosition` places a track and attenuates it by distance, with
  the listener fixed at the origin. A caller would give each position relative to its listener.
- Due when a game needs sound in a 3D scene.
- Background: [LitScenesAtScale, step 10](plans/completed/LitScenesAtScale.md#step-10--close-phase-3).

## Tests

[Testing.md](contributing/Testing.md) describes what the suites cover.
[ToolingDebt](plans/ToolingDebt.md) takes the fixture generators and the pipeline cache.

[] **The glTF fixture generators are Python.** The repository holds no Python, and committed
scripts are TypeScript run by Node. Five generators in `api/asset/tests/data/` write glTF by hand
and can move to TypeScript: `make_embedded_fixture.py`, `make_model_fixture.py`,
`make_modes_fixture.py`, `make_parts_fixture.py` and `make_skin_fixture.py`.
`make_blender_fixture.py` runs inside Blender, which scripts only in Python, so it needs a
decision of its own: keep it as the one exception, or keep only the fixture it wrote.

[] **Nothing tests a requested model that decodes and does not upload.** `MeshRegistry::poll`
logs and abandons such a request, but no fixture decodes cleanly and then fails `upload()`'s
checks, so the branch never runs in a test.

[] **Nothing asserts the pipeline cache.** The device suite covers everything below the
recorder in `api/render` except the pipeline cache. A test for it would count what was compiled,
not compare a picture.

[] **Two things need a window or a sound device to test:** `Feature::Window` and
`audio::Engine::initialize()`. A software Vulkan driver does not help with either.
`ui::TextRenderer`'s atlas upload needs a device, which the `render_device` suite could give it.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. Blending, filtered sampling, multisampling
and text are therefore checked only by a silent validation log and by spot checks. Checking them
more strongly needs a second Vulkan implementation to compare against, and this tree has none.

## Known review findings

Defects a review found that the change under review did not introduce.
[Review.md](contributing/Review.md) says how this list is used: a reviewer reads it first and
does not report an entry again. Each entry names its class.
[ReleaseHardening](plans/ReleaseHardening.md) fixes these, except the weak test, which
[ToolingDebt](plans/ToolingDebt.md) takes.

### Weak test

[] **`a_frame_begun_again_restarts_its_claims` never claims on the slot it began twice.** It passes
without the turn counter it was written for. Its sibling test does fail without it.

## Documentation

[ReleaseHardening](plans/ReleaseHardening.md) takes this entry.

[] **The reasons for moving to SDL3 are not recorded.** No ADR records why SDL3 replaced
SDL2.

## Editor

Open work for when the editor app itself moves forward. [editor/](editor/README.md) describes the
editor. [EditorModelling](roadmap/EditorModelling.md) orders these.

[] 55 of the menu's 77 commands have no handler, and only log themselves when chosen.

[] There are no modelling operations. In a component mode you can select a face, but moving it
moves the whole object.

[] Only one thing can be selected at a time. There is no rubber-band selection and no
shift-click.

[] There is no dirty flag, so nothing warns before a load or a quit discards unsaved work.

[] The viewport panes cannot be resized by dragging.
