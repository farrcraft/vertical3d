# Game Loop Foundations — A Fixed Simulation Step, Window Focus, And The Camera Loader

Drafted 2026-09-05, staged into this tree and **closed** on 2026-09-06, all six steps done.
Three things the game loop owes every app that subclasses it, and one that is nearly free once
the first lands. None of them is new capability — each is a gap that every
app has worked around separately, or has failed to.

The prompting case is a game being built against the api in another repository, which needed a
fixed timestep, a focus event and a camera loaded from config, and found that the first two do not
exist and the third belongs to the editor. But nothing below is for that app's benefit. **Step 3 is
a bug fix in two apps that are in this tree today.**

## Context

`Engine::eventLoop()` measures a frame with `SDL_GetTicks()`, hands the difference to
`tick(delta)`, and that is the whole of the timing contract. What each app does with it:

| App | `tick(delta)` |
|---|---|
| **pong** | [`PongScene::tick()`](../../../pong/src/PongScene.cxx) takes **no argument**. The delta is received by `PongEngine` and dropped. Ball and paddle speed are frame-rate dependent. |
| **odyssey** | [`Movement::tick()`](../../../odyssey/system/Movement.cpp) takes **no argument**. Same. |
| **tetris** | Passes `delta` to `TetrisScene::tick(delta)`. Frame-rate aware, variable step. |
| **voxel** | Passes `delta` to the scene and the renderer. Same. |
| **editor** | Not a simulation. |

Five subclasses, five different answers, two of them wrong. That is the signature of something the
base class should have been doing.

The millisecond resolution is a second defect underneath the first. `SDL_GetTicks()` returns whole
milliseconds, so:

- a frame faster than 1 ms reports **zero** elapsed time, and a simulation written against `delta`
  stops advancing entirely;
- at 144 Hz a frame is ~6.9 ms and quantises to 6 or 7 — roughly 14% jitter, every frame;
- 60 Hz is 16.67 ms, which is not representable at all. The choices are 16 ms (62.5 Hz) and 17 ms
  (58.8 Hz), and neither is the number anyone means.

`SDL_GetTicksNS()` has been there the whole time. SDL3 is already `PRIVATE` to `v3dlib_engine`, so
using it changes no interface and no consumer.

Two smaller gaps travel with this. `SDL_EVENT_WINDOW_FOCUS_LOST` and `SDL_EVENT_WINDOW_FOCUS_GAINED`
reach the loop's event switch and fall through its `default:` — discarded, where the resize a case
above them dispatches a `v3d::event::WindowResize`. And `config::Type::Camera` is a config type the api
defines whose only loader, `CameraProfiles`, lives in `vertical3d/src/view/` in namespace
`v3d::editor`.

## Decisions

Recorded in [adr/](../../adr/), not here.

| ADR | Decision |
|---|---|
| [0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md) | The loop simulates at a fixed step and renders at a variable one — written by step 1 |
| [0012](../../adr/0012-camera-builds-vulkan-clip-space.md) | `v3d::type::Camera` builds Vulkan clip space — unchanged; step 5's loader produces profiles for it |
| [0016](../../adr/0016-undo-records-what-has-already-happened.md), [0017](../../adr/0017-a-command-is-a-name-in-a-context.md) | One consumer is not a library, and it moves when a second app wants it — **step 5 applies it**, and needs no record of its own |

`0026` is a deliberate gap, reserved by [the open shading plan](OfflineRenderingPhase3.md) —
[adr/README.md](../../adr/README.md) says so above its table. 0028 through 0031 have been taken since
this plan was drafted, so the record here is **0032**.

## What blocks what

```
1. ADR-0032 ──> 2. the loop accumulates ──┬──> 3. pong and odyssey move onto it
                                          └──> 6. frame statistics

4. window focus is an event        (independent)
5. the camera loader moves          (independent)
```

Steps 4 and 5 touch nothing steps 1–3 touch and can land in any order, before or after. Step 2 is
additive — every existing app compiles and behaves identically the moment it lands — which is what
makes step 3 a separate commit rather than the same one. **That separation is the point:** step 2 is
a capability and step 3 is a behaviour change to two apps, and one concern per commit means they do
not travel together.

## Steps

### Step 1 — ADR-0032, the loop simulates at a fixed step

**Done.** [ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md), accepted. It settles
one thing this plan had not seen: `simulate()` takes its step as a `float` in seconds rather
than the `unsigned int` milliseconds written below, because 16.67 ms is not an integer and an
integer step would either lie about the time it drained or move the rate to 62.5 Hz. `tick()`
keeps its milliseconds, and the differing types are what stops the two being confused.

The record comes first, per [sdlc.md](../../sdlc.md): written before or alongside, not after.

What it has to settle, because each constrains later work and none is obvious from the diff:

- **The rate**, and whether it is a constant or configurable. A constant is one fewer thing to get
  wrong; configurable invites two machines that disagree about physics, and a save file that is
  only correct on the machine that wrote it.
- **The clamp**, and that dropping simulated time is the correct failure. A window drag or a
  breakpoint produces a delta of seconds; without a ceiling that is thousands of steps in one frame,
  each making the next frame later. 250 ms is a reasonable bound.
- **Why `tick(delta)` is kept alongside `simulate(step)`** rather than replaced. Three apps have
  per-frame work that is not simulation, and a loop with only a fixed step has nowhere to put it.
- **That the accumulator's remainder is exposed as an interpolation alpha**, even with no reader
  yet. Rendering between two simulation states is the entire reason to separate them; retrofitting
  the alpha after `render()` has been written against snapped positions is a rewrite of every draw
  call in the tree.

### Step 2 — `eventLoop()` accumulates, and `simulate()` is the new virtual

**Done.** `v3d::engine::Accumulator` holds the step, the clamp and the alpha;
`Engine::simulate(float step)` is the new virtual and `alpha()` reads the remainder. Seven
cases in `api/engine/tests/AccumulatorTest.cpp`.

In [`api/engine/Engine.h`](../../../api/engine/Engine.h) and
[`Engine.cpp`](../../../api/engine/Engine.cpp).

`eventLoop()` measures the frame with `SDL_GetTicksNS()`, clamps it, adds it to an accumulator, and
drains the accumulator in fixed steps calling a new `virtual bool simulate(unsigned int step)`
whose base implementation returns true and does nothing. `tick(delta)` keeps its exact signature and
its position in the loop.

`SDL_NS_PER_SECOND` and the `SDL_MS_TO_NS` family are already in `SDL_timer.h`; the step is exact in
nanoseconds and there is no reason to round it. Keep `tick`'s parameter in whole milliseconds —
changing that signature would break five subclasses for no gain.

Nothing in the public headers gains an SDL type: the accumulator and the clock are implementation
detail inside `Engine.cpp`, and `v3dlib_engine` already links `SDL3::SDL3` `PRIVATE` for exactly
this reason.

**Additive.** Every app compiles untouched and runs identically, because nothing overrides
`simulate()` yet.

**Tests.** [`api/engine/tests/`](../../../api/engine/tests/) already has a `TestMain`, an `EngineTest`
and a `fixtures/` tree of config scenarios. The accumulator is pure logic and needs no window or
device, which makes it the most testable thing in `api/engine` and the part most worth covering:
scripted delta sequences — zeros, a multi-second stall, a 6/7 ms alternation at 144 Hz — asserting
total simulated time, step count, and that the clamp bounds the worst frame. Extracting the
accumulator as a small type is what makes this testable without standing an `Engine` up; that is
probably the right shape regardless.

### Step 3 — pong and odyssey move onto it

**Done**, and one thing wider than written: `v3d::ecs::System::tick()` became
`simulate(float step)` too, because odyssey's `Movement` reaches its step through that
interface and "tick" would otherwise have meant a frame in one place and a fixed step in
another. Two implementations and one test file.

Pong's speeds are now per second — the ball's direction is a velocity, the paddle run is 90
pixels a second, and the english a travelling paddle puts into a return is scaled to match.

The bug fix, and the reason this plan is not just infrastructure.

[`PongScene::tick()`](../../../pong/src/PongScene.cxx) gains a step parameter and `PongEngine` calls it
from `simulate()` rather than `tick()`. Same for odyssey's
[`Movement`](../../../odyssey/system/Movement.cpp) and `Engine::tick`. Both are currently advancing
their worlds by one increment per *frame*, so both run faster on a faster machine — pong's ball
speed is a function of the frame rate today.

Pong has [tests](../../../pong/tests/) covering `Ball`, `Paddle` and `PongScene`, which is a useful
place to land the assertion that the same simulated duration produces the same result whatever the
frame pacing was.

**Tetris and voxel are deliberately not moved here.** They moved on 2026-09-06 instead, and the
premise below turned out to be half wrong - voxel's `Player::tick` ignored its delta and moved a
hardcoded 0.1 seconds per frame. They already take the delta and scale by it,
so they are correct-ish and changing them is a behaviour change without a bug behind it. They move
when someone is in them for another reason; note it in [TODO.md](../../TODO.md) rather than growing
this plan.

### Step 4 — window focus is an event, like resize

**Done.** The two cases were not empty as written below — they were absent, and reached the
switch's `default:`.

`api/event/WindowFocus.h` and `.cpp`, modelled on
[`WindowResize`](../../../api/event/WindowResize.h) — twenty lines, one bool for whether focus was
gained. Add it to [`api/event/CMakeLists.txt`](../../../api/event/CMakeLists.txt), and add the two cases to
the loop's switch with a `dispatcher_->trigger(...)`, exactly as `SDL_EVENT_WINDOW_RESIZED` already
does a case above.

What it unlocks for every windowed app: auto-pause, muting on focus loss, and dropping held input so
that a key released while unfocused does not stay stuck down. Without it the only route is polling
`SDL_GetWindowFlags` — an app polling for something the loop has in hand and discards.

**While in there:** [`WindowResize.cpp`](../../../api/event/WindowResize.cpp) carries a `#pragma once`
at the top of a `.cpp`. Harmless, and a copy-paste artifact from the header. Delete it in the same
commit and do not reproduce it in `WindowFocus.cpp`.

No ADR — this is smaller than a record. A comment beside the dispatch is the right size.

### Step 5 — the camera loader moves to `api/config`

**Done**, along with its four existing test cases, which already covered the parse cases this
step asked for. `api/config` gained one link edge, `v3dlib_type`.

Move `vertical3d/src/view/CameraProfiles.h`/`.cxx` to
[`api/config/CameraProfiles.h`/`.cpp`](../../../api/config/CameraProfiles.h), namespace
`v3d::editor` → `v3d::config`. Rename the
implementation to `.cpp` to match its new neighbours, per the convention in
[CLAUDE.md](../../../CLAUDE.md) — `api/config` is `Config.cpp`.

**To `config`, not to `type`.** The instinct is `api/type`, beside `CameraProfile` itself. It is the
wrong home:

- [`api/type`](../../../api/type/CMakeLists.txt) links **only `glm::glm`**. It is the one library in
  the tree with no dependency on the rest of it.
- `CameraProfiles` names `asset::Json` and `log::Logger`.
- [`api/config`](../../../api/config/CMakeLists.txt) already links `v3dlib_log` and `v3dlib_asset`,
  `PUBLIC`, for precisely this kind of reason. It gains one edge, `v3dlib_type`, because the header
  names a `type::CameraProfile`.

Putting it in `type` would add two dependency edges to the only library that has none, to hold a
class whose whole job is parsing a config document — which is what `api/config` is for. One edge
onto a leaf library is the cheaper trade.

The editor's diff is an include path and a namespace. Its
[`data/cameras.json`](../../../vertical3d/data/cameras.json) is unchanged and remains the format;
`Controller` keeps calling `load(config_->get(v3d::config::Type::Camera))`.

**Tests.** [`api/config/tests/`](../../../api/config/tests/) already asserts that `Type::Camera`
resolves and that an absent one returns empty. Add parse cases now that there is something to parse:
a named profile round-tripping, a profile missing a name rejected, and the documented defaults
applied to a sparse entry.

No ADR: [ADR-0016](../../adr/0016-undo-records-what-has-already-happened.md) and
[ADR-0017](../../adr/0017-a-command-is-a-name-in-a-context.md) both already settle that one consumer is
not a library and that the thing moves when a second app wants it. This is that rule being applied,
not a new decision. Cite it in the commit message.

### Step 6 — frame statistics

**Done.** `v3d::engine::Statistics`, read off `Engine::statistics()`: last frame, a mean over
64 frames, and steps per frame.

Optional, and nearly free once step 2 exists, because the accumulator already holds the numbers.

Last frame time, a small rolling window, and steps-per-frame, readable from the engine. Collection
belongs in the loop because the loop is the only thing that knows; drawing stays with whoever wants
to draw it.

Steps-per-frame is the number actually worth having. It sits at 0 or 1 with occasional 2s on a
healthy frame; a sustained 3 or more means step 2's clamp is doing real work and something is too
slow to keep up.

Drop this step if the plan is running long. Nothing depends on it.

## Verification

Done on 2026-09-06: the tree builds with no warnings at `/W4 /WX`, all 23 ctest suites pass,
cpplint is clean, and both pong and the editor run with the validation layer on and nothing at
error or warning in the log — the editor's `data/cameras.json` still loads and every view
resolves its profile.

Per [sdlc.md](../../sdlc.md) §4:

- **Build.** `ninja -C out/build/x64-Debug`. Everything in the tree, since step 3 touches apps and
  step 5 touches the editor.
- **Tests.** `ctest --test-dir out/build/x64-Debug --output-on-failure`. New cases in
  `api/engine/tests` (the accumulator), `api/config/tests` (profile parsing) and `pong/tests`
  (frame-rate independence).
- **Lint.** cpplint, per `CLAUDE.md`. The tree is clean, so every finding is real. Note that the new
  files are `api/event/WindowFocus.*` and `api/config/CameraProfiles.*`.
- **Run.** Pong is the app that proves step 3: it should play identically with the frame rate
  uncapped and with a sleep injected into its render, and it does not today. The editor proves step
  5 — its seven view cameras still load. Validation layer silent in both.

## What this plan does not do

**It does not make "only a destination event is a command" structural.** Every app subscribing to
`sink<v3d::event::Event>()` receives both halves of a mapping and filters by context name rather
than by `type()`. [ADR-0017](../../adr/0017-a-command-is-a-name-in-a-context.md) already states the
convention and records the editor hitting the bug it prevents. Dispatching commands as a distinct
type so the sink cannot carry both would supersede part of that record, touch five apps, and break
the property it is built on — that a key binding and a menu item are *the same object*. Not worth
it for a guard that is one line and written down.

**It does not promote `CommandDirectory` to `api/event`.** ADR-0017's Alternative 3 rejected that
with a condition attached: *"It moves when a second app wants it."* A game with menus and bindings
naming the same commands is that second app, and one is being built — but it does not have menus
yet. The condition is not met today. When it is, the trigger is already recorded.

**It does not add a pause concept to the engine.** If the loop owns the timestep it arguably owns
whether the simulation advances, but `simulate()` simply not being called is the entire mechanism,
and the states around it are per-app.

## Open questions

Both of the first two are settled by [ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md):
the rate is a constant, and the accumulator is its own type.

The third — **do tetris and voxel eventually move onto `simulate()`?** — was carried out to
[TODO.md](../../TODO.md) and answered there on 2026-09-06: yes, and sooner than this expected.
The premise that they were "not wrong today" did not survive reading them. Tetris counted whole
milliseconds, so a step shorter than one rounded to zero and nothing fell; voxel's player
ignored its delta and moved a hardcoded tenth of a second per frame.
