# ADR-0008: Shaders: descriptor sets by update frequency

**Status**: amended
**Date**: 2026-08-31
**Amended by**: [ADR-0064](0064-lighting-lit-passes-use-the-shared-recorder.md)
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md)

## Context

A draw item names its pipeline and material by handle
([ADR-0004](0004-rendering-submit-draw-items-as-data.md)), but how data reaches a shader is still
open. Where each piece of data is bound decides the sort key's field order, what a material
owns, and whether two adjacent items can be merged. Every pipeline layout and every shader
encodes the answer, so changing it later means rewriting all of them at once.

## Decision

Descriptor sets are grouped by how often their contents change. Set 0 holds per-frame data such
as the camera and is bound once by the pass; set 1 holds a material, such as its texture; data
that changes per object, such as a transform or tint, goes in push constants rather than a third
set. The sort key is ordered to match: layer, pipeline, material, depth.

## Alternatives

### One set per draw, holding everything
- **For**: the simplest to write. One layout, no thought about which frequency a value belongs
  to, and one uniform block in every shader.
- **Against**: camera data is copied into every draw's set, so descriptor writes grow with draw
  count. No two draws share a set, so nothing can be merged.
- **Rejected because**: it defeats the reason draw items are data. The engine would sort a frame
  it could never merge, and the quad batching of
  [ADR-0005](0005-2d-one-batched-quad-pipeline.md) would gain nothing.

### Three sets: per frame, per material, per object
- **For**: uniform, since everything is a descriptor set. Per-object data is not limited by the
  push constant budget and can live in one large buffer with a dynamic offset, a common pattern.
- **Against**: a third bind per draw, a descriptor pool sized by object count, and per-frame
  descriptor writes or offset bookkeeping for a few bytes of data.
- **Rejected because**: the per-object data here is a transform and a tint. A descriptor set is
  the expensive way to move 80 bytes.

## Consequences

- **Gains**:
  - Whether two items can merge is decided from the items alone: the same pipeline and material,
    adjacent after sorting.
  - A texture atlas is a material, so a batch break is a material change, with no special case.
  - A second viewport is a pass with its own set 0.
- **Costs**:
  - Push constants are guaranteed only 128 bytes, which caps per-object data. A 4x4 transform
    and a tint take 80 of them.
  - Every shader has to agree on the set numbers, and nothing checks it at compile time. A
    mismatch shows as a validation error or wrong uniforms.
  - A value whose frequency falls between per frame and per material has no natural home and
    ends up duplicated across materials in set 1.
- **Revisit when**: per-object data outgrows the push constant budget. The first step is then a
  push constant holding an index into a storage buffer, which leaves sets 0 and 1 unchanged.
