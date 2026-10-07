# ADR-0064: Lighting: lit passes use the shared recorder

**Status**: amended
**Date**: 2026-10-03
**Amends**: [ADR-0008](0008-shaders-descriptor-sets-by-update-frequency.md)
**Amended by**: [ADR-0071](0071-skinning-joint-matrices-in-one-storage-buffer.md)
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md)

## Context

A lit mesh renderer binds three descriptor sets: the camera, the material, and the scene (a
light, cel bands and a shadow map). A shadow pipeline also needs a depth bias recorded with
`vkCmdSetDepthBias` before it draws. [ADR-0008](0008-shaders-descriptor-sets-by-update-frequency.md)
puts per-frame data in set 0, which every pipeline in the tree declares. A renderer whose
needs the shared recorder cannot meet records its own passes outside `Frame` and `Pass`, and
nothing else can draw into them.

## Decision

A `Pass` may name a scene set, which the recorder binds at set 2 once per pass. It may also name a
depth bias, which the recorder records whenever it binds a pipeline built with one. A biased
pipeline in a pass that names no bias is an error at record time. Lit passes are ordinary passes in
a `Frame`, recorded by the same recorder as quad, world and line passes. ADR-0008's per-frame set
is split into the camera at set 0 and the scene at set 2.

## Alternatives

### A separate lit renderer that records its own passes
- **For**: The least code, and a working lit renderer could be moved in unchanged.
- **Against**: A second way to record a frame beside the recorder. Its pass order is fixed in a
  class rather than in a frame, and nothing drawn by the api's other renderers can share its
  passes.
- **Rejected because**: The api would carry a frame model that only lit scenes use, and every
  later renderer would have to choose between the two.

### The scene data in set 0
- **For**: No recorder change, and one per-frame set as ADR-0008 has it.
- **Against**: Every pipeline declares set 0, so widening it changes every shader, and the 2D
  passes would bind a shadow map they never sample.
- **Rejected because**: It costs every pipeline for data only the lit ones read.

### The scene data in push constants
- **For**: No set, no pool and no descriptor write.
- **Against**: Push constants are 128 bytes. The light's matrix alone is 64, and the per-object
  data already takes most of the block. A shadow map cannot be a push constant at all.
- **Rejected because**: It does not fit.

## Consequences

- **Gains**:
  - A lit pass, a shadow pass and a post pass are ordered by the frame like every other pass,
    and 2D content can draw into them.
  - The scene set is bound once per pass, the rate it changes at, and the sort key is
    unchanged. Set 0 stays compatible across quad, line and lit pipelines in one pass.
  - A depth bias left unset is reported rather than drawn with whatever was last recorded.
- **Costs**:
  - The recorder handles two more pass properties, and a pipeline has to state whether it
    declares set 2 and whether it is biased.
  - A reader has to know that set 2 is the scene and that only lit pipelines declare it.
  - A second user of set 2 with different contents shares the slot by convention only.
- **Revisit when**: a non-lit pass needs different data at set 2. The set belongs to the pass,
  so a pass can name whatever set its pipelines declare there.
