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

**Done.** The emitter sets `Program::traces` when a program's instructions call `trace` or
`transmission`. A shader's own functions are inlined, so a call through one counts with no call
graph to walk. `RenderContext` notes each surface, displacement, light and imager shader as it is
bound, and `render()` calls the new `trace::Scene::clear()` under the reyes hider when none of
them traces. A shader bound but never used by a primitive still keeps the scene, which errs
towards keeping it. An area light's geometry was already its own copy.

Two existing cases read the traced scene after `WorldEnd` under reyes with no shadowing light:
the specification cube and the displaced shadow. Each now has a `distantlight`, so its subject is
unchanged. The reference pictures are unchanged.

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

**Done.** [ADR-0108](../adr/0108-memory-an-upload-is-its-own-submission-freed-by-the-ring.md)
chose a submission of its own per upload, on the graphics queue, over copies recorded into the
frame and over a transfer queue. `Uploader::submit(record, keep)` replaces `oneShot` and does not
wait. The Uploader takes the ring, and retires the staging buffer it is handed with the command
buffer it used. Its pool is shared with those retirements, so an Uploader can go before its
uploads have run.

- `DeviceBuffer` and `TextureFactory` hand over their staging buffers, and `MeshRegistry::poll`
  uploads through them. `RenderTarget`'s layout transition uses the same path, with an Uploader
  of its own, so there is no waiting form left.
- Each upload in a poll is its own submission. Step 6 measures the poll as it is.
- The textured quad picture covers an upload drawn in the next frame. New device cases check
  that the staging buffer is held until its frame has finished, that an Uploader can go first,
  and that a buffer upload reads back whole in the next frame. With the buffer's barrier taken
  out by hand, synchronization validation fails that case. The device suites run clean with
  `VK_LAYER_VALIDATE_SYNC=1`.

### Step 5 — A texture request

**Done.** [ADR-0109](../adr/0109-textures-one-bindless-array-a-draw-names-a-slot.md)
chose one bindless array of textures, which a draw indexes by slot, over swapping the image
behind a stable handle and over a pending material. The binding model has moved: every texture
is a slot of `pipeline::TextureArray` at set 1, quads and world quads push their slot, and a lit
object carries its albedo's. Device selection now requires the descriptor indexing features.
Full-screen passes keep their own sets. Every reference picture is unchanged.

`Textures::request(path)` returns a handle at once. `requestFrom(assets, worker, logger)` tells
it where to read and what to decode on, since the context that owns it has neither. The handle
names the white texture's image and reads its slot until `poll()` uploads the image, takes a
slot for it, and puts it behind the handle with the new `Resources::replace()`. A request
released before its image arrives is dropped, and a missing file is logged and draws white. The
budget constant moved to `Textures::DEFAULT_UPLOAD_BUDGET`, which `MeshRegistry` shares, so
step 6 sets one number.

- Device cases draw a requested texture white and then blue after a poll, and cover a path
  asked for twice, a request released early, a missing file and a request with no worker.

- Tests: a requested texture draws its placeholder, then its pixels after a poll. Releasing a
  pending request abandons it. A missing file logs and leaves the placeholder.

### Step 6 — The upload budget is measured

**Done.** `upload_budget_test`, a device case run by hand in Release, streams a region of 24
textures of 512 by 512 and 48 meshes of 64 by 64 vertices, and times each poll with
`engine::Statistics`. It is disabled, because its numbers belong to the machine.

- At the old 4 MiB, on direct allocation, a texture poll took 3 to 6 ms and a mesh poll 12 to
  26 ms against a share of 2.1 ms. Making device memory was most of it: on suballocated memory
  the same polls took a quarter as long.
- [ADR-0110](../adr/0110-memory-suballocate-by-default.md) made suballocation the default,
  amending ADR-0053, whose own revisit condition this was. Every suite and reference picture
  passes on it.
- The budget is now 1 MiB. On an AMD Radeon RX 6600 a texture poll takes 0.2 to 0.6 ms and a
  mesh poll 0.3 to 1.2 ms. The one poll that has the allocator open a new block takes up to
  4.7 ms, and the case allows one such poll of each kind. The numbers are beside the constant.

### Step 7 — Remembered sight in `api/grid`

**Done.** `v3d::grid::Sight` is in `api/grid/Sight.h`, with its range given to the constructor.
`look()` also takes the `SightBlocker` that `hasLineOfSight()` takes, so a game's smoke reaches
it. The test moved to the grid suite, building its rows with `setCover()` instead of odyssey's
map reader, and gained a case for the blocker and for a second range. odyssey's range is
`World::SIGHT_RANGE`. [Grid.md](../api/Grid.md#remembered-sight) describes it.

### Step 8 — Routes and sight across regions

**Done.** [ADR-0111](../adr/0111-grid-a-search-may-span-a-set-of-placed-grids.md) chose a search
over a set of placed grids, amending ADR-0087, over stitching the loaded grids into one and over
searches through callbacks. A region that is not loaded is impassable and opaque.

- `GridSet` holds references to the loaded grids, maps a world tile to the grid holding it, and
  refuses a grid whose tile size differs, whose corner is off the tile lattice, or that overlaps
  one already there. `findPath`, `reachableTiles`, `DistanceField` and `hasLineOfSight` take it,
  and their one-grid forms are a search over `GridSet::local(grid)`, so there is one search. A
  `DistanceField` keeps the layout and outlives the grids. `Sight` looks at one grid of a set
  from a world tile.
- odyssey builds `World::grids()` for each search. A click routes into a loaded neighbour, and
  each region's sight is looked at across the set. Its crossing test now expects sight past the
  seam.
- The grid suite has the plan's two 4 by 3 grids: a route through row 2, the goal at (3, 0)
  reached only through the other grid, the diagonal between walls in two grids refused, a set of
  one routing tie for tie, no route to a tile in no grid, symmetric sight, and the refusals.

### Step 9 — A voice with a position

**Open.** ADR first. `Play` gains a position, and `Engine::position(voice, ...)` moves a playing
voice. A voice is either panned or placed, and switching one way clears the other.

- `event::kind::Sound` gains a position, so a game can send a placed sound as an event.
- [Audio.md](../api/engine/Audio.md) states the axes and the rule.
- Tests: under the dummy driver, a placed voice reads back its position in mixer axes. A pan
  after a position clears it, and the reverse. Whether attenuation is audible is not testable.

### Step 10 — Packing and unpacking sprite sheets

**Done.** `image::pack()` and `image::unpack()` are in `api/image/Pack.h`, and imagetool has
`--pack`, `--sheet` and `--unpack`. The two library functions work in memory, on named images and
named rectangles, and do not know the sprites document. imagetool converts between the rectangles
and `config::SpriteSheets`. That keeps `image` from depending on `config`, and a game can pack
sprites at load time without a document. The document half is three calls a test of
`SpriteSheets` already covers.

- Pack tries square sheets from 16 pixels a side, doubling, then the largest side, which
  imagetool sets at 4096. It also refuses mixed depths, a depth the atlas cannot hold, and two
  sprites of one name, which `SpriteSheet::place()` would otherwise drop silently.
- A pack reads an existing `sprites.json` first and replaces only its own sheet. An unpack writes
  `<sheet>/<sprite>.png` beside the document, and refuses an image whose size is not the size its
  sheet states.

## Outcome

Written when the plan closes.
