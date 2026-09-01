# The Rendering Pipeline

Describes what `api/render/realtime` actually does, as of 2026-08-31. It was a page of open
questions until the Vulkan frame loop landed; the questions that are still open are at the
bottom, and the rest is now a description rather than a proposal.

The decisions behind the shape of this are [ADR-0001](adr/0001-vulkan-replaces-opengl.md)
through [ADR-0005](adr/0005-one-batched-quad-primitive.md). Read those for why; this is what.

## The chain of objects

```
Window3D  ->  Context3D  ->  Frame  ->  Pass  ->  DrawItem
                  |
                  +-- vulkan::Device      the gpu, its queues, and the 1.3 features
                  +-- vulkan::Swapchain   the images presented to the window
                  +-- vulkan::Presenter   acquire, submit, present, and the sync between them
                  +-- vulkan::PipelineCache
                  +-- vulkan::Resources   pipelines, materials and textures, addressed by handle
```

`Window3D` creates an `SDL_WINDOW_VULKAN` window and owns the `vulkan::Instance` and
`vulkan::Surface`. `Context3D` is built from a created window and owns everything that
belongs to the device. `Engine3D` drives a frame per tick.

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
drawing: whether it clears and to what, whether it depth tests, and what region of the target
it draws into.

A `DrawItem` is a description of one draw, not something that draws itself
([ADR-0004](adr/0004-operations-as-draw-data.md)). It names its pipeline and material by
handle and carries a `SortKey`. The engine owns sorting, merging and recording.

**Nothing sorts yet.** `Recorder` walks each pass in submission order. The sort key is filled
in from the first version anyway, because the layer field is the thing that keeps painter
ordering correct once sorting arrives, and retrofitting it means auditing every call site
with an invisible failure mode.

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
| set 0 | per frame, bound once by the pass | camera, projection, viewport, time |
| set 1 | per material | the sampled texture and whatever else the material needs |
| push constants | per object | transform and tint |

A draw item names a material, and the material owns its set 1. Anything that changes per
object goes in push constants rather than in a third set, which keeps the number of
descriptor sets bound per draw at two and makes merging adjacent items a matter of comparing
two handles.

The sort key is ordered to match: layer, then pipeline, then material, then depth. Sorting on
it groups exactly the draws that can share a binding.

## Resource handles

`DrawItem` refers to pipelines, materials and textures by handle, never by pointer - a
pointer sorts by whatever the allocator handed out, which reorders a frame differently on
every run. `vulkan::Resources` owns them, hands out the handles, and destroys everything when
the context goes. Slots are never reused, so a handle cannot come to mean something other
than what it was given for.

Nothing frees an individual resource. Textures and pipelines are built at load time and used
until the app closes; per-level unloading is the thing that will ask for more.

## What is not built yet

- **Any pipeline at all.** Nothing binds a pipeline or a descriptor set, so the only draw
  item that draws anything is one carrying a `record` callback. The batched quad of
  [ADR-0005](adr/0005-one-batched-quad-primitive.md) is the first real one, in phase 3.
- **Sorting and merging.** The recorder walks submission order.
- **Depth.** `Pass::depth` is recorded and ignored; there is no depth attachment.
- **Offscreen targets**, and with them compositing and logical presentation.
- **The GL path is still in the tree.** `api/gl`, `operation::Canvas`, `operation::GLTexture`
  and the rest still exist, and pong and tetris still call them against a context nothing
  creates. They go in phase 3.

## Still open: how this meets the ECS

`Scene::collect()` returning a frame is the shape the 2D engine was written to, and it fits
the pass model - a scene collects its renderables into passes. What is not settled is what a
renderable component looks like. `Renderable` as a marker says nothing about how to draw the
entity, and the candidates - Sprite, Canvas, TextureFont - are the old operation classes
wearing component names, which is the design [ADR-0004](adr/0004-operations-as-draw-data.md)
moved away from. The likely answer is a component that names a material and a mesh or quad,
and a rendering system that turns those into draw items, but nothing has been built to prove
it. See [ECSDesign.md](ECSDesign.md).
