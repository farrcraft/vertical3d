# ADR-0254: Offline: first pictures are coarse, then passes grow

**Status**: accepted
**Date**: 2026-11-20
**Tags**: offline, editor
**Documented in**: [offline/Interactive.md](../../offline/Interactive.md)

## Context

After an edit the user should see a picture within a moment, and a better one as samples arrive.
A pass of 16 samples over every pixel takes far longer than a moment for a lit scene. A session
that took edits must still end on the bytes of a fresh render, and moya's picture is the same
whatever samples each pass holds. Cycles' viewport starts at a coarse resolution, halves the block
size up to the full one, and then grows the samples of each update so that pictures come at a
steady rate. Karma XPU and RenderMan XPU render at full resolution from the first pass.

## Decision

After an edit a session renders coarse pictures first, each pixel standing for a block, halving the
block until it reaches full resolution, and the coarse samples are never added to the film. The
full-resolution passes then grow from one sample a pixel, doubling up to the most a pass takes.
The caller sets the least time between two pictures, and a session makes one after a pass once
that time has passed.

## Alternatives

### Full resolution from the first pass, in passes that grow
- **For**: Every sample counts toward the film, and no work is thrown away.
- **Against**: The first picture waits for one sample of every pixel, which for a large view or a
  slow scene is longer than a moment. A camera orbit shows nothing until each pass ends.
- **Rejected because**: The first picture after an edit matters more to an interactive user than
  the samples a coarse picture costs.

### Fixed passes, with a picture as each ends
- **For**: The pass schedule does not change, and only the time a picture is made does.
- **Against**: The first picture waits for a whole pass of 16 samples. On a heavy scene pictures
  come seconds apart.
- **Rejected because**: It does not make an edit visible quickly, which is the purpose of the plan.

## Consequences

- **Gains**:
  - A picture follows an edit within a fraction of a pass, however heavy the scene.
  - The final bytes are those of a render that made no pictures, since coarse samples are dropped
    and the size of a pass does not change the picture.
  - Pictures come at the rate the caller can show them.
- **Costs**:
  - The coarse samples are work thrown away on every edit.
  - A coarse picture is blocky, and a thin object may be missed by it.
  - A session needs a schedule of its own, which a batch render does not use.
- **Revisit when**: a single sample of every pixel at full resolution comes within the time the
  user waits for a picture, on the scenes the editor renders.
