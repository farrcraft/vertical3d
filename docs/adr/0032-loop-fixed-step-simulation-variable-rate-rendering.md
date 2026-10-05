# ADR-0032: Loop: fixed-step simulation, variable-rate rendering

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [api/Engine.md](../api/Engine.md)

## Context

Every app built on `engine::Engine` advances a world, and each has to make that independent of the
frame rate. Left to each app, some scale by the frame's length and some advance once per frame, so
a faster machine runs the game faster. A variable step is also not reproducible: the same inputs
give different results depending on how frames happened to land. That makes bugs that appear on
one machine only, and makes a recorded input sequence worthless. Some per-frame work, such as
reading input, animating a ui or moving a camera with the mouse, is not simulation and should run
once per frame.

## Decision

The loop accumulates real time, calls `simulate()` once for each whole 60 Hz step it holds, and
then calls `render()` once, with `alpha()` giving the fraction of a step left over. `tick()` stays
as a per-frame call, before the steps, for work that is not simulation. The rate is a constant in
code, not configuration, so every machine simulates the same world the same way.

## Alternatives

### Fix the clock only, and keep a variable step
- **For**: a few lines. No new virtual and nothing new for an app to learn.
- **Against**: it fixes the clock's resolution and nothing else. A variable step is still not
  reproducible, still integrates differently at 30 and 300 frames a second, and still leaves each
  app to scale by the delta correctly.
- **Rejected because**: it fixes the easiest symptom to measure, not the one that costs.

### Replace `tick()` with `simulate()` outright
- **For**: one virtual and one timing model, so no app can put work on the wrong one.
- **Against**: per-frame work such as input polling, ui animation and camera smoothing has nowhere
  to go but a fixed step it does not want.
- **Rejected because**: the two kinds of work are different, so the loop has both.

### `simulate()` takes whole milliseconds, like `tick()`
- **For**: one unit across the whole loop.
- **Against**: 60 Hz is 16.67 ms and cannot be expressed. The loop would either pass 16 while
  draining 16.67, a slow drift in every simulation, or run at 62.5 Hz.
- **Rejected because**: it moves the clock's quantisation into the step, where it is harder to see.

### `simulate()` takes no argument
- **For**: the cleanest signature, and it makes plain that the step never varies.
- **Against**: nearly every override needs the step's length, so each would read the constant
  itself.
- **Rejected because**: a value wanted at nearly every call site is what a parameter is for.

### A configurable rate
- **For**: a slow machine could use a cheaper step, and a physics-heavy app a finer one.
- **Against**: the rate becomes part of what every simulation means. A save, a replay or a network
  peer is then correct only against the configuration that produced it, and the failure is
  silent.
- **Rejected because**: the rate changes rarely, and changing a constant is enough for that.

## Consequences

- **Gains**:
  - Simulation is independent of frame rate, and the same inputs give the same result, so a test
    can assert a duration rather than a frame count.
  - The accumulator is tested on its own, with no window or device.
  - Steps per frame is a health measure. A sustained three or more means the loop cannot keep up.
- **Costs**:
  - There are two virtuals, and nothing makes an app put its work in the right one. Simulation left
    in `tick()` is frame-rate dependent and compiles.
  - `tick()` takes milliseconds and `simulate()` takes seconds, so a reader meets two units in
    one class.
  - An app whose `simulate()` is slower than its step can never catch up. The loop drops the
    excess, so the result is slow motion rather than a hang, and nothing but the frame statistics
    shows it.
- **Revisit when**: a game needs a different rate, or needs its simulation reproduced across
  machines bit for bit, which a fixed step alone does not guarantee for floating point.
