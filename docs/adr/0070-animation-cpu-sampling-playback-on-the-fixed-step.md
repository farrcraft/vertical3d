# ADR-0070: Animation: CPU sampling, playback on the fixed step

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

A model can carry a skeleton
([ADR-0069](0069-models-material-parts-over-one-vertex-buffer.md)), and nothing reads a
clip, samples a pose or advances one. [SkeletalAnimation](../plans/completed/SkeletalAnimation.md) needs
three things settled before writing that: where the code lives, what is kept per step so that
[ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md) can draw a character between steps,
and whether the api decides which clip plays. retcon, the consumer, has about twelve
characters on screen and no state machine. Its clip selectors would be its own components:
awareness, intent, the result of a move. [MotionAndQueries](../plans/completed/MotionAndQueries.md#step-6--a-sprite-clip)
held a sprite clip's clock until a second consumer needed it, and this is that consumer.

## Decision

**The clock, clips, poses, sampling and blending live in `api/type`**, in
`type::animation`. They are glm and arithmetic, readable by either renderer
([ADR-0024](0024-api-type-serves-both-renderers.md)). The clock keeps no time of its own, so a
sprite clip can be written over it later.

**Playback is a component in `api/ecs`**, advanced in `simulate()` on the fixed step
([ADR-0032](0032-loop-fixed-step-simulation-variable-rate-rendering.md)). It has an `interpolate()`, so
ADR-0060's snapshot draws it between steps. **What is interpolated is the playback state** (a
clip, an unwrapped time and a fade), **not the pose.** The pose is sampled once a frame, at
draw time, from the interpolated state.

**Which clip plays, and when it gives way to another, is the game's.** The api offers a way to
play a clip with a fade, and nothing that chooses one.

## Alternatives Considered

### Alternative 1: A library of its own, `api/animation`
- **Pros**: One place to look for everything animation-shaped, and a name that says so.
- **Cons**: One more manifest entry
  ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)) for code that has no
  dependency beyond what `api/type` and `api/ecs` already carry. The playback component would
  still belong beside `Transform`, so the split would remain.
- **Why not**: The data is a value type and the playback is a component, and both already have
  a home.

### Alternative 2: A pose per step, interpolated
- **Pros**: The renderer reads poses and nothing else, and sampling happens once per step rather
  than once per frame.
- **Cons**: A `Previous` of every joint's transform, about 65 of them for a Mixamo rig, for every
  character. Interpolating that means slerping each joint, which is a blend of two whole poses
  every frame anyway.
- **Why not**: It costs a copy of every pose per step to save a sample that costs about the same
  as the blend it replaces.

### Alternative 3: Sampling on the device
- **Pros**: It scales to thousands of characters. The cpu does no per-joint work.
- **Cons**: Clip data goes into buffers, and a compute pass writes the palette before the lit
  passes read it. None of it can be tested headless.
- **Why not**: Twelve characters do not need it. It can replace the cpu sampler behind the same
  palette if a count ever does.

### Alternative 4: A state machine in the api
- **Pros**: Every game would get transitions, conditions and blend trees without writing them.
- **Cons**: No game here has written one, so its edges would be guessed. retcon's turn-based
  moves and its real-time horde want different rules, and its selectors are its own
  components.
- **Why not**: The roadmap asked for this line to be drawn after a game had written one. None
  has.

## Consequences

### Positive
- The clock is written once, for the sprite clip and any later timeline as well.
- A character is drawn between steps by the same mechanism a transform is, with nothing new in
  the loop.
- Everything but the palette's upload is tested headless.

### Negative
- Each frame samples every animated character, even one whose state did not change.
- A game writes its own selection: which clip, when, and how long a fade.
- Interpolating across a change of clip has no meaningful halfway point. The frame after a
  `play()` draws the new clip, and only the fade smooths it.

### Risks
- Time kept unwrapped grows without bound on a clip that loops forever. At the fixed step, a
  float stops resolving a millisecond after a little over two hours. The component can
  rebase its time by whole loops when it advances, which changes nothing the interpolation
  sees.
