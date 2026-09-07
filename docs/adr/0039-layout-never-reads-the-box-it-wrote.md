# ADR-0039: UI Layout — The Walk Never Reads The Box It Wrote, And Auto Is The Room A Component Is Offered

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0034](0034-a-component-has-children-and-a-box.md) put layout in the draw walk and recorded
what it cost: "a percentage of a parent that has never been drawn is a percentage of zero, so
the frame after a resize places a child against the previous size". The percentage half of that
is not what happens — the walk resolves top down and hands each child the box it has just
written for the parent, so a percentage is always of a box worked out this frame. The stale
reads are all `Auto`, and there are two of them.

`Arranger::natural()` answered an `Auto` **extent** with `Component::size()` for a panel, a bar
and a box, and with `Component::size()` in one axis for a scrollbar and a select list — the box
the last walk wrote. `Layout::resolve()` answered an `Auto` **position** with
`Component::position()`, likewise. Both are the walk's own output from a previous frame, which
is zero before the first one.

The second is worse than stale. An `Auto` position resolves to wherever the component already
is *in absolute terms*, so a child of a panel at (100, 50) is placed at the canvas origin on the
first frame and, because that is then where it already is, stays there for every frame after.
`Length()` defaults to `Auto`, so every component in a config that names no `x` or `y` has this.

## Decision

**Nothing in layout reads a box a previous walk wrote.** An `Auto` extent is what the component
makes of the axis, and a component that makes nothing of itself takes the room it is offered; an
`Auto` position is no offset at all, so the component sits at the corner it is anchored to.
`Arranger::natural()` takes the room as an argument and `Layout::resolve()` no longer takes
where the component already was.

A flow box offers its children the whole extent across the line it lays out and none of it along
the line, because along the line the room is shared. So `Auto` fills across a `VerticalBox` and
is content-sized down it, which is the distinction that keeps a column of panels a column rather
than a stack of full-height panels.

## Alternatives Considered

### Alternative 1: `natural()` takes the room, `Auto` position is the anchored corner — **chosen**
- **Pros**: Layout becomes a pure function of the tree and the canvas, so the same tree laid out
  twice lands in the same place and the first frame is the same as the tenth. It needs no extra
  pass, no dirty flag and no retained state — the room is already in the walk's hand at the
  point `natural()` is called. The two axes of `Auto` stop meaning unrelated things.
- **Cons**: `Auto` now means something slightly different inside a flow box than outside one,
  and nothing in the type says so. An `Auto` position is also now indistinguishable from
  `Length(0, Pixels)`, so the unit carries less than it looks like it does.
- **Why not**: n/a — chosen.

### Alternative 2: Two passes — measure, then place
- **Pros**: The textbook answer, and the one that would let a parent size itself to what it
  holds, which none of this does. A `VerticalBox` that is as tall as its rows is out of reach
  today.
- **Cons**: Doubles the walk, and needs a measured size cached per component per frame for the
  place pass to read — which is the retained layout state ADR-0034 chose against and ADR-0019
  chose against before it.
- **Why not**: Out of proportion to the defect. Worth revisiting when something wants a parent
  sized by its children, which is a different feature rather than this fix.

### Alternative 3: Keep reading the last box, and lay the tree out twice on the first frame
- **Pros**: No signature changes at all. The second walk sees the first walk's boxes, so the
  numbers settle.
- **Cons**: Settles by iteration rather than by construction, and only for trees whose fixed
  point is reachable in one step. It does nothing for the `Auto` position, whose fixed point is
  the canvas origin — running it twice arrives at the wrong answer twice.
- **Why not**: It hides the defect behind a warm-up frame instead of removing it.

### Alternative 4: Leave `Auto` alone and fix only the scrollbar and the select list
- **Pros**: The smallest change. Those two already document their length as "its parent's rather
  than its own", so they are the clearest cases.
- **Cons**: Leaves a panel with an `Auto` size at zero on its first frame and leaves the `Auto`
  position stuck at the origin, which is the one that will be met first — a config that names no
  position is the common case, not the odd one.
- **Why not**: Fixes the symptoms that were written down rather than the thing causing them.

## Consequences

### Positive
- A ui is in the right place on the frame it first appears. Nothing needs a warm-up frame, and
  a window resize places every child against the new size rather than the old one.
- A component in a config can leave out `x`, `y`, `width` and `height` and land filling its
  parent, which is the sane default and was previously a zero-sized box at the canvas origin.
- `Arranger::natural()` is now answerable without a component having been drawn, so layout can
  be asked about a tree that has never been on screen — which is what ADR-0034 promised when it
  split the walk from the paint.

### Negative
- `Auto` reads differently by context: filling outside a flow box, content-sized along one. A
  reader who expects one rule has to learn two.
- A component can no longer be moved by writing `position()` and leaving `x` and `y` on `Auto`.
  Nothing in the tree did that, and the escape hatch is `Length(n, Pixels)`.
- `Arranger::natural()` takes an argument that three of its cases ignore, because a label, an
  icon and a button are sized by their text whatever room they are in.

### Risks
- A `Panel` with no width now covers its parent rather than nothing, so a config that relied on
  an unsized panel drawing nothing gets a filled box. No config in the tree does — nothing here
  loads a component tree yet — but the first one written against the old behaviour would.
- The room a flow box offers along its line is zero, so a `SelectList` or a `Scrollbar` dropped
  into a `VerticalBox` with no explicit length has none. It is the honest answer for a shared
  line, and it will read as a bug the first time.
