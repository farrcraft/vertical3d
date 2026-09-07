# ADR-0032: Game Loop — The Loop Simulates At A Fixed Step And Renders At A Variable One

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`Engine::eventLoop()` measured a frame with `SDL_GetTicks()`, handed the difference to
`tick(delta)`, and that was the whole of the timing contract. Five subclasses answered it five
ways, and two of them answered it wrong: `PongScene::tick()` and odyssey's `Movement::tick()`
both take no argument at all, so both advance their worlds one increment per *frame* and run
faster on a faster machine. Tetris and voxel scale by the delta and are frame-rate aware. The
editor is not a simulation. When every subclass has to solve the same problem and most of them
get it wrong, the problem belongs to the base class.

The clock underneath it was a second defect. `SDL_GetTicks()` returns whole milliseconds, so a
frame faster than 1 ms reports zero elapsed time and a simulation written against `delta` stops
advancing entirely; a 144 Hz frame of ~6.9 ms quantises to 6 or 7, which is 14% jitter every
frame; and 60 Hz is 16.67 ms, which is not representable — the choices are 16 ms and 17 ms, and
neither is the number anyone means.

Variable-step simulation is not merely imprecise, it is not reproducible. The same inputs
produce different results depending on how the frames happened to land, which is what makes a
physics bug that only appears on one machine, and what makes a recorded input sequence worth
nothing. Fixing the clock alone does not fix that; only a fixed step does.

## Decision

**The loop accumulates real time and drains it in fixed steps.** `eventLoop()` measures each
frame with `SDL_GetTicksNS()`, clamps it, adds it to an accumulator, and calls a new
`virtual bool simulate(unsigned int step)` once per whole step the accumulator holds.
`render()` is still called once per frame.

**The step is a constant, 60 Hz.** It is not read from config and not settable at runtime. Two
machines that disagree about the step disagree about physics, and a replay or a save is then
only correct on the machine that wrote it.

**A frame is clamped to 250 ms, and the excess is dropped.** A window drag, a breakpoint or a
stalled load produces a delta measured in seconds; without a ceiling that is hundreds of steps
in one frame, each of which makes the next frame later still. Dropping simulated time is the
correct failure: the world runs slow for a moment, rather than the loop spiralling.

**`simulate()` receives its step as a `float` in seconds, and `tick()` keeps its `unsigned int`
milliseconds.** Two units in one class is a cost paid deliberately. A millisecond step cannot
express 60 Hz — 16.67 is not an integer, and 16 is 62.5 Hz — so an integer step either lies
about how much time the loop drained or moves the rate to whatever milliseconds can say.
Seconds are also the units simulation is written in: a velocity is units per second, so
`position += velocity * step` needs no conversion. That the two virtuals differ in type as well
as in meaning is a second benefit, because passing one where the other belongs does not compile.

**`tick(delta)` keeps its signature and its place in the loop**, called once per frame before
the steps. Three apps have per-frame work that is not simulation — reading input state,
animating a UI, updating a camera against the mouse — and a loop with only a fixed step has
nowhere to put it. `simulate()` is what is added, not what replaces it.

**The accumulator's remainder is exposed as an interpolation alpha**, `Engine::alpha()`,
returning the fraction of a step left over when rendering begins. Nothing reads it yet.
Rendering between two simulation states is the entire reason to separate them, and retrofitting
the alpha once every `render()` in the tree has been written against snapped positions is a
rewrite of every draw call, not an addition.

**The accumulator is its own type**, `v3d::engine::Accumulator`, rather than private state on
`Engine`. It is the only part of the loop that is pure logic, and a type is testable without a
window, a device or a config tree.

## Alternatives Considered

### Alternative 1: A fixed step in the loop, with `tick()` kept beside it — **chosen**
- **Pros**: Additive. Every app compiles untouched and behaves identically the moment it lands,
  because nothing overrides `simulate()` yet, which lets the capability and the two-app bug fix
  be separate commits. The simulation becomes reproducible for every app at once.
- **Cons**: Two virtuals with adjacent-sounding names, and a subclass author has to know which
  one their work belongs in. The loop is meaningfully more complicated than a subtraction.
- **Why not**: n/a — chosen.

### Alternative 2: Fix the clock only — `SDL_GetTicksNS()`, still a variable step
- **Pros**: A few lines. No new virtual, no new type, nothing for a subclass to learn.
- **Cons**: Answers the resolution defect and none of the rest. A variable step is still not
  reproducible, still integrates differently at 30 and 300 fps, and still leaves each app to
  scale by the delta correctly on its own — which is the thing two apps already failed at.
- **Why not**: It fixes the symptom that is easiest to measure and not the one that costs.

### Alternative 3: Replace `tick(delta)` with `simulate(step)` outright
- **Pros**: One virtual, one timing model, nothing to choose between. No app can be on the
  wrong one because there is no wrong one.
- **Cons**: Breaks five subclasses in the same commit as the capability lands, and gives
  per-frame work that genuinely is per-frame — input polling, UI animation, camera smoothing —
  nowhere to go but a fixed step it does not want to run at.
- **Why not**: The two kinds of work are actually different, so the loop should have both.

### Alternative 4: `simulate(unsigned int step)` in milliseconds, matching `tick()`
- **Pros**: One unit across the whole loop. A subclass author learns milliseconds once.
- **Cons**: 60 Hz is 16.67 ms and is not representable. The loop would either pass 16 while
  draining 16.67 — an integrator told a sixth less time than it was given, which is a slow drift
  in every simulation and impossible to attribute — or the rate becomes 62.5 Hz because that is
  what a whole number of milliseconds can express.
- **Why not**: It reproduces the quantisation defect this record exists to remove, one layer
  further in where it is harder to see.

### Alternative 5: `simulate()` with no argument, the step read from the constant
- **Pros**: The cleanest possible signature, and it makes the fact that the step never varies
  impossible to miss.
- **Cons**: Every app that actually needs the number reaches for `Accumulator::seconds`, which
  is most of them, so the parameter is removed from the signature and added to every body.
- **Why not**: The number is wanted at nearly every call site. Passing it is what a parameter is
  for.

### Alternative 6: A configurable rate, read from config
- **Pros**: A slower machine could be given a cheaper step; a physics-heavy app could ask for a
  finer one.
- **Cons**: The rate becomes part of every simulation's meaning. A save, a replay or a network
  peer is then only correct against the config that produced it, and the failure is silent and
  nearly impossible to attribute.
- **Why not**: The constant is one line, and this ADR is the place it changes. That is enough
  configurability for something that changes once.

## Consequences

### Positive
- Pong and odyssey stop being frame-rate dependent, which is the bug this record exists to
  license fixing.
- The same inputs produce the same simulation whatever the frame pacing was, so a scene test
  can assert a duration rather than a frame count.
- The accumulator is testable in isolation: scripted delta sequences assert total simulated
  time, step count, and that the clamp bounds the worst frame, with no window and no device.
- Steps-per-frame is a free health metric. It sits at 0 or 1 with occasional 2s on a healthy
  frame; a sustained 3 or more means the clamp is doing real work.

### Negative
- Two virtuals, and nothing enforces that a subclass put its work in the right one. Simulation
  left in `tick()` is frame-rate dependent exactly as before, and compiles.
- Rendering without reading `alpha()` shows the world snapped to the last completed step, so
  motion is quantised to 60 Hz however fast the display is. That is what every app does today,
  and the alpha is there for whoever fixes it first.
- Two units live in one class, and a reader has to notice that `tick` is milliseconds and
  `simulate` is seconds. The types differ, so the compiler catches the mistake, but the reader
  meets it first.
- The nanosecond step for 60 Hz is 16,666,666 ns, which is 2/3 ns short. That is 40 ns of drift
  per simulated second and does not accumulate anywhere that matters, but the step is not
  exactly 60 Hz.

### Risks
- An app whose `simulate()` is slower than the step it is given can never catch up, and the
  clamp turns that into permanent slow motion rather than a hang. It is the right failure, but
  it is a quiet one — the frame statistics are how it gets noticed.
- Tetris and voxel stay on the variable `tick(delta)`. They are not wrong, so there is no
  forcing reason to move them, but the tree now has two timing models in it and the wrong one
  is the easier to copy.
