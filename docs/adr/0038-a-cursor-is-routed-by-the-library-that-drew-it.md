# ADR-0038: UI Input — A Cursor Is Routed By The Library That Drew It, And A Press Dispatches A Command

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`api/ui` has three answers to what a click means and no rule tying them together.
`component::Toolbar::press()` finds the button under the cursor and dispatches its event;
`component::MenuBar::press()` does the same for an item in a dropped panel; and
`Container::pick()` returns the topmost pickable component and dispatches nothing, because a
`Container` holds no dispatcher. So a `Button`, `CheckBox`, `RadioButton` or `SelectList`
anywhere outside a strip carries a bound event that nothing sends — which is why nothing in
the tree drives any of them, five months after
[ADR-0034](0034-a-component-has-children-and-a-box.md) gave them a box to be clicked in.

The ordering between the three is meanwhile written by hand in the one app that has strips.
`vertical3d/src/Controller.cxx` offers the cursor to the menu bar first *"because an open panel
is drawn over a toolbar"*, then tells every toolbar to `leave()` or `motion()`, and repeats the
ordering for a press. That rule is a fact about how this library draws, not about the editor,
and [ADR-0028](0028-an-apps-shell-belongs-to-the-api.md) is the standing answer for what every
app would otherwise repeat.

## Decision

**`v3d::ui::Cursor` routes a cursor over a `ui::Engine` and dispatches what it lands on**,
offering the point to the containers in the reverse of the order they are drawn — menu bars,
then toolbars, then the component tree — and stopping at the first thing that takes it. A press
that lands on a pickable component sends that component's bound event; a press that lands on
nothing pickable is not consumed, and reaches whatever is under the ui.

## Alternatives Considered

### Alternative 1: A router over the engine, dispatching what it picked — **chosen**
- **Pros**: The ordering rule lives beside the drawing it is a fact about, so the two cannot
  drift; the editor's hand-written version deletes. One place knows that a press on a select
  list means `at()` then `selected()` then dispatch, which is otherwise per app and per widget.
  It is where a press has to be remembered anyway — a scrollbar drag is a press held across
  frames — and that state belongs to neither the component nor the app.
- **Cons**: A fourth thing in a library that already has two ways to write a ui. It needs the
  engine and the dispatcher together, which nothing else in `api/ui` holds at once.
- **Why not**: n/a — chosen.

### Alternative 2: `Container` gains a dispatcher and a `press()`/`motion()` pair
- **Pros**: Symmetric with `Toolbar` and `MenuBar`, which already work this way, and needs no
  new class. A container is already what `pick()` walks.
- **Cons**: The ordering *between* containers, and between a container's tree and its strips,
  still belongs to nobody — so the editor keeps its loop and gains a third call in it. And it
  puts a dispatcher on the class an app looks components up by name in, which is a loader's
  output rather than an input sink.
- **Why not**: It fixes the dispatch and leaves the ordering, which is the half the editor is
  writing by hand.

### Alternative 3: The app keeps doing it, and the library documents the order
- **Pros**: No new code, and an app that wants a different order can have one.
- **Cons**: Every app writes the same twenty lines, and the order is only correct while a
  reader keeps the document and the renderer in agreement. `Container::pick()` still hands back
  a `shared_ptr<Component>` an app has to downcast to know what it may do with.
- **Why not**: This is the state that left the widget set with no consumer.

### Alternative 4: Components handle their own input, as `Toolbar` and `MenuBar` do
- **Pros**: One rule for every component rather than two, and no new class.
- **Cons**: Every component needs a dispatcher, so the loader threads one into all of them; and
  a widget still cannot know whether something drawn over it should have taken the point first,
  which is the question the ordering answers.
- **Why not**: It scales the shape that works for a self-contained strip to a tree, where the
  interesting part is what is on top.

## Consequences

### Positive
- A `Button`, `CheckBox`, `RadioButton` or `SelectList` in a container answers a click, which
  none of them did. The types the loader has been able to build since ADR-0034 become usable
  without an app writing a downcast.
- The editor's `uiMotion` and `uiPress` become one call each, and the ordering comment moves to
  where the ordering is.
- A press is remembered, so a scrollbar follows the cursor across frames — the one widget whose
  input is a drag rather than a click.
- The rule that a press only counts on a `pickable()` component is enforced in one place, so
  ADR-0034's default of false keeps meaning what it says: a hud of labels over a scene leaves
  the scene clickable.

### Negative
- **Hover stays the strips'.** `Toolbar::motion()` writes `ButtonState::Hover` onto its buttons
  and no other component has a hover state at all, so a button in a tree does not light up under
  the cursor. Giving it one means a state on `Component` and a second reason for the router to
  walk every frame; nothing has asked for it.
- The router decides what a press on each component type means, which is a switch on
  `component::Type` — a fifth place that has to be edited in step with the other four to add a
  widget, and one the compiler does not check against them.
- An app that wants a different ordering has to bypass the router entirely rather than adjust
  it, since the order is the point of the class.

### Risks
- **A component that has never been drawn cannot be picked**, per
  [ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md), so a router asked about a cursor
  before the first frame answers nothing. That is the existing rule rather than a new one, but
  it now has a caller that will meet it — an app that routes input before it draws sees a dead
  ui for one frame. The escape hatch is the order of the app's own loop.
- **What a click on a select list means is still the app's**, and the router only sends the
  list's bound event. An app that wanted the row index has to read `selected()` when the command
  arrives, which is the same shape as a check box not owning its own mark.
