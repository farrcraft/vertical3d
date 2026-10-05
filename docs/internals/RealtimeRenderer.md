# The realtime renderer, inside

This document is for someone changing `api/render/realtime`. It covers how the objects own each
other, how a frame reaches the GPU, the memory and descriptor layout, and the invariants that
are easy to break. How an app uses the renderer is in [api/rendering/](../api/rendering/README.md);
this document does not repeat it.

- [Object chain and ownership](#object-chain-and-ownership)
- [Device selection](#device-selection)
- [Frames in flight, the presenter and the swapchain](#frames-in-flight-the-presenter-and-the-swapchain)
- [Recording a frame](#recording-a-frame)
- [Layout transitions](#layout-transitions)
- [Depth buffers](#depth-buffers)
- [Building a pipeline](#building-a-pipeline)
- [Memory](#memory)
- [Buffers and streaming](#buffers-and-streaming)
- [Descriptor sets and push constants](#descriptor-sets-and-push-constants)
- [Draw items and the sort key](#draw-items-and-the-sort-key)
- [Resources and deferred destruction](#resources-and-deferred-destruction)
- [Textures](#textures)
- [The renderers](#the-renderers)
- [Shaders](#shaders)
- [Timings and statistics](#timings-and-statistics)
- [Frame capture](#frame-capture)
- [Invariants that are easy to break](#invariants-that-are-easy-to-break)

Names are in `v3d::render::realtime`; `vulkan::` is `v3d::render::realtime::vulkan`. The code is
in [api/render/realtime](../../api/render/realtime), split into `vulkan/device`, `vulkan/frame`,
`vulkan/memory`, `vulkan/pipeline` and `vulkan/renderer`.

## Object chain and ownership

```
realtime::Window           SDL_WINDOW_VULKAN window; owns vulkan::device::Instance and Surface
Engine3D                   owns the Context3D and the Frame being built
  Context3D : DeviceContext
    vulkan::frame::Swapchain   the images presented to the window
    vulkan::frame::Presenter   acquire, submit, present, and their semaphores
  DeviceContext              everything that needs only a device
    vulkan::device::Device     physical + logical device, queues, memory::Allocator
    vulkan::frame::Ring        frames in flight: a command buffer and a fence each
    vulkan::pipeline::Cache    the VkPipelineCache
    vulkan::pipeline::Resources pipelines, materials and textures, by handle
    vulkan::memory::Uploader   one-shot copies to device-local memory
    vulkan::frame::FrameUniforms set 0: one camera per pass per frame in flight
    vulkan::frame::DepthBuffer the window's depth image, made on first request
    Textures                   texture factory, set 1 layout and pool, white texture, materials
    vulkan::renderer::Quad / Line / World   built on first request
Frame -> Pass -> DrawItem
```

There is one window class for 2D and 3D. A 2D app differs only in what its passes ask for: an
orthographic projection and no depth.

**`DeviceContext` is usable without a window.** It is given a colour format and an extent at
construction, or later through the protected `describe()`. Every pipeline built through it is
compiled against that format. A headless context (as in the device tests) is a
`DeviceContext` on a `Device` built with no surface. `Context3D` cannot know its format at
construction because its swapchain does not exist yet, so it calls `describe()` after creating
the swapchain and after every rebuild. A renderer already built keeps the format it was compiled
with.

**Member order sets teardown order.** C++ destroys members in reverse declaration order:

- `DeviceContext` declares the `Ring` before everything it paces, so the ring is destroyed after
  them. A renderer's buffers may still be in flight on one of the ring's fences, and the ring's
  destructor waits for the device and runs every pending retirement.
- `Context3D` declares the `Presenter` last, so it is destroyed first. Nothing else may go while
  a frame it submitted is in flight.
- `Device` resets its `memory::Allocator` explicitly in its destructor body, before
  `vkDestroyDevice`. A suballocator frees its blocks against the device, which must still exist.

**`Engine3D::shutdown()`** waits for the ring to go idle, drops the frame, and resets the
context. The context must go before the window: the device holds the window's surface alive,
and `Window::destroy()` unloads the Vulkan library. A surface destroyed after that is never
destroyed, and the instance reports it leaked.

There is no `VkRenderPass` and no `VkFramebuffer` anywhere. Every pass draws through dynamic
rendering.

Background: [ADR-0003](../adr/0003-rendering-one-engine-for-2d-and-3d.md),
[ADR-0051](../adr/0051-frames-in-flight-ring-separate-from-presenting.md)

## Device selection

The instance declares `VK_API_VERSION_1_3`. `Device::selectPhysical()` skips a physical device
that:

- reports an API version below 1.3,
- lacks a required extension (`VK_KHR_swapchain`, only when presenting),
- lacks the 1.3 features `dynamicRendering` and `synchronization2`, or
- has no graphics family, or no present family for the surface when presenting.

Of the devices left, the first discrete GPU wins; otherwise the first acceptable device. If none
qualifies, the constructor throws naming the requirement.

Both features are requested explicitly through a `VkPhysicalDeviceVulkan13Features` chained
onto `VkPhysicalDeviceFeatures2`. A chained features struct and `pEnabledFeatures` are mutually
exclusive, so the base features travel in the chain too. `wideLines` is not requested, which is
why lines are one pixel wide.

A device given no surface is headless: it selects on the graphics family alone, enables no
swapchain extension, and has no present queue. Everything that draws works on it; `Swapchain` and
`Presenter` need a presenting device.

`Device` also records the timestamp period and the graphics family's `timestampValidBits`, for
[timings](#timings-and-statistics).

**Validation.** `Instance` enables `VK_LAYER_KHRONOS_validation` when it is installed, with a
messenger that routes messages through the logger and counts errors and warnings
(`errors()`, `warnings()`, `firstError()`). Without the messenger the layer reports nowhere,
which reads the same as a clean run. Check `validating()` alongside the counts. How the tests use
this is in [contributing/Testing.md](../contributing/Testing.md).

A `VkResult` that is not a success throws through `device::check(result, what)`, naming the call
and the result.

Background: [ADR-0001](../adr/0001-rendering-replace-opengl-with-vulkan.md),
[ADR-0002](../adr/0002-vulkan-require-version-1-3.md)

## Frames in flight, the presenter and the swapchain

Three classes split the work.

**`vulkan::frame::Ring`** needs only a device. It holds, per frame in flight (two by default):

- a command buffer,
- a fence that the frame's submit signals,
- a timestamp query pool (through `Timings`).

`frame()` is the slot being recorded. `waitFrame()` waits for that slot's last submit; anything
writing a per-frame resource before recording calls it. `begin()` waits on the fence, resets it,
begins the command buffer, reads the slot's timings, and collects retired objects. `advance()`
moves to the next slot after the submit. Every renderer and `FrameUniforms` is built on the ring,
not on the presenter, because pacing is not presenting.

**`vulkan::frame::Presenter`** adds what needs the swapchain:

- an image-available semaphore per frame in flight,
- a render-finished semaphore per swapchain image, because presentation waits on it and
  presentation is tied to the image.

**The fence is shared.** The ring creates and waits on it; the submit in `Presenter::present()`
signals it. A submit that does not signal `ring.fence()` leaves the next turn around the ring
waiting forever. `acquire()` waits on the ring's fence *before* acquiring, rather than leaving it
to `Ring::begin()`, because this slot's image-available semaphore may still be pending from its
last use. `begin()` is what resets the fence, so a chain found out of date between the wait and
`begin()` leaves the ring unchanged.

**`vulkan::frame::Swapchain`** owns the images and views.

- `chooseFormat(formats, preferred)` returns the preferred format where the surface offers it in
  a non-linear sRGB colour space; otherwise a 32-bit `UNORM` format (`B8G8R8A8` or `R8G8B8A8`);
  otherwise the first offered. A missed preference logs a warning. The function needs no device
  and has a unit test.
- The present mode is mailbox where offered, otherwise FIFO.
- `recreate()` waits for the device to go idle, then rebuilds with the same requested format.
- A window with no area gets no chain. `create` leaves it empty and `valid()` is false.
  `Presenter::acquire()` then returns `Skip`.

**Rebuilding.** The swapchain is rebuilt when acquire or present reports it out of date. That is
the only reliable trigger: a resize event can arrive before or after the driver notices, and on
some drivers not at all. `VK_SUBOPTIMAL_KHR` counts too: that frame is still drawn and presented,
and the chain is rebuilt before the next one. `Presenter` never recreates the chain itself; the
caller (`Context3D::resize()`) rebuilds it and calls `Presenter::reset()`, since a new chain can
hold a different number of images.

### What `Engine3D::renderFrame()` does

1. `Presenter::acquire()` waits on the frame's fence, acquires the next image, and begins the
   command buffer. `OutOfDate` rebuilds the chain and drops the frame. `Skip` rebuilds the chain
   if the window has an area again, and drops the frame.
2. It fills a `Recorder::Target` with the swapchain image, view, extent and format, plus the
   context's depth buffer if any pass on the swapchain tests depth.
3. `FrameUniforms::begin(ring->frame())` starts the frame's set 0 slots.
4. `Recorder::record()` records every pass.
5. `Presenter::present()` ends the buffer, submits it with `vkQueueSubmit2`, and presents.
   `OutOfDate` rebuilds the chain for the next frame.
6. `Frame::reset()` drops every pass's items.

`beginFrame()` calls `renderFrame()` itself when the window has no area, so a minimized app still
turns the loop over.

## Recording a frame

`vulkan::frame::Recorder` is the only code that writes draw commands into a command buffer.

`Recorder::record(commands, frame, target, resources, uniforms, timings)`:

1. Transitions the swapchain image (if any) for drawing, and the window's depth image if a pass
   on the swapchain uses depth.
2. Walks `Frame::ordered()`. For each pass:
   - if it draws into a target and is that target's first writer this frame, opens the target;
   - opens a timing span under the pass name;
   - writes the pass camera into the next `FrameUniforms` slot;
   - begins rendering (`vkCmdBeginRendering`), sets viewport and scissor, records the items, and
     ends rendering;
   - closes the timing span;
   - if it is the target's last writer, closes the target.
3. Transitions the swapchain image into `Target::finalLayout`.

`finalLayout` defaults to `PRESENT_SRC_KHR`. That layout is only valid where `VK_KHR_swapchain` is
enabled, so a headless frame names another, usually `SHADER_READ_ONLY_OPTIMAL`. A frame given
no image is one whose every pass has its own target.

**Pass order.** `Frame::ordered()` places every pass that writes a target before every pass that
`reads()` it, and otherwise keeps creation order. Passes into the same target, the swapchain
included, always keep creation order. A pass that reads its own target is reading `previous()`
and is ordered only by creation. A cycle throws. `Frame::order(nodes)` is the same ordering over
plain identities, so it is unit tested without a device.

**Binding.** The recorder remembers what the last item bound: pipeline, set 0, set 1, set 2,
vertex and index buffers with offsets, and the scissor. It binds only what changes, so a run of
quads sharing a texture costs one bind.

**Scissor.** An item with `scissored` set gets a dynamic scissor clamped to the pass region. An
item without one gets the pass region back. A clip rectangle off the top or left of the image is
cut; one turned inside out cuts the draw to nothing (`renderer/Clip.h`).

**`Recorder::check(pass, pipeline, into)`** runs each time a pipeline is bound. It throws,
naming the pass, when:

- the pipeline's layout declares a set 2 and the pass names no `scene()`;
- the pipeline was built with depth bias and the pass names no `depthBias()`;
- the pipeline's colour format differs from what the pass draws into;
- the pass tests depth and the pipeline's depth format differs from the pass's.

A format is checked only where both sides state one. Vulkan would otherwise read whatever set was
left bound, draw at the last bias set, or (for formats) report a validation error per draw. The
check needs no device and has a unit test.

**Set 2 and depth bias** belong to the pass. The recorder binds `Pass::scene()` once per pass,
only for pipelines whose layout declares a third set, so a quad in a lit pass binds nothing
extra. `Pass::depthBias()` is recorded with `vkCmdSetDepthBias` whenever a pipeline built with
`Builder::depthBias(true)` is bound, and ignored for other pipelines.

**`DrawItem::record`** is an escape hatch: a callback the recorder calls instead of issuing its
own draw. It exists for work the item fields cannot describe. Reaching for it routinely means
the item model needs extending.

Background: [ADR-0004](../adr/0004-rendering-submit-draw-items-as-data.md),
[ADR-0064](../adr/0064-lighting-lit-passes-use-the-shared-recorder.md),
[ADR-0068](../adr/0068-rendering-order-passes-by-what-they-read.md)

## Layout transitions

Every layout transition is a synchronization2 barrier built by a named function in
[`vulkan/memory/Barriers.h`](../../api/render/realtime/vulkan/memory/Barriers.h). Each function
decides the stages and access masks on both sides, so two places moving an image the same way
cannot disagree. Add a function there rather than writing a barrier inline.

| Function | From → to | Used for |
|---|---|---|
| `colourForDrawing` | `UNDEFINED` → `COLOR_ATTACHMENT_OPTIMAL` | swapchain image and target colour before the first pass |
| `colourAfterDrawing` | `COLOR_ATTACHMENT_OPTIMAL` → `SHADER_READ_ONLY_OPTIMAL` or `PRESENT_SRC_KHR` | after the last pass |
| `depthForDrawing` | `UNDEFINED` → `DEPTH_ATTACHMENT_OPTIMAL` | depth before the first pass |
| `depthForSampling` | `DEPTH_ATTACHMENT_OPTIMAL` → `DEPTH_READ_ONLY_OPTIMAL` | sampled depth after its last writer |
| `forUpload`, `uploadedForSampling` | `UNDEFINED` → `TRANSFER_DST` → `SHADER_READ_ONLY_OPTIMAL` | texture uploads |
| `forReadback`, `afterReadback` | any → `TRANSFER_SRC` → back | frame capture |
| `record(commands, {...})` | | records several as one dependency |

Every transition into an attachment layout starts from `UNDEFINED` and discards the contents.
Nothing carries between frames, so the first pass to use an image in a frame must clear it.

**A target costs one pair of barriers per frame** however many passes draw into it: opened before
its first writer and closed after its last. After closing, colour is in
`SHADER_READ_ONLY_OPTIMAL` and sampled depth in `DEPTH_READ_ONLY_OPTIMAL`, so every later pass can
read it. A target with no colour image has only its depth moved.

## Depth buffers

`vulkan::frame::DepthBuffer` is a depth image, its view, and (when sampled) a sampler.

**The window's depth buffer.** `DeviceContext` owns one, sized by the context's extent and
rebuilt with it. It is allocated the first frame a pass on the swapchain asks for depth, and
never otherwise, so a 2D app pays nothing. `DeviceContext::depthFormat()` is known from the
device before the image exists, so pipelines can be built first. Every pass on the swapchain that
tests depth shares this one image.

**Format choice.** `DepthBuffer::chooseFormat(physical, sampled)` returns the first of
`D32_SFLOAT`, `D32_SFLOAT_S8_UINT`, `D24_UNORM_S8_UINT` whose optimal-tiling features include
`DEPTH_STENCIL_ATTACHMENT`. With `sampled` true it also requires `SAMPLED_IMAGE`, which can give a
different answer: a device may draw depth into a format without letting a shader sample it.
`stencil()` says whether the format has a stencil aspect, which a barrier must name.

**Sampled depth** is chosen at construction and cannot be switched on later, because it can
change the format. A sampled buffer:

- uses the sampled format list,
- adds `SAMPLED` and `TRANSFER_SRC` usage (the second so it can be captured),
- has a sampler clamped to a **white border**, so a shadow lookup outside the light's frustum
  reads as lit,
- is left in `DEPTH_READ_ONLY_OPTIMAL` by the recorder after its last writer. That layout allows
  the depth aspect to be sampled and depth-tested at the same time.

`pipeline::Texture::descriptor()` names `DEPTH_READ_ONLY_OPTIMAL` for a depth image wherever one
is sampled, a canvas included. The swapchain's depth buffer is never sampled.

A pass that samples a target's depth and draws into it in the same frame breaks the read-only
promise. The recorder cannot detect it; the validation layer reports it.

**Render targets** own a depth buffer per image slot, sized with the target. `RenderTarget`
throws when it has no colour and no sampled depth. Its images, on resize or destruction, are
retired through the ring rather than destroyed at once. A target of more than one image clears
every slot at construction and leaves it readable (`ready()`), so `previous()` is defined on the
first frame.

## Building a pipeline

`vulkan::pipeline::Builder` describes a graphics pipeline one chained call at a time.

```cpp
pipeline::Builder(device)
    .name("quad")
    .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
    .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
    .vertexBinding(0, sizeof(Canvas::Vertex))
    .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Canvas::Vertex, position))
    .set(uniforms->layout())     // set 0 first: sets are numbered in the order added
    .set(textures->layout())     // set 1
    .push(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Push))
    .colourFormat(colour)
    .build(cache);
```

Defaults:

- dynamic viewport and scissor, so a resize needs no rebuild;
- one sample, triangle list, filled polygons;
- no culling, counter-clockwise front face;
- no depth test or write;
- straight alpha blending (`Builder::Blend` defaults: colour `SRC_ALPHA`/`ONE_MINUS_SRC_ALPHA`,
  alpha `ONE`/`ONE_MINUS_SRC_ALPHA`, op `ADD`);
- dynamic rendering, no render pass.

Notes:

- `colourFormats({...})` takes 0..N attachments. An empty list writes no colour: a shadow pass.
  `colourFormat(f)` is the one-attachment form. Every attachment gets the same blend state.
- **The colour format is required**, and a pipeline that tests or writes depth also needs
  `depthFormat()`. The recorder's check compares these against the pass at bind time.
- `blend(Blend{...})` sets custom factors and turns blending on. A pass compositing into
  something composited later wants `destinationAlpha = ZERO`; the default erodes the source's
  alpha.
- `depthBias(true)` sets `depthBiasEnable` and adds `VK_DYNAMIC_STATE_DEPTH_BIAS`, so the values
  come from the pass.
- `layout(VkPipelineLayout)` compiles against a layout the caller owns, for several pipelines
  that share one layout under one bound set. `set()` is then read only to detect a set 2, and
  `push()` only for stage flags. The caller destroys that layout.
- Shader modules belong to the builder and are destroyed with it. The pipeline and its layout are
  returned for `Resources` to own. Building twice gives two identical pipelines.

**`rasterization()`, `colourBlend()` and `dynamics()`** return the state `build()` will use;
`build()` calls these same functions. They exist because a compiled `VkPipeline` cannot be read
back, and a wrong value is a wrong picture rather than an error. For example, a depth bias left
out of the dynamic list compiles and validates cleanly, then uses the zero in the create info,
so `vkCmdSetDepthBias` does nothing. Test pipeline state through these functions.

**Two variants per depth state.** Dynamic rendering matches a pipeline to the attachments of the
pass it draws into, so a pipeline built with no depth format cannot draw into a pass with one.
Each renderer compiles its pipelines twice and picks by `Pass::depth()`.

## Memory

Every buffer and image gets memory from `vulkan::memory::Allocator`, owned by the `Device` and
built once the logical device exists.

| `Allocator::Kind` | Behaviour |
|---|---|
| `Direct` (default) | One `vkAllocateMemory` per resource. Every app in the tree uses it. |
| `Suballocated` | Regions of larger blocks through the Vulkan Memory Allocator (VMA). For an app with per-frame resources, because `maxMemoryAllocationCount` is a real device limit. |

A consumer chooses the kind in the `Device` constructor.

- A resource holds a `memory::Allocation` (block, offset, size, VMA handle), not a
  `VkDeviceMemory`. A suballocated region starts part way into a shared block, so mapping and
  freeing go through the allocator.
- **`Allocator::bind(buffer or image, properties, &allocation)` allocates and binds in one call.**
  A resource never chooses a memory type itself. It throws when no memory type has the
  properties, and returns the `VkResult` for an allocation that failed.
- `free()` on an allocation that names no memory does nothing, so a resource can free
  unconditionally.
- VMA is a private dependency. `Allocator.h` and `Allocation.h` declare its handle types
  themselves, and `vk_mem_alloc.h` is compiled in exactly one translation unit,
  [`VmaImpl.cxx`](../../api/render/realtime/vulkan/memory/VmaImpl.cxx), with warnings and analysis
  off.
- The allocator is destroyed before the device. See [ownership](#object-chain-and-ownership).

Background: [ADR-0053](../adr/0053-memory-optional-vma-suballocation.md)

## Buffers and streaming

| Class | Memory | Use |
|---|---|---|
| `memory::Buffer` | Host visible, coherent, mapped for its whole life | Geometry rewritten every frame; readback |
| `memory::DeviceBuffer` | Device local, filled by staging copy | Geometry built once and drawn many times |
| `memory::Mesh` | Two `DeviceBuffer`s: vertices, and indices when indexed | Static geometry an app owns |

- `Buffer::grow(bytes)` doubles until large enough and **replaces the allocation**, invalidating
  `handle()`. It waits for the device first. `Buffer::read()` is slow (coherent, not cached), so
  use it for one-off readback only.
- `DeviceBuffer::upload()` waits for the copy, so nothing may be in flight against the buffer.
- `memory::Uploader::oneShot(record)` records, submits on the graphics queue and waits. Because it
  waits, a staging allocation never outlives the function that made it. It reuses one command
  buffer. It is for load-time work; uploading during play would need a transfer queue and a
  fence, which do not exist.
- `Mesh` is filled once. Changing geometry means building a new mesh, because a refill would have
  to wait for every frame still drawing the old one. `Mesh::describe(&item)` fills an item's
  buffers, index type and counts.

**`vulkan::frame::StreamRing`** is how the renderers stream per-frame geometry. It keeps a list of
`Buffer` pairs per frame in flight.

- `claim(vertexBytes, indexBytes)` waits on the frame's fence (`Ring::waitFrame()`), then returns
  the next pair in the current frame's list, growing it as needed. That is the fence `acquire`
  waits on anyway, so it costs nothing extra.
- The cursor restarts the first time a slot is claimed from after the ring has begun another
  frame (it compares `Ring::begun()`). Nothing has to be told a frame ended, so a renderer an app
  builds itself reuses its buffers the same way.
- **Each submission gets its own pair.** Appending several canvases into one buffer would break,
  because growing it replaces the allocation and invalidates the handle earlier items hold.
- A buffer the content outgrows is replaced by one twice the size, and the old one is retired
  through the ring.
- `held()` is the busiest frame's claim count times the frames in flight, and does not grow with
  the number of frames run.

Starting sizes: `Quad` 64 KiB of vertices and 32 KiB of indices; `World` 64 KiB and 16 KiB;
`Line` 64 KiB with no index buffer.

Background: [ADR-0010](../adr/0010-meshes-owned-by-the-app-that-built-them.md)

## Descriptor sets and push constants

Sets are organised by how often their contents change.

| Set | Changes | Holds | Owner |
|---|---|---|---|
| 0 | per pass, per frame | the camera: `view`, `projection`, `viewProjection` (three `mat4`) and `viewport` (`vec4`), 208 bytes std140 | `vulkan::frame::FrameUniforms` |
| 1 | per material | one combined image sampler at binding 0, fragment stage (`FullScreen` declares one per source) | `Textures` (and `FullScreen`, `Grade`) |
| 2 | per pass, only for pipelines that declare it | a lit scene: `Scene` uniform (binding 0), shadow map (binding 1), joint palette storage buffer (binding 2) | `vulkan::renderer::Lit` |
| push | per draw | transform, tint, flags, a skinned object's first joint | the item |

Rules:

- **Every pipeline declares set 0's layout first**, even one that reads nothing from it. A set
  bound for one pipeline then stays bound across a switch to another built against the same
  layout. Compatibility runs from set 0 upwards, so pipelines with nothing beyond set 0 (lines)
  stay compatible with ones that add set 1.
- **Per-object data goes in push constants, never in a set.** An item binds at most one set of
  its own (set 1), and merging two adjacent items is a comparison of two handles.
- A pipeline that declares set 2 must be drawn in a pass that names a scene set; the recorder
  checks this.

**Set 0.** `FrameUniforms` owns the layout (one uniform buffer at binding 0, vertex and fragment
stages) and a list of slots per frame in flight, each a small buffer and a set that points at it
permanently. The recorder writes each pass's camera into the next slot of the current frame and
binds it for every item in the pass. Slots are added as passes need them and never returned; the
pass count settles in the first few frames. A slot is written during recording, after the fence
wait, so nothing is reading it.

Who reads set 0:

- The quad pipeline declares it and reads nothing. A canvas pushes its own projection, so two
  canvases in one pass may map different spaces.
- Lines, world quads and the lit pipelines read it.
- voxel's terrain pipeline reads it and nothing else per draw except a 16-byte push constant
  (the chunk origin), with its block palette at set 1.

**Push constants.** `DrawItem::pushCapacity` is 128 bytes, the Vulkan minimum guarantee.
Current blocks:

| Pipeline | Block | Size |
|---|---|---|
| Quad | `mat4 projection; uint text` | 68 bytes |
| Lit | `Lit::Object`: `mat4 model; vec4 baseColour; float outline; uint firstJoint` | 88 bytes |
| Line, World, FullScreen | none | |

The block is copied into the item by value, so the unused part of the 128 bytes is copied per
item per frame whether or not a pipeline declared it.

**Descriptor pools.** `vulkan::pipeline::DescriptorPool` holds one layout and a list of Vulkan
pools, each created for a fixed number of sets. When the last pool is full, another is added. A
set handed back with `release()` is retired through the ring and then reused; the pools are not
created with the free flag. A reused set holds whatever was last written into it, so the caller
writes it before binding.

| Pool | Sets per Vulkan pool |
|---|---|
| set 0 (`FrameUniforms`) | 32 |
| set 1 (`Textures`) | 64 |
| `FullScreen` sources | 8 |
| set 2 (`Lit` scenes) | frames in flight |

Background: [ADR-0008](../adr/0008-shaders-descriptor-sets-by-update-frequency.md),
[ADR-0064](../adr/0064-lighting-lit-passes-use-the-shared-recorder.md)

## Draw items and the sort key

A `DrawItem` describes one draw: pipeline and material handles, vertex and index buffers with
offsets, index type, the push block and its size, vertex/index/instance counts and firsts, an
optional scissor, and the optional `record` callback.

**`SortKey`** packs four 16-bit fields into one `uint64_t`, coarsest first, so a pass sorts on a
single comparison:

| Bits | Field | Set by |
|---|---|---|
| 63–48 | `layer` | the caller (painter order) |
| 47–32 | `pipeline` | `Pass::submit`, from the pipeline handle's slot |
| 31–16 | `material` | `Pass::submit`, from the material handle's slot |
| 15–0 | `depth` | the caller (view depth quantised) |

Layer is first because 2D content is painter ordered. Pipeline then material groups the items
that can share bindings. `Pass::submit` overwrites the pipeline and material fields from the
item's handles, so a caller sets only layer and depth. Sorting is opt-in per pass and stable.

Handles are used instead of pointers because a pointer's value depends on the allocator, which
would reorder a frame differently on each run. There is no geometry field in the key and no
geometry handle; items hold raw `VkBuffer`s.

## Resources and deferred destruction

`vulkan::pipeline::Resources` owns everything a draw item names by handle: pipelines (with their
layouts), materials and textures. It is built on `Registry<Tag, T>`
([`Registry.h`](../../api/render/realtime/Registry.h)).

**Handles.** `Handle<Tag>` ([`Handle.h`](../../api/render/realtime/Handle.h)) holds a 32-bit slot
and a 32-bit generation. The tag keeps `PipelineHandle`, `MaterialHandle`, `TextureHandle` and
`MeshHandle` distinct types. A released slot is reused by the next `add()` with its generation
incremented, and `resolve()` refuses a handle whose generation is stale. So a released handle
resolves to nothing at once, and can never come to name whatever fills its slot next. Only the
slot is a sort order.

**Deferred destruction.** Releasing a handle hands the objects behind it to the ring as a
callback (`Ring::retire()`). `vulkan::frame::Retirement` stores each callback with the count of
frames begun at release. `Ring::begin()` runs every callback whose frames have finished: a frame
begun before the release has finished once `framesInFlight` more frames have begun, because
beginning a frame waits on the fence of the slot's previous use. The ring's destructor waits for
the device and runs everything left.

- **Anything driving frames must call `Ring::begin()`,** or nothing retired is ever collected. A
  device test counting live allocations has to run the ring that far first.
- A released material returns its descriptor set to its `DescriptorPool` the same way.
- `Resources::release(TextureHandle)` does not release a material naming the texture; whoever
  made the material does that. `Textures::release()` releases both.
- Pipelines are built at load time and never released.
- `Resources`' destructor destroys everything still registered, in the order Vulkan requires.

**What is registered for a render target shares its images.** A `pipeline::Texture` holds its
image through a shared pointer, so whichever of the target and the registration lets go last
frees them. After `RenderTarget::recreate()` the old registration still names the old
images, so it must be released and the target registered again.

**Meshes are outside `Resources`.** An app's `memory::Mesh` belongs to whatever built it.
`MeshRegistry` is a separate registry with the same slot-and-generation handles: an entry holds
a `memory::Mesh`, its parts and an optional skin. Releasing an entry retires its mesh through the
ring and drops each albedo's user count, releasing the albedo with its last user. The registry's
destructor destroys remaining meshes at once, so it must run after the device is idle.

Background: [ADR-0061](../adr/0061-resources-explicit-release-generational-handles.md),
[ADR-0065](../adr/0065-meshes-shared-registry-keyed-by-path.md)

## Textures

`Textures` lives on `DeviceContext` and is built with it, so a context that draws no quads (a
headless one, or one only loading meshes) can still register textures. It owns:

- the `memory::TextureFactory`, which copies through the context's one `Uploader`;
- set 1's `DescriptorPool` and layout, which every textured pipeline declares;
- the 1×1 white texture, alive as long as the context;
- the map from texture to material, keyed by the whole handle so a reused slot never finds an old
  set.

`Quad`, `World`, `Lit`, `Grade` and `MeshRegistry` all take `Textures`, so one uploaded atlas
serves every primitive from one pool.

`TextureFactory`:

- uploads `UNORM` by default (`Encoding::Display`), or `_SRGB` with `Encoding::Srgb`;
- gives a single-channel image (`R8_UNORM`) a view swizzled to `(1, 1, 1, R)`, so a coverage
  mask or a glyph atlas samples as white with alpha and the shader needs no branch for it;
- can create a 3D texture from raw texels, which `Grade` uses for its table.

`Textures::depthTexture(target)` and `texture(target)` return the white texture for a target with
nothing of that kind to sample. A descriptor written against an image without sampled usage is
undefined, and a white shadow map reads as unshadowed. `release()` never releases the white
texture, so such a handle can be released like any other.

Background: [ADR-0082](../adr/0082-textures-owned-by-the-device-context.md)

## The renderers

Each renderer in `vulkan/renderer` takes the context's device, pipeline cache, resources, ring
and frame uniforms, plus the colour and depth formats it draws into. Each compiles a variant per
depth state.

| Renderer | Pipelines | Depth variant | Notes |
|---|---|---|---|
| `Quad` | 2 | neither tests nor writes | one vertex format (position, uv, colour); untextured quads sample white; `text` push flag selects the distance-field branch |
| `Line` | 2 | tests and writes | `LINE_LIST`, no index buffer; positions in world space through set 0 |
| `World` | 4 (alpha and additive, each with and without depth) | tests, does not write | additive: colour added by source alpha, destination alpha kept |
| `Lit` | cel, outline, shadow, and a skinned version of each | tests and writes | front face clockwise, back faces culled (outline culls front); shadow pipeline has depth bias; built only when given a shadow format |
| `FullScreen` | 1 | neither | one triangle from three vertices and no vertex buffer; caller's fragment SPIR-V |

`DeviceContext` builds `Quad`, `Line` and `World` on first request against its colour format and
`depthFormat()`. `Quad` is lazy too, because `Context3D` does not know its format until the
swapchain exists.

**Quad text.** A batch carries a text flag, and `Canvas` never merges across it. The fragment
shader thresholds a text batch's sampled distance at 0.5 with `smoothstep`, using `fwidth` for a
one-pixel edge at any scale. Other batches return `colour * texel`.

**Lit front faces are clockwise.** A model is wound counter-clockwise seen from outside, and the
cameras in `api/type` flip y into Vulkan clip space. `shadow::light()` builds its view the way
`type::camera::Camera` does, so faces wind the same under the light and the shadow pipeline culls
as the cel one does.

**`Lit::scene()`** writes the frame's slot: a uniform buffer (`SceneUniforms`, std140, matching
the `Scene` block in `lit.glsl`), the shadow map binding, and the palette storage buffer. It waits
on `Ring::waitFrame()` first, so it is called while the frame is built and before the ring begins
it. The palette buffer is one per frame in flight, grows by doubling, and retires an outgrown
buffer through the ring. A static item pushes `firstJoint = 0` and its pipeline ignores it.

**Skinned vertex.** `MeshRegistry::SkinnedVertex` is `type::Model::Vertex` (32 bytes) followed by
`type::Model::Influence`: four joints as unsigned shorts and four float weights, 56 bytes in all.

**`FullScreen` sources** are allocated from its own `DescriptorPool`, one binding per source.
A depth image is bound in `DEPTH_READ_ONLY_OPTIMAL`. A source must be released before the
`FullScreen` goes, since the set belongs to its pool. `Grade` keeps a map from the handle a caller
holds to the current material; `replace()` creates a new table, rebinds every source to new
materials, and retires the old table and materials through the ring.

Background: [ADR-0005](../adr/0005-2d-one-batched-quad-pipeline.md),
[ADR-0011](../adr/0011-rendering-lines-as-a-world-space-primitive.md),
[ADR-0036](../adr/0036-text-sdf-glyphs-through-the-quad-shader.md),
[ADR-0042](../adr/0042-rendering-world-space-sprites.md),
[ADR-0071](../adr/0071-skinning-joint-matrices-in-one-storage-buffer.md)

## Shaders

The engine's shaders are in [api/render/shaders](../../api/render/shaders). They are compiled to
SPIR-V by `glslc` at build time and embedded in the library as `uint32_t` arrays. They are not
data files: CMake does not copy per-app data into the build tree, and a shader file beside an
executable would go stale silently. `v3d_add_shader`, `OUTPUT`, `DEFINES`, includes and depfiles
are described in [contributing/Build.md](../contributing/Build.md#shaders).

| Shader | Used by |
|---|---|
| `quad.vert`, `quad.frag` | `Quad` |
| `line.vert`, `line.frag` | `Line` |
| `world.vert`, `world.frag` | `World` |
| `fullscreen.vert`, `grade.frag` | `FullScreen`, `Grade` |
| `lit/mesh.vert`, `lit/cel.frag`, `lit/outline.vert`, `lit/outline.frag`, `lit/shadow.vert` | `Lit` |
| `skinned_mesh.vert`, `skinned_outline.vert`, `skinned_shadow.vert` | `Lit`, the three lit vertex stages compiled with `SKINNED` |

The lit shaders share their blocks through includes:

- `lit/lit.glsl` declares the `Camera` (set 0), `Scene` (set 2, binding 0) and `Object` (push)
  blocks. Every lit shader includes it, so a block is written once.
- `lit/pose.glsl` defines `pose()`: the identity normally, or the weighted joint matrices when
  `SKINNED` is defined, in which case it includes `lit/skin.glsl` (the palette at set 2, binding 2,
  and the joint and weight attributes at locations 3 and 4).

`Lit::Shaders::embedded()` returns the built-in SPIR-V; a consumer may pass its own (see
[api/rendering/Lighting.md](../api/rendering/Lighting.md#replacing-the-lit-shaders)). Pipeline creation is the only
check that a replacement matches the layout. When changing a block in `lit.glsl`, change
`SceneUniforms` or `Lit::Object` to match; reordering members is not caught by anything.

## Timings and statistics

**GPU timings.** The ring owns a `vulkan::frame::Timings`: a timestamp query pool per slot, with
room for 64 spans a frame. `Ring::begin()` reads what the slot timed on its last use (its fence
has signalled, so this never waits) and resets the queries in the new command buffer. The
recorder writes a timestamp either side of every pass, under the pass name. So
`Engine3D::timings()` reports the frame that is as old as the ring is deep. A caller recording
its own commands into the ring's buffer may `open()` and `close()` spans of its own; spans do not
nest. A graphics family with zero `timestampValidBits` leaves timings off, and every call does
nothing.

**CPU statistics.** `engine::Statistics::scope(name)` times a span of the frame on the CPU into
its own row, and `ui::shell::StatisticsOverlay` draws a line per span it is given.

## Frame capture

`vulkan::frame::Capture` copies an image into a host-visible `Buffer` and writes a PNG. Its use is
in [api/rendering/FramesAndTargets.md](../api/rendering/FramesAndTargets.md#reading-a-frame-back). Inside:

- `record()` takes a `Capture::Source` (image, extent, format, layout, depth flag), so one code
  path reads a swapchain image or a target. The swapchain overload fills one in from an acquired
  image in `PRESENT_SRC_KHR`. That is the only point a chain image may be read: after recording,
  before present, while acquired.
- The image goes to `TRANSFER_SRC` and back to the layout it arrived in. **The barrier back
  differs by destination.** Returning a chain image to `PRESENT_SRC_KHR` needs nothing made
  visible, because the semaphore presentation waits on orders it. Returning a target to a layout
  a later command in the same submit may sample or draw into has no such semaphore, so the
  transition must be complete and visible before them.
- Target colour images and sampled depth images carry `TRANSFER_SRC` usage so they can be copied.
- `convert()` swaps BGRA to RGBA and writes alpha opaque. It needs no device and is unit tested.
- A depth source must be `D32_SFLOAT`, since its copy is exactly the float a test compares; any
  other depth format throws.

Background: [ADR-0054](../adr/0054-testing-golden-images-hold-only-spec-exact-output.md)

## Invariants that are easy to break

- **Build every pipeline against the format of what it draws into.** Use
  `DeviceContext::colourFormat()` (the swapchain's actual format) or the target's `format()` and
  `depthFormat()`, never a requested or default format. The recorder throws on a mismatch.
- **Every pipeline declares set 0 first.** Add `FrameUniforms::layout()` with the first `set()`
  call, even if the shader reads nothing from it.
- **Per-object data goes in push constants**, within `DrawItem::pushCapacity` (128 bytes).
- **Never write a barrier inline.** Use or add a function in `memory/Barriers.h`.
- **Never destroy a GPU object a frame may still use.** Retire it through `Ring::retire()`. This
  covers textures, sets, outgrown stream buffers and target images.
- **Never share one stream buffer between two submissions.** `Buffer::grow()` replaces the
  allocation and invalidates the handle earlier draw items hold.
- **Write per-frame data after `Ring::waitFrame()`** and before the ring begins the frame.
  `StreamRing::claim()` and `Lit::scene()` do this; anything new keeping per-frame data must too.
- **A submit must signal `Ring::fence()`.** Otherwise the next turn around the ring waits
  forever.
- **Drive frames through `Ring::begin()`.** Otherwise retired objects are never destroyed.
- **The first pass to use an attachment in a frame must clear it.** Attachments are transitioned
  from `UNDEFINED` every frame.
- **Teardown order matters.** The ring outlives what it paces; the presenter goes first; the
  allocator goes before the device; the context goes before the window.
- **A pipeline built with depth bias must put `VK_DYNAMIC_STATE_DEPTH_BIAS` in its dynamic list.**
  `Builder::depthBias(true)` does both; check with `Builder::dynamics()`.
- **Change `lit.glsl` blocks and their C++ structs together** (`SceneUniforms`, `Lit::Object`,
  `FrameUniforms::Camera`).
- **A sorted pass must not hold painter-ordered 2D content.** Sorting groups by pipeline and
  material within a layer.

## Tests

Unit tests that need no GPU (canvas batching, sort keys, frame ordering, recorder checks,
swapchain format choice, retirement, capture conversion) and device tests that draw headless are
in [api/render/tests](../../api/render/tests). How to run them and verify a rendering change is in
[contributing/Testing.md](../contributing/Testing.md).
