# ADR-0036: 2D Drawing — Text Is A Distinct Kind Of Quad, And The Primitive Carries Which

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

Amends [ADR-0005](0005-one-batched-quad-primitive.md). One pipeline still draws every 2D thing
and a glyph is still a quad with a texture; what changes is 0005's claim that no branch is
needed in the fragment shader.

## Context

`ui::TextRenderer` rasterizes a font at a size fixed in its constructor, and nothing scales a
glyph afterwards, so a second size is a second atlas. Four apps in the tree — pong, tetris,
voxel and the editor — each hardcode one font size around this. Worse, the ui lays out in canvas
pixels while glyphs are rasterized at a fixed pixel size, so the same text is physically half as
large on a 4K display as on 1080p.

Signed distance field glyphs are the standard answer: one atlas serves every size because the
size becomes an argument at the draw. But an SDF texel is a distance rather than a coverage, so
it has to be thresholded with a `smoothstep` to become an alpha. `quad.frag` is shared by
panels, sprites and glyphs, and applying that threshold unconditionally would corrupt every
non-text quad in the tree.

That is the tension. [ADR-0005](0005-one-batched-quad-primitive.md) chose the 1x1 white texture
specifically so that the fragment shader would need no branch, and single-channel atlases reach
alpha through a swizzled view for the same reason. Resolution-independent text cannot be had
without the primitive knowing which of its batches are text.

## Decision

A batch carries a flag saying whether it is text, `Canvas::open()` refuses to merge across it,
and `QuadRenderer::submit` writes it into the push constant block alongside the projection. The
quad pipeline's push range grows to the fragment stage, which it does not reach today, and
`quad.frag` branches on it: a text fragment takes the sampled distance and `smoothstep`s it
against a width derived from the screen-space derivative, and every other fragment samples
exactly as it does now.

## Alternatives Considered

### Alternative 1: A flag in the push constant, and one branch in the fragment shader — **chosen**
- **Pros**: Stays at two pipelines. The branch is uniform across a whole draw — every fragment
  of a batch takes the same side — so it costs a scalar test rather than divergence. `DrawItem`
  already carries a 128-byte push block of which the quad primitive spends 64, and
  `QuadRenderer::submit` already builds one item per batch, so both the room and the granularity
  exist without a new mechanism.
- **Cons**: Contradicts 0005's "no shader branch" in the letter. The push range has to be
  declared for the fragment stage as well as the vertex stage.
- **Why not**: n/a — chosen.

### Alternative 2: A second pipeline, selected per batch
- **Pros**: No branch at all, which is what 0005 asked for. Has precedent — the renderer already
  builds two pipelines and picks between them by whether the pass has a depth buffer.
- **Cons**: The two axes multiply rather than add: text and non-text against depth and no depth
  is four pipelines, four `PipelineBuilder` calls and four cache entries. It adds pipeline
  switches per frame, since a ui frame alternates panels and labels constantly, to save a branch
  that is uniform across the draw either way.
- **Why not**: It pays a real per-frame cost — a pipeline bind is the expensive state change in
  a Vulkan frame — to avoid a branch whose cost is close to zero precisely because it never
  diverges. The trade runs the wrong way.

### Alternative 3: Keep coverage glyphs and rasterize a second atlas per size
- **Pros**: No shader change, no ADR, no amendment. It is what the tree does today.
- **Cons**: An atlas per size, uploaded per size, and a size that must be known before anything
  is drawn. It does not answer the display-scaling problem at all — a ui that wants to be the
  same physical size on a 4K display still needs an atlas rebuilt when the window moves between
  monitors.
- **Why not**: The four hardcoded font sizes in the tree are the evidence that this does not
  scale past one size per app, and a text size setting is a thing apps are expected to offer.

### Alternative 4: Multi-channel distance fields (msdfgen)
- **Pros**: Preserves sharp corners at large scales, which single-channel fields soften.
- **Cons**: A new dependency, a third-party generator in the asset path, and a three-channel
  atlas where the swizzled single-channel view is what makes the current upload simple.
- **Why not**: Deferred, not rejected. No app in the tree draws text at a size where
  single-channel softening is visible, and the flag this record adds is what a later switch
  would key off anyway.

## Consequences

### Positive
- One atlas serves every size, so `TextRenderer` can take its size at the call and four apps
  stop hardcoding one.
- A ui scale becomes one number rather than an atlas rebuild, which is what a text size setting
  and a 4K display both need.
- The flag is a place for a later text mode — multi-channel fields, or an outline — to live
  without another amendment.

### Negative
- 0005's "no shader branch" no longer holds. It held for four ADRs and bought a simpler
  pipeline while text had one size; it stops being worth its price once text needs two.
- The push block grows past the projection, and the fragment stage now reads push constants,
  so the pipeline layout is no longer vertex-only.

### Risks
- **A text batch merging with a non-text one drawing from the same texture.** `Canvas::open()`
  extends the open batch when the texture handle matches the last, and a glyph atlas is a
  texture like any other — a sprite drawn from the atlas immediately before a label would merge
  today and be thresholded as text tomorrow. The merge condition has to widen to the flag as
  well as the handle, and it is the one place where getting this wrong is silent.
- **Derivative-based antialiasing needs the derivative to be meaningful.** `fwidth` on the
  distance is what keeps a glyph crisp at any scale, and it is undefined in a fragment shader
  invoked outside a full quad of pixels. At the sizes a ui draws text this is not reachable, but
  it is the reason the threshold width is derived rather than a constant.
