# ADR-0034: UI: layout is resolved while drawing

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [internals/UserInterface.md](../internals/UserInterface.md), [api/ui/DocumentsAndLayout.md](../api/ui/DocumentsAndLayout.md)

## Context

A ui needs nested boxes, sizes given as a fraction of a parent, rows whose number is known only
at run time, and panels laid over other panels. A click has to land on what the user sees, so
the box a component is hit-tested against must be the box it was drawn in. Measuring text needs
the app's font callback, which the renderer already holds. A layout that reads its own output
from an earlier frame is wrong on the first frame and lags one frame behind a resize.

## Decision

Layout is computed in the same pass that draws: each component's box is resolved against its
parent's box as the walk reaches it, and written onto the component. Hit-testing uses the boxes
that pass wrote, and a walk given no canvas resolves every box without drawing. No layout step
reads a box written by a previous frame, so layout depends only on the tree and the canvas size.

## Alternatives

### A separate measure pass and arrange pass before drawing
- **For**: the conventional design. A component's size is computed before it is drawn, and a parent
  can size itself to its children.
- **Against**: two walks have to agree about the same rectangle, and the hit test has to choose
  which one to believe. Measuring text still needs the app's font callback, so both passes share
  that coupling. The arrange pass needs a measured size cached per component per frame.
- **Rejected because**: it adds a second source for each box and retained state to keep in step,
  and no current layout needs a parent sized by its children.

### Keep each component's last box as an input to layout
- **For**: no extra state and no signature changes; a component without an explicit size or
  position stays where it was.
- **Against**: before the first frame the last box is zero, so the first frame is wrong. A
  position read back from the last frame can settle at the canvas origin and stay there. A
  resize places children against the old size.
- **Rejected because**: the result would depend on history instead of on the tree.

### Read the last box, but lay the tree out twice on the first frame
- **For**: no signature changes; the second walk sees the first walk's boxes.
- **Against**: it converges only when one extra step reaches the right answer. A position read
  back from the last frame converges on the canvas origin, which is wrong.
- **Rejected because**: it hides the stale read behind a warm-up frame instead of removing it.

### Pass the cursor into the draw call and answer the hit there
- **For**: no stored boxes at all, so none can be stale.
- **Against**: a press could be answered only on the frame that draws, so input would wait for
  the renderer and act a frame late. The renderer would also need the dispatcher.
- **Rejected because**: apps handle input as it arrives and draw afterwards.

### Leave the components flat and let each app compute positions
- **For**: no library change.
- **Against**: every app with a HUD writes the same nested-rectangle arithmetic.
- **Rejected because**: shared ui work belongs in the api, per
  [ADR-0028](0028-apps-the-shared-app-shell-lives-in-the-api.md).

### A CSS box model with floats, margin collapsing and grids
- **For**: a known model, and stylesheets would translate rule for rule.
- **Against**: a large amount of machinery that nothing needs.
- **Rejected because**: the requirement is boxes in boxes, fractions, anchors and one flow
  direction.

## Consequences

- **Gains**:
  - A hit box cannot drift from what is drawn, because one pass decides both.
  - The first frame matches every later frame, and a resize places every child against the new
    size, with no dirty flag and no warm-up frame.
  - Layout can be checked in a test, or asked for before drawing, by walking with no canvas.
- **Costs**:
  - Nothing has a box until it has been walked. A component that has never been drawn cannot be
    picked, and neither can anything inside a component that was skipped.
  - A hidden component keeps the box it was last drawn in, so picking must test its visibility
    flag rather than its box.
  - A parent cannot size itself to fit its children.
  - Layout and drawing cannot be separated later without moving both.
- **Revisit when**: a layout needs a parent sized by its children, or layout cost per frame
  becomes measurable.
