# ADR-0031: Offscreen Rendering — A Pass Draws Into A Target It Names, And The Recorder Leaves It Readable

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Every pass drew into the swapchain image. `Pass.h` said so outright — "only the swapchain
image is a valid target so far, so a pass has no target field yet" — and `Recorder::Target`
was the acquired image and the context's one depth buffer.

That is the wall in front of everything a renderer does in more than one step. A shadow map is
a depth image written by one pass and sampled by the next. A colour grade is a scene rendered
somewhere, then read back through a lookup. A second view of one scene — a mirror, a security
monitor, a thumbnail — is a smaller image drawn before the one it appears in. None of them can
be faked from outside: `Recorder` owns the pass walk, and `DrawItem::record` is an escape hatch
for one draw ([ADR-0004](0004-operations-as-draw-data.md)), not for a whole pass with its own
attachment and its own layout transitions.

The engine already had the pieces around the hole. Passes are a list rather than a single pass
precisely because "compositing, offscreen targets and an editor's several viewports are all
more passes over the same frame" ([ADR-0003](0003-one-realtime-engine.md)). Drawing is dynamic
rendering, so an attachment is a view named at `vkCmdBeginRendering` and not a
`VkFramebuffer` baked at load time. What was missing was somewhere for a pass to name, and
someone to move the image between the layout it is written in and the one it is read in.

## Decision

**A pass names an optional `vulkan::RenderTarget`, and draws into the swapchain image when it
names none.** A target owns a colour image created with sampled usage, its view, its sampler,
and optionally a depth image of its own size.

**The recorder makes a target readable.** It transitions a target into
`COLOR_ATTACHMENT_OPTIMAL` before the first pass of the frame that draws into it and into
`SHADER_READ_ONLY_OPTIMAL` after the last one, so two passes drawing into one target cost one
pair of barriers. Every pass recorded after that point can sample it.

**A target is sampled the way every other image is** — registered in `Resources` and named as
a material's texture — so nothing about reading one is special. What is registered names the
target's images rather than taking them over: `Texture::owned` is false, and `Resources` does
not free them.

**A target's size and format are the caller's.** It does not follow the swapchain, because a
shadow map is sized by the detail it needs; an app that wants one to track the window recreates
it when `Engine3D::beginFrame` reports a new size.

## Alternatives Considered

### Alternative 1: A pass holds a target, and the recorder transitions it — **chosen**
- **Pros**: The pass is already the unit of variation, so this is the field it was missing
  rather than a new concept. Nothing about submitting a draw changes. A frame stays one command
  buffer and one submission.
- **Cons**: `Pass`, which was Vulkan-free in its own body, now holds a pointer to a Vulkan
  object. The recorder has to reason about first and last use of each target across the frame.
- **Why not**: n/a — chosen.

### Alternative 2: A target handle into `Resources`, like a pipeline or a texture
- **Pros**: `Pass` would name a `TargetHandle` and stay free of Vulkan types; it matches how a
  draw item names everything else.
- **Cons**: `Resources` never frees an individual resource and never reuses a slot, on purpose —
  its things are built at load time and live until the context does. A target is thrown away and
  rebuilt whenever what it is sized against changes, so it would either leak on every resize or
  break the invariant that makes a handle's sort order meaningful.
- **Why not**: The lifetimes are opposite. Handles are for what outlives frames.

### Alternative 3: A second engine, or a second recorder, for offscreen work
- **Pros**: The swapchain path stays exactly as it was.
- **Cons**: [ADR-0003](0003-one-realtime-engine.md) settled that there is one engine and the
  pass is where drawing varies. Two recorders means two places that know how to bind a material
  and skip a rebind.
- **Why not**: It is the same decision ADR-0003 already made, asked again.

### Alternative 4: A target per frame in flight
- **Pros**: No write-after-read between a frame and the one still sampling the previous
  contents, which is the hazard a single image has with two frames in flight.
- **Cons**: Three images, three views, three samplers and three descriptor sets per target, and
  an app that samples one has to know which slot the frame is in.
- **Why not**: A barrier answers it instead. The transition into `COLOR_ATTACHMENT_OPTIMAL`
  names `FRAGMENT_SHADER` in its first scope, and a barrier's first scope reaches work already
  submitted to the queue, so this frame's writes are ordered after the previous frame's reads.
  Revisit if a target is ever wanted for something a barrier cannot order.

## Consequences

### Positive
- Shadow maps, post-processing and a second view are now possible app-side, over `Context3D`,
  the way the voxel app already builds its own pipeline and descriptor layout.
- A pass into a target gets that target's extent as its default viewport, so a smaller target is
  not drawn as though it were window sized.
- Registering a target as a texture means `Canvas` can composite one with no new primitive:
  what a pass rendered is a quad's texture.

### Negative
- A target is single-buffered and carries nothing from one frame to the next. A pass that wants
  the previous frame's contents needs two targets and has to swap them itself.
- The pipeline drawing into a target is built against that target's colour format, so a target
  of a different format needs its own pipeline. Nothing catches a mismatch at submission time.
- `Frame::passBefore` exists because `Engine3D` creates the colour pass in its constructor, so
  an app's offscreen pass would otherwise be recorded after the pass that samples it. Ordering
  is the caller's, and getting it wrong reads what is in the image rather than failing.

### Risks
- Reading a target in the same pass that writes it, or before the last pass that writes it, is
  not diagnosed. Synchronization validation catches it when it becomes a hazard, but a pass that
  merely reads stale contents is silent.
- `RenderTarget::recreate` allocates new images, so a `TextureHandle` registered for the old
  ones names images that no longer exist. Registering again after a resize is the caller's, and
  a stale handle is a use-after-free rather than a blank picture.
