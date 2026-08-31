# ADR-0004: Render Submission — Operations Are Draw Data, Not Draw Code

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`Operation` declares `bool run(shared_ptr<Context>)`, and `Frame::draw()` does nothing but
iterate the list calling it; each operation issues its own draw calls. Moving to Vulkan under
[ADR-0001](0001-vulkan-replaces-opengl.md) forces a new contract, and the existing one is
already used at two incompatible granularities — pong creates one operation for an entire
batched canvas, odyssey creates one per sprite with a heap allocation each frame. The frame
loop cannot be written without settling which of those the model is.

## Decision

An operation becomes a description of work submitted to a queue rather than something that
draws itself. The engine owns sorting, merging and recording. The contract is adopted now;
the first implementation records items in submission order with no sorting and no merging.

## Alternatives Considered

### Alternative 1: Operations are data, the engine records — **chosen**
- **Pros**: Batching and state-change minimisation happen centrally, so they improve
  everywhere at once. App code never touches Vulkan. Opens the way to sorting, instancing
  and GPU-driven work without app changes.
- **Cons**: The largest rewrite of the three. Needs a sort key, and needs stable resource
  handles before sorting means anything. Sorting breaks painter ordering unless 2D
  submissions carry an explicit layer.
- **Why not**: n/a — chosen.

### Alternative 2: Operations record directly into a command buffer
- **Pros**: The smallest step from the current code — each operation records instead of
  calling GL. Maximum flexibility, and ordering is trivially correct because record order is
  submission order.
- **Cons**: No batching across operations, so odyssey stays at one draw per sprite. Every
  operation has to know about pipelines and descriptor sets, spreading Vulkan outward into
  app-adjacent code.
- **Why not**: It optimises for the workload that exists rather than the one being built
  toward, which sits badly beside [ADR-0001](0001-vulkan-replaces-opengl.md) and
  [ADR-0003](0003-one-realtime-engine.md), both taken for capability rather than to reduce
  near-term work. Voxel is the named next consumer and is the app with real geometry and
  materials, which is exactly the workload a draw stream exists for.

### Alternative 3: A batcher for the common cases, with operations as an escape hatch
- **Pros**: Matches what pong already does, since `Canvas` is a batcher and
  `operation::Canvas` draws the batch. Fixes the granularity conflict with the least work.
- **Why not**: The same objection as alternative 2, more mildly. It weighs current shape
  over future capability.

## Consequences

### Positive
- The contract is the expensive thing to change and it is settled early; the implementation
  behind it is invisible to app code and can grow at leisure.
- The per-sprite heap allocation in odyssey disappears with the granularity change.
- The primitive in [ADR-0005](0005-one-batched-quad-primitive.md) gets simpler: flushing on
  texture change becomes what the merge step does naturally to items in layer order.

### Negative
- Stable, comparable resource handles — a pipeline cache, a material or descriptor registry,
  texture handles — become a prerequisite rather than a refinement. This is the part of the
  decision most likely to be underestimated.
- `Operation` becomes a misleading name for a passive description, and should be renamed
  while there are only a handful of implementations.
- Debugging gains a step: a missing draw may be a submission problem or a sort problem.

### Risks
- 2D submissions must carry an explicit layer field from the first version, even though
  nothing sorts yet. Adding it later means auditing every call site, and the symptom of
  getting it wrong is invisible or misordered sprites.
- Where the data model cannot express something, the escape hatch is a custom item carrying
  a record callback. That is part of the design, but reaching for it constantly is the
  signal that the model is wrong.
