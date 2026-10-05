# A World Larger Than the Screen

Milestone 2 of [the game engine roadmap](GameEngine.md). What a game needs once its world no
longer fits in one screen or one load: resources that can be let go, world sprites drawn in the
order the camera sees them, things off screen not drawn, and maps read from a file. Every
consumer in and out of this tree has drawn a single board so far, and every gap here follows
from that.

**Done by [LargeWorlds](../../plans/completed/LargeWorlds.md)**, closed 2026-10-03. Regions,
remembered sight and the movement filter are held in [TODO.md](../../TODO.md#tile-grids) behind
their triggers. Resource lifetime and the grid's part of a map have records of their own in
[ADR-0061](../../adr/0061-resources-explicit-release-generational-handles.md) and
[ADR-0062](../../adr/0062-grid-parse-terrain-not-map-files.md). What follows is the
reasoning the plan was drafted from, as it stood then.

cozy's M6 (world and map) is where all of it is first due, and cozy's own roadmap names this
tree's add-only resources as that milestone's trigger. Nothing here waits on another milestone
except culling, which uses [milestone 1](m1-MotionAndQueries.md)'s `Frustum`.

## What exists

* **[`pipeline::Resources`](../../../api/render/realtime/vulkan/pipeline/Resources.h) adds and never
  removes.** It hands out a handle for a pipeline, a material or a texture, and nothing frees
  one. That costs something already: cozy's debug hot reload leaks the texture it replaces on
  every reload, and [`Quad::texture(target)`](../../../api/render/realtime/vulkan/renderer/Quad.h)
  tells its caller to register a target again after it is recreated, which leaves the old
  registration naming images that no longer exist.
* **[`WorldCanvas`](../../../api/render/realtime/WorldCanvas.h) draws in submission order**, and
  says why: in an isometric projection a sprite is behind another when its feet are further up
  the ground plane, which is the caller's knowledge, and by
  [ADR-0042](../../adr/0042-rendering-world-space-sprites.md) one world quad never occludes
  another through depth. Its stream is cut wherever the bound texture changes
  ([ADR-0005](../../adr/0005-2d-one-batched-quad-pipeline.md)).
* **A pass can sort its items by key** ([Pass.h](../../../api/render/realtime/Pass.h)), by
  pipeline and material, for a depth-tested scene with an item per object. That is a different
  sort from the one above: a canvas is one stream, and the order inside it is what matters.
* **Nothing is culled**, per [RenderingPipeline.md](../../RenderingPipeline.md#what-is-not-built-yet).
  voxel submits every meshed chunk, and its chunk-local vertices were laid out to allow culling
  later.
* **[`api/grid`](../../../api/grid/)** is one rectangle of tiles on Y = 0 with world and tile
  conversion, A*, a reachable set, a distance field and line of sight
  ([ADR-0029](../../adr/0029-grid-8-way-movement-symmetric-line-of-sight.md)). Its movement filter is a
  `std::function` called for every neighbour of every visited tile, which
  is the first thing to templatise once a board is large enough to notice.
* **Two map formats exist and they are the same idea.** odyssey's
  [`Map.h`](../../../odyssey/tile/Map.h) is rows of characters, one per tile, so that the file
  looks like the board; retcon's `game/mission/MapFile` is a picture of glyphs with a legend
  saying what each glyph's terrain is and what stands on it. Both say a format is a game's until
  a second reader exists.
* **odyssey remembers what it has seen.** [`Sight.h`](../../../odyssey/tile/Sight.h) keeps two
  answers per tile — in sight now, and seen before — on top of `grid::hasLineOfSight`, and
  says the range and the memory are what it adds.

## What it needs

### Releasing a resource

A handle that can be released, and a registry whose slots can be reused without a stale handle
reaching the new occupant. Both are needed for the same reason: a region that unloads its sheet
and a region that loads one must not see each other's texture.

**The decision is lifetime, and it wants a record.** The choices are an explicit release per
handle, a reference count held by whatever keeps a handle, or a scope — everything registered
for a scene released with it — and they differ in who can get it wrong. Whatever is released
must also not be in a frame still in flight, so the free waits on the in-flight ring
([ADR-0051](../../adr/0051-frames-in-flight-ring-separate-from-presenting.md)), which is what knows when
a frame has finished with it.

[Milestone 4](m4-LitScene.md) moves textures and samplers into classes of their own, so this
milestone should give them a registry that release works in, rather than give `Resources` a
release that milestone 4 then works around.

### Ordering world quads

A way for a caller to give each world quad a depth key and have the stream drawn in key order,
so that cozy does not sort its whole world itself every frame. Sorting reorders texture changes
and therefore cuts the stream more often, which is the cost cozy's M3 plan already weighs in
asking whether its art is one sheet or several. An atlas per region keeps the cost down; a key
that sorts by depth and breaks ties by texture keeps it down further.

Whether the key goes on `WorldCanvas` or on a helper that feeds it is the plan's to decide. The
canvas staying in submission order unless asked is not, for the same reason a pass does not sort
unless asked.

### Culling

Testing a box against [milestone 1](m1-MotionAndQueries.md)'s `Frustum` before an item is submitted.
For voxel that is one box per chunk, and for retcon one per entity. For cozy it is one per region
or chunk of a region rather than per quad: the test is cheaper than a quad, but not by enough to
run thousands of them a frame.

What does not belong here is a spatial structure. Neither game has a world large enough to need
one yet, and a list of region boxes tested in a loop is the honest starting point.

### A map document

**Which part of a map is the grid's is the decision, and it wants a record.** The two existing
formats agree on a picture of glyphs and a legend, and disagree on everything else: odyssey's
glyphs are fixed in code, and retcon's legend carries props, encounters, a horde and items
beside terrain. The shareable part is the picture, the legend's terrain and cover, and a start
position — which is enough to build a `TileGrid` and nothing else. Whatever else a game hangs on
a glyph stays the game's, which means the reader has to hand back the glyphs it does not
understand rather than refuse them.

cozy is the third consumer, and its M6 roadmap is still deciding between a format of its own and
adopting Tiled. A Tiled reader is a different piece of work from the glyph picture, and it is
worth knowing which cozy chooses before this is drafted.

### Regions

`TileGrid` is one rectangle centred on the origin. A world built from regions wants several
grids, each offset in the world and each loaded and released with its sheet. Whether that is a
grid with an origin, a grid of grids, or a game's list of grids is cozy's M6 to answer first;
this tree should take what M6 learns rather than guess ahead of it.

### Remembered sight, and the movement filter

odyssey's two-state visibility is a game's fog of war and nothing about it is odyssey's. It
moves to `api/grid` once a second consumer wants it. retcon's detection is cones and noise,
which are its own rules, so retcon is not that consumer.

The filter's templatisation is due when a board is large enough to notice, and a region-sized
board is the first one likely to be.

## Verification

Release is headless at the registry: handles minted, released and reused, and a stale handle
refused. Whether a released texture is still in flight is a device question, and the device
suite under lavapipe is where a use after free shows up as a validation error.

Ordering is headless: a canvas given keys emits its vertices in key order, and the number of
texture cuts is a count that can be asserted. Culling is the frustum's tests from milestone 1
plus a count of items submitted. The map reader is the round trip every document in
`api/config` has, plus the case that matters most: a glyph the reader does not know is handed
back rather than lost.

## Not in this milestone

* **Streaming in the background.** Loading a region without a hitch needs loading off the main
  thread, which is [milestone 7](m7-ShellAndShipping.md#asynchronous-loading). A region loaded on
  the main thread is correct, and M6 can start there.
* **A minimap.** Drawing the world into a target and showing it in the ui is what
  [ADR-0031](../../adr/0031-rendering-passes-draw-into-offscreen-targets.md) already allows. It is a
  consumer's first use of it rather than a missing piece.
