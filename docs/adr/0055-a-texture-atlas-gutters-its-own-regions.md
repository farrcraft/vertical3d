# ADR-0055: Atlas Packing — A Texture Atlas Reserves The Gutter Around Every Region, And A Caller Asks For The Size It Will Blit

**Date**: 2026-09-12
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`image::TextureAtlas` packed regions flush. The skyline starts at `(1, 1)` and `fit()` stops at
`width_ - 1` and `height_ - 1`, so the *sheet* has a one texel border, but between two
allocations there was nothing. A region's edge texel was its neighbour's edge texel, and a
linear sampler reading a quad's edge reads both.

Every consumer that needed a gap therefore wrote its own, and the four call sites in and around
the tree had settled on three different answers:

| Caller | Asks for | Blits | Gutter |
|---|---|---|---|
| `font::TextureFont` glyphs | `w + 1, h + 1` | `w, h` | right and bottom |
| `font::TextureFont` line quad | `5, 5` | `4, 4` | right and bottom |
| `tetris::TetrisRenderer` pieces | `w, h` | `w, h` | none — half a texel of UV inset instead |
| cozy's `tools/pack` | `w + 2, h + 2` | `w, h` inset by one | all four sides |

The failure a caller who forgets gets is a picture, not an error. cozy shipped a screenshot of
one: every tree in a title backdrop drew with a hard dark line across its top, because the
`tree` region began one row below `hill`'s bottom row — opaque by design, a hill having a flat
bottom — and the sampler read the boundary texel of both. Nothing logged, nothing asserted.

The library had no test of its own to state any of this: there was no `TextureAtlasTest` at all.

## Decision

The atlas reserves the gutter. `region(width, height)` takes the size the caller will blit,
allocates one texel larger on **all four sides**, and returns the *usable* rectangle positioned
inside that reservation. A caller neither adds the gutter to what it asks for nor subtracts it
from what it gets.

The gutter is left as the constructor leaves it — zero, which is black at depth 3 and
transparent black at depth 4. Nothing replicates edge texels into it.

The width is a constant rather than an argument, and `region()` is changed rather than joined by
a padded sibling: flush packing has no correct use for a sampled atlas, and leaving it reachable
leaves the divergence this record exists to end.

## Alternatives Considered

### Alternative 1: right and bottom only, relying on the sheet border
- **Pros**: half the texels. Every region still ends up fully surrounded — its own right and
  bottom, plus its left and top neighbour's — because the sheet already borders all four edges.
  `TextureFont` already did exactly this, so its migration would have been pure deletion.
- **Cons**: the invariant holds only by a chain of reasoning about the allocator, the sheet
  border and the packing order, rather than locally about one region. A future change to the
  skyline — regions placed right to left, say, or a second atlas policy — breaks it silently and
  in a picture.
- **Why not**: rejected on robustness, knowing the cost. The empirical worry was the font atlas,
  whose suite records that a 48pt distance field with a spread of 8 fits its 512 square and that
  "raising either knob one step overflows it". Measured rather than assumed: it still fits, and
  spread 12 still does not, so both halves of that budget still assert what they claim.

### Alternative 2: replicate the edge texels into the gutter
- **Pros**: bilinear at a region edge reads the region's own colour, so the gutter is correct
  without any UV inset, and `tetris`'s half-texel arithmetic could go.
- **Cons**: the blit stops being a copy, and a 24-bit atlas and a 32-bit one disagree about what
  an edge even is — smearing opaque colour outward is right for a photograph and wrong for a
  silhouette whose border is meant to be transparent.
- **Why not**: it solves a different failure from the one observed. The bug was a region reading
  its *neighbour*; a zeroed gutter ends that. Reading one's own gutter is a fade at an edge, and
  a half-texel inset already answers it where it matters.

### Alternative 3: a padded `region()` beside the flush one
- **Pros**: nothing that packs today changes, so no sheet is re-laid out.
- **Cons**: keeps two behaviours in the allocator and leaves the choice with the caller, which
  is the thing that produced three conventions.
- **Why not**: the point is to stop the caller having the choice.

### Alternative 4: a constructor argument for the width
- **Pros**: a mipmapped atlas needs the gutter to grow with the chain.
- **Cons**: nothing in the tree mipmaps an atlas, and a per-atlas knob is a per-atlas
  disagreement waiting to be had.
- **Why not**: speculative. One constant in one file becomes an argument the day something needs
  a second value, and the comment beside it says so.

## Consequences

### Positive
- The gutter is the allocator's policy, so a new consumer gets it without knowing it exists.
  This is [ADR-0028](0028-an-apps-shell-belongs-to-the-api.md)'s argument — what every consumer
  repeats belongs to the api — applied where the copies had already begun to disagree.
- `TextureFont`'s two call sites lose their arithmetic rather than gaining any: it asks for the
  glyph's own size now, and `w - 1` / `h - 1` is gone.
- `TextureAtlasTest` exists. The case that matters fills each region and reads the ring around
  it, so it states the invariant without knowing where the packer put anything — and it fails on
  the old behaviour, which is how cozy's dark line is now a test rather than a screenshot.

### Negative
- Every sheet is laid out differently. One packed before this does not match one packed after,
  which matters to anything that pins a sheet or its coordinates as a fixture.
- A region costs two texels more per axis, so a full atlas holds fewer of them. The largest
  region a 16 square can hold is 12, not 14.

### Risks
- The font atlas was the one close to its budget, and it is still inside it — but by less than
  it was. A face, size or spread that fits today may not after some later change, and the signal
  is `TextureFont`'s `missed` count rather than a failure. `texturefont_distance_field_packing_test`
  is what watches this.
- `tetris` keeps its half-texel UV inset, which is now guarding against the gutter rather than
  against a neighbour. Its comment says so; the code did not have to change.
- cozy's `tools/pack` still adds its own all-four-sides gutter, so until it is pointed at this
  and its local padding deleted, its sheets carry two gutters and waste a texel per side. That
  is part of taking this change rather than a follow-up, and is recorded in that repository's
  sprite sheet tooling handoff.
