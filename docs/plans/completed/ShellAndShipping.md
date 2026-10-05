# The Shell, Finished — Strips, Held Commands, Read-Forward Documents, A Shared Screen, A Game Space, And Pass Timings

Drafted 2026-10-04 against `e939bec`, and closed 2026-10-04. Thirteen steps across `api/ui`, `api/event`, `api/engine`,
`api/asset`, `api/render`, four apps and the documents, one of them held, taking up
[milestone 7](../../roadmap/completed/m7-ShellAndShipping.md) of [the game engine roadmap](../../roadmap/completed/GameEngine.md).
Every step is in this tree. cozy and retcon are the consumers it is written for, and each adopts
after it ships. Neither is changed nor run here.

The roadmap calls this milestone **a collection rather than a sequence**, and the plan keeps that:
almost nothing here blocks anything else. What the plan adds is a decision about which pieces are
taken now and which wait, and an order among the ones taken so that the smallest defects land
before the structural work. It changes four things about the roadmap:

* **The strips' `pickable()` is step 1**, because it is a defect a game has written a test to work
  around, and it is a filter in one function.
* **The renderer helper cannot live on the realtime side.** `v3dlib_ui` links `v3dlib_render`, so a
  helper in `api/render` that built a `TextRenderer` would be a cycle. It goes in `api/ui/shell`
  with `Engine3D` declared rather than included, and [step 5](#step-5--the-record-the-shell-builds-the-uis-renderers)
  records that.
* **A game space is the canvas's, not set 0's.** The roadmap reads the 2D pass not reading set 0 and
  pong's court as one change. They are not: set 0 is per pass and the quad projection is a push
  constant per submit, so per canvas, and pong draws its court and its menu in one pass.
  [Step 7](#step-7--a-canvas-with-a-space-of-its-own) gives a canvas a space of its own and closes
  the set 0 item as decided against.
* **Asynchronous loading is held.** It is the first thread in `api/`, and the case it serves —
  a region loaded while walking toward it — is not designed yet in either game.
  [Step 12](#step-12--asynchronous-loading-held).

**Little is due.** cozy is in its M5, and nothing in M5 waits on the engine. Its M6 names region
streaming and its M7 names free-form saves, and neither has started. retcon is in its phase 5. Its
phase 7 wants a shell, bindings as data and wheel zoom, and its phase 11 wants profiling. The plan
is drafted now, while the surveys are fresh, and the steps that have an in-tree consumer — the
editor, pong, voxel and `engine::Settings` — are the ones that carry the work.

## What the surveys found

cozy was read at `3217bc2` and retcon at `3d22935`. Both pin this tree at `13a9557`, so neither has
milestones 1 to 6. This tree was read at `e939bec`.

**In cozy:**

* **It builds the renderer set the way the four apps here do.** `AppEngine::initialize` makes an
  `Engine3D`, a `TextRenderer` with the upload lambda, then the ui engine, menu, cursor and keys
  (`src/AppEngine.cxx:455-496`). `buildTextRenderers()` rebuilds `ComponentRenderer` and
  `StatisticsOverlay` together whenever the window's height changes the text scale, because "both
  classes close over the size they draw at" (`:1656-1679`). Text scales by height over 720, clamped
  to 0.75–1.8. It resizes on `WindowResize` and checks `beginFrame`'s size every frame as well.
* **It has no 2D game space.** Its world is 3D under an orthographic three-quarter camera whose
  zoom is a half-height in world units. Its ui is in canvas pixels.
* **Its volume is four choices per bus, recorded as a decision.** `src/Settings.h:116-120` says
  so, and M3's "Considered" section calls four labelled choices "arguably the better control" and
  names the editor's property panes as a slider's second consumer.
* **Its inventory is a `VerticalBox` of `HorizontalBox`es** with every size stated, because a box
  neither wraps nor sizes from its children (`AppEngine.cxx:1131-1170`, M5 plan `:373-378`).
* **Its HUD has nothing pickable**, and `HudDocumentTest.cxx:155-173` asserts that it holds no
  `menubar` or `toolbar`, "a strip, which takes a press before pickable() is consulted".
* **Its walk keys are read held by name**, through `Engine::keys()->held("w")` on the fixed step
  (`:1219-1235`), "not rebindable yet". The M5 plan's open question names a held command as the
  fix and a keyboard layout that is not QWERTY as the trigger.
* **Its save is a versioned JSON document walked forward by a chain.** `src/Save.h` declares
  `using Migration = bool (*)(boost::json::object&)`, a `chain` that is empty today, and a
  `migrate()` that walks a copy and assigns back only if every step succeeds, stamping the version
  after each (`Save.cxx:88-130`). Load refuses a missing version, a version that is not a number,
  one above its own, one below one, and a gap. Writes go through `asset::writeDocument`. Its
  ADR-0003 records the shape and twenty cases in `SaveDocumentTest.cxx` hold it. M5 step 11 makes
  the first real migration.
* **Its frame graph** is 120 bars drawn beside `StatisticsOverlay` on F1 (`src/FrameGraph.h`). It
  measures the whole frame, as the overlay does.
* **It has no threads and names no loading hitch.** Its hot reload is debug-only polling, refused
  as an api file watcher in its M3 plan.

**In retcon:**

* **It builds its own renderer and uses no `Engine3D`.** `Game::Devices` makes a `RenderContext`,
  a `Renderer` over this tree's `Swapchain`, `DepthBuffer`, `Ring` and `Presenter`, and a
  `DevtoolsCanvas` holding a `Canvas`, a `ui::Immediate` and a `TextRenderer`
  (`game/Game.cpp:52-66`). A helper built around `Engine3D` does not reach it; anything built on
  `Ring` does.
* **Its ui is immediate only**, and it keeps its own list of window rectangles for pick-through
  rather than `Immediate::capturing()`, because it also needs modal backdrops. It has no slider
  (it uses `dragInt`), no wrapping grid and no file chooser.
* **Its camera keys are held keys read by name** in `IsometricCamera.cpp:24-52`, and
  `mappings.json` was deliberately not adopted. Phase 7 wants bindings as data, rebinding with
  conflict detection, wheel zoom and edge pan. It has no mouse capture or relative mode.
* **It has no saves and no settings file yet.** Phase 6 plans Boost.Serialization archives with a
  version and per-transition migrations, never overwriting a valid save with a failed one
  (`docs/game/save-load.md:124-141`). That is the same rule as cozy's in a different format, and
  it is retcon's to write.
* **It has no profiling** beyond `statistics().last()` in its devtools window. Phase 11 lists a gpu
  frame budget and a simulation tick cost.
* **It has no threads.** A scene change is `vkDeviceWaitIdle` and a synchronous load.
* **Its known issues name the api's MSVC flags** — `/EHsc` and `/utf-8` on interfaces with no
  `if(MSVC)` guard — as breaking its macOS and sanitizer jobs today. The roadmap puts that outside
  the milestone, and retcon has filed no handoff for it.

**In this tree:**

* **Six apps write the renderer set, all differently.** pong builds the overlay before the
  component renderer (`PongRenderer.cxx:33-48`). tetris loads its atlas between the engine and the
  text. voxel has no overlay and builds a `ui::Immediate` from the same measure and write
  (`voxel/src/Renderer.cxx:102-138`). The editor sets four dressing fields and a theme, and never
  calls `beginFrame`: it reads `window()`'s size in `drawUi` and resizes the canvas there
  (`vertical3d/src/Renderer.cxx:213-227`). The starter and odyssey have no text at all. Font sizes
  are 28, 22, 18 and 15 times the ui scale. No helper exists.
* **`v3dlib_ui` and `v3dlib_engine` both link `v3dlib_render` publicly**, and `v3dlib_engine`
  does not link `v3dlib_ui`. `Engine3D.h` includes the recorder, so it names Vulkan types, and an
  `api/ui` header must not ([EmbeddingSeams](EmbeddingSeams.md)).
* **The quad projection is a push constant per submit.** `Canvas::projection()` maps pixels to clip
  space with the origin at the top left (`api/render/realtime/Canvas.cpp:67-77`), and `Quad::submit`
  pushes it with each canvas. The quad pipelines already declare set 0 and do not read it.
  [RenderingPipeline.md](../../RenderingPipeline.md#what-is-not-built-yet) says only voxel's terrain
  reads set 0, which is stale: `line.vert`, `world.vert` and `lit.glsl` read it too.
* **`type::camera::Camera::orthographic` is centred and symmetric**, so it cannot express a
  rectangle with its origin at a corner.
* **pong's court is the window in pixels with 800 by 600 mixed in.** The FIXME has moved to
  `PongScene.cxx:196`. Paddle travel is 40 to 560, the right paddle stands at 785, and
  `GameState.cxx:23` assumes 800. tetris derives its cell from the canvas every frame, and odyssey's
  tile is 64 pixels (`odyssey/engine/Unit.h:11-16`) and is a tile size rather than a space.
* **The strips pick before `pickable()` is asked.** `Toolbar` and `MenuBar` never set the flag,
  so it is false. `strips()` filters only on `visible()` (`api/ui/input/Cursor.cpp:35-47`), and
  `motion` and `press` offer every point to every strip before the tree (`:127-140`, `:173-182`).
  `enabled()` is not consulted either, where the tree's `probe` honours both
  (`api/ui/component/Container.cpp:42-80`).
* **A `Scrollbar` takes keys.** `Keys::nudge` moves it by a line, a page or to an end
  (`api/ui/input/Keys.cpp:269-293`).
* **A box never sizes from its children.** `Arranger::arrange` advances one pen along one line
  (`api/ui/Arranger.cpp:228-269`), and `natural()` gives a box the room it was offered
  (`:207`, `:216`).
* **No file chooser exists**, and nothing lists a directory. The editor reads and writes a fixed
  `project.json` ([Editor.md](../../Editor.md), [TODO.md](../../TODO.md#editor)). `ui::shell::GameMenu`
  is the precedent for a shell class that drives components a document names, which touches none of
  [ADR-0047](../../adr/0047-code-exhaustive-enum-switches.md)'s places.
* **A binding reaches key-up**, and a binding may fire on press, release or both. `KeyState` knows
  what is held by key name. Nothing maps a command back to its keys, so nothing can say a command
  is held.
* **The window has `cursor()` and `warpCursor()` and no relative mode.** voxel warps to the
  centre on every motion and ignores `MouseMotion::motion()`, which `Mouse.cpp:64-66` already
  fills with SDL's relative motion.
* **`engine::Settings` carries a version and walks nothing.** A newer file is refused and made
  unwritable, which is right. An older or missing version is read as it is, and a version that is
  not a number skips the check silently (`api/engine/Settings.cpp:76-86`). The editor's project
  refuses anything but its own version (`vertical3d/src/scene/Project.cxx:332-334`). No migration
  code exists anywhere.
* **Nothing measures a pass.** No query pool, no `vkCmdWriteTimestamp`, and the device keeps no
  properties beyond logging its name (`vulkan/device/Device.cxx:242-285`). The recorder's pass loop
  (`vulkan/frame/Recorder.cxx:153-169`) is where a pair of timestamps would go, and a pool per ring
  slot is readable after that slot's fence, as `FrameUniforms` is per slot today.
* **Nothing starts a thread.** `asset::Manager` hands out shared, stateful loaders
  (`api/asset/Manager.cpp:52-63`). The uploader idles the queue (`vulkan/memory/Uploader.cxx:85`),
  and [TODO.md](../../TODO.md#voxel) records that voxel's remeshing relies on that.
  `MeshRegistry::add(name, model, albedos)` and `TextureFactory::create(image)` are already seams
  between a decode and an upload, except that a texture a glTF names is decoded inside the upload
  (`api/render/realtime/MeshRegistry.cpp:161-200`).

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--strips-that-respect-pickable) | Strips that respect `pickable()` and `enabled()` | `api/ui` | — | done |
| [2](#step-2--a-held-command) | A command held while any key bound to it is | `api/event`, `api/engine` | — | done |
| [3](#step-3--a-relative-mouse) | A window in relative mouse mode, and voxel's mouselook on it | `api/render`, `voxel` | — | done |
| [4](#step-4--a-document-read-forward) | A document read forward through a chain, and `Settings` and the project on it | `api/asset`, `api/engine`, `vertical3d` | **[0073](../../adr/0073-files-migrate-old-documents-one-version-at-a-time.md)** | done; accepted |
| [5](#step-5--the-record-the-shell-builds-the-uis-renderers) | The record: the shell builds the ui's renderers | `docs/adr` | **[0074](../../adr/0074-the-shell-builds-the-uis-renderers.md)** | done; accepted |
| [6](#step-6--one-screen-and-four-apps-on-it) | `ui::shell::Screen`, and pong, tetris, voxel and the editor on it | `api/ui`, four apps | 0074 | done |
| [7](#step-7--a-canvas-with-a-space-of-its-own) | A canvas with a space of its own, and pong's court in it | `api/render`, `pong` | **[0075](../../adr/0075-2d-a-canvas-may-have-its-own-coordinate-space.md)** | done; accepted |
| [8](#step-8--a-box-that-wraps) | A box that wraps and sizes itself from its lines | `api/ui` | — | done |
| [9](#step-9--a-file-chooser-and-the-editors-save-as) | A file chooser, and the editor's open and save as | `api/ui`, `vertical3d` | — | done |
| [10](#step-10--where-the-time-goes) | Named cpu scopes and gpu timestamps per pass | `api/engine`, `api/render`, `api/ui` | — | done |
| [11](#step-11--a-slider) | A slider: a value in a range, moved by a drag or a key | `api/ui` | — | done |
| [12](#step-12--asynchronous-loading-held) | Decoding off the main thread | — | — | held |
| [13](#step-13--the-handoff) | The documents and the handoff | `docs` | — | done |

Steps 1, 2, 3, 4, 8, 10 and 11 depend on nothing. Step 6 needs step 5. Step 7 needs step 6, because
pong's court gets a canvas of its own beside the one the screen owns. Step 9 needs nothing in this
plan, though it is easier to try in an editor that is on the screen. Step 13 needs everything else.

---

### Step 1 — Strips that respect `pickable()`

**`Toolbar` and `MenuBar` set `pickable(true)` in their constructors**, as `Button` and
`Scrollbar` do, and `strips()` keeps only a strip that is visible, pickable and enabled. Every
strip in the tree today is then picked exactly as before. A strip a document marks
`"pickable": false` — which the loader already allows for any entry (`api/ui/Loader.cpp:527-528`) —
stops taking presses and motion, and they fall to the tree and then the scene. A disabled strip is
skipped, as a disabled subtree is.

**A press in a pickable strip's gap is still the strip's.** `Toolbar::press` claims its whole
bounds. A HUD that wants its buttons pickable and its gaps not is not a strip, and cozy's M5 plan
has already refused making its hotbar one.

**Tests**, headless, in `CursorTest`:

* a toolbar built with no flag set still takes a press inside it;
* a toolbar marked unpickable lets a press reach a pickable button beneath it, and lets a press on
  nothing report that nothing took it;
* a disabled menu bar takes no motion and no press;
* a loaded document's `"pickable": false` reaches a strip.

With the filter taken out of `strips()`, the second and third cases fail.

**cozy's handoff** is to drop the "no strip" clause from `nothing_in_the_hud_is_pickable_test`,
since a strip marked unpickable is now scenery.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the ui
suite. There are four new cases in `cursor_test`. With the filter taken back out of `strips()`,
the scenery case and the disabled case both fail. One thing came out differently:

* **A menu bar read nothing from a document but its name.** The loader skipped every attribute on a
  menu and a menu bar, because their box is the renderer's, so `"pickable": false` on a menu bar
  was dropped. The loader now skips only their layout, and `loadAttributes` became static once it
  no longer read the layout. No document in the tree sets anything on a menu or a menu bar but
  its name and type, so nothing that loads today changes.

### Step 2 — A held command

**`Engine::held(command)` is true while any key bound to the command is held.** It asks the
mapper for the sources bound to a destination, and asks `KeyState` about each. The mapper gains
`sources(destination)`, the reverse of the lookup it has. Because the mapper is rebuilt on every
`rebind()`, a rebound walk key is the one `held()` reads with nothing more.

Asking `KeyState` rather than counting the edges a destination receives is the point. It holds
whatever edge the binding fires on, so a binding written for press only still answers `held()`. It
is also right after a focus change, because `KeyState` already is, where a counted edge would stay
down if its release went to another window.

**Keys only.** A mouse button bound to a command is not asked: `MouseState` holds buttons by
index, not by the names a binding uses, and nothing asks for a held click.

**Tests**, headless:

* in `api/event`: a destination's sources are every source bound to it, across contexts the mapper
  holds, and none after a rebind moves it;
* in `api/engine`, if its suite can build an engine without a window, and otherwise in the event
  suite against a `KeyState`: a command is held while either of its two keys is, and not after
  both are released; a rebound command follows its new key.

**cozy's handoff** is its walk keys as bindings, `walk-north` and the rest in its mappings, read
through `held()` on the fixed step and rebindable on its controls page. **retcon's** is its camera
pan as bindings, when its phase 7 makes bindings data.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
event, engine and ui suites. There is one new case in `MapperTest` and three in `EngineTest`, which
drive a headless engine with SDL key events. With `sources()` matching a name without its
context, the mapper case fails. Two things came out differently:

* **The engine keeps the mapper it built**, as `mapper_`, rather than asking `event::Engine` for
  it by name. `event::Engine` has no lookup, and the engine is what rebuilds the mapper.
* **A command's param is not part of it.** `held()` takes the command as `"context::name"`, as
  `rebind()` does, so two directions bound to one command with different params cannot be held
  apart. A game binds each direction as a command of its own.

### Step 3 — A relative mouse

**`Window::relativeMouse(bool)` and `relativeMouse()`**, through
`SDL_SetWindowRelativeMouseMode`. In relative mode SDL hides the cursor and keeps it in the window,
and `MouseMotion::motion()` carries the relative motion it already carries. The window does not
release the mode on focus loss itself. SDL does, and an app re-enters it on focus gain, which is an
event the loop already sends ([GameLoopFoundations](GameLoopFoundations.md)).

**voxel adopts it.** Its controller stops warping to the centre on every motion and reads
`motion()` (`voxel/src/Controller.cxx:207-224`). `suspend()` leaves relative mode and resuming
re-enters it, in place of the warp at `:150-157`.

**Verification is by hand.** A window needs a display, and the device suite's hidden window may not
accept the mode, so no case is promised. voxel is run, and the look turns with the mouse, does not
drift at the window's edge, and lets go on alt-tab and on the debug window. The log is silent.

**State: done.** All 25 suites pass, and cpplint is clean. voxel was run with a relative mouse move
sent to it and captured before and after with `PrintWindow`: the view turns, Escape brings up the
menu with a pointer, and the log has no warning or error. Two things came out differently:

* **Focus needs nothing from the app.** SDL3 keeps relative mode as a property of the window,
  releases the mouse while the window is unfocused and takes it back on focus. So voxel sets the
  mode once, and leaves it only while its menu is up.
* **voxel's look was already very sensitive.** Ten pixels of motion turns it a long way, and
  rolls it. The same ten pixels does the same on the code before this step, so the step keeps
  the scale it had: a pixel of motion is a pixel of the old offset from the centre.

### Step 4 — A document read forward

**ADR-0073: a document is read forward one version at a time, through a chain, and is refused
rather than half read.** It generalises cozy's ADR-0003 and is the read half of
[ADR-0041](../../adr/0041-files-write-documents-atomically.md). It goes in as `proposed` and
is accepted when `Settings` reads through it.

The shape, in `api/asset/`, Boost.JSON only:

```cpp
using Migration = std::function<bool(boost::json::object&)>;

enum class Reading {
    Current,    // already at this build's version
    Migrated,   // walked forward; the caller may write it back
    Newer,      // a later build wrote it; read nothing, and do not overwrite it
    Refused     // no version, not a number, below one, a missing step, or a step that failed
};

// chain[0] takes version 1 to 2. The walk runs on a copy, stamps "version" after each step, and
// assigns back only when every step succeeded.
Reading readForward(boost::json::object* document, int current,
    std::span<const Migration> chain);
```

**What a caller does with each reading is the caller's**, and the record says what the two in this
tree do. `Settings` treats `Newer` as it does today, running on defaults and refusing to save, and
treats `Refused` as a damaged file: defaults, and a save allowed. The editor refuses both, as it
does today, and a `Migrated` project is not written back until the player saves.

**A missing version is refused**, as cozy's is. That is a change for `Settings`, which reads one as
zero today. Every settings file this tree has written carries its version, so only a file edited
by hand is refused, and a version that is not a number — which the check skips silently today — is
refused with it.

**Alternatives the record weighs:** the version in the file name, which an atomic rename cannot
change; a migration from any version to the current one, which is a migration per pair; Boost
.Serialization's class versions, which are retcon's choice and a binary format; and leaving the
walk in each game, which is where it is today.

**Tests**, headless, in `api/asset`, cozy's cases carried across:

* a current document is `Current` and unchanged;
* a version 1 document under a chain of two steps runs both in order and ends at 3, each step
  seeing the version before it;
* a failing second step returns `Refused` and leaves the document as it was handed in;
* a missing version, a string version, version 0, and a version past the chain are `Refused`;
* a version above `current` is `Newer` and untouched;

and in the engine's suite, that a settings file whose version is a string is refused and saves.

**cozy's handoff** is to replace `Save.cxx`'s `migrate()` with `readForward()`. Its function
pointers convert to the `std::function` with nothing more, and its ADR-0003 points at 0073.

**State: done; ADR-0073 accepted.** All 25 suites pass and cpplint is clean. There are six cases in
a new `migration_test` and one more in the settings suite. The editor's existing project cases,
for a later version and for none, carry its change unchanged. With the stamp taken out of the
walk, the second step sees version 1 and the order case fails. Three things came out differently:

* **The header is `api/asset/Migration.h`**, beside `Writer.h`, and a step is a
  `std::function`, so cozy's function pointers and a capturing lambda both convert.
* **`Settings` keeps a refused document writable**, as it keeps a malformed one: nothing in it
  says a later build wrote it, so saving over it loses nothing that build stored.
* **The record was accepted when the plan closed**, by its decider, rather than when `Settings`
  read through it.

### Step 5 — The record: the shell builds the ui's renderers

**ADR-0074: the text, component and statistics renderers an app draws its ui with, and the canvas
they draw into, are built by one `api/ui/shell` class over an `Engine3D` it is handed.** It extends
[ADR-0028](../../adr/0028-apps-the-shared-app-shell-lives-in-the-api.md) and goes in as `proposed`, accepted when
the fourth app is on it.

* **It lives in `api/ui/shell`.** `v3dlib_ui` already links `v3dlib_render`. The header declares
  `Engine3D` and includes nothing of `api/render/realtime` beyond `Canvas`, so `api/ui` still
  names no Vulkan type. The upload lambda is in its `.cpp`, written once.
* **It owns** a `TextRenderer`, a `ComponentRenderer` over it, optionally a `StatisticsOverlay`
  and an `ui::Immediate`, the canvas they draw into, the resize that follows `beginFrame`'s size,
  and a text scale that rebuilds the renderers that close over a size, as cozy does by hand.
* **It does not own** the passes, the draw order, the theme or the dressing, which differ between
  every app surveyed. The dressing and the font size are handed in. The app still submits the
  canvas into whatever pass it likes.

**Alternatives the record weighs:**

* **In `api/render`.** It would link `v3dlib_ui`, which links it: a cycle.
* **A library of its own above both**, `api/shell`. It has no dependency `v3dlib_ui` lacks, and a
  consumer selecting libraries ([ADR-0033](../../adr/0033-build-select-api-libraries-through-a-manifest.md))
  would select one more for one class.
* **A `TextRenderer` constructor that builds its own upload**, which
  [EmbeddingSeams](EmbeddingSeams.md) removed to keep Vulkan out of `api/ui`'s headers.
* **On `engine::Engine`.** It would take `v3dlib_ui` into every app's base class, including
  odyssey's and the starter's, which draw no text.

**State: done; ADR-0074 accepted.** The record follows the draft. It was accepted when the plan
closed, by its decider, as ADR-0073 and ADR-0075 were.

### Step 6 — One screen, and four apps on it

**`ui::shell::Screen`**:

```cpp
class Screen final {
 public:
    struct Options final {
        float size;                     // the font's size at a scale of one
        ui::paint::Dressing dressing;   // line height, padding and the rest
        bool statistics = true;
        bool immediate = false;
    };

    Screen(boost::shared_ptr<render::realtime::Engine3D> renderer,
        boost::shared_ptr<asset::Manager> assets, boost::shared_ptr<log::Logger> logger,
        Options options);

    // beginFrame, and a canvas sized to the frame. False when there is nothing to draw into.
    bool begin();
    bool resized() const noexcept;      // the frame's size changed in this begin()
    void scale(float factor);           // rebuilds what closes over the font's size

    render::realtime::Canvas& canvas() noexcept;
    paint::TextRenderer& text() noexcept;
    paint::ComponentRenderer& components() noexcept;
    StatisticsOverlay* statistics() noexcept;   // null when not asked for
    Immediate* immediate() noexcept;            // null when not asked for
};
```

**`resized()` is how an app does its own resize**: pong's scene, tetris's layout and voxel's
camera are each the app's, after `begin()`.

**The apps adopt one at a time**, a commit each:

* **pong** at 28, with a line height of 1.4. The overlay is now built after the component renderer,
  which changes nothing drawn.
* **tetris** at 22. Its atlas is still loaded by the app, before or after the screen.
* **voxel** at 18, with no overlay and with the immediate layer, whose debug window keeps reading
  the statistics sample.
* **the editor** at 15 times its ui scale, with its four dressing fields and its theme. It moves
  from reading `window()`'s size to `begin()`, which is the one behavioural change here, and is
  the commit to watch.

odyssey and the starter draw no text and are not changed.

**Tests:**

* on the device: a screen over the suite's engine begins a frame with its canvas at the frame's
  size; after the hidden window is resized, `begin()` reports it and the canvas follows; a scale
  change rebuilds the component renderer at the new size. Silent;
* each app run by hand, with its menu, its overlay where it has one, and a window resized and
  minimised. Silent.

**This is the plan's midpoint.** If the four apps cannot share the class without an option per
app, it shows here, and the plan stops to rethink ADR-0074 rather than giving pong a canvas on top
of it.

**State: done.** All 25 suites pass and cpplint is clean. There are five cases in a new
`screen_test`, built over no renderer. Each app was run, captured with `PrintWindow`, and its log
read: pong with its menu up, tetris with its statistics shown, voxel with its debug readout and
its menu, and the editor with its strips and theme. Every log is silent, and the editor's picture
is the same with the screen and without it. The midpoint held. The four apps share the class with
two options between them: voxel's `immediate` and no overlay, and the editor's dressing. Five
things came out differently:

* **The screen takes an `Engine3D*`, and each app holds the screen by a shared pointer**, made
  after `engine_.initialize()`. Every app holds its `Engine3D` by value, and the atlas cannot be
  uploaded before the engine is initialized, so the screen cannot be a member built in the
  initializer list. A null engine builds a screen with no device, which is what the cases use.
* **The dressing is a function of the size**, `Options::dress`, rather than a `Dressing`. It is
  called again on every rescale, so the editor's four proportions stay proportional. Left empty,
  it sets a line height of 1.4 sizes, which is what pong, tetris and voxel all wrote. The
  immediate layer takes the component renderer's line height.
* **`draw(ui, sample)` draws the ui and then the overlay**, which every app wrote in that order.
  An app still draws its own game before it, and submits the canvas itself.
* **The overlay is not rebuilt on a rescale.** `StatisticsOverlay` gained a size setter, so an app
  that toggles it through the handle it got from `statistics()` still reaches the one drawn.
* **There is no device case.** `begin()` is `Engine3D::beginFrame` and a canvas resize, and the
  device suite builds no `Engine3D` to begin a frame on. The four apps beginning frames through it
  is the check. pong was also resized to 1200 by 700, minimised and restored: the canvas follows
  the window and the log has no warning or error.

Minimising pong found what nothing had: a minimised window spins and logs `Window has no area`
every frame, twelve thousand lines in two seconds. It is the path pong already took through
`beginFrame` before this step, and it is in [TODO.md](../../TODO.md#frames).

### Step 7 — A canvas with a space of its own

**ADR-0075: a 2D canvas may map a space of the game's own into the window, and its projection
stays a push constant per submit.** It goes in as `proposed` and is accepted when pong's court is
in one.

```cpp
enum class Fit { Stretch, Contain };    // Contain keeps the aspect and centres, with bars

void Canvas::space(glm::vec2 size, Fit fit);
glm::vec4 Canvas::viewport() const;     // where the space lands, in pixels
glm::vec2 Canvas::toSpace(glm::vec2 pixel) const;
```

A canvas with no space is in pixels, as every canvas is today, and `projection()` gives what it
gives now. With a space, `projection()` maps the space into `viewport()`, and the canvas's
`resize()` moves the viewport and nothing the game placed. `toSpace()` is the inverse, for a mouse
position.

**Why not set 0.** Set 0 is a camera per pass. pong draws its court and its menu in one pass, and
the menu is in pixels, so a camera per pass would need a second pass for the menu or a menu laid
out in court units. A projection per submit is a projection per canvas, which is what a game space
is. The [RenderingPipeline.md](../../RenderingPipeline.md#what-is-not-built-yet) item about the 2D
pass not reading set 0 closes as decided against, and its claim that only voxel reads set 0 is
corrected.

**pong adopts it.** Its court is a canvas with an 800 by 600 space, contained, beside the screen's
canvas for the menu. The FIXME at `PongScene.cxx:196`, the paddle travel, the paddle at 785 and
`GameState.cxx:23` all read the court's size rather than the window's, and `PongScene::resize`
stops moving the court. A wide window shows bars at the sides.

**tetris and odyssey are not changed.** tetris's layout fits a well to the window on purpose, and
odyssey's 64 is a tile's size, which a space would not remove.

**Tests**, headless, in the canvas suite:

* an 800 by 600 space contained in 1600 by 900 lands at 1200 by 900, centred, and its corners map
  to the viewport's;
* a stretched space fills the canvas;
* `toSpace()` inverts the mapping at the corners and the centre;
* a canvas with no space projects byte for byte as before.

pong's suite carries its collision and scoring cases unchanged, which is what shows the court's
numbers moved without its rules moving. pong is played, windowed, wide and narrow.

**State: done; ADR-0075 accepted.** All 25 suites pass and cpplint is clean. There are six new cases
in `canvas_test`, and pong's suite passes with its fixture no longer sizing the scene. pong was run
at its own 800 by 600, where it draws as it did, and at 1200 by 700, where the court is centred
at its own aspect with black bars either side. Its log is silent. With the space taken out of
`clip()`, the clip case fails. Four things came out differently:

* **A clip is mapped out of the space.** The plan had not seen that a scissor is in pixels, so a
  clip drawn in court units would have cut the wrong rectangle.
* **The court has a colour of its own, and pong clears to black.** Otherwise the bars are the
  court's colour and a wide window does not show where the court ends.
* **The court is constants on `PongScene`**, `width` and `height`, and `resize()` is gone. The
  paddle travel is the wall plus half a paddle's length from either end, and the right paddle
  stands its own width in from the far side, which are the 40, 560 and 785 that were written in.
* **The menu is a layer above the court**, by `Quad::submit`'s layer argument, since the two
  canvases go in one pass.

### Step 8 — A box that wraps

**`Box::wrap(bool)`, and `"wrap": true` in a document.** A wrapping horizontal box places its
children along a line until the next does not fit the room, then starts a line below, spaced by
its `spacing`. A vertical one wraps into columns. **A wrapping box's natural extent across its
lines is the sum of its lines**, so a grid of known cells needs no stated height. A box that does
not wrap is arranged exactly as today, and its natural size is still the room it is offered.

No new component type is added, so none of ADR-0047's places is touched. The change is
`Arranger::arrange` and `natural`, and the loader's box reader.

**Tests**, headless, in `ArrangerTest`:

* twenty 48-pixel cells with 8 pixels between, in a box 280 wide, make four rows of five;
* the box's natural height is four rows and three gaps;
* a cell wider than the room has a line of its own;
* every existing box case is unchanged.

**cozy's handoff** is its inventory as one wrapping box of twenty cells.

**State: done.** All 25 suites pass and cpplint is clean. There are four new cases in
`arranger_test`. With a wrapping box's natural size taken back to the room, four checks fail.
Two things came out differently:

* **The walk is one function, `Arranger::wrapped()`**, which places the children and answers how
  deep their lines reach. `natural()` calls it with nowhere to write the boxes, so the size a
  wrapping box asks for and the boxes it lays out cannot disagree.
* **A wrapping box takes the room it is given along its line**, unless its layout names a width.
  Inside a row that room is nothing, so a wrapping box in a row needs a width, which
  UserInterface.md says.
* **A box's natural size is `Arranger::box()`.** Written inline, the wrapping case took
  `natural()` past clang-tidy's cognitive complexity threshold.

### Step 9 — A file chooser, and the editor's save as

**`ui::shell::FileChooser`**, shaped like `GameMenu`: it drives a `SelectList`, a `TextBox` and two
`Button`s that a document names, so it adds no component type. It lists a directory through
`boost::filesystem` with directories first, filters by extension, steps into a directory and up out
of it, and joins a typed name to the directory it shows. It opens for reading or for saving, and
reports the chosen path through a callback. When saving over a file that exists, it asks once.

**Why not SDL's own dialog.** SDL3 has `SDL_ShowSaveFileDialog`, which is native and costs nothing
to draw. It cannot be drawn over a fullscreen game or themed, and its callback may arrive on
another thread. A game that ever wants free-form saves needs the one drawn in its own ui. The
editor could use either, and using the drawn one gives the chooser a consumer.

**The editor adopts it.** File gets Open and Save As, and the controller holds the project's path
rather than a fixed `project.json`. Save writes to that path. The dirty flag
[TODO.md](../../TODO.md#editor) lists beside the chooser stays there.

**Tests**, headless:

* a temporary directory lists its directories first, then the files that pass the filter, each
  sorted;
* stepping into a directory and up returns the listing it started from;
* a typed name is joined to the shown directory, and a name holding a separator is refused;
* choosing an existing file to save over asks before it reports.

The editor is driven by hand: save as a new name, open it, and see the scene come back.

**State: done.** All 25 suites pass and cpplint is clean. There are six cases in a new
`file_chooser_test`, over a temporary directory. The editor was driven with clicks and keys and
captured: a cube created, saved as `m7test` through the chooser, and then, in a fresh run, loaded
back through it and drawn in all four views. The log has no warning or error. Four things came out
differently:

* **The way up is a row**, `..`, first in every listing but the root's, rather than a button. A
  row is reached the way every other row is.
* **The confirmation is the label's.** A save over a file that is there writes "is there - accept
  again to replace it" where the directory was, and accepting the same name again replaces it.
* **Driving the editor found that reopening kept the last name in the field**, because the text
  box is typed into directly. `open()` now clears it.
* **`v3dlib_ui` links Boost.Filesystem itself**, since `FileChooser.h` names a path, rather than
  having it through `v3dlib_asset`.
* **The chooser has two constructors** rather than a defaulted `Names` argument. clang cannot use
  a nested struct's member initializers in a default argument of the class that holds it, which
  MSVC accepts and the gate refused.

### Step 10 — Where the time goes

**On the cpu, `Statistics::scope(name)`** returns a guard that adds its lifetime to a named row for
the frame. `Statistics` keeps a rolling mean per name as it does for the frame. It takes its clock
from the same source as the frame does, which a test can replace.

**On the gpu, a pool of timestamps per ring slot**, `vulkan::frame::Timings`, owned by the `Ring`.
It is reset in the slot's command buffer when `Ring::begin()` starts it. The recorder writes a pair
around each pass, named by `Pass::name()`. A slot's results are read when the slot is begun again,
after its fence, so they are `framesInFlight` frames old and never waited on. `Engine3D::timings()`
gives the last read frame's passes and their milliseconds. Because the pool is the ring's, retcon,
which records its own passes against this tree's `Ring`, can write the same pairs.

**The device keeps what it needs.** `timestampPeriod` and the graphics family's
`timestampValidBits` are kept when the device is selected. With no valid bits, timings are off and
come back empty.

**`StatisticsOverlay` shows the rows**, cpu and gpu, below the frame's. Its sample is a copy, as
it is today, because `api/ui` sits below `api/engine`.

**Tests:**

* headless: a scope under a stepped clock adds its time to its row; two scopes of one name in a
  frame sum; a row's mean rolls as the frame's does;
* on the device: after `framesInFlight` frames of a two-pass frame, timings name both passes in
  order, each finite and not negative; the whole suite stays silent with `VK_LAYER_VALIDATE_SYNC=1`.

voxel is run with F1 to see its terrain and overlay passes timed.
[TODO.md](../../TODO.md#lit-scenes)'s instancing waits on "a profile showing recording time", and this
is what would show one.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the tree.
There are three new cases in the engine's statistics suite, one in the overlay's, and two in a new
`timings_test` on the device. The whole device suite, now 62 cases, is silent with
`VK_LAYER_VALIDATE_SYNC=1`, and all 38 pictures it wrote before the step are byte-identical after
it. With the recorder no longer opening a span, the timing case fails. voxel's readout, on F3,
shows where its time goes: meshing chunks takes 129.5 ms of a 127 ms mean frame in a debug build,
and the colour pass 0.2 ms. Five things came out differently:

* **The timings are `Timings` on the `Ring`, and `Ring::begin()` drives them**, so anything that
  begins frames through the ring — `Presenter`, the device harness, and retcon's renderer — reads
  them without another call. The recorder takes them as an optional last argument.
* **A span runs from the end of the commands before it**, because both timestamps are written at
  `ALL_COMMANDS`. That is the honest reading of a serial queue, and it needs no stage per pass.
* **A scope finds its row as it opens.** Closing one only adds a number, because clang-tidy holds
  a destructor to not throwing, and adding a row allocates.
* **The overlay's lines are `Sample::spans`**, a name and nanoseconds each, which the app fills
  from `Statistics::rows()` and `Engine3D::timings()`. `api/ui` still cannot name either.
* **voxel's readout shows them, on F3**, since voxel has no overlay. The key was F1 in the draft.

### Step 11 — A slider

**`component::Slider`**, a new component type: a value between a minimum and a maximum, moved in
steps, drawn as a horizontal track with a thumb in the `"slider"` style class.

```cpp
class Slider : public Component {
 public:
    Slider();

    // A step of zero is continuous. The value is clamped and snapped to the new range.
    void range(float minimum, float maximum, float step);
    float minimum() const noexcept;
    float maximum() const noexcept;
    float step() const noexcept;

    // Clamped to the range and snapped to a whole number of steps from the minimum.
    void value(float v);
    float value() const noexcept;
    float fraction() const noexcept;    // where the thumb stands, 0..1 along the track

    // The value under a point, along the track. What the cursor calls on a press and a drag.
    void drag(glm::vec2 point);

    void event(const v3d::event::Event& command);
    const v3d::event::Event& event() const noexcept;
};
```

**It carries a command, as a check box does.** The command is sent each time the value changes,
whether by a press, a drag or a key, and is not sent when the value does not change. The app reads
`value()` from the slider, as it reads a check box's `checked()`. A drag across one step sends one
command, not one per motion.

**Keys, as `Keys::nudge` moves a scrollbar.** Left and right move it by a step, or by a hundredth of
the range when it is continuous. Page up and page down move it by a tenth, and home and end move it
to the ends. A key that would not change the value is not taken, so a slider at its minimum passes
the left arrow on to whatever uses it. That is the scrollbar's rule.

**Why not a scrollbar or a bar.** A scrollbar's offset is in pixels of content it does not hold,
and its thumb's length is a page. A slider's value is in its own units and its thumb is a fixed
size. `Bar` is a readout that is not pickable, by design. Making either do both would put a mode
in every switch that reads it.

**The cost is a component type**: the eight places
[UserInterface.md](../../UserInterface.md#still-open) counts, a ninth in `ui::input::command()`, and
two that the compiler does not check. Those are the resolver's class and the drag-follow `if`s in
`Cursor::motion` and `Cursor::release` (`api/ui/input/Cursor.cpp:104`, `:200`). The two `if`s
become one test that names both dragging types, so a third is added in one place. The loader reads
`minimum`, `maximum`, `step`, `value` and `command`. The arranger gives the slider the room's width
and a height from its style.

**Tests**, headless:

* in a new `SliderTest`: a value is clamped to its range and snapped to its steps; a new range
  re-clamps the value; a continuous slider keeps any value; `fraction()` is 0, ½ and 1 at the
  minimum, the middle and the maximum; `drag()` at either end of the track and beyond it gives the
  ends;
* in `CursorTest`: a press on the track sets the value under it and sends the command once; a drag
  past the end holds the maximum; a drag within one step sends nothing more;
* in `KeyboardDriveTest`: each key moves the value as above; a key at an end is not taken;
* in the loader's and the arranger's cases: a slider is read from a document, and is as tall as
  its style says;
* in `ComponentRendererTest`: the thumb is painted at its fraction along the track;
* `TypeTest`'s bound still covers every type.

With `drag` left out of the drag-follow test, the drag case stops at the press. With the snap
taken out, the steps case fails.

**No app in this tree adopts it**, because none has a continuous setting. **cozy's handoff** offers
it for volume, and whether its four choices give way to it is cozy's decision to revisit. The
editor's property panes take it when they are written.

**State: done.** All 25 suites pass and cpplint is clean. There are seven cases in a new
`slider_test`, covering the value, the cursor, the keys, a document, the layout and the paint.
With the slider taken out of the drag-follow, four checks fail. Four things came out
differently:

* **The maximum is always reachable.** A step that does not divide the range would otherwise
  leave End short of the maximum, so a value at or past it is the maximum.
* **The resolver counted its classes by hand**, as nine, and indexed an array of resolved
  dressings by class. The new class indexed past it. The count is now taken from the last
  enumerator.
* **The drag-follow is `Cursor::follow()`**, one function naming the scrollbar, the slider and the
  text box, which `motion` and `release` both call. A slider sends its command from there only
  when the drag changed its value.
* **The count of places a component touches is nine**, with `ui::input::command()` among them,
  and UserInterface.md now says so and names the two the compiler does not check.

### Step 12 — Asynchronous loading, held

**Held, and here is why.** It is the first thread anywhere in `api/`, which is a record of its own,
and the case it is for is a region loaded while walking toward it. Regions are not designed: they
wait on cozy's M6 in [TODO.md](../../TODO.md#tile-grids), and cozy's ADR-0003 names that same streaming
as its trigger to revisit its save. Building the loader first would build it against a guess.

What it would be, so that the survey is not lost:

* **Decoding on a worker, uploading on the main thread**, as the roadmap says. The worker loads
  through an `asset::Manager` of its own, because the manager's loaders are shared and stateful.
* **The main thread polls once a frame** and uploads what is ready through
  `TextureFactory::create(image)` and `MeshRegistry::add(name, model, albedos)`, which already take
  decoded data.
* **A texture a glTF names is decoded inside `MeshRegistry`'s upload today**, so that decode moves
  to the load side first. That is the one change it needs before any thread.
* **The upload keeps idling the queue**, so voxel's remesh in [TODO.md](../../TODO.md#voxel) is not
  affected. An upload that stops idling is a later change, and that entry says what it costs.
* `spdlog`'s logger is already the thread-safe one.

**The trigger is cozy's M6 region streaming**, or any load a person can see as a hitch. This moves
to TODO.md with that trigger when the plan closes.

### Step 13 — The handoff

Written here for each game to read, not sent to it. **Both must move their pin past `e939bec`
first.**

| A game needs | here |
|---|---|
| a HUD strip that clicks fall through | `"pickable": false` on a `toolbar` or `menubar` |
| a menu bar marked anything at all | its flags are read now, though not its box |
| walk or pan keys that can be rebound | bindings, read through `Engine::held(command)` |
| mouselook | `Window::relativeMouse(true)` and `MouseMotion::motion()` |
| a save or settings file a later build can read | `asset::readForward()` and a chain ([ADR-0073](../../adr/0073-files-migrate-old-documents-one-version-at-a-time.md)) |
| the text, component and statistics renderers built once | `ui::shell::Screen` ([ADR-0074](../../adr/0074-the-shell-builds-the-uis-renderers.md)) |
| a mouse position in those coordinates | `Canvas::toSpace()` |
| a 2D game in its own coordinates | `Canvas::space()` and `toSpace()` ([ADR-0075](../../adr/0075-2d-a-canvas-may-have-its-own-coordinate-space.md)) |
| a grid of slots | a wrapping box |
| a named save | `ui::shell::FileChooser` |
| a volume or a sensitivity | `component::Slider`, `"slider"` in a document |
| which pass is slow | `Engine3D::timings()`, or `Ring::timings()` around passes of its own, and `Statistics::scope()`, drawn as `StatisticsOverlay::Sample::spans` |

What adopting involves:

* **cozy** drops the strip clause from its HUD test, moves its walk keys into its mappings, replaces
  `Save.cxx`'s walk with `readForward()`, builds its renderers through a `Screen` with its height
  scale in `scale()`, and makes its inventory one wrapping box. Its volume may become a slider,
  if it revisits the four choices it recorded. None of it is due before its M5 closes.
* **retcon** takes `Ring::timings()` around its own passes when its phase 11 measures, since its
  renderer begins frames through this tree's `Ring` and the timings are read with nothing more, and bindings for its
  camera when its phase 7 makes them data. It uses no `Engine3D`, so `Screen` does not reach it
  unless it adopts the frame model. Its saves stay Boost.Serialization, and ADR-0073's rule is the
  same rule its own design states.

**State: done.** The handoff names what landed rather than what was drafted: `Ring::timings()`
rather than a `Timings` a caller makes, `Sample::spans` for what the overlay draws, and a menu
bar's flags read from its document. ADRs 0073, 0074 and 0075 were accepted
when the plan closed.

---

## Sequence

**Step 1 first.** It is a defect a game tests around, and it is a filter.

**Steps 2, 3 and 4 next, in any order.** Each is small, and each has a consumer in this tree or a
handoff written: voxel for step 3, `Settings` and the editor for step 4.

**Then the record, then step 6.** Step 6 is the midpoint. Four apps on one class is what the
record claims, and an option per app is what would disprove it.

**Step 7 after step 6**, because pong's court is a second canvas beside the screen's.

**Steps 8, 9, 10 and 11 can go any time.** Step 10 is the largest and touches the ring and the
recorder, so it is not taken alongside step 6. Steps 8 and 11 both edit the arranger, and step 11
also edits the cursor that step 1 does, so each goes after the other has landed rather than
alongside it.

**Step 13 last.**

## Verification

Per [sdlc.md](../../sdlc.md), every step that changes code: `ninja -C out/build/x64-Debug`, `ctest`,
cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all of them, so
every finding is the step's. Steps 3, 6, 7 and 9 change apps, and the analysis gates must be run
with the apps built. Step 10 changes what every frame records, so it runs the device suite once with
`VK_LAYER_VALIDATE_SYNC=1`, and every picture the suite wrote before it is compared byte for byte
after it.

**What can be pinned is pinned:**

* the strips' picking, the held command, the read-forward walk, the canvas's space, the wrapping
  box, the chooser's listing, the slider's value, keys and drag, and the cpu scopes, headless;
* the screen's resize and the timings' names and order, on the device.

**What cannot be pinned is an app run by hand**: voxel's look, the four apps on the screen, pong's
court in a wide window, and the editor's save as. Each is run with the validation log read, and
nothing is verified in another repository. No reference picture is blessed.

## What this does not do

* **No hot reload**, per the roadmap.
* **No crash reporting, packaging or macOS**, and no `if(MSVC)` guard on the api's interface
  flags. retcon's known issues name the flags as breaking its jobs now. That is a defect to fix when
  a build other than MSVC's is attempted here, and it earns a handoff from retcon when retcon wants
  it.
* **No gamepad and no HiDPI.** retcon creates its window without high pixel density, and neither
  game has asked.
* **No rebinding ui.** `Engine::rebind()` and the settings exist; a controls page is the game's,
  and retcon's conflict detection is retcon's.
* **No retained ui for retcon.** It has the retained tree and `pickable()` available and has not
  adopted them. That is retcon's work, as the roadmap says.
* **No removal from a container.** cozy re-points its inventory cells rather than removing them,
  and nothing has asked for more.

## When a step lands

Update the state in the table above.

* **Drafting** points [the roadmap](../../roadmap/completed/m7-ShellAndShipping.md) and its index here, corrects
  its line about the scrollbar and its pong line number, and says in the plans index that this plan
  is open.
* **Step 1** adds the strips' rule to [UserInterface.md](../../UserInterface.md)'s account of
  `pickable()`.
* **Step 2** adds `held()` beside `Engine::rebind()` in [Architecture.md](../../Architecture.md)'s
  account of the shell.
* **Step 3** adds relative mode beside `warpCursor()` in Architecture.md.
* **Step 4** adds the index row for 0073 as `proposed`, accepts it when `Settings` is on it, and
  updates [ADR-0018](../../adr/0018-editor-projects-saved-as-json-with-exact-topology.md)'s note that a
  version is the whole of the migration story.
* **Step 5** adds the row for 0074 as `proposed`.
* **Step 6** accepts 0074 with the fourth app, and replaces the renderer setup in UserInterface.md
  and each app's account with the screen.
* **Step 7** adds the row for 0075, accepts it with pong, closes the set 0 item in
  [RenderingPipeline.md](../../RenderingPipeline.md#what-is-not-built-yet) and corrects which shaders
  read set 0.
* **Step 8** adds wrapping to UserInterface.md's account of the arranger.
* **Step 9** adds the chooser beside `GameMenu` in UserInterface.md, updates
  [Editor.md](../../Editor.md), and narrows the editor's entry in TODO.md to the dirty flag.
* **Step 10** adds timings to RenderingPipeline.md's account of the frame, and the new device cases
  to [Testing.md](../../Testing.md).
* **Step 11** adds the slider to UserInterface.md's list of components, and corrects that
  document's count of places a component type touches if the drag-follow test changes it.
* **When the plan closes**, [m7](../../roadmap/completed/m7-ShellAndShipping.md) moves to `roadmap/completed/`
  and points here as done. Asynchronous loading moves to TODO.md with its trigger, the roadmap's
  table row says so, and this file moves to [completed/](.).
