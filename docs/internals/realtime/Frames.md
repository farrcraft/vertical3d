# Frames

How a frame reaches the GPU: the frames in flight, recording passes into a command buffer, the
layout transitions between passes, and what is measured and captured along the way.

## Frames in flight, the presenter and the swapchain

Three classes split the work.

**`vulkan::frame::Ring`** needs only a device. It holds, per frame in flight (two by default):

- a command buffer,
- a fence that the frame's submit signals,
- a timestamp query pool (through `Timings`).

`frame()` is the slot being recorded. `waitFrame()` waits for that slot's last submit; anything
writing a per-frame resource before recording calls it. `begin()` waits on the fence, begins the
command buffer, reads the slot's timings, and collects retired objects. It does not reset the
fence: `submitting()` does that, immediately before the submit. A slot begun and abandoned before
its submit is begun again without being counted as a new frame: `begun()`, and so the collection
of retired objects, does not move. `turns()` does move, on every begin, every begin again and
every `skip()`, so per-frame state that restarts when a frame begins restarts for the retried
attempt too. `advance()` moves to the next slot after the submit. Every renderer and
`FrameUniforms` is built on the ring, not on the presenter, because pacing is not presenting.

**`vulkan::frame::Presenter`** adds what needs the swapchain:

- an image-available semaphore per frame in flight,
- a render-finished semaphore per swapchain image, because presentation waits on it and
  presentation is tied to the image.

**The fence is shared.** The ring creates and waits on it; the submit in `Presenter::present()`
signals it. That submit passes `ring.submitting()`, which resets the fence and returns it, and
nothing else resets it. A submit that does not signal it leaves the next turn around the ring
waiting forever. `acquire()` waits on the ring's fence *before* acquiring, rather than leaving it
to `Ring::begin()`, because this slot's image-available semaphore may still be pending from its
last use. Because the fence stays signalled until the submit, a chain found out of date after
the wait, or a frame abandoned because recording threw, leaves nothing waiting on it.

**`vulkan::frame::Swapchain`** owns the images and views.

- `chooseFormat(formats, preferred)` returns the preferred format where the surface offers it in
  a non-linear sRGB colour space; otherwise a 32-bit `UNORM` format (`B8G8R8A8` or `R8G8B8A8`);
  otherwise the first offered. A missed preference logs a warning. The function needs no device
  and has a unit test.
- The present mode is mailbox where offered, otherwise FIFO.
- The images have `COLOR_ATTACHMENT` usage, plus `TRANSFER_SRC` where the surface's
  `supportedUsageFlags` include it. `copyable()` reports which.
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

Background: [ADR-0004](../../adr/0004-rendering-submit-draw-items-as-data.md),
[ADR-0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md),
[ADR-0068](../../adr/0068-rendering-order-passes-by-what-they-read.md)

## Layout transitions

Every layout transition is a synchronization2 barrier built by a named function in
[`vulkan/memory/Barriers.h`](../../../api/render/realtime/vulkan/memory/Barriers.h). Each function
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
read it. A target with no colour image has only its depth moved. Its depth image is moved if any
pass of the frame that writes the target uses depth, whichever passes are first and last.

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
in [api/rendering/FramesAndTargets.md](../../api/rendering/FramesAndTargets.md#reading-a-frame-back). Inside:

- `record()` takes a `Capture::Source` (image, extent, format, layout, depth flag), so one code
  path reads a swapchain image or a target. The swapchain overload fills one in from an acquired
  image in `PRESENT_SRC_KHR`. That is the only point a chain image may be read: after recording,
  before present, while acquired. It throws when the chain is not `copyable()` or the index is
  outside the chain.
- The image goes to `TRANSFER_SRC` and back to the layout it arrived in. **The barrier back
  differs by destination.** Returning a chain image to `PRESENT_SRC_KHR` needs nothing made
  visible, because the semaphore presentation waits on orders it. Returning a target to a layout
  a later command in the same submit may sample or draw into has no such semaphore, so the
  transition must be complete and visible before them.
- Target colour images and sampled depth images carry `TRANSFER_SRC` usage so they can be copied.
- `convert()` swaps BGRA to RGBA and writes alpha opaque. It needs no device and is unit tested.
- A depth source must be `D32_SFLOAT`, since its copy is exactly the float a test compares; any
  other depth format throws.

Background: [ADR-0054](../../adr/0054-testing-golden-images-hold-only-spec-exact-output.md)
