# ADR-0008: Shader Bindings — Descriptor Sets By Update Frequency, Per-Object Data In Push Constants

**Date**: 2026-08-31
**Status**: proposed
**Deciders**: Joshua Farr

## Context

[ADR-0004](0004-operations-as-draw-data.md) made a draw item a description that names its
pipeline and material by handle, and left how those are actually bound open. The phase 2
frame loop cannot stay open on it: the sort key's field order, what a material owns, and
whether two adjacent items can be merged all follow from where a piece of data is bound.
Every pipeline layout built from here on encodes the answer, and changing it later means
rewriting every shader and every pipeline layout at once.

## Decision

Descriptor sets are organised by how often their contents change: **set 0 per frame** —
camera, projection, viewport, time — bound once by the pass; **set 1 per material** — the
sampled texture and anything else the material needs. Anything that changes per object —
transform and tint — goes in **push constants**, not in a third set. The draw item's sort
key is ordered to match: layer, pipeline, material, depth.

## Alternatives Considered

### Alternative 1: Two sets by frequency plus push constants — **chosen**
- **Pros**: Two sets bound per draw at most, and merging adjacent items is a comparison of
  two handles. Push constants need no allocation, no pool and no descriptor write, which
  suits transform and tint exactly — small, different every draw. The sort key's fields fall
  out of the same order, so sorting groups precisely what can be merged.
- **Cons**: Push constants are a small guaranteed budget — 128 bytes on the floor of what
  Vulkan requires — so anything per-object that does not fit has to become a material or a
  buffer indexed by push constant. Fixes the set numbering across every shader in the tree.
- **Why not**: n/a — chosen.

### Alternative 2: One set per draw, holding everything
- **Pros**: The simplest thing to write first. One layout, no thinking about which frequency
  a value belongs to, and the shaders read one uniform block.
- **Cons**: Camera data is duplicated into every draw's set, so a frame allocates and writes
  descriptors in proportion to draw count. Nothing can be merged, because no two draws share
  a set. It is the shape that makes the batching in
  [ADR-0005](0005-one-batched-quad-primitive.md) pointless.
- **Why not**: It defeats the reason draw items are data. The engine would sort a frame it
  can never merge.

### Alternative 3: Three sets — per frame, per material, per object
- **Pros**: Uniform: everything is a descriptor set, and per-object data is not limited to
  the push constant budget. Object data can live in one large buffer with a dynamic offset,
  which is a well-trodden pattern.
- **Cons**: A third bind per draw, a descriptor pool sized by object count, and per-frame
  descriptor writes or dynamic-offset bookkeeping for data that is a handful of bytes.
- **Why not**: The per-object data this repo actually has is a transform and a tint. Paying a
  descriptor set for that is the expensive way to move 80 bytes. This is the alternative to
  revisit if per-object data grows — the escape hatch is a push constant holding an index
  into a storage buffer, which changes neither set 0 nor set 1.

## Consequences

### Positive
- Merging is decidable from the draw item alone: same pipeline, same material, adjacent
  after sorting, therefore mergeable.
- The batched quad of [ADR-0005](0005-one-batched-quad-primitive.md) fits without special
  casing — the atlas is a material, and a flush is a material change.
- An editor viewport is a pass with its own set 0, which is what
  [ADR-0003](0003-one-realtime-engine.md) needs multiple viewports to be.

### Negative
- The 128 byte push constant floor is a real ceiling on per-object data. A 4x4 transform and
  an rgba tint is 80 bytes, leaving little room; a second matrix does not fit.
- Every shader in the tree has to agree on the set numbers, including ones not yet written.
  There is no compiler check for getting it wrong — the symptom is a validation error or
  garbage uniforms.
- A value whose frequency is genuinely between per-frame and per-material has nowhere
  natural to go and will be pushed into set 1, duplicating it across materials.

### Risks
- Guessing a value's frequency wrong is cheap to fix while there is one pipeline and
  expensive once there are several. Phase 3 builds the first one, and it is the point at
  which to check the choice against something real rather than against this document.
- Devices vary in maximum bound descriptor sets, but the floor is four and this uses two, so
  nothing here is at risk from a weak device.
