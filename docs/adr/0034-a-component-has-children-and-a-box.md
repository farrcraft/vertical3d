# ADR-0034: UI Layout — A Component Has Children, And The Draw Walk Resolves Its Box

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md) decided that drawing is what lays the
ui out, and considered and rejected a box model in its Alternative 2: "far more machinery than
a strip of labels justifies", for one consumer with one arrangement. That was true of a menu
bar. It is not true of the second consumer, a game HUD, which nests four levels deep, sizes
half its boxes as a fraction of the one around them, lists a number of rows that is only known
at run time, and puts a modal plate over everything below it.

`api/ui` cannot express any of that. `Component` carries an absolute position and size and no
relation to any other component; `Container` is a flat vector walked in the order things were
added, and `Component::depth()` has no writer and no reader. `HorizontalBox.h` and
`VerticalBox.h` are still a `#pragma once` and a copyright header, exactly as ADR-0019 found
them, and there is no component that draws a plain filled box at all — `Frame` is an empty
declaration.

## Decision

**A component has children and a box: a layout spec measured in pixels, in a fraction of its
parent, or left to the component itself, resolved against the parent's box by the same draw
walk that already places a menu.** ADR-0019's rule is extended down a tree rather than
replaced — `position()` and `size()` remain the absolute bounds a component was last drawn in,
and remain what the cursor is tested against.

## Alternatives Considered

### Alternative 1: Children, and the box resolved inside the draw walk — **chosen**
- **Pros**: One walk still decides both what is drawn and what is clicked, which is the whole
  of ADR-0019 and the reason a hit box cannot drift from its label. A parent's box is known at
  the moment its children are reached, so a percentage resolves against a real number rather
  than against a cached one. Nothing that draws today changes: a root component with a pixel
  box resolves to the position it was authored at, and a `Length` left `Auto` keeps the
  natural size a button and an icon already compute for themselves.
- **Cons**: A component is still not clickable until it has been drawn, and that failure now
  inherits — every child of a component that was skipped is unplaced too. Layout cannot be
  asked for without a canvas and a font, so a test that wants a box has to draw one.
- **Why not**: n/a — chosen.

### Alternative 2: A measure and arrange pass before drawing, as ADR-0019's Alternative 2 described
- **Pros**: The conventional shape. A component would know its size before anything drew it,
  and layout would be answerable in a test with no canvas at all.
- **Cons**: Two walks that have to agree about the same rectangle, which is the failure ADR-0019
  chose against. Measuring text still needs the app's font callback, so the pass takes the same
  `Measure` the renderer holds and the two share the coupling anyway. The bounds would be
  written by the arrange pass and then again by the draw, and the hit test would have to say
  which of the two it believes.
- **Why not**: The second consumer changes how much layout is needed, not who is allowed to do
  it. ADR-0019's objection survives its Alternative 2 being wanted.

### Alternative 3: Leave the components flat and let each app do the arithmetic
- **Pros**: No change at all, and the editor's menu bar and toolbars are already laid out this
  way by the renderer.
- **Cons**: Every app that wants a HUD writes the same nested-rectangle arithmetic, and a ui
  library that cannot express a box inside a box is not a substitute for the third-party
  toolkits it is meant to replace.
- **Why not**: The point of the library is that the second app does not repeat the first one's
  arithmetic, which is [ADR-0028](0028-an-apps-shell-belongs-to-the-api.md)'s reasoning applied
  one level down.

### Alternative 4: Adopt a CSS box — flow, floats, margin collapsing, a grid
- **Pros**: A known model with known semantics, and a stylesheet would translate rule for rule.
- **Cons**: An enormous amount of machinery, most of which has no consumer. The requirement is
  a box that holds boxes, a percentage, an anchor and one flow direction; margin collapsing and
  a grid are neither asked for nor cheap.
- **Why not**: Out of proportion to what any app in the tree, or the one outside it, needs.

## Consequences

### Positive
- `Component` holds children and a parent, `Container` walks in z-index order, and
  `VerticalBox` and `HorizontalBox` are flow lists that arrange however many children they were
  given rather than empty headers.
- A `Panel` — a filled box with a border and an optional corner radius — and a `Bar` — a fill
  as a fraction of a track — are the two drawable boxes every HUD is built out of, and both are
  dressed by a named style like a button, per
  [ADR-0020](0020-a-theme-is-data-and-the-app-resolves-its-images.md), rather than carrying
  colours of their own.
- A rounded corner costs no new primitive: it is the quads and the fans of
  [ADR-0005](0005-one-batched-quad-primitive.md) in the same stream, so the ui still adds no
  pass and no draw of its own.
- Any component can be hit tested, not only a menu or a toolbar. `Container::pick` answers the
  topmost component under a point, so a plate laid over a scene can be what stops a click from
  reaching it.

### Negative
- **`Auto` resolves to nothing useful before the first frame.** An `Auto` extent is the box the
  component was last drawn in, which is nothing until it has been drawn once, and an `Auto`
  position is wherever it already was — ADR-0019 already had this for one component and it now
  applies to a subtree. Superseded by
  [ADR-0039](0039-layout-never-reads-the-box-it-wrote.md), which took the last box out of
  layout altogether. A percentage was never affected: the walk resolves top down and hands each
  child a parent box worked out on the same frame.
- Sorting a container by z-index happens every frame rather than on insertion, because a
  component's depth can change after it was added. It is a stable sort of a handful of
  pointers, and add order is what a container with no z-indices keeps.
- `Auto` means two different things by axis — for a size it is the component's natural extent,
  for a position it is wherever the component was last placed — and nothing in the type says so.
  [ADR-0039](0039-layout-never-reads-the-box-it-wrote.md) changed what the second one means and
  not that there are two.

### Risks
- **A component takes no press unless it is marked pickable**, which is the opposite default
  from every retained-mode toolkit and will read as a bug the first time a button in a box does
  nothing. It is deliberate: a HUD is mostly labels drawn over a board that must stay clickable,
  and the alternative default would have every one of them eating an order. The escape hatch is
  one flag on the component and one key in the config.
- Nothing checks that a child's box stays inside its parent's, and nothing clips one that does
  not. A box model without clipping is what a scrollbar would need first, and `Scrollbar` is
  still an empty declaration.
