# ADR-0070: Animation: CPU sampling, playback on the fixed step

**Status**: accepted
**Date**: 2026-10-03
**Documented in**: [api/ECS.md](../api/ECS.md), [api/Types.md](../api/Types.md)

## Context

A model can carry a skeleton and clips
([ADR-0069](0069-models-material-parts-over-one-vertex-buffer.md)), and something has to advance
a clip and sample a pose from it. Simulation runs on a fixed step
and drawing runs once a frame, so a character has to be drawn between steps the way a transform
is ([ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md)). A skeleton has dozens of
joints, and a game shows at most a few dozen animated characters. The rules for choosing a clip
differ between games and come from each game's own components.

## Decision

What is interpolated between steps is the playback state, not the pose: playback is an `api/ecs`
component advanced in `simulate()`, and each frame the CPU samples a pose from the interpolated
state. Clips, poses, sampling and blending are value code in `type::animation`, in `api/type`.
Which clip plays is the game's, and the api offers only a way to play a clip with a fade.

## Alternatives

### A pose per step, interpolated
- **For**: the renderer reads poses and nothing else, and sampling happens once a step rather
  than once a frame.
- **Against**: every character keeps a previous copy of every joint's transform. Interpolating
  them means blending two whole poses every frame anyway.
- **Rejected because**: it copies every pose each step to save a sample that costs about the same
  as the blend that replaces it.

### Sampling on the GPU
- **For**: it scales to thousands of characters, with no per-joint work on the CPU.
- **Against**: clip data goes into device buffers, and a compute pass writes the joint matrices
  before the lit passes read them. None of it can be tested headless.
- **Rejected because**: a few dozen characters do not need it. It can replace the CPU sampler
  behind the same joint matrices if a count ever does.

### A library of its own, `api/animation`
- **For**: one place for everything animation-shaped.
- **Against**: one more library to select
  ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)) for code that needs nothing
  beyond `api/type` and `api/ecs`. The playback component still belongs beside `Transform`, so
  the split would remain.
- **Rejected because**: the data is a value type and the playback is a component, and both
  already have a home.

### A state machine in the api
- **For**: every game would get transitions, conditions and blend trees without writing them.
- **Against**: no game here has written one, so its shape would be guessed. Turn-based moves and
  real-time crowds want different rules.
- **Rejected because**: the line between the api and the game should be drawn after a game has
  written one.

## Consequences

- **Gains**:
  - A character is drawn between steps by the same mechanism as a transform, with nothing new in
    the loop.
  - The clock is written once and also serves sprite clips.
  - Everything except the upload of joint matrices is tested headless.
- **Costs**:
  - Every animated character is sampled every frame, even when its state did not change.
  - Each game writes its own clip selection: which clip, when, and how long a fade.
  - Interpolating across a change of clip has no meaningful halfway point, so only the fade smooths
    it.
  - An unwrapped time grows without limit on a looping clip, and a float loses millisecond
    resolution after about two hours.
- **Revisit when**: the character count makes CPU sampling show in a profile, or two games write
  the same clip-selection logic.
