# ADR-0050: Frame Capture — A Presented Frame Is Read Back In Two Calls, And Nothing In The Tree Compares It

**Date**: 2026-09-10
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0007](0007-ci-rendering-tests.md) settled that render tests assert the absence of
validation errors rather than comparing pixels, and its fifth alternative deferred
golden-image comparison rather than rejecting it: *"worth revisiting once there is stable
output worth pinning"*. Nothing in the tree can read a frame back, so revisiting it has
never been possible — the decision has been resting on an unmeasured claim.

A consumer of the api already has the missing piece. retcon drives a reference capture as
its renderer's regression net: it writes the presented frame to a png, and a masked pixel
diff against a blessed reference is what every change to its renderer is proved with. That
code is a swapchain copy, a host visible buffer and a channel swizzle, none of which is
specific to the app that wrote it.

## Decision

`vulkan::frame::Capture` copies an acquired swapchain image into a host visible buffer and
writes it as a png, in two calls — `record()` into the command buffer the frame is already
being drawn into, and `write()` once the submit carrying it has completed. Nothing in this
repository calls it; supplying the mechanism is not the same as adopting golden images in
CI, and ADR-0007 stands until something blesses a reference.

## Alternatives Considered

### Alternative 1: Two calls, with the caller ordering them — **chosen**
- **Pros**: A queue submit sits between the copy and the read, and the caller already owns
  what orders it — a fence it waits on, or a device wait before shutdown. The class makes no
  claim about how a frame is synchronised, which is the one thing a renderer varies.
  `record()` costs a caller that never captures nothing at all, because the readback
  allocation is made by the first one.
- **Cons**: Two calls to get one file, and a caller that forgets the second gets no
  diagnostic — the copy simply runs and is thrown away.
- **Why not**: n/a — chosen.

### Alternative 2: One blocking `capture(path)` that waits for the device itself
- **Pros**: One call. Impossible to get the order wrong.
- **Cons**: The wait has to be a `vkDeviceWaitIdle`, because the class does not own the
  fence the frame was submitted with. That is correct once per process for a screenshot and
  ruinous for anything capturing a sequence, and it forces the cost on the caller whose
  presenter already has exactly the fence needed.
- **Why not**: It builds the heaviest possible synchronisation into the only thing the
  caller could have done better.

### Alternative 3: Capture owns its own command buffer and submits it
- **Pros**: Self-contained: no precondition on a layout, and no borrowed command buffer.
- **Cons**: A swapchain image may only be read while it is acquired, and the acquisition
  belongs to the presenter. A second submit would have to be ordered against the first with
  semaphores the class does not own, which is the frame's business, not the capture's.
- **Why not**: It would have to re-implement the presenter to avoid depending on it.

### Alternative 4: Leave it in the consuming app
- **Pros**: No addition here. The app has it working.
- **Cons**: ADR-0007's deferred alternative stays undecidable, and every later consumer that
  wants a reference frame writes the same barrier pair against the same chain.
- **Why not**: The barriers around a swapchain readback are exactly the kind of thing that
  is written once and read by validation, not the kind of thing every app should own.

## Consequences

### Positive
- ADR-0007's fifth alternative becomes a decision that can be taken on its merits rather
  than on what exists.
- The channel order is device free and tested: a chain is commonly BGRA, a png is RGBA, and
  which byte is red does not depend on the transfer function the format carries.
- A captured frame presents exactly as an uncaptured one — the image is handed back in the
  layout it arrived in — so a capture is not itself a rendering change.

### Negative
- A facility with no consumer in this tree, which is a shape that goes wrong in small ways
  until an application finds them.
- `memory::Buffer` grew a read path. Its allocation is coherent rather than cached, so
  reading one back is slow; that is the right trade for a buffer filled once and read once
  and the wrong one for anything doing it per frame, and the header says so.

### Risks
- **A caller records a capture and never writes it.** The copy is recorded and discarded,
  costing a frame's worth of bandwidth silently. Mitigated only by the docblock: a facility
  that diagnosed it would have to know when the frame ended, which is what alternative 3
  rejected.
