# ADR-0072: Particles: an emitter component owns its particles

**Status**: accepted
**Date**: 2026-10-04
**Documented in**: [api/ECS.md](../api/ECS.md)

## Context

Games need particle effects and weather: rain, fire, impacts and muzzle flashes. Particles are
many, short-lived and alike, and an effect often follows the entity that carries it, such as a
torch in a character's hand. Some games treat effects as decoration that need not be reproducible.
Others seed combat effects and need them identical on every run. A particle has to be drawn between
steps like anything else ([ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md)), and
has to sort among the sprites it stands between.

## Decision

An emitter is a component in `api/ecs` that owns its particles as an array, is stepped from its
entity's `Transform`, and keeps a previous position in each particle for drawing between steps.
The emitter's description, its seeded state and its step are value code in `type::effect`, and a
particle's look is a component in `api/render` beside `Sprite`, drawn in the same depth order
([ADR-0063](0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md)). When an effect fires
and what the weather does are the game's.

## Alternatives

### A particle per entity
- **For**: it reuses `Transform`, `Sprite`, the snapshot and the sprite walk with nothing new.
- **Against**: a shower of rain creates and destroys hundreds of entities a second, each paying a
  snapshot and a lookup per component every step. A particle's age and lifetime need another
  component.
- **Rejected because**: many short-lived, identical things are an array, not a set of entities.

### Snapshot the emitter through `Previous<Emitter>`
- **For**: it is [ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md) unchanged, and
  the emitter is drawn between steps as a transform is.
- **Against**: the snapshot copies every particle every step to keep one position each, and a
  particle born this step has nothing in the copy to interpolate from.
- **Rejected because**: one position per particle holds the same information, and a newborn
  particle sets it to where it was born.

### A particle system the game holds outside the registry
- **For**: no component, so a game with no ECS can use it.
- **Against**: it does not follow an entity, so a carried effect is placed by hand each step, and
  a game writes the walk the api would otherwise offer.
- **Rejected because**: the step is plain `api/type` code that a game without a registry can
  already call. The component adds only the walk.

### A library of its own, `api/effects`
- **For**: everything effect-shaped in one place.
- **Against**: one more library to select
  ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)) for code that needs nothing
  beyond `api/type`, `api/ecs` and `api/render`, and the split across those three would remain
  inside it.
- **Rejected because**: [ADR-0070](0070-animation-cpu-sampling-playback-on-the-fixed-step.md)
  declined the same library for animation for the same reason.

### Particles on the GPU
- **For**: a compute pass steps tens of thousands of particles at no CPU cost.
- **Against**: nothing about it can be tested headless, and a particle could not sort among the
  sprites around it.
- **Rejected because**: no game here needs counts that large.

## Consequences

- **Gains**:
  - A seeded emitter gives the same particles on every run and every standard library, so the
    whole simulation is tested headless.
  - A particle sorts among sprites, so a fire stands between the characters around it.
  - An effect follows the entity that carries it with nothing written by the game.
- **Costs**:
  - Every particle carries a previous position.
  - A particle's look is one texture and one clip per emitter. Smoke above a flame is a second
    emitter.
  - The emitter component copies its particles when it is moved.
  - A game that steps emitters from `tick()` instead of the fixed step gives up reproducibility,
    and a long frame spawns a burst and moves it in one large step.
- **Revisit when**: moving emitters shows in a profile, which would put the particles behind a
  pointer, or a game needs particle counts only the GPU can step.
