# Design Review of the Tree

A design-principles review of the whole tree at `78af1431` (2026-11-22): SRP, DRY, OCP, LSP, ISP,
DIP, coupling, ownership and dead abstraction. It is not about a tree being deleted. It is an
itemised list of debt, kept here because the list is the point and it will be worked off item by
item. The first such review covered `api/` alone, at `7beb5f3`. This one covers `moya/libmoya`
and the editor for the first time, and `api/` again after the change since.

## Method

The tree was read in eight slices, about 190k lines:

- moya's scene side: `ri`, `scene`, `material`, `texture`, `colour`, `lpe`, `camera`, `view`,
  `volume`, with `api/rib`
- moya's render core: `integrate`, `accel`, `geometry`, `light`, `film`, `output`, `denoise`,
  `device`, `moya.cxx`
- moya's kernel
- the editor's `app`, `command`, `control`, `document`, `script` and `selection`
- the editor's `scene`, `store`, `tool`, `view`, `render` and `manipulator`, with `api/brep`
- `api/render`
- `api/ui`
- the rest of `api/`, the demos, imagetool, and the edges between libraries

Each slice was checked against the ADRs first. A shape an ADR decides is listed only where the
code no longer matches the ADR's own reasoning, and the entry says so. House style is out of
scope. An item with no planned consumer is not called dead for that alone. Every defect was read
twice by the slice that found it. The defects marked **re-read** were read a third time against
the code by the reviewer who wrote this record. None was built or run.

Two subjects were reviewed beforehand and are planned in
[HeaderBoundaries](../plans/HeaderBoundaries.md): public headers that include internal ones, and
`ri::SceneClient`'s jobs. They are not repeated here.

## Verdict

The large-scale structure is sound. The library graph has no cycle, and the configure checks it
in both directions. The kernel includes nothing from the host. The editor's include layering
holds. Lifetime in `render/realtime` goes through generational handles and the frame ring. The
first review's defects and its cross-cutting items are fixed where they were checked.

The debt has the same shape as last time: **a rule written by hand in several places, which has
already drifted.** This time the drift has produced defects. The integrator's parameters are
copied three times, and one copy drops a field, so a counted render never converges. The main
window and the other windows keep two copies of the present logic, and a minimised main window
freezes the others. The ui's held items are walked by hand in four places and missed in three,
so a named toolbar button cannot be found. Most of the defects below come from one of these
copies.

The second theme is error handling. The api, moya and the editor each use three conventions, and
no document states which applies where. Two defects come from a failure that was logged and
then treated as a success.

The third theme is a handful of classes that grew with every feature: the editor's
`Controller`, its `Context`, moya's `integrate::Session` and `geometry::internal::Walk`.

## Defects found

These are bugs, not design notes, and are worth fixing ahead of anything else. Each id's prefix
names its slice, and the same id is used in the sections below.

| Id | Where | Defect |
|---|---|---|
| R-D1 | `api/render/realtime/lighting/LitScene.cpp:127` | **re-read.** A dropped frame never advances `Ring::recording()`, which ignores skipped frames. The next draw of a `LitScene` then throws "drawn into this frame already". Toggling vsync, or restoring diorama from minimised, ends diorama. |
| R-D2 | `api/render/realtime/vulkan/frame/Presenter.cxx:220` | `acquire()` returns `Skip` for an invalid main swapchain before it reaches the joined ones. Minimising the editor's main window stops its render window presenting. |
| R-D3 | `api/render/realtime/vulkan/pipeline/Builder.cxx:323` | A pipeline built with a caller's layout does not mark the layout as borrowed. Two registered pipelines that share it destroy it twice. Latent: no caller registers one. |
| R-D4 | `api/render/realtime/vulkan/frame/Presenter.cxx:226` | Plausible. A frame begun again after a recording throw acquires on a semaphore that is still signalled. Latent: nothing catches around `renderFrame`. |
| MS-D1 | `moya/libmoya/ri/SceneClient.cxx:359` | **re-read.** A second `PxrCryptomatte` of one layer creates a second channel of the same name, and `Scene::create` throws. A multi-frame RIB that repeats the filter stops moya. |
| MS-D2 | `moya/libmoya/ri/SceneClient.cxx:299` | A `DisplayChannel` that redeclares a channel removes the old one, but an output still names it. `Outputs.cxx:124` and `Programs.cxx:166` then read the removed handle and throw. |
| MS-D3 | `moya/libmoya/ri/SceneClient.cxx:294` | `DisplayChannel` reads the declaration's type by a list of its own. `colour` makes a channel of one component, where `rib::Declaration` reads three. |
| MS-D4 | `moya/libmoya/ri/internal/Deforming.h:42` | `Deforming` has no `volume()`. A `Volume` repeated in a motion block drops its later pose, with no warning. |
| MS-D5 | `moya/libmoya/ri/internal/Lights.cxx:135` | A handle that failed once stays in `unmade_` when a later light reuses it. `Illuminate` on that handle is then ignored. |
| MS-D6 | `api/rib/Reader.cxx:646` | A lexer error inside a request's arguments is reported as its consequence, such as "expected a quoted string", not its cause. |
| MS-D7 | `api/rib/internal/Primitives.cxx:81` | The reader counts vertices from `Pz`, which moya's client never reads. `Polygon "Pz"` becomes a mesh over no points, refused with a confusing message. |
| MR-D1 | `moya/libmoya/device/cpu/Entry.cxx:369` | **re-read.** `profiledPaths` copies the integrator's parameters by hand and leaves out `run`. A counted render adds nothing to the half beauty, so adaptive sampling never stops a pixel. |
| MR-D2 | `moya/moya.cxx:231` | `--denoise` adds the denoised beauty only when the scene's own output is EXR. A PNG scene rendered to `--output x.exr` gets no denoised beauty. |
| MR-D3 | `moya/libmoya/geometry/internal/Displace.cxx:195` | The displacement fingerprint prints six significant digits. An edit smaller than that keeps stale diced geometry in an interactive session. |
| MR-D4 | `moya/libmoya/output/ExrImage.cxx:56` | The slice base is formed with a negative offset for a data window that does not start at 0. That is undefined behaviour, which `ExrWindows.h::based()` exists to avoid. |
| MR-D5 | `moya/libmoya/integrate/Session.cxx:338` | Passes are counted as run before a resume and as whole passes after it. Latent: no client reads the count. |
| MK-D1 | `moya/libmoya/kernel/integrate/LightSamples.h:508` | **re-read.** Light sampling stops at the diffuse limit for every lobe, while BSDF sampling stops per lobe. Past the limit, a glossy reflection of a light is weighted for a light sample that was never taken, so it comes out too dark. The darkening still needs a render to show it. |
| MK-D2 | `moya/libmoya/kernel/integrate/LightSamples.h:378` | The shadow ray applies a cut-out sphere's opacity once without media and twice with them. Adding unrelated fog darkens the shadow. |
| MK-D3 | `moya/libmoya/kernel/integrate/Paths.h:388` | A rejected near root of a cut-out quadric or curve is never followed by its far root. The back bars of a lattice-textured sphere vanish. |
| MK-D4 | `moya/libmoya/kernel/bsdf/Layering.h:126` | Plausible. The coat and the opaque specular lobe ignore the outside medium's index, while the transmission lobe uses it. |
| MK-D5 | `moya/libmoya/kernel/light/Dome.h:119` | Plausible. The dome's density is measured in its own space. Under a non-uniform scale or shear its estimate is biased. |
| EA-D1 | `vertical3d/src/app/ProjectFiles.cxx:36` | **re-read.** `project::save` discards `Document::save()`'s result. A failed save posts no notice and no refusal, and a script that saves carries on as if it succeeded. |
| EA-D2 | `vertical3d/src/control/CommandSearch.cxx:250` | The search removes duplicates by command name and runs each with no data. Of a scene's cameras it lists only the first, and running it does nothing. |
| EA-D3 | `vertical3d/src/Controller.cxx:213` | **re-read.** `start()` offers recovery and then runs the startup script at once. Answering Recover after the script ran replaces its work and clears the history. |
| EA-D4 | `vertical3d/src/control/PropertiesEditor.cxx:82` | Plausible. Opening a project leaves a pending properties edit, which is pushed into the new history on the next draw. |
| EV-D1 | `vertical3d/src/tool/Picker.cxx:344` | **re-read.** A mesh offers its ray parameter and a glyph its projected depth, and one comparison takes both. In a perspective view a light glyph in front of a cube loses to the cube. |
| EV-D2 | `vertical3d/src/tool/Picker.cxx:104` | The picker ignores the view's display flags, and `Transform::takes()` ignores the handles flag. A hidden light or handle still takes a click. |
| EV-D3 | `vertical3d/src/render/Shot.cxx:31` | With no single camera selected, a render takes the view's profile, which `framed()` widened to fill the area. The picture is wider than the camera's lens. |
| EV-D4 | `vertical3d/src/render/SceneTranslator.cxx:144` | Derived names collide: a curve's second run is `name 2`. A mesh named `Wire 2` overwrites the instance of curve `Wire`'s second run. |
| EV-D5 | `vertical3d/src/manipulator/Target.cxx:159` | Plausible. Two selected items on one shared geometry, each with the same vertex chosen, move that vertex twice. |
| EV-D6 | `vertical3d/src/store/Scene.cxx:679` | A name made unique on load is never saved. After a later save and reopen, the item takes back its old name. |
| EV-D7 | `vertical3d/src/scene/Properties.cxx:171` | Plausible. An animated lens or emission is not checked against its valid range between keys. |
| U-D1 | `api/ui/Container.cpp:90` | **re-read.** `Container::get()` walks `children()` alone. A named toolbar button, menu or menu item is never found. |
| U-D2 | `api/ui/Interface.cpp:115` | Plausible. Removing a container leaves a held toolbar button hovered, and its tooltip stays drawn. |
| U-D3 | `api/ui/input/Keys.cpp:43` | Keys reach a menu bar that is disabled, while the cursor skips it. Return on an invisible menu dispatches its command. |
| U-D4 | `api/ui/component/menu/Bar.cpp:165` | Arrow keys land on a disabled menu item, which the mouse skips. |
| U-D5 | `api/ui/component/menu/Bar.cpp:238` | An input item in a menu bar sends its command at once, with no value, where a menu captures one. |
| U-D6 | `api/ui/Interface.cpp:65` | A document with two containers of one name adds both, and only the first can be reached. |
| A-D1 | `demos/voxel/src/Renderer.h:170` | **re-read.** voxel keeps its own `DeviceContext` past `WindowRenderer::shutdown()`, so the device and its surface outlive the window. The editor's `ShadedPass` does the same. These are the two apps of the TODO entry "the editor and voxel leave a surface when they quit". Plausibly its cause. |
| A-D2 | `api/engine/Engine.cpp:490` | Plausible. A consumed key release never reaches the input devices, so the key stays held. |
| A-D3 | `api/log/Logger.cpp` | Plausible. Two threads building the first logger at once make the second throw from its constructor. |

## Cross-cutting

**X1. A rule copied by hand has drifted into a defect.** In each case below, one copy is wrong.
Give each rule one implementation.

- The integrator's parameters, copied three times (MR1). MR-D1.
- The main window's present logic beside the other windows' (R1). R-D2.
- Per-frame state reset five ways (R2). R-D1, and `Headless::draw` grows a list each draw.
- The ui's held items, walked by hand in four places and missed in three (U1). U-D1, U-D2.
- Light sampling written twice, for a surface and in a medium (MK4). MK-D1.
- The primitive dispatch, in `SceneClient` and in `Deforming` (MS5). MS-D4.
- What a view shows, decided in three places (EV7). EV-D2.
- Declaration types, read in a third copy (MS10). MS-D3.
- Output rules, repeated in `moya.cxx` (MR14). MR-D2.

**X2. Rules copied by hand that have not drifted yet:**

- energy compensation in four lobes (MK1)
- majorant stepping three times (MK5)
- "an option, or its kind's fallback" four times, found by two slices (MS4, MR2)
- `uploaded<T>()` in nine files, found by two slices (MS8, MR10)
- a shader node and a connection written in four places (MS1)
- program emission four times (MS2)
- a camera's and a light's properties three times in one file, and again in each codec (EV1)
- select mask names in four places (EA6)
- the editor's translator repeating moya's RIB client (EV2)
- pause in five demos (A2)
- the renderer and shell frame in five demos (A3)
- image extensions in three places (A4)

**X3. Failures follow no stated rule.** The api logs and returns false or null, carries an error
in a value, or throws (A7). The editor returns an `Outcome`, a bool and a log line, or nothing
(EA3). Its store returns a bool and logs, so the document can say only "failed" (EV8). moya's
scene edits throw, and the RIB reader's bool does not catch them (MS6). One option reader throws
and another returns 0 (MR2). EA-D1 and MS-D1 come from this. A rule stated once in Conventions.md
would settle each case: which failures throw, which return a value, and what the log is for. It
qualifies as a record.

**X4. Classes that grow with every feature:**

- `Controller`, about 45 private methods (EA1)
- the editor's `Context`, a service locator of about 25 nullable pointers (EA11)
- `integrate::Session`, about 60 members and two modes (MR3)
- `geometry::internal::Walk`, about 40 members (MR15)
- `ui::Interface`, four jobs (U8)
- `DeviceContext`, a closed list that its renderers include in a cycle (R6)

**X5. Closed lists that a new kind must join by hand.** The exhaustive switches in the ui, in
`scene::Visitor` and in the kernel already make the compiler find every place. These lists do
not:

- editor panels (EA2)
- the BSDF's nine switches (MK2)
- the light shape `if` chains (MK3)
- scene kind names (MR11)
- `codecs()` (EV4)
- the image extensions (A4)
- the pipeline variants (R4)

**X6. Handles and holders that outlive what they name:**

- A removed scene object is still named by its readers (MS3). MS-D2.
- Pipelines are never released (R3).
- Holders keep the whole device context alive (R11). A-D1.
- A replaced keyboard clears its successor's listener (U6).

**X7. Documents that no longer match the code:**

- The dependency table in docs/api/README.md misses entries of the manifest (A5).
- Demos.md names the wrong `run<T>` overload (A15).
- Two comments describe the rejected option of ADR-0121 (EV9).
- The editor's include table is stale (EV10).
- The ui's paint still does layout, which internals/ui says it does not (U3).
- `Handler::deformation()` is not an RI request, against ADR-0025 (MS5).

## By area

### moya's scene side

- **MS1 (high).** A shader node, a connection and an input are written in four places, and RIB
  and `.mtlx` give one MaterialX input two shapes. One shading-graph writer.
- **MS2 (high).** Program emission and its overflow fallback are repeated four times in
  `material/Compile.cxx`. One helper.
- **MS3 (medium).** A removed handle names nothing, but no read enforces it. A scene read that
  yields only live handles.
- **MS4 (medium).** See MR2.
- **MS5 (medium).** `Handler::deformation()` widens an RI-only interface. Its sinks inherit about
  60 methods to answer 13. A primitive interface that `Handler` extends.
- **MS6 (medium).** See X3.
- **MS7 (low).** The `ND_ri_…` definition names are built by concatenation in six places.
- **MS8 (low).** See MR10.
- **MS9 (low).** The integer range is stated six ways, and one client has two limits.
- **MS10 (low).** Declaration words are two tables that must agree, and MS-D3 is a third.
- **MS11 (low).** `material` finds `.mtlx` files through `texture`'s search path. The search path
  is RI's rule, not a texture's.
- **MS12 (low).** Plausible. `texture::Cache::read()` reads `files_` without the lock, and numbers
  are never reused across a long session.

### moya's render core

- **MR1 (high).** The integrator's parameters are written three times. One builder, and the
  profiled variant rebinds the template.
- **MR2 (high).** "An option, or its declared fallback" is written four times with different
  errors. One `scene::option()`.
- **MR3 (medium-high).** `Session` builds structures, owns the film, schedules passes, makes
  pictures, denoises and runs a thread. Split it into a structure builder, a film owner and a
  picture maker.
- **MR4 (medium).** Two threading models: `Session::start()` and the editor's own loop.
- **MR5 (medium).** A raw channel's facts are spread over five places. One table.
- **MR6 (medium).** EXR channel naming and chromaticity mapping are repeated in `output` and
  `texture`.
- **MR7 (medium).** `output` mixes channel setup, writers, EXR reading and checkpoints, and
  `light` and `texture` depend on it. Move the readers and chromaticities lower.
- **MR8 (medium).** `Lights` converts the materials a second time.
- **MR9 (medium).** `gather()` makes its own device and colour configuration.
- **MR10 (medium-low).** Device buffers are listed by hand three times in each structure.
  `uploaded<T>()` is copied into nine files.
- **MR11 (medium-low).** See X5.
- **MR12 (low-medium).** Hierarchy building is written twice, and still and moving builds are
  parallel copies.
- **MR13 (low).** ACEScg's luminance row and π are defined in several files.
- **MR14 (low).** `moya.cxx` repeats the library's output rules and mixes error channels.
- **MR15 (low).** `Walk` picks a converter by string comparison, and treats an unknown quadric as
  a torus.
- **MR16 (low).** `film/Compare` ships but only tests use it. `Session::picture()` makes a new
  denoiser each call.

### moya's kernel

- **MK1 (high).** Energy compensation, its table alpha and the face-up flip are copied into four
  lobes, and one copy has drifted. One `Compensation.h`.
- **MK2 (high).** A new BSDF model touches about nine switches, and its albedo is estimated twice.
  Derive one from the other, and give the models one signature.
- **MK3 (medium).** Light shapes are chosen by `if` chains in the integrator. Exhaustive switches.
- **MK4 (medium).** Light sampling at a surface and in a medium are two copies. One routine.
- **MK5 (medium).** Majorant stepping is written three times. One iterator.
- **MK6 (medium).** Every helper takes the whole `Paths`, against the narrow inputs of queued
  stages.
- **MK7 (medium).** `Closure` carries every model's payload, about a kilobyte per shading point.
- **MK8 (low).** Discrete pick-and-stretch is written five times.
- **MK9 (low).** Concentric mapping and the orthonormal frame are written twice. π has about a
  dozen names.
- **MK10 (low).** The specular index rule is written three times, and a medium can take another
  index than its surface.
- **MK11 (low).** The ACEScg conversion is composed in five places.
- **MK12 (low).** Light evaluation inverts its transform on every call.

### The editor's app and control

- **EA1 (high).** See X4. Take out a window manager, a panel registry and the camera-menu sync.
- **EA2 (high).** A panel kind is added in five places. One `Panel` interface in one table.
- **EA3 (high).** See X3. A scripted command returns an `Outcome`, rather than having
  `CommandTable::run` scan the journal.
- **EA4 (medium).** The properties editor pushes three kinds of command straight onto the
  history, past the document.
- **EA5 (medium).** A toggle's state is written in `syncUi`, apart from its command.
- **EA6 (medium).** See X2.
- **EA7 (medium).** A viewport's state is a schema of raw string keys, written in four places.
- **EA8 (medium).** "Does this item have part N" is written twice with different rules for curves.
- **EA9 (medium).** Selecting parts from a script mixes kinds under any mask.
- **EA10 (medium).** Several commands read inputs they do not declare, so the journal does not
  replay them.
- **EA11 (medium).** See X4.
- **EA12 (low).** Each document operation repeats the same boilerplate.
- **EA13 (low).** Five panels never prune their state for closed editors.
- **EA14 (low).** `Context::areaOf` repeats `placeOf`'s fallback.
- **EA15 (low).** Plurals are written three ways, and a refused batch gives no reason.
- **EA16 (low).** A timeline test cannot tell the documented rule from a reset.

### The editor's scene, store, tools and views

- **EV1 (high).** See X2. One table of path, read and write for each kind.
- **EV2 (high).** `SceneTranslator` repeats moya's RIB client by hand. One mapping that both
  call.
- **EV3 (medium).** Scene lookups are linear and run per item per frame. Index items by id.
- **EV4 (medium).** `codecs()` is a second list beside the exhaustive `codecOf()`. Derive it.
- **EV5 (medium).** The picker fans faces where every other consumer clips ears.
- **EV6 (medium).** `api/brep` has two mesh builders with different rules. Primitives go through
  `build()`.
- **EV7 (medium).** See X1.
- **EV8 (medium).** See X3.
- **EV9 (low).** See X7.
- **EV10 (low).** See X7.
- **EV11 (low).** `Tool::activate()` and `deactivate()` are never called through `Tool`.
- **EV12 (low).** `Delegate::Signature` is a third way to detect a change.

### render/realtime

- **R1 (high).** See X1. The main window is one output among the others.
- **R2 (medium).** See X1. One frame-turn id that every per-frame holder checks.
- **R3 (medium).** See X6. A release that the ring retires.
- **R4 (medium).** See X5. One keyed set of variants that builds and picks.
- **R5 (medium).** The sRGB predicate lives on `Quad`, and the text modes are numbered two ways
  across C++ and GLSL.
- **R6 (medium).** See X4. Renderers take a narrow struct of services.
- **R7 (medium).** Two buffer patterns are written twice.
- **R8 (low-medium).** `Pass::reset` keeps persistent state, so stale shadow passes record every
  frame.
- **R9 (low-medium).** `Lit::object` guards only the first frame against a write into a frame the
  GPU may still read.
- **R10 (low).** A texture can be registered past `Textures`, and then has no slot.
- **R11 (low).** See X6.
- **R12 (low).** One barrier is written inline.
- **R13 (low).** `Recorder::check` misses a depth pipeline in a target with no depth.

### ui

- **U1 (high).** See X1. A `forEachHeld()` beside `forEachDrawn()`.
- **U2 (medium).** Whether a component takes input is checked four ways. One `accepts()`.
- **U3 (medium).** See X7.
- **U4 (medium).** The immediate text field repeats the retained text box's line drawing, and the
  two place the baseline differently.
- **U5 (medium).** Commands are sent from four places with three rules, against ADR-0038.
- **U6 (medium).** See X6.
- **U7 (low).** The strips are drawn in depth order and hit in insertion order.
- **U8 (low).** See X4.
- **U9 (low).** A click between text boxes clears the focus first, which drops an IME composition.

### The rest of the api, the demos and imagetool

- **A1 (high).** `IsometricController` listens to commands itself, against ADR-0090. A pause
  cannot gate it.
- **A2 (medium).** See X2. The shell owns a suspended state.
- **A3 (medium).** See X2.
- **A4 (medium).** See X2. `image::Factory` lists its kinds.
- **A5 (medium).** See X7. Generate the table, or check it in a gate.
- **A6 (medium).** `engine` registers every media loader, so it carries every decoder.
- **A7 (medium).** See X3.
- **A8 (low-medium).** tetris and voxel use `rand()` rather than `type::Random`.
- **A9 (low-medium).** pong and diorama start the audio player by the same hand-written steps.
- **A10 (low-medium).** `font::texture` has no consumer outside its tests, and repeats
  `glyph::Store`. Ask the owner whether one is planned.
- **A11 (low).** The glTF preamble is copied into `asset` and `interchange`.
- **A12 (low).** `event` and `interchange` depend on `asset` only for helpers.
- **A13 (low).** `serializeDocument` copies every subtree at every depth.
- **A14 (low).** imagetool calls `exit()` from helpers that also return a status.
- **A15 (low).** See X7.

## Library edges

- The manifest has no cycle, and the configure checks the link graph against it.
- `engine` reaches `asset_media` and `image` (A6). `event` and `interchange` reach `asset` for
  helpers (A12).
- `window` sets `SDL_WINDOW_VULKAN` while documenting that it names no graphics API.
- voxel reaches into `render/realtime/vulkan` by design. diorama reaches it only to wait for the
  device before releasing, which is a symptom of A3.
- The kernel includes only kernel headers, glm, NanoVDB and allowed standard headers.

## Strengths worth keeping

- **The scene's edit, commit and apply model.** Every edit is checked as it is made, handles
  carry generations, and `changes(since)` lets each consumer rebuild only what changed.
- **Exhaustive switches with no `default`** in the ui, the editor's visitors and the kernel. A new
  kind fails to compile everywhere that has to handle it.
- **Lifetime in `render/realtime`.** Generational handles, destruction through the ring, and one
  recorder that checks each pass against its pipeline.
- **The editor's document.** Every operation returns an `Outcome`. Undo, scripts and menus share
  one path. Each scriptable command is registered once, and that drives its menus, keys,
  declarations and journal line.
- **Physics tests that check physics.** Density fits, reciprocity and white furnaces, and
  incremental rebuilds held to "the same bytes as a fresh build".
- **The engine owns ordering and failure.** `initialize()` and `shutdown()` are reached only from
  `run<T>`, and the old call-super problem is gone.

## Suggested order

1. **The defects**, each with a test that fails first. Those that end a run or lose work go
   first: R-D1, MS-D1, MS-D2, EA-D3, EA-D1. Then the wrong pictures: MR-D1, MK-D1, MK-D2, MK-D3.
   Then picking: EV-D1, EV-D2. Then A-D1, against its TODO entry.
2. **The copies that already drifted** (X1). Each fix removes the cause of a defect in step 1, so
   it lands with or just after that defect's fix.
3. **A record of how failures are reported** (X3), then the libraries brought in line with it.
4. **The classes that grow** (X4). `Session`'s split belongs with step 7 of HeaderBoundaries,
   which hides its members but does not reduce them.
5. **The closed lists** (X5) and **the remaining copies** (X2).
6. **The documents** (X7), which are small and independent.
