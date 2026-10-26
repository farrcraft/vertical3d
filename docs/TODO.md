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

[ScaleAndReach](plans/ScaleAndReach.md) orders this, in step 2.

[] **`offline::trace::Tracer` runs a shader once per hit.**

- Each hit gets its own `HitShader` and its own run of the machine, so a ray-hidden image pays
  the machine's per-run cost once per pixel sample.
- The fix batches the hits of a scanline that share a shader into one run, as the reyes hider
  shades a grid. It is for speed alone, and gives no derivatives.
- Measure the gain with a profile of a Release render. A pixel's samples are seeded per pixel,
  so batching them in another order does not change the picture.

Background: [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration).

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

[] **Leak: a started FMOD event leaves 72 bytes behind.** In a build with `V3D_FMOD_ROOT`,
`fmod_events_test/the_fmod_backend_stops_an_event_it_started` run alone ends with the debug
heap reporting one 72 byte block. The other FMOD cases, each run alone, report none. The block
is allocated on the CRT heap, not FMOD's, so it is something `FmodEvents` or the case allocates
around `start()` and `stop()`.

## Editor

Open work on the editor app. [editor/](editor/README.md) describes the editor.
[EditorScene](plans/EditorScene.md) orders every group here.

6 of the menu's 83 commands have no handler, and only log themselves when chosen. Each group
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

[] **A render shows nothing until it finishes.** moya resolves its samples into pixels at the end
of a frame, so the render window waits for the whole picture. Resolving each bucket as it
finishes would let `Delegate` hand over pictures as they improve.

[] **A live render starts again from nothing.** An edit restarts the whole render, because moya
reads a scene once. Taking edits into a running render needs moya to keep its scene and update
it, which the editor's interface already sends.

[] **A batch renders one frame at a time.** Its frames are independent files, and could run as
several moya.exe processes at once.


