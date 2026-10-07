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
- The fit covers each caster's position and the margin the caller passes, not the caster's
  extent or scale, and it reads the stepped position rather than the one drawn between steps. A
  caller with large or fast casters has to pass a margin big enough for both.
- A world larger than one look-dev scene needs the fit to follow the camera, and beyond that,
  cascaded shadow maps.
- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

[] **The colour grade's table is sampled by linear colour.**

- `Grade` stores a 16-entry cube as UNORM and indexes it by the scene's linear colour. A strip
  authored for display, as most are, grades wrongly, and 16 entries over linear light leave
  little resolution in the shadows.
- The fix encodes the lookup coordinate to sRGB before sampling, or stores a larger table.
- Due when a game uses a grade authored outside the tree.

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

[] **A path outside the ANSI code page cannot be opened.**

- `engine::appPath()` builds a narrow string with `boost::filesystem::path::string()`, which
  converts through the ANSI code page. A character that page cannot hold becomes `?`.
- spdlog opens its file with a narrow name, because the vcpkg port is built without
  `SPDLOG_WCHAR_FILENAMES`. `asset::Manager` takes the narrow path as its root.
- An app under such a directory logs to stderr instead of `v3d.log`, and finds none of its
  assets.
- The fix carries paths as `boost::filesystem::path` or UTF-8 through `appPath()`, the logger
  and the asset manager, or gives every app a manifest that sets the UTF-8 code page.
- Due when a player reports it, or before a release.

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

[] **Two things need a window or a sound device to test:** `Feature::Window` and
`audio::Engine::initialize()`. A software Vulkan driver does not help with either.
`ui::TextRenderer`'s atlas upload needs a device, which the `render_device` suite could give it.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. So blending, filtered sampling,
multisampling and text are checked only by a silent validation log and by spot checks. Checking
them more strongly needs a second Vulkan implementation to compare against, and this tree has
none.

## Known review findings

Defects a review found that the change under review did not introduce.
[Review.md](contributing/Review.md) says how this list is used: a reviewer reads it first and
does not report an entry again. Each entry names its class.

### Boundary input

[] **`TileGrid` lets a tile size that is not a number through.** `TileGrid.cpp` tests
`tileSize <= 0`, which NaN and infinity pass; its header says a size that is not positive throws.

[] **The editor's orthographic zoom has no floor.** `Camera::zoom` scales the half height by a
factor that one large drag takes to zero or below, and a zero never recovers. `Isometric::zoom`
clamps; this does not.

[] **The C interface reads an integer parameter as a float.** `rib::Arguments` reads every
non-string value through `const float*`, so an `RtInt` array arrives as its bit pattern.

[] **`Polygon::clip` ignores whether an edge met the plane.** `intersectEdge`'s result is dropped
and the point it did not set is used. `Polygon::split` has the same shape.

### Contract

[] **A held activation key repeats a ui command.** `Keys::press` takes every key-down, so a held
space or return on a button, check box or list sends its command at the key repeat rate.

[] **The centred game menu draws a disabled item as live.** `ComponentRenderer` greys a disabled
item in a panel and not in the centred menu, which refuses to activate it.

[] **A theme overlays the one before it.** `Resolver::chrome` writes only what the new theme names,
so a switch keeps the old theme's values where the new one is silent, and a null theme keeps all
of them.

[] **Two views of one camera share one pass.** A layout may name a camera profile twice, and the
pass is named after the profile, so the second view overwrites the first.

[] **Playing no clip with a fade freezes the old pose.** `Playback::advance` returns before the
fade code when the clip is `none`, so the old clip is blended at weight zero for ever.

### Cleanup

[] **The direct allocator leaks a buffer or image when no memory type suits it.**
`memoryType()` throws after the object is made, and the constructors clean up only a failed bind.

[] **`Presenter` leaks its semaphores when its constructor throws part way.**

### Weak test

[] **`a_frame_begun_again_restarts_its_claims` never claims on the slot it began twice.** It passes
without the turn counter it was written for. Its sibling test does fail without it.

### Drift

[] **Stale comments and doc sentences.**

- `Cursor.h` and `docs/api/ui/Mouse.md` say a release sends nothing; a slider can send its command
  on release.
- `Menu.h` says `previous()` returns false on a wrap; it returns true.
- `Manipulator.h` says the profile's normals do not follow its rotation; they do now.
- `ProfileTest.cxx` says "looking down -z" of a camera that looks along +z.
- `Retirement.h` documents its tag as the frames begun; it is the last frame that may name the
  object.
- `docs/internals/realtime/Memory.md` and `Barriers.h` list two depth-only layouts and leave out
  the readback barrier, which needs the same feature.
- `Renderer.h` and `docs/offline/ShadingLanguage.md` call "shader" space the light's placement; it
  is the inverse.
- `docs/internals/realtime/Frames.md` does not state the scissor's rules for an edge that is not a
  number or is past 2^24.
- The root `CMakeLists.txt` says the apps name every api library between them; none names font.
- `docs/contributing/Testing.md`'s fixture list leaves out the font and ui suites, which take the
  shared font through a build rule.
- `api/asset/media/tests/CMakeLists.txt` says every glTF fixture has a generator script; three are
  written by hand.
- `RotateManipulator.h` has two doc blocks above `ringDistance`, one of which belongs to `swept()`.

### Prose

[] **Writing rules broken in text the tenth round added.** A "So" opener in
`docs/internals/realtime/Memory.md`; "which is what" in `docs/internals/realtime/Device.md` and
`docs/offline/ShadingLanguage.md`; a 52-word comment in `api/event/Bindings.cpp`; a figure of
speech in a `PostTest.cpp` comment; and lines extended past the wrap in `docs/api/Assets.md`,
`docs/internals/realtime/Device.md`, `docs/offline/CamerasAndSampling.md`,
`api/render/offline/Film.h` and `docs/internals/realtime/Memory.md`.

## Documentation

[] **The reasons for moving to SDL3 are not recorded.** No ADR records why SDL3 replaced
SDL2.

## Games

[] **Three bindings name commands nothing handles:** F1 → `pong::toggleFS`,
F1 → `tetris::toggleFS` and Space → `odyssey::moveUp`.

## Editor

Open work for when the editor app itself moves forward. [editor/](editor/README.md) describes the
editor.

[] 55 of the menu's 77 commands have no handler, and only log themselves when chosen.

[] There are no modelling operations. In a component mode you can select a face, but moving it
moves the whole object.

[] Only one thing can be selected at a time. There is no rubber-band selection and no
shift-click.

[] There is no dirty flag, so nothing warns before a load or a quit discards unsaved work.

[] The viewport panes cannot be resized by dragging.
