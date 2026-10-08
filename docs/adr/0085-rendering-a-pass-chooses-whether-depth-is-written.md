# ADR-0085: Rendering: a pass chooses whether depth is written

**Status**: accepted
**Date**: 2026-10-07
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md),
[api/rendering/FramesAndTargets.md](../api/rendering/FramesAndTargets.md)

## Context

A pipeline fixes whether it writes depth when it is built, and the line and world quad
renderers are each one shared instance per context. Two uses of the same line renderer need
opposite answers. An overlay of translucent lines that share edges must test depth without
writing it, or the first line on an edge hides the second and draw order decides the colour. A
wireframe editor must write it, so a near wireframe hides a far one. Depth bias already reaches a
pipeline as dynamic state set by the pass, and Vulkan 1.3 has depth write enable in core.

## Decision

Whether depth is written becomes dynamic state on the pipelines that opt into it, and a pass may
name a value for it. A pass that names nothing draws each pipeline as it was built. The line and
world quad pipelines opt in, and the lit pipelines do not, because a lit surface is opaque and
always writes.

## Alternatives

### A flag on the line renderer's constructor
- **For**: one parameter, with no change to passes, the recorder or pipeline state.
- **Against**: the renderer is shared by every app on a context, so the flag is chosen once for
  all of them, and an app that needs both behaviours in one frame cannot have them.
- **Rejected because**: the choice belongs to what is being drawn, not to the context.

### A choice on each submit
- **For**: the finest control, since two submits into one pass can differ.
- **Against**: each renderer needs a second pipeline or its own dynamic state handling, and a
  pass then mixes writing and non-writing draws whose order matters.
- **Rejected because**: a pass is already the unit that chooses depth testing and depth bias,
  and no use needs the two behaviours inside one pass.

### Change the line renderer's default to not write
- **For**: the overlay case works with no new state.
- **Against**: the editor draws only wireframes, and its near lines would stop hiding far ones.
- **Rejected because**: it fixes one consumer by breaking the other.

## Consequences

- **Gains**:
  - One shared renderer serves both an overlay and a wireframe view, in the same frame if
    needed.
  - Every existing pass draws as before, because a pass that names nothing changes nothing.
  - It follows the pattern depth bias set, so the recorder has one way to apply per-pass state.
- **Costs**:
  - The recorder sets the state every time it binds an opted-in pipeline, since validation
    rejects a draw that declares the state and never sets it.
  - A pass's choice reaches only the pipelines that opted in, which a reader of the pass cannot
    see from the pass alone.
- **Revisit when**: a renderer needs writing and not writing within one pass, or a pipeline
  that did not opt in needs the pass to turn its writes off.
