# The loop

What the engine calls each frame, and which call a piece of work belongs in.

## The loop

`eventLoop()` repeats these steps until something calls `quit()`:

1. **Poll events.** Each SDL event is routed as described in
   [Event routing](Input.md#event-routing). If a handler called `quit()`, the loop stops here and
   nothing else runs for that frame.
2. **`tick(delta)`**, once, with the time since the last frame in whole milliseconds.
3. **`simulate(step)`**, zero or more times: once for each whole fixed step the elapsed time
   covers. `step` is always `1/60` of a second.
4. **`render()`**, once.
5. **Clear the input edges**, so the next frame starts with no presses or releases recorded.

If `tick`, `simulate` or `render` returns false, the loop ends and the run fails.

### tick and simulate

The two virtual functions take different units on purpose:

| | Called | Argument | Use it for |
|---|---|---|---|
| `tick(unsigned int delta)` | once per frame | milliseconds since the last frame | input state, ui animation, camera smoothing, per-frame budgets |
| `simulate(float step)` | once per fixed step | seconds, always `Accumulator::seconds` | anything that changes the game world |

**Simulation goes in `simulate()`, not in `tick()`.** Code in `simulate()` gives the same
result at any frame rate. Code in `tick()` does not. Nothing enforces this, and simulation left
in `tick()` compiles and runs, but its results depend on the frame rate. The differing argument
types mean that passing one function's time to the other does not compile.

Write simulation in seconds: a velocity is units per second, so `position += velocity * step`
needs no conversion.

voxel is a useful example because it overrides both. Its world steps in `simulate()`. Chunk
remeshing, which has a budget of chunks per frame, and the debug overlay's frame-time average
stay in `tick()`. Neither is simulation, and neither should run twice on a slow frame.

### The fixed step

`v3d::engine::Accumulator` in [api/engine/Accumulator.h](../../../api/engine/Accumulator.h) does
the arithmetic:

- The loop measures each frame in nanoseconds with `SDL_GetTicksNS()`.
- **The step is 60 Hz and is a constant.** It is not read from config and cannot change at run
  time. `Accumulator::step` is 16,666,666 ns and `Accumulator::seconds` is `1.0f / 60.0f`.
- **A frame longer than 250 ms is clamped to 250 ms, and the excess time is dropped.** A window
  drag or a breakpoint therefore slows the world for a moment, rather than making the loop run
  hundreds of steps to catch up.
- Time that does not fill a whole step is carried to the next frame.

An app whose `simulate()` takes longer than the step it is given can never catch up. The clamp
turns that into permanent slow motion rather than a hang. The frame statistics show it.

### Alpha

`Engine::alpha()` is the fraction of a step that has elapsed but not yet been simulated, in
`[0, 1)`. A renderer that draws between the last two simulated states blends by it. Without
it, motion is drawn snapped to the last 60 Hz step, which judders on a faster display.
[ECS.md](../ECS.md#drawing-between-steps) describes how to keep the previous state and draw with
`alpha()`.

### Frame statistics

`Engine::statistics()` returns an `engine::Statistics`
([api/engine/Statistics.h](../../../api/engine/Statistics.h)):

- `last()` — the last frame's length in nanoseconds, before clamping.
- `mean()` — the mean frame length over the last 64 frames (`Statistics::window`).
- `steps()` — how many simulation steps the last frame ran.
- `frames()` — frames since the loop started.
- `rows()` — named timings. Each row has the last frame's time and the mean over the window.

**Steps per frame is the number to watch.** A healthy frame runs 0 or 1 steps, with an
occasional 2. A sustained 3 or more means the clamp is dropping time and something cannot keep
up.

To time part of a frame, hold the scope that `measure(name)` returns for as long as the work
runs. Two scopes with the same name in one frame add together.

```cpp
{
    auto timing = measure("remesh");
    remeshChunks();
}
```

`ui::shell::StatisticsOverlay` draws these numbers. See [ui/](../ui/README.md).

Background: [ADR-0032](../../adr/0032-loop-fixed-step-simulation-variable-rate-rendering.md)
