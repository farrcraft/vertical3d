# ADR-0060: ECS: interpolate from a previous-step component

**Status**: accepted
**Date**: 2026-10-03
**Documented in**: [api/ECS.md](../api/ECS.md)

## Context

The loop calls `simulate()` zero or more times a frame and `render()` once, and `Engine::alpha()`
says how far the frame is between the last step and the next
([ADR-0032](0032-loop-fixed-step-simulation-variable-rate-rendering.md)). A world drawn at its last
step judders on any display faster than 60 Hz. Drawing between steps needs the state before the
last step as well as after it. Every game needs this, and a game's own component types should not
have to change shape to get it.

## Decision

The previous step is a component, `v3d::ecs::Previous<T>`, which a game fills for each moving type
with one `snapshot<T>()` call at the top of `simulate()`. A renderer reads `interpolated<T>()`,
which blends the two through an `interpolate()` function written beside `T`. A type with no
`interpolate()` fails to compile rather than being drawn snapped.

## Alternatives

### A previous and a current value inside the component
- **For**: the simplest to write. There is no second component, and one lookup reads both values.
- **Against**: every type that is interpolated changes shape, including each game's own. The
  pair must be shifted every step whether or not the thing moved, or a stopped thing is drawn
  still sliding.
- **Rejected because**: it puts the bookkeeping in every writer of every moving type, and one of
  them will forget it.

### Snapshot the whole registry, or keep two registries
- **For**: nothing to do per type, and anything drawn could be interpolated.
- **Against**: it copies everything every step, including what never moves and what is never
  drawn, and EnTT has no cheap registry copy. Two registries also make every entity handle
  ambiguous.
- **Rejected because**: it pays for every component to interpolate a few.

## Consequences

- **Gains**:
  - A game that interpolates is visibly smoother above 60 Hz.
  - Game components keep their shape, and a type opts in by having an `interpolate()`.
  - The mechanism is generic over `T`, so it works over whatever component carries a position.
- **Costs**:
  - A second component on every moving entity, and a `snapshot<T>()` call per type that a game
    has to remember.
  - `T` must be copyable.
  - An entity created during a step has no previous value, so `interpolated` has to check for
    one and draws the current value when there is none.
  - A teleport not followed by `settle<T>()` is drawn sliding for one frame, and only a person
    watching notices.
- **Revisit when**: snapshotting every moving entity each step shows in a profile, or a moving
  thing needs more history than one step.
