# Memory and resources

How GPU memory is allocated, how buffers, depth buffers and textures are made, and how a
resource is released while frames that use it may still be in flight.

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
`stencil()` says whether the format has a stencil aspect. Nothing uses the stencil aspect, and
every depth barrier names only the depth aspect, in `DEPTH_ATTACHMENT_OPTIMAL` or
`DEPTH_READ_ONLY_OPTIMAL`. For a combined format that is valid only because the device enables
`separateDepthStencilLayouts`. Naming the stencil aspect as well would be an error, because
those two layouts are depth-only.

**Sampled depth** is chosen at construction and cannot be switched on later, because it can
change the format. A sampled buffer:

- uses the sampled format list,
- adds `SAMPLED` and `TRANSFER_SRC` usage (the second so it can be captured),
- has a sampler clamped to a **white border**, so a shadow lookup outside the light's frustum
  reads as lit,
- filters with `NEAREST`, so each read is one stored depth. The cel shader averages a 3x3 set
  of comparisons itself, and a device need not support linear filtering of a depth format,
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
  [`VmaImpl.cxx`](../../../api/render/realtime/vulkan/memory/VmaImpl.cxx), with warnings and
  analysis off.
- The allocator is destroyed before the device. See [ownership](README.md#object-chain-and-ownership).

Background: [ADR-0053](../../adr/0053-memory-optional-vma-suballocation.md)

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
- The cursor restarts the first time a slot is claimed from after the ring has begun, begun
  again, skipped or advanced (it compares `Ring::turns()` and `Ring::frame()`). A slot begun
  again after an abandoned attempt reuses that attempt's claims. The cursor never passes the end
  of the slot's list, so a claim takes a held pair or appends one. Nothing has to be told a frame
  ended, so a renderer an app builds itself reuses its buffers the same way. `Engine3D` calls
  `Ring::skip()` for a frame it does not draw, out of date or with no swapchain, so the claims
  of a run of skipped frames reuse one frame's buffers rather than piling up.
- **Each submission gets its own pair.** Appending several canvases into one buffer would break,
  because growing it replaces the allocation and invalidates the handle earlier items hold.
- A buffer the content outgrows is replaced by one twice the size, and the old one is retired
  through the ring.
- `held()` is the busiest frame's claim count times the frames in flight, and does not grow with
  the number of frames run.

Starting sizes: `Quad` 64 KiB of vertices and 32 KiB of indices; `World` 64 KiB and 16 KiB;
`Line` 64 KiB with no index buffer.

Background: [ADR-0010](../../adr/0010-meshes-owned-by-the-app-that-built-them.md)

## Resources and deferred destruction

`vulkan::pipeline::Resources` owns everything a draw item names by handle: pipelines (with their
layouts), materials and textures. It is built on `Registry<Tag, T>`
([`Registry.h`](../../../api/render/realtime/Registry.h)).

**Handles.** `Handle<Tag>` ([`Handle.h`](../../../api/render/realtime/Handle.h)) holds a 32-bit slot
and a 32-bit generation. The tag keeps `PipelineHandle`, `MaterialHandle`, `TextureHandle` and
`MeshHandle` distinct types. A released slot is reused by a later `add()` with its generation
incremented, and `resolve()` refuses a handle whose generation is stale. A handle can therefore
never come to name whatever fills its slot next. Only the slot is a sort order.

**Deferred destruction.** Releasing a handle hands the objects behind it to the ring as a
callback (`Ring::retire()`). `vulkan::frame::Retirement` stores each callback with the last frame
that may name the released object, `Ring::recording()`. While a frame is begun and not yet
submitted, that is the frame being recorded. Between a submit and the next begin, it is the frame
about to be begun. `Ring::begin()` runs every callback whose frames have finished: a frame has
finished once `framesInFlight` more frames have begun, because beginning a frame waits on the
fence of the slot's previous use. The ring's destructor waits for the device and runs everything
left.

**A release and the items already queued.** A draw item names its material by handle, and the
recorder resolves it when the frame is recorded. A release therefore cannot take effect at
once:

- A released material goes on resolving until the frame `Ring::recording()` named at the release
  is submitted. Items queued before the release draw with it. It resolves to nothing after.
- A released texture resolves to nothing at once. A texture is resolved only to write a
  descriptor, and that descriptor may be bound after the frame. Its image lives until the frame
  has finished.
- A released material or texture keeps its slot until its callback runs. `textureCount()` counts
  a released texture until then.
- Releasing a handle already waiting in the ring returns false.

As a result, a mesh, a texture or a post source released after its items are queued and before
`renderFrame()` is drawn by that frame. The callbacks share `Resources`' registries through a
shared pointer, because the ring outlives `Resources` and may run them after it is destroyed.

- **Anything driving frames must call `Ring::begin()`,** or nothing retired is ever collected. A
  device test counting live allocations has to run the ring that far first.
- A released material returns its descriptor set to its `DescriptorPool` the same way. Its owner
  hands the set back only when `Resources::release()` returned true, so a set is never handed
  back twice.
- A `DescriptorPool`'s destructor retires its Vulkan pools and layout the same way, so a set from
  a destroyed `FullScreen` lives until the frames binding it have finished.
- `Resources::release(TextureHandle)` does not release a material naming the texture; whoever
  made the material does that. `Textures::release()` releases both.
- Pipelines are built at load time and never released.
- `Resources`' destructor destroys only the pipelines. Textures and materials still registered
  are destroyed with the registries `Resources` shares with pending releases, once the ring has
  run those releases. A material's descriptor set is freed with the pool it came from.

**What is registered for a render target shares its images.** A `pipeline::Texture` holds its
image through a shared pointer, so whichever of the target and the registration lets go last
frees them. After `RenderTarget::recreate()` the old registration still names the old
images, so it must be released and the target registered again.

**Meshes are outside `Resources`.** An app's `memory::Mesh` belongs to whatever built it.
`MeshRegistry` is a separate registry with the same slot-and-generation handles: an entry holds
a `memory::Mesh`, its parts and an optional skin. Releasing an entry retires its mesh through the
ring and drops each albedo's user count, releasing the albedo with its last user. The registry's
destructor destroys remaining meshes at once, so it must run after the device is idle.

Background: [ADR-0061](../../adr/0061-resources-explicit-release-generational-handles.md),
[ADR-0065](../../adr/0065-meshes-shared-registry-keyed-by-path.md)

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

Background: [ADR-0082](../../adr/0082-textures-owned-by-the-device-context.md)
