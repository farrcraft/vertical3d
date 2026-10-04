# ADR-0060: Interpolation — A Moving Thing's Previous Step Is A Component, Snapshotted Before Each Step

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The loop calls `simulate()` zero or more times a frame and `render()` once
([ADR-0032](0032-the-loop-simulates-at-a-fixed-step.md)), and `Engine::alpha()` says how far the
renderer is between the last step and the next. Nothing reads it, so every world in this tree
and in cozy and retcon is drawn snapped to the last 60 Hz step, which judders on any display
faster than that. Drawing between steps needs the state before the last step as well as after
it, and nothing keeps that. The question is where it lives and who keeps it: the answer is
shared by every game, and a game's own component types should not have to change shape for it.
[Motion and queries](../plans/completed/MotionAndQueries.md#step-5--a-moving-thing-keeps-its-previous-step)
is the plan that needs it.

## Decision

**The previous step is a component, `v3d::ecs::Previous<T>`, and a game snapshots it at the top
of `simulate()`.** `api/ecs` provides `snapshot<T>(registry)`, which copies every entity's `T`
into its `Previous<T>`, `settle<T>(registry, entity)` for a teleport, and
`interpolated<T>(registry, entity, alpha)`, which blends the two through an
`interpolate(const T&, const T&, float)` found by argument-dependent lookup. `api/ecs` writes
that function for `Position1D` and `Position2D`; a game writes it beside its own component.

## Alternatives Considered

### Alternative 1: A previous and a current value inside the component
- **Pros**: Simplest to write; no second component, and reading both needs one lookup.
- **Cons**: Every type that wants interpolation changes shape, including each game's own.
  The pair has to be shifted every step whether or not the thing moved, or a thing that has
  stopped is drawn still sliding toward where it stopped, and each caller has to remember that
  for every field.
- **Why not**: It puts the bookkeeping in every writer of every moving type, where it will be
  forgotten in one of them.

### Alternative 2: Snapshot the whole registry, or keep two registries
- **Pros**: Nothing per type at all; anything drawn could be interpolated.
- **Cons**: Copies everything every step, including what never moves and what is not drawn,
  and entt has no cheap registry copy. Two registries also make every entity handle ambiguous.
- **Why not**: It pays for every component to interpolate a few.

### Alternative 3: A `Previous<T>` component and a per-type snapshot — **chosen**
- **Pros**: One call per moving type at the top of the step covers every entity carrying it,
  moved or not. Game components keep their shape. A type opts in by having an `interpolate`.
- **Cons**: A second component per moving entity, and a `snapshot<T>` call per type the game
  has to remember to make.
- **Why chosen**: The one thing a game must remember is one line per type, in one place.

## Consequences

### Positive
- `alpha()` gets a reader, and a game that interpolates is visibly smoother above 60 Hz.
- The mechanism is generic over `T`, so it does not wait on what an entity's transform is,
  which is [milestone 3](../roadmap/completed/m3-RenderableComponent.md)'s decision.

### Negative
- **`T` has to be copyable.** `Position1D` and `Position2D` declare a move constructor and no
  copy, so they gain one. Each holds one value, so nothing is lost.
- An entity created during a step has no previous the first time it is drawn, and is drawn at
  its current value. That is right, but it means `interpolated` has to look for `Previous<T>`.
- A teleport left unsettled is drawn streaking for one frame. Nothing catches it but a person
  watching.

### Risks
- **A type without an `interpolate` must fail to compile rather than snap silently.** A
  `static_assert` in `interpolated` makes it fail, and is the whole guard.
- `PositionFixed2D` is a tile position and is deliberately given no `interpolate`. A game that
  wants a tile to glide between cells interpolates a position of its own, not this.
- If milestone 3 decides a transform is not a component, this still works over whatever does
  carry the position; the escape hatch is `interpolate` for that type, not a change here.
