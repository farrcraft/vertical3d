# The realtime renderer, inside

These pages are for someone changing `api/render/realtime`. How an app uses the renderer is in
[api/rendering/](../../api/rendering/README.md), and these pages do not repeat it.

Names are in `v3d::render::realtime`; `vulkan::` is `v3d::render::realtime::vulkan`. The code is
in [api/render/realtime](../../../api/render/realtime). The root holds what an app names: the
contexts, the frame and its recorder, render targets, meshes, materials and the handle registry,
and the stores a context owns. `renderer/` holds the renderers, `canvas/` the canvases an app
fills on the CPU, `entity/` the functions that draw ECS entities, and `lighting/` the lit pass's
settings and shadow fitting.

`vulkan/` holds the wrappers over Vulkan objects, split into `vulkan/device`, `vulkan/frame`,
`vulkan/memory` and `vulkan/pipeline`. **Nothing in `vulkan/` includes a header above it.** It
speaks Vulkan's own terms, and the root turns an app's terms into them: `Present` becomes the
swapchain's list of present modes, for one. An app names `vulkan::` only when it builds a pipeline
or a buffer of its own.

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
window::Window             the SDL window, in its own library; holds no Vulkan object
sharedInstance()           the process's vulkan::device::Instance, shared by every window
WindowRenderer             owns the WindowContext and the Frame being built
  WindowContext : DeviceContext
    vulkan::frame::Swapchain   the images presented to the window
    vulkan::frame::Presenter   acquire, submit, present, and their semaphores
  DeviceContext              everything that needs only a device
    vulkan::device::Device     physical + logical device, queues, memory::Allocator, and the
                               Surface WindowContext made for the window
    vulkan::frame::Ring        frames in flight: a command buffer and a fence each
    vulkan::pipeline::Cache    the VkPipelineCache
    Resources                  pipelines, materials and textures, by handle
    vulkan::memory::Uploader   one-shot copies to device-local memory
    vulkan::frame::FrameUniforms set 0: one camera per pass per frame in flight
    vulkan::frame::DepthBuffer the window's depth image, made on first request
    Textures                   texture factory, the texture array at set 1, white texture, slots
    renderer::Quad / Line / World   built on first request
Frame -> Pass -> DrawItem
```

There is one window class for 2D and 3D. A 2D app differs only in what its passes ask for: an
orthographic projection and no depth.

**`DeviceContext` is usable without a window.** It is given a colour format and an extent at
construction, or later through the protected `describe()`. Every pipeline built through it is
compiled against that format. A headless context (as in the device tests) is a
`DeviceContext` on a `Device` built with no surface. `WindowContext` has no format at construction
because its swapchain does not exist yet, so it calls `describe()` after creating the swapchain
and after every rebuild. A renderer already built keeps the format it was compiled
with.

**Member order sets teardown order.** C++ destroys members in reverse declaration order:

- `DeviceContext` declares the `Ring` before everything it paces, so the ring is destroyed after
  them. A renderer's buffers may still be in flight on one of the ring's fences. The context's
  destructor calls `Ring::drain()`, which waits for the device and runs every pending
  retirement. A retired object may hold the ring itself, as a mesh does through its uploader, so
  the ring cannot be left to run them in its own destructor.
- `WindowContext` declares the `Presenter` last, so it is destroyed first. Nothing else may go while
  a frame it submitted is in flight.
- `Device` resets its `memory::Allocator` explicitly in its destructor body, before
  `vkDestroyDevice`. A suballocator frees its blocks against the device, which must still exist.

**`WindowRenderer::shutdown()`** waits for the ring to go idle, drops the frame, and resets the
context. The context must go before the window, because the device holds the surface made for
the window, and a surface has to be destroyed before its window. The surface holds the shared
instance, so the instance, and the Vulkan loader under it, outlive every surface made against
them.

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
- **Per-object data goes in push constants**, within `DrawItem::pushCapacity` (128 bytes),
  except in a lit pipeline, which reads it from the scene's object buffer by instance index.
- **Never write a barrier inline.** Use or add a function in `memory/Barriers.h`.
- **Never destroy a GPU object a frame may still use.** Retire it through `Ring::retire()`. This
  covers materials, textures, sets, descriptor pools, outgrown stream and readback buffers,
  target images and the window's depth buffer. Only `Swapchain::recreate()` waits for the device
  instead.
- **Never share one stream buffer between two submissions.** The second submission's writes land
  while the device may still be reading the first's.
- **Write per-frame data after `Ring::waitFrame()`** and before the ring begins the frame.
  `StreamRing::claim()` and `Lit::scene()` do this, and `Lit::object()` writes into the slot
  `scene()` waited for. Anything new keeping per-frame data must do the same.
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
swapchain format choice, retirement, capture conversion) are in
[api/render/tests](../../../api/render/tests). Device tests that draw headless are in its
`device/` directory, and the `render_window` suite in `window/` presents to a real window. How
to run them and verify a rendering change is in
[contributing/Testing.md](../../contributing/Testing.md).
