# ADR-0128: UI: theme colours derive from base, accent and contrast

**Status**: accepted
**Date**: 2026-10-30
**Documented in**: [api/ui/Themes.md](../api/ui/Themes.md#themes-and-styles)

## Context

A theme names colours of the dressing one by one, and a colour it does not name keeps a built-in
value. A polished ui draws hover, pressed, selected, disabled and focused states that agree with
one another. Text needs 4.5:1 contrast against its surface and a control's edge needs 3:1. A
theme that lists every role and every state repeats about twenty-five colours, and they drift
apart as states are added. Godot and Linear derive a whole palette from a base colour, an accent
colour and a contrast setting, and Unreal lists its roles explicitly.

## Decision

A theme may give a base colour, an accent colour and a contrast. `api/ui` derives a twelve-step
neutral scale and a twelve-step accent scale from them, and assigns every role and state to a
fixed step. A role the theme names directly overrides its derived value, and a theme that gives
none of the three derives from built-in values.

## Alternatives

### Explicit roles only
- **For**: what the document says is exactly what is drawn, with nothing computed.
- **Against**: every theme restates every state, a new state needs a value in every theme, and
  re-colouring a game means writing about twenty-five colours.
- **Rejected because**: the states stay consistent only if one rule produces them.

### Derived roles with no overrides
- **For**: every theme is three values, and no two themes can disagree on a rule.
- **Against**: a role that has to differ, such as a game's score colour, cannot be set.
- **Rejected because**: an app sometimes needs one colour of its own.

## Consequences

- **Gains**:
  - A game re-colours its whole ui by giving one accent.
  - Hover, pressed, selected, disabled and focus states agree in every theme.
  - Contrast is checked once, by tests over the derivation, rather than per theme.
- **Costs**:
  - A derived colour does not appear in the document, so reading a theme does not show every
    value drawn.
  - The derivation is code whose output every app depends on, and a test has to pin it.
  - A value can come from three places: the derivation, the theme and the app's dressing.
- **Revisit when**: a theme needs a second accent scale, or roles that do not fit a lightness
  scale.
