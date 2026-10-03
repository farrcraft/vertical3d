# Large Worlds — Releasing A Resource, Ordering World Quads, Culling, And A Map Picture

Drafted 2026-10-03 against `a551015`, and closed the same day. Nine steps across `api/render`,
`api/grid`, `voxel`, `odyssey` and the editor, taking up
[milestone 2](../../roadmap/completed/m2-LargeWorlds.md) of
[the game engine roadmap](../../roadmap/GameEngine.md). Six were done, and the three held behind
named triggers moved to [TODO.md](../../TODO.md#tile-grids).

Every consumer has drawn a single board so far, and every gap here follows from that.
[`pipeline::Resources`](../../../api/render/realtime/vulkan/pipeline/Resources.h) adds and never
removes, so cozy's debug hot reload leaks the texture it replaces, and cozy's icon handoff
already counts 127 uploads and 127 materials that are never released.
[`WorldCanvas`](../../../api/render/realtime/WorldCanvas.h) draws in submission order and leaves
the sorting to its caller. cozy does not sort today: it draws two billboards and says depth is
M6's question. Nothing in the realtime tree culls, although `type::geometry::Frustum` has been
correct for Vulkan clip space since [MotionAndQueries](MotionAndQueries.md). And the
two map formats that exist, odyssey's JSON rows and retcon's text `.map`, agree on a picture
and a terrain legend and on nothing else.

**What is due first is release.** cozy's M6 roadmap names this tree's add-only resources as
that milestone's trigger, and [milestone 4](../../roadmap/m4-LitScene.md) waits on how a texture
is released. Both games are pinned at `13a9557`, and neither has drafted the plan (cozy M6,
retcon phase 6) that would consume the rest. So the order here is what blocks something, not
what a consumer is waiting on this week.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--a-registry-slot-is-reused-and-a-stale-handle-is-refused) | A registry slot is reused, and a stale handle is refused | `api/render` | **0061** | done |
| [2](#step-2--a-release-waits-for-the-frames-in-flight) | A release waits for the frames in flight | `api/render` | 0061 | done |
| [3](#step-3--a-texture-is-released-with-its-material) | A texture is released with its material, and a target re-registered after a resize releases the old one | `api/render` | — | done |
| [4](#step-4--world-quads-in-depth-order) | World quads drawn in an order the caller keys | `api/render` | cites 0042 | done |
| [5](#step-5--voxel-culls-its-chunks) | voxel culls its chunks against the camera's frustum | `voxel` | — | done |
| [6](#step-6--a-grid-from-a-picture-and-a-legend) | A grid from a picture of glyphs and a terrain legend, and odyssey adopts it | `api/grid`, `odyssey` | **0062** | done |
| [7](#step-7--regions) | A grid placed in the world, for regions | `api/grid` | — | moved to [TODO.md](../../TODO.md#tile-grids) with its trigger |
| [8](#step-8--remembered-sight) | odyssey's remembered sight moves to `api/grid` | `api/grid`, `odyssey` | — | moved to [TODO.md](../../TODO.md#tile-grids) with its trigger |
| [9](#step-9--the-movement-filter-as-a-template) | `TileFilter` becomes a template parameter | `api/grid` | — | moved to [TODO.md](../../TODO.md#tile-grids) with its trigger |

Step 1 blocks 2, and 2 blocks 3. Steps 4, 5 and 6 depend on nothing and on each other not at
all. Step 7 would take step 6's reader as its input if it is taken up.

---

### Step 1 — A registry slot is reused, and a stale handle is refused

**This step and the next carry the decision the roadmap asks for, and they earn ADR-0061: a
resource's lifetime.**

[`Registry`](../../../api/render/realtime/Registry.h) is a `std::vector` that grows. Its class
comment and [RenderingPipeline.md](../../RenderingPipeline.md#resource-handles) both state the
invariant "slots are never reused, so a handle cannot come to refer to something other than
what it was given for". The second half of that stays true; the first half has to go.

**The recommendation is explicit release, per handle.** The roadmap names three choices:

- **A reference count** held by whatever keeps a handle. A handle is a value. It is copied into
  every `DrawItem` and packed into a `SortKey` ([ADR-0004](../../adr/0004-operations-as-draw-data.md)),
  so counting it would make each of those copies a counted one. That costs the draw path for a
  convenience that only load and unload need.
- **A scope**, where everything registered for a scene is released with it. That is what cozy's
  region and retcon's scene both want, and it is a `std::vector<TextureHandle>` the caller
  keeps and releases in a loop. Built into the registry, it is a second ownership model the
  editor and the ui never use.
- **An explicit `release(handle)`.** The mistake it allows is drawing with a handle after
  releasing it. The generation below turns that mistake into a handle that resolves to nothing,
  rather than into a use after free. That makes it the cheapest of the three to get wrong.

**The shape.** A handle carries a slot and a generation. A slot that is released is put on a
free list, and the next `add` reuses it with its generation incremented. `resolve` refuses a
handle whose generation is not the slot's current one.

```cpp
template <typename Tag>
class Handle final {
    ...
    uint32_t id() const noexcept;          // the slot, and still the sort order
    uint32_t generation() const noexcept;  // which occupant of the slot this names
};
```

`id()` keeps its meaning, so `SortKey`'s 16-bit `pipeline` and `material` fields and voxel's
`static_cast<uint16_t>(material_.id())` do not change. The generation is a second 32-bit field
rather than bits taken from the slot. A narrow generation wraps after a few hundred reuses of
one slot, and a region streamed in and out of a long session reaches that.

`Registry::resources()` currently hands the destructor a dense vector. It becomes a walk over
the live slots, because a released slot holds nothing to destroy.

**The trap.** `Registry::clear()` and the destructor of `Resources` both assume every slot is
occupied. Each of them gets a case.

**Tests**, in [`RegistryTest.cpp`](../../../api/render/tests/RegistryTest.cpp): a handle released
and then resolved gives nothing; a slot reused after a release gives the new occupant to the
new handle and nothing to the old one; a released handle released again is a no-op; and a
registry with holes still walks only its live slots.

### Step 2 — A release waits for the frames in flight

A resource that is released while a recorded frame still reads it is a use after free on the
device. The device suite under lavapipe is where that shows up as a validation error. What
knows when a frame has finished is the in-flight ring
([ADR-0051](../../adr/0051-the-in-flight-ring-is-not-the-swapchain.md)).

**The shape.** `Resources::release(handle)` removes the handle from its registry at once, so
the handle stops resolving from that moment. The Vulkan objects behind it go onto a retirement
queue, stamped with a count of frames begun that the ring keeps beside the slot index `frame()`
already returns. The queue is the ring's, and `Ring::begin()` collects whatever was stamped at
least `framesInFlight` frames ago, once its fence wait has returned. That is a change from
the draft, which had `DeviceContext` call a `Resources::collect()`: nothing in `DeviceContext`
drives a frame. `Presenter` and the device suite both call `begin()` themselves, and `begin()`
is already where the ring learns that a frame has finished.

**The queue holds destruction, not resource types.** Each entry is a callback. The queue does
not know what a `pipeline::Texture` is, so
[milestone 4](../../roadmap/m4-LitScene.md)'s image, sampler and texture classes retire into it
without a change here. That is what the roadmap means by "give them a registry that release
works in". A mesh can be retired the same way. voxel has no reason to yet: its remesh is safe
only because [`Uploader`](../../../api/render/realtime/vulkan/memory/Uploader.cxx) idles the
queue after every copy, which [`ChunkMeshPool.h`](../../../voxel/src/voxel/ChunkMeshPool.h) says
in as many words. That stops being true when
[milestone 7](../../roadmap/m7-ShellAndShipping.md#asynchronous-loading) stops idling the queue,
and a TODO entry says so when this plan closes.

**What the ADR records:** explicit release; a generation that refuses stale handles; and
destruction deferred by the ring rather than by a device wait. A `vkDeviceWaitIdle` on every
release would be correct and would hitch a region load on the main thread, which is the case
the roadmap says M6 starts with. The ADR amends [ADR-0010](../../adr/0010-meshes-are-owned-by-the-app.md)'s
premise that `Resources` "never frees". It does not supersede it, because meshes stay the
app's.

**Tests.** The queue is a class of its own, `frame::Retirement`, and is headless when the
ring's count is faked: something retired at frame
*n* is not destroyed at *n* + 1 with two frames in flight, and it is destroyed at *n* + 2. On
the device, [`ReleaseTest`](../../../api/render/tests/device/ReleaseTest.cpp) draws a texture,
releases it while that frame is still in flight, and runs three more frames. A silent
validation log is the assertion, which is how [WorldDepthTest](../../../api/render/tests/device/WorldDepthTest.cpp)
and its neighbours already work. Destroying on release instead fails it with
`VUID-vkDestroySampler-sampler-01082`.

### Step 3 — A texture is released with its material

[`Quad::material`](../../../api/render/realtime/vulkan/renderer/Quad.cxx) keeps a map from texture
slot to material, and allocates the material's descriptor set from pools that were created
without `FREE_DESCRIPTOR_SET`. Releasing a texture therefore leaves three things stale: the map
entry, which a reused slot would hit and hand back a set naming the old image; the material in
`Resources`; and the descriptor set.

**The shape.** `Quad::release(const TextureHandle&)` releases the texture, its material, and
its map entry. The set goes onto a free list once the frames in flight have finished with it,
and the next material rewrites it rather than allocating a new one. That needs no pool flag and
no change to the pool's sizing. The map is keyed by the whole handle rather than by `id()`,
so a slot that has been reused can never reach the old entry. Even if a release were missed,
a stale map entry would go unused rather than be handed out.

**A target that is resized** gets the same treatment. `Quad::texture(target)`'s comment tells
its caller to register the target again after `recreate()`, and nothing frees the first
registration. The comment becomes: release the old handle, then register again. That is two
calls now that releasing is possible, and it is the only fix the comment needs. `depthTexture`
is the same, except that a target with no sampled depth hands back `white()`, and releasing
`white()` is refused.

**In-tree consumers.** tetris, odyssey and the editor each register at load time and never
replace anything, so none of them changes. The consumer is cozy's hot reload and its region
sheets.

**The handoff note.** For cozy: a texture can be released, through
`renderer_->quads()->release(handle)`, which releases its material with it
([ADR-0061](../../adr/0061-a-resource-is-released-explicitly.md)). Calling it on the old handle
before `rebindTextures()` closes the leak `Watch.h:40` acknowledges, and `resolveTexture`'s
cache can drop an entry once it has released what the entry held. A released handle resolves to
nothing at once, so a draw that still names one is skipped rather than drawn wrong, and the
image itself is destroyed once the frames in flight have finished with it. A region's sheets are
released the same way, one call per handle, when the region unloads. Releasing the white
texture is refused, so a handle `depthTexture()` gave back can be released like any other.

**Tests.** The map half is headless only if `Quad`'s bookkeeping is separable from its device
calls, and today it is not. The device case from step 2 is extended: release a texture, register
a new one that reuses its slot, draw with the new one, and read every pixel back through
[`Capture`](../../../api/render/realtime/vulkan/frame/Capture.h). That is a flat colour at one
texel per pixel, so [ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)
allows it to be asserted exactly. The case that matters is the stale map entry: with the map
looked up by slot and the entry left behind, the case draws nothing and fails.

### Step 4 — World quads in depth order

**The decision the roadmap leaves to the plan is where the key goes, and the answer is a
helper beside the canvas, not the canvas itself.** `WorldCanvas` applies a transform stack as
quads arrive and cuts batches as it goes. Sorting inside it would mean holding every quad until
the end and then re-cutting, which is a second canvas inside the first. A helper that collects
`(key, quad)` and replays them into a canvas in key order leaves `WorldCanvas` exactly as it
is. The canvas still draws in submission order unless it is asked otherwise, and here asking
means feeding it through the helper.

**The shape.** In `api/render/realtime`, headless like the canvas:

```cpp
/**
 * World quads collected with a depth key and handed to a canvas furthest first.
 *
 * The key is the caller's, per ADR-0042: in an isometric projection it is how far up the
 * ground plane a sprite's feet are, which nothing here can know. Equal keys are drawn
 * grouped by texture and otherwise in the order they were added, so a caller that wants
 * fewer batch cuts quantises its key - to a tile row, say - and lets the tie-break merge them.
 **/
class DepthOrder final {
 public:
    void clear();
    void quad(float key, const WorldCanvas::Corners& corners, const glm::vec4& colour);
    void quad(float key, const WorldCanvas::Corners& corners, const glm::vec2& uv0,
        const glm::vec2& uv1, const glm::vec4& colour, const TextureHandle& texture);
    void into(WorldCanvas* canvas) const;  // the canvas's transform applies as usual
};
```

A larger key is drawn first, meaning further away. A stable sort on `(key, texture)` keeps
submission order among exact equals.

**The tie-break is what keeps batch cuts down.** cozy's own M6 roadmap is where the cost lands
("a sheet per object is a cut per object"). An atlas per region is the caller's half of the
answer, and a quantised key is the other half.

**No ADR.** This is [ADR-0042](../../adr/0042-a-textured-quad-in-world-space.md)'s "ordered by its
caller", given a tool. The order is still the caller's, and depth testing still never hides
one quad behind another.

**Tests**, in a new `DepthOrderTest.cpp` beside
[`WorldCanvasTest.cpp`](../../../api/render/tests/WorldCanvasTest.cpp):

- quads added nearest-first come out furthest-first in the canvas's vertices;
- equal keys with alternating textures come out as two batches, not four, which is a count the
  canvas already exposes;
- equal keys and one texture keep their submission order;
- a canvas with a transform pushed applies it to what the helper emits.

**Downstream.** cozy is the consumer, and its key is ADR-0001's ground plane: the feet's
position along the camera's ground-projected forward.

**The handoff note.** For cozy: [`realtime::DepthOrder`](../../../api/render/realtime/DepthOrder.h)
collects world quads with a key and hands them to a `WorldCanvas` largest key first. The key is
cozy's to compute, and under its ADR-0001 it is the feet's position along the camera's
ground-projected forward, larger meaning further away. Quantising the key to a tile row lets
equal keys group by texture, which is the half of the batch-cut cost an atlas per region does
not answer. Depth testing still never hides one world quad behind another, so the order is the
only thing deciding which sprite is in front.

### Step 5 — voxel culls its chunks

**What the api needs for culling is already there.** It is
`Frustum(camera.projection() * camera.view())` and `intersect(box) != OUTSIDE`, and the depth
range is the realtime default. The roadmap rules out a spatial structure, and a wrapper around
two lines would be a `cull()` whose whole content is which side of `OUTSIDE` counts.

So this step is the first consumer, which is what turns "can cull" into "culls".
[`Renderer::drawTerrain`](../../../voxel/src/Renderer.cxx) builds the frustum once per frame and
skips any chunk whose box is outside it. The box is `entry.origin` to `entry.origin +
chunkSize` in blocks, from the chunk's own constants. The statistics overlay gains "chunks
drawn / chunks meshed". That count is how a person sees the cull working, and it is the number
the roadmap asks to assert.

**Tests.** The box-from-entry arithmetic goes in a free function in voxel, tested in
[`voxel/tests`](../../../voxel/tests/) against a camera looking down one axis. One chunk is ahead,
one behind, and one straddling the near plane, which is the plane the old frustum got wrong.
The frustum's own cases are milestone 1's. Watching voxel is the remaining check: fly, turn
around, and the drawn count falls while nothing at the edge of the screen pops.

**Downstream.** For cozy the box is a region, or a chunk of one, rather than a quad, and the
handoff note repeats the roadmap's reason: a test per quad costs nearly as much as drawing it.
retcon has a shadow frustum and no view culling, and adopting this is retcon's to decide.

**The handoff note.** For cozy: culling is
`Frustum(camera.projection() * camera.view()).intersect(box) != Frustum::OUTSIDE`, from
[`api/type/geometry`](../../../api/type/geometry/Frustum.h), with the default depth range for any
camera in `api/type`. voxel's [`ChunkCulling`](../../../voxel/src/voxel/ChunkCulling.h) is the
worked example. Test a region, or a chunk of one, rather than each quad, because a test per quad
costs nearly as much as drawing it. A box crossing the frustum's edge counts as in view, so
nothing at the edge of the screen pops.

**[RenderingPipeline.md](../../RenderingPipeline.md#what-is-not-built-yet)'s culling bullet is
deleted** when this lands, because the bullet stops being true.

### Step 6 — A grid from a picture and a legend

**This step carries the second decision, and it earns ADR-0062: which part of a map is the
grid's.**

The roadmap expected this to wait on cozy's choice between a format of its own and Tiled. That
choice is still open, because cozy's M6 roadmap says "consider whether to build one or adopt
Tiled" and its status is "not started". It does not have to wait, because the shareable part
is smaller than a format.

- odyssey's [`Map`](../../../odyssey/tile/Map.h) is JSON with a `"tiles"` array of rows and glyphs
  fixed in code.
- retcon's `MapFile` is a line-oriented text file: `legend <glyph> <passable|blocked>
  <none|half|full> [prop]`, a `tiles`…`end` block, and records for spawns, a horde, items and
  objectives. Its ADR-0036 rejected Tiled for the cost of authoring the non-terrain half, and
  the format has to stay emittable with `std::format` by phase 6's generator.

The two containers have nothing in common. A shared *file* reader would therefore have to
replace one of them, and retcon has a recorded reason not to give up its own. What they share
is what happens after parsing: rows of glyphs, and a legend that maps a glyph to passability
and cover, turned into a `TileGrid`.

**The recommendation.** `api/grid` takes the picture and the legend, not a file:

```cpp
struct Terrain final {
    bool passable;
    Cover cover;
};

struct Picture final {
    boost::shared_ptr<TileGrid> grid;  // or null, with the reason in error
    std::map<char, std::vector<TileCoord>> unknown;  // glyphs the legend does not name
    std::string error;
};

// Rows northmost first, one glyph per tile, all the same length.
Picture fromPicture(const std::vector<std::string>& rows, const std::map<char, Terrain>& legend,
    float tileSize = TileGrid::DEFAULT_TILE_SIZE);
```

**A glyph the legend does not name is handed back rather than refused**, which is the
roadmap's requirement. Its tiles are left impassable with `Cover::None`, and its positions are
returned, so a game that hangs its own meaning on the glyph can place its own content there and
set the terrain itself. odyssey's `'@'` is the obvious case: a start position is odyssey's,
since retcon places its party with `spawn` records, and so a start position is **not** part of
what the grid owns. Ragged rows and an empty picture are still errors, because neither has a
tile the game could hand back.

**What the ADR records:** the grid owns a picture and a terrain legend; the container is the
game's; and props, spawns, items and start positions are the game's, keyed by glyph or by
record. A Tiled reader is a separate piece of work, and its trigger is cozy choosing Tiled. If
cozy does, a Tiled layer of tile ids is again rows plus a legend once the ids are mapped to
glyphs, so it would call this same function.

**Does this need `boost::shared_ptr`?** No. `api/grid` links glm and nothing else
([`CMakeLists.txt`](../../../api/grid/CMakeLists.txt)), so `Picture` holds a
`std::optional<TileGrid>` and odyssey moves it into the `boost::shared_ptr` it already keeps.
The unknown glyphs are a vector of `Unknown { glyph, tiles }` in order of first appearance
rather than a map, because clang-tidy holds `Picture`'s move constructor to not throwing and
MSVC's `std::map` move allocates.

**odyssey adopts it.** `Map::load` keeps its JSON, its `Kind` per tile for drawing and its `'@'`.
It builds its grid through `fromPicture` with a legend of `'.'`, `'#'` and `'o'`, and finds `'@'`
in `unknown`. [`MapTest.cxx`](../../../odyssey/tests/MapTest.cxx) passes unchanged, and that is
what shows odyssey's behaviour survived.

**Tests**, in `api/grid/tests/PictureTest.cpp`: a picture whose terrain matches the legend tile
for tile; a glyph the legend lacks handed back at every position it appears; ragged rows and an
empty picture refused with a reason; and the tile size passed through.

**Downstream.** For retcon, `MapFile`'s terrain half becomes this call once its legend is parsed.
Adopting it is retcon's decision under its ADR-0041, and a validator that refuses an unknown
glyph stays retcon's own rule, applied on top. For cozy, this is the terrain half of whichever
format M6 picks.

**The handoff note.** For retcon: [`v3d::grid::fromPicture(rows, legend, tileSize)`](../../../api/grid/Picture.h)
builds a `TileGrid` from the rows of a `tiles`…`end` block and a `std::map<char, Terrain>` made from
the `legend` records' passability and cover. A glyph the legend lacks comes back in
`Picture::unknown` with its tiles rather than as an error, so `MapFile`'s rule that an unknown
glyph is invalid stays retcon's, checked against that list. Props stay keyed by glyph in
retcon's own legend, and spawns, the horde, items and objectives stay records. Picture row *y*
is tile row *y*. Adopting it is retcon's decision under its ADR-0041
([ADR-0062](../../adr/0062-a-map-picture-and-legend-are-the-grids.md)).

For cozy: whichever format M6 picks, its terrain half is this call. A Tiled tile layer becomes
rows plus a legend once its tile ids are mapped to glyphs. Whatever a glyph means beyond
passability and cover, a prop, a spawn or a region's entry point, is cozy's to look up from the
same rows, and `Picture::unknown` lists the tiles of every glyph the legend left out.

### Step 7 — Regions

**Held, and the trigger is cozy's M6 plan.** `TileGrid` is one rectangle centred on the origin.
A world of regions wants several grids, each offset in the world and each loaded and released
with its sheet, which steps 1 to 3 make possible. Whether that is a grid with an origin, a grid
of grids, or a game's list of grids is what cozy's M6 has to answer first. The roadmap says this
tree takes what M6 learns rather than guessing ahead of it, and cozy's M6 is not started. The
likeliest answer is a world origin on `TileGrid` replacing "centred on the origin". That is a
change to every world/tile conversion, and it is cheap to make once a caller exists to say what
the origin means.

### Step 8 — Remembered sight

**Held, and the trigger is a second consumer.** [`Sight.h`](../../../odyssey/tile/Sight.h) is fog of
war and nothing about it is odyssey's. retcon's detection is cones and noise, which are its own
rules, so retcon is not that consumer. cozy might become one if its M6 hides unexplored ground.

### Step 9 — The movement filter as a template

**Held, and the trigger is a board large enough to notice.**
A region-sized board is the first one likely to be large enough. That is step 7's trigger as well, so it waits behind step 7.

---

## Sequence

**Steps 1 to 3 first, in order.** Each needs the one before. ADR-0061 is written and accepted
before step 1's code, as every ADR here is. Step 2 is the step to get right, because a release
that frees too early produces only a validation message, and only on a device. So step 2's
device case runs before step 3 builds anything on top of it.

**Step 4 after them, or alongside.** It touches neither `Resources` nor the device. It goes
second only because what it sorts is textures, and cozy will want both together.

**Step 5 whenever.** It is one app and one function, and it is the smallest step here.

**Step 6 last of the open ones.** Nothing waits on it. Its ADR is the one the roadmap flagged,
and drafting it last gives cozy's M6 the most time to have chosen a format, which would only
confirm the shape.

**Steps 7 to 9 close with the plan, moving to TODO.md with their triggers**, unless a trigger
fires first.

## Verification

Per [sdlc.md](../../sdlc.md), each step: `ninja -C out/build/x64-Debug`, `ctest`, cpplint, and the
`/W4 /WX`, `/analyze` and clang-tidy gates, run with apps on, since steps 5 and 6 touch voxel and
odyssey. The tree is clean at all of them, so every finding is the step's.

Steps 1, 4 and 6 are headless, and so is step 2's queue. Step 2's release and step 3's slot
reuse are device questions. There is one GPU here and no local lavapipe, so they are checked on
the Radeon with the validation layer locally, and on the runner's lavapipe when the branch is
opened as a pull request. Step 3's readback is the only picture this plan asserts, and it
qualifies under [ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)
because it is a flat colour. Step 5 is the one thing a person has to watch, and the step says
what to watch for.

## What this does not do

- **It does not load in the background.** A region loaded on the main thread is correct, and
  that is [milestone 7](../../roadmap/m7-ShellAndShipping.md#asynchronous-loading). Step 2's
  deferral is what keeps an unload from hitching; a load still costs what it costs.
- **It does not give the editor or the ui release.** Neither replaces a texture today. A theme
  switch that re-resolves images would be the first case, and nothing in the tree does that yet.
- **It does not add a spatial structure.** It adds a loop over boxes, per the roadmap.
- **It does not read Tiled, or any file.** Step 6 is a picture and a legend; the container stays
  the game's.
- **It does not draw a minimap.** That is [ADR-0031](../../adr/0031-a-pass-draws-into-a-target-it-names.md)
  used by a consumer, and step 3 makes it safe to resize the target that minimap draws into.

## When a step lands

Update the state in the table above, and set ADR-0061's status when step 2 lands and ADR-0062's
when step 6 does.

- **Steps 1 to 3** change [RenderingPipeline.md](../../RenderingPipeline.md#resource-handles): "slots
  are never reused" and "nothing frees an individual resource" both stop being true, and the
  section says what replaced them. The target bullet under the offscreen section changes from
  "a handle registered before a resize is stale" to the release-then-register rule.
  [Architecture.md](../../Architecture.md#invariants-that-bite) gains the invariant that a released
  handle resolves to nothing at once and its objects outlive it by the frames in flight.
  [m4-LitScene.md](../../roadmap/m4-LitScene.md) is told its texture class retires into step 2's queue.
- **Step 3** writes cozy's handoff note for hot reload and region sheets.
- **Step 4** writes cozy's handoff note for depth order.
- **Step 5** deletes RenderingPipeline.md's culling bullet.
- **Step 6** rewrites [`Map.h`](../../../odyssey/tile/Map.h)'s comment on why its format is odyssey's own,
  and writes the handoff notes for retcon and cozy.
- **When the plan closes**, steps 7 to 9 move to TODO.md with their triggers, under a tile grids
  section. A TODO entry records that voxel's remesh depends on the uploader idling the
  queue, with milestone 7 as its trigger. The roadmap's
  [m2](../../roadmap/completed/m2-LargeWorlds.md) moves to `roadmap/completed/` and points here as done, and
  this file moves to [completed/](./).
