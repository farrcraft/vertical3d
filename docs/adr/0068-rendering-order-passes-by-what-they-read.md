# ADR-0068: Rendering: order passes by what they read

**Status**: accepted
**Date**: 2026-10-03
**Amends**: [ADR-0031](0031-rendering-passes-draw-into-offscreen-targets.md)
**Documented in**: [api/Rendering.md](../api/Rendering.md), [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

[ADR-0031](0031-rendering-passes-draw-into-offscreen-targets.md) gives each pass a target of one
image, and records passes in the order they were created. A shadow pass, a lit pass and a chain
of post passes depend on each other, and the engine creates its own colour pass before an app
adds any. A pass recorded in the wrong order reads stale contents and nothing reports it. One
image also cannot be read as last frame's result and written as this frame's, so a pass cannot
read what it drew the frame before.

## Decision

A pass declares the targets it reads with `Pass::reads()`, and the frame records every pass that
writes a target before every pass that reads it, otherwise keeping creation order; a cycle
throws. A `RenderTarget` holds either one image or one per frame in flight, and with more than
one a pass draws into `current()` and a reader may sample `previous()`.

## Alternatives

### Two targets, swapped by the caller each frame
- **For**: No change to `RenderTarget`. It is ADR-0031's own answer.
- **Against**: Every reader of last frame writes the same swap. The pass's target changes every
  frame, so the recorder sees two unrelated targets.
- **Rejected because**: It is the same images with the bookkeeping moved into every app.

### The order stays the caller's, set by placing one pass before another
- **For**: Nothing to compute, and whoever builds the frame knows the chain.
- **Against**: The order is spread across everyone who adds a pass, and the engine's colour pass
  exists before an app says anything. A wrong order reads stale contents silently.
- **Rejected because**: The recorder already finds a target's writers. Adding readers lets the
  frame place passes rather than only check them.

### Readers found from the draw items' materials
- **For**: No declaration to forget.
- **Against**: The scene set binds the shadow map outside any material, and `Resources` would
  need to map an image back to its target.
- **Rejected because**: It would miss a reader that already exists.

## Consequences

- **Gains**:
  - A pass can read what it drew last frame, which temporal effects and feedback buffers need.
  - A shadow map with one image per frame lets one frame's shadow pass overlap the previous
    frame's lit pass.
  - The order is stated once, where each pass is made, and the engine creating its pass first
    does not decide where an app's passes go.
- **Costs**:
  - A pass that forgets `reads()` is recorded in creation order. Synchronisation validation
    reports it only as a hazard.
  - A multi-image target costs memory per image. Its reader holds one handle per slot and
    chooses each frame, and creating it submits once and waits so every image starts readable.
  - The slot is chosen by the ring's frame index, so a frame built for one index and recorded
    under another draws into the wrong slot.
  - The recorder also checks a pipeline's formats against its pass, documented in the
    internals.
- **Revisit when**: passes need to depend on something other than a render target, such as a
  buffer written by one pass and read by another.
