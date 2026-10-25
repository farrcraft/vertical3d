# Scale and Reach — Cascades, Non-Blocking Loads, Cross-Region Worlds, Sound in Space

Drafted 2026-10-24 against `ab4bdea1`. This plan takes the open feature and performance entries
in [TODO.md](../TODO.md) that are not editor work: offline rendering, tile grids, sprite sheets,
lit scenes, loading and audio. When it closes, a lit scene casts cascaded shadows, and a load
never blocks the main thread on a copy. A texture can be requested like a mesh. A route and sight
cross region seams, a voice has a position, and imagetool packs and unpacks sprite sheets.

The steps fall in `api/render/realtime`, `api/render/offline`, `moya`, `api/grid`, `odyssey`,
`api/audio`, `api/event`, `api/config` and `imagetool`. Most of them are independent of each
other. The two chains are the loading steps and the grid steps.

Each entry this plan takes stays in TODO.md until the step that does it lands, and is then
deleted there. A step with a testable CPU half brings test cases with it.

## What the survey found

The tree was read at `ab4bdea1`. Line numbers are from that commit.

**Offline:**

* **Every polygon is fanned into the traced scene as it arrives**, under both hiders
  (`moya/libmoya/RenderContext.cxx:916-933`, `trace()` at `:775-808`). The bounding volume
  hierarchy is built on the first ray (`api/render/offline/trace/Scene.h:121,176`), and the scene
  has no way to drop its triangles.
* **Nothing records whether a shader can trace.** `sl::runtime::Program` has no flag for it. The
  builtins that reach the tracer are `trace` and `transmission` (`sl/Builtins.cxx:202-203`), and
  every standard light with a direction calls `transmission()` (`sl/ShaderLibrary.cxx:130-163`).
* **The ray hider runs a machine per hit.** `Tracer::shade` builds a `HitShader` per hit
  (`api/render/offline/trace/Tracer.cxx:175-180`), each a batch of three for derivatives. The
  machine already runs a batch of any size under a mask (`sl/runtime/Machine.h:41`), as the reyes
  `GridShader` does. Compositing in `Tracer::see` is sequential per ray.
* **ADR-0093 considered a scanline's hits as the batch and rejected it for derivatives.**
  Batching for speed alone, with the stencil kept per hit, is a new choice.
* **There is no timing in moya and no benchmark harness in the tree.** An `x64-Release` build
  tree exists.

**Grids:**

* **`findPath` and `hasLineOfSight` take one grid** (`api/grid/Pathfinding.h:60`,
  `api/grid/LineOfSight.h:47`). Their filters are `std::function<bool(TileCoord)>`.
* **ADR-0087 left a route across regions to the game.** [Grid.md](../api/Grid.md#regions) says
  so. odyssey refuses a click outside the player's region (`odyssey/engine/Engine.cpp:100-124`)
  and blinds every other region (`odyssey/tile/World.cpp:349-362`).
* **odyssey's `Sight` uses only `api/grid`** (`odyssey/tile/Sight.h:8-9`). It has its own test,
  `odyssey/tests/SightTest.cxx`, and a fixed `RANGE` of 7.

**Sprite sheets:**

* **imagetool is one file** (`imagetool/imagetool.cxx`, options at `:54-68`) with no test suite.
  It links only `log` and `image`.
* **`config::SpriteSheets` reads and writes the document** (`api/config/SpriteSheets.h:31-91`).
  `image::TextureAtlas` places regions in a fixed size with a one-texel gutter
  (`api/image/TextureAtlas.h:20-58`), and `image::crop` cuts one out (`api/image/Crop.h:29`).

**Shadows:**

* **One shadow map, one matrix.** `fitShadowToView` fits a sphere from the near plane to a
  distance and snaps it to texels (`api/render/realtime/lighting/Shadow.h:102`).
  `SceneUniforms` holds one `lightViewProjection` (`lighting/SceneUniforms.h:21-26`, mirrored in
  `shaders/lit/lit.glsl:18-27`), and `cel.frag` samples one `sampler2D` with a 3x3 filter.
* **`Lit::scene()` binds one scene a frame** (`vulkan/renderer/Lit.h:146-156`), and the shadow
  pipelines read its matrix. A cascade needs a matrix per slice in the shadow pass.
* **No app draws a lit scene.** Only the device tests do.

**Loading:**

* **`Uploader::oneShot` waits on its own fence** (`vulkan/memory/Uploader.cxx:45-82`). Its callers
  are `DeviceBuffer` (`memory/DeviceBuffer.cxx:111`), `TextureFactory`
  (`memory/TextureFactory.cxx:163`) and `RenderTarget` (`frame/RenderTarget.cxx:191`).
  `frame::Ring::retire` (`frame/Ring.h:127`) already defers work to a frame's end.
* **A texture is resolved only to write a descriptor** (`pipeline/Resources.h:86`), so swapping the
  image behind a live handle means rewriting every material set that names it.
* **`DEFAULT_UPLOAD_BUDGET` is 4 MiB** (`MeshRegistry.h:106`). The engine has CPU spans through
  `Engine::measure` and GPU pass times through `Engine3D::timings()`.

**Audio:**

* **A voice is placed by `MIX_SetTrackStereo`** (`api/audio/Engine.cpp:307-314`). The installed
  SDL_mixer is 3.2.0.
* **`MIX_SetTrack3DPosition` places a track with the listener at the origin**, and attenuates it
  with distance by a fixed model. Stereo and 3D are exclusive modes: clearing either clears both.
  The centred pan passes null to `MIX_SetTrackStereo`, which would clear a position.
* **`event::kind::Sound` carries a pan**, and its comment leaves the conversion from a position to
  the game (`api/event/kind/Sound.h:16-26`). The FMOD events behind ADR-0084 have no 3D
  attributes.

## Decisions

| Decision | Step |
|---|---|
| Rendering: cascaded shadows. A texture array or an atlas, how the shadow pass gets a slice's matrix, the split scheme, and whether one map stays a single cascade. | 3 |
| Memory: how an upload returns at once. A submission per upload retired by the frame ring, copies recorded into the frame, or a transfer queue. | 4 |
| Resources: how a texture handle fills later. Swap the image and rewrite its sets, or a pending material that draws a placeholder until it resolves. | 5 |
| Grid: how a route and sight cross regions. Amends ADR-0087. A search over a set of placed grids, a stitched grid, or a search over world tiles through callbacks. | 8 |
| Audio: how a voice is placed in space. Who holds the listener, how a position and a pan relate, and what the FMOD events do. | 9 |
| Offline: a scanline's hits shaded in one batch for speed. Amends ADR-0093. | 2 |

## What blocks what

- **Step 4 before step 5.** A texture request is the first caller of an upload that returns at
  once.
- **Step 6 after steps 4 and 5.** The budget is measured on the upload path that will ship, not on
  the blocking one.
- **Step 7 before step 8.** Remembered sight moves into `api/grid` first, so cross-region sight is
  designed once, in the library.
- **Steps 1, 2, 3, 9 and 10 are independent** of everything else.

## Steps

### Step 1 — The traced scene is dropped when nothing traces

**Open.** The shading language compiler marks a program that calls `trace` or `transmission`,
directly or through a function. When the reyes hider starts rendering and no bound shader is
marked, `RenderContext` drops the traced scene's triangles. Area lights keep their own copy.

- `trace::Scene` gains `clear()`.
- Tests: the mark is set through a function call and unset without one. A scene with no shadowing
  light holds no triangles once rendering starts, and one with a shadowing light still does. The
  reference pictures are unchanged.

### Step 2 — A scanline's hits in one batch

**Open.** ADR first. The ray hider collects a scanline's first hits, groups them by shader, and
runs each group as one batch, each hit keeping its stencil of three. A hit that is not opaque
continues as before, one ray at a time, so compositing stays in order.

- Profile a Release render under `Hider "raytrace"` before and after, by hand. moya gains a
  render time in its log, so the comparison can be repeated.
- Tests: the ray-traced reference pictures are identical, since samples are seeded per pixel.
  A batch with mixed shaders gives each hit its own shader's result.

### Step 3 — Cascaded shadow maps

**Open.** ADR first.

- A function fits a slice of the view between two distances, as `fitShadowToView` fits one, with
  a radius that holds still as the camera turns. A second splits the view into slices.
- `SceneUniforms` holds a matrix and a far distance per cascade. `cel.frag` picks a fragment's
  cascade by its view depth, and blends across a seam.
- The shadow pass draws each cascade into its layer with its own matrix.
- One cascade reproduces today's single map, so a caller of `fitShadowToView` keeps working.
- [Lighting.md](../api/rendering/Lighting.md) states the new layout for a game that replaces the
  lit shaders.
- Tests: slices cover the view without gaps, and keep their radius under rotation and their snap
  under motion. A device case draws a long view with three cascades with a silent validation log.
  Pass timings record the cost of each cascade.

### Step 4 — An upload that returns at once

**Open.** ADR first. An upload submits and returns, and the staging buffer is retired when the
frame ring says the copy is done. The consumer's barrier is checked under synchronization
validation.

- `DeviceBuffer`, `TextureFactory` and `MeshRegistry::poll` use it. `RenderTarget`'s layout
  transition may keep the waiting form, and the step says why if it does.
- Tests: an upload's data is right in the next frame, against the spec-exact textured quad
  picture. The staging buffer outlives the copy. The device suite runs clean with
  `VK_LAYER_VALIDATE_SYNC=1`.

### Step 5 — A texture request

**Open.** ADR first. `Textures::request(path)` returns a handle at once. A worker decodes the
image and `poll()` uploads it under the same budget as meshes. Until then the handle draws as the
decision says.

- Tests: a requested texture draws its placeholder, then its pixels after a poll. Releasing a
  pending request abandons it. A missing file logs and leaves the placeholder.

### Step 6 — The upload budget is measured

**Open.** A device test, run in Release, streams a region-sized set of meshes and textures and
records the frame time of each `poll()` through `Engine::measure`. The default is set so that one
poll stays inside a stated share of a 60 Hz frame on the development machine, and the number and
the machine are written beside the constant.

### Step 7 — Remembered sight in `api/grid`

**Open.** `odyssey/tile/Sight` moves to `api/grid` as `v3d::grid::Sight`, with its range given to
the constructor. Its test moves to the grid suite. odyssey uses the library's.
[Grid.md](../api/Grid.md) describes it.

### Step 8 — Routes and sight across regions

**Open.** ADR first, which amends ADR-0087. It also decides how a region that is not loaded is
treated: as a wall, or as unknown ground a route may plan through.

A second consumer wants this: cozy, a game in another repository, whose pet follows the player
across region seams. Its regions own their grids and its stream loads and drops them, so it would
hand the search references to the loaded grids, not copies. It asks for `reachableTiles` and
`DistanceField` across a seam as well as `findPath`, and it treats a tile in no loaded grid as
impassable. It rejects stitching the loaded regions into one grid, because every game with
regions would write that stitching again.

- `findPath`, `reachableTiles`, `DistanceField` and `hasLineOfSight` work over a set of grids, in
  world tiles, with the movement rules of one grid. A diagonal may not pass between two blocked
  tiles in different grids.
- A set whose grids differ in tile size, or whose corners are not whole tiles apart, is refused
  with a reason, as `TileGrid::refusal()` gives one.
- odyssey's click routes into a neighbouring region, and its sight sees across a seam.
- Tests: a route crosses a seam and is as short as one over a single grid of the same tiles.
  Sight across a seam is symmetric, as within one grid. A wall on the seam blocks both.
- Tests, on two 4 by 3 grids side by side, A at (0, 0) and B at (4, 0):

  ```
        A                 B
  y=2   . . . .           . . . .
  y=1   S . . #           # . . G
  y=0   # # # .           . . . .
  ```

  The route from S to G crosses on row 2, between the two walls that face each other. A goal at
  world (3, 0) is reached only by leaving A, going round B's wall through B's (1, 1) and coming
  back into A on row 0, which two routes joined at the seam cannot find. With A's wall moved to
  (3, 2), the diagonal from A's (3, 1) to B's (0, 2) is refused. A set of one grid gives the same
  route as `findPath` on that grid, tie for tie. A goal in no grid has no route.

### Step 9 — A voice with a position

**Open.** ADR first. `Play` gains a position, and `Engine::position(voice, ...)` moves a playing
voice. A voice is either panned or placed, and switching one way clears the other.

- `event::kind::Sound` gains a position, so a game can send a placed sound as an event.
- [Audio.md](../api/engine/Audio.md) states the axes and the rule.
- Tests: under the dummy driver, a placed voice reads back its position in mixer axes. A pan
  after a position clears it, and the reverse. Whether attenuation is audible is not testable.

### Step 10 — Packing and unpacking sprite sheets

**Open.** Pack and unpack are library functions, so they can be tested, and imagetool calls them.
imagetool gains `--pack <dir|files> --sheet <name>` and `--unpack sprites.json`.

- Pack places each image through `TextureAtlas`, trying larger sizes until all fit, and writes the
  sheet and its `sprites.json` through `SpriteSheets`.
- Unpack reads `sprites.json`, crops each region, and writes one file per sprite.
- Tests: pack then unpack returns every image unchanged, compared with `image::compare`. Sprites
  that cannot fit in the largest size are refused with a reason.

## Outcome

Written when the plan closes.
