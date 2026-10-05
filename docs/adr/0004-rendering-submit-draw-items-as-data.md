# ADR-0004: Rendering: submit draw items as data

**Status**: accepted
**Date**: 2026-08-30
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md)

## Context

The move to Vulkan ([ADR-0001](0001-rendering-replace-opengl-with-vulkan.md)) needs a new contract
between what an app submits and what the engine records. Under the old contract each operation
drew itself, and apps used it at two incompatible sizes: one operation for a whole batched
canvas, or one heap-allocated operation per sprite per frame. Batching, sorting and
state-change reduction can only be done centrally if the engine sees the work before it is
recorded. The frame loop cannot be written until the contract is settled.

## Decision

A draw is a `DrawItem`: a description of work submitted to a pass, not code that draws itself.
The engine owns sorting, merging and recording, and app code does not touch a command buffer.
2D submissions carry an explicit layer from the start, so painter order survives any later
sorting.

## Alternatives

### Operations record directly into a command buffer
- **For**: the smallest step from the old code. Each operation records instead of calling
  OpenGL, and order is correct because record order is submission order.
- **Against**: no batching across operations, so a sprite-heavy app stays at one draw per
  sprite. Every operation has to know about pipelines and descriptor sets, which spreads
  Vulkan into code next to the apps.
- **Rejected because**: it suits the workload that existed, not the one being built toward.
  Apps with real geometry and materials are the workload a draw stream exists for.

### A batcher for the common cases, with operations as an escape hatch
- **For**: matches what the batched canvas already did, and fixes the size mismatch with the
  least work.
- **Against**: anything outside the batcher still records itself, with the same costs as the
  previous option.
- **Rejected because**: it weighs the current code over future capability, for the same reason
  as the previous option.

## Consequences

- **Gains**:
  - The contract, which is the expensive thing to change, is settled early. What happens
    behind it can grow without app changes.
  - Batching and state-change reduction improve everywhere at once.
  - The per-sprite heap allocation disappears.
- **Costs**:
  - Stable, comparable resource handles for pipelines, materials and textures become a
    prerequisite, because a sort key built from them has to mean something.
  - Debugging has another step: a missing draw may be a submission problem or an ordering
    problem.
  - Work the item's fields cannot describe goes through a record callback on the item, which
    bypasses the engine's merging.
- **Revisit when**: the record callback is needed routinely. That means the data model is
  missing something.
