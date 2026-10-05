# ADR-0031: Rendering: passes draw into offscreen targets

**Status**: amended
**Date**: 2026-09-06
**Amended by**: [ADR-0068](0068-rendering-order-passes-by-what-they-read.md)
**Documented in**: [api/Rendering.md](../api/Rendering.md), [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

Every pass draws into the swapchain image, which rules out any rendering done in more than one
step. A shadow map is a depth image written by one pass and sampled by a later one. A colour
grade reads a scene rendered somewhere else, and a second view of a scene is a smaller image
drawn before the one it appears in. An app cannot build these from outside, because the recorder
owns the walk over the passes and a draw item's record callback covers one draw, not a pass with
its own attachments and layout transitions. Drawing already uses dynamic rendering, so an
attachment is any image view named when the pass begins.

## Decision

A pass may name a `frame::RenderTarget` and draws into the swapchain image when it names none.
The recorder moves a target's images into an attachment layout before the first pass that draws
into it and into a readable layout after the last one, so every later pass can sample it as an
ordinary texture. A target's depth can be sampled too, as a shadow map needs, and the target is
told so at creation because a device may allow a depth format as an attachment but not for
sampling.

## Alternatives

### A target handle registered in `pipeline::Resources`
- **For**: `Pass` would name a handle and stay free of Vulkan types, matching how a draw item
  names everything else.
- **Against**: registered things are built once and live for many frames. A target's images
  are discarded and rebuilt whenever its size changes, so its handle would go stale on every
  resize.
- **Rejected because**: the lifetimes do not match. Handles are for things that outlive frames.

### A second engine, or a second recorder, for offscreen work
- **For**: the swapchain path stays exactly as it is.
- **Against**: two recorders means two places that bind materials and skip redundant binds.
- **Rejected because**: [ADR-0003](0003-rendering-one-engine-for-2d-and-3d.md) already settled that
  there is one engine and the pass is what varies.

### One image per frame in flight in every target
- **For**: a frame never writes the image the previous frame is still sampling.
- **Against**: three images, views, samplers and descriptor sets per target, and an app sampling
  one has to know which slot the frame is in.
- **Rejected because**: a barrier orders that hazard. The transition into the attachment layout
  waits on fragment shader work already submitted, so this frame's writes follow the previous
  frame's reads.

## Consequences

- **Gains**:
  - Shadow maps, post-processing and second views are possible, with no change to how a draw is
    submitted. A frame stays one command buffer and one submission.
  - A pass into a target gets the target's extent as its default viewport.
  - What a pass rendered can be drawn as a quad's texture, so compositing needs no new
    primitive.
- **Costs**:
  - `Pass` holds a pointer to a Vulkan object, and the recorder tracks the first and last use of
    each target across the frame.
  - A pipeline is built against the formats it draws into, so a target of a different format
    needs its own pipeline.
  - A target's size is the caller's. A target that should follow the window is recreated by the
    app on resize, and anything registered against the old images has to be registered again.
  - A pass that samples a target's depth while also drawing into it is a hazard the recorder
    does not detect; only the validation layer reports it.
- **Revisit when**: a pass needs what a target held in the previous frame, which a single image
  cannot hold while it is being drawn again.
