# Modernization Plan

Draft, 2026-08-30. Living document — update the state notes as things land.

Several rewrites are in flight at once. This is an attempt to work out what actually
blocks what, so they can be finished in an order that keeps the tree buildable instead
of half-finishing all of them at the same time.

**All six phases are done as of 2026-09-04**, and with `v3dlibs/` deleted there is no legacy
tree left. What remains is the two ongoing workstreams and a set of open items carried in the
phase notes — collected under [What is left](#what-is-left) at the end rather than scattered,
because they are no longer phased work.

## The rewrites

1. SDL2 → SDL3
2. Legacy `v3dlibs/` and `luxa/` folded into the `api/` layout
3. OpenGL → Vulkan, with GL support removed entirely
4. The 2D and 3D realtime engines consolidated into a single Vulkan engine
5. pong and tetris moved onto the new `api/` framework
6. voxel and odyssey after that — neither was built on the v3d frameworks, so both need
   api features that do not exist yet
7. the Vertical3D editor rewritten onto the new api, folding in what rigel prototyped
8. moya and talyn — offline renderers, no realtime dependency

Plus two things that need backfilling throughout: documentation and tests.

## Decisions

Recorded in [docs/adr/](../adr/), not here. The ones that shape this plan:

| ADR | Decision |
|---|---|
| [0001](../adr/0001-vulkan-replaces-opengl.md) | Vulkan replaces OpenGL outright — no dual backend, no fallback |
| [0002](../adr/0002-target-vulkan-1-3.md) | Target Vulkan 1.3, for dynamic rendering and synchronization2 |
| [0003](../adr/0003-one-realtime-engine.md) | One realtime engine, with the render pass as the unit of variation |
| [0004](../adr/0004-operations-as-draw-data.md) | Operations are draw data; the engine sorts, merges and records |
| [0005](../adr/0005-one-batched-quad-primitive.md) | One batched quad primitive with an optional texture |
| [0006](../adr/0006-keep-both-pong-and-tetris.md) | Both pong and tetris are kept |
| [0007](../adr/0007-ci-rendering-tests.md) | Render tests run on a Windows runner against software Vulkan |

## Where things actually stand

Verified against the tree on 2026-09-04.

**SDL3 is done.** No SDL2 code remains anywhere. The dead references went on 2026-08-31:
`talyn`'s `${SDL2_LIBRARIES}` link, `odyssey/cmake/FindSDL2.cmake`, and a stale comment in
the root `CMakeLists.txt` claiming `find_package(SDL3)` resolves through a `FindSDL2` module.
The last of it was `v3dlibs/hookah/drivers/sdl2/`, which went with that tree on 2026-09-04.
This workstream is closed.

**The legacy trees are all deleted, as of 2026-09-04.** `vault/`, `rigel/`, `luxa/` and
`v3dlibs/` are gone, each after the list in its own survey or audit was worked off — in
vault's case, with no list to work off. Every directory in the root now builds, and there is
no tree left that "not built" says anything about.

- **`vault/quantumxml` was genuinely archived** — an XML parser superseded by the JSON
  config work, per `docs/Vault.md` and the first item on `docs/TODO.md`.
- **`rigel/` was the earlier prototype of the editor** and held functionality the rewrite had
  to absorb first — viewport layout, an arcball camera, a construction plane, transform
  manipulators, and the poly modelling command sets. The survey,
  [docs/RigelSurvey.md](../RigelSurvey.md), found none of it portable: the fold-in was a
  harvest of behaviour plus one XML data file, and its eleven-item list is Phase 6.
- **`luxa/` and `v3dlibs/` were migrations**, and each got the functional equivalence audit
  its removal needed rather than an assumption that `api/` covered it —
  [docs/LuxaAudit.md](../LuxaAudit.md) and [docs/V3dlibsAudit.md](../V3dlibsAudit.md), both
  closed.
- **`vertical3d/` is an app and was never in this category.** It is the desktop 3D editing
  tool the repository is named after; it was rewritten onto the new api rather than deleted,
  and is back in the root `add_subdirectory` list as of 2026-09-01.

What kept `v3dlibs/` alive longest was not code anyone still called. The test corpus moved
into per-library `api/<lib>/tests` on 2026-08-31; `vertical3d/` dropped its six legacy
includes with the rewrite of 2026-09-01 and `core/` moved into `vertical3d/src` on
2026-09-02, emptying that directory; `luxa/` was the last thing in the tree including a
`v3dlibs/` header, from seven of its own files, and went on 2026-09-04. The files deleted
rather than migrated - `luxa/tests/` and two v3dlibs tests - turned out to be empty stubs.

**The luxa audit is done, and closed.** Written up in
[docs/LuxaAudit.md](../LuxaAudit.md), 2026-08-31, worked off on 2026-09-04 and the tree
deleted with it. The nine item list as it stood: The short version:
`ComponentRenderer` has to be rebuilt on the Vulkan quad rather than ported, and takes the
ortho UI pass and theme-to-font resolution with it; `ComponentManager` is only half covered
by `ui::Engine` plus `ui::Container`, with mouse hit-testing, hover/focus, the active theme
and image loading all unported; and `ui::Engine::load` covers menus alone, so the whole of
`api/ui/style/` is migrated but unreachable. Two of the three flagged unknowns shrank on
inspection — `Window` is an empty stub with no implementation, and `MenuStack` is a `draw()`
routine whose navigation was always in `Menu`, which migrated intact, so `ui::Navigation`
needs no reconciliation. What closed it: the renderer was rebuilt on the batched quad with
the menu bar (ADR-0019), hit-testing arrived with it against the bounds a draw leaves, and
the theme half landed as
[ADR-0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) - a theme is JSON,
`ComponentRenderer` draws with the `ui` style it names, and an image the config names is
resolved to a texture by a callback the app supplies rather than by the library loading it.

The audit also turned up regressions in `api/ui` itself, none blocked by Vulkan. The worst:
`Menu::activate()` had its dispatch commented out, and the menu never navigated at all
because `Engine::loadMenu` set neither the active level nor the active item. Both are fixed
as of 2026-08-31, along with `MenuItem`'s missing value field and `ui::Engine`'s missing
theme accessor. Input capture for input-type menu items is still unbuilt, so the five input
items in `pong/data/vgui.json` remain unreachable.

**The v3dlibs audit is done, and closed.** Written up in
[docs/V3dlibsAudit.md](../V3dlibsAudit.md), 2026-08-31, worked off and the tree deleted on
2026-09-04. Most of it was genuinely covered: `input/` by `api/input`, `hookah/Window` by
`api/render/realtime/Window`, `gui/InputEventAdapter` by `api/event` plus `api/input`, and
`command/` by `api/event` — nothing still needed `CommandDirectory`, `CommandTable` or
`StateController`. What kept the tree alive was narrower than expected: the test corpus had to
move, and `vertical3d/` still included six of its headers. `core/Scene` and `SceneVisitor`
should *not* be folded into `api/dag` — that library is 374 lines of skeleton with no
traversal, no visitor and no consumers anywhere — so they went with the editor in Phase 6, as
`CreatePolyCommandSet` did. **All three moved on 2026-09-02**, which took `v3dlibs/core/` and
`v3dlib_core` with them; `command`, `gui`, `hookah`, `input` and `component` were what
remained, and they went with the tree once `luxa/` — the last thing including any of it — was
deleted.

The replacement for the command layer was less finished than assumed, and the audit turned up
`api/` defects alongside it — every one of them fixed on 2026-08-31. The worst were that
`api/input/Mouse::handleEvent` was empty and returned true, so **every mouse event in every
app was swallowed**; that `Event::operator<` compared only `context::name`, so key press and
release mapped to the same destination with the edge dropped; and that `Mapper`'s `std::map`
let a second binding on a key silently replace the first. `api/event` now has a `State` on
every event, a `multimap` of bindings, an optional `"state"` and `"param"` per binding, and
`dispatch(context, name)` for invoke-by-name; `api/input` has a working mouse with
`MouseState` and a `MouseMotion` event; `Engine::tick(delta)` supplies the frame delta again;
and `config::BindingContext` is deleted in favour of `event::Mapper`. Still open:
`event::Context::active` is written and read by nothing, which is the state scoping the
editor will need.

**Vulkan is the critical path, and as of 2026-08-31 a window clears to a colour.** Phase 2
is done. `Context3D` now owns a `vulkan::Presenter` - command pool, per frame command
buffers, image-available and render-finished semaphores, in-flight fences, two frames in
flight - and `Engine3D::renderFrame()` acquires, records, submits and presents, rebuilding
the swapchain whenever acquiring or presenting reports it out of date. Drawing is through
dynamic rendering with synchronization2 barriers; there is no `VkRenderPass` and no
`VkFramebuffer` anywhere. Verified by running pong with its GL setup stubbed out: it cleared,
survived five programmatic resizes, a minimize and a restore, with the Khronos validation
layer loaded and silent.

Getting there turned up one thing nobody had noticed: **`sdl3` was installed without its
`vulkan` feature**, so `SDL_Vulkan_LoadLibrary` failed with "No dynamic Vulkan support in
current SDL video driver (windows)" and pong aborted on startup. That is fixed in
`vcpkg.json`. It cost nothing but a five minute SDL rebuild, and it means the vulkan path had
never once run - "it builds" really was the only signal there was.

**Phase 3 put pixels down.** There is one pipeline - the batched quad of ADR-0005 - and pong
draws its whole frame, ui included, through it.

**Phase 4 took tetris off OpenGL.** It draws through the same canvas, with its seven block
textures packed into one atlas so the whole well is a single batch.

**There is no OpenGL left anywhere.** `api/gl` was deleted on 2026-09-01 with the last thing
that held it, and the root `find_package(OpenGL)` and `find_package(GLEW)` calls and the
`glew` port went with it. Voxel was the only app still calling GL, against a context nothing
created; odyssey never had any, drawing through `Context2D`, which was `SDL_Renderer`. Both
carried a stale `OpenGL::GL`/`GLEW::GLEW` pair in their `CMakeLists.txt` and neither does now.
A target naming either, or `v3dlib_gl`, will not configure.

**Build health.** Everything compiles and links, every app included.

**Odyssey runs and is ported, as of 2026-09-01.** It opens a 1280x768 window and draws one
64x64 sprite through the batched quad, at the tile its entity's `PositionFixed2D` names — and
that is the whole of what it draws, `Movement::tick` returning true and doing nothing while
`Sprite`, `SpriteSheet`, `Actor`, `Tile` and `ui::Screen` are empty declarations. It was the
last consumer of the `SDL_Renderer` path, so porting it is what let that tree and `api/gl` go.
Getting it to build first, on 2026-08-31, turned up three things: the `Operation2D` signature
mismatch was only the first, because fixing it let odyssey reach the link stage for the first
time and exposed a `CMakeLists.txt` that had never listed `engine/Engine.cpp` or
`system/Movement.cpp` — two files that had therefore never been compiled and had drifted as
far behind the api as voxel had — and a local `find_package(Boost COMPONENTS log)` that
narrowed `Boost_LIBRARIES` and lost boost::json.

**Voxel is ported, as of 2026-09-01**, and draws terrain through Vulkan: a depth tested,
sorted scene pass of its own pipeline, one draw item per meshed chunk over an app-owned
`vulkan::Mesh`, and a second painter-ordered pass of batched quads for the debug overlay and
the game menu. Its shaders are the first in the tree to read set 0.

**The voxel survey is what scoped that.** Written up in
[docs/VoxelSurvey.md](../VoxelSurvey.md), 2026-08-31, when **voxel did not run, and the render
port was the fourth thing wrong with it rather than the first.** It exited 1 before a window
opened, because it never called `v3d_add_app_data` and so had no `config.json` in the build
tree; with one there `Config::load` threw uncaught on `doc.at("configs")`, voxel being the
last app on the rejected inline `keys` format; given a valid config the entire Vulkan stack
came up clean — window, instance, device, swapchain — and then it segfaulted in `Renderer`'s
constructor, which streamed `glGetString(GL_RENDERER)` into a log message with no GL context
and no `glewInit` behind it. All three were observed, not inferred, and all three are fixed.

The survey also found what voxel needs that the api does not have, which is what makes this
a phase rather than a port: a depth buffer (`Pass::depth` is stored and ignored, and there is
no depth image in `api/render` at all), a way for an app to build a second pipeline without
forking `QuadRenderer`'s 150 inline lines, set 0 from
[ADR-0008](../adr/0008-binding-by-update-frequency.md) (created with zero bindings, never
bound), device-local buffers with a staging upload for static geometry, and the sorting that
a few hundred chunk draws is the first frame to need. Items one to three are what phase 6's
multiple viewports need too.

Six defects in `voxel/` came with it, none of them reachable while the app could not start.
The two that matter: `Scene` passes `Chunk` a world height counted in **chunks** where the
scaling wants **blocks**, so terrain is at most 4 blocks tall in a 64 block world and 768 of
1,024 chunks are empty by construction; and three of the six cross-chunk face occlusion
checks move to the neighbouring chunk without moving to its facing block, so those seams cull
the wrong faces.

**The 2D engine is deleted**, on 2026-09-01 with odyssey's port. It was never much: all of
`api/render/realtime/2D/` was roughly 610 lines of thin `SDL_Renderer` wrapping — `Context2D`
an `SDL_Renderer`, `Texture2D` an `SDL_Texture`, `Scene2D::collect()` returning an empty
`Frame` with the real collection commented out — and odyssey's entire use of it was to build
a texture from a surface, cache it, and emit one `Blit2DTexture` per renderable. That is why
the consolidation port had a small surface: the one operation it needed, a textured quad, was
the primitive tetris needed anyway. `Engine2D`, `Context2D`, `Scene2D`, `Texture2D`,
`Window2D`, `Blit2DTexture` and the `Operation` submission path they used are all gone.

## What blocks what

Nothing is blocked now — every phase is closed. This section is kept because the ordering it
worked out is what the phases were built on, and because its central claim held up.

The instinct was to queue everything behind Vulkan, and that was wrong for about half the
work. Blocked by the Vulkan frame loop, which landed on 2026-08-31: pong's renderer, tetris's
renderer, voxel, and odyssey's port onto the consolidated engine — and what those were waiting
on turned out to be the batched quad pipeline rather than the loop itself. Odyssey *building*
was never blocked; only its move off `SDL_Renderer` was, and the SDL path kept running until
the Vulkan one reached parity, which is exactly how it went.

Voxel turned out not to be blocked by the frame loop either, in the sense that mattered: it
never reached one. It exited before opening a window, for reasons that had nothing to do with
rendering. Getting it to start was unblocked by everything and did not wait for the api work
queued behind it.

Not blocked by anything, and all of it done: deleting the legacy trees, the SDL2 leftovers,
the `Operation` signature fix and odyssey with it, the config-format migrations for tetris and
voxel, voxel's missing `v3d_add_app_data`, the test framework, and docs. Roughly half the work
was in this bucket, and it made the Vulkan work easier to review by shrinking the noise around
it.

The one hard ordering constraint inside the Vulkan work was that the apps needed batched quads
and nothing provided them: `v3d::gl::Canvas` batched coloured quads but carried no texture
coordinates, so every tetris piece, all text and every odyssey tile was unserved. Because the
engine was consolidating, the primitive had to satisfy all three apps, and its shape had to be
decided before the frame loop was built around it or it would have been built twice. That is
[ADR-0005](../adr/0005-one-batched-quad-primitive.md), and it held: one quad serves a
rectangle, a sprite, a glyph and a menu panel across four apps and the editor's ui.

## Phases

### Phase 1 — Clear the ground

None of this is blocked. It shrinks the surface area everything else has to work against.

- ~~Delete `vault/quantumxml`. It is the only tree that can go without an audit first.~~
  Done 2026-09-04.
- ~~Audit `luxa/` against `api/ui`~~ — done, [docs/LuxaAudit.md](../LuxaAudit.md), and
  **the nine-item list is worked off and the tree deleted as of 2026-09-04**. The last two
  items were one piece of work, recorded as
  [ADR-0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md): a theme is
  data the ui engine reads, and an image it names is resolved to a texture by the app.
- ~~Audit `v3dlibs/` against the api libraries~~ — done,
  [docs/V3dlibsAudit.md](../V3dlibsAudit.md), and **the five-item list is worked off and the
  tree deleted as of 2026-09-04**. The last of them was `core/`, which moved across early
  rather than waiting on Phase 6 — the cheaper order the audit called for.
- ~~From the luxa audit: wire `Menu::activate()` to the dispatcher, restore `MenuItem`'s
  value field, give `ui::Engine` a theme accessor.~~ Done 2026-08-31. Menu navigation had to
  be fixed alongside — `level_`, `active_` and each item's owning menu were never set at
  load, and the `next()`/`previous()` wrap-around ran off both ends of `items_`, so the menu
  had not been navigating either. Input capture for input-type items is still missing; the
  value field it writes to now exists.
- ~~From the v3dlibs audit: implement `api/input/Mouse::handleEvent`, make key press and
  release distinguishable, give bindings a parameter, add invoke-by-name, restore the frame
  delta, settle `config::BindingContext` against `event::Mapper`.~~ Done 2026-08-31. Two more
  defects in the same machinery had to go with them: `Mapper` used a `std::map`, so a second
  binding on one key silently replaced the first and pong's right paddle had never worked;
  and `Keyboard::handleEvent` cleared key state on the wrong condition. Still open from that
  audit: `event::Context::active` is set and read by nothing, which is the state-scoping the
  editor will need.
- ~~Drop `v3dlib_core` from tetris's link list.~~ Done 2026-08-31. Tetris's link list is
  still short in the other direction — it includes `api/engine`, `api/event`, `api/gl`,
  `api/log` and `api/render` while linking only `v3dlib_image` — which is part of the
  "fix the link list" item below and will only surface once tetris compiles.
- ~~Salvage `v3dlibs/tests/` into per-library `tests/` directories.~~ Done 2026-08-31.
  `v3dlibs/tests/` is gone; six Boost.Test binaries now build from `api/<lib>/tests` and run
  under ctest, covering `type`, `brep`, `image`, `font`, `input` and `event` in 47 cases.
  Almost nothing was a path rewrite: the type and brep tests were written against
  `v3D::Vector3`, the image tests against an `ImageFactory` that now takes a logger, and the
  font and input tests against classes that no longer exist. Of the six command-layer tests,
  `BindTest`, `CommandDirectoryTest` and `EventInfoTest` became `MapperTest`, `EngineTest`
  and `EventTest`; the other three were dropped as covered. The three recoveries were a dead
  end worth recording: `BRepTest`, `CameraProfileTest` and all of `luxa/tests/` are empty
  test bodies, a zero-byte file and a log4cxx fixture, so nothing was lost when they were
  deleted. Running the corpus for the first time in years turned up eleven `api/` defects,
  including a BMP writer that corrupted the heap and a `Logger` constructor that threw on
  the second one built — which is to say tetris threw on startup. All are fixed and listed
  in [docs/V3dlibsAudit.md](../V3dlibsAudit.md).
- ~~Leave `rigel/` and `vertical3d/` alone. They belong to Phase 6.~~ Both settled there:
  `vertical3d/` was rewritten onto the api and `rigel/` deleted after the fold-in.
- ~~Delete `talyn`'s `${SDL2_LIBRARIES}` link and `odyssey/cmake/FindSDL2.cmake`.~~ Done
  2026-08-31, along with a stale `FindSDL2` comment in the root `CMakeLists.txt`. Neither
  changed a build: the variable was undefined and expanded to nothing, and nothing set
  `CMAKE_MODULE_PATH` to reach the module. The last SDL2 in the tree was
  `v3dlibs/hookah/drivers/sdl2/`, and it went with that tree on 2026-09-04.
- ~~Fix the `Operation::run` / `Operation2D::run` signature mismatch.~~ Done 2026-08-31, and
  odyssey builds. The signature fix was small; what it exposed was not. Odyssey's
  `CMakeLists.txt` was missing four source files and eleven libraries, and the two source
  files it had never compiled needed the same kind of api-drift repair voxel still needs.
  Two small api changes went with it: `Scene2D::collect` is now virtual with a scene setter
  on `Engine2D`, which is how an app gets its own renderables into the frame, and `Context`
  and `Operation` gained virtual destructors.
- ~~Make tetris compile.~~ Done 2026-08-31, and it links. The `GLFontRenderer.h` include,
  the debug-text block and `fonts_` are gone; `GLTexture` had also grown a logger parameter
  that its seven call sites did not pass, and `LOG_ERROR` became the spdlog wrapper. The link
  list went from one library to thirteen plus their externals — and had to take `fmt::fmt`
  rather than `spdlog::spdlog`, because the api libraries compile spdlog header-only and the
  compiled target duplicates `spdlog::logger::log` on top of them. Tetris does not run: its
  config format is fixed (next item), but it draws through fixed-function GL against a
  context nothing creates.
- ~~Migrate `tetris/data/config.json` from the old inline `keys`/`menu` form to the
  `{"configs": [...]}` form that `Config::load` requires.~~ Done 2026-08-31, against pong's
  `data/` as the reference. The eleven `keys` entries are now `mappings.json` and the `Quit`
  menu is now `vgui.json`; every old entry was `"catch": "on"`, so each mapping source
  carries `"state": "pressed"`. No `window` or `sound` config is listed, because the old file
  had neither and `Window::create` falls back to its defaults without one. Two gaps this
  leaves: `Controller` never constructs a `v3d::ui::Engine`, so `vgui.json` is loaded as an
  asset and nothing handles the `ui::` commands or `toggleMenu`/`toggleFS` — that is the
  same wiring `PongEngine` already does, and it is now a Phase 4 item; and
  `tetris/CMakeLists.txt` does not copy `data/` to the build tree, so a run out of
  `out/build/` still reads the years-stale manual copy.

Done when: the whole tree builds, `vault/` is gone, and the luxa and v3dlibs audits have
produced a written list of what still has to move. Both audits are closed and both trees are
deleted, `vault/` with them, all on 2026-09-04. **Phase 1 is done.**

### Phase 2 — Vulkan to first pixel

Done, 2026-08-31. A window clears to a colour and survives a resize and a minimize.

- ~~Dynamic rendering against the swapchain image views.~~ Done. `VkPhysicalDeviceVulkan13Features`
  chained onto `VkPhysicalDeviceFeatures2` at device creation, asking for `dynamicRendering`
  and `synchronization2`; `pEnabledFeatures` is now null, since a feature struct in the chain
  and that field are mutually exclusive. Physical device selection rejects anything that does
  not offer both. No `VkRenderPass` and no `VkFramebuffer` exist anywhere in the renderer.
- ~~Command pool and per-frame command buffers.~~ Done, as `vulkan::CommandPool` and the
  buffers `vulkan::Presenter` allocates from it. Buffers are reset individually rather than
  by resetting the pool, because a frame in flight still owns its buffer while the next one
  is being recorded.
- ~~Sync.~~ Done. Two frames in flight, each with an image-available semaphore, a fence and a
  command buffer; the render-finished semaphore is **per swapchain image** rather than per
  frame, because presentation waits on it and presentation is tied to the image. Submission
  is `vkQueueSubmit2` and both layout transitions are `vkCmdPipelineBarrier2`.
- ~~`Engine3D::renderFrame()` acquires, records, submits, presents, and recreates the
  swapchain on out-of-date or suboptimal.~~ Done. A suboptimal frame is still drawn and
  presented and the chain is rebuilt before the next one. The fence is reset only once the
  frame is certain to be submitted, so an acquire that gives up leaves nothing waiting. A
  window with no area has no swapchain at all: `acquire` answers `Skip` and the engine
  rebuilds once the window has an area again, which is what makes minimizing survivable.
  `render::realtime::Engine::resize` is still the dead code it was - the out-of-date result
  is the trigger, and nothing needs the event.
- ~~Build the frame as a list of passes from the start.~~ Done. `Frame` is a list of `Pass`,
  each carrying its clear, depth flag, viewport region and queue of draw items. The engine
  builds one pass called `colour`; `Frame::addOperation` and `draw()` survive alongside it as
  the pre-vulkan path the unported apps still call, and go in phase 3.
- ~~The draw-item type and its sort key, including the 2D layer field.~~ Done. `SortKey` packs
  layer, pipeline, material and depth into one 64 bit integer, ordered coarsest first so that
  a sort groups exactly what can be merged. Nothing sorts yet - the recorder walks submission
  order - but the layer field is filled in from the first version, which was the point.
- ~~Resource handles: a pipeline cache, a material or descriptor registry, and texture
  handles.~~ Done. `Handle<Tag>` is a typed, comparable slot id; `Registry<Tag, Resource>`
  hands out stable slots; `vulkan::Resources` holds the pipeline, material and texture
  registries and destroys what it was given. `vulkan::PipelineCache` is a real
  `VkPipelineCache`, created before the first pipeline exists because a pipeline built
  outside the cache is not retroactively put into it. Nothing is registered yet - phase 3
  fills these when it builds the quad pipeline.
- ~~Bind by update frequency, and write the convention down.~~ Done, in
  [docs/RenderingPipeline.md](../RenderingPipeline.md): set 0 per frame bound by the pass,
  set 1 per material, push constants per object. The sort key's field order matches it.

Two things landed alongside, neither of them planned:

- **`sdl3` needed its `vulkan` feature.** See the state notes above. This is the reason the
  vulkan code had never run.
- **`api/render` has a test suite now** - 16 cases over the frame model, the sort key and the
  handle registry, which are the parts that need neither a window nor a GPU. Everything below
  the recorder still waits on [ADR-0007](../adr/0007-ci-rendering-tests.md).

Done when: a window clears to a colour and survives a resize and a minimize. It does.

### Phase 3 — pong as the pilot

Done, 2026-08-31. pong plays, drawn entirely through the batched quad, and its own sources
name no GL at all.

- ~~The batched quad primitive: a Vulkan replacement for `v3d::gl::Canvas` plus its
  operation, the 1x1 white texture, and flush-on-texture-change.~~ Done, and split across the
  cpu/gpu line: `realtime::Canvas` accumulates quads, cuts a batch where the texture changes,
  and owns the modelview stack and the projection - all of it vulkan-free and unit tested -
  while `vulkan::QuadRenderer` owns the one pipeline, the descriptor pool, the white texture
  and a vertex and index buffer per frame in flight. The infrastructure that had to land
  under it was most of the work: `vulkan::Buffer` (host visible, persistently mapped,
  grow-only), `vulkan::Memory`, `vulkan::TextureFactory` (staging upload on a one-shot
  buffer), and a `Recorder` that binds pipelines, descriptor sets and buffers and issues real
  draws instead of only calling an item's record callback.
- ~~Text: a Vulkan path for `operation::TextureFont` and `TextureFontCache`, which is the
  same primitive drawing from the glyph atlas.~~ Done, and it needed no new operation at all:
  the font library already lays glyphs out into positions, atlas coordinates and colours, so
  `Canvas::text` copies them into the same stream. The atlas is one channel and its image
  view swizzles that channel into alpha, so a glyph samples as white-with-coverage and the
  one quad shader serves text without a branch.
- ~~Re-wire pong's UI to `v3d::ui`; delete the commented-out Luxa block.~~ Done. The Luxa
  blocks are gone from `PongEngine`, and `v3d::ui::ComponentRenderer` is the rebuild the luxa
  audit called for - it draws a menu onto the canvas the game is already filling, so the ui
  costs no pass and no draw of its own. Text measuring and writing are handed in as callbacks
  rather than the ui library depending on the font one, which is also what makes its layout
  testable without a device.
- **Not done: deleting `api/gl` and dropping GLEW and OpenGL from every link list.** The
  phase's own bullet says "once nothing references them", and things still do - tetris and
  voxel both draw with `v3d::gl`, and they are phases 4 and 5. What did land: six operations
  nothing used any more are deleted (`operation::Canvas`, `GLFont`, `GLTexture`,
  `GLTexturedQuad`, `Overlay`, `BitmapFont`), leaving only `operation::TextureFont`, which
  voxel still constructs. And GL is no longer named by any app: `v3dlib_gl` links OpenGL and
  GLEW itself, and `v3dlib_asset` links `v3dlib_gl`, because its `Shader` and `ShaderProgram`
  asset types build a `v3d::gl::Program`. So pong's link line has no GL in it, and the two
  lines that remain are the ones that go with the tree.

One decision and several defects turned up on the way:

- **The swapchain was the wrong colour space**, and phase 2 could not have noticed because it
  only ever cleared. An `_SRGB` target encodes on write, so every colour in the engine was
  displayed brighter than it was written - the `(0.06, 0.07, 0.10)` clear came out mid slate
  grey. Now a `UNORM` format, recorded as
  [ADR-0009](../adr/0009-colour-authored-in-display-space.md), which is the decision voxel
  will have to revisit.
- **Validation was loaded and silent, which is indistinguishable from validation that is not
  loaded.** Nothing created a debug messenger, so nothing the layer said reached anybody.
  `vulkan::Instance` now enables the Khronos layer when it is installed and routes its
  warnings and errors through the logger. Phase 2's "validation layer loaded and silent"
  should be read as unverified.
- **No app had a way to quit.** `Engine::eventLoop` handled `SDL_EVENT_QUIT`, which SDL only
  sends once the last window is destroyed, and nothing destroyed it - so the close button did
  nothing. It now handles `SDL_EVENT_WINDOW_CLOSE_REQUESTED`.
- **The menu was toggled on the wrong object.** pong read and wrote visibility on the menu
  component, which is visible from the moment it is built; it is the container the config
  starts hidden. So the first press of escape thought the menu was already open and hid it.
- **Glyph quads were the wrong height.** `TextureTextBuffer` took a glyph's top as its
  bearing above its bottom rather than its height, which is only the same when a bitmap is
  exactly as tall as its bearing - so descenders were drawn short. It had never shown,
  because nothing had ever drawn text.
- **`TextBuffer::dirty_` was never initialized**, and `clear()` did not reset it.

Also landed: the log flushes from info up, so a killed process leaves a log worth reading;
pong's `main` catches and prints an exception rather than dying in a message-less abort
dialog; and `api/ui` has a test suite - six cases over menu layout, submenu descent and
visibility - alongside fourteen new ones for the canvas.

Verified by running pong: it draws the board, the paddles, the ball and the scoreboard, opens
its menu over the game with the active item highlighted, and the Khronos validation layer -
now actually reporting - says nothing across the run.

Done when: pong plays. It does. The GL deletion moves to phase 5, behind the two apps that
still need it.

### Phase 4 — tetris

Done, 2026-08-31. Tetris plays: the well fills, rows clear, the preview and the score are
drawn, and the menu opens over a paused board.

- ~~Pack the seven piece textures into one `v3d::image::TextureAtlas` at load time, so the
  board draws as a single batch instead of flushing per block.~~ Done. A 256x256 RGB atlas,
  uploaded once; the uv rect for each colour is inset half a texel so filtering a 64x64
  block down to a ~28 pixel cell cannot reach into its neighbour across the packer's border.
- ~~Replace the fixed-function draw path wholesale.~~ Done. `TetrisRenderer` is a rewrite
  against `realtime::Canvas` and names no GL at all - the well, every block, the preview,
  the text and the menu are all the one quad, submitted as one canvas per frame.
- ~~Rewrite the debug text against `TextureFont`.~~ Done, the same way pong loads its font.
  F2 shows the falling tetrad's position and extent.
- ~~Wire `Controller` to `v3d::ui`.~~ Done. It constructs the engine, loads the `ui` config,
  and handles `toggleMenu` plus the `ui::` commands. `vgui.json` gained a New Game item, so
  the menu does something beyond quitting.
- ~~Load piece textures through `asset::Manager` instead of `image::Factory` with hardcoded
  relative paths.~~ Done, which needed an `asset::Type::ImageTga` and a `loader::Tga`
  alongside the png and jpeg ones. `pieces/shapes.txt` was read the same way - a cwd
  relative `ifstream` - and now loads as an `asset::Text`.

Five things turned up that the render port could not have run without, and one that only
this app's data could have exposed:

- **The TGA reader handed its rows on in file order**, so every image it read came out
  upside down. The origin bit in the image descriptor is clear far more often than it is
  set, which means bottom-up rows, and every other reader in `api/image` - and every
  consumer of one - is top-down. It had never shown because the only fixture testing it is
  a uniformly coloured 2x2. The blocks are bevelled, so on the atlas it showed immediately.
  Fixed, with a red-over-green fixture that can tell the difference.
- **Nothing copied an app's own `data/` into the build tree.** Tetris had no data there at
  all, so it could not have started. `v3d_add_app_data` in the root CMakeLists now copies
  `<app>/data` beside the executable the way `v3d_add_shared_data` copies the root one.
  Only tetris uses it; pong still runs from a years-stale manual copy.
- **`Controller::initialize` fell off the end without returning**, `tick` never ticked the
  scene, and `render` drew nothing - so even with a renderer, nothing would have moved.
- **`Tetrad::operator=` did not copy the position**, which the copy constructor beside it
  does. Nothing had noticed because the one assignment in the tree set the position
  immediately afterwards.
- **`Tetrad::rotate` turned the shape one way and did nothing at all the other way** - the
  counter-clockwise branch copied the layout unchanged - and `width()`/`height()` seeded
  their minimum at 0, so both were really `max + 1`. All three are fixed; rotation now
  maintains `orientation_` itself rather than leaving the caller to.
- **The board wrote a landed tetrad into `pieces_` without bounds checking**, from a
  collision test built out of per-axis offset arithmetic that the controller partly
  duplicated. That is one `GameBoard::fits(tetrad, column, row)` now, which the fall, the
  sideways moves and the rotation all go through, so there is one description of what a
  legal position is. Rotation against a wall tries a one and two cell kick either side
  before giving up. Row clearing scans bottom up and re-examines a cleared row rather than
  stepping past it, which the old loop did not; the score it feeds was a field nobody set
  and nobody read.

`tetris/tests/` builds the first per-app suite, ten cases over the board and the tetrad -
`fits` against the walls, the floor and a block; a row that clears and one that does not;
game over; shape normalisation on load; the four rotation states, the two directions being
each other's inverse, assignment carrying the position, and the extents. None of it needs a
window or a device, and the tetrad cases are written directly against the three defects
listed above - the dead counter-clockwise branch, the assignment that dropped the position,
and the extents that were really `max + 1`. Reading a shape set and installing one are now
separate `GameBoard::load` overloads so a test can supply its own shapes rather than a file,
and `piece()` gained the setter that matches its getter, so a board can be arranged.

Still not done: `tetris/run-unit-tests.sh` invokes a `unit_tests` binary that does not
exist, and the renderer has no coverage - that waits on
[ADR 0007](../adr/0007-ci-rendering-tests.md) like the rest of the device-side work.

### Phase 5 — voxel and odyssey, and the engine consolidation

Voxel needs a 3D scene path — depth, shaders, meshes, a shared camera — that pong and tetris
never exercised, so it is the app that drives real api growth. Surveyed on 2026-08-31 in
[docs/VoxelSurvey.md](../VoxelSurvey.md); read it before starting, particularly for the
defect list, which is not repeated here.

The order below is the survey's: make it start, grow the api, port the renderer, then delete
`api/gl`. The first group is half an hour of work and unblocks every observation after it —
until voxel runs, every claim about it is a claim about a binary nobody has executed.

**Make it start.** Done, 2026-08-31, and it goes exactly as far as the survey predicted it
would: config, window, Vulkan instance, device and swapchain all come up, and the process
then dies in `Renderer`'s constructor at the `glGetString` call. Nothing short of the render
port gets past that — every GL call after it is against a context nothing creates.

- ~~Call `v3d_add_app_data(voxel)`.~~ Done. `voxel/data/` now reaches the build tree, which
  it never had.
- ~~Migrate `voxel/data/config.json` to the indirect form, splitting the eight `keys` entries
  into a `mappings.json`.~~ Done, against pong's `data/` as the reference, plus the
  `window.json` the old format had no place for. Seven of the eight entries carried over.
  The eighth was `mouse::motion` → `look`, and it is dropped rather than translated: motion
  is not a bindable source event — `input::Mouse` dispatches a `MouseMotion` and returns
  before naming one — and `Controller::handleMotion` is already connected to that signal
  directly, so the binding never did anything. The slot went to `f3` → `debug`, which
  `Controller` has always handled and nothing had ever bound.
- ~~Make `Config::load` survive a config it does not understand.~~ Done. Every lookup is
  guarded by a `contains()` now, so the old inline format comes back as a logged `false`
  like every other rejection rather than as an exception out of engine startup.
- ~~Drop the dead links from `voxel/CMakeLists.txt`: `v3dlib_audio` and `soloud` ... and
  `OpenGL::GL` and `GLEW::GLEW`.~~ Partly done, and **the audio half of this item was
  wrong**. The GL pair really is redundant and is gone from both voxel and odyssey. The
  audio pair is not: `v3dlib_asset`'s Wav loader calls `v3d::audio::AudioClip::load`, so
  dropping it fails the link outright. Fixed at the layer instead, the way `v3dlib_gl`
  already was — `v3dlib_asset` links `v3dlib_audio` PUBLIC and `v3dlib_audio` links `soloud`
  PUBLIC, so no app names either unless it plays a sound itself. Odyssey's explicit pair is
  gone with its GL one; pong keeps `v3dlib_audio` because it does play sounds.
- ~~Fix the six defects in the survey.~~ Done, all six. The two that change what the app
  draws: `Scene` now passes `worldHeight * chunkSize` as the chunk ceiling, so terrain
  scales into 64 blocks rather than 4; and `MeshBuilder`'s `RIGHT`, `FRONT` and `TOP`
  cross-chunk checks move the block to the near edge of the neighbour instead of leaving it
  at 15, with the three that were right no longer hardcoding that 15. The other four:
  `MeshCache::createFace` builds a quad from four vertices indexed six times rather than six
  vertices indexed in order, so the index buffer carries information and
  `VertexBufferBuilder` writes four per-vertex info entries per face instead of six; the two
  triangles `VertexBufferBuilder` read and never used are gone; `GameState` initialises its
  three fields; and `Chunk` and `MeshCache` list their initialisers in declaration order.

**Grow the api.** Done, 2026-08-31 — all six items. These were the phase's real content, and
the first three are what phase 6's multiple viewports need as well. Nothing here has a 3D
consumer yet: the parts are built, verified against the validation layer through pong and
tetris, and waiting for the port below to use them for what they were built for.

- ~~**A depth buffer.**~~ Done, as `vulkan::DepthBuffer`, owned by `Context3D` and rebuilt
  with the swapchain. It is **allocated lazily** — the first frame a pass asks for depth, and
  never otherwise, so pong and tetris do not pay a full-screen image for something painter
  ordering does not read. `Recorder` attaches it when the pass asks, clears it exactly when
  the pass clears colour, and transitions it from `UNDEFINED` once per frame. The one thing
  the item did not anticipate: dynamic rendering matches a pipeline to its pass's
  attachments, so a pipeline built without a depth format cannot draw into a pass that has
  one — `QuadRenderer` therefore compiles its pipeline twice and picks between them from
  `Pass::depth()`.
- ~~**A pipeline builder.**~~ Done, as `vulkan::PipelineBuilder`. `QuadRenderer`'s ~150 lines
  of inline create-info are now fifteen chained calls, which is what proves the builder
  rather than leaving it to the first app that tries.
- ~~**Set 0.**~~ Done, as `vulkan::FrameUniforms` — the layout every pipeline in the engine
  declares, plus a slot per pass per frame in flight holding view, projection, their product
  and the viewport. `Recorder` writes and binds it per pass.
  [ADR-0008](../adr/0008-binding-by-update-frequency.md) stays **proposed**: the set is built
  and bound, and no shader has read it yet, which is the check that record asked for.
- ~~**Device-local buffers with a staging upload.**~~ Done, as `vulkan::DeviceBuffer` over a
  shared `vulkan::Uploader` — the one-shot record/submit/wait that `TextureFactory` had
  privately and now uses from the same place. `vulkan::Mesh` is the pair of them a draw
  reads.
- ~~**Decide whether geometry is a `Resources` handle.**~~ Decided: **no**, and recorded as
  [ADR-0010](../adr/0010-meshes-are-owned-by-the-app.md). `Resources` never frees an
  individual resource, and a chunk mesh dies while the app runs; the sort key has no geometry
  field, so a handle would sort nothing. The app owns meshes, and `vulkan::Mesh` is the api
  type it owns them as.
- ~~**Sort the frame.**~~ Done, and **opt-in per pass** — `Pass::sort(true)`. It has to be:
  the key groups by pipeline and material within a layer, so sorting a canvas of batches
  would put a panel over the text drawn on it. `Pass::ordered()` is what the recorder walks,
  which puts the decision where it can be unit tested without a device.

**Port voxel.** Done, 2026-09-01, and voxel draws terrain. It is the first app in the tree
with a depth tested, sorted scene of its own pipeline, and the first thing to read set 0 —
which is what [ADR-0008](../adr/0008-binding-by-update-frequency.md) was waiting for.

- ~~Rewrite the two shaders it loads for Vulkan GLSL — explicit `set` and `binding`, matrices
  in set 0, the 16-material table in a UBO — and compile them with `v3d_add_shader`.~~ Done,
  as `voxel/shaders/voxel.{vert,frag}`, and the twelve GLSL files in `voxel/data/shaders/`
  are gone with them. Shading is per vertex rather than per fragment: a block face is flat,
  so its normal and its material are constant across the quad and the four corners carry
  everything the interpolator needs. `engine/SceneUniforms.h` is the C++ mirror of the set 1
  block, written as vec4s throughout because that is what std140 rounds a vec3 up to anyway.
- ~~Replace `ChunkBufferPool` and `VertexBufferBuilder` with device-local mesh buffers and one
  `DrawItem` per chunk.~~ Done, as `voxel/ChunkMeshPool` over `engine/ChunkMeshBuilder`, and
  **the vertices did move into chunk-local space** — the chunk's corner goes in a push
  constant, so a chunk can be culled or moved without rebuilding its mesh. The mesh cache
  subtracts the origin as it extracts, which is one line and is unit tested.
- ~~Rewrite `DebugOverlay` against `realtime::Canvas` and the font library.~~ Done, and it
  shrank to what it always was: a rolling frame time and the player position, as lines of
  text. Drawing them is the renderer's, which already owns a font for the ui.
  `operation::TextureFont` has no consumers left.
- ~~Wire `Controller` to `v3d::ui`.~~ Done. `data/vgui.json` is a game menu of Resume and
  Quit, escape shows and hides it, and the menu pauses the world and gives the pointer back —
  mouselook warps the cursor to the centre every frame, which a menu cannot be used through.
  `GameState::pause` was dead until now. Both items work: `Menu::activate()` dispatches an
  action item's bound event, which [LuxaAudit.md](../LuxaAudit.md) records as fixed on
  2026-08-31 — CLAUDE.md still listed it as an open regression and no longer does.
- ~~Add `voxel/tests/`.~~ Done — 17 cases over the chunk, the mesh cache, the Morton code and
  the seam culling. The seam arithmetic moved out of `MeshBuilder` into `voxel/FaceCulling`
  so that it could be tested without a device, which is what carries the regression test for
  the three cross-chunk checks that used to look at the wrong block. `TerrainMap::height` is
  virtual now, so a chunk can be built against a flat map rather than against perlin noise.

Four things the port turned up that the survey did not:

- **A quit command must not call `shutdown()`.** Voxel is the first app whose menu anyone has
  driven to a Quit item, and doing so threw. `Engine::eventLoop` ticks and renders after an
  event handler returns, so a handler that destroys the window and calls `SDL_Quit` leaves
  the frame after it acquiring a swapchain image from a lost surface — `Presenter::acquire`
  throws on it, or the process spins on with no window, depending on what the driver hands
  back. `v3d::engine::Engine::quit()` is the fix: a flag the loop breaks on, with `main`
  shutting down once outside it. **Pong and tetris have the same defect** and still call
  `shutdown()` from their handlers.

- **The camera's projection was gl's.** `Camera::perspective` built a matrix mapping z to
  [-1, 1] with y up. Vulkan clips against [0, 1] with y down, so it now negates the y scale
  and maps near to zero — and, because flipping y reverses the winding a front face presents,
  the terrain pipeline calls its front faces clockwise. Nothing else in the tree had noticed:
  pong and tetris are orthographic and build their projection by hand in `Canvas`.
- **The spawn point is inside the hills.** The player has always started at `(0, 25, 100)`,
  which was above a world whose terrain reached four blocks. With the chunk ceiling fixed the
  terrain reaches sixty-four, and the old spawn is buried in it — the app draws a screen of
  sky and the odd sliver of a face seen from inside. Moved to `(128, 80, 240)`, above the
  terrain and looking across it. There is no collision or gravity — `checkWorldCollision`
  returns false and always has — so standing on the ground is not on offer yet.
- **Nothing culls, and the frame costs about 58 ms in a debug build.** One draw item per
  meshed chunk, all of them submitted every frame whether or not they are in front of the
  camera. The chunk-local vertices are what a frustum cull needs and it is now a small piece
  of work, but it is not this phase's.

Odyssey is where the consolidation actually lands. It builds, and it runs on `SDL_Renderer`
rather than on GL, so it was never blocked by Vulkan.

**Make it start.** Done, 2026-09-01, and the two items the note predicted were the first two
of six. Odyssey now opens a window and draws, which nobody had seen it do.

- ~~Call `v3d_add_app_data(odyssey)` and fix the config type name to `"binding"`.~~ Done, and
  four more failures sat behind them, each only reachable once the one in front was fixed:
  - `odyssey/data/bindings.json` was in a third binding format — `contexts` holding an
    event-string-to-command map — that nothing has read since the `mappings` shape landed.
    `registerEventMappings` reaches for `doc.at("mappings")`, which throws. Migrated to
    `mappings.json` against pong's as the reference. The `player::movement::*` destinations
    became `odyssey::move*`, since a destination is a context and a name rather than a path,
    and `ui::navigation::up` is dropped: odyssey has no ui. An `escape` → `odyssey::quit`
    binding replaces it, handled through `Engine::quit()` rather than `shutdown()`.
  - `odyssey/data/window.json` carried only width and height. `Engine::initialize` reads
    `logicalWidth` and `logicalHeight` with `at()`, which throws on a config that omits them —
    the one lookup in engine startup that `Config::load`'s `contains()` guards do not cover.
    Set to 1280x768, which is what `unit::tile_* * unit::screen_tile_*` comes to and what the
    app overrides the window's logical size with anyway.
  - **`Window2D::create` took the window surface, and that alone made `Engine2D` impossible to
    build.** `SDL_GetWindowSurface` associates a surface with the window for as long as it
    lives, and SDL then refuses `SDL_CreateRenderer` on it — `Context2D`'s constructor threw
    "Surface already associated with window" out of `Engine2D::initialize`, which reached
    `main` as an `abort()`. The surface is only ever read by `paint()`, which has no callers,
    so it is acquired there lazily instead. Nothing else in the tree had noticed: pong and
    tetris are both on `Window3D` since their ports, and odyssey is the only `Window2D`
    consumer there has ever been.
  - `sample.png`, the one asset odyssey draws, is not in the repository and never has been. A
    placeholder sprite is committed as `odyssey/data/sample.png`. The missing file was not
    reported: `image::reader::Png` returns an empty image, and `asset::loader::Png` wrapped
    that in an `asset::Image` that looks loaded until a consumer dereferences it — `Surface`
    did, and asserted. All three image loaders now log the path and return no asset at all
    when the read fails, which is the convention `loader::Json` already had.

**What odyssey does**, now that it has been seen: it opens a 1280x768 window, clears to black,
and blits one 64x64 sprite at the origin. That is all. `Movement::tick` returns true and does
nothing; the `PositionFixed2D` component `engine::Player` puts on its entity is never read, so
the sprite is not drawn from it and cannot move. `Sprite`, `SpriteSheet`, `Actor`, `Tile` and
`ui::Screen` are empty declarations. The port below is therefore not a port of a game — it is
a port of one textured quad, and everything the app would need to be a game is still unwritten.

**Port odyssey, and the consolidation with it.** Done, 2026-09-01, except for the two items
struck through below that turn out not to be this phase's after all.

- ~~Replace its `Blit2DTexture` usage with the batched textured quad.~~ Done, as
  `odyssey/render/Renderer`, which replaces `Scene`, `Renderable` and `renderable::Player` at
  about half their combined size. The sprite is drawn at the tile its entity's
  `PositionFixed2D` names rather than at a hardcoded origin, which is the first time anything
  has read that component — the old renderable held a `boost::shared_ptr<engine::Player>` and
  ignored it. `engine::Player::entity()` is what the renderer looks it up by.
- ~~Move it from `Feature::Window2D` to the single window and engine.~~ Done.
- ~~Delete `api/render/realtime/2D/` and `operation/2D/`, collapse `Window2D`/`Window3D` into
  one `Window`, and drop the `Window2D`/`Window3D` feature flags for a single windowing
  flag.~~ Done, and more went with them than the item names:
  - `realtime::Surface`, the SDL surface wrapper, whose only consumer was `Texture2D`.
  - `Frame::addOperation`, `Frame::draw` and `realtime::Operation` — the pre-Vulkan
    submission path, which the frame's own comment said was kept "only until the apps still
    calling it are ported". Odyssey was the last.
  - `realtime::Engine::resize`, which called `window_->resize()` a second time after
    `engine::Engine::eventLoop` had already done it. Odyssey held the only connection to it.
  - `operation::TextureFont`, which derived from `Operation` and drew a GL font.
  - The window config's `logicalWidth` and `logicalHeight`, which were `Window2D`'s logical
    presentation and named nothing once it was gone. All four apps' `window.json` lost them.
    While the window creation was being rewritten it also stopped being conditional on
    `Feature::Config`: an app with no window config now gets a window at the default size
    rather than a window that is constructed and never created.
  - The commented-out GL setup in `Window3D::create`. It was kept as reference for what the
    Vulkan path had to replace, and the Vulkan path has replaced all of it — depth test and
    range in `DepthBuffer` and the pipeline, culling and winding in `PipelineBuilder`, the
    clear in `Recorder`, vsync in `Presenter`.
- ~~**Delete `api/gl`**, carried over from phase 3.~~ Done, along with the root
  `find_package(OpenGL)` and `find_package(GLEW)` calls and the `glew` port in `vcpkg.json`.
  Both holders turned out smaller than the item expected:
  - `api/ui/style/property/Image` and `api/ui/component/Icon` hold a
    `render::realtime::TextureHandle` now. This cost `v3dlib_ui` no new dependency — it
    already links `v3dlib_render`, because `ComponentRenderer` draws onto a `Canvas`. Neither
    class ever called a method on the GL texture; both used it as an opaque handle, which is
    exactly what `TextureHandle` is. **Nothing sets either handle yet**, so this is the type
    half of [LuxaAudit.md](../LuxaAudit.md) item 5 and not the image-loading half.
  - The `Shader` and `ShaderProgram` asset types, their two loaders, their three
    `asset::Type` values and their `Manager` registrations are **deleted rather than ported**.
    Nothing has loaded a shader asset since `v3d_add_shader` started compiling GLSL at build
    time and embedding the SPIR-V; they existed only to build a `v3d::gl::Program` nobody
    asked for.
- ~~Add the sprite/orthographic pass properly, so a 2D game gets painter ordering and no depth
  buffer without special-casing the engine.~~ **Half of this was already true, and the other
  half is not this item.** A pass is painter ordered unless it asks for `sort(true)`, and the
  depth buffer is allocated the first frame a pass asks for depth, so a 2D game already pays
  for neither — odyssey's renderer says nothing about either and gets both right. What is
  still a special case is the projection: `Canvas::projection()` builds an orthographic matrix
  by hand and the quad pipeline reads it from a push constant, while `FrameUniforms` holds a
  camera per pass that only voxel's terrain reads. Giving `Pass` an orthographic camera and
  having the quad pipeline read set 0 like everything else is a change to the shared pipeline
  all four apps draw through, and it belongs with phase 6's multiple viewports — the first
  thing that actually needs a per-pass camera on a 2D pass.
- **Revisit [ADR-0009](../adr/0009-colour-authored-in-display-space.md).** Not yet due. The
  record itself says the reckoning comes when a lit scene has to blend in linear space, and
  nothing in this phase added lighting. Carried to whichever phase does.

**What the port turned up that the plan did not.** Both are library defects that no app had
been in a position to notice:

- **`image::reader::Png` handed back an upside down picture**, and `image::writer::Png`
  reversed its rows to match, so a round trip cancelled out and the writer's round-trip test
  passed. Only a *displayed* png saw it, and nothing displayed one — tetris's textures are
  tga, and the pre-Vulkan 2D path presented its whole back buffer with an
  `SDL_FLIP_VERTICAL`, which compensated for every image whether or not it needed it. Both
  leave the rows alone now. `writer::Tga` had the mirror of the bug: it wrote top down
  without setting bit 5 of the descriptor, so its own reader believed the file was bottom up
  and turned it over. `imagewriter_orientation_test` pins both, with an image whose rows
  differ — the existing round-trip test uses a uniform blue square, which cannot see a flip.
  The jpeg reader and writer were left alone at the time - self consistent, and nothing
  displays a jpeg - and were fixed together on 2026-09-04.
- **The three image loaders wrapped a failed read in an asset that looks loaded.**
  `asset::loader::Png` built an `asset::Image` around the empty image the reader returns for
  a missing file, so the failure surfaced only where something dereferenced it — for odyssey,
  inside `realtime::Surface`, as an assert. All three now log the path and return no asset at
  all, which is what `loader::Json` already did.

### Phase 6 — the Vertical3D editor

The app the repository is named after, and the largest piece of work here. It is a desktop
3D editing tool, which means it needs things no game in this repo does: multiple viewports,
manipulator gizmos, a construction plane, selection, and an undoable command model.

- ~~Survey `rigel/` and decide what to fold in.~~ Done 2026-09-01, written up in
  [docs/RigelSurvey.md](../RigelSurvey.md). **Nothing in rigel can be ported.** It is a
  gtkmm 2 / gtkglextmm / libxml++ application with no build files, whose own support
  libraries were moved out from under it in 2022; every line of drawing is immediate-mode GL
  and every line of windowing is GTK. What comes across is behaviour, a handful of
  algorithms, and `rigel/docs/xml/gui.xml` — the editor's whole menu, toolbar, keybinding,
  camera-profile and viewport-layout definition, which is a data asset to translate into the
  JSON config form. `RenderView` turns out to be an empty `Gtk::DrawingArea` and is worth
  nothing. The survey's eleven-item delete list replaces this bullet.
- ~~**The api has no line primitive, and that is this phase's largest gap.**~~ Landed
  2026-09-01 as [ADR-0011](../adr/0011-lines-are-the-second-primitive.md).
  `realtime::LineCanvas` accumulates segments on the cpu - `line`, `polyline`, `box`,
  `circle` and a transform stack - and `vulkan::LineRenderer` draws the whole of one as a
  single non-indexed line list. Positions are in world space and read the pass camera at set
  0, which makes it the first thing in `api/render` to do so and the worked example for
  moving the quad pipeline onto the same footing. The pipeline built for a pass with a depth
  attachment tests and writes depth, so a wireframe is occluded and an overlay is a pass
  without depth rather than a flag. Lines are one pixel wide - `wideLines` is an optional
  device feature and the device does not ask for it.
  - The renderer is built on the first call to `Context3D::lines()` rather than at startup,
    the way the depth buffer is, so a 2D game pays neither the two pipeline compiles nor a
    vertex buffer per frame in flight.
  - ~~**The device half has not been run.**~~ Run on 2026-09-01, by the editor. Both
    pipelines draw, the depth one is what a viewport uses, and the validation layer is
    silent across a run with four line canvases a frame, four programmatic resizes, a
    minimize and a restore. What the first consumer found is below.
  - A construction grid is deliberately not in the api. `LineCanvas` offers primitives; the
    grid's extent, spacing, major intervals and orientation are the editor's policy, and
    rigel's `ConstructionPlane` is the behaviour to fold in there.
- ~~**`v3d::type::CameraProfile` is write-only**, which blocks both the `gui.xml`
  translation and the camera control that has already been ported.~~ Done 2026-09-01. Every
  field has a getter and a setter now, `size()` among them, so a profile table can be loaded
  into one; adaptive projection and position are back as their own option bits, and
  `OPTION_DEFAULT` — which had no reader, no accessor and sat on rigel's adaptive-projection
  bit — went to make room. The five-argument constructor `vertical3d/Controller` already
  calls exists. `orthoFactor()` is gone, replaced by `orthoFactorHorizontal()` and
  `orthoFactorVertical()`: the pixel aspect ratio belongs to width alone, and the single
  factor was being used for both axes. Both return zero rather than dividing by an unset
  viewport size. Three things in `vertical3d/` went with it, none of them built yet:
  - `ViewPort::resize` writes the camera's viewport size and pixel aspect, which is what
    makes the ortho factors non-zero, and `ViewPort::VisibleFilter` is `(1 << n)` per value
    instead of the sequential values it copied from rigel.
  - `CameraControlTool::buttonPressed` clicks the arcball and a new `resize()` gives it its
    bounds — the two things the port dropped. Vertical truck uses the vertical factor.
  - `CameraControlTool::pan` drags the arcball to the point the gesture has reached rather
    than the one it came from; `motion()` records `last_` only after `pan()` returns, so
    every rotation was one event stale and the first after a click was identity.
- ~~**`v3d::type::Camera` builds OpenGL clip space**, which nothing can draw with.~~
  Fixed 2026-09-01, recorded as
  [ADR-0012](../adr/0012-camera-builds-vulkan-clip-space.md). This was not on the list
  because nothing had ever drawn through the class: its `createProjection()` was written
  against `glFrustum` and `glOrtho`, so y pointed up and depth ran -1 to 1, and under Vulkan
  a scene comes out mirrored with the near half of the frustum clipped away. Voxel had
  already answered this privately, with a second camera class of its own. Three further
  defects in the same three functions went with it, none of them observable before:
  - **The projection looked down -z while the profile's basis pointed the view along +z.**
    The default profile sits at `(0, 0, -1)` with `direction` `(0, 0, 1)`, so the origin was
    *behind* the camera. Both are now +z, which is what the profile documents and what every
    profile in `gui.xml` assumes.
  - **`CameraProfile::lookat()` stored the transpose of the rotation it meant**, so a camera
    told to look at a point looked away from it under any non-identity orientation - which
    is every view but Front.
  - **`Camera::project()` never divided by w**, and `project()` and `unproject()` measured y
    in opposite directions, so only points on the horizontal centre line round tripped. The
    test suite had recorded that as expected behaviour. Picking needs the round trip.
- ~~**A frame could only ever submit one canvas.**~~ Fixed 2026-09-01. `QuadRenderer` and
  `LineRenderer` both wrote every submission into one buffer per frame in flight, from
  offset zero - so a second `submit()` in a frame overwrote the first and left its draw item
  pointing at the wrong geometry. Four games never noticed because each submits exactly
  once; four viewports submit four times. Each submission now takes a buffer of its own out
  of the frame's ring, which `Engine3D` returns after recording. Appending into one buffer
  would not do: growing a buffer replaces the allocation and invalidates the handle every
  draw item recorded before it is holding.
- ~~Selection needed three things the api did not have.~~ **All three landed 2026-09-02.**
  The first two are [ADR-0013](../adr/0013-mesh-is-a-dag-node.md): `brep::BRep` derives from
  `dag::Node` and `dag::Transform`, so a mesh has an id and a placement, and `selected()` is
  on all four of `Vertex`, `HalfEdge`, `Face` and `BRep` — `Face`'s had been commented out
  rather than kept, so it was four of four missing rather than three. `dag::Transform` had to
  be made to compile first: it named members its own header does not declare, called three
  glm methods that do not exist, and was left out of its `CMakeLists.txt`, so nothing had
  ever built it. It now composes translation * rotation * scale, and `translation(v)` sets
  where it used to accumulate.
  - ~~**Picking is the third**~~ — landed as
    [ADR-0014](../adr/0014-picking-is-a-cpu-ray-cast.md): a cpu ray cast against the brep,
    not an id-buffer pass. An id buffer would start by writing the triangle primitive the
    tree does not have, needs a readback that either stalls the frame or answers a click a
    frame late, and cannot answer for a one pixel line at all. So an object and a face —
    which have area — are hit by the ray meeting a triangle of a fan over the face's loop,
    and a vertex and an edge by screen space proximity, nearest to the camera winning as
    rigel's depth sorted hit buffer did. `v3d::type::Ray` and `Camera::ray()` are the api
    half; `v3d::editor::Picker` is the policy. The ray is moved into each mesh's space by the
    inverse of its matrix rather than the geometry into the world, and
    `Ray::transformed()` does not renormalise, so a distance is comparable across meshes of
    different scales. Rigel's integer name-space encoding is not ported: a `Hit` is a struct
    and can say what kind of thing it holds.
  - ~~What decides which of vertex, edge and face a click writes is a select mask, which does
    not exist yet.~~ `v3d::editor::SelectMask` is object, vertex, edge or face, held by
    `SelectTool` and bound to o, v, e and f because gui.xml puts the masks on a menu and a
    toolbar and there are neither yet. Changing it clears the component selection, which is
    what keeps one kind selected at a time. The curve, mesh, light, camera and handle masks
    gui.xml also lists have no node type to select and are left out rather than stubbed.
  - `SelectTool` is the second `Tool` and shares the primary button with the camera one: a
    drag with a modifier held drives a camera and a bare click picks, which is how rigel
    divided them. Rigel's rules come with it — an object has to be selected before any of
    its components may be, a miss in object mode deselects everything and a miss in a
    component mode clears only the components, and clicking the same component twice
    deselects it. Verified against a run: a cube selects, deselects, and toggles one of its
    faces, with the validation layer silent.
  - Still open: **multiple selection**. One thing is selected at a time, which is rigel's
    limit too, so a rubber band or a shift-click is a change to the selection model rather
    than to the tool.
- ~~Rewrite `vertical3d/` onto the current api.~~ Done 2026-09-01. **The editor opens, and
  draws four viewports of one scene.** It runs on `v3d::engine::Engine` the way every other
  app does - `Feature::Config | Window | MouseInput | KeyboardInput` - with `src/`, `data/`
  and `tests/` directories like its neighbours, and it is in the root `add_subdirectory`
  list again.
  - `gui.xml`'s two data halves are translated: `data/cameras.json` is the eight camera
    profiles and `data/layout.json` is the quad viewport tree. `api/config` gained a
    `camera` and a `layout` type so both load through `Config` like every other config file.
    Each profile is given as eye, lookat and up rather than as three normals, because the
    normals and the rotation have to agree and `lookat()` is the one call that writes all
    four; and the near plane is 0.1 rather than the 0.001 in `gui.xml`, which spends the
    whole depth range on the first thousandth of the scene.
  - `ViewLayout` flattens the nested `<viewgroup>` tree into one pixel region per viewport,
    which is what rigel built a tree of `Gtk::Paned` from. The panes are not draggable yet;
    that is the only behaviour the flattening drops.
  - `ConstructionPlane` is folded in from rigel and draws through `LineCanvas`. Rigel drew
    its origin lines thicker with `glLineWidth`; lines are one pixel wide, so colour carries
    the emphasis. It is the editor's rather than the api's, per ADR-0011.
  - `CameraControlTool` drives whichever view the cursor is over, so a four way split is
    four cameras and one tool. The modifier held picks the move, as `view::camera::*` did.
  - `HWRenderContext` and `Visitor` are deleted. The first is the GL render context and does
    not survive [ADR-0001](../adr/0001-vulkan-replaces-opengl.md); the second was an empty
    class in the old `v3D` namespace with no members and no consumer.
  - **There is a scene as of 2026-09-02.** `v3d::editor::Scene`, `SceneVisitor` and the four
    `create_poly_*` primitives moved out of `v3dlibs/core` into `vertical3d/src`, which
    emptied that directory and deleted `v3dlib_core` with it. `WireframeVisitor` turns a
    scene into a `LineCanvas` - a segment per edge, with a half edge and its pair drawn once
    between them, through the mesh's own transform - and every view that shows meshes draws
    the same scene through its own camera. The create commands arrive in a `create` context
    bound to keys 1 to 4, because `gui.xml` puts them on menus and there are no menus yet.
    Verified against a run: cube and cylinder, four viewports, validation silent.
  - **Selection landed 2026-09-02**, per ADR-0014 above: a click picks, the wireframe shows
    what is selected - a selected object recolours, a selected face draws its boundary in the
    component colour and a selected vertex draws a small box, there being no filled primitive
    to shade either with - and the mask keys switch what a click looks for.
  - **The manipulators landed 2026-09-02**, per ADR-0015 above: q, w, e and r choose between
    none, translate, rotate and scale, a handle takes a bare press if the cursor is on one
    and a press no handle took is what picks, and the handles are drawn in an overlay pass
    per viewport. The select masks moved off o, v, e and f to the digits 5 to 8, because
    gui.xml binds e to the rotate tool.
  - **Undo landed 2026-09-02**, per ADR-0016 below: z and y step the history, a create and a
    whole transform gesture are each one step, and `CommandStack` is the editor's.
  - **The command directory landed 2026-09-02**, per ADR-0017 below: a command is its
    context and name together, `CommandDirectory` maps that to a handler, and
    `Controller::handleEvent` is a lookup rather than the chain of context and name
    comparisons it had been. `data/mappings.json` now names gui.xml's own commands.
  - **A project saves and opens as of 2026-09-02**, per ADR-0018 below: `project::load` and
    `project::save` read and write one JSON document, and `v3d::editor::Project` is the
    reader and the writer.
  - **The menus landed 2026-09-02**, per ADR-0019 below: `data/vgui.json` is gui.xml's menu
    tree, drawn as a bar across the top of the window with dropped panels and flyouts, and a
    click on an item sends the same command a key binding does. The views divide what is left
    of the window under it.
  - **The toolbars landed 2026-09-04**, as two more components of `data/vgui.json`: a row of
    select mask buttons under the menu bar and a column of tool buttons down the left side,
    both of them applying ADR-0019's two rules to `component::Button` - a press is answered
    out of the bounds the last draw left, and a toggle shows a flag it does not own.
    `ComponentRenderer::insets()` is what the views are shrunk by. 55 of the menu's 75
    commands still have no handler and log themselves when clicked.
- ~~Multiple viewports are the feature that will push hardest on the pass model from
  [ADR-0003](../adr/0003-one-realtime-engine.md).~~ Done 2026-09-01, and **the model
  expressed it**. Four views is four `Pass`es over one `Frame`: each carries its region as
  its viewport and scissor, its own camera at set 0, and clears its own region of the colour
  and depth attachments. Nothing in the frame model had to change to allow it -
  `FrameUniforms` already kept a slot per pass per frame in flight, and `Recorder` already
  honoured a pass viewport. What did have to change was the one-canvas-per-frame limit
  above, which is a renderer defect rather than a model one. The orthographic per-pass
  camera deferred out of phase 5 lands with ADR-0012.
- ~~The three manipulators.~~ Landed 2026-09-02 as
  [ADR-0015](../adr/0015-manipulators-write-the-object-transform.md). A handle writes the
  mesh's `dag::Transform` and never its geometry, is drawn at the object's own origin
  because that is where the transform pivots, and is picked by projecting itself to the
  screen and measuring the cursor's distance from it. `Manipulator` is the base - an axis
  constraint, a coordinate space, and a placement giving the origin, the alignment and a
  handle length that is a constant number of pixels converted through the view;
  `TranslateManipulator`, `RotateManipulator` and `ScaleManipulator` are the three, and
  `TransformTool` is the third `Tool`, with q, w, e and r choosing which is in force.
  - **A drag is measured rather than read.** `apply()` is given the two cursor positions
    either side of one motion event and adds what it measures. Both rigel defects go with
    that: a free drag keeps its horizontal component, and a translate accumulates instead of
    writing one frame's delta as an absolute position. The uninitialised coordinate space
    goes too.
  - An axis drag is the gesture's component along the projected handle, and a rotate ring
    turns by how far the cursor swept round the origin on screen. Rigel asked instead whether
    an axis was more horizontal or more vertical and used that whole component, which is the
    same answer only for an axis aligned view.
  - **The handles are an overlay**: a second pass per viewport with no depth attachment, over
    what the scene pass left, per ADR-0011. A handle shares its plane with the construction
    grid's own axis lines, and the depth test decides between them arbitrarily.
  - A ring within about eight degrees of edge on is not offered to a click. Its projection is
    a line through the middle of the manipulator whose ends land on the rim of the ring
    facing the camera, so it would take the clicks meant for that rim.
  - Still open: a component mode selects a face and then moves the object, because a
    manipulator writes the transform. Moving a component is a modelling operation and there
    is none.
- ~~**Undo has no prototype.**~~ Designed and landed 2026-09-02 as
  [ADR-0016](../adr/0016-undo-records-what-has-already-happened.md). Rigel has no undo or
  redo anywhere, so there was nothing to fold in. **A command is a record of a change that
  has already been made**, not a request to make one: `Command` has `undo()`, `redo()` and
  `name()` and no `execute()`, and `CommandStack::push` never applies anything. That is what
  an interactive gesture needs - a drag applies a manipulator on every motion event, because
  it cannot wait for its own end to show what it is doing, so by the time anything can be
  recorded the change has been made several hundred times over.
  - **One gesture is one command.** `TransformTool` is where a gesture's beginning and end
    are known, so it snapshots the placement when a handle is grabbed and pushes a
    `TransformCommand` when the drag ends. Every path out of a drag commits - a release and
    a mode change that drops the drag alike - and a handle grabbed and released without
    moving records nothing.
  - `CreateCommand` is the other one, and the first do goes through its `redo()`, so making
    a mesh and redoing one are the same code rather than two that have to agree.
  - A command holds the mesh it acts on rather than its id: a mesh taken out of the scene by
    an undo stays alive in the command that removed it and comes back with the id it had, so
    a transform command deeper in the history still names the same object.
  - Selection is not history. Undo keeps the scene's own invariant that at most one mesh is
    selected and restores nothing else, so undoing a create leaves nothing selected.
  - Undo and redo are on z and y, unmodified: a mapping binds one key with one state and has
    no notion of a chord, so ctrl-z is not expressible and control is already the truck
    camera modifier. Placeholder keys, like the create and select mask ones.
  - Verified against a run: a cylinder created, undone, undone again against an empty
    history and redone, with the validation layer silent.
  - Still open: nothing but a create and a transform is undoable, and a modelling operation
    that edits geometry will need to record the topology it changed rather than a placement.
- ~~**There is nothing a menu item could invoke.**~~ Landed 2026-09-02 as
  [ADR-0017](../adr/0017-a-command-is-a-name-in-a-context.md), which is item 8 of the
  survey's delete list. A command is identified by its context and name together -
  `Event::str()`, which is the form gui.xml's 51 command strings are already in - and
  `v3d::editor::CommandDirectory` maps that to a handler. A key binding and a menu item carry
  the same `event::Event`, so both reach the same handler with nothing added for the second.
  - The command names are gui.xml's, and `data/mappings.json` was rewritten onto them, so
    translating the menus is now writing the menu tree rather than also inventing a name
    table. `ui::quit` and `edit::undo`/`edit::redo` are the two deviations - gui.xml gives
    quit no context and has no history commands at all.
  - **Only a destination event is a command**, which fixes a defect the old chain hid: every
    keypress reached the editor's handler twice, once as itself and once as what it mapped
    to, and the chain dropped the raw one silently because its `keyboard` context matched
    none of the six it knew.
  - An unregistered command is logged rather than ignored, and `names()` says what the editor
    can do, so a translated menu is checkable against it. The 24 registrations cover 21 of
    the 51; the other 30 have no handler.
  - **`Tool` stays in the editor**, which is the other half of survey item 8. No game in the
    repository holds a gesture open across events, and one consumer is not a library.
- ~~**A project is not saved anywhere.**~~ Landed 2026-09-02 as
  [ADR-0018](../adr/0018-a-project-is-json-and-stores-topology-verbatim.md), which is item 9
  of the survey's delete list. A project is a JSON document holding a version, a name and one
  array of meshes - rigel's `<project>`/`<scene>`/`<mesh>` shape in the encoding the tree
  already parses, since the XML library rigel used went with `vault/quantumxml`.
  - **The topology is stored as it stands** rather than as the calls that would rebuild it.
    `BRep::addFace(points, normal)` welds vertices and pairs edges by search, so a mesh
    rebuilt through it comes back renumbered, and anything that names an index - a selection,
    a modelling record, a per-face material - would then name something else. Written index
    for index, a round trip is the identity.
  - Neither a mesh's id nor its selection is stored. An id comes from a process wide counter
    and a saved one would collide with a mesh already loaded; selection is where the user is
    rather than what the document holds, which is the rule ADR-0016 applies to history.
  - A file the reader does not fully understand is refused rather than loaded as far as it
    gets, and the document in memory is left alone. An index naming a vertex, edge or face
    the mesh does not hold is what the wireframe and the picker would walk off the end of.
  - `Vertex::edge_` turned out to have no writer in any construction path and no reader
    outside its own test, so it was indeterminate on every mesh in the tree. It is
    initialised to `INVALID_ID` now, and is not one of the things a file carries.
  - Reading clears the history: the commands describe a scene that no longer exists.
    `TransformTool::cancel()` is new for the same reason - a gesture under way is holding a
    mesh the read is about to take out of the scene.
  - Verified against a run: a cube and a cylinder created and saved, a plane created, the
    file loaded back over all three, and an undo after it reporting an empty history - with
    the validation layer silent.
  - Still open: there is no file chooser, so both commands work on one document at a fixed
    path beside the executable. No "save as", no dirty flag, and nothing warns before a load
    replaces unsaved work.
- ~~**There is nothing that draws a menu.**~~ Landed 2026-09-02 as
  [ADR-0019](../adr/0019-the-ui-is-laid-out-by-what-draws-it.md). `api/ui` had a menu, but it
  was a game's pause menu - one panel centred on the canvas, navigated by the keyboard, with
  no notion of a strip and no idea where the cursor is. **Drawing is what lays the ui out**:
  `ComponentRenderer` leaves every component holding the bounds it was drawn in, and
  `component::MenuBar` answers the cursor by testing it against those. The components already
  carried a position, a size and a `bound()` and nothing had ever written to them.
  - A menu is drawn twice at once - a label in the strip and the panel it drops - so the
    label's bounds live on the bar and the menu's own are the panel. One component cannot
    hold two rectangles.
  - **A check item does not own the state it shows.** `menu::ItemType` gained `Check` and
    `Radio`, which is what gui.xml marks its `<menuitem>`s with; activating one sends its
    command and marks nothing, and `Controller::syncMenu` reads the editor back afterwards.
    Otherwise the mark would disagree with the flag the first time a key invoked the same
    command.
  - The bar is offered the cursor before the tools, except during a gesture, and a press it
    took is remembered so the release does not reach three tools that never saw the press.
  - **gui.xml has 79 distinct command strings, not the 51 recorded here and in the survey**;
    73 of them are on a menu and six more only on a toolbar or a binding. The editor answers
    to 24, 20 of which the menu names - the other four are `view::drag` and the three camera
    modifiers, which are held rather than invoked.
  - Verified against a run: the bar over four viewports, Create > Poly > Cube making a cube,
    the three show flags marked and toggling, an unregistered command logging itself, and
    Project > Quit shutting down - validation silent throughout.
  - ~~Still open: the two toolbars.~~ Landed 2026-09-04, under the same ADR: `component::Toolbar`
    is a strip of buttons on the top or the left edge, `Button` carries the same
    `event::Event` a menu item does, and both are drawn onto the ui canvas. Two things the
    translation could not take verbatim: gui.xml's top toolbar gives its nine buttons neither
    a name nor an icon, so the four the editor has a mask for are labelled with the mask's own
    name and the five with no node type to select are left out as ADR-0014 left them out of
    the mask; and the left toolbar's four icons were labelled instead, there being no image
    path in `api/ui` at the time. **The left toolbar is iconic as of 2026-09-04**, with
    [ADR-0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md): a `Button`
    names an image, the ui engine's image pass resolves it through the app, and the four PNGs
    were recovered out of `rigel/icons/` into `vertical3d/data/icons/` before that tree was
    deleted.
- ~~Delete `rigel/` once the fold-in is complete, and not before.~~ **Deleted 2026-09-04**,
  the survey's list having been worked off. The four icons in `rigel/icons/` went with it and
  are the only thing a later change would want; `git show 54d79e8^:rigel/icons/rotate.png` is
  where they are if an iconic toolbar is ever built.

Done when: the editor opens a project, draws a scene from multiple viewports, and rigel has
nothing left worth taking. It draws a scene from multiple viewports as of 2026-09-02, selects
what is in it, moves, turns and resizes what is selected, takes any of it back, dispatches
every one of those by name, saves and opens what it has made, and carries gui.xml's menus and
both of its toolbars as of 2026-09-04, and `rigel/` is deleted. **Phase 6 is done.**

### Ongoing — tests

Deliberately not last. This is independent of the render rewrite and blocked by nothing.

Tier 1 landed on 2026-08-31. `enable_testing()` and a `v3d_add_test` helper are in the root
CMakeLists, eight binaries build from `api/<lib>/tests`, and `ctest --test-dir
out/build/x64-Debug` runs the lot in about a second. **Twenty suites and 381 cases run as of
2026-09-04**, every app but odyssey included. Coverage is every `api/` library bar the render
code below the recorder: `type`, `brep`, `dag`, `image`, `font`, `input`, `event`, `asset`,
`config`, `ecs`, `audio`, `log`, `engine`, the window-free half of `render` - which since
phase 3 includes the canvas's batching, transform stack and projection - and `ui`, whose
ComponentRenderer is testable because it takes text measuring and writing as callbacks. Still
uncovered: everything in `api/render` below the recorder, and the two entry points that open
a device - `Feature::Window` and `audio::Engine::initialize()`.

Tier 3 is done for every app that has logic worth covering. Odyssey is the exception and is
not an omission: `Movement::tick` returns true and does nothing, and `Sprite`, `SpriteSheet`,
`Actor`, `Tile` and `ui::Screen` are empty declarations, so there is nothing in it to test
yet. The two `run-unit-tests.sh` scripts are deleted - they invoked a `unit_tests` binary no
CMakeLists builds, from an autotools tree that is gone, and ctest is the runner now.
`tetris/build-code-coverage-report.sh` is the last of that family and still calls
`make gcov-clean`.

Three tiers, decided 2026-08-30:

1. **Unit tests per api library** — each `api/<lib>` owns its own `tests/` and its own
   target. This is where the bulk of coverage should live.
2. **An integration suite across libraries**, for the behaviour that only appears when
   several of them are wired together — asset loading through config, input through event
   mapping, a scene collecting into a frame.
3. **Per-app suites**, for app logic that is not in a library — tetris board and tetrad
   rules, pong scoring and collision, voxel chunk meshing.

The layout partly exists already: `moya/tests/` holds real tests (Polygon, RenderContext,
Renderer, ReyesPrimitive, Vertex), `tetris/tests/` has a bare `TestMain.cxx`, and the two
`run-unit-tests.sh` scripts expect a per-app binary. So tier 3 is a revival, not an invention.

Work:

- ~~Split `v3dlibs/tests/` by target library and move each file to its api home before the
  legacy tree is deleted.~~ Done 2026-08-31, command-layer tests included — three rewritten
  against `api/event`, three dropped as covered.
- ~~`enable_testing()` at the root plus `add_test` per target, so one `ctest` run covers
  everything and individual suites stay separately runnable.~~ Done 2026-08-31, as
  `v3d_add_test(<lib> <sources>)`.
- ~~Add ctest to CI alongside cpplint. Nothing yet builds the tree in CI, so this is a new
  workflow rather than a step added to the cpplint one.~~ Done 2026-09-04 as
  [.github/workflows/ctest.yml](../../.github/workflows/ctest.yml), and it is a new workflow
  because cpplint needs nothing but python where this needs four things the repository does
  not carry: an MSVC toolchain, the Vulkan SDK, a vcpkg install and both vendor submodules
  built. Three of those are worth knowing about.
  - **Every configure needs the Vulkan SDK**, including one that only builds the suites that
    never touch a device - the root CMakeLists calls `find_package(Vulkan)` and looks for
    `glslc` with a `FATAL_ERROR`. So there is no cheap subset job to run instead, and the
    workflow builds and tests the whole tree.
  - **`vendor/vcpkg` is not tracked**, so CI uses the runner's own vcpkg through
    `VCPKG_INSTALLATION_ROOT`. A cold install builds boost from source, which is most of an
    hour; the binary cache is what makes a second run cheap.
  - **libnoise is built out of source**, because the `CMakeCache.txt` it commits names a
    generator no runner has. It is the only vendored library the workflow builds since
    [ADR-0021](../adr/0021-sdl3-mixer-replaces-soloud.md) took the mixer from vcpkg.
  - Ninja and a `vcvars64.bat` located through `vswhere`, rather than the Visual Studio
    generator, so a CI failure means what a local one does. Everything but the runner-specific
    half - the SDK install, the runner's vcpkg, and `vswhere` - was verified locally by
    configuring, building and testing a fresh tree with the same commands.
- ~~Cover the api libraries the salvage did not reach: `asset`, `config`, `dag`, `ecs`,
  `audio`.~~ `dag` and `ui` landed 2026-09-04, and `asset` and `config` the same day - the
  two an app most visibly depends on, and the ones the config-format migrations in this phase
  were verified against by reading `Config::load` rather than running it. Running it found
  the hole that reading it had not: `Manager::loadTypeFromExt` throws for an extension it has
  no loader for, and that was the one path out of `Config::load` that escaped as an exception
  rather than the false return every other rejection takes. Fixed with the test. `ecs`,
  `audio`, `log` and `engine` closed the list the same day - the last four libraries that need
  neither a window nor a GPU - and running them found three more defects that reading had not:
  `ecs::component::Color3`'s definition named its second and third parameters in the opposite
  order to its declaration, so `green()` returned the blue argument and `blue()` the green one,
  invisible because white is the only colour anything constructs; `audio::AudioClip::load`
  returned true whatever the backend reported, so a missing wav became a clip that plays
  silence rather than an error a caller can see; and `engine::Engine::registerEventMappings` reached
  for `at()` on a document that need not hold the key, which is the same throw out of startup
  `Config::load` had already been fixed for. All three are fixed with the tests, and
  `ecs::System` gained the virtual destructor a polymorphic base needs.
- ~~Revive `moya/tests/` (five real test files, no target), which is tier 3 and now needs
  only a CMakeLists.~~ Done 2026-09-04, and it needed more than a CMakeLists: the five files
  were written against the old `v3D::Moya` namespace and `v3D::Vector3`/`Matrix4`, all of
  which the library left behind when it moved to `v3d::moya` and glm, so they were rewritten
  rather than merely wired up. 18 cases over the vertex, the polygon and its bound, the
  render context's defaults and coordinate systems, and the renderer's context stack.
  `pong/tests/` landed the same day and is the tier 3 item this list never named: the whole
  of `PongScene::tick` - collision, scoring, the wall bounce, the paddle run and the
  single-player ai - which turned out to have its ai travel inverted, moving the paddle away
  from the ball it was meant to be returning. `tetris/tests/`, `voxel/tests/` and
  `vertical3d/tests/` were already done.
- ~~Start coverage on the libraries that need neither a window nor a GPU.~~ Done, and closed
  as of 2026-09-04 - `dag`, `asset`, `config`, `ecs`, `audio`, `log` and `engine` included.
  All of it can run in CI from day one, which the render libraries cannot — see
  [ADR-0007](../adr/0007-ci-rendering-tests.md).

Two items on `docs/TODO.md` — "Get tests working again" and "integrate tests into github
actions" — are this workstream.

### Ongoing — documentation

`CLAUDE.md` now covers build, lint, architecture and conventions. The gaps worth filling,
in rough order of value:

- The rationale for the Vulkan move. Nothing anywhere records why, and the same is true of
  the SDL3 upgrade. This does not need to be a formal ADR, but the reasoning should exist
  somewhere before it is forgotten.
- ~~`docs/RenderingPipeline.md` is a design note written as open questions - rewrite it to
  describe what exists once phase 2 settles the `Frame`/`Operation` question.~~ Done
  2026-08-31: it now describes the frame loop, the pass and draw item model, the binding
  convention and what is still missing. `docs/ECSDesign.md` is still a set of open questions,
  and the one about what a renderable component looks like is now the live one.
- `docs/Dependencies.md` still lists sdl2 and does not mention Vulkan.

## Where moya and talyn fit

Suggestion: they do not need to fit into the realtime stack at all, and trying to make them
is the wrong goal.

Both are offline renderers. talyn already links only `v3dlib_log` and `v3dlib_image`; moya
links `v3dlib_type` and its own `v3dlib_moya`. Neither touches the window, render engine,
input or UI layers, and neither should. The natural boundary is that `api/` contains a set of
libraries with no realtime dependency — `type`, `brep`, `dag`, `image`, `log`, `asset` — and
the offline renderers are consumers of exactly those.

Concretely that means: keep them building, let them take from the non-realtime libraries, and
do not schedule them into the Vulkan work at all. If anything, they are a useful forcing
function for keeping those libraries free of realtime dependencies.

## What is left

Every phase is closed, so nothing below is blocked by anything else and none of it has an
order. It is here because a phase note is a bad place to leave an open item once the phase is
done.

**The two ongoing workstreams**, both above:

- Tests: `ecs`, `audio`, `log` and `api/engine` landed on 2026-09-04, which is every library
  that needs neither a window nor a GPU. What is left needs one: everything below the recorder
  in `api/render`, `Feature::Window`, and `audio::Engine::initialize()` - all of it waiting on
  [ADR-0007](../adr/0007-ci-rendering-tests.md).
- Documentation: the rationale for the Vulkan move and for the SDL3 upgrade is recorded
  nowhere — [ADR-0001](../adr/0001-vulkan-replaces-opengl.md) records the decision, not the
  reasoning behind it. `docs/ECSDesign.md` is still a set of open questions, and the one about
  what a renderable component looks like is the live one.

**Defects carried in the notes, none of them scheduled:**

- ~~**Pong and tetris call `shutdown()` from their quit handlers**, which is the
  frame-after-teardown defect `CLAUDE.md` describes.~~ Fixed 2026-09-04: both handlers call
  `Engine::quit()`, and `main` is the only caller of `shutdown()` left in either app. Closing
  the window turned up a second defect on the same path - neither app tore its renderer down,
  so `Engine3D::shutdown()` never ran, the context outlived
  `render::realtime::Window::destroy()`, and the instance reported a leaked `VkSurfaceKHR` on
  every exit. Both now call `renderer_->shutdown()` first, as voxel, odyssey and the editor
  already did, and both exit validation clean.
- ~~**Nothing in the tree makes a sound, and the reason is the vendored soloud.**~~ Fixed
  2026-09-04 by replacing it: `api/audio` is SDL3_mixer from vcpkg per
  [ADR-0021](../adr/0021-sdl3-mixer-replaces-soloud.md), and pong plays its clips. The
  faults underneath it went the same day - a clip resolves through the asset manager,
  `initialize()` reports what the backend said instead of ignoring it, and `playClip` and
  teardown are guarded against a device that never opened. Taking the port moved the vcpkg
  baseline from 2025-02-21 to 2026-05-09, which carried boost 1.86 to 1.91 and SDL3 3.2.4 to
  3.4.8 with it; the only source change that cost was `boost::json::error_code` and
  `boost::json::system_error`, which 1.91 removed in favour of the `boost::system` names they
  aliased. `libnoise` is the only submodule left.
- ~~The jpeg reader and writer both reverse their rows.~~ Fixed 2026-09-04, both together.
  `imagewriter_jpeg_orientation_test` pins them by encoding and decoding through libjpeg
  directly on one side of each check, because a round trip returns the image it was given
  whichever way round the pair agrees to store it.
- `event::Context::active` is written and read by nothing. It is the state scoping the editor
  was expected to need, from the v3dlibs audit.
- Two items survive on `docs/TODO.md` that nobody has scoped: the `size_t` / `unsigned int`
  audit, and the build warnings.

**Editor work the phase left open**, if the app is what gets pushed rather than the platform:
55 of the menu's 75 commands have no handler and log themselves; there is no modelling
operation, so a component mode selects a face and then moves the whole object; one thing is
selected at a time; there is no file chooser, no "save as" and no dirty flag; the viewport
panes are not draggable; and input capture for input-type menu items is still unbuilt, so the
five in `pong/data/vgui.json` are unreachable.

## Open questions

None outstanding. The last one — how render tests run in CI — is settled in
[ADR-0007](../adr/0007-ci-rendering-tests.md).
