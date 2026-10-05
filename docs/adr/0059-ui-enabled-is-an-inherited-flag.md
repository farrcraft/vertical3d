# ADR-0059: UI: enabled is an inherited flag

**Status**: accepted
**Date**: 2026-09-13
**Documented in**: [api/ui/Components.md](../api/ui/Components.md), [internals/UserInterface.md](../internals/UserInterface.md)

## Context

A ui often shows a control that cannot be used yet, such as Continue before there is a saved
game. A disabled control has to look disabled, ignore the cursor, drop out of the tab order and
ignore keys, and every control type needs the same ability. A button's state field also holds
hover and press, which the cursor rewrites on every move, so a lasting property kept in that
field is overwritten. Greying out a group of controls is a common need.

## Decision

Whether a component can be used is `Component::enabled()`, a flag beside `pickable()`,
`focusable()` and `visible()`, and it is inherited down the tree. `ui::usable(component)` is the
derived answer, false if the component or anything holding it is disabled, and picking, the tab
order, keys and drawing all read it. A theme's button styles name their states with their own
enum, separate from the cursor's transient button state.

## Alternatives

### Draw the existing inactive button state, and change nothing else
- **For**: a small change that gives the right picture.
- **Against**: the cursor still overwrites the state, the tab order still reaches the button,
  and the other control types still cannot be disabled.
- **Rejected because**: it fixes the drawing and leaves the behaviour wrong.

### A per-component flag that is not inherited
- **For**: one bool per call site and no walk of the parent chain.
- **Against**: a disabled box holding enabled buttons would block clicks while its contents look
  and behave as usable. Disabling a group would mean setting every control by hand.
- **Rejected because**: disabling a group is most of what the flag is for.

### Keep the inactive button state beside `enabled()`
- **For**: no consumer changes.
- **Against**: two ways to say one thing, one of which a hover destroys.
- **Rejected because**: a transient field cannot hold a lasting property. A theme can still name
  the old `inactive` style state, which the loader reads as disabled.

### Per-state button styles carry the disabled colours
- **For**: a theme says everything about a disabled button in one place, and styles are already
  found by state.
- **Against**: a style by state holds images, not colours. Other controls choose their label
  colour outside the button style, so the colour would be named in two places.
- **Rejected because**: a shared `disabled-text` colour in the base `ui` style dresses every
  control type, beside the other shared colours of
  [ADR-0020](0020-ui-themes-are-data-apps-load-the-images.md).

## Consequences

- **Gains**:
  - A disabled control looks and behaves the same way for every control type.
  - Disabling is one call that reaches the cursor, the keyboard and drawing.
  - Disabling a box disables everything in it.
- **Costs**:
  - Drawing asks `usable()` for each component each frame, which walks the parent chain.
  - A component reports `enabled()` true while `usable()` is false when something above it is
    disabled. Code must read the right one.
  - A component disabled while it has the focus stays focused until the focus moves, and focus
    listeners are not told.
  - A theme cannot name a plate colour for a disabled button, only a plate image.
  - Two button state enums, the theme's and the cursor's, have to be told apart.
- **Revisit when**: drawing deep trees makes the parent walk measurable, or a component needs to
  learn that it lost usability while focused.
