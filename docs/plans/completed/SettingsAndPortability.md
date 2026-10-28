# Settings and Portability — What A Game's Shell Needs, And A Build Off MSVC

Drafted 2026-10-25 against `51dee3f2`. Eight findings across the build, `api/render/realtime`,
`api/audio`, `api/engine`, `api/event` and `api/ui`.

The prompting case is retcon, a game built against this api in another repository. A survey of
its roadmap found what its settings screen, pause menu, controls screen and save panel need from
this tree, and what stands between it and a build on another compiler or a portability driver.
Its handoffs live in that repository under `docs/upstream/` and are not linked here, because
their paths do not resolve from this one. Each handoff names the gate its change must bring, and
those gates are carried into the steps.

When this plan closes, an app can choose its present mode and read what it got. It can set a
volume on an FMOD bus and pause one. It can read and reset its key bindings, and list its
overrides to save them. It can draw a text field, a checkbox, a float slider and a combo with
`ui::Immediate`, ask whether the last widget is hovered, and draw a segment between any two
points on the 2D canvas. The isometric controller zooms on the wheel and can pan at the window's
edge. The api libraries compile under a compiler other than MSVC, and a Vulkan instance can see
a portability driver.

## What the survey got right, and where the tree has moved

Every cited line was checked against `51dee3f2`, 21 commits past the survey's `ab4bdea`, and
holds:

* **The build hands MSVC flags to any compiler.** `/permissive-`, `/W4` and `/w14062` are
  directory options in `CMakeLists.txt:32-43`, with no `if(MSVC)`. `/EHsc /utf-8` is on the
  `INTERFACE` of every api target through `cmake/v3dHelpers.cmake`, so it reaches every consumer.
  33 lines in 11 `api/*/CMakeLists.txt` add more, and the `set(CMAKE_CXX_FLAGS "/utf-8")` form
  replaces the directory's flags rather than adding to them.
* **Nothing in `api/` names portability.** The instance never sets
  `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`, and the device enables `VK_KHR_swapchain`
  only. A loader since SDK 1.3.216 hides a portability driver from such an instance, and a device
  that offers `VK_KHR_portability_subset` must enable it.
* **`Swapchain::choosePresentMode` takes no preference** (`vulkan/frame/Swapchain.cxx:170`). It
  picks mailbox where offered and FIFO otherwise. It is a pure static function.
* **`audio::Events` has no bus, VCA, volume or pause.** `FmodEvents` never calls `getBus` or
  `getVCA`. `audio::Engine::busGain` covers clips, not FMOD events.
* **`Engine::rebind` is public but `bindings_` is private** (`api/engine/Engine.h:303,342`).
  `event::Bindings::sources()` and `commands()` exist (`api/event/Bindings.h:70,75`) and an app
  cannot reach them. There is no reset, and no list of overrides.
* **`ui::Immediate` has no text field, checkbox, float slider or combo**, and a widget's hover is
  held in the protected `Reaction` and the private `hovered_` and `hovering_`.
* **The canvas cannot draw a segment off the axes.** `vertex()` and `quad()` are private, and the
  transform stack has no rotation.
* **`IsometricController` reads held commands only.** It has no wheel and no cursor input.

## Decisions

| Decision | Step |
|---|---|
| Build: which compilers besides MSVC the api libraries are built with, which libraries, and how CI proves it. A `clang-cl` job accepts the MSVC spellings and so proves nothing. ADR first. | 8 |
| Immediate: whether the checkbox, float slider and combo are taken beside the text field. **Taken.** Each is a widget any settings screen draws, and each is a handful of lines over `interact()` and `place()`. A consumer subclassing `Immediate` for them would repeat that in every game. | 1 |
| Canvas: a segment, not `rotate()` on the transform stack. **Segment only.** A rotation applies to clip rectangles too, and a clip resolves to an axis-aligned scissor. | 2 |
| Audio: a volume is set on a bus or a VCA, told apart by the path's `bus:/` or `vca:/` prefix. A pause is set on a bus only, since FMOD has no paused VCA. | 4 |
| Present mode: the request is an enum the app's settings can name, not a `VkPresentModeKHR`. | 5 |
| Isometric: wheel zoom and edge pan belong on the controller, as options. Edge pan is off by default, and wheel zoom acts only on a delta the app hands in. | 6 |

## What blocks what

Nothing here blocks anything else here. The order is by when retcon needs each:

- **Steps 1 and 2 come first.** Retcon's current phase wants a named save slot and a tech tree's
  connectors.
- **Steps 3 to 6 are its settings screen, controls screen and pause menu**, which follow.
- **Steps 7 and 8 are portability.** No consumer runs off Windows yet. Step 8 starts with its
  ADR.

What retcon does after each step lands is retcon's work: it bumps its pin and deletes the
handoff.

## Steps

### Step 1 — Text field, hover and settings widgets on `Immediate`

**Done.** `textField`, `checkbox`, `sliderFloat`, `combo` and `itemHovered` are on `Immediate`.
The editing moved out of `TextBox` into `input::LineEdit` and `input::edit()`, which both ui
models now call. Keys reach a field through `Immediate::press()` and `typed()`, which return
whether the field took them, and `shell::Keyboard` offers an immediate layer whatever the
retained ui did not take. It can be built with no `ui::Engine`.

A focused field does not make `capturing()` true. The engine's bindings fire before a frame
is drawn, so a query the app asks while drawing comes too late to stop a typed key. The key
is kept from the bindings when it arrives, by `onEvent()` returning what `press()` returned,
as it already is for a `TextBox`. `typing()` says whether a field has the keyboard.

- A single-line text field that edits a caller's UTF-8 string, with a caret, backspace and
  delete, and a length cap. It takes focus on a click and gives it up on Enter, Escape or a click
  elsewhere. The editing is `ui/input/Keys`'s, which `component::TextBox` already uses.
- A query for whether the widget just drawn is hovered, under the same one-frame-late rule every
  hover uses.
- A checkbox, a float slider and a combo.
- [ui/](../../api/ui/README.md) describes each.
- Tests: a typed string edits the caller's, and a focused field makes `capturing()` true, so a
  typed key does not also fire a binding. With two widgets drawn and the second hovered, the
  first answers false. Each of the three settings widgets changes its value through input, not
  by assignment.

### Step 2 — A segment on the canvas

**Done.** `Canvas` draws a segment between two points as one quad, its long sides parallel to the
segment at half the width either side. It is untextured, so it joins the open untextured batch.
`Canvas::segment()` takes a `Segment` of two points and a width, as `arc()` and `ring()` take a
struct.

- Tests: the four corners of a segment at 30 degrees match the perpendicular offset. A test of a
  horizontal segment alone would pass against a `rect()`. A segment under `translate()` and
  `scale()` lands where a `rect()` under the same transform would. A `rect()`, a segment and a
  `rect()` in a row make one batch, inside a clip and out of one.

### Step 3 — Reading, resetting and saving key bindings

**Done.** `Engine::bindings()` hands an app the `Bindings` the keys go through, not a read-only
view, because a reset and a restore change them. `Bindings` gained `rebind(command, source,
key)` for one source of several, `reset`, `overrides`, `restore` and `bound`. A rebind of a
command bound to two keys now leaves it bound to one: before, the two mappings it left were
both matched by a press, and the command fired twice.

- Reset one command's override, and reset all of them, back to the binding document.
- List the overrides, so an app can save them, and apply a saved list at start.
- `rebind()` states what it does to a command the document binds to two sources.
- [engine/](../../api/engine/README.md) states the rules.
- Tests: a rebind reads back the new source. A reset reads back the document's. Rebinding one of
  two sources leaves the other as stated. A saved list applied to a fresh engine reads back the
  same sources. A test that only checks `rebind()` returned true passes today.

### Step 4 — Bus volume and pause on `audio::Events`

**Done.** `Events::volume()` and `paused()` each have a setter and a getter that returns nothing
for a path no bank holds. The FMOD case ran against the SDK's example banks in a build
configured with `V3D_FMOD_ROOT`, on 2026-10-25. CI builds without the SDK and runs only the null
backend's case. `Events` sets a volume on a bus or VCA by path, and pauses a bus by path. Both return
whether the path exists, and a missing path is logged once per name, as a missing event is.

- The null backend returns false for both.
- [Audio.md](../../api/engine/Audio.md) states the paths and the rule.
- Tests: the null backend's answers run in CI. In a build with `V3D_FMOD_ROOT`, a volume set on a
  bus is the one read back, and a paused bus reads back paused. CI has no FMOD SDK, so the step
  records where that case ran.

### Step 5 — A requested present mode

**Done.** `Engine3D::present()` and `presenting()` take and report a `realtime::Present`. On this
machine's AMD Radeon RX 6600, which offers immediate, FIFO and FIFO relaxed but no mailbox, the
default request is granted FIFO, and the log now says so. Retcon's handoff expected mailbox here. `Engine3D` takes a present mode request and can change it at runtime, rebuilding the
chain as a resize does. It answers the mode it was granted, and the chain logs it once each time
it is built.

- The default is today's choice, mailbox and then FIFO, so no consumer changes behaviour without
  asking.
- [rendering/](../../api/rendering/README.md) states the request and the fallbacks.
- Tests: `choosePresentMode` with every request against FIFO alone, FIFO and mailbox, and all
  four modes. A test of the default alone passes today.

### Step 6 — Wheel zoom and edge pan on `IsometricController`

**Done.** `wheel()`, `edgePan()` and `cursor()` are on the controller, and `Speeds::notch` is the
wheel's step, its sign the direction.

- The controller takes a wheel delta each step and zooms by a stated step per notch, through
  `zoomBy()`, inside the orbit's clamps. The direction is an option.
- Edge pan takes the cursor, the viewport and a margin in pixels, and pans at `Speeds::pan`. It is
  off by default, and off while the app says the UI has the cursor.
- [engine/](../../api/engine/README.md) states both.
- Tests, with no device: a delta of +1 and -1 moves `zoom()` by the step and stops at the clamp.
  A cursor in the margin for one step moves `target()` by `pan × step`, and one outside it, or
  over the UI, does not.

### Step 7 — An instance that can see a portability driver

**Done.** `Instance::enable()` and `Device::extensions()` make the two choices from what is
offered. Selection already refused a device without dynamic rendering or synchronization2, and
now logs each device it passes over with what it lacks. This machine's loader offers portability
enumeration, so every instance here now has it on, and the device suite and its reference
pictures are unchanged. The choice of instance extensions and flags, and of device extensions, is a function
of what is offered.

- The instance enables `VK_KHR_portability_enumeration` and sets the enumerate-portability flag
  when the loader offers it. The device enables `VK_KHR_portability_subset` when the physical
  device offers it.
- Device selection refuses a device that lacks dynamic rendering or synchronization2, and names
  what it lacks.
- The instance's log line says whether portability enumeration is on.
- Tests: both branches of each choice, offered and not offered. No CI runner has a portability
  driver, so the test of the choice is the gate. The device suite on a conformant driver is the
  check that nothing changed there.

### Step 8 — The api libraries build off MSVC

**Done.** [ADR-0107](../../adr/0107-build-the-api-libraries-build-with-clang-off-windows.md) chose
Clang on Linux in CI over the libraries that need neither Vulkan nor SDL. The job is
[portable.yml](../../../.github/workflows/portable.yml). The manifest's SDL3 and Vulkan ports moved
into a default `platform` feature that the job leaves out. Clang found eight warnings in those
libraries, all fixed: initializer lists out of declaration order, three private fields nothing
read, and a scanline count nothing read. Linux found two more things Clang on Windows could not:
MSVC's `#pragma warning` unguarded around the codecs and cgltf, and the image writers opening
files with MSVC's `fopen_s`, which `image::openFile()` now hides. Clang 18 cannot see libstdc++'s
`std::expected`, so the job installs Clang 20. The job's steps passed in an `ubuntu:24.04`
container on 2026-10-25, before the first push.

- Every compiler flag is guarded by compiler, with its Clang and GCC intent beside it. The
  `set(CMAKE_CXX_FLAGS ...)` lines become `target_compile_options` or go. The UTF-8 code-page
  manifest is attached on Windows only.
- A CI job configures and builds the libraries the ADR names with Clang or GCC. It compiles them,
  since the flags fail at compile and not at configure.
- MSVC's `/WX`, `/analyze` and clang-tidy gates run as they do now.
- [Build.md](../../contributing/Build.md) states the rule for a new flag.

## Outcome

Drafted 2026-10-25 against `51dee3f2` and closed the same day. All eight steps landed, each with
the gate its handoff named, and every new case failed without its change. Where failsfirst could
only show a case failing to build, hand mutations showed the cases catch the behaviour.

- **A game's shell.** `ui::Immediate` has a text field, a checkbox, a float slider, a combo and
  `itemHovered()`. The canvas draws a segment at any angle. An app can read, rebind one key of
  several, reset, save and restore its key bindings. `audio::Events` sets and reads a bus or VCA
  volume and pauses a bus. `Engine3D` takes a present mode and reports the one granted. The
  isometric controller zooms on the wheel and pans at the window's edge.
- **Portability.** The Vulkan instance sees a portability driver, and a device passed over is
  logged with what it lacks. Every compiler flag is guarded by compiler, and the libraries that
  need neither Vulkan nor SDL build and test with Clang on Linux in CI
  ([ADR-0107](../../adr/0107-build-the-api-libraries-build-with-clang-off-windows.md)).

What turned out differently from the plan:

- **A focused text field does not make `capturing()` true.** The bindings fire before a frame is
  drawn, so the key is kept from them as it arrives, by `Immediate::press()`. The editing moved out
  of `TextBox` into `input::LineEdit`, which both ui models now use, and `shell::Keyboard` can feed
  an immediate layer with no `ui::Engine`.
- **A rebind of a command bound to two keys made it fire twice.** The two mappings it left were
  both matched by a press. A rebuild now keeps one.
- **This machine's driver offers no mailbox mode**, so the default present request is granted FIFO
  here. The handoff expected mailbox.
- **The FMOD cases ran here**, against the SDK's example banks, in a build configured with
  `V3D_FMOD_ROOT`. CI still builds without the SDK.
- **Building off MSVC found more than flags.** Clang found eight warnings in the covered
  libraries, and Linux found MSVC's pragmas and `fopen_s` in the image codecs. The manifest's
  SDL3 and Vulkan ports moved into a default `platform` feature the Linux job leaves out, and a
  build with tests can now select a subset of libraries.

Left open: a 72 byte leak in one FMOD case that predates this plan, recorded in
[TODO.md](../../TODO.md#known-review-findings).
