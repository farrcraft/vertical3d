# Streaming Worlds

What the realtime api needs for a game whose world is loaded and released piece by piece while
it is played. A model can be loaded on a worker thread and uploaded while frames are in flight.
A tile grid is still one rectangle at the world origin, and a shadow map covers one fixed sphere.
Each of those holds for a world that is one board. Neither holds for a world of regions.

The gaps are each described in [TODO.md](../../TODO.md). This roadmap orders them, because most of
them block the next.

**Steps 1 to 6 are done by [the StreamingWorlds plan](../../plans/completed/StreamingWorlds.md)**,
closed 2026-10-16. Step 7 is held in [TODO.md](../../TODO.md#tile-grids) behind its trigger, and
what the plan left open is there too. What follows is the reasoning the plan was drafted from.

## What exists

- **[`asset::Worker`](../../../api/asset/Worker.h)** decodes on one thread and hands results back
  in a poll on the main thread. One `asset::Manager` loads correctly from several threads.
- **[`MeshRegistry::request`](../../../api/render/realtime/MeshRegistry.h)** decodes a model on a
  worker, and `poll()` uploads it under a byte budget. A texture has no request of its own; a
  worker job decodes it and its completion registers it.
- **[`memory::Uploader`](../../../api/render/realtime/vulkan/memory/Uploader.h) waits for its own
  upload**, not for frames in flight. Every object a frame may still read is retired through
  [`frame::Ring::retire`](../../../api/render/realtime/vulkan/frame/Ring.h), voxel's replaced chunk
  meshes among them.
- **[`TileGrid`](../../../api/grid/TileGrid.h)** is one rectangle of tiles centred on the world
  origin. Every conversion between world and tile coordinates goes through `worldMin()`, which
  assumes that origin.
- **`TileFilter` is a `std::function`**, called for every neighbour of every visited tile.
- **Remembered sight** is in odyssey's [tile/Sight.h](../../../odyssey/tile/Sight.h). Nothing in it
  is specific to odyssey.
- **[`shadow::fit`](../../../api/render/realtime/Shadow.h)** fits the shadow map once, to a sphere
  around the casters' positions plus a caller's margin.

## What it needs

### 1. Decoding separate from uploading

A texture named by a glTF file is decoded when the model is loaded, not when it is uploaded.
`MeshRegistry` then receives decoded images and only uploads them. This adds no thread. It is a
change of order inside one thread, and it can be tested headless.

### 2. Loading on a worker thread

- A worker thread decodes through its own `asset::Manager`.
- The main thread polls once a frame. It uploads what is ready through
  `TextureFactory::create(image)` and `MeshRegistry::add(name, model, albedos)`.
- A load that is requested and then cancelled before it finishes is dropped on the main thread.
- This is the first thread in `api/`. It needs an ADR before code: what owns the thread, how a
  result is handed back, and what an app may call from the worker.

### 3. Uploads that do not wait for the queue

`memory::Uploader` stops waiting for idle once loads arrive while frames are in flight. Before that
change lands, every caller that relies on the wait moves to `frame::Ring::retire`. voxel's old
chunk meshes are the known case. Steps 2 and 3 can land together, but step 3 must not land before
the voxel change.

### 4. Regions

- A region is a piece of the world that is loaded and released with its sprite sheets.
- Each region needs an offset in the world. The most likely shape is a world origin on
  `TileGrid`. A grid of grids and a game's own list of grids are the alternatives.
- The choice decides how every caller places a grid, so it needs an ADR.
- A region is loaded through step 2, so loading a region does not stall a frame.

### 5. The movement filter as a template parameter

`TileFilter` becomes a template parameter of the search functions. A region-sized board is the
first case where the cost of a `std::function` call per neighbour is likely to show. Measure before
changing it.

### 6. A shadow map that follows the camera

- `shadow::fit` fits to the part of the world the camera sees, refitted each frame, rather than to
  every caster once.
- The fit reads the drawn position between steps and each caster's extent, not only its stepped
  position.
- Cascaded shadow maps follow when one map over the visible area is too coarse.

### 7. Remembered sight in `api/grid`

Move [tile/Sight.h](../../../odyssey/tile/Sight.h) into `api/grid` when a second game needs fog of war.
With regions, remembered sight is kept per region and released with it.

## The ordering

```
1 Decode apart from upload ──> 2 Worker thread ──> 3 Uploads stop waiting
                                     │                 (voxel retires first)
                                     ▼
                               4 Regions ──> 5 Movement filter
                                     │
                                     ├─────> 7 Remembered sight in api/grid
                                     ▼
6 Shadow map follows camera   (no order against 1–5; due with the first world
                               larger than one view)
```

## What this needs decided

- **Threads in `api/`**: step 2, an ADR.
- **How a region is placed in the world**: step 4, an ADR.

## Verification

- Steps 1, 4, 5 and 7 are headless and are tested like the rest of `api/asset` and `api/grid`.
- Step 2's hand-off between threads is tested headless, with a manager that decodes from memory.
- Step 3 is checked by the device suite and by a run of voxel with the validation layer on. A run
  that remeshes chunks and logs no validation message is the signal.
- Step 6 can be checked only by a screenshot and a silent validation log. A shadow is filtered
  output, which a reference image cannot hold.

## Triggers

The roadmap is due when a game streams regions, or when any load is long enough for a player to
see. Step 6 is due on its own when a world is larger than one view.
