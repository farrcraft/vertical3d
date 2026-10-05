# ADR-0051: Frames: in-flight ring separate from presenting

**Status**: accepted
**Date**: 2026-09-11
**Documented in**: [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

Recording frames ahead of the device needs a ring of frames in flight: a command pool, a
command buffer and a fence for each. Presenting needs a swapchain, a semaphore per frame for
image acquisition and a semaphore per image for render completion. The renderers and
`FrameUniforms` use only the ring: they size and index their per-frame buffers by it and wait
on its fence before rewriting a slot. A context with no window, as render tests use
([ADR-0007](0007-ci-render-tests-on-software-vulkan.md)), can select a device and draw into an
offscreen target ([ADR-0031](0031-rendering-passes-draw-into-offscreen-targets.md)), but has
no swapchain to present to.

## Decision

The ring is `vulkan::frame::Ring`, which needs a device and nothing else. `Presenter` holds a
`Ring` and adds the swapchain, the semaphores and the acquire and present loop. Renderers and
`FrameUniforms` take a `Ring`, so a context with no window can build every renderer.

## Alternatives

### The presenter's swapchain is optional
- **For**: The smallest change, and it matches how a device may be created without a surface.
- **Against**: A class named `Presenter` would, in one of its two modes, not present. The two
  jobs would stay in one class, separated by a null check. A device without a surface is still
  a device; presenting is not what a device is for.
- **Rejected because**: The class name would describe only one of its modes.

### Renderers take an abstract interface that the presenter implements
- **For**: `Presenter` is untouched, so the present path carries no risk. A headless
  implementation sits beside it.
- **Against**: It adds virtual dispatch to `frame()`, which is called per batch per frame. It
  also creates a second implementation of frame pacing, which has to stay in step with the
  first and can drift silently.
- **Rejected because**: It trades a one-off risk on the present path for permanent duplication
  on the pacing path.

### Render tests cover only what needs no renderer
- **For**: No change. A clearing pass into a target, captured with the validation layer silent,
  already covers device selection, recording, layout transitions and barriers.
- **Against**: The quad, line and world renderers, the largest untested code in the renderer,
  would stay untested.
- **Rejected because**: It gives up the coverage most worth having.

## Consequences

- **Gains**:
  - A headless context builds every renderer, so render tests can draw real content.
  - A renderer depends on the ring it uses, not on a presenter it never calls.
  - `Presenter`'s public interface is unchanged.
- **Costs**:
  - The per-frame fence is created and waited on by `Ring` and signalled by `Presenter`'s
    submit. One responsibility is split across two classes, and a reader has to be told so.
  - The ring is on every frame's path. A mistake in the fence or command-buffer rotation is a
    synchronisation bug, not a compile error; synchronisation validation detects that class
    of fault.
  - A frame driven without `Ring::begin()` never waits and never collects retired objects.
    Every driver of the ring has to begin through it.
- **Revisit when**: the split proves wrong. `Ring` can fold back into `Presenter` without
  changing any caller of `acquire()` or `present()`.
