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

Each entry states the gap in the code, not who is waiting for it. Every entry is ready to pick
up. None waits for a game, a scene or a second caller to need it first.

An open plan or roadmap may order entries from here. The entry stays here until the work lands,
and its section names the plan or roadmap that orders it.

## Offline rendering

[ScaleAndReach](plans/ScaleAndReach.md) orders these, in steps 1 and 2.

[] **The reyes hider adds every polygon to the traced scene.**

- moya fans each polygon into the traced scene as it arrives, before culling, and shades its
  lights once for it. This lets an off-screen caster cast a shadow.
- Every standard light with a direction calls `transmission()`, so only a scene with no
  shadowing light gains. Such a scene casts no ray, and the hierarchy is built on the first
  ray, so it pays only the memory the triangles hold.
- The triangles still have to be made as the scene is read, because a shader bound later may
  trace. The fix would drop them when rendering starts if nothing traced.

[] **`offline::trace::Tracer` runs a shader once per hit.**

- Each hit gets its own `HitShader` and its own run of the machine, so a ray-hidden image pays
  the machine's per-run cost once per pixel sample.
- The fix batches the hits of a scanline that share a shader into one run, as the reyes hider
  shades a grid. It is for speed alone, and gives no derivatives.
- Measure the gain with a profile of a Release render. A pixel's samples are seeded per pixel,
  so batching them in another order does not change the picture.

Background: [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration).

## Tile grids

[ScaleAndReach](plans/ScaleAndReach.md) orders these, in steps 7 and 8.

`api/grid` is a library of its own, and odyssey uses it. The grid owns a map's picture and
terrain legend; the rest of a map belongs to the game. [Grid.md](api/Grid.md) describes the
library.

[] **A route and sight stop at a region's edge.**

- A region is a grid placed by its corner, and [Grid.md](api/Grid.md#regions) states the rules.
  odyssey loads its regions as the player approaches and crosses between them by stepping.
- `findPath` and `hasLineOfSight` each take one grid, so odyssey's clicked routes and its sight
  stay within the region the player stands in. The next region is dark until the player crosses.
- Joining them needs a search over several grids, or a game that stitches one together.

[] **Remembered sight belongs in `api/grid`.**

- Fog of war that keeps seen ground revealed is in odyssey's
  [tile/Sight.h](../odyssey/tile/Sight.h), but nothing about it is specific to odyssey.
- Move it to `api/grid`, and have odyssey use it from there.
- Background: [LargeWorlds, step 8](plans/completed/LargeWorlds.md#step-8--remembered-sight).

## Sprite sheets

[ScaleAndReach](plans/ScaleAndReach.md) orders this, in step 10.

The api has every piece of a sprite sheet round trip: `image::TextureAtlas` places regions,
`config::SpriteSheets` reads and writes the document that names them, and `image::crop` cuts a
region back out of a sheet.

[] **Nothing packs a sheet, and nothing unpacks a whole sheet.**

- `imagetool --crop` cuts out one region, so a caller that already knows where the sprites are
  can split a sheet one rectangle at a time.
- Nothing reads a `sprites.json` and cuts out every region it names.
- Undecided: whether this belongs in `imagetool` or in a separate `spritetool`.

## Lit scenes

[ScaleAndReach](plans/ScaleAndReach.md) orders this, in step 3.

[] **There are no cascaded shadow maps.**

- `lighting::fitShadowToView` fits one map to the camera's view out to a distance. Over a long
  view, one map spreads its texels thinly, and near shadows lose detail.
- Cascades split the view into slices by distance, each with its own map, and the lit shader
  picks the slice a fragment falls in.
- `lighting::fitShadow` covers each caster's position and a margin, not its extent or scale. A
  caller with large casters passes a margin big enough for them.
- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

## Loading

[ScaleAndReach](plans/ScaleAndReach.md) orders these, in steps 4 to 6.

[] **A texture has no request of its own.**

- `MeshRegistry::request` returns a mesh handle at once and fills it later. A texture handle
  cannot do the same, because `pipeline::Resources` would have to swap the image behind a live
  handle and rewrite its descriptor set.
- A worker job that decodes the image, and a completion that calls `Textures::texture(image)`,
  covers the need meanwhile.

[] **An upload still waits for its copy.**

- `memory::Uploader::oneShot` waits on its own fence, so the caller can free its staging buffer
  when it returns. Frames in flight keep running, and the main thread waits only for the copy.
- An upload that returned at once would retire its staging buffer through the frame ring.

[] **The upload budget's default is not measured.** `MeshRegistry::DEFAULT_UPLOAD_BUDGET` is
4 MiB, chosen without measuring a region's upload. Measure it against a region-sized set of
meshes and set it from that.

## Audio

[ScaleAndReach](plans/ScaleAndReach.md) orders this, in step 9.

`audio::Engine` plays a voice on a named bus, with a fade, a gain and a pan.

[] **A voice has no position.**

- A pan places a sound between the speakers. Nothing places it in a 3D scene, or makes it
  quieter with distance.
- SDL_mixer 3.2's `MIX_SetTrack3DPosition` places a track and attenuates it by distance, with
  the listener fixed at the origin. A caller would give each position relative to its listener.
- Background: [LitScenesAtScale, step 10](plans/completed/LitScenesAtScale.md#step-10--close-phase-3).

## Tests

[Testing.md](contributing/Testing.md) describes what the suites cover.

[] **Two things need a window or a sound card to test:** `Feature::Window`, and whether
`audio::Engine` is audible. SDL's dummy audio driver tests what the mixer is told, not what is
heard. A software Vulkan driver does not help with either.
`ui::TextRenderer`'s atlas upload needs a device, which the `render_device` suite could give it.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. Blending, filtered sampling, multisampling
and text are therefore checked only by a silent validation log and by spot checks. Checking them
more strongly needs a second Vulkan implementation to compare against, and this tree has none.

## Known review findings

Defects a review found that the change under review did not introduce.
[Review.md](contributing/Review.md) says how this list is used: a reviewer reads it first and
does not report an entry again. Each entry names its class.

There are none open.

## Editor

Open work on the editor app. [editor/](editor/README.md) describes the editor.
[EditorScene](plans/EditorScene.md) orders every group here.

9 of the menu's 83 commands have no handler, and only log themselves when chosen. Each group
below is a piece of work of its own, in any order.

[] **Shared meshes in a scene document.** Two mesh items may share one `BRep`, but the scene store
writes each item's geometry in full, so they read back as two copies. Keeping them shared needs
the geometry as a record of its own, which the mesh items refer to by id, and a new mesh version.

[] **Curve and handle select masks** (`select::mask::curve`, `select::mask::handle`). There is no
curve kind of item, and nothing defines what a handle is. EditorScene steps 13 and 14 build them.

[] **Importers** (`project::import::*`) each need a format reader. An importer creates meshes
through the command stack, so an import can be undone.

[] **A timeline and a graph editor.** Keys can be set and stepped through, and the current frame
shows only in the caption. Nothing draws the keys on a time line, scrubs the frame, sets the rate
or the range, or edits a key's interpolation or slopes, so every key is an automatic Bezier one.

[] **Rotation order.** Every item turns about x, then y, then z, so an item that locks in that
order cannot be keyed out of it. Each item needs an order of its own, saved and offered to
whatever keys it.

[] **The Render menu** (`render::view`, `render::batch`, `render::settings`) would hand the scene to
moya. The RIB export already writes it. `render::batch` renders a range of frames, posing the
scene at each.


