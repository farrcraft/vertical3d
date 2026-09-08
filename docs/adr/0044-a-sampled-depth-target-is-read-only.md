# ADR-0044: Sampled Depth — Asking For A Readable Depth Target Chooses The Format, And The Recorder Leaves It Read Only

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0031](0031-a-pass-draws-into-a-target-it-names.md) gave a pass a target of its own and had
the recorder leave that target's colour image in `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`, so
what one pass rendered is what the next one samples. Its depth image got half of that: the
target allocates one, a pass tests and writes it, and then nothing can read it. The image was
created with `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT` alone, there was no sampler, and
`RenderTarget::texture()` described only the colour attachment.

A shadow map is exactly the case that gap forbids: it has no colour worth keeping and is
nothing *but* the depth image, read by a later pass. So the one target shape that would have
justified offscreen rendering first was the one it could not express.

The thing that makes this more than a missing usage bit is that `VK_FORMAT_D32_SFLOAT` being
usable as a depth attachment does not mean the device will let a shader sample it. Attachment
and sampled are separate bits in `optimalTilingFeatures`, and a device may offer the first and
not the second.

## Decision

A depth buffer is told at construction whether it will be sampled. That answer picks the
format — the walk requires `SAMPLED_IMAGE` as well as `DEPTH_STENCIL_ATTACHMENT` — and adds the
usage bit and a sampler. The recorder then transitions a sampled target's depth image to
`VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL` after the last pass that wrote it, the way it already
does for colour.

## Alternatives Considered

### Alternative 1: Always allocate depth with sampled usage
- **Pros**: no flag, no second format question, no way to get it wrong
- **Cons**: every window's depth buffer pays for a sampler and a possibly worse format, to
  serve the passes that read one — which today is none of them
- **Why not**: `chooseFormat` would have to answer the harder question for every app in the
  tree, and a device that offers `D32_SFLOAT` as an attachment but not as a sampled image would
  quietly move every app to a combined depth-stencil format.

### Alternative 2: Leave it in `SHADER_READ_ONLY_OPTIMAL`, matching colour
- **Pros**: one layout for everything a later pass samples; symmetric with the colour path
- **Cons**: `DEPTH_READ_ONLY_OPTIMAL` is the layout that allows a depth aspect to be *both*
  sampled and tested against, which is what a shadow pass that keeps depth-testing wants
- **Why not**: the narrower layout costs nothing and permits strictly more.

### Alternative 3: Let a pass declare that it samples a target, and derive the barrier
- **Pros**: the recorder would know rather than be told, and could reject a pass that both
  samples a target's depth and draws into it
- **Cons**: a dependency graph between passes, which nothing else in the frame has
- **Why not**: worth doing when a frame has enough passes to need one. Until then it is
  machinery for one edge.

## Consequences

### Positive
- A shadow map is expressible: one pass draws depth into a target, a later one samples it
  through `QuadRenderer::depthTexture()`, and the recorder puts the barrier between them.
- The format question is asked once, where it can be answered — a pipeline drawing into a
  sampled target is built against that target's `depthFormat()`, which is the format the
  device actually gave.
- Nothing in the tree changes: a depth buffer defaults to not sampled, which is the format,
  the usage and the barriers it had before.

### Negative
- A pass that both samples a target's depth and draws into it is a hazard this design forbids
  rather than detects. The recorder makes the read-only promise on the app's behalf and has no
  way to know the app broke it; the validation layer is what says so.
- Two depth formats can now be in play in one process — the swapchain's and a sampled target's
  — so a pipeline built against the wrong one is a new mistake that is available to make.

### Risks
- The sampled path has no consumer in this tree, so it is verified by the validation layer on a
  consumer outside it rather than by anything here. The unsampled path every app uses is
  covered, and is what a regression would most likely break.
