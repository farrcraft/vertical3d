# The Rendering Pipeline

Describes what `api/render/realtime` actually does, as of 2026-09-01. It was a page of open
questions until the Vulkan frame loop landed; the questions that are still open are at the
bottom, and the rest is now a description rather than a proposal.

The decisions behind the shape of this are [ADR-0001](adr/0001-vulkan-replaces-opengl.md)
through [ADR-0005](adr/0005-one-batched-quad-primitive.md), plus
[ADR-0008](adr/0008-binding-by-update-frequency.md),
[ADR-0009](adr/0009-colour-authored-in-display-space.md),
[ADR-0010](adr/0010-meshes-are-owned-by-the-app.md) and
[ADR-0011](adr/0011-lines-are-the-second-primitive.md). Read those for why; this is what.

## The chain of objects

```
Window    ->  Context3D  ->  Frame  ->  Pass  ->  DrawItem
                  |
                  +-- vulkan::Device      the gpu, its queues, and the 1.3 features
                  +-- vulkan::Swapchain   the images presented to the window
                  +-- vulkan::Presenter   acquire, submit, present, and the sync between them
                  +-- vulkan::PipelineCache
                  +-- vulkan::Resources   pipelines, materials and textures, addressed by handle
                  +-- vulkan::FrameUniforms  set 0 - a camera per pass per frame in flight
                  +-- vulkan::Uploader    the one-shot queue everything device local is copied by
                  +-- vulkan::DepthBuffer the depth image, allocated the first frame a pass asks
                  +-- vulkan::QuadRenderer the 2D pipelines, and the geometry buffers they upload through
```

`realtime::Window` creates an `SDL_WINDOW_VULKAN` window and owns the `vulkan::Instance`
and `vulkan::Surface`. There is one window class rather than a 2D and a 3D one: since
odyssey's port on 2026-09-01 every app presents through a swapchain, and a 2D game differs
from a 3D one in what its passes ask for - an orthographic projection and no depth - not in
the window underneath them. `Context3D` is built from a created window and owns everything
that belongs to the device. `Engine3D` drives a frame per tick.

There is no `VkRenderPass` and no `VkFramebuffer` anywhere. Passes draw through dynamic
rendering, straight into the swapchain image views, per
[ADR-0002](adr/0002-target-vulkan-1-3.md).

## A frame

An app builds a frame during its tick and the engine records and presents it in one step at
the end. `Engine3D::frame()` is the frame being built; `Engine3D::renderFrame()` draws it and
empties it ready for the next one.

```
frame->pass("colour")->submit(item);   // during the tick, as many times as it likes
engine.renderFrame();                  // once, at the end
```

A `Frame` is a list of `Pass`es, in the order they are recorded. There is one pass so far -
the engine's `colour` pass, drawing straight to the window - but the list is a list from the
first version because compositing, offscreen targets and an editor's four viewports of one
scene are all more passes rather than a different kind of frame
([ADR-0003](adr/0003-one-realtime-engine.md)). A pass carries what varies between 2D and 3D
drawing: whether it clears and to what, whether it depth tests, what region of the target it
draws into, the camera it draws through, and whether its items are sorted.

A `DrawItem` is a description of one draw, not something that draws itself
([ADR-0004](adr/0004-operations-as-draw-data.md)). It names its pipeline and material by
handle and carries a `SortKey`. The engine owns sorting, merging and recording.

**Sorting is per pass and off by default.** `Pass::ordered()` hands the recorder either the
submission order or the sort key order, and `Pass::sort(true)` is what asks for the second.
The default has to be submission order: 2D content is painter ordered, and the key groups by
pipeline and material within a layer, so sorting a canvas of batches would put a panel over
the text drawn on it. A depth tested scene pass is what sorting is for - one item per object,
grouped so the recorder can skip the binds between them. The sort is stable, so items whose
keys are equal keep the order they arrived in.

What the recorder does either way is skip rebinding what is already bound. A pipeline, a
descriptor set and a vertex buffer are bound only when the item asks for a different one than
the last item did, so a run of quads sharing a texture costs one bind between them.

## Depth

`Context3D` owns one depth image beside the swapchain, sized with it and rebuilt with it. It
is **allocated the first frame a pass asks for depth** and never at all otherwise, so pong and
tetris - painter ordered, reading no depth - pay nothing for it.

A pass with `depth(true)` gets it as a `pDepthAttachment` on its `VkRenderingInfo`. Depth is
cleared exactly when colour is, so a pass drawing on top of what the pass before it left keeps
that depth too. The image is transitioned to `DEPTH_ATTACHMENT_OPTIMAL` once per frame, from
`UNDEFINED` - nothing carries depth between frames, so what the last one left is not worth a
barrier to preserve, and the first pass to use it therefore has to clear.

Dynamic rendering matches a pipeline to the attachments of the pass it draws into: a pipeline
built with no depth format cannot draw into a pass that has one. That is why `QuadRenderer`
compiles its pipeline twice, once each way, and picks between them from `Pass::depth()`.
Neither tests or writes depth - a ui drawn over a scene has to stay on top of it whatever the
scene left in the buffer.

## Building a pipeline

`vulkan::PipelineBuilder` describes a graphics pipeline a chained call at a time. What it
defaults is what every pipeline in this engine has agreed on: a dynamic viewport and scissor
so a resize costs no rebuild, one sample, one colour attachment, no culling, alpha blending,
and dynamic rendering rather than a render pass. Shader modules belong to the builder and are
destroyed with it; the pipeline and its layout are handed back for `Resources` to own.

```
PipelineBuilder(device)
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

## Buffers

There are two, and which one a caller wants follows from how often the contents change.

- **`vulkan::Buffer`** is host visible and kept mapped for its whole life. This is what a
  frame of geometry is built in: a batcher rewrites the whole thing every frame, so a staging
  copy would cost more than the slower reads do.
- **`vulkan::DeviceBuffer`** is device local and filled through a staging copy. Geometry built
  once and drawn for the life of the process pays for that copy at load time and is read out
  of the memory closest to the device every frame after.

Both go through `vulkan::Uploader`, which records, submits and waits for one command buffer -
and the waiting is what keeps a staging allocation to the function that made it. It waits, so
it is for load time work rather than for anything overlapping the frame loop.

`vulkan::Mesh` is the pair of device local buffers a draw reads: vertices, and indices where
the draw is indexed. `Mesh::describe(&item)` fills in a draw item of geometry fields and
leaves what it draws with, and where it sorts, to the caller.

## 2D drawing: the batched quad

Every 2D thing in the engine - a rectangle, a sprite, a glyph - is one quad with a texture,
per [ADR-0005](adr/0005-one-batched-quad-primitive.md). Lines are the other primitive and are
described below. The quad is split across the cpu/gpu line:

- **`realtime::Canvas`** accumulates the quads. It holds a vertex stream of position, uv and
  colour, an index stream, and the batches those are cut into - and it cuts a batch only
  where the bound texture changes. It has a modelview stack that applies as vertices are
  added, and it produces the pixels-to-clip-space projection the pipeline is pushed. None of
  it touches vulkan, which is why the batching has unit tests.
- **`vulkan::QuadRenderer`** owns the one pipeline, the descriptor pool and layouts, the 1x1
  white texture an untextured quad is drawn against, and a vertex and index buffer per frame
  in flight. `submit(canvas, pass)` uploads the canvas into the buffers belonging to the
  frame about to be recorded, and turns each batch into a `DrawItem`.

The buffers are per frame in flight because the device may still be reading the previous
frame's geometry. `submit` calls `Presenter::waitFrame()` before writing, which is the same
fence `acquire` waits on, so it costs the frame nothing it was not going to pay.

Text goes through the same path. A `v3d::font` text buffer lays glyphs out into positions,
atlas coordinates and colours; `Canvas::text` copies those into the stream against the atlas
texture. A single channel atlas is given an image view that swizzles its one channel into
alpha and ones into rgb, so the glyph samples as white-with-coverage and the shader needs no
branch for text.

The ui draws through the same canvas rather than a pass of its own -
`v3d::ui::ComponentRenderer` adds its panels and highlights as quads and asks the app to
write its labels, so a game and its menu are one upload and a draw per texture.

## Line drawing

The second primitive, per [ADR-0011](adr/0011-lines-are-the-second-primitive.md). It is what
the editor's construction grid, axis decoration, wireframe display, selected-edge highlight
and manipulators are all made of, and it is split across the cpu/gpu line the same way:

- **`realtime::LineCanvas`** accumulates segments - `line`, `polyline`, `box` and `circle`
  over a modelview stack that applies as vertices are added. There is no index stream and no
  batching, because there is no texture to cut a batch on: a whole canvas is one draw.
- **`vulkan::LineRenderer`** owns two pipelines and a vertex buffer per frame in flight.
  `submit(canvas, pass)` uploads and adds one `DrawItem`.

Two things differ from the quad. Positions are in **world space**, and the transform is the
camera the pass carries at set 0 rather than a projection in a push constant - lines are the
first thing in the engine to read set 0 and the quad pipeline is meant to follow. And the two
pipelines differ in behaviour as well as in attachment format: the one built for a pass with
depth **tests and writes** it, so a wireframe is occluded by the geometry in front of it,
while the quad's depth variant does neither. Lines drawn over a scene rather than into it go
in a pass without depth, which is the pass model choosing rather than a flag on the renderer.

Lines are one pixel wide; `wideLines` is an optional device feature and the device does not
ask for it. The renderer is built on the first call to `Context3D::lines()`, the way the
depth buffer is, so an app that draws no lines pays nothing for it.

## Shaders

The engine's shaders live in `api/render/shaders`, are compiled to SPIR-V by `glslc` at build
time, and are embedded in the library. They are not data files: they belong to the engine
rather than to any app, and per-app data is not copied into the build tree by CMake, so a
shader on disk beside an executable is a shader that goes stale silently. `v3d_add_shader` in
the root CMakeLists does the compiling; `glslc -mfmt=c` writes the module out as a C
initialiser list that the source includes into a `uint32_t` array.

## Colour

The swapchain is a `UNORM` format, not an `_SRGB` one, so a colour a shader writes is the
colour that appears - see [ADR-0009](adr/0009-colour-authored-in-display-space.md). Every
colour in the tree is authored in display space, and textures are uploaded as `UNORM` to
match. This is the decision a lit 3D scene will have to revisit.

## What renderFrame does

1. `Presenter::acquire` waits on the frame's fence, takes the next swapchain image, and
   begins that frame's command buffer.
2. `Recorder::record` transitions the image to `COLOR_ATTACHMENT_OPTIMAL`, walks the passes -
   `vkCmdBeginRendering`, viewport and scissor, the items, `vkCmdEndRendering` - then
   transitions the image to `PRESENT_SRC_KHR`. Both transitions are synchronization2
   barriers.
3. `Presenter::present` ends the buffer, submits it with `vkQueueSubmit2`, and presents.

Two frames are in flight. Each owns a command buffer, an image-available semaphore and a
fence; the render-finished semaphore is per swapchain image rather than per frame, because
presentation waits on it and presentation is tied to the image, not to the frame.

### Resize and minimize

The swapchain is rebuilt when presenting or acquiring says it is out of date, which is the
only reliable trigger - a resize event can arrive before or after the driver notices, and on
some drivers it does not arrive at all. `VK_SUBOPTIMAL_KHR` counts: the frame that saw it is
still drawn and presented, and the chain is rebuilt before the next one.

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
object goes in push constants rather than in a third set, which keeps the number of
descriptor sets bound per draw at two and makes merging adjacent items a matter of comparing
two handles.

The sort key is ordered to match: layer, then pipeline, then material, then depth. Sorting on
it groups exactly the draws that can share a binding.

**Set 0 is built.** `vulkan::FrameUniforms` owns the layout - one uniform buffer at binding 0,
visible to both the vertex and the fragment stage - and a slot per pass per frame in flight,
each a small buffer and the descriptor set that points at it for good. `Recorder` writes the
view, the projection, their product and the viewport rectangle of a pass into the next slot
and binds it at 0 for every item in that pass. A slot is written during recording, which is
after the presenter has waited on the fence of the frame, so nothing is reading what is
overwritten.

Every pipeline in the engine declares that same layout at set 0, which is what makes them
interchangeable within a pass: a set bound for one stays bound across a pipeline change to
another built against the same layout. The quad pipeline declares it and reads nothing from
it - a canvas carries its own orthographic projection in a push constant. Voxel's terrain
pipeline is the first that does read it, and reads nothing else per draw: one camera at set
0, one block palette at set 1, and the chunk's origin in a 16 byte push constant. The line
pipelines read it and declare nothing else at all - no set 1 and no push constant - which
still leaves them compatible for set 0 with the quad and terrain pipelines, since
compatibility runs from set 0 upwards.

## Resource handles

`DrawItem` refers to pipelines, materials and textures by handle, never by pointer - a
pointer sorts by whatever the allocator handed out, which reorders a frame differently on
every run. `vulkan::Resources` owns them, hands out the handles, and destroys everything when
the context goes. Slots are never reused, so a handle cannot come to mean something other
than what it was given for.

Nothing frees an individual resource. Textures and pipelines are built at load time and used
until the app closes; per-level unloading is the thing that will ask for more.

**Geometry is not one of them.** A mesh is created and destroyed while the app runs, which a
registry that never frees cannot hold without leaking, and the sort key has no geometry field
for a handle to sort on. So `vulkan::Mesh` is owned by whatever built it - a chunk, a model -
`DrawItem` keeps raw `VkBuffer`s, and a draw item is valid only while its mesh is alive. See
[ADR-0010](adr/0010-meshes-are-owned-by-the-app.md).

## What is not built yet

- **Merging.** Sorting groups the draws that could be merged into one, and nothing merges
  them. Adjacent items sharing a pipeline and a material still cost a draw call each.
- **Offscreen targets**, and with them compositing and logical presentation.
- **A second depth buffer.** There is one per context, so two passes wanting different depth
  at the same time - which four editor viewports may - would share it.
- **Culling.** Nothing is culled against the frustum. Voxel submits an item per meshed chunk
  whether or not the chunk is in front of the camera, which is what its chunk-local vertices
  and per-chunk origin were put in place to make possible.
- **A 2D pass does not use set 0.** `Canvas::projection()` builds an orthographic matrix by
  hand and the quad pipeline reads it from a push constant, while `vulkan::FrameUniforms`
  holds a camera per pass that only voxel's terrain pipeline reads. A pass carrying an
  orthographic camera, with the quad pipeline reading it from set 0 like everything else,
  is what would make the 2D path stop being a special case.

## Still open: how this meets the ECS

`Scene::collect()` returning a frame is the shape the 2D engine was written to, and it fits
the pass model - a scene collects its renderables into passes. What is not settled is what a
renderable component looks like. `Renderable` as a marker says nothing about how to draw the
entity, and the candidates - Sprite, Canvas, TextureFont - are the old operation classes
wearing component names, which is the design [ADR-0004](adr/0004-operations-as-draw-data.md)
moved away from. The likely answer is a component that names a material and a mesh or quad,
and a rendering system that turns those into draw items, but nothing has been built to prove
it. See [ECSDesign.md](ECSDesign.md).
