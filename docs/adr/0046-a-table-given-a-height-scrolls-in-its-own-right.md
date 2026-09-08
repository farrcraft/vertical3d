# ADR-0046: Table Scrolling — A Table Given A Height Scrolls Its Own Rows, And Its Header Stays Above Them

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`Immediate::headerRow` draws the column names at the layout pen, so inside a scrolling window
the header scrolls away with the rows under it and a long roster is read against nothing. The
window is the only thing in the layer that scrolls: it clips to its body, keeps a `scroll` in
`Retained`, and draws one bar down its right ([ADR-0037](0037-clipping-is-a-scissor-the-batch-carries.md)).
A table has no region of its own, so there is nothing for a header to be frozen *against*.

That makes "freeze the header" two different features depending on where the table sits. If a
table is the whole of a window's content, pinning the band at the window's content top is
almost free. If it is one of several things in the window, or if two tables share one, there is
no coherent answer at all — the rows a header belongs to are not the only thing moving.

`Canvas::clip` intersects with whatever is already clipped, so a region nested inside a
window's is already expressible.

## Decision

**A table may be given a height, and one that has been given a height scrolls its own rows
inside it: its own clip, its own `scroll` in `Retained`, its own bar down its right, and its
header row drawn once above the region rather than inside it.** A table given no height is
unchanged. The wheel turns the innermost region under the cursor, so a table inside a window
takes the wheel from the window.

## Alternatives Considered

### Alternative 1: A table owns a scroll region when it is given a height — **chosen**
- **Pros**: The header is pinned wherever the table sits, several tables in one window each
  scroll, and a table below other content works like one at the top. It composes with the
  window rather than special-casing it, because the clip already nests. A table given no height
  keeps exactly today's behaviour, so no call site changes.
- **Cons**: It is the larger change. The scroll, the bar, the wheel routing and the content
  measurement all exist once for a window and now exist again for a table, and the two have to
  agree about what a scroll is.
- **Why not**: n/a — chosen.

### Alternative 2: Pin the band against the window's viewport, for a table at the top only
- **Pros**: Much smaller — the band draws at `window_.contentTop` unscrolled and the rows
  re-clip below it, reusing the window's scroll entirely.
- **Cons**: It only works when the table is the first thing in the window, and nothing in the
  code can say so. A table further down silently keeps the old behaviour, which is a rule a
  caller discovers by the header not sticking.
- **Why not**: A feature that works depending on what was drawn before it is worse than not
  having it, because the failure is invisible.

### Alternative 3: Leave it, and record that a header scrolls away
- **Pros**: Nothing to build, and only a roster long enough to scroll ever notices.
- **Cons**: The layer is a replacement for a toolkit that has this, and a table is the widget
  where a header matters most. The gap is recorded rather than closed, and the next consumer
  meets it in the same place.
- **Why not**: The clip nesting that makes it tractable already exists, so the cost is
  duplication of the scroll machinery rather than a new mechanism.

### Alternative 4: Let a table nest a window
- **Pros**: One scroll implementation, reused rather than repeated.
- **Cons**: A window has a title bar, a fold and a background; a table's region wants none of
  them. Making those optional turns `window()` into a flags parameter, which is the shape this
  layer has deliberately avoided.
- **Why not**: It unifies the mechanism at the cost of the concept, and the concept is what
  makes the layer readable.

## Consequences

### Positive
- A header row stays above the rows it names, which was the second of the two behaviour losses
  a consuming app recorded against this layer.
- **A table's bar has no one-frame lag.** A table given a height always reserves the width its
  bar would take, so the columns do not re-flow when one more row arrives and the bar can be
  drawn the same frame the content overflows — unlike a window's, which appears the frame after
  because the space it needs was not reserved.
- Several tables in one window each scroll independently, and a table below other content works
  like one at the top.
- The wheel routing generalises rather than branching: the innermost region under the cursor
  claims it, which is the same "last to claim the cursor keeps it" rule the layer already
  applies to overlapping windows.

### Negative
- **The scroll machinery exists twice.** A window and a table each measure their content, clamp
  their scroll, draw their bar and take the wheel. `scrollbar()` is shared, the rest is not, and
  a change to how scrolling feels has to be made in both.
- **A row scrolled out of sight is still offered to the cursor.** `interact()` knows nothing
  about the clip, so a row clipped away answers a hover if the cursor happens to be where its
  box went. This is the window's existing behaviour inherited rather than a new defect, and it
  is only reachable when something else is drawn where the clipped rows land.
- The gutter is reserved whether or not there is anything to scroll, so a table given a height
  is narrower than the same table without one even when it holds two rows.
- `Table` grows from five members to eleven, and `endTable()` from six lines to twenty.

### Risks
- **A height smaller than the header leaves a region of no height.** The view clamps to zero and
  the rows are clipped to nothing, which draws an empty table rather than reporting anything.
  The escape hatch is the caller's height; nothing in the layer can pick a sensible one for it.
- A table whose rows are taller than its region can be scrolled to a position where no row is
  fully visible. That is true of a window too, and is the caller's row height rather than the
  region's.
