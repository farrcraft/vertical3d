# ADR-0132: UI: transition state lives on what it animates

**Status**: accepted
**Date**: 2026-10-30
**Documented in**: [internals/ui/README.md](../internals/ui/README.md#transitions)

## Context

A transition eases a colour or an opacity from what was shown toward what is asked for, so the
colour it started from and when it started have to outlive the frame that started it. A retained
component is laid out and drawn again every frame, and the component renderer is rebuilt
whenever the scale changes. The immediate layer has no component at all, only an id per widget
and a map of state kept per id. A notice fading or a menu opened from the keyboard changes with
no pointer event to start it.

## Decision

A transition's state is kept where the thing it animates is identified: on a retained
`Component`, and in the immediate layer's per-id state. A transition starts when the colour
asked for changes, whatever changed it, and runs on a ui clock that the screen advances by the
real time between frames rather than the simulation's. Layout never reads it.

## Alternatives

### A store in the component renderer, keyed by component
- **For**: components stay plain data, and one object holds everything animating, as egui's
  animation memory does.
- **Against**: the renderer is rebuilt on every rescale, which would drop every transition
  under way. Its keys need weak pointers and pruning, and the immediate layer would need a
  second store keyed by id.
- **Rejected because**: the state has to outlive the renderer, and the two ui models would keep
  it in two different ways.

### A record kept by the cursor router
- **For**: only the few things the pointer last changed are animating, so a short list is
  enough and nothing else changes.
- **Against**: a change the pointer did not make, such as a notice fading or a menu opened from
  the keyboard, starts no transition. It covers the retained model only.
- **Rejected because**: transitions have to follow any change of colour, not only the
  pointer's.

## Consequences

- **Gains**:
  - A transition survives a rescale and a theme change, and ends when its component is
    destroyed or its id ages out.
  - Any change of a colour eases, whatever caused it, as a CSS transition does.
  - The game menu opens while the game is paused, because the ui clock does not follow the
    simulation.
- **Costs**:
  - A component carries presentation state, which drawing writes through a const renderer.
  - A widget of the immediate layer that eases holds a per-id entry, so the map grows with the
    widgets drawn rather than only the stateful ones.
  - The screen reads the time itself, so a ui drawn without a screen has to be given its clock.
- **Revisit when**: one component is drawn into two screens at once, which would need a
  transition per screen.
