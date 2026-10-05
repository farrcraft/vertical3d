# ADR-0036: Text: SDF glyphs through the quad shader

**Status**: accepted
**Date**: 2026-09-06
**Amends**: [ADR-0005](0005-2d-one-batched-quad-pipeline.md)
**Documented in**: [api/rendering/Canvas.md](../api/rendering/Canvas.md)

## Context

Coverage glyphs are rasterized at a fixed size, so every text size needs its own atlas, and text
drawn in canvas pixels is physically half as large on a 4K display as on 1080p. Signed distance
field (SDF) glyphs let one atlas serve every size, because size becomes an argument at the draw.
An SDF texel is a distance rather than a coverage, so the fragment shader has to threshold it to
get an alpha. The quad shader is shared by panels, sprites and glyphs, and applying that
threshold to everything would corrupt every quad that is not text. The quad primitive of
[ADR-0005](0005-2d-one-batched-quad-pipeline.md) was designed so its fragment shader needs no
branch.

## Decision

Glyphs are SDFs drawn through the same quad pipeline. Each batch carries a flag saying whether it
is text, the canvas never merges across a change in that flag, and the flag reaches the fragment
shader as a push constant. The shader branches on it once: text thresholds the sampled distance
into a smooth edge, and every other quad samples as before.

## Alternatives

### A second pipeline for text, chosen per batch
- **For**: no branch in the shader, as ADR-0005 intended. There is precedent: the quad renderer
  already picks a pipeline by whether the pass has depth.
- **Against**: the two choices multiply. Text or not, with depth or without, is four pipelines.
  A UI frame alternates panels and labels constantly, so it adds pipeline switches every frame.
- **Rejected because**: a pipeline bind is the expensive state change in a Vulkan frame. The
  branch it avoids costs almost nothing, because every fragment in a draw takes the same side.

### Keep coverage glyphs and build an atlas per size
- **For**: no shader change at all.
- **Against**: one atlas per size, each uploaded separately, with every size known in advance.
  It does nothing for display scaling: keeping text the same physical size across monitors still
  means rebuilding an atlas.
- **Rejected because**: apps were each hard-coding one font size around this, and a text size
  setting is something apps are expected to offer.

### Multi-channel distance fields (msdfgen)
- **For**: keeps sharp corners at large sizes, which single-channel fields soften.
- **Against**: a new dependency in the asset path, and a three-channel atlas in place of a simple
  single-channel upload.
- **Rejected because**: deferred rather than ruled out. No app draws text large enough for the
  softening to show, and the text flag is where a later switch would hook in.

## Consequences

- **Gains**:
  - One atlas serves every size, so the text size is chosen at the draw.
  - UI scale becomes one number rather than an atlas rebuild.
  - The flag gives a later text mode, such as multi-channel fields or outlines, a place to go.
- **Costs**:
  - The quad shader has one branch, and the push constant range now reaches the fragment stage.
  - A sprite drawn from a glyph atlas next to a label shares its texture. Only the text flag
    keeps them in separate batches, and merging them by mistake would silently threshold the
    sprite.
  - The edge width comes from screen-space derivatives (`fwidth`), which are undefined for a
    fragment shaded outside a full 2x2 pixel quad. UI text sizes do not reach that case.
- **Revisit when**: text is drawn large enough that single-channel corners visibly soften.
