# ADR-0005: 2D Drawing — One Batched Quad Primitive With An Optional Texture

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`v3d::gl::Canvas` batches coloured quads, but its vertices carry position and rgba only —
no texture coordinates — so every textured thing the apps need is unserved: tetris piece
sprites, odyssey tiles, and all text. Replacing it on Vulkan means deciding whether textured
and untextured quads share a path, and the answer shapes the vertex format, the pipeline and
the batcher that [ADR-0004](0004-operations-as-draw-data.md) submits into.

## Decision

One vertex format — position, colour, uv — and one pipeline for all 2D drawing. Untextured
quads sample a 1x1 opaque white texture, so `colour * texel = colour`, with no shader branch
and no second pipeline. A new draw is emitted whenever the bound texture changes, which
preserves painter ordering without sorting.

## Alternatives Considered

### Alternative 1: One primitive, batch broken on texture change — **chosen**
- **Pros**: One pipeline, one shader pair, one upload path. Ordering across mixed content is
  correct with no sorting. Text is the same primitive drawing from a glyph atlas. Needs no
  Vulkan feature beyond what is already in use.
- **Cons**: Every vertex carries a uv even when unused. Draw count depends on how often the
  bound texture changes, so it relies on an atlas to stay low.
- **Why not**: n/a — chosen.

### Alternative 2: Separate coloured and textured paths
- **Pros**: Each path is simpler alone, and the coloured one is exactly what `Canvas` is
  today, so pong's port is more mechanical. No unused vertex attributes. The coloured path
  could ship first and put something on screen sooner.
- **Cons**: Two pipelines, two vertex layouts, two upload paths. Ordering between the two
  batches has to be managed by interleaving draws, which reintroduces the complexity the
  split was meant to avoid.
- **Why not**: Text is textured quads and pong needs it in its first frame, so the textured
  path gets built regardless — the simpler-alone claim expires immediately. Two near
  identical operation families is also the `Engine2D`/`Engine3D` split one layer down, which
  [ADR-0003](0003-one-realtime-engine.md) just removed.

### Alternative 3: Bindless — a texture array with a per-vertex texture index
- **Pros**: One draw per frame regardless of how many textures are involved. No atlas
  needed, no flushing.
- **Cons**: Needs descriptor indexing, feature queries, and a fallback decision for devices
  that do not support it.
- **Why not**: Deferred, not rejected. [ADR-0002](0002-target-vulkan-1-3.md) makes
  descriptor indexing available, so this is now a question of complexity rather than
  capability — worth doing when measurement asks for it, not before.

## Consequences

### Positive
- One code path serves pong, tetris and odyssey, and text along with them.
- Painter ordering is preserved with no sort key and no layer comparison in the batcher.
- Upgrading to alternative 3 later means adding a texture-index attribute, not redesigning
  the vertex format's meaning.

### Negative
- The engine must own a 1x1 white texture as a permanent resource.
- Eight bytes a vertex are wasted on untextured quads. Irrelevant at 2D quad counts, and
  worth stating so nobody re-opens it as an optimisation.

### Risks
- `v3d::image::TextureAtlas` becomes load-bearing rather than an optimisation. Without it,
  tetris drawing a board of mixed piece colours breaks the batch on nearly every block —
  worst case one draw per block. Packing all seven piece textures into one atlas makes the
  board a single draw, and the same applies to odyssey's tiles. The atlas already exists and
  is proven by the font path, so the risk is forgetting it is required, not building it.
