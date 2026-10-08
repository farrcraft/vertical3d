# Retcon Upstream — What A Game On This Api Still Writes For Itself

Drafted 2026-10-07 against `55509de`. Thirteen findings across `api/render/realtime`, `api/ui`,
`api/ecs`, `api/type`, `api/engine` and `api/audio`.

The prompting case is retcon, a game built against this api in another repository. A survey of
everything left in its `engine/` after it adopted `55509de` found, for each piece, whether this
tree already has it and what stops the game using this tree's instead. Its handoffs live in that
repository under `docs/upstream/` and are not linked here, because their paths do not resolve
from this one. What they found is restated below where the reasoning is worth keeping. Each
handoff also names the gate its change must bring, and those gates are carried into the steps.

**The reason to do this here is the same as [EmbeddingSeams](completed/EmbeddingSeams.md)'s.**
Each finding is a place where a library has only met the apps in this tree. `Lit` has only met
a camera on `Profile::Hand::UpCrossDirection`. `Quad` has only met a UNORM target. Three of
this tree's own apps include `SDL3/SDL_main.h` themselves, beside the `run<T>` that is meant to
be their whole `main`. None of the apps in the tree will notice these seams being cut, because
every new parameter defaults to today's behaviour.

## What the survey got right, and where the tree has moved

The cited lines were checked against `55509de` and hold, with one exception. `Capture` now has
two `record()` overloads (`vulkan/frame/Capture.h`): one takes a `Source`, and one takes a
`Swapchain` and an image index. The second is the one `Engine3D` needs, and it requires the
image in `PRESENT_SRC`, which `Recorder::record` leaves it in.

Two findings need more than the handoff says:

- **A renderer's built pipeline cannot be inspected.** A compiled `VkPipeline` does not expose its
  state. `PipelineBuilderTest` reads state from the `Builder`, but `Lit`, `Line` and `Quad` keep
  only handles. Their gates are therefore pictures, drawn through `test::Headless` into a
  `RenderTarget` and read back with `Capture`, as `WorldDepthTest` does. A picture also proves
  more than a state check: it shows the right faces survive, not only that a flag was set.
- **`Engine3D` cannot run headless.** `Context3D` takes its device from a window's surface, and no
  test in the tree builds one. The capture gate the handoff describes needs a window. Step 1
  settles how before the code is written.

## Decisions

| Question | Who | Step |
|---|---|---|
| How does an overlay draw lines that test depth without writing it? | **Decided**, 2026-10-07: depth writing is a pass's choice, as dynamic state. [ADR-0085](../adr/0085-rendering-a-pass-chooses-whether-depth-is-written.md) | 4 |
| Does the `Engine3D` capture test open a hidden window, and does CI run it? | **Half decided**, 2026-10-07: a visible window in a suite of its own. CI waits on a run | 1 |
| Where does an isometric controller live? | **Decided**, 2026-10-07: `api/engine`, reading commands | 9 |
| Does `api/audio` grow events and parameters, with an optional FMOD backend? | **Decided yes**, 2026-10-07. ADR-0084 records it and amends ADR-0021 | 11 |

Two of retcon's findings are declined here and get no step:

- **The record lexer.** retcon recommends keeping it, and this plan agrees. It is a file format
  with one consumer, and owning a grammar here commits the api to its stability for games that
  do not exist yet.
- **Shaders read from disk.** Every app in this tree embeds its SPIR-V, and a game that wants to
  swap a shader without relinking can read the words itself and hand them to `Builder::shader()`.

## What blocks what

Nothing here blocks anything else here. The order is by what each step unblocks for retcon:

- **Step 1 comes first.** It is the only finding that blocks a migration on its own: retcon cannot
  move onto `Engine3D` without losing the readback its reference captures are taken through.
- **Steps 2 to 5 are the four renderer changes.** Each lets retcon delete one of its pipelines,
  and each is useful only once retcon is on the frame model, so they follow step 1. They are
  independent of each other.
- **Steps 6 to 10 are small engine-layer helpers.** Each stands alone and can be taken in any
  order, or between the others.
- **Step 11 starts with its ADR.** The interface, the null backend and the FMOD backend follow it,
  in that order, because the FMOD backend is the one that has to fit the interface.

What retcon does after each step lands is retcon's work: it bumps its pin, re-baselines its
reference captures where a step says they move, and deletes the handoff.

## Steps

| | What | Where | State |
|---|---|---|---|
| [1](#step-1--engine3d-captures-the-frame-it-presents) | `Engine3D::capture()` | `api/render/realtime` | Built; CI's window unknown |
| [2](#step-2--lit-takes-a-front-face) | A front face on `renderer::Lit` | `api/render/realtime` | Done |
| [3](#step-3--a-texture-names-its-sampler) | A sampler spec on `TextureFactory::create()` | `api/render/realtime` | Done |
| [4](#step-4--a-pass-decides-whether-depth-is-written) | Depth writing as a pass's dynamic state | `api/render/realtime` | Done |
| [5](#step-5--quad-linearises-into-an-srgb-target) | `renderer::Quad` linearising into an `_SRGB` target | `api/render` | Done |
| [6](#step-6--the-device-describes-itself-without-vulkan-types) | Device information without Vulkan types | `api/render/realtime` | Done |
| [7](#step-7--immediate-blocks-a-rectangle) | `Immediate::block()` | `api/ui` | Done |
| [8](#step-8--an-entity-reference-that-knows-its-entity-is-gone) | `ecs::Ref` | `api/ecs` | Done |
| [9](#step-9--an-isometric-camera-controller) | A command-driven controller for `camera::Isometric` | `api/engine` | Done |
| [10](#step-10--picking-and-the-entry-point) | `Camera::screenPoint()`, `pick()`, and an `SDL_main` header | `api/type`, `api/engine` | Done |
| [11](#step-11--audio-events-and-parameters) | Audio events and parameters | `api/audio` | Open |

### Step 1 — `Engine3D` captures the frame it presents

`Engine3D::renderFrame()` acquires, records and presents in one call, and offers no point between
recording and presenting. `Capture` needs exactly that point. An app on `Engine3D` therefore
cannot take a screenshot or run a golden-image test of its own window.

- `Engine3D::capture(std::string_view path)` names a file for the next `renderFrame()`. That frame
  calls `Capture::record(commands, swapchain, image)` after `Recorder::record()` and before
  `present()`, waits on `ring()->waitIdle()`, and writes the file. The request is then cleared,
  so a path that cannot be written fails once.
- Failure is reported as the rest of `Engine3D` reports it. A chain that is not
  `Swapchain::copyable()` is a failure, not an exception escaping `renderFrame()`.
- A frame that is skipped or out of date keeps the request for the next frame that presents.

**Settle first: how the gate runs.** The test needs a window. One option is a hidden SDL window
in the render test suite, skipped with exit code 77 when no surface can be made. The device tests
already skip that way without a device. The other is a test outside CI. Find out whether the
Windows CI runner can create a window before choosing.

**State, 2026-10-07.** Built, with the gate below in `render_window`, a third render suite under
`api/render/tests/window/`. Each case opens a small visible SDL window. `Window` has no hidden
flag, and adding one only for a test was not worth an api change. The suite exits 77 when no
window can be made, so a machine without a display skips it. It passes locally, and fails with
the copy moved before `Recorder::record()`. What is left: a CI run on a pull request, to learn
whether the Windows runner can open a window. If it can, the workflow fails on a skip of this
suite as it does for `render_device`. If it cannot, the skip stays and this test runs only
locally.

**Gate.** Request a capture, render one frame of a known clear colour, and read the file back to
that colour. Vacuous if it asserts only that the file exists: a capture recorded before
`Recorder::record()` also writes a file, of the previous frame. A second case asserts the request
is cleared: a second frame writes nothing.

### Step 2 — `Lit` takes a front face

`Lit` builds its cel, outline and shadow pipelines with the front face fixed at clockwise. That
is right for `Profile::Hand::UpCrossDirection`. Under `DirectionCrossUp`, the basis `glm::lookAt`
builds and the one retcon's camera uses, the image is mirrored and every cull mode culls the
wrong side.

- A `VkFrontFace front = VK_FRONT_FACE_CLOCKWISE` parameter, last on the constructor, read by all
  three `cull()` calls in `Lit.cxx`. Taking a `Profile::Hand` instead would tie a renderer to the
  camera type, and the face is what the pipeline needs.

**Done, 2026-10-07.** The shadow pass needed more than the face. `shadow::light()` builds the
light's view in `UpCrossDirection`, so a counter-clockwise face culled the caster's near side and
the map stored its far side. `shadow::light()` therefore takes a `Profile::Hand` too, defaulting
to the current one. The gate's shadow check is the exact depth case run in the mirrored hand,
which a closed mesh's picture could not have caught: its far side still casts. Each of the four
cull and hand sites, reverted alone, fails a case.

**Gate.** A picture in `LitSceneTest`: a closed mesh wound counter-clockwise, drawn by a `Lit`
built with `VK_FRONT_FACE_COUNTER_CLOCKWISE`, shows its lit faces and its outline. Vacuous if only
the cel pass is checked: the outline culls front faces, so a change that reached the cel and
shadow pipelines alone would pass and draw the outline inside out. The picture checks the
outline ring and a shadow it casts.

### Step 3 — A texture names its sampler

`TextureFactory` attaches one sampler to every texture, linear and `CLAMP_TO_EDGE`. That is right
for an atlas and a sprite, and wrong for a surface meant to tile. `Textures::material()` and
`MeshRegistry` bind a texture's own sampler, so neither can serve a tiling material.

- `TextureFactory::create()` takes a `const pipeline::Sampler::Spec&`, defaulting to the current
  spec. A default spec shares the factory's sampler, as now. Any other is made once per distinct
  spec and shared, so a scene of tiling textures holds one repeating sampler.
- `Textures::texture()` passes it through. `Sampler::Spec` needs an equality operator.

**Done, 2026-10-07.** Both `Textures::texture()` overloads take the spec last. The gate is
`texture_sampler_test` in the device suite.

**Gate.** Create one default and two repeating textures. The default's sampler differs from the
repeating ones, and the two repeating textures share theirs. Vacuous if it asserts only that a
repeating texture has a sampler: the default passes that.

This is one of three things between retcon and `MeshRegistry`. The other two are retcon's: its
albedos are `Encoding::Display` where `MeshRegistry` uploads `Srgb`, and a missing albedo throws
there where `MeshRegistry` draws white and warns.

### Step 4 — A pass decides whether depth is written

`Line`'s depth pipeline tests and writes depth. `World`'s tests and does not write. An overlay of
translucent, coincident lines needs the second: with writes on, the first of two lines on a shared
edge fails the second's `LESS` test, and draw order decides the colour. The editor needs the
first: it draws only wireframes, and a near wireframe hides a far one because lines write depth.
`Line` is one shared instance per context, so a choice made when it is built is made for every
app on that context.

**Decided**, 2026-10-07: depth writing becomes dynamic state that a pass sets, the way depth bias
already is. Vulkan 1.3 has `vkCmdSetDepthWriteEnable` in core. ADR-0085 records the choice and
its rivals: a flag on `Line`'s constructor, a choice on each `submit()`, and changing `Line`'s
default.

- **`Builder::depthWriteDynamic(bool)`** adds `VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE`. The write flag
  given to `depth()` becomes the pipeline's own value, used when the pass names none. Off by
  default, so every other pipeline is built as now.
- **`Resources` records it per pipeline**, beside `biased`.
- **`Pass::depthWrite(std::optional<bool>)`**, empty by default. An empty pass draws every
  pipeline as it was built, so the editor's picture does not change.
- **`Recorder` sets the state each time it binds an opted-in pipeline**: the pass's value if it has
  one, the pipeline's own otherwise. A pipeline that declares the state and is drawn without it
  being set is a validation error, so the set is unconditional.
- **`Line` and `World` opt in.** `Lit` does not: a lit surface is opaque and always writes, and a
  pass that turns writing off leaves it alone.

**Done, 2026-10-07.** As designed. The line cases are `depth_write_test` in the device suite, and
the lit case is in `LitSceneTest`. Each of four faults fails a case: the pass ignored, writing
off when the pass names nothing, the state left unset then, and `Lit` opting in.

**Gate.** Three pictures and one silence:

- A red line then a green line along the same pixels at the same depth, in a pass with
  `depthWrite(false)`, read green. The depth buffer still holds the clear value when the green
  line is tested, so it passes.
- The same two lines in a pass that names nothing read red, as today. Vacuous without this case:
  a change that turned writing off everywhere would pass the first.
- In a pass with `depthWrite(false)`, a near `Lit` mesh drawn first still hides a far one drawn
  after it. This shows the pass reaches only the pipelines that opted in.
- Each case is silent under validation. A missed `vkCmdSetDepthWriteEnable` shows up there before
  it shows in a picture.

### Step 5 — `Quad` linearises into an sRGB target

`quad.frag` writes display-referred colour. A UNORM target stores it unchanged. An `_SRGB` target
encodes on store, so the same value lands a shade too bright. An app whose scene is linear and
whose chain is `_SRGB` hits this as soon as it draws UI over the scene.

- `Quad` decides from the colour format it is already given: an `_SRGB` format gets a fragment
  variant that converts to linear before writing, and any other format gets the current one. No
  new parameter, and every caller in the tree is on UNORM.
- The variant is a second embedded shader, or a specialisation constant on `quad.frag`. The
  constant keeps one source file.

**Done, 2026-10-07.** The variant is a second compile of `quad.frag` with `LINEARISE` defined, as
the skinned lit shaders are made, which keeps one source file without adding specialisation
constants to `Builder`. `Quad::linearises()` names the formats. The gate's cases are in
`OffscreenFrameTest`. The sRGB case allows one either side of `0x80` for the encode's rounding.
The local GPU reads exactly `0x80`; lavapipe is checked only in CI.

**Gate.** Two pictures. A `#808080` quad drawn into an `R8G8B8A8_SRGB` target reads back as
`0x80` through `Capture::convert`, and the same quad into a UNORM target reads back `0x80` too.
Vacuous if only the sRGB case is tested: a change that linearised unconditionally would pass it
and darken every UNORM app's UI.

### Step 6 — The device describes itself without Vulkan types

`Device` offers `physical()` and nothing that names the device or its API version without a
Vulkan include. A diagnostics overlay needs both.

- A plain struct on `Device`, filled once at creation: the device name, the API version as major,
  minor and patch, the vendor ID and the driver version. No Vulkan type in it.
- `ui::shell::StatisticsOverlay` shows the device name, so the struct has a reader in this tree.

Waiting for the device to go idle is not a gap: `DeviceContext::ring()->waitIdle()` already needs
no Vulkan type.

**Done, 2026-10-07.** `Device::Description`, read by `description()`. The version is the one the
physical device reports supporting. `selectPhysical()` checks that version against 1.3.
`Screen` names the device on the overlay it builds. Gates: `device_description_test` in the device
suite, and a case in `StatisticsOverlayTest` for the top line.

**Gate.** The struct's version matches the version the device was created against, read from the
headless device. Vacuous if it checks only that the name is not empty.

### Step 7 — `Immediate` blocks a rectangle

`Immediate::capturing()` reports whether the last frame's windows or widgets were under the
cursor. A modal backdrop drawn by some other renderer over the whole viewport is neither, so a
click on it reaches the scene below.

- `Immediate::block(min, max)` counts toward `capturing()` exactly as a window does, and like a
  window is answered one frame behind.

**Done, 2026-10-07.** The case is in `ImmediateTest`. An empty `block()` fails it.

**Gate.** A blocked rectangle with no widget in it makes `capturing()` true for a cursor inside it
on the next frame, and false for one outside it. Vacuous if a widget is drawn in the rectangle
too, which passes today.

### Step 8 — An entity reference that knows its entity is gone

EnTT recycles identifiers, so a stored `entt::entity` can come back naming a different entity.
Any app that keeps a selection, an inspector target or a follow target across frames has this
hazard.

- `v3d::ecs::Ref` with `get(registry)`, `set()` and `clear()`. `get()` returns `entt::null` once
  the entity has been destroyed, by checking the stored entity's version against the registry.
- Its comment says it is for view state. A container the rules maintain, such as a turn order, is
  kept correct by whatever destroyed the entity, and checking on read there would hide a fault in
  that maintenance.

**Done, 2026-10-07.** Header only, in `api/ecs/Ref.h`. `get()` is `registry.valid()`, which is
the version check. An index-only lookup fails the gate.

**Gate.** Destroy the entity, create entities until EnTT recycles its index, and assert the
reference still reads null. Vacuous without the recycle: a plain destroy then read passes for a
reference that only compares identifiers.

### Step 9 — An isometric camera controller

`camera::Isometric` is the orbit, and its `pan()` already moves along the view's own axes, so a
pan still means up after a rotate. What each game still writes is the input: step the azimuth on
a press, pan and zoom at a speed while a key is held.

**Decided**, 2026-10-07: the controller lives in `api/engine` and reads commands, not keys. The
keys are bound in the app's binding document as every other command is, so they can be rebound
from settings.

- **`engine::IsometricController`** over an `Isometric` the app owns. Its bindings are command
  names (`context::name`): rotate left and right, pan in four directions, zoom in and out. Its
  speeds are parameters: world units a second for panning and orthographic height a second for
  zooming.
- **Rotation listens on `sink<event::Event>`** and steps once for a `Pressed` command that is not
  a repeat. A held rotate key does not spin the camera.
- **Pan and zoom are read with `held()` from `simulate(step)`.** The app's `simulate()` calls the
  controller's, so movement is measured in seconds and does not depend on the frame rate.
- **The listener is disconnected when the controller is destroyed**, so a controller released in
  `release()` leaves nothing on the dispatcher.
- `api/engine` already links `api/type` and `api/input`, so no link changes. Each pan, rotate and
  zoom goes through `Isometric`'s own methods, which keep the clamps and the wrap in one place.

**Done, 2026-10-07.** `held()` is passed in as a callable, with a constructor that takes the
`Engine`, so the gate runs with no window. The cases are `isometric_controller_test` in the engine
suite, with a fourth for the disconnect. Each of three faults fails a case: rotating on a repeat,
moving per call, and panning along world axes.

**Gate.**

- A pressed rotate command turns the azimuth one step, and a repeat of it does not. Vacuous
  without the repeat: a controller that rotated on every event passes a single press.
- Two simulate steps of half a second move the target as far as one step of a second. Vacuous
  with one step size, which a per-call move passes.
- A pan after a rotate command moves the target along the rotated axis. `Isometric`'s own test
  covers the axes. This case covers the controller handing the pan to them rather than to a
  world axis, and is vacuous at azimuth zero, where the two agree.

### Step 10 — Picking, and the entry point

Two small helpers, taken together because each is a few lines.

- **`Camera::screenPoint()`** returns `std::optional<glm::vec3>`, empty for a point behind the
  camera or a viewport with no area. `project()` stays as it is. A free `pick(camera, cursor,
  viewport, plane)` is `ray()` followed by `Ray::intersects(Plane)`, the question every grid game
  asks.
  **Gate.** A point behind a perspective camera has no screen point. Vacuous under an orthographic
  camera, where w is always 1 and nothing is ever behind it.
- **An `SDL_main` header beside `run<T>`.** pong, tetris and voxel each include
  `SDL3/SDL_main.h` in their main file, and it must be in exactly one translation unit. A header
  in `api/engine` that includes it, documented as the one the file calling `run<T>` includes,
  removes the three copies. **Gate:** the three apps build and link with their own include
  removed.

**Done, 2026-10-07.** `pick()` is a member of `Camera` beside `ray()`, rather than a free
function. `api/engine/Main.h` is the header. Five apps carried the include, not three: vertical3d
and odyssey did too, and all five build and link on `Main.h`. The starter example is a console
executable and includes neither. A log call given one argument for two fields fails to compile
with C7595, so the logging finding is closed and the engine's logging section says so.

Format-checked logging is not a step. `Logger::get()` returns spdlog's logger, which checks its
format string at compile time under C++23. Confirm that with one deliberately wrong call before
closing the finding.

### Step 11 — Audio events and parameters

`api/audio` plays clips by key on named buses through SDL3_mixer. retcon's audio is FMOD Studio:
banks, named events and global parameters that a bank's mix reads. ADR-0021 chose SDL3_mixer, and
SDL3_mixer cannot back events or parameters.

**Decided yes.** `api/audio` grows events and parameters beside clips. In order:

- **ADR-0084**, amending ADR-0021: why an event and parameter interface, why FMOD behind it, and
  what SDL3_mixer does when asked for an event. Written before the code.
- **The interface.** Load a bank, play a named event, set a global parameter. Clips, buses and
  fades stay as they are.
- **A null backend**, chosen when no event backend is configured, so an app that plays events
  builds and runs without FMOD.
- **An FMOD Studio backend** behind a configure option, off by default. Its SDK is fetched at
  configure time and never committed, because its licence does not allow redistribution.
  CI builds without it.

**Gate.** The null backend accepts a bank, an event and a parameter and reports each as not
played. The FMOD backend has no CI gate, because CI has no SDK. It is proved by running an app
against a real bank. Vacuous if the null backend's test only checks that nothing throws.

## Closing

When every step is closed, or closed as declined, this plan moves to
[completed/](completed/) with an Outcome section. A declined step's reasoning goes into the
document that owns its subject, and anything unfinished goes to [TODO.md](../TODO.md).
