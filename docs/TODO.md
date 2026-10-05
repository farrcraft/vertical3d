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

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables the bugprone, performance, misc and readability families
and disables 20 checks by name. The tree is clean at the 186 checks left enabled.

Seven of the disabled checks are disabled for good, and `.clang-tidy` says why; they are not
listed here. The other thirteen are in the table below, with the number of distinct sites each
reports over a full `-DV3D_CLANG_TIDY=ON` build.

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

Each of these waits until a scene needs it.

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

[] **`offline::trace::Scene::nearest` tests every primitive.**

- moya traces shadow rays, so every shadow ray from every grid point pays for the whole scene.
- An acceleration structure goes inside `offline::trace`. Neither renderer needs to change.
- Due when a scene takes a second to render. The test suites' timings would show it.

[] **moya's `--grid` and `--bucket` do not override a scene's `Option "limits"`.** The driver
applies them before reading the scene, so a scene that names its own sizes replaces them.
Applying them again after the read would make the command line win.

Background: [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration).

## Tile grids

`api/grid` is a library of its own, and odyssey uses it. The grid owns a map's picture and
terrain legend; the rest of a map belongs to the game. [Grid.md](api/Grid.md) describes the
library.

[] **A world cannot be made of regions.**

- A `TileGrid` is one rectangle centred on the world origin.
- A world of regions needs each region offset in the world, and loaded and released with its
  sprite sheets. There is no way to place a grid like that.
- A region could be a grid with an origin, a grid of grids, or a game's list of grids. The first
  game with regions decides. A world origin on `TileGrid` is the most likely answer, and it
  changes every conversion between world and tile coordinates.
- Background: [LargeWorlds, step 7](plans/completed/LargeWorlds.md#step-7--regions).

[] **Remembered sight belongs in `api/grid`.**

- Fog of war that keeps seen ground revealed is in odyssey's
  [tile/Sight.h](../odyssey/tile/Sight.h), but nothing about it is specific to odyssey.
- Move it to `api/grid` when a second game needs it.
- Background: [LargeWorlds, step 8](plans/completed/LargeWorlds.md#step-8--remembered-sight).

[] **`TileFilter` is a `std::function`.**

- It is called for every neighbour of every visited tile.
- Make it a template parameter if a board becomes large enough for the cost to show. A
  region-sized board is the most likely case, so this waits for regions.
- Background: [LargeWorlds, step 9](plans/completed/LargeWorlds.md#step-9--the-movement-filter-as-a-template).

## Voxel

[] **A remeshed chunk destroys its old mesh too early once uploads become asynchronous.**

- Today a remeshed chunk destroys its old mesh when the last reference goes. That is safe only
  because `memory::Uploader` waits for the queue to go idle after every copy, as
  [ChunkMeshPool.h](../voxel/src/voxel/ChunkMeshPool.h) says.
- When uploads stop waiting for the queue (see [Loading](#loading)), the old mesh must be retired
  through `frame::Ring::retire` instead.

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
models with `brep::BRep`. So the gap below comes from the library's own tests, not from an app.
[Assets.md](api/Assets.md) describes the loader.

[] **External buffers are resolved by cgltf, not by the asset manager.** A `.gltf` file with
external buffers has them resolved relative to the file, which is cgltf's own behaviour. It
matches the asset manager's path handling only because the manager passes cgltf a full path.

## Lit scenes

[] **A shadow map covers only a fixed sphere.**

- `shadow::fit` fits the shadow map to its casters once. Anything that moves out of that sphere
  is not covered.
- A world larger than one look-dev scene needs the fit to follow the camera, and beyond that,
  cascaded shadow maps.
- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

[] **The grade tests allow a one-step tolerance that may not be needed.**

- The identity grade and the inverting grade were exact on the authoring GPU (a Radeon), but
  [PostTest.cpp](../api/render/tests/device/PostTest.cpp) allows a difference of one step,
  because lavapipe, the software Vulkan driver CI uses, had not been checked.
- If CI shows lavapipe is exact too, reduce the tolerance to zero. If it is not, add a comment
  beside the case saying lavapipe is the reason.
- Background: [LitScene, step 10](plans/completed/LitScene.md#step-10--the-chain-after-the-scene).

[] **There is no instanced drawing.**

- `meshes()` and `casters()` submit one draw item per part per entity. A scene with a dozen
  characters and a few dozen props is a few hundred draws.
- Instancing would move `Lit::Object` from the push constant block into a per-frame storage
  buffer, read by `gl_InstanceIndex`, beside the joint palette at set 2. Draw items would be
  grouped by mesh entry and part. The recorder can already draw instances.
- Due when a scene's object count or a profile of recording time needs it.
- Background: [SkeletalAnimation, step 8](plans/completed/SkeletalAnimation.md#step-8--instancing-held).

## Loading

[] **Every load runs on the main thread**, so each load is a frame hitch the size of the load.

- First piece: decode on a worker thread through its own `asset::Manager`, because the manager's
  loaders are shared and stateful. The main thread polls once a frame and uploads what is ready
  through `TextureFactory::create(image)` and `MeshRegistry::add(name, model, albedos)`.
- Before any thread is added, a texture named by a glTF file must be decoded on the load side.
  Today `MeshRegistry::acquire` decodes it inside the upload.
- This would be the first thread in `api/`, which needs an ADR of its own.
- Due when a game streams regions, or any load is long enough for a player to see.
- Background: [ShellAndShipping, step 12](plans/completed/ShellAndShipping.md#step-12--asynchronous-loading-held).

## Frames

[] **A minimised window busy-loops and floods the log.**

- When the window has no area, `Engine3D::beginFrame` presents an empty frame.
- `Swapchain::create` is asked for a new swapchain on every frame, and logs
  `Window has no area` at info level each time.
- Nothing in the loop waits. Two seconds minimised wrote twelve thousand lines to pong's log.
- Possible fixes: the loop waits for an event while the window is minimised, or the swapchain
  logs only the change of state.
- Background: [ShellAndShipping, step 6](plans/completed/ShellAndShipping.md#step-6--one-screen-and-four-apps-on-it).

## Audio

`audio::Engine` plays a voice on a named bus, with a fade and a gain. Nothing gives a voice a
position.

[] **A voice has no pan.**

- A footstep on the left of a fixed camera's screen should sound from the left.
- The caller converts a position to a pan, because in a flat world under a fixed camera that
  mapping belongs to the game.
- `Play` would gain a pan value, and the engine a `pan(voice, value)` call, using whatever
  SDL_mixer 3 offers for a track.
- Due when a game needs positional sound.
- Background: [Effects, step 10](plans/completed/Effects.md#step-10--a-panned-voice-held).

## Tests

[Testing.md](contributing/Testing.md) describes what the suites cover.

[] **Nothing asserts the pipeline cache.** The device suite covers everything below the
recorder in `api/render` except the pipeline cache. A test for it would count what was compiled,
not compare a picture.

[] **Three things need a window or a sound device to test:** `Feature::Window`,
`ui::TextRenderer` and `audio::Engine::initialize()`. A software Vulkan driver does not help
with any of them.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. So blending, filtered sampling,
multisampling and text are checked only by a silent validation log and by spot checks. Checking
them more strongly needs a second Vulkan implementation to compare against, and this tree has
none.

## Documentation

[] **The reasons for moving to Vulkan and to SDL3 are not recorded.**
[ADR-0001](adr/0001-rendering-replace-opengl-with-vulkan.md) records the decision to move to
Vulkan, but not the reasoning, and nothing records why SDL3 replaced SDL2.

## Games

[] **pong's `data/` is not copied into the build.** `pong/CMakeLists.txt` calls
`v3d_add_shared_data(pong)` but not `v3d_add_app_data(pong)`, so a fresh build has no
`window.json` beside the executable. Every other game calls both.

[] **Three bindings name commands nothing handles:** F1 → `pong::toggleFS`,
F1 → `tetris::toggleFS` and Space → `odyssey::moveUp`.

## Editor

Open work for when the editor app itself moves forward. [Editor.md](Editor.md) describes the
editor.

[] 55 of the menu's 77 commands have no handler, and only log themselves when chosen.

[] There are no modelling operations. In a component mode you can select a face, but moving it
moves the whole object.

[] Only one thing can be selected at a time. There is no rubber-band selection and no
shift-click.

[] There is no dirty flag, so nothing warns before a load or a quit discards unsaved work.

[] The viewport panes cannot be resized by dragging.
