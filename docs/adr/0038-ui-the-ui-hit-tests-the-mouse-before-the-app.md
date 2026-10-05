# ADR-0038: UI: the UI hit-tests the mouse before the app

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [The User Interface](../api/UserInterface.md), [User Interface Internals](../internals/UserInterface.md)

## Context

Several parts of a ui can take a click: menu bars, toolbars and the component tree. Which one
should take a point first depends on what is drawn over what, which is a fact about how the
library draws, not about any app. A component that takes a press has to send its bound event,
which needs the ui engine and the event dispatcher together. Some presses span frames, such as
a scrollbar drag, so the press has to be remembered somewhere. An app should not have to repeat
this logic, per [ADR-0028](0028-apps-the-shared-app-shell-lives-in-the-api.md).

## Decision

`v3d::ui::input::Cursor` routes the cursor over a `ui::Engine` and dispatches the event of what
it lands on, stopping at the first thing that takes the point. Within a container it offers the
point to menu bars, then toolbars, then the component tree, which is the reverse of the order
they are drawn in. A press on a pickable component sends that component's bound event, and a
press on nothing pickable is not consumed and reaches the app.

## Alternatives

### `Container` gains a dispatcher and its own `press()` and `motion()`
- **For**: the same shape as `Toolbar` and `MenuBar`, and no new class.
- **Against**: the order between containers, and between a container's tree and its strips,
  still belongs to nobody, so each app keeps writing it. It also puts a dispatcher on the class
  apps use to look components up by name.
- **Rejected because**: it fixes dispatch and leaves the ordering to the app.

### The app routes input, and the library documents the order
- **For**: no new code, and an app can choose a different order.
- **Against**: every app writes the same routing, which is correct only while it matches the
  renderer. `Container::pick()` returns a base `Component` that an app must downcast.
- **Rejected because**: the order is a property of the library's drawing, so the library should
  own it.

### Each component handles its own input, as the strips do
- **For**: one rule for every component, and no new class.
- **Against**: every component needs a dispatcher. A component also cannot tell whether
  something drawn over it should have taken the point first.
- **Rejected because**: the hard part in a tree is what is on top, and a single component cannot
  answer that.

## Consequences

- **Gains**:
  - Any button, check box, radio button or list in a container answers a click, with no
    downcast in the app.
  - The ordering rule lives beside the drawing it depends on.
  - A press is remembered, so a drag follows the cursor across frames.
  - Only `pickable()` components take a press, so labels drawn over a scene leave the scene
    clickable.
- **Costs**:
  - What a press means is a switch on `component::Type`. Adding a widget means editing it
    alongside the other per-type switches.
  - An app that wants a different order has to bypass the router entirely.
  - A component that has never been drawn cannot be picked, so input routed before the first
    frame finds nothing.
  - Containers are offered the point in document order, which is also the order they are
    drawn. Where two containers overlap, the one drawn underneath takes the press.
- **Revisit when**: an app needs overlapping containers, or a different routing order.
