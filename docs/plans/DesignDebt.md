# Design Debt — The Review's Defects Fixed, and Each Rule in One Place

Drafted 2026-11-22 against `78af1431`. When this plan closes, every defect in
[DesignReview.md](../audits/DesignReview.md) is fixed with a test that failed first, or recorded as
not a defect. Each rule the review found written in several places has one implementation. A
failure is reported by one stated rule. The editor's classes that grew with every feature are
split by job, and the tree has one text stack.

The plan works off the review's list, and names each item by the review's id. It follows
[HeaderBoundaries](HeaderBoundaries.md), which splits `ri::SceneClient` and puts internal state
behind private implementations. Where the two touch the same code, this plan waits for that one.

---

## What the survey found

The review is the survey. Its verdict, in short:

* **41 defects.** Five end a run or lose work: R-D1, MS-D1, MS-D2, EA-D3 and EA-D1. Five give a
  wrong picture: MR-D1 and MK-D1 to MK-D3, with MK-D4 and MK-D5 plausible. Two make picking choose
  the wrong item: EV-D1 and EV-D2. A-D1 is plausibly the cause of the TODO entry on a surface left
  when the editor and voxel quit.
* **Most of the defects come from a rule copied by hand** (the review's X1). Fixing the defect
  without the copy leaves the next one to drift, so each defect step below takes its copy with it.
* **Failures follow no stated rule** (X3). The api, moya and the editor each use three conventions.
* **Six classes grow with every feature** (X4), and **seven lists** must be joined by hand when a
  kind is added (X5).
* **`font::texture` repeats the newer text stack** (A10). It rasterizes a face into an atlas and
  lays text out as quads, without shaping. `face`, `text` and `glyph::Store` do the same with
  shaping, eviction, colour glyphs and decorations. Nothing outside its own tests uses it. The one
  thing only it has is coverage of three channels for subpixel displays.

MS-D1 to MS-D3 are fixed by step 3 of HeaderBoundaries, which moves the code they are in.

## Decisions

| ADR | Decides | Step |
|---|---|---|
| — | How a failure is reported: thrown, returned in a value, or logged | [1](#step-1--the-record-how-a-failure-is-reported) |
| — | Who pauses a game: the engine or the game | [2](#step-2--the-record-who-pauses-a-game) |
| — | Who registers the media loaders | [3](#step-3--the-record-who-registers-the-media-loaders) |
| — | How the editor's scene reaches moya's | [4](#step-4--the-record-the-editors-path-into-moyas-scene) |

Each record step researches how established projects make the choice, and the owner chooses among
the options before the record is drafted. Its number is taken when it is drafted.

---

## The steps

| | What | Where | Review | State |
|---|---|---|---|---|
| [1](#step-1--the-record-how-a-failure-is-reported) | The record: how a failure is reported | `docs/adr` | X3 | open |
| [2](#step-2--the-record-who-pauses-a-game) | The record: who pauses a game | `docs/adr` | A2 | open |
| [3](#step-3--the-record-who-registers-the-media-loaders) | The record: who registers the media loaders | `docs/adr` | A6 | open |
| [4](#step-4--the-record-the-editors-path-into-moyas-scene) | The record: the editor's path into moya's scene | `docs/adr` | EV2 | open |
| [5](#step-5--runs-that-end-and-work-that-is-lost) | Runs that end, and work that is lost | `api/render`, `vertical3d/src` | R-D1, EA-D1, EA-D3 | open |
| [6](#step-6--wrong-pictures) | Wrong pictures | `moya/libmoya` | MR-D1, MK-D1 to MK-D5 | open |
| [7](#step-7--picking) | Picking | `vertical3d/src` | EV-D1 to EV-D3, EV-D5 | open |
| [8](#step-8--windows-and-the-device) | Windows and the device | `api/render`, `demos/voxel`, `vertical3d/src/view` | R-D2 to R-D4, A-D1 | open |
| [9](#step-9--the-uis-held-items-and-menus) | The ui's held items and menus | `api/ui` | U-D1 to U-D6 | open |
| [10](#step-10--moyas-and-ribs-other-defects) | moya's and rib's other defects | `moya`, `api/rib` | MS-D4 to MS-D7, MR-D2 to MR-D5 | open |
| [11](#step-11--the-editors-and-the-engines-other-defects) | The editor's and the engine's other defects | `vertical3d/src`, `api/engine`, `api/log` | EA-D2, EA-D4, EV-D4, EV-D6, EV-D7, A-D2, A-D3 | open |
| [12](#step-12--failures-reported-by-the-record) | Failures reported by the record | the tree | X3 | open |
| [13](#step-13--the-kernels-copies) | The kernel's copies | `moya/libmoya/kernel` | MK1 to MK3, MK5 to MK12 | open |
| [14](#step-14--moyas-copies) | moya's copies | `moya/libmoya` | MS1 to MS4, MS7 to MS11, MR2, MR5, MR6, MR10, MR13 | open |
| [15](#step-15--moyas-structure) | moya's structure | `moya/libmoya` | MR3, MR4, MR7 to MR9, MR11, MR12, MR15, MR16, MS12 | open |
| [16](#step-16--renderrealtime) | render/realtime | `api/render/realtime` | R3 to R10, R12, R13 | open |
| [17](#step-17--the-ui) | The ui | `api/ui` | U3, U4, U6 to U9 | open |
| [18](#step-18--the-editors-controller-and-panels) | The editor's controller and panels | `vertical3d/src`, `src/control` | EA1, EA2, EA5, EA13, EA14 | open |
| [19](#step-19--the-editors-commands-and-context) | The editor's commands and context | `vertical3d/src/app`, `src/document` | EA4, EA6 to EA12, EA15, EA16 | open |
| [20](#step-20--the-editors-scene-and-tools) | The editor's scene and tools | `vertical3d/src`, `api/brep` | EV1, EV3 to EV6, EV11, EV12 | open |
| [21](#step-21--the-editors-path-into-moyas-scene) | The editor's path into moya's scene | `vertical3d/src/render`, `moya/libmoya/ri` | EV2 | open |
| [22](#step-22--games) | Games | `api/engine`, `api/ui/shell`, `demos` | A1 to A3, A8, A9 | open |
| [23](#step-23--the-apis-edges) | The api's edges | `api/asset`, `api/image`, `api/event`, `api/interchange`, `api/window` | A4, A6, A11 to A13 | open |
| [24](#step-24--one-text-stack) | One text stack | `api/font` | A10 | open |
| [25](#step-25--the-documents) | The documents | `docs`, `imagetool`, `scripts` | X7, A14 | open |

## What blocks what

```
1 ADR ────────────────────────────────────> 12 Failures
2 ADR ────────────────────────────────────> 22 Games
3 ADR ────────────────────────────────────> 23 Edges
4 ADR ──┐
HB 2-6 ─┴─────────────────────────────────> 21 Editor into moya
HB 7 ─────────────────────────────────────> 15 moya's structure
5, 8 Windows ─────────────────────────────> 16 render/realtime
6 Pictures ───────────────────────────────> 13 Kernel's copies
9 ui defects ─────────────────────────────> 17 ui
10 moya defects ──────────────────────────> 14 moya's copies
11 editor defects ──┬─────────────────────> 18 Controller ──> 19 Commands
7 Picking ──────────┴─────────────────────> 20 Scene and tools
24 Text, and every step ───────────────────> 25 Documents
```

The records in steps 1 to 4 and the defects in steps 5 to 11 depend on nothing, and can be taken in
any order. The defects go first because each removes a fault a user can meet. Each later step
works on code a defect step has already fixed, so the two do not collide. Step 13 follows step 6,
which already joins the two copies of light sampling. Step 16 follows steps 5 and 8, which make
the frame-turn id and the one kind of window it builds on. Step 21 needs the record and
HeaderBoundaries' split of the handler, whose parts it reuses. Step 15 needs HeaderBoundaries step
7, so the session is split behind its private implementation. Step 24 depends on nothing.

## How every step is verified

* **A defect is fixed with a case that fails first.** Each defect step adds a Boost.Test case that
  fails on the tree before the fix and passes after it, and runs `failsfirst.ts` on it. A defect
  the review marked plausible is confirmed by that case first. A plausible defect the case cannot
  produce is recorded in the step's state note as not a defect, with the reason.
* **A copy removed changes no behaviour.** A step that joins copies passes the suites of every
  copy's callers unchanged. Where one copy had drifted, the step names which behaviour it kept, and
  adds a case for it.
* **A picture is checked against its reference.** A change to the kernel passes moya's reference
  renders and its statistical oracle. A change to `render/realtime` passes the device suite here,
  and its golden images across drivers on the pull request.
* **A step on a render path is measured.** Steps 6, 13, 15 and 16 run `moya/bench` or a frame of
  diorama before and after, and record both in the state note.
* **The verify preset builds each step.** x64-Debug and the gates run neither clang-tidy nor
  `/analyze`, so each step is clean at `/W4 /WX`, at `/analyze`, at clang-tidy and at every gate.

---

### Step 1 — The record: how a failure is reported

**The question.** Which failures throw, which return a value that carries the problem, and what
the log is for. The answer decides how a caller learns that a call failed and why, in every
library and app.

**The options:**

* **A value for an expected failure, a throw for a broken precondition.** A call that can fail on
  good input, such as opening a file, returns `std::expected<T, Problem>` or a result that carries
  the problem. A broken precondition, such as an index out of range, throws. The log only
  describes. LLVM's `Expected<T>` and Abseil's `StatusOr<T>` work this way, without the throw.
  The tree already returns `std::expected` from `asset::Worker`.
* **A throw for every failure.** A call that cannot do its job throws, as the C++ Core Guidelines
  advise (E.2, E.3). A caller that expects the failure catches it. moya's scene edits work this way
  now.
* **A false or null and a line in the log, with the reason returned beside it.** The api's present
  rule, kept, with every such call also giving the reason to its caller. SDL and most C libraries
  work this way.

The record also settles whether a handler may throw through the RIB reader (MS6), and what a
script sees of a command that failed (EA3).

### Step 2 — The record: who pauses a game

**The question.** Whether the engine or each game holds a game's paused state, and how a command
says it runs while paused. Five demos write the rule by hand today, and two have drifted (A2).

**The options:**

* **The engine holds it, and a command declares that it runs while paused.** The engine skips
  `simulate()` while paused, and the registry drops a command not marked to run while paused.
  Godot gives each node a process mode of pausable, always or when paused. Unreal stops the
  world's tick, and an input binding can execute while paused.
* **The game holds it, through one helper.** The shell gives a game a paused flag and a wrapper
  for its commands, and the game gates its own simulation. Unity's games set the time scale to
  zero themselves.

The record also settles whether the isometric controller's rotation (A1) pauses with the game.

### Step 3 — The record: who registers the media loaders

**The question.** Whether `engine` registers every media loader, so it carries every decoder, or
something above it does (A6).

**The options:**

* **The engine, as now.** Every app reads every format with no setup. A tool that runs the engine
  without a window carries PNG, JPEG, SVG and glTF.
* **The shell.** A game gets every loader through the shell it already uses. A tool that links
  only the engine gets none, and registers what it reads.
* **Each app.** An app registers the loaders it uses, as an app that uses SDL_image links it
  itself. Godot registers each format's loader in the module that brings it.

### Step 4 — The record: the editor's path into moya's scene

**The question.** How the editor's scene becomes moya's retained scene. Today
`render::SceneTranslator` repeats, by hand, what moya's RIB handler does with the editor's RIB
export (EV2). Each RI feature then has three implementations.

**The options:**

* **One mapping that both call.** The parts of the RIB handler that HeaderBoundaries splits out
  take values, not requests, and the translator calls them. Each rule lives once, in moya.
* **The translator replays its RIB through the handler.** The editor writes RI calls straight into
  a `SceneClient`, and its scene is diffed against the last. Each rule lives once, and the
  translator holds no mapping. Each edit costs a replay of the scene.
* **The editor translates on its own, kept in step by a test.** Blender's Cycles syncs from
  Blender's scene by code of its own, apart from any exporter. RenderMan for Maya translates
  Maya's scene directly, not through RIB. `SceneTranslatorTest` pins the two paths together.

### Step 5 — Runs that end, and work that is lost

* **R-D1.** A dropped frame advances the ring's frame-turn id. `LitScene` checks that id rather
  than `recording()`. The case draws, drops a frame and draws again.
* **R2.** Every per-frame holder checks the same frame-turn id lazily: `StreamRing`,
  `FrameUniforms`, `Lit`, `LitScene` and `Timings`. `Headless::draw` then stops growing a list on
  each draw.
* **EA-D3.** The startup script runs after recovery is answered, or recovery is not offered when a
  script is given. The step chooses by which loses no work in either order.
* **EA-D1.** A failed save posts a notice and a refusal, and a script's save fails with it. The
  whole of EA3 waits for step 12.

### Step 6 — Wrong pictures

* **MR-D1 and MR1.** The integrator's parameters are built in one place, and the profiled variant
  rebinds that template rather than copying it. The case checks that a counted render stops where
  an uncounted one does.
* **MK-D1 and MK4.** Light sampling at a surface and in a medium become one routine, and it checks
  each closure's own limit.
* **MK-D2.** A shadow ray applies a primitive's opacity once, with or without media.
* **MK-D3.** A rejected near root of a quadric or a curve is followed by its far root.
* **MK-D4 and MK-D5.** Each is confirmed first: a coated sphere in a water medium against a
  reference, and a furnace under a dome scaled by (1, 2, 1).

**Tests:** the reference renders and the oracle pass, each case fails first, and `moya/bench` is
recorded.

### Step 7 — Picking

* **EV-D1.** Every candidate offers one measure of depth. A mesh and a glyph in a perspective view
  are then compared fairly.
* **EV-D2 and EV7.** One predicate answers whether an item is drawn and pickable in a view, from
  the display flags and the camera looked through. Drawing, the picker and the transform tool all
  ask it.
* **EV-D3.** One rule says which camera a render takes. The render view, the RIB export and the
  batch all call it, and it never takes the view's widened profile.
* **EV-D5.** Confirmed first, with two items on one geometry.

### Step 8 — Windows and the device

* **R-D2 and R1.** The main window becomes one output among the others, and the presenter loops
  over equal swapchains. A minimised window then skips only itself.
* **R-D3.** A pipeline marks a layout it does not own, and the resources destroy only their own.
* **R-D4.** Confirmed first, with validation on: a recording throw, then a frame begun again.
* **A-D1 and R11.** voxel's renderer and the editor's `ShadedPass` hold the services they use, not
  the device context. The TODO entry on a surface left at quit is closed when validation on both
  apps reports none.

### Step 9 — The ui's held items and menus

* **U-D1, U-D2 and U1.** `forEachHeld()` beside `forEachDrawn()`, as an exhaustive switch over the
  component's type. Search, the hover test and image resolution walk through it, and the strip
  walks stop casting.
* **U-D3 and U2.** One test of whether a component takes input, used by the keys, the cursor and
  the keyboard.
* **U-D4, U-D5 and U5.** Strips and bars return the item chosen, and one place sends its command,
  as ADR-0038 decides. A disabled item is skipped by keys as by the mouse, and an input item
  captures its value in a bar as in a menu.
* **U-D6.** A document whose containers share a name is refused.

### Step 10 — moya's and rib's other defects

* **MS-D4 and MS5.** A primitive interface that `Handler` extends, which `deformation()` returns.
  One dispatch turns a request into a shape for either sink. `Handler` then holds RI's requests
  alone, as ADR-0025 decides.
* **MS-D5.** A handle a later light makes is removed from those that failed.
* **MS-D6.** The lexer's error is kept as the cause.
* **MS-D7.** `Pz` is read as RI defines it, or listed as unread with a warning.
* **MR-D2 and MR14.** One library function chooses a render's outputs from the scene and the
  override, and `moya.cxx` calls it.
* **MR-D3.** The displacement fingerprint keeps every bit of each value.
* **MR-D4.** The EXR readers form their slice base through `based()`.
* **MR-D5.** One unit of passes before and after a resume.

### Step 11 — The editor's and the engine's other defects

* **EA-D2.** The command search keeps each item's data and runs it with that data.
* **EA-D4.** Confirmed first. If it holds, opening a project cancels a pending properties edit.
* **EV-D4.** Derived moya names cannot meet an item's name.
* **EV-D6.** A name made unique on load marks the item changed, so it is saved.
* **EV-D7.** Confirmed first. If it holds, a posed lens and emission are clamped to their valid
  range.
* **A-D2.** Confirmed first. If it holds, a key release always reaches the input devices.
* **A-D3.** The first logger is made once, whichever thread asks first.

### Step 12 — Failures reported by the record

Each library and app is brought in line with step 1's record:

* **A7**, in the api.
* **EA3**, in the editor's commands and the script's refusals.
* **EV8**, in the store and the project.
* **MS6**, in the RIB reader and moya's handler.
* **MR2**'s error half, in moya's option reads.

engine/README.md's account of failures is rewritten to the record.

### Step 13 — The kernel's copies

* **MK1.** One `Compensation.h` for the lobe shape, the bounced colour, the table alpha and the
  face-up flip.
* **MK2.** The albedo a closure adds is derived from what it reflects. The models take one
  signature, so each operation is one switch.
* **MK3.** Exhaustive switches over a light's shape.
* **MK5.** One iterator over majorant segments.
* **MK6.** Helpers take the lights, surfaces, settings and sampler they use, not the whole `Paths`.
* **MK7.** A closure's shared core and its model's payload, measured on `moya/bench` against the
  present layout. The layout that wins is kept.
* **MK8 to MK12.** One pick-and-stretch, one constants header, one warp header, one specular index
  rule, one ACEScg conversion, and each light's inverse kept beside it.

### Step 14 — moya's copies

* **MS1.** One writer of shading graphs: a node, a connection and an input declaration.
* **MS2.** One helper runs a program's fill with its fallback, and one moves it into `Compiled`.
* **MS3.** A scene read that yields only the live handles of a parameter, and every reader uses it.
* **MS4 and MR2.** One `scene::option()`.
* **MS7, MS9 and MS10.** One function names an RI definition, one integer range, and one table
  for each declaration word.
* **MS8 and MR10.** One `device::uploaded()`.
* **MS11.** The RI search path moves to a module that both `texture` and `material` read.
* **MR5, MR6 and MR13.** One table of raw channels, one home for EXR naming and chromaticities, and
  each constant named once.

### Step 15 — moya's structure

* **MR3.** `Session` splits into a structure builder, a film owner and a picture maker, behind the
  private implementation HeaderBoundaries step 7 gives it. One-shot and edited sessions are two
  strategies of the builder.
* **MR4.** One threading model. The editor's delegate uses the session's own thread, or the
  session's thread is removed.
* **MR7.** EXR reading and chromaticities move below `output`, and the channel setup moves beside
  `integrate`.
* **MR8 and MR9.** `Lights` reads the materials `Surfaces` converted, and `gather()` takes the
  session's device and colour configuration.
* **MR11, MR12 and MR15.** Kind categories in the schema; one hierarchy builder, with still and
  moving builds from one template; and a table of converters in `Walk`.
* **MR16 and MS12.** `Session::picture()` reuses its denoiser. The texture cache's precondition on
  `open()` is stated, and closed numbers are reused.

### Step 16 — render/realtime

* **R3.** A pipeline can be released through the ring, and each renderer releases what it added.
* **R4.** One keyed set of pipeline variants, which builds and picks.
* **R5.** The sRGB test moves to `Formats.h`, and the text modes are numbered once for C++ and
  GLSL.
* **R6.** Renderers take a narrow struct of services, and `DeviceContext` stops including them.
* **R7.** One growable per-frame buffer.
* **R8, R9 and R10.** `Pass::reset` clears every per-frame field; `Lit::object` checks the frame
  turn; and every texture registers through `Textures`.
* **R12 and R13.** The inline barrier moves to `Barriers.h`, and `Recorder::check` refuses a
  pipeline whose formats the pass cannot attach.

### Step 17 — The ui

* **U3.** Tab boxes, bar labels and strip placement move into `Arranger`, called from `walk()`. The
  renderer then holds no layout, as internals/ui says.
* **U4.** One line drawing for the text box and the immediate text field, with one baseline rule.
* **U6.** `onFocus` and `onTyping` return a connection that removes only its own listener.
* **U7, U8 and U9.** Strips are hit in the order they are drawn; image resolution leaves
  `Interface`; and a press works out the new focus before changing it.

### Step 18 — The editor's controller and panels

* **EA2 and EA13.** A `Panel` interface, registered by kind in one table, with a `forget()` that
  prunes a closed editor's state. `Area::kinds()` and `drawAreas` read the table.
* **EA1.** A window manager and the camera-menu sync leave `Controller`, which then wires the parts
  together.
* **EA5.** A toggle is registered with its state, and `syncUi` reads the registry.
* **EA14.** `Context::areaOf` is removed for `placeOf`.

### Step 19 — The editor's commands and context

* **EA11.** Each command class takes the pieces it uses, not the whole `Context`. The chosen keys
  move out of the timeline panel.
* **EA4.** Reparenting, render visibility and units become document operations, and the properties
  editor takes the document as its siblings do.
* **EA6, EA7 and EA8.** One table of select masks, a typed viewport state, and one test of whether
  an item has a part.
* **EA9 and EA10.** A script's selection respects the mask, and each command declares the inputs
  it reads, so the journal replays it.
* **EA12, EA15 and EA16.** One helper for a document operation, one plural, and a timeline test
  that tells the rule from a reset.

### Step 20 — The editor's scene and tools

* **EV1.** Each kind's properties are one table of path, read and write. Properties, values, poses
  and the codecs read it.
* **EV3.** The scene indexes its items by id, and one evaluation is kept per revision per frame.
* **EV4.** `codecs()` is built from `codecOf()`.
* **EV5.** The picker cuts faces with `triangulateFace`, as everything else does.
* **EV6.** Primitives build through `brep::build()`, and `BRep`'s second builder is removed.
* **EV11 and EV12.** Each tool takes a typed mode, not `activate(name)`. A scene revision replaces
  the delegate's signature.

### Step 21 — The editor's path into moya's scene

`render::SceneTranslator` reaches moya's scene by the path step 4's record chooses. The rules it
repeats today live once. `SceneTranslatorTest` keeps passing.

### Step 22 — Games

* **A2.** Pause as step 2's record decides, and the five demos drop their own.
* **A1.** The isometric controller registers its commands in the engine's registry, as ADR-0090
  decides.
* **A3.** One end of a frame and one release, in the shell, so the order lives once.
* **A8 and A9.** tetris and voxel take a `type::Random`. The audio player is started and stopped
  in one place.

### Step 23 — The api's edges

* **A6.** The media loaders are registered as step 3's record decides.
* **A4.** `image::Factory` lists the kinds it reads and writes, and the asset loaders and
  imagetool read the list.
* **A11 and A12.** One glTF preamble, in `interchange`. The checked JSON reads and `readFile` move
  to a leaf, or the manifest's comment states why `event` and `interchange` depend on `asset`.
* **A13.** `serializeDocument` tests a subtree by reference.
* **`window`** leaves the graphics flag to the renderer that needs it.

### Step 24 — One text stack

* **`font::texture` is removed**: `Font`, `FontCache`, `TextBuffer`, `Buffer`, its internal
  headers and its three suites. Its section in Fonts.md goes with it.
* **Text is drawn one way.** A text drawn at one size into an atlas is a `glyph::Store` at that
  size, laid out by `text`.
* **Coverage of three channels for subpixel displays is dropped.** The newer stack stores one
  channel, or four for colour, and nothing draws text for a subpixel display. A text needing it
  later adds it to `glyph::Store`.
* **`image::TextureAtlas` stays.** tetris and `image::Pack` use it.

**Tests:** the font suites pass without the removed three, and no file names `font/texture/`.

### Step 25 — The documents

* **A5.** The table of libraries' dependencies in docs/api/README.md is checked against the
  manifest by a gate, as `librarytable.ts` checks the list of libraries.
* **A15, EV9 and EV10.** Demos.md names the right `run<T>`. The two comments of ADR-0121's rejected
  option state its rule. The editor's include table matches the includes.
* **A14.** imagetool returns its status rather than calling `exit()`.
* **The records' statuses** are set, the review moves to `audits/completed/`, and this plan moves
  to `completed/` with an outcome.
