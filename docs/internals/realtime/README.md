# The realtime renderer, inside

These pages are for someone changing `api/render/realtime`. How an app uses the renderer is in
[api/rendering/](../../api/rendering/README.md), and these pages do not repeat it.

Names are in `v3d::render::realtime`; `vulkan::` is `v3d::render::realtime::vulkan`. The code is
in [api/render/realtime](../../../api/render/realtime), split into `vulkan/device`,
`vulkan/frame`, `vulkan/memory`, `vulkan/pipeline` and `vulkan/renderer`.

| Page | Read it to |
|---|---|
| This page | See which object owns which, the invariants that are easy to break, and where the tests are |
| [Device.md](Device.md) | Change how a GPU is chosen, or what is asked of it |
| [Frames.md](Frames.md) | Follow a frame from acquire to present: the ring, recording, barriers, timings and capture |
| [Pipelines.md](Pipelines.md) | Build a pipeline, lay out descriptor sets and push constants, order draws, and add shaders |
| [Memory.md](Memory.md) | Allocate memory and buffers, create depth buffers and textures, and release resources safely |
| [Renderers.md](Renderers.md) | Change or add one of the renderers that turn primitives into draw items |

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

Background: [ADR-0003](../../adr/0003-rendering-one-engine-for-2d-and-3d.md),
[ADR-0051](../../adr/0051-frames-in-flight-ring-separate-from-presenting.md)

## Invariants that are easy to break

- **Build every pipeline against the format of what it draws into.** Use
  `DeviceContext::colourFormat()` (the swapchain's actual format) or the target's `format()` and
  `depthFormat()`, never a requested or default format. The recorder throws on a mismatch.
- **Every pipeline declares set 0 first.** Add `FrameUniforms::layout()` with the first `set()`
  call, even if the shader reads nothing from it.
- **Per-object data goes in push constants**, within `DrawItem::pushCapacity` (128 bytes).
- **Never write a barrier inline.** Use or add a function in `memory/Barriers.h`.
- **Never destroy a GPU object a frame may still use.** Retire it through `Ring::retire()`. This
  covers materials, textures, sets, descriptor pools, outgrown stream buffers and target images.
- **Never share one stream buffer between two submissions.** `Buffer::grow()` replaces the
  allocation and invalidates the handle earlier draw items hold.
- **Write per-frame data after `Ring::waitFrame()`** and before the ring begins the frame.
  `StreamRing::claim()` and `Lit::scene()` do this; anything new keeping per-frame data must too.
- **A submit must signal `Ring::submitting()`, called immediately before it.** Otherwise the
  next turn around the ring waits forever.
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
in [api/render/tests](../../../api/render/tests). How to run them and verify a rendering change is in
[contributing/Testing.md](../../contributing/Testing.md).
