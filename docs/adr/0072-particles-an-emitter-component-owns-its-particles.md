# ADR-0072: Particles: an emitter component owns its particles

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[Effects](../plans/completed/Effects.md) takes up particles and weather, and nothing in the api spawns,
moves or draws either. Three things need settling before it is written. The first is where the
code lives. The second is how a particle is drawn between steps so that
[ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md) holds. The third is whether the api
decides when an effect fires. cozy calls its weather and fires decoration and steps them in
`tick()`, because nothing about them needs to be reproducible. retcon wants muzzle flashes,
impacts and fire in combat, which it seeds, and its random source is now `type::Random`.
[ADR-0070](0070-animation-cpu-sampling-playback-on-the-fixed-step.md) settled the same questions for
animation.

## Decision

**An emitter's description, its particles and the step that moves them live in `api/type`**,
in `type::effect`. They are glm and arithmetic, seeded by a `type::Random` the state owns.

**The emitter is a component in `api/ecs`, and it owns its particles.** A walk steps every
emitter from its entity's `Transform`, so an effect carried by an entity moves with it. **A
particle keeps its own previous position**, which is what draws it between steps. The
component is not snapshotted.

**What a particle looks like is a component in `api/render`**, a texture and a sprite clip,
beside `Sprite`. An entity is drawn from a transform and a component per kind of drawing
([ADR-0063](0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md)). The particles
go into the same depth order as the sprites.

**Emitters are stepped on the fixed step by default** ([ADR-0032](0032-loop-fixed-step-simulation-variable-rate-rendering.md)).
The step takes seconds and does not know where it is called from. A game that does not need its
effects reproduced may step them from `tick()` and draw them at an alpha of one.

**When an effect fires, and what the weather is, is the game's.** The api offers a rate, a burst
and a weather intensity, and nothing that chooses them.

## Alternatives Considered

### Alternative 1: A particle per entity
- **Pros**: It reuses `Transform`, `Sprite`, the snapshot and `sprites()` with nothing new.
- **Cons**: A shower of rain creates and destroys hundreds of entities a second. Each one pays a
  snapshot and a lookup per component every step, and a particle's age and life have nowhere
  to go but another component.
- **Why not**: Particles are many, short-lived and identical in kind. That is an array, not a
  set of entities.

### Alternative 2: The emitter snapshotted through `Previous<Emitter>`
- **Pros**: It is ADR-0060 unchanged, and `interpolated()` draws the emitter as it does a
  transform.
- **Cons**: The snapshot copies every particle every step to keep one position each. A
  particle born this step has nothing in the copy to interpolate from.
- **Why not**: One `glm::vec3` per particle holds the same information, and a newborn particle
  sets it to where it was born.

### Alternative 3: A particle system the game holds outside the registry
- **Pros**: No component. A game that has no ECS can still use it.
- **Cons**: It does not follow an entity, so a torch carried by a character is placed by hand
  each step. Both games draw through the registry, and a game would write the walk the api
  offers.
- **Why not**: The description, the state and the step are plain `api/type` code, so a game
  without a registry can still call them. The component adds the walk, and nothing more.

### Alternative 4: A library of its own, `api/effects`
- **Pros**: Everything effect-shaped in one place.
- **Cons**: It is one more manifest entry
  ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)) for code that needs
  nothing beyond what `api/type`, `api/ecs` and `api/render` already have. The split across
  those three would remain inside it.
- **Why not**: ADR-0070 weighed the same library for animation and declined it for the same
  reason.

### Alternative 5: Particles on the device
- **Pros**: A compute pass steps tens of thousands of particles for nothing on the cpu.
- **Cons**: Nothing about it can be tested headless, and a particle could not sort among the
  sprites it stands between.
- **Why not**: Neither game's counts need it, and the roadmap rules it out.

## Consequences

### Positive
- A seeded emitter gives the same particles on every run and every standard library, so the
  whole simulation is tested headless.
- A particle sorts among sprites, so a fire stands between the characters around it.
- An effect follows the entity that carries it with nothing written by the game.

### Negative
- Every particle carries a previous position it would not need if it were stepped per frame.
- A game writes its own triggers: when a burst fires, and what the weather does.
- A particle's look is one texture and one clip per emitter. Smoke rising from a flame is a
  second emitter on the same entity, or on a child of it.

### Risks
- An emitter stepped from `tick()` with a long frame spawns a burst of what the rate owed and
  moves it in one large step. That is the game's choice, and the fixed step is the escape
  hatch.
- A large emitter copies its particles when the component is moved. If that shows in a profile,
  the state can hold its particles behind a pointer without changing the step.
