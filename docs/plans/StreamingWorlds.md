# Streaming Worlds — Loading While the Game Runs

Drafted 2026-10-16 from [the Streaming Worlds roadmap](../roadmap/StreamingWorlds.md). The goal is
that a game can load and release parts of its world while frames are drawn, without a frame
hitch the size of the load. It covers the roadmap's steps 1 to 6. Remembered sight and cascaded
shadow maps are held, each behind its own trigger.

Each step starts with a test that fails first, or records why none can be written. A step that
deletes or changes a [TODO.md](../TODO.md) entry does so when it lands.

## What the code shows

Reading the code for this plan corrected three things the roadmap and TODO.md assumed.

- **The loaders may be safe to share.** A `Loader` holds a logger and its type and nothing else.
  The image readers create their libpng and libjpeg state per call, and cgltf keeps none. The
  logger's sinks are spdlog's thread-safe ones. The manager's two maps are only read after
  registration. TODO.md says the loaders keep state. Step 1 settles which is true.
- **Every world and tile conversion goes through `TileGrid::worldMin()`.** A world origin changes
  one function and the grid's constructor, not every conversion. `Overlay` reads `worldMin()` and
  `tileToWorld()`, and follows without a change.
- **`shadow::fit` has no caller outside the tests.** A fit that follows the camera is a new
  function beside it, and nothing has to migrate.

Three facts the plan relies on:

- **A handle that resolves to nothing is skipped by every draw.** `Meshes.cpp` and `Poses.cpp`
  both `continue` on a null entry. A mesh handle that is not uploaded yet therefore draws nothing
  and breaks nothing.
- **`DeviceBuffer::upload` is called only from its constructors.** No caller copies into a buffer
  a frame may be reading, so an upload never overwrites data in use.
- **`memory::Uploader::oneShot` ends in `vkQueueWaitIdle` on the graphics queue.** That drains
  every frame in flight, not only the copy. voxel's
  [ChunkMeshPool](../../voxel/src/voxel/ChunkMeshPool.h) relies on the drain to destroy a
  replaced mesh at once.

## Decisions

| ADR | Decision | Step |
|---|---|---|
| **New** | Threads in `api/`: what owns a worker, how a result returns to the main thread, and what code may run on a worker | 1 |
| **New** | How a region is placed in the world: a world origin on `TileGrid`, against a grid of grids or a game's own list | 8 |

## What blocks what

```
Phase 1 — loading off the main thread
1 ADR, and the loaders' thread safety ──> 3 Worker ──> 4 Requests that complete later
2 Decode apart from upload ─────────────────────────────┘

Phase 2 — uploads that do not drain the queue
5 voxel retires its old meshes ──> 6 Uploader waits on its own fence

Phase 3 — regions
8 World origin on TileGrid ──> 9 Two regions in odyssey ──> 10 Movement filter, measured

Phase 4 — shadows
11 Fit to the view    (no order against phases 1 to 3)

7 and 12 close their phases.
```

- Phase 2 needs neither the worker nor step 4. It can land first if voxel's stall shows sooner.
- Step 9 needs step 4, so a region's sprite sheets load without a stall.
- Step 6 must not land before step 5. Without the drain, voxel destroys a mesh a frame in flight
  may still read.

## Steps

### Phase 1 — Loading off the main thread

#### Step 1 — The ADR, and whether the loaders are safe to share

**Closed.** The loaders are safe to share. `ThreadTest.cpp` in the
asset media suite loads a PNG, a JPEG and three glTF files on eight threads through one manager,
and every load matches the same load on one thread. A PNG reader that shares one buffer between
calls fails it. [Assets.md](../api/Assets.md#the-asset-manager) states the rule, and the TODO.md
entry under "Loading" is corrected. ADR-0086 records the worker's design.

- Write a test that loads the same PNG, JPEG and glTF files from one manager on several threads
  at once, and compares each result with a load on the main thread.
- If it passes, record that loaders are safe to share and a worker uses the app's manager. If
  it fails, each worker owns its own manager, registered through the same `registerLoaders`
  functions.
- Correct the TODO.md entry under "Loading" to match.
- Write the ADR. It records the thread's owner, the hand-back and what may run on a worker:
  decoding and parsing, never a Vulkan call and never an ECS registry.

MSVC has no thread sanitiser. The ADR says how a threaded test is made deterministic instead.

#### Step 2 — Decoding apart from uploading

**Closed.** `realtime::DecodedModel` and `decodeModel()` read a model and its albedos with no
device call. `MeshRegistry::add(decoded)` uploads one, and `acquire()` no longer reads files. The
headless render suite tests decoding, and the device suite's registry cases pass unchanged.
[TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md#meshes-and-the-mesh-registry)
states the split.

- Add a value type holding a decoded glTF model and the pixels of every albedo it names. Add a
  free function that builds one from an `asset::Manager` and a path. The function makes no
  device call.
- `MeshRegistry::load(path)` becomes that function followed by an upload of its result.
- `MeshRegistry::acquire` stops loading images. It receives pixels or nothing.
- An albedo that is already registered under its key is decoded and then dropped. That waste is
  accepted, because the worker in step 4 cannot read the registry's albedos.
- Test headless: the function resolves a named texture beside the file, keeps an embedded one,
  and reports a missing one as no pixels.

#### Step 3 — The worker

**Closed.** `asset::Worker` runs jobs on one thread in posting order, and hands each result or
exception to a completion that runs in `poll()`. The asset suite tests the order, the thread each
side runs on, exceptions both ways, a completion that posts again, and destruction.
[Assets.md](../api/Assets.md#loading-on-a-worker-thread) states the rules.

- A worker runs jobs on one thread and holds each result until the main thread polls.
- `poll()` runs on the main thread. It hands back each finished result and runs its completion
  there.
- Destroying the worker finishes or abandons the running job, then joins the thread. It never
  detaches.
- A job that throws hands back its exception, and `poll()` rethrows it into the completion.
- Where the worker lives follows the ADR. `api/asset` is the likely place, because it has no
  device dependency.
- Test with jobs that block on a latch, so the order of events is fixed.

#### Step 4 — Requests that complete later

**Closed for meshes.** `MeshRegistry::request(path)` returns a handle at once, which resolves to
nothing until `poll(budget)` uploads it. A release before the upload drops the result, a
`load()` of the path completes it at once, and a failed file is logged and abandoned. The
default budget is 4 MiB, which is not yet measured against a real region. The device suite tests
each case, including a registry destroyed before its requests finish.
[TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md#requesting-a-mesh) states the
rules.

A texture request is held. A texture handle that resolves before its image exists would need
`pipeline::Resources` to swap the image behind a live handle and rewrite its descriptor set. A
worker job that decodes the image, and a completion that registers it, already covers the need.
It is due when a game wants a texture handle before its pixels arrive.

- `MeshRegistry` gains a request for a path. It returns a handle at once. The handle resolves
  to nothing until the model is uploaded, so the draws skip it in the meantime.
- The registry's poll uploads finished results. It uploads no more than a byte budget per call,
  so a large region spreads its uploads over several frames.
- Releasing a handle before its load finishes drops the result when it arrives.
- A second request for a path already in flight returns the same handle.
- The device suite tests requests, because each one ends in an upload.

### Phase 2 — Uploads that do not drain the queue

#### Step 5 — voxel retires its old meshes

**Closed.** `ChunkMeshPool` takes a retire function and hands it each replaced mesh. voxel's
renderer passes one that retires through the context's ring. The voxel suite tests the hand-off
without a device, and a run of voxel with validation on logs nothing. The TODO.md entry is
deleted.

#### Step 6 — The uploader waits on its own fence

**Closed.** `Uploader::oneShot` submits with a fence of its own and waits on it, so frames in
flight keep running. An audit of every device object destroyed outside the ring found nothing
that relied on the drain: each is destroyed after an explicit device wait, or before any frame
used it. The comments in `Uploader.h`, `DeviceBuffer.h` and `TextureFactory.h`, and
[Memory.md](../internals/realtime/Memory.md), state the new rule. The suites pass, and pong,
tetris, odyssey, voxel and the editor each run with validation on and log nothing.

- `Uploader::oneShot` submits with a fence and waits on that fence, not on the queue. Frames in
  flight keep running.
- Check every caller of `Uploader`, every `Mesh`, `DeviceBuffer` and `TextureFactory` user, and
  every place that destroys a device object, for the drain they relied on. The barrier each copy
  already records orders it against later work on the same queue.
- Update the comments in `Uploader.h`, `DeviceBuffer.h` and `TextureFactory.h` that say an upload
  happens at load time, off the frame loop.
- Verify by running voxel and pong with the validation layer on, and by the device suite in CI.

An upload that returns before its copy runs, with its staging buffer retired through the ring, is
held. It is due when a profile shows the fence wait itself.

#### Step 7 — Close phases 1 and 2

**Closed.** The TODO.md entry "Every load runs on the main thread" is deleted.
[Assets.md](../api/Assets.md) states the worker,
[TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md) the request, the poll and the
budget, and [Memory.md](../internals/realtime/Memory.md) the uploader's wait. The roadmap
describes the tree as it now is.

### Phase 3 — Regions

#### Step 8 — A world origin on TileGrid

**Closed.** ADR-0087 places a region as a grid by its minimum corner. `TileGrid` takes the
corner as an optional constructor argument and returns it from `worldMin()`, so every conversion
follows. The grid suite tests a placed grid, two grids agreeing on their seam, round trips at a
negative corner, and the refusal of a tile size or corner that is not finite. The tile size check
fixes the known review finding beside it. [Grid.md](../api/Grid.md#regions) states the rules.

- `TileGrid` takes an optional world position for its minimum corner. Without it, the grid is
  centred on the origin as it is now, so no existing caller changes.
- `worldMin()` returns the stored corner. Every conversion follows from it.
- Test that two grids placed side by side agree on their shared edge. A point on the boundary
  belongs to the higher tile in both. Test round trips at negative origins.
- Update [Grid.md](../api/Grid.md).

#### Step 9 — Two regions in odyssey

**Open.** Needs steps 4 and 8.

- odyssey's map becomes two regions, each with its own map picture, grid and textures.
- A region is requested when the player approaches it, and released when the player is far
  from it.
- Movement and sight work within a region. Crossing to another region is a step from the edge
  tile of one grid onto the edge tile of the next.
- Delete the TODO.md entry "A world cannot be made of regions".

A path that crosses regions is not in this plan. It is due when a game's mover plans a route past
its own region.

#### Step 10 — The movement filter, measured

**Closed.** On a 256 by 256 board with walls, in an optimised build, an always-true filter adds
about 3% to `findPath` and 3 to 6% to `reachableTiles`. That is not worth a template parameter
on every search, so `TileFilter` stays a `std::function`. [Grid.md](../api/Grid.md) states the
cost, and the TODO.md entry is deleted.

- Time `findPath` and `reachableTiles` on a 256 by 256 grid, with a filter and without one.
- If the filter's call is a measurable share of the time, make `TileFilter` a template parameter.
  If it is not, state the measurement in [Grid.md](../api/Grid.md).
- Either way, delete the TODO.md entry "`TileFilter` is a `std::function`".

### Phase 4 — Shadows

#### Step 11 — A shadow map fitted to the view

**Closed.** `shadow::fitView` fits a sphere to the camera's view volume out to a distance, with a
radius fixed by the volume's shape and a centre snapped to the map's texels. `shadow::fit` reads
each caster where it is drawn, through an `alpha` that defaults to the current step. The render
suite tests the enclosure, the cut at the distance, the radius under a moving camera, a world
point keeping its place within its texel, and the refusals. Removing the snap fails the texel
case. No app in the tree draws a shadowed scene, so no screenshot was taken.
[Lighting.md](../api/rendering/Lighting.md) states the rules, and the TODO.md entry keeps only
cascaded shadow maps.

- Add a fit to the camera's view volume, out to a shadow distance the caller gives. It encloses
  the whole volume rather than the ground within it, and un-projects the camera's matrix rather
  than reading `type::Frustum`, which does not give its corners.
- Snap the fitted centre to the shadow map's texel grid. Without the snap, shadow edges shimmer
  as the camera moves.
- The existing fit reads each caster's drawn position, interpolated between steps, rather than its
  stepped one.
- Test the fit's sphere headless, as `ShadowTest` tests the existing one. Check the picture by
  running a lit scene with the validation layer on and taking a screenshot.
- Update the TODO.md entry under "Lit scenes": it keeps only cascaded shadow maps.

#### Step 12 — Close the plan

**Open.** Write the Outcome section. Move what was held to TODO.md: remembered sight in
`api/grid`, cascaded shadow maps, a path across regions and uploads that return before the copy
runs. Move the roadmap and this plan to their `completed/` directories.

## Open questions

- **Is odyssey the right place to show regions?** It is the only game in the tree with a grid, and
  a demonstration in the tree is the only way this tree verifies a feature. Recommendation: yes,
  with two regions only.
- **Does a caster's extent come from its model?** `type::Model` keeps no bound. Step 11 either adds
  one at upload, or leaves the extent to the caller's margin as now. Recommendation: leave it to
  the margin until a game's casters vary widely in size.
- **The upload budget's default.** Step 4 picks one by measuring a region's upload in voxel or
  odyssey, and lets the app change it.
