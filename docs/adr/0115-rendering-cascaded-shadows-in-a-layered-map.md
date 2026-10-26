# ADR-0115: Rendering: cascaded shadows in a layered map

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: [api/rendering/Lighting.md](../api/rendering/Lighting.md)

## Context

One shadow map fitted to a long view spreads its texels over all of it, so a near shadow loses
its detail. Cascades split the view by distance and give each slice a map of its own, but the
lit shaders read one map through one matrix, and the shadow pass draws through that matrix.
Each cascade's draws need their own matrix and their own target, and a fragment has to find its
cascade. The worlds the games grow into are long and outdoor, where the near slice holds a small
part of what the far one does.

## Decision

The shadow map is a depth target with a layer per cascade, up to four, read as a
`sampler2DArray`. Each cascade is drawn by a pass of its own into its layer, and each shadow draw
pushes the cascade's index, which picks its matrix from the scene block. The default splits are
the practical split scheme, a lambda blending logarithmic and uniform splits, and a game may give
its own.

## Alternatives

### An atlas: the cascades as tiles of one 2D map
- **For**: today's single-layer target serves, each tile can have its own size, and spot light
  shadows could share the atlas later.
- **Against**: filtering at a tile's edge reaches into the next tile unless every lookup is
  clamped to it, and one image rules out drawing every cascade in one pass with multiview.
- **Rejected because**: separate layers cannot bleed into each other, and keep multiview open.

### A scene set per cascade
- **For**: no push constant, and a shadow stage reads its matrix as it did.
- **Against**: a uniform buffer and a descriptor set for each cascade every frame, and the lit
  pass still needs every matrix in its own set.
- **Rejected because**: one set and an index carry the same with less to keep in step.

### Multiview: one pass, the view index picking the matrix
- **For**: every caster is drawn once, and the device repeats it into each layer.
- **Against**: every caster reaches every cascade, so none can be left out of the near one, and
  the renderer has no multiview yet.
- **Rejected because**: a cascade culled to its own casters costs less in the large views that
  cascades are for. The layered map keeps multiview open if a scene's casters are few.

### Geometric splits from a first bound, or explicit distances only
- **For**: a geometric scheme is one parameter fewer. Explicit distances leave the choice wholly
  to the game.
- **Against**: geometric splits are one end of the practical scheme, and a game with only
  explicit distances writes the same arithmetic for every view.
- **Rejected because**: the lambda covers geometric, uniform and every blend between, and the
  fit takes explicit distances anyway.

## Consequences

- **Gains**: near shadows keep their detail over a long view, each cascade's pass draws only the
  casters that reach it and is timed on its own, and one map is a single cascade, so a game
  fitting one map is unchanged.
- **Costs**: a pass and a set of draws per cascade, each caster's object added to the frame once
  per cascade it reaches. Every layer has the same size. A replacement shader declares the
  push constant and reads the map as an array. A skinned caster is never culled, because only
  its bind pose is bounded. Past the last split there is no shadow.
- **Revisit when**: a scene draws so many small casters that one multiview pass would cost less
  than per-cascade passes, or a cascade needs a resolution of its own.
