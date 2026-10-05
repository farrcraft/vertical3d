# ADR-0064: Lighting: lit passes use the shared recorder

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

Amended by [ADR-0071](0071-skinning-joint-matrices-in-one-storage-buffer.md): the scene
set also holds the frame's joint palettes, at binding 2, and a lit item's push block names where
its palette starts.

## Context

[Milestone 4](../roadmap/completed/m4-LitScene.md) brings a lit mesh tier into the api. The only one that
exists is retcon's, which binds three descriptor sets: the camera, the material, and the scene
(a light, cel bands and a shadow map). `Recorder` binds set 0 and set 1 and nothing more, and
nothing in the tree records `vkCmdSetDepthBias`, so a biased shadow pipeline compiles and cannot
be drawn. retcon declined `Frame`, `Pass` and `DrawItem` for the first of those reasons and
hand-records every pass. [LitScene](../plans/completed/LitScene.md) is the plan that needs this settled,
and its survey is the evidence here.

## Decision

**A `Pass` may name a scene set, which the recorder binds at set 2 once for the pass, and a
depth bias, which it records whenever it binds a pipeline built with one.** A biased pipeline
drawn in a pass that names no bias is an error at record time. The lit tier's passes are
ordinary passes in a `Frame`, so a lit scene, its shadow and whatever a game draws with `Quad`,
`World` or `Line` are recorded by one recorder. This amends
[ADR-0008](0008-shaders-descriptor-sets-by-update-frequency.md): its per-frame set is split into the camera at
set 0 and the scene at set 2.

## Alternatives Considered

### Alternative 1: Move retcon's `SceneRenderer` as it is
- **Pros**: The least code, and retcon's reference capture is unchanged by construction.
- **Cons**: A second way to record a frame beside the recorder. Its pass order is a class rather
  than a frame, and nothing drawn by the api's other renderers can share one of its passes,
  which is why retcon draws its own canvas and debug lines.
- **Why not**: The api would carry a frame model only one game uses, and every later tier
  (skinning, particles) would have to choose between the two.

### Alternative 2: The scene data in set 0
- **Pros**: No recorder change. One set per frame, as ADR-0008 wrote it.
- **Cons**: Every pipeline in the tree declares set 0, so widening it changes every shader. A
  shadow map in set 0 is bound for the 2D passes that never sample it.
- **Why not**: It costs every pipeline for what only the lit ones read.

### Alternative 3: The scene data in push constants
- **Pros**: No set, no pool, no descriptor write.
- **Cons**: The light's matrix alone is 64 of the 128 bytes, and retcon's per-object data is 96.
  A shadow map cannot be a push constant at all.
- **Why not**: It does not fit, and half of it cannot.

### Alternative 4: A scene set and a bias on the pass, recorded by the recorder — **chosen**
- **Pros**: One bind per pass, the frequency the data changes at. The sort key does not change.
  Set 0 stays compatible across quad, line and lit pipelines within a pass, because
  compatibility runs from set 0 upwards. retcon's reason for leaving the frame model is gone.
- **Cons**: The recorder learns two more things a pass can carry. A pipeline has to say whether
  its layout declares a set 2 and whether it is biased.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- A lit pass, a shadow pass and a post pass are placed by the frame, as every other pass is.
- A depth bias left unset is reported rather than drawn with whatever was last recorded.

### Negative
- Two sets per frame-rate frequency where ADR-0008 had one. A reader has to know that set 2 is
  the scene and that only lit pipelines declare it.
- retcon adopts the frame model to adopt the tier, which is more than replacing its classes.

### Risks
- A second consumer of set 2 with different contents (a particle pass wanting a wind field, say)
  would share the slot by convention only. The escape hatch is that the set is the pass's, not
  the tier's: a pass names whichever set its pipelines declare at 2.
