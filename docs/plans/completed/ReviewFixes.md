# Review Fixes — The Defects Found In A Review Of The Branch

Drafted 2026-10-05 from a code review of `feat/motion-and-queries` against `main`: 71 commits
and about 1,300 files. Ten reviewers read the branch by area. No finding was high severity. This
plan fixes every finding, in 30 steps over eight phases.

Each step states its defects and the failure each one causes, because the review has no document
of its own. A finding marked **verified** was re-read in the code after the review. A finding marked
**unconfirmed** comes from reading alone and may not happen in practice. The step that owns it
starts with a test that shows the failure. When no such test can be written, the step records why
and drops the finding.

## Decisions

No step needs a new ADR. Two steps touch existing records:

| ADR | Effect | Step |
|---|---|---|
| [0059](../../adr/0059-ui-enabled-is-an-inherited-flag.md) | **Extended** by step 22 if a hidden ancestor makes a component unusable. If step 22 clears focus on hide instead, 0059 is unchanged | 22 |
| [0010](../../adr/0010-meshes-owned-by-the-app-that-built-them.md) | **Header corrected** by step 30: 0061 amends it, and its header does not say so | 30 |

Four choices are left to the step that meets them. Each is listed under
[Open questions](#open-questions) with a recommendation.

## What blocks what

Most steps are independent. The dependencies are these:

```
Phase 1 — shading language        Phase 2 — offline renderer, moya
1  equality on every component    6  shader failure, sphere, filter comment
2  setters write their argument   7  traced triangles carry per-vertex colour
3  condition read once            8  motion with a singular open end
4  return inside illuminance      9  moya's command line and stale comments
5  inference, initialise, solar

Phase 3 — realtime (verified on a PR, in CI)
10 depth barrier waits for the fragment shader
11 a shared target's depth decided across all its passes
12 a throw while recording leaves the frame usable
13 handle growth, upload rollback, ring leaks, depth-format check

Phase 4 — input and engine
14 key repeat is not a command ──> 15 pong ──> 16 voxel
17 the logger opens inside the catch
18 rebind failure, edges, lifecycle tests

Phase 5 — ui, font, image                Phase 6 — assets and geometry
19 hidden toolbar buttons                24 glTF primitive modes and image URIs
20 markup size, file chooser selection   25 geometry with bad input
21 parent pointers, submenus, slider     26 Previous after a teleport
22 a hidden component loses focus
23 image contracts

Phase 7 — build                          Phase 8 — documents and tests
27 links named by the user               29 tests the review found missing
28 test file names, stale counts         30 documents that state the wrong thing
```

Step 14 comes before 15 and 16 because it changes what a held key sends. Step 22 runs after 19,
because both change what visibility means for input. Phase 8 runs last, because steps in every
phase move things the documents describe.

## Steps

### Phase 1 — The shading language

In [`api/render/offline/sl/`](../../../api/render/offline/sl/). Tests go in
[`api/render/offline/tests/`](../../../api/render/offline/tests/).

#### Step 1 — `==` and `!=` compare every component

**Closed.** `Machine::compare` compares every component, and promotes a float to a matrix as an assignment does, as a diagonal. One case in `SlMachineTest.cxx`.

**Verified.** `Machine::compare` reads `left.number(point)`, which is component 0 only. The
compiler accepts `==` between colours, points, vectors, normals and matrices. So
`C == color(0, 1, 1)` is true whenever the red channel of `C` is 0.

Compare all `components()` for `EQUAL` and `NOT_EQUAL`. A float on one side is broadcast, as
`arithmetic()` already does. The ordered comparisons stay float-only, and the compiler rejects them
for other types if it does not already.

Tests: equal and unequal colours that share component 0, a point against a float, and a matrix.

#### Step 2 — The component setters are writing built-ins

**Closed.** The setters are declared with a new `Signature::updates`, the argument a built-in changes in place, rather than with `outputs`, whose arguments are written only. The compiler requires every written argument, `fresnel`'s included, to be a variable the shader may assign. Cases in `SlCompilerTest.cxx` and `SlMachineTest.cxx`.

**Verified.** `setxcomp`, `setycomp`, `setzcomp` and `setcomp` are declared with `declare()`, not
`writing()`, in `Builtins.cxx`. Their outputs index is therefore −1. This has three consequences:

- the compiler accepts `setxcomp(P + 1, 0)`, which writes to a temporary and does nothing;
- the compiler accepts `setxcomp(P, s)` on a read-only global;
- inference ignores the write, so in `point p = point(0,0,0); setxcomp(p, s);` the variable `p`
  stays uniform, and every lane receives lane 0 of `s`.

Declare them with `writing()` and output index 0. `Machine::builtin` treats an output argument as
written only. These built-ins read it too, so the write-through path needs to read the old value
before it writes the new one.

Tests: both compile errors, and a varying `s` written into a point that ends up varying.

#### Step 3 — A varying condition is read once

**Closed.** One case in `SlMachineTest.cxx`.

`emitConditional` emits `MASK_NOT` from the condition register after the true branch has run. A bare
variable as the condition is its own register, so the true branch can change it. In
`if (flag) { flag = 0; } else { y = 1; }` the lanes that ran the true branch then run the else
branch as well. This predates the branch, but the plan fixes it with the rest.

Copy a bare-variable condition into a temporary before the first `MASK`.

Test: exactly that shader, with a varying `flag`.

#### Step 4 — A `return` inside `illuminance` stays finished

**Closed.** Confirmed for `return` and for `break`, which did nothing at all inside `illuminance`. The construct now keeps its lanes in a loop entry, so both clear them as they clear any loop's. One case in `SlLightingTest.cxx`.

**Unconfirmed.** `nextLight` rebuilds each light's lanes from `round.base`, which it copies once.
`finish()` clears only `masks_` and `loops_`. A lane that returned during the first light comes
back for the next one and overwrites the return register. `break` has the same shape.

Write the test first: a function with `illuminance(...) { if (c) return C; }` and two lights. If it
fails, clear the finished lane in every open `illuminations_[].base` from `finish()`.

#### Step 5 — Inference, initialisation and `solar`

**Closed.** `Instance::write` returns false when the defaults fail, and its four callers treat that as a failed run. `solar` with an angle is reported, not implemented; see [open question 1](#open-questions).

Four small defects:

- **`Inference::run`** gives up after 64 rounds without a word, which leaves a symbol uniform that
  should be varying. Each round can only move a symbol from uniform to varying, so the loop always
  finishes. Loop until nothing changes, and remove the cap.
- **`Instance::write`** ignores what `machine->initialise()` returns. A failed prologue leaves the
  parameters at no defaults and the render continues. Log the failure and return it.
- **`Instance.cxx`** calls `std::ranges::none_of` without including `<algorithm>`.
- **`Machine::illuminate`** skips the cone test for `solar`, so `solar(axis, angle)` ignores
  `angle`. See [open question 1](#open-questions).

Tests: a dependency chain longer than 64 links, and whichever `solar` behaviour is chosen.

### Phase 2 — The offline renderer and moya

#### Step 6 — Shader failure, sphere radius and the filter comment

**Closed.** Moya logs and skips a sphere whose radius is not positive. A test fixture, `endless.sl`, reaches the failed-shader path.

In [`api/render/offline/`](../../../api/render/offline/).

- **`HitShader::shade`** returns `primitive.colour()` when the machine fails. The path with no
  shader returns `opacity * colour`. A transparent primitive whose shader fails therefore draws
  fully bright. Return `opacity * colour` on both paths.
- **`Sphere`'s constructor** calls `std::clamp(z, -radius, radius)`. The result is undefined when
  the radius is negative or NaN, and a Debug build asserts. Validate the radius before clamping,
  and log and skip a sphere whose radius is not positive.
- **`Sampling.cxx`** has a comment saying Catmull-Rom has "a support of two pixels whatever the
  width says". `filter()` cuts every kernel at half the filter width. The RenderMan interface
  evaluates a filter only inside the width it is given, so the code is right and the comment is
  wrong. Correct the comment for Catmull-Rom and sinc. Fix the "peaks at two" comment in
  `FilmTest.cxx` the same way.

Tests: a shader that fails on a half-transparent primitive, and a sphere with radius −1.

#### Step 7 — Traced triangles carry per-vertex colour

**Closed.** `Hit::colour` carries Cs at the hit. The ray hider matches the exact blend, and the reyes hider is within 0.1 of it. Cases in `TraceSceneTest.cxx` and `RayHiderTest.cxx`.

In [`moya/libmoya/RenderContext.cxx`](../../../moya/libmoya/RenderContext.cxx) and
[`api/render/offline/trace/`](../../../api/render/offline/trace/).

`RenderContext::trace` builds every `trace::Triangle` with the single current colour, `color_`.
`RIBHandler::build` reads per-vertex `"Cs"` onto the vertices, and the reyes hider draws the
gradient. Under `Hider "raytrace"` the same polygon renders flat. Shaders that call `trace()` or
`transmission()` see the flat colour under both hiders.

Give `Triangle` a colour per vertex. The tracer interpolates it with the barycentric coordinates it
already computes. The single-colour constructor stays and fills all three corners.

Test: a RIB fixture with a per-vertex `"Cs"` gradient, rendered under both hiders and compared by
pixel. Add a `Triangle` unit test for the interpolated colour.

#### Step 8 — Motion with a singular open end

**Closed.** Confirmed in both hiders. `MovingTransform::reference()` is the end a primitive is stored at: the open end, or the close end when the open one has no inverse. The motion cache is capped at a million matrices per grid, past which each micropolygon computes its own.

In [`moya/libmoya/Bucket.cxx`](../../../moya/libmoya/Bucket.cxx).

**Unconfirmed.** `Placement` stores `glm::inverse(motion.open())`. A motion block whose open end
scales an axis to zero has no inverse. Every moving grid then has NaN corners, `covers()` is false
everywhere, and the object disappears without a message.

`Motions` also caches one `mat4` per sample over the region a grid sweeps. A grid that sweeps most of
a 4096 by 4096 frame at 16 samples a pixel needs about 17 GB.

Write a fixture that scales from zero first. If it fails, compose the delta directly from the two
motion ends rather than through the inverse. If that is not possible, log the object and draw it at
its close placement. For the cache, measure first. If the bound is real, cache per bucket rather
than per grid, or compute the matrix per sample.

#### Step 9 — moya's command line and its stale comments

**Closed.** `--width`, `--height` and `--silent` are back, through `RIBHandler::resolution`. `clip` now carries `st` across a crossing through `crossing()`, as `split` does; like `split`, it does not carry colour or normal, because a piece takes those from the state its parent was submitted under. No error report was added: every request moya cannot honour is logged and the scene still renders, so there is no case where the retired renderer's report would fire.

In [`moya/`](../../../moya/).

- **Options the retired renderer had.** `--width` and `--height`, `--silent`, the extension check
  on `.rib`, and the "cannot render this scene" report. moya has none of them, and no document says
  so. See [open question 2](#open-questions).
- **`render()`** has a comment saying "the alpha and depth modes need planes the hider does not
  write". Both hiders resolve coverage and depth through `Film::resolve`. Write the alpha and depth
  display modes, or state in the comment that the RGB channels are the only output.
- **`RIBHandler::sphere`** has a doc comment saying "Not drawn: moya dices polygons only". The ray
  hider draws spheres. Say which hider draws it.
- **`Polygon::clip` and the free `clip(Polygon&, Frustum)`** have no caller outside
  `PolygonClipTest`. They stay, because the editor and later hiders are expected callers. At a
  crossing, `clip` builds a vertex from the point alone and drops colour, normal and `st`. Use the
  `crossing()` helper that `split` uses.
- **The `--grid` and `--bucket` help text** says these values override the scene. A scene's
  `Option "limits"` replaces them. Make the help text say so.

### Phase 3 — The realtime renderer

In [`api/render/realtime/`](../../../api/render/realtime/). Steps 10 to 12 change synchronisation. Each
needs a run with the synchronisation validation layer on and silent. The golden images confirm them
on a PR in CI, because this machine cannot compare pictures across drivers.

#### Step 10 — The depth barrier waits for the fragment shader

**Closed, no defect.** A barrier's first scope covers the stages it names and every stage logically earlier. FRAGMENT_SHADER comes before LATE_FRAGMENT_TESTS, so `depthTests` already orders the transition after the previous frame's reads. Synchronization validation is silent with and without the extra stage, so `Barriers.cxx` is unchanged. `a_shadow_map_is_shared_by_frames_in_flight` keeps two frames in flight on one shadow map as a regression case.

**Verified.** `depthForDrawing` in
[`Barriers.cxx`](../../../api/render/realtime/vulkan/memory/Barriers.cxx) sets `srcStageMask` to the
depth test stages only. A shadow map is one depth image shared by both frames in flight. Frame N's
lit pass samples it in the fragment shader. Frame N+1's shadow pass then transitions it, and nothing
orders that transition after frame N's read. Frame N+1 can overwrite the shadow map while frame N
still samples it.

Add `VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT` to the first scope, as `colourForDrawing` does, and
give the comment the same reason.

Test: a device test that draws a sampled depth target in two consecutive frames. Synchronisation
validation must stay silent.

#### Step 11 — A shared target's depth is decided across all its passes

**Closed.** Confirmed: validation reported both orders. `a_pass_without_depth_beside_one_with_it` in `DepthTargetTest.cpp`.

**Unconfirmed.** `Recorder::openTarget` transitions depth only if the first pass that writes the
target uses depth. `closeTarget` moves a sampled depth image to read-only only if the last pass uses
depth.

- If a pass without depth draws first, the depth image is never transitioned. A later depth pass
  renders into it in `UNDEFINED`.
- If an overlay without depth draws last, the depth stays in attachment layout. `Texture::descriptor()`
  declares it read-only.

Decide both from whether any pass that writes the target uses depth.

Test: both orders, as device tests, with validation on.

#### Step 12 — A throw while recording leaves the frame usable

**Closed.** The fence is reset by `Ring::submitting()`, immediately before the submit, and not by `begin()`. A frame abandoned before its submit leaves the fence signalled, so neither the next frame nor shutdown waits forever. Today only `run<T>` catches, and it ends the app, so the acquired image and its semaphore need no recovery. No guard was added for them.

**Unconfirmed.** `Ring::begin()` resets the frame fence inside `acquire()`. `Recorder::record` can
throw after that: a missing scene, a missing bias, a format mismatch, or a cycle in `Frame::order`.
Nothing then signals the fence. An app that catches the exception and keeps running waits forever
in the next `waitFrame`.

Reset the fence just before the submit that signals it, not when the frame begins. Record the
command buffer inside a guard that ends it and returns the frame to the ring if recording throws.

Test: a device test that makes `record` throw once, then draws the next frame.

#### Step 13 — Handle growth, upload rollback, ring leaks and the depth-format check

**Closed.** No app registers a target texture today. Registering the same image again now returns its handle. A frame Engine3D skips is counted by `Ring::skip()`, and `StreamRing` resets on `Ring::turns()`. The depth-format item is **no defect**: the world quad renderer draws a depth pipeline in a pass without depth, and validation accepts it. The `MeshRegistry` rollback has no test, because it needs a texture upload to fail part way.

- **`Textures::texture(RenderTarget, slot)` and `depthTexture`** add a new handle on every call, and
  each new handle makes a new material and descriptor set. Return the existing handle for the same
  target and slot. Release it when the target is recreated.
- **`MeshRegistry::upload`** can throw after it has acquired albedo textures for earlier parts.
  Their user counts are never dropped. Release what was acquired when an exception passes through.
- **`Ring`'s constructor** leaks the fences it has made if a later `vkCreateFence` throws. Hold each
  fence in an owner as soon as it exists.
- **`StreamRing`** resets its cursor only when a frame begins. A frame that never begins, because
  acquire returned out-of-date or skip, leaves the cursor advancing. Each canvas in that frame
  then allocates another geometry pair. Reset the cursor on every frame, begun or not.
- **`Recorder::check`** validates the depth format only when the pass uses depth. A pipeline built
  with a depth format and drawn into a pass without depth passes the check. Check both directions.
- **`DepthOrder`** breaks ties on texture before submission order, and a NaN key breaks the sort's
  ordering. Document the tie order in the header and in the test's comment. Order a NaN key last.

A unit or device test for each change.

### Phase 4 — Input and the engine

#### Step 14 — A key repeat is not a command

**Closed, differently.** Dropping repeats would have broken tetris, where a held A or D moves the piece through key repeat. Repeats still reach commands, marked by `Event::repeat()`, which a binding carries from the key to the command. The toggles ignore them: the game menu, pong's and tetris's statistics, tetris's debug mode and voxel's debug overlay. Cases in `KeyboardTest.cpp`, the event `EngineTest.cpp` and `GameMenuTest.cpp`.

In [`api/input/Keyboard.cpp`](../../../api/input/Keyboard.cpp).

**Unconfirmed.** `Keyboard::handleEvent` forwards every SDL key repeat as `Pressed`.
`keyboard_held_key_test` asserts this. A command bound to `state: pressed` therefore fires again at
the repeat rate. Holding Esc opens and closes the game menu in pong, tetris and voxel, and holding
the statistics key does the same in every app.

Publish the `Source` event only for a key that is not a repeat, because the bindings turn `Source`
into commands. `KeyDown` keeps its repeats. The ui shell's `Keyboard` reads SDL key and text events
itself, so typing and a held Backspace still repeat. Change `keyboard_held_key_test` to assert that a held key sends
one `Pressed`, and that `Engine::held` stays true.

#### Step 15 — Pong's paddles and pause

**Closed.** The paddle rules moved into `PongScene::steer()` and `PongScene::coop()`, where the pong suite can reach them. Three cases in `PongSceneTest.cxx`, and `GameStateTest.cxx` now expects the pause to survive a reset.

In [`pong/src/`](../../../pong/src/). Three defects, the first verified and the other two confirmed by
reading:

- **`handlePlayEvent`** ignores every paddle event while paused, releases included. Hold W, open the
  menu, release W and close the menu: the left paddle keeps moving. Apply a release whatever the
  pause state.
- **`steerOpponent`** sets the right paddle's travel in single-player mode. Switching to coop calls
  `reset()`, and `Paddle::reset()` leaves travel alone, so the right paddle keeps the AI's last
  direction. Clear both paddles' travel when the mode changes.
- **`GameState::reset()`** sets `paused_ = false`. A mode change from the open menu calls it, so the
  ball moves behind the menu. Remove the pause from `reset()`, so the menu decides it.

Tests in `PongSceneTest.cxx` for all three.

#### Step 16 — Voxel's movement follows held keys

**Closed.** `Player::move` takes whether the direction is held. A new `PlayerTest.cxx` is in the voxel suite.

In [`voxel/src/`](../../../voxel/src/).

`Player::move` flips a direction bit on every event. `Controller::handleEvent` returns early while
the menu is visible. Hold W, open the menu, release W and close it: forward stays on, and the next
press turns it off.

Read movement from `Engine::held` inside `simulate()`, and remove the toggle. This also makes the
movement independent of step 14.

Test: a `Player` case that sets held state and steps.

#### Step 17 — The logger opens inside the catch

**Closed, in part.** `Logger::open()` returns false and logs to stderr rather than throwing, and `run<T>` catches a throw from `shutdown()`. Non-ASCII paths are not fixed: `appPath()` and the asset manager convert through the ANSI code page as the logger does, so the fix is wider than this step. It is in [TODO.md](../../TODO.md#loading).

In [`api/engine/Application.h`](../../../api/engine/Application.h) and
[`api/log/Logger.cpp`](../../../api/log/Logger.cpp).

- **`run<>`** calls `Logger::open()` outside its try/catch. spdlog throws when it cannot open the
  file. An app installed in a read-only directory aborts with no message, which is the failure the
  catch block exists to prevent. Move `open()` inside the try. If it throws, log to the console
  sink and exit with a non-zero code.
- **`release()` and `shutdown()`** also run outside the catch. A lost device makes `waitIdle` throw
  during release. Put the shutdown call inside the same catch.
- **The log path** is a narrow `std::string`. spdlog opens narrow names through the ANSI code page
  unless `SPDLOG_WCHAR_FILENAMES` is defined. See [open question 3](#open-questions).

Tests: `Logger::open` on a path that cannot be created returns a failure instead of throwing, and a
non-ASCII directory name opens.

#### Step 18 — Rebinding, input edges and the lifecycle

**Closed.** The rebind finding is **no defect**: `build()` fails only on a malformed document, and `load()` never keeps one, so a rebind cannot fail. `Engine::keys()` says to read edges in `tick()` or `render()`. `engine_releases_once_whether_or_not_it_started_test` covers both outcomes of `start()`. Release before the window is destroyed needs a window, so it has no case.

- **`Bindings::rebind`** stores `rebindings_[command]` before `build()`. A failed build leaves the
  entry, and every later build applies it again. Restore the previous entry on failure.
- **Input edges.** `inputEngine_->flush()` clears `pressed()` and `released()` after every frame,
  even a frame that ran no `simulate()` step. No app reads edges in `simulate()`. State in the
  `Engine` header that edges are read in `tick()` and `render()`. Do not change the flush.

Tests in `EngineTest.cpp`:
- a failed rebind leaves the old binding in place;
- `start()` returning false still calls `release()` once;
- a second `shutdown()` does not release twice;
- `release()` runs before the window is destroyed.

### Phase 5 — ui, font and image

In [`api/ui/`](../../../api/ui/), [`api/font/`](../../../api/font/) and
[`api/image/`](../../../api/image/).

#### Step 19 — A hidden toolbar button is hidden

**Closed.** One case in `ToolbarTest.cpp`.

`Loader::loadToolbar` reads `visible` on each button. `Arranger::strip`, `Arranger::widest`,
`ComponentRenderer::draw(Toolbar)` and `Toolbar::buttonAt` never check it. A button set to
`"visible": false` still takes width, draws, highlights and sends its command.

Skip invisible buttons in all four places.

Test: a strip with a hidden middle button, checked for layout, hit testing and the command sent.

#### Step 20 — Markup size and the file chooser's selection

**Closed.** Both confirmed. The markup test that claimed to check an unset size passed the base size, so it checked nothing; it now passes 0. `a_new_listing_has_nothing_chosen` drives the chooser through a real container.

- **`Markup::size_`** defaults to 0, and `TextureTextBuffer` scales by `size_ / font->size()`. A
  markup that sets a font and no size draws glyphs of zero width. The comment says such a markup
  leaves the scale at one. Treat a size of 0 or less as the font's own size.
- **`FileChooser`** keeps the selected index when it lists a new directory. **Unconfirmed.**
  `SelectList::items` revalidates the index but does not reset it. Opening the directory at row 2
  leaves row 2 selected in the new listing, and `pick()` with no argument then opens or picks it.
  Clear the selection whenever the chooser lists a directory.

`FileChooserTest` constructs `FileChooser(nullptr)` in every case, so the container paths are never
run. Add cases with a real container. In `a_name_that_is_not_one_is_refused`, the string
`"art\a.json"` holds a BEL character, not a backslash. Make it `"art\\a.json"`.

#### Step 21 — Parent pointers, submenus, the slider and the strip corners

**Closed.** A holder's destructor disowns the items it adopted, through `Component::disown()`, because by `~Component` the holder's item list is already gone. `Immediate::text()` replaces the measure and write functions, so `Screen` keeps the layer. The dressing item is **no defect**: `Immediate` has a dressing of its own kind that the app's dress cannot fill. The `TabBar` item is **no defect** either: a bar starts on page 0, and an out-of-range index chooses none as documented. Its comments were made exact. Cases in `MenuBarTest.cpp`, `SliderTest.cpp`, `ToolbarTest.cpp` and `ScreenTest.cpp`.

- **`Component::parent_`** is a raw pointer. `~Component()` does not clear its children's pointers,
  and `usable()` follows them. A child an app keeps after its parent is destroyed reads freed
  memory. Clear each child's `parent_` in the destructor.
- **`MenuItem::submenu()`** stores the submenu without calling `adopt()`. A disabled menu or item
  therefore leaves its flyout items usable. `Menu::down()` and `Menu::activate()` also enter a
  disabled submenu. Adopt the submenu, and check `usable()` in both.
- **`Slider::drag`** maps the pointer across the full track. The thumb is drawn within
  `size.x - side`, so grabbing the thumb moves the value by up to half a thumb. Map over the same
  range the thumb is drawn in, as `Scrollbar` does with `dragOffset`.
- **`Arranger::stack`** gives each strip a corner from the strips listed before it. A left strip
  listed before a top strip spans the full height and overlaps it. Place the top and bottom strips
  first, whatever the order they are listed in.
- **`Screen::scale()`** rebuilds `Immediate` and `ComponentRenderer`, which loses window positions,
  folds and scroll offsets. It also passes `dressing()` only the line height. **Unconfirmed.**
  Keep the `Immediate` state across a rescale, and pass the full dressing.
- **`TabBar`** has a comment saying the first page shows until another is chosen. The code returns
  null when nothing is selected. Make the code fall back to the first page.

Tests for each in the existing ui test files.

#### Step 22 — A hidden component loses focus

**Closed.** `Engine::reachable()` is whether a component is in the tab order now. `Keys` drops the focus from one that is not, and passes the key on. `usable()` is unchanged, so ADR-0059 is too. One case in `FocusTest.cpp`.

**Verified.** `Keys::press` and `Keys::text` check `usable()`, and `usable()` checks only
`enabled`. Nothing clears focus when a component or its container is hidden. `FileChooser::close()`
hides its container while its name box has focus. From then on the box takes every printable key,
so the game sees no W, A, S or D until something is clicked. `GameMenu` hides a container the same
way.

Clear focus when the focused component or any of its ancestors is hidden, and have `Keys` refuse a
component that is not shown. See [open question 4](#open-questions) for whether `usable()` itself
checks visibility.

Tests in `FocusTest.cpp`: keys and text after the focused component's container is hidden.

#### Step 23 — The image contracts

**Closed.** The reader was rewritten row by row:

- it reads the pixels from the offset the file header gives, and the palette from after the info header, whatever that header's length;
- uncompressed 16 bits is 5-5-5, which the old code read as 5-6-5;
- 16 and 32 bits read through masks, the file's own under `BI_BITFIELDS`;
- 32 bits comes back as RGBA, and opaque when its alpha is zero everywhere.

The committed `2x2x32_green.bmp`, an external `BI_BITFIELDS` file, now reads instead of being refused. The orientation case builds its files byte by byte, so the reader and writer cannot pass by agreeing.

- **`Image(uint64_t len)`** is declared in `Image.h` and has no definition. Remove the declaration.
- **BMP orientation.** The writer stores a positive height, which means rows run bottom to top, but
  writes the top row first. The 24-bit reader never flips. The 8-bit path walks backwards for a
  top-down file. The result is that a BMP written here is upside down in any other viewer, and an
  external BMP loads upside down. Make the reader return rows top to bottom for both signs of
  height, and make the writer write rows bottom to top.
- **32-bit BMP.** The writer emits 32-bit files for RGBA images, and `reader::Bmp` rejects 32-bit
  files. Accept 32-bit in the reader.
- **16-bit BMP.** `convert16` indexes three-channel output with a two-byte stride. **Unconfirmed.**
  Write a 16-bit fixture first.

Tests: an orientation case for BMP, matching the existing one for TGA and PNG, and a 32-bit
round trip. Use fixtures made by an external tool, not only ones written here.

### Phase 6 — Assets and geometry

#### Step 24 — glTF primitive modes and image URIs

**Closed.** The fixtures are hand-built, because no exporter writes strips, fans or an image without `mimeType`. `make_modes_fixture.py` generates `primitive_modes.gltf` and `data_uri_texture.gltf`. Two cases in `GltfTest.cpp`, both failing before the change.

In [`api/asset/media/loader/Gltf.cpp`](../../../api/asset/media/loader/Gltf.cpp).

- **`appendPrimitive`** ignores `primitive.type`. Strips and fans are merged as triangle lists, and
  lines and points draw as garbage, with no warning. Convert strips and fans to lists. Skip lines and
  points with a warning, as the loader does for its other unsupported features.
- **`readerFor`** chooses a decoder from `image.mime_type`. A `data:image/png;base64,...` URI with
  no `mimeType` property gets no decoder, so the texture is dropped. `mimeType` is optional for such
  images. Read the type from the URI prefix when the property is absent.
- **Texture URIs** are stored without decoding, so `my%20tex.png` is not found. Call
  `cgltf_decode_uri` on relative URIs.

Fixtures: export a strip, a data URI without `mimeType` and a texture with a space in its name from
Blender, which gives real exporter output.

#### Step 25 — Geometry with bad input

**Closed.** The settings are public fields, so they are clamped where they are used rather than where they are set. Cases in `RayTest.cxx`, `EffectTest.cxx`, `WeatherTest.cxx` and the ecs `TransformTest.cpp`, all failing before.

In [`api/type/`](../../../api/type/).

- **`Ray::intersects(AABBox)`** reports a hit at distance 0 when the direction has a NaN component.
  Both `std::max` and `std::min` return their first argument when the second is NaN. Return false
  when the origin or direction is not finite.
- **`Transform::matrix()` and `interpolate()`** use the rotation without normalising it. An editor
  rotation built up by repeated multiplication drifts, and the matrix gains scale and shear.
  Normalise in `matrix()`.
- **`Weather::fall`** calls `std::clamp` with low above high when `ease` is negative, which is
  undefined. **`Emitter::owing`** casts a negative count to `uint32_t`. Clamp `ease`, `target` and
  `density` to zero or more where they are set.
- **`Frustum.h`** names moya and OpenGL in the `Depth` comments. Describe the range as the one
  `glm::perspective` builds by default. The `planes()` comment does not say the planes are
  unnormalised. Say so, so a caller knows `signedDistance` is not a true distance.

Tests: a NaN ray, a drifted quaternion, and a negative ease.

#### Step 26 — `Previous` after a teleport

**Closed.** `snapshot()` drops the `Previous<T>` of an entity with no `T`, so no hook is needed, and no app has to remember to connect one. One case in `PreviousTest.cpp`.

In [`api/ecs/Previous.h`](../../../api/ecs/Previous.h).

`settle` copies the current value, so it must run after the new value is written. The comment does
not say this. `Previous<T>` also stays when `T` is removed, so an entity that gains `T` again blends
from a stale value. State the order in the comment. Remove `Previous<T>` when `T` is removed, with
an EnTT `on_destroy` hook.

Test: remove and re-add `T`, then check the first interpolated frame.

### Phase 7 — The build

#### Step 27 — Links named by the user

**Closed.** libmoya links `v3dlib_log` PUBLIC, because `RenderContext.h` includes it, and `v3dlib_image` PRIVATE, because only its source writes images.

- **[`vertical3d/CMakeLists.txt`](../../../vertical3d/CMakeLists.txt).** `command/Placement.h` includes
  `api/dag/Transform.h`, but the editor gets `v3dlib_dag` only through `v3dlib_brep`. Link
  `v3dlib_dag` directly.
- **[`moya/libmoya/CMakeLists.txt`](../../../moya/libmoya/CMakeLists.txt).** `RenderContext` includes
  `api/image` and `api/log`, which it gets only through `v3dlib_render_offline`. Link
  `v3dlib_image` and `v3dlib_log` directly.

#### Step 28 — Test file names and stale counts

**Closed.** The eight `SL*` and `RIB*` files were renamed to `Sl*` and `Rib*`, which their neighbours and the list already used. There are 25 binaries over 1,537 cases. The paragraph in `build-resolver.md` also said `ecs`, `audio`, `log` and `engine` had no suite, which was stale too.

- **[`api/render/offline/tests/CMakeLists.txt`](../../../api/render/offline/tests/CMakeLists.txt)**
  lists `Sl*Test.cxx` and `Rib*Test.cxx`. The files are named `SL*` and `RIB*`. This builds only on a
  filesystem that ignores case. Match the file names.
- **[`.claude/agents/build-resolver.md`](../../../.claude/agents/build-resolver.md)** says there are 16
  test binaries. There are 25. Recount the cases from a ctest run, and update both numbers.

### Phase 8 — Tests and documents

#### Step 29 — Tests the review found missing

**Closed.** The lifecycle case drives `run<T>` with a real `Engine`, from step 18, so `ApplicationTest` keeps its stub.

The defect steps each add their own tests. These four cover claims that are untested today:

- `logger_open_moves_the_log_test` checks only that the file exists, and spdlog creates the file
  when it opens it. Write a message, flush, read the file back, and remove it afterwards.
- `MediaTest.cpp` has a comment saying a plain `Manager` loads only documents. Assert that loading
  a PNG and a GLB through one fails.
- `renderman_sampling_test` sets the depth of field and asserts only `fstop`. Assert `focalLength`
  and `focalDistance` too.
- `ApplicationTest` drives `run<T>` with a stub engine. Add a case that drives the real `Engine`
  headless, if the loop can run without a window. Otherwise record the gap in
  [Testing.md](../../contributing/Testing.md).

#### Step 30 — Documents that state the wrong thing

**Closed.** Games.md was made true rather than corrected: pong now copies its own data and Odyssey the shared fonts. That finished the TODO.md entry about pong's data, which was deleted. All 66 dead links in completed plans and roadmaps are repaired, so the tree's link check is clean. The reference documents state every rule this plan changed.

- **[Games.md](../../Games.md)** says every game copies its own `data/` and the shared fonts. pong
  copies only the shared fonts, and Odyssey copies only its own data. Correct it. Then either make
  each game call both functions, or state which one each game calls.
- **[TODO.md](../../TODO.md)** says "every other game calls both". Correct it with Games.md. Its entry
  about ADR-0001 says the ADR gives no reasons for Vulkan. The rewritten ADR does, so keep only the
  SDL3 half.
- **[Build.md](../../contributing/Build.md)** says `v3dlib_audio` links SDL3_mixer PUBLIC. It links
  PRIVATE.
- **[api/README.md](../../api/README.md)** names `vulkan::Presenter` in the glossary. The class is
  `vulkan::frame::Presenter`.
- **[FramesAndTargets.md](../../api/rendering/FramesAndTargets.md)** gives `viewport(x, y, w, h)`. The
  signature is `viewport(const glm::vec4& region)`.
- **[UsingTheApi.md](../../api/UsingTheApi.md)** links the roadmap and plans from a reference
  document. Remove the links.
- **[ADR-0010](../../adr/0010-meshes-owned-by-the-app-that-built-them.md)'s header** and its row in
  the index do not list `Amended by: ADR-0061`. Add it.
- **Dead links in completed plans.** About 10 broke on this branch and about 53 were already broken.
  Point anchors at the page the section moved to. Point a link to a deleted source file at the
  nearest surviving file, or drop the link and keep the name as code. A completed plan's text does
  not change otherwise.

Last, re-read every comment the plan added against
[Conventions.md](../../contributing/Conventions.md#writing).

## Verification

Per [sdlc.md](../../sdlc.md) §4, for every step:

- **Build.** `scripts\build.cmd`. Steps 14 to 18 reach every app.
- **Tests.** `scripts\test.cmd`. Every defect arrives with a test that fails before the fix. An
  unconfirmed finding whose test passes before any change is dropped, and the step says so.
- **Lint and analysis.** cpplint, `/W4 /WX`, and the `/analyze` and clang-tidy trees.
- **Run.**
  - moya under both hiders, for steps 6 to 9;
  - pong, tetris and voxel, for steps 14 to 18, holding keys across the menu;
  - the editor, for steps 21, 22 and 25, with the file chooser opened and closed.
- **Pictures.** Steps 10 to 13 need the synchronisation validation layer on and silent locally,
  then the device suite and golden images on a PR.

## What this plan does not do

**It does not change the colour grade's domain.** The 16-entry grade LUT is sampled by linear colour,
and a strip authored for display may grade wrongly. This is a design question with no failing case.
It goes to [TODO.md](../../TODO.md).

**It does not refit the shadow frustum to mesh extent.** `Shadow::fit` covers caster positions plus a
margin the caller sets. A caller that sets a small margin for large casters loses shadows. The
margin is the documented control. TODO.md records fitting to bounds.

**It does not stop the reyes hider adding every polygon to the traced scene.** The hider does this
on purpose, so that off-screen casters still cast shadows. Adding geometry only when a shader traces
is an optimisation, and it goes to TODO.md.

**It does not port the build off Windows.** Step 28 fixes the file-name case because it costs
nothing.

## Open questions

1. **`solar` with an angle.** *Answered: reported.* An angle lets L be any direction inside a cone,
   chosen against the surface's own illuminance cone. A light shader is not given that cone, so the
   per-point test `illuminate` uses does not apply. The machine reports the angle once and lights
   along the axis. Implementing it means passing the surface's cone to the light, which is
   [TODO.md](../../TODO.md) work.
2. **The retired renderer's options.** Add `--width`, `--height` and `--silent` to moya, or record
   that they are gone. *Recommendation:* add all three, and make the error report a log line when
   `RIBHandler` meets a request it cannot honour. Do not add the `.rib` extension check, because the
   reader decides what it can parse.
3. **Wide log paths.** *Answered: deferred.* The vcpkg spdlog port is built without
   `SPDLOG_WCHAR_FILENAMES`, and `appPath()` and the asset manager lose the same characters the
   logger would. The fix covers all three, and is in [TODO.md](../../TODO.md#loading).
4. **Whether `usable()` checks visibility.** *Answered: it does not.* `Keys` asks
   `Engine::reachable()`, which also covers a hidden container and a tab page that is not up.
   "Enabled" and "shown" stay separate properties, and
   [ADR-0059](../../adr/0059-ui-enabled-is-an-inherited-flag.md) is unchanged.

## Outcome

Drafted and closed on 2026-10-05. Every step is closed.

- **Fixed.** Of the findings, these were real and are fixed, each with a test that failed
  before the fix:
  - two shading-language defects in comparison and the component setters, and two in
    conditionals and `illuminance`;
  - per-vertex colour and motion that starts from nothing, under both hiders;
  - a shared target's depth layout;
  - input left stuck across pong's and voxel's menus;
  - hidden components keeping the focus;
  - a BMP reader that read most files upside down and refused 32 bits;
  - glTF strips, fans and data URIs;
  - geometry given NaN or out-of-range settings.
- **No defect.** Four findings were not defects, and each step says why:
  - the shadow map barrier (step 10);
  - the depth-format check (step 13);
  - the rebind failure (step 18);
  - the TabBar selection and the immediate layer's dressing (step 21).
- **Done differently.**
  - Key repeats are marked rather than dropped, because tetris moves a piece through them
    (step 14).
  - `solar`'s angle is reported rather than implemented (open question 1).
- **Moved to TODO.md.** Non-ASCII paths (step 17), `solar` with an angle, the colour grade's
  domain, fitting the shadow map to caster extent, and the reyes hider tracing every polygon.
- **Verified.**
  - The build, all 25 test suites with 1,537 cases, and cpplint are clean.
  - The `/analyze` and clang-tidy gates are clean. They found five new sites on the way, all
    in code this plan changed.
  - The device suite is clean under synchronization validation locally. The golden images
    across drivers are checked on a PR.

### After a second review

A second review of the branch found one defect the first round missed, and gaps in this
round's own fixes. All are fixed, each with a test where the fix is code:

- tetris's `dropPiece` is a toggle and took key repeats; `GameBoard::dropTetrad` now ignores one;
- a focused component hidden by a closing dialog still took the first `tab` or `escape`;
- an end flattened on one axis decomposed to the wrong rotation, and a motion flat at both ends
  now logs once and is left out;
- the bound of a moving primitive stored at its close end left out its open end;
- a failed shader prologue now logs, and a machine's reports reach the log;
- a material that fails after its texture is made releases that texture, and a replaced submenu
  is disowned;
- out-of-range emitter rates, a NaN weather target, an infinite rotation, a throwing
  `release()` and bad `--width` and `--height` values are handled;
- the motion cache's limit is `RenderContext::motionCache()`, so its fallback is tested;
- the documents and comments the review named state the rules as they are.

A fully transparent 32 bit bmp reads back opaque. That is the stated trade for writers that
leave alpha at zero, and it stays.
