# The Rendering Pipeline

What `api/render/realtime` does, as of 2026-09-06. Open questions are at the end.

The decisions behind its shape are [ADR-0001](adr/0001-vulkan-replaces-opengl.md) through
[ADR-0005](adr/0005-one-batched-quad-primitive.md), plus
[ADR-0008](adr/0008-binding-by-update-frequency.md),
[ADR-0009](adr/0009-colour-authored-in-display-space.md),
[ADR-0010](adr/0010-meshes-are-owned-by-the-app.md),
[ADR-0011](adr/0011-lines-are-the-second-primitive.md),
[ADR-0031](adr/0031-a-pass-draws-into-a-target-it-names.md) and
[ADR-0042](adr/0042-a-textured-quad-in-world-space.md). Those say why; this says what.

## The chain of objects

```
Window    ->  Context3D  ->  Frame  ->  Pass  ->  DrawItem
                  |
                  +-- vulkan::Swapchain   the images presented to the window
                  +-- vulkan::Presenter   acquire, submit, present, and the sync between them
                  |
              DeviceContext, the base - everything that needs only a device
                  +-- vulkan::Device      the gpu, its queues, and the 1.3 features
                  +-- vulkan::frame::Ring the frames in flight, and their buffers and fences
                  +-- vulkan::pipeline::Cache
                  +-- vulkan::Resources   pipelines, materials and textures, addressed by handle
                  +-- vulkan::FrameUniforms  set 0 - a camera per pass per frame in flight
                  +-- vulkan::Uploader    the one-shot queue everything device local is copied by
                  +-- vulkan::DepthBuffer the depth image, allocated the first frame a pass asks
                  +-- vulkan::renderer::Quad the 2D pipelines, and the geometry buffers they upload through
```

`realtime::Window` creates an `SDL_WINDOW_VULKAN` window and owns the `vulkan::Instance` and
`vulkan::Surface`. There is one window class, not a 2D one and a 3D one: every app has
presented through a swapchain since odyssey's port on 2026-09-01, and a 2D game differs only
in what its passes ask for, an orthographic projection and no depth. Everything that belongs
to the device is `DeviceContext`, and `Context3D` is that plus the window's chain and presenter
([ADR-0051](adr/0051-the-in-flight-ring-is-not-the-swapchain.md)). A context is told what it
draws into rather than asking a chain for it, which is what lets one exist with no window under
it at all. `Engine3D` drives one frame per tick.

There is no `VkRenderPass` and no `VkFramebuffer` anywhere. Passes draw through dynamic
rendering, straight into the swapchain image views, per
[ADR-0002](adr/0002-target-vulkan-1-3.md).

## A frame

An app builds a frame during its tick. The engine records and presents it in one step at the
end. `Engine3D::frame()` is the frame being built; `Engine3D::renderFrame()` draws it and
empties it for the next one.

```
frame->pass("colour")->submit(item);   // during the tick, as many times as it likes
engine.renderFrame();                  // once, at the end
```

A `Frame` is a list of `Pass`es in the order they were recorded. `Engine3D` creates one, the
`colour` pass, which draws straight to the window. Everything else is more passes rather than
a different kind of frame ([ADR-0003](adr/0003-one-realtime-engine.md)): the editor builds
one pass per viewport of the same scene, and an offscreen target is a pass placed in front of
the colour pass with `Frame::passBefore`.

A pass carries what varies between 2D and 3D drawing: whether it clears and to what, whether
it depth tests, what region of the target it draws into, the camera it draws through, and
whether its items are sorted.

A `DrawItem` describes one draw rather than performing it
([ADR-0004](adr/0004-operations-as-draw-data.md)). It names its pipeline and material by
handle and carries a `SortKey`. The engine owns sorting, merging and recording.

**Sorting is per pass and off by default.** `Pass::ordered()` hands the recorder either the
submission order or the sort key order; `Pass::sort(true)` asks for the second. The default
is submission order because 2D content is painter ordered: the key groups by pipeline and
material within a layer, so sorting a canvas of batches would put a panel over the text drawn
on it. Sorting is for a depth tested scene pass, where there is one item per object and
grouping lets the recorder skip binds. The sort is stable, so items with equal keys keep the
order they arrived in.

Either way, the recorder skips rebinding what is already bound. A pipeline, a descriptor set
and a vertex buffer are bound only when an item asks for a different one than the last item
did, so a run of quads sharing a texture costs one bind between them.

## Depth

`DeviceContext` owns one depth image, sized by what the context draws into and rebuilt with
it. It
is **allocated the first frame a pass asks for depth**, and not at all otherwise, so pong and
tetris pay nothing for it.

A pass with `depth(true)` gets it as a `pDepthAttachment` on its `VkRenderingInfo`. Depth is
cleared exactly when colour is, so a pass drawing on top of what the pass before it left
keeps that pass's depth too. The image is transitioned to `DEPTH_ATTACHMENT_OPTIMAL` once per
frame, from `UNDEFINED`: nothing carries depth between frames, so preserving the last frame's
contents is not worth a barrier, and the first pass to use it must clear.

### A depth image that is read as well as written

A `RenderTarget` built with `sampledDepth` allocates its depth image with sampled usage and a
sampler, and the recorder leaves it in `DEPTH_READ_ONLY_OPTIMAL` after the last pass that wrote
it — which is a shadow map, and is
[ADR-0044](adr/0044-a-sampled-depth-target-is-read-only.md). `renderer::Quad::depthTexture()`
registers it, the same borrowed-rather-than-owned way a target's colour is registered.

**Asking for it changes the format.** A format the device will draw depth into is not
necessarily one it will let a shader read, so `DepthBuffer::chooseFormat(device, true)` walks
a shorter list, and a pipeline drawing into a sampled target has to be built against that
target's `depthFormat()`. The swapchain's depth buffer is unsampled and unchanged.

Nothing in this tree draws into a target at all, so this half is exercised only by a consumer
outside it.

Dynamic rendering matches a pipeline to the attachments of the pass it draws into, so a
pipeline built with no depth format cannot draw into a pass that has one. `renderer::Quad`
therefore compiles its pipeline twice, once each way, and picks between them from
`Pass::depth()`. Neither variant tests or writes depth: a ui drawn over a scene has to stay
on top of it whatever the scene left in the buffer.

## Building a pipeline

`vulkan::pipeline::Builder` describes a graphics pipeline one chained call at a time. Its
defaults are what every pipeline in this engine has agreed on: a dynamic viewport and scissor
so a resize costs no rebuild, one sample, one colour attachment, no culling, alpha blending,
and dynamic rendering rather than a render pass. How many colour attachments there are is a
property of the pass, so `colourFormats()` takes 0..N of them and an empty list is a pipeline
that writes depth and no colour - a shadow pass. `colourFormat()` is the one-attachment
spelling and is what everything here uses. Shader modules belong to the builder and are
destroyed with it; the pipeline and its layout are handed back for `Resources` to own.

Two knobs exist for consumers rather than for this tree, both defaulting to what it already
did. `blend()` also takes a `Builder::Blend` of four factors, whose defaults are the straight
alpha it has always applied — a pass compositing into something composited later names a
destination alpha of `ZERO`, where the default erodes the source's. And `depthBias(true)` sets
`depthBiasEnable` and puts `VK_DYNAMIC_STATE_DEPTH_BIAS` in the dynamic list, so the constant
and the slope are a scene's numbers set with `vkCmdSetDepthBias` rather than a pipeline's:
that is what a shadow pass needs to separate its own geometry from the surface tested against
it. Nothing here draws with either.

**Which is why the builder reports what it will build.** `rasterization()`, `colourBlend()` and
`dynamics()` return the three pieces of state a caller cannot otherwise see, and `build()`
assembles the pipeline out of those same three calls, so what is read is what is compiled. They
exist because nothing about a compiled `VkPipeline` says what it was built from, and a wrong
answer in any of them is a picture rather than an error: a depth bias left out of the dynamic
list compiles and validates in silence, then silently uses the zero in the create info, so
`vkCmdSetDepthBias` does nothing and a shadow does not shift. A compile cannot catch that and
neither can the validation layer, so the state is asserted directly.

```
pipeline::Builder(device)
    .name("quad")
    .shader(VK_SHADER_STAGE_VERTEX_BIT, code, sizeof(code))
    .vertexBinding(0, sizeof(Vertex))
    .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position))
    .set(uniforms->layout())     // set 0 first - they are numbered in the order they are added
    .set(materialLayout)
    .push(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4))
    .colourFormat(swapchain->format())
    .build(cache);
```

## Memory

Every buffer and image gets its memory from `memory::Allocator`, which the `Device` owns and
builds once the logical device exists. It finds memory one of two ways
([ADR-0053](adr/0053-a-consumer-chooses-how-memory-is-found.md)): `Kind::Direct` is one device
allocation per resource and is what everything here uses, and `Kind::Suballocated` hands out
regions of larger blocks through the Vulkan Memory Allocator, which is what an application with
per-frame resources needs — `maxMemoryAllocationCount` is a real limit. A consumer names the
kind when it constructs its `Device`.

A resource therefore holds an `Allocation` rather than a `VkDeviceMemory`: a suballocated region
starts part way into its block and several share one, so mapping and freeing go through the
allocator that made it. **`Allocator::bind()` allocates and binds in one call** — a resource
creates itself, hands the handle over, and never sees a memory type.

## Buffers

There are two. Which one to use follows from how often the contents change.

- **`vulkan::Buffer`** is host visible and stays mapped for its whole life. Use it for a frame
  of geometry: a batcher rewrites the whole buffer every frame, so a staging copy would cost
  more than the slower reads do.
- **`vulkan::DeviceBuffer`** is device local and filled through a staging copy. Geometry built
  once and drawn for the life of the process pays for the copy at load time, then is read
  from the memory closest to the device every frame after.

Both go through `vulkan::Uploader`, which records, submits and waits for one command buffer.
Because it waits, a staging allocation never outlives the function that made it, and the
uploader is for load time work rather than for anything overlapping the frame loop.

`vulkan::Mesh` is the pair of device local buffers a draw reads: vertices, and indices where
the draw is indexed. `Mesh::describe(&item)` fills in a draw item's geometry fields and leaves
the caller to say what it draws with and where it sorts.

## 2D drawing: the batched quad

Every 2D thing in the engine — a rectangle, a sprite, a glyph — is one quad with a texture,
per [ADR-0005](adr/0005-one-batched-quad-primitive.md). Lines are the other primitive and are
described below. The quad is split across the cpu/gpu line:

- **`realtime::Canvas`** accumulates the quads. It holds a vertex stream of position, uv and
  colour, an index stream, and the batches those are cut into; it cuts a batch where the bound
  texture, the text flag or the clip rectangle changes. It has a modelview stack of translates
  and scales that applies as vertices are added, and it produces the pixels-to-clip-space
  projection the pipeline is pushed. None of it touches vulkan, so the batching has unit
  tests.
- **`vulkan::renderer::Quad`** owns the one pipeline, the descriptor pool and layouts, the 1x1
  white texture an untextured quad is drawn against, and a vertex and index buffer per frame
  in flight. `submit(canvas, pass)` uploads the canvas into the buffers belonging to the frame
  about to be recorded, and turns each batch into a `DrawItem`.

**A clip is batch state and the device scissors the draw**, per
[ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md). `Canvas::clip` pushes a
rectangle, in the coordinates being drawn in and intersected with whatever is already clipped;
the batch carries it, `renderer::Quad` puts it on the `DrawItem`, and the recorder sets a dynamic
scissor per item and puts the pass's own region back for an item that names none. Nothing is
clipped on the cpu, so a quad straddling the edge is drawn whole and half of it lands.

`LineCanvas` clips on different terms. It cuts its stream into batches the same way, but the
rectangle is in the pixels of the image drawn into and the modelview does not apply to it: a
line canvas is world space, so there is no transform there that a screen rectangle could go
through.

The buffers are per frame in flight because the device may still be reading the previous
frame's geometry. `submit` calls `Presenter::waitFrame()` before writing. That is the same
fence `acquire` waits on, so it costs the frame nothing it was not going to pay.

Text goes through the same path. A `v3d::font` text buffer lays glyphs out into positions,
atlas coordinates and colours, and `Canvas::text` copies those into the stream against the
atlas texture. A single channel atlas is given an image view that swizzles its one channel
into alpha and ones into rgb, so the glyph samples as white with coverage and the shader
needs no branch for text.

The ui draws through the same canvas rather than a pass of its own.
`v3d::ui::paint::ComponentRenderer` adds its panels and highlights as quads and asks the app to write
its labels, so a game and its menu cost one upload and a draw per texture.

**Drawing the ui is also what lays it out**, per
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md) and
[ADR-0034](adr/0034-a-component-has-children-and-a-box.md). A component holds other
components; `Component::layout()` says where it sits in the one holding it, as a length per
axis that is either pixels, a percentage of the parent or `Auto`; and the walk that draws a
container resolves each box against the box around it and leaves the component holding the
absolute result in `position()` and `size()`. That result is what `Container::pick` tests a
cursor against, so nothing is clickable until it has been drawn, and a component answers the
cursor only when it is `pickable()`. A `VerticalBox` or a `HorizontalBox` writes its
children's boxes itself rather than resolving them, because their order along the line is
what a flow list is for. A `SelectList` shows as many rows as its box has room for and a
`TabBar` walks only the page its chosen tab holds, so what is not on screen is neither drawn
nor laid out - and a component that was not laid out cannot be picked, which is ADR-0019 read
the other way round. `Panel`, `Bar` and `Scrollbar` round their corners with `Canvas::arc`,
which is the same triangle fan `circle` is built from and so stays inside the one batched
primitive of [ADR-0005](adr/0005-one-batched-quad-primitive.md). A component cuts what it holds
off at its own box when it asks to, with `Component::clip(true)`; a `Scrollbar` is the
arithmetic of how far something is scrolled and leaves the input to whoever picked it.

**There is a second way to write a ui, onto the same canvas**, per
[ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md). `v3d::ui::Immediate`
takes the same `Measure` and `Write` callbacks and is driven by calls rather than by a tree:
a window, a tab strip, a table, a button and a scrubbable int between `begin()` and `end()`,
each placed where a layout pen has got to and hit tested against the box it was just drawn
in. It is the shape a tool wants, because a panel written that way is a function of the state
it reads and cannot show something stale; a hud is the other shape and stays retained. Which
widget the cursor is on is settled at `end()` and used by the next frame, which is what lets
a window drawn later take the cursor from one under it. A window cuts what it holds off at its
own edges and scrolls it on the wheel; how tall the content is is measured as it is drawn, so
the bar appears on the frame after the one that overflowed.

## Line drawing

The second primitive, per [ADR-0011](adr/0011-lines-are-the-second-primitive.md). The editor's
construction grid, axis decoration, wireframe display, selected-edge highlight and
manipulators are all made of it. It splits across the cpu/gpu line the same way:

- **`realtime::LineCanvas`** accumulates segments — `line`, `polyline`, `box` and `circle` over
  a modelview stack that applies as vertices are added. There is no index stream, and the only
  thing that cuts a batch is a clip changing, since there is no texture: an uncut canvas is one
  batch and one draw.
- **`vulkan::renderer::Line`** owns two pipelines and a vertex buffer per frame in flight.
  `submit(canvas, pass)` uploads and adds one `DrawItem` per batch.

Two things differ from the quad. Positions are in **world space**, and the transform is the
camera the pass carries at set 0 rather than a projection in a push constant — lines are the
first thing in the engine to read set 0, and the quad pipeline is meant to follow. The two
pipelines also differ in behaviour, not only in attachment format: the one built for a pass
with depth **tests and writes** it, so geometry in front of a wireframe occludes it, while the
quad's depth variant does neither. Lines drawn over a scene rather than into it go in a pass
without depth. That is the pass model choosing, not a flag on the renderer.

Lines are one pixel wide. `wideLines` is an optional device feature and the device does not
ask for it. The renderer is built on the first call to `DeviceContext::lines()`, the way the depth
buffer is, so an app that draws no lines pays nothing for it.

## World space quads

The third primitive, per [ADR-0042](adr/0042-a-textured-quad-in-world-space.md): a textured
rectangle with four world corners, for a sprite standing on a ground plane and for a filled
tile highlight.

- **`realtime::WorldCanvas`** accumulates quads over a modelview stack of `glm::mat4`, which
  applies as vertices are added. A quad takes its four corners in perimeter order — the order
  `grid::tileCorners` hands them out in — and is fanned from the first, so any convex quad
  comes out whole. The stream cuts where the bound texture changes and nowhere else.
- **`vulkan::renderer::World`** owns two pipelines and a pair of buffers per frame in flight, and
  takes its textures and its set 1 descriptors from the `renderer::Quad` so that an atlas
  uploaded once serves both primitives out of one descriptor pool.

Positions are in world space through the pass camera at set 0, as lines are. **The order is
the caller's**: quads are drawn in the order they were added, because what a quad's depth means
is the game's — in an isometric projection a sprite is behind another when its feet are further
up the ground plane, not when it is further from the camera. The depth variant therefore
**tests without writing**, which is the third of the three answers the engine now has: lines
test and write, ui quads do neither, world quads test only. So solid geometry hides a world
quad and a world quad never hides another.

There is no clip and no text branch. The renderer is built on the first call to
`DeviceContext::worldQuads()`, the way the line renderer is. So is the quad renderer, since a
context is built before it has been told the format its pipelines compile against.

## Shaders

The engine's shaders live in `api/render/shaders`, are compiled to SPIR-V by `glslc` at build
time, and are embedded in the library. They are not data files: they belong to the engine
rather than to any app, and CMake does not copy per-app data into the build tree, so a shader
sitting on disk beside an executable would go stale silently. `v3d_add_shader` does the
compiling, and `glslc -mfmt=c` writes the module out as a C initialiser list that the source
includes into a `uint32_t` array. See [Build.md](Build.md#shaders).

## Colour

The swapchain is a `UNORM` format rather than an `_SRGB` one, so the colour a shader writes is
the colour that appears — see [ADR-0009](adr/0009-colour-authored-in-display-space.md). Every
colour in the tree is authored in display space, and textures are uploaded as `UNORM` to
match.

**A consumer that writes linear light names its own format**, per
[ADR-0049](adr/0049-a-consumer-chooses-the-swapchain-format.md). `Swapchain`, `Context3D` and
`Engine3D` take a preferred format, defaulting to none and therefore to the rule above; a
format the surface does not offer in a non-linear sRGB colour space falls back to it, with a
warning, so silence means the preference was met. An app on the engine shell names one where
it constructs its `Engine3D`, which is the whole of what it has to do. Nothing in this tree
passes one. **Build a pipeline against `Swapchain::format()` rather than against the
default** — that was always the contract under dynamic rendering, and it is now the only way
to be right.

## What renderFrame does

1. `Presenter::acquire` waits on the frame's fence, takes the next swapchain image, and
   begins that frame's command buffer.
2. `Recorder::record` transitions the image to `COLOR_ATTACHMENT_OPTIMAL`, walks the passes —
   `vkCmdBeginRendering`, viewport and scissor, the items and the scissor any of them asks
   for, `vkCmdEndRendering` — then
   transitions the image to `PRESENT_SRC_KHR`. Both transitions are synchronization2 barriers.
3. `Presenter::present` ends the buffer, submits it with `vkQueueSubmit2`, and presents.

Two frames are in flight. What they are is a `vulkan::frame::Ring`, which needs a device and
nothing else ([ADR-0051](adr/0051-the-in-flight-ring-is-not-the-swapchain.md)): a command
buffer and a fence per frame, `frame()` to say which slot is being recorded, and `waitFrame()`
for anything else keeping a resource per frame in flight. Every renderer is built on the ring
rather than on the presenter, because sizing and indexing a geometry ring is pacing rather than
presenting.

What stays on the `Presenter` is what needs the chain: an image-available semaphore per frame,
and a render-finished semaphore per swapchain image rather than per frame, because presentation
waits on it and presentation is tied to the image.

The seam between them is the fence. The ring creates it and waits on it; the submit in
`present()` is what signals it. `acquire()` waits the ring's fence *before* acquiring rather
than leaving it to `Ring::begin()`, because the image-available semaphore is per frame and this
slot's may still be pending from its last turn — and it is `begin()` that unsignals the fence,
so a chain found out of date in between leaves the ring exactly as it was found.

### Reading a frame back

`vulkan::frame::Capture` copies a drawn image into a host visible buffer and writes it as a
png. It is two calls because the submit sits between them — `record()` into the frame's own
command buffer, `write()` once whatever the caller synchronises with says that submit has
completed ([ADR-0050](adr/0050-a-frame-is-read-back-in-two-calls.md)).

`record()` takes a `Capture::Source` — an image, its extent, its format and the layout it is
in — so the same call reads a presented frame or an offscreen `RenderTarget`. The swapchain
overload fills one in: a chain image is captured between step 2 and step 3 above, where it is
in `PRESENT_SRC_KHR` and still acquired, which is the only point it may legally be read. A
target is captured in whatever layout the recorder left it, which for one a later pass samples
is `SHADER_READ_ONLY_OPTIMAL`. The image is handed back in the layout it arrived in either way.

The barrier either side of the copy is not the same for both. Returning a chain image to
`PRESENT_SRC_KHR` needs nothing made visible, because the semaphore presentation waits on is
what orders it; returning a target to a layout something in the same submit may sample or draw
into has no such semaphore, so that transition has to be complete and visible before any of
them. A target's colour image carries `TRANSFER_SRC` usage so that it can be copied out at all.

Nothing in this tree calls `Capture`; [ADR-0007](adr/0007-ci-rendering-tests.md) still asserts
on validation errors rather than on pixels.

### Resize and minimize

The swapchain is rebuilt when presenting or acquiring reports it out of date. That is the only
reliable trigger: a resize event can arrive before or after the driver notices, and on some
drivers it does not arrive at all. `VK_SUBOPTIMAL_KHR` counts — the frame that saw it is still
drawn and presented, and the chain is rebuilt before the next one.

A window with no area has no swapchain at all. `Swapchain::create` leaves the chain empty
rather than failing, `Presenter::acquire` answers `Skip`, and `Engine3D` tries again once the
window has an area. That is what makes minimizing survivable.

## Binding by update frequency

Everything downstream depends on this being fixed, so it is fixed here:

| Set | Frequency | Holds |
|---|---|---|
| set 0 | per frame, bound once by the pass | camera, projection, viewport |
| set 1 | per material | the sampled texture and whatever else the material needs |
| push constants | per object | transform and tint |

A draw item names a material, and the material owns its set 1. Anything that changes per
object goes in push constants rather than in a third set. That keeps the number of descriptor
sets bound per draw at two, and makes merging adjacent items a matter of comparing two
handles.

The sort key is ordered to match: layer, then pipeline, then material, then depth. Sorting on
it groups exactly the draws that can share a binding.

**Set 0 is built.** `vulkan::FrameUniforms` owns the layout — one uniform buffer at binding 0,
visible to both the vertex and the fragment stage — and a slot per pass per frame in flight,
each a small buffer with a descriptor set that points at it permanently. `Recorder` writes a
pass's view, projection, their product and viewport rectangle into the next slot, then binds
it at 0 for every item in that pass. A slot is written during recording, after the presenter
has waited on that frame's fence, so nothing is reading what is overwritten.

Every pipeline in the engine declares that same layout at set 0, which makes them
interchangeable within a pass: a set bound for one stays bound across a pipeline change to
another built against the same layout. The quad pipeline declares it and reads nothing from
it, since a canvas carries its own orthographic projection in a push constant. Voxel's terrain
pipeline is the first that does read it, and reads nothing else per draw: one camera at set 0,
one block palette at set 1, and the chunk's origin in a 16 byte push constant. The line
pipelines read it and declare nothing else at all — no set 1 and no push constant — and are
still compatible for set 0 with the quad and terrain pipelines, because compatibility runs
from set 0 upwards.

## Offscreen targets

**A pass draws into the swapchain image unless it names a `vulkan::RenderTarget`**, per
[ADR-0031](adr/0031-a-pass-draws-into-a-target-it-names.md). A target is a colour image,
optionally a depth image, a view and a sampler. It is created with sampled usage and sized in
pixels rather than by the window.

`Recorder` scans the pass list and moves a target into the attachment layout before the first
pass that writes it, then into `SHADER_READ_ONLY_OPTIMAL` after the last. A target therefore
costs one pair of barriers however many passes draw into it, and every later pass can read it.
Register it with `renderer::Quad::texture(target)` to get a texture handle a canvas can
composite. Ordering the passes is the caller's job: a pass that reads a target it also draws
into, or one a later pass writes, reads whatever happens to be there.

Three consequences bite:

- **A target is single-buffered and keeps nothing between frames.** The recorder transitions it
  from `UNDEFINED` each frame. Wanting the previous frame's contents means two targets and
  swapping them.
- **`Engine3D` creates the colour pass in its constructor**, so an app's offscreen pass has to
  go in front of it with `Frame::passBefore`. Otherwise it is recorded after the pass that
  samples it.
- **What `Resources` is given for a target names its images rather than owning them**
  (`Texture::owned` is false), and `recreate()` allocates new ones, so a handle registered
  before a resize is stale.

A pipeline under dynamic rendering is built against the format of what it draws into, and
nothing catches a pipeline built for one colour format drawing into a target of another. It
produces a wrong picture rather than a validation error. A target's colour format defaults to
the swapchain's to avoid that.

## Resource handles

`DrawItem` refers to pipelines, materials and textures by handle, never by pointer. A pointer
sorts by whatever the allocator handed out, which reorders a frame differently on every run.
`vulkan::Resources` owns them, hands out the handles, and destroys everything when the context
goes. Slots are never reused, so a handle cannot come to mean something other than what it was
given for.

Nothing frees an individual resource. Textures and pipelines are built at load time and used
until the app closes; per-level unloading is what will ask for more.

**Geometry is not one of them.** A mesh is created and destroyed while the app runs, which a
registry that never frees cannot hold without leaking, and the sort key has no geometry field
to sort a handle on. So `vulkan::Mesh` is owned by whatever built it — a chunk, a model —
`DrawItem` keeps raw `VkBuffer`s, and a draw item is valid only while its mesh is alive. See
[ADR-0010](adr/0010-meshes-are-owned-by-the-app.md).

## What is not built yet

- **Merging.** Sorting groups the draws that could be merged, and nothing merges them.
  Adjacent items sharing a pipeline and a material still cost a draw call each.
- **A depth target that can be read.** `RenderTarget` allocates a depth image and a pass writes
  it, but the image has no sampled usage and no view a descriptor set can bind, so a shadow map
  is written and cannot be sampled.
- **A second depth buffer.** There is one per context, and the editor's four viewports share
  it. That works only because their regions do not overlap and each pass clears its own. Two
  passes wanting different depth over the same pixels would not work.
- **Culling.** Nothing is culled against the frustum. Voxel submits an item per meshed chunk
  whether or not the chunk is in front of the camera; its chunk-local vertices and per-chunk
  origin are there to make culling possible later.
- **A 2D pass does not use set 0.** `Canvas::projection()` builds an orthographic matrix by
  hand and the quad pipeline reads it from a push constant, while `vulkan::FrameUniforms` holds
  a camera per pass that only voxel's terrain pipeline reads. The 2D path would stop being a
  special case if a pass carried an orthographic camera and the quad pipeline read it from set
  0 like everything else.

## Still open: how this meets the ECS

`Scene::collect()` returning a frame is the shape the 2D engine was written to, and it fits
the pass model: a scene collects its renderables into passes. What is not settled is what a
renderable component looks like. `Renderable` as a marker says nothing about how to draw the
entity, and the candidates — Sprite, Canvas, TextureFont — are the old operation classes
wearing component names, which is the design
[ADR-0004](adr/0004-operations-as-draw-data.md) moved away from. The likely answer is a
component naming a material and a mesh or quad, with a rendering system turning those into
draw items, but nothing has been built to prove it. See [ECSDesign.md](ECSDesign.md).
