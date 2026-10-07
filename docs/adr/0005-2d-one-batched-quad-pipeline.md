# ADR-0005: 2D: one batched quad pipeline

**Status**: amended
**Date**: 2026-08-30
**Amended by**: [ADR-0036](0036-text-sdf-glyphs-through-the-quad-shader.md), [ADR-0042](0042-rendering-world-space-sprites.md)
**Documented in**: [api/rendering/Canvas.md](../api/rendering/Canvas.md)

## Context

The 2D canvas being replaced batched coloured quads only. Its vertices had no texture
coordinates, so tile sprites, piece sprites and all text were unserved. On Vulkan, textured and
untextured quads either share one path or get two. The answer fixes the vertex format, the
pipeline and the batcher that feeds draw items
([ADR-0004](0004-rendering-submit-draw-items-as-data.md)).

## Decision

All 2D content in canvas pixels is one primitive: a quad with one vertex format (position,
colour, texture coordinates) drawn through one pipeline. An untextured quad samples a 1x1 white
texture, so it needs no separate path. The canvas starts a new batch whenever the bound texture
changes, which keeps painter order without sorting.

## Alternatives

### Separate coloured and textured paths
- **For**: each path is simpler on its own, and the coloured one matches the old canvas, so
  porting is more mechanical. No unused vertex attributes. The coloured path could ship first.
- **Against**: two pipelines, two vertex layouts and two upload paths. Order between the two
  kinds of batch has to be managed by interleaving draws.
- **Rejected because**: text is textured quads and the first app needs text in its first frame,
  so the textured path has to be built anyway. Two near-identical families of draw are also the
  two-engine split of [ADR-0003](0003-rendering-one-engine-for-2d-and-3d.md) one layer down.

### Bindless: a texture array with a texture index per vertex
- **For**: one draw per canvas however many textures it uses, with no atlas and no batch breaks.
- **Against**: needs descriptor indexing, feature queries, and a decision about devices that
  lack it.
- **Rejected because**: deferred rather than ruled out. [ADR-0002](0002-vulkan-require-version-1-3.md)
  makes descriptor indexing available, so this is a question of complexity, worth taking on when
  measurement shows draw counts matter.

## Consequences

- **Gains**:
  - One code path serves every 2D app, and text with them.
  - Painter order holds with no sort key and no layer comparison in the batcher.
  - Moving to bindless later adds a vertex attribute rather than changing what the format means.
- **Costs**:
  - The engine owns a 1x1 white texture for the life of the context.
  - Untextured quads carry eight bytes of unused texture coordinates per vertex. This is
    negligible at 2D quad counts.
  - Draw count depends on how often the texture changes, so a texture atlas is required rather
    than optional. Drawing many small textures without one breaks the batch on nearly every quad.
- **Revisit when**: draw counts from texture changes show up in profiles, which is the case for
  bindless.
