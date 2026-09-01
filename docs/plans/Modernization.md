# Modernization Plan

Draft, 2026-08-30. Living document — update the state notes as things land.

Several rewrites are in flight at once. This is an attempt to work out what actually
blocks what, so they can be finished in an order that keeps the tree buildable instead
of half-finishing all of them at the same time.

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

Verified against the tree on 2026-08-30.

**SDL3 is done.** No SDL2 code remains in `api/` or in any app. The dead references are gone
as of 2026-08-31: `talyn`'s `${SDL2_LIBRARIES}` link, `odyssey/cmake/FindSDL2.cmake`, and a
stale comment in the root `CMakeLists.txt` claiming `find_package(SDL3)` resolves through a
`FindSDL2` module. The only SDL2 code left anywhere is `v3dlibs/hookah/drivers/sdl2/`, which
is not built and goes when that tree does - see [docs/V3dlibsAudit.md](../V3dlibsAudit.md).
Treat this workstream as closed.

**The legacy trees are not all the same kind of thing, and only one is disposable.**
None of `v3dlibs/`, `luxa/`, `rigel/`, `vertical3d/` or `vault/` is in the root
`add_subdirectory` list, so "not built" says nothing about whether it is finished with.

- **`vertical3d/` is an app, not dead code.** It is the desktop 3D editing tool the
  repository is named after, listed in `README.md` alongside the others. It is to be
  rewritten onto the new api, not deleted.
- **`rigel/` is the earlier prototype of that app** and holds functionality the rewrite
  needs to absorb first — viewport layout, an arcball camera, a construction plane,
  transform manipulators, and the poly modelling command sets. Two items already on
  `docs/TODO.md` cover part of this. It can only be deleted after the fold-in.
- **`luxa/` and `v3dlibs/` are migrations in progress**, and each needs a functional
  equivalence audit before removal, not an assumption that `api/` covers it.
- **`vault/quantumxml` is genuinely archived** — an XML parser superseded by the JSON
  config work, per `docs/Vault.md` and the first item on `docs/TODO.md`.

Among the apps, none still includes a legacy header — `voxel/src/Controller.h` is already
clean, and pong's Luxa usage is commented out. `vertical3d/` is the only consumer left, of
`v3dlibs/core`, `hookah` and `command`. `v3dlibs/tests/` is the only test corpus in the
working tree and must be salvaged before that tree goes; `luxa/tests/` and two v3dlibs tests
were deleted rather than migrated and are recoverable from history.

**The luxa audit is done.** Written up in [docs/LuxaAudit.md](../LuxaAudit.md), 2026-08-31.
`luxa/` cannot be deleted yet, and the blocking list is nine items long. The short version:
`ComponentRenderer` has to be rebuilt on the Vulkan quad rather than ported, and takes the
ortho UI pass and theme-to-font resolution with it; `ComponentManager` is only half covered
by `ui::Engine` plus `ui::Container`, with mouse hit-testing, hover/focus, the active theme
and image loading all unported; and `ui::Engine::load` covers menus alone, so the whole of
`api/ui/style/` is migrated but unreachable. Two of the three flagged unknowns shrank on
inspection — `Window` is an empty stub with no implementation, and `MenuStack` is a `draw()`
routine whose navigation was always in `Menu`, which migrated intact, so `ui::Navigation`
needs no reconciliation.

The audit also turned up regressions in `api/ui` itself, none blocked by Vulkan. The worst:
`Menu::activate()` had its dispatch commented out, and the menu never navigated at all
because `Engine::loadMenu` set neither the active level nor the active item. Both are fixed
as of 2026-08-31, along with `MenuItem`'s missing value field and `ui::Engine`'s missing
theme accessor. Input capture for input-type menu items is still unbuilt, so the five input
items in `pong/data/vgui.json` remain unreachable.

**The v3dlibs audit is done.** Written up in [docs/V3dlibsAudit.md](../V3dlibsAudit.md),
2026-08-31. Most of the tree is genuinely covered: `input/` by `api/input`, `hookah/Window`
by `api/render/realtime/Window`, `gui/InputEventAdapter` by `api/event` plus `api/input`, and
`command/` by `api/event` — nothing still needs `CommandDirectory`, `CommandTable` or
`StateController`. What keeps the tree alive is narrower than expected: the test corpus has
to move, and `vertical3d/` still includes six of its headers. `core/Scene` and `SceneVisitor`
should *not* be folded into `api/dag` — that library is 374 lines of skeleton with no
traversal, no visitor and no consumers anywhere — they go with the editor in Phase 6, as
`CreatePolyCommandSet` already does.

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

**Vulkan is the critical path and nothing renders right now.** `Window3D` owns a
`vulkan::Instance` and `vulkan::Surface`; `Context3D` owns a `vulkan::Device` and
`vulkan::Swapchain`. Missing: render pass, framebuffers, command pool and buffers, sync
objects, and the acquire/submit/present loop. Meanwhile `Engine3D::renderFrame()` and every
app renderer still call OpenGL against a context that is no longer created. pong links and
does not draw. Until the frame loop exists, "it builds" is the only signal available.

**Build health.** Clean: all `api/` libraries, pong, talyn, v3dshell, imagetool, and - since
2026-08-31 - odyssey and tetris. Broken: voxel alone, drifted behind api changes.

**Odyssey builds, as of 2026-08-31.** It runs on `Feature::Window2D` → `Engine2D` →
`Context2D`, which is SDL's own renderer, not GL and not Vulkan, so it was never blocked by
Vulkan. The signature bug is fixed - `Operation2D` now narrows the frame's `Context` to a
`Context2D` once and calls a protected `run2D`, which `Blit2DTexture` implements - but that
turned out to be only the first of several problems, because fixing it let odyssey reach the
link stage for the first time. Its `CMakeLists.txt` had never listed `engine/Engine.cpp` or
`system/Movement.cpp`, so those two files had never been compiled and had drifted as far
behind the api as voxel has; and it re-ran `find_package(Boost COMPONENTS log)` locally,
narrowing `Boost_LIBRARIES` and losing boost::json. All of that is fixed. Whether odyssey
*runs* is still a question nobody has asked.

**Voxel is blocked by Vulkan.** It uses `Feature::Window3D`.

**The 2D engine is small and half-finished.** All of `api/render/realtime/2D/` is roughly
610 lines of thin `SDL_Renderer` wrapping — `Context2D` is an `SDL_Renderer`, `Texture2D` is
an `SDL_Texture`, `Scene2D::collect()` returns an empty `Frame` with the real collection
commented out. Odyssey's entire use of it is: build a texture from a surface, cache it, and
emit one `Blit2DTexture` per renderable. So the consolidation port has a small surface, and
the one operation it needs — a textured quad — is the same primitive tetris needs anyway.

## What blocks what

The instinct is to queue everything behind Vulkan. That is wrong for about half the work.

Blocked by the Vulkan frame loop: pong's renderer, tetris's renderer, voxel, and odyssey's
eventual port onto the consolidated engine. Note that odyssey *building* is not blocked — only
its move off `SDL_Renderer` is, and the SDL path can keep running until the Vulkan one reaches
parity.

Not blocked by anything: deleting the legacy trees, the SDL2 leftovers, the `Operation`
signature fix and odyssey with it, tetris's config-format migration, the test framework, and
docs. Roughly half the outstanding work is in this bucket, and all of it makes the Vulkan
work easier to review by shrinking the noise around it.

The one hard ordering constraint inside the Vulkan work: the apps need batched quads, and
nothing provides them. `v3d::gl::Canvas` batches coloured quads but carries no texture
coordinates, so every tetris piece, all text, and every odyssey tile is unserved. Because the
engine is consolidating, this primitive has to satisfy all three apps — decide its shape
before building the frame loop around it, or it gets built twice.

## Phases

### Phase 1 — Clear the ground

None of this is blocked. It shrinks the surface area everything else has to work against.

- Delete `vault/quantumxml`. It is the only tree that can go without an audit first.
- ~~Audit `luxa/` against `api/ui`~~ — done, [docs/LuxaAudit.md](../LuxaAudit.md). The tree
  stays until its nine-item blocking list is worked off; five of those items need nothing
  that does not already exist, and the rest land in Phase 3.
- ~~Audit `v3dlibs/` against the api libraries~~ — done,
  [docs/V3dlibsAudit.md](../V3dlibsAudit.md). Five things have to land before the tree can
  go, and the last of them waits on Phase 6 unless `core/` is moved across early.
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
- Salvage `v3dlibs/tests/` — the largest reason that tree is still alive — into per-library
  `tests/` directories, fixing the stale `../3dtypes/` and `ImageFactory.h` include paths on
  the way. Recover `BRepTest`/`CameraProfileTest` from `6cfb4b6^` and `luxa/tests/` from
  `d31a2e9^`; all three were deleted rather than migrated.
- Leave `rigel/` and `vertical3d/` alone. They belong to Phase 6.
- ~~Delete `talyn`'s `${SDL2_LIBRARIES}` link and `odyssey/cmake/FindSDL2.cmake`.~~ Done
  2026-08-31, along with a stale `FindSDL2` comment in the root `CMakeLists.txt`. Neither
  changed a build: the variable was undefined and expanded to nothing, and nothing set
  `CMAKE_MODULE_PATH` to reach the module. SDL3 workstream closed apart from
  `v3dlibs/hookah/drivers/sdl2/`, which goes with that tree.
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
  config is still the old inline form (next item), and it draws through fixed-function GL
  against a context nothing creates.
- Migrate `tetris/data/config.json` from the old inline `keys`/`menu` form to the
  `{"configs": [...]}` form that `Config::load` requires. Pong's `data/` is the reference.

Done when: the whole tree builds, `vault/` is gone, and the luxa and v3dlibs audits have
produced a written list of what still has to move. Both audits are closed.

### Phase 2 — Vulkan to first pixel

- Dynamic rendering against the swapchain image views. No `VkRenderPass`, no
  `VkFramebuffer` — enable `dynamicRendering` through `VkPhysicalDeviceVulkan13Features`
  chained onto device creation, which also means moving from `pEnabledFeatures` to
  `VkPhysicalDeviceFeatures2`.
- Command pool and per-frame command buffers.
- Sync: image-available and render-finished semaphores, in-flight fences, frames-in-flight.
  Use `synchronization2` barriers rather than the 1.0 forms.
- `Engine3D::renderFrame()` acquires, records, submits, presents — and recreates the
  swapchain on `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR`. That is the reliable resize
  trigger; note `render::realtime::Engine::resize` is dead code that nothing ever connects.
- Build the frame as a list of passes from the start, even while there is only one.
  Retrofitting the pass model after the fact is the expensive version of this.
- The draw-item type and its sort key, including the 2D layer field. Recording walks each
  pass's queue in submission order for now; sorting and merging come later.
- Resource handles: a pipeline cache, a material or descriptor registry, and texture
  handles. Nothing can be sorted or merged until these exist, so they belong here rather
  than being deferred to the phase that first needs them.
- Bind by update frequency, and write the convention down: set 0 per-frame — camera,
  projection, viewport — bound once by the pass; set 1 per-material; push constants
  per-object for transform and tint. Everything downstream depends on this being fixed.

Done when: a window clears to a colour and survives a resize and a minimize.

### Phase 3 — pong as the pilot

pong is the only app already on the `api/` framework and the only one that builds, so it
defines what the api actually needs. Port it before generalising.

- The batched quad primitive: a Vulkan replacement for `v3d::gl::Canvas` plus its operation,
  the 1x1 white texture, and flush-on-texture-change. This serves every app, so build it
  once and build it properly.
- Text: a Vulkan path for `operation::TextureFont` and `TextureFontCache`, which is the same
  primitive drawing from the glyph atlas.
- Re-wire pong's UI to `v3d::ui`; delete the commented-out Luxa block.
- Delete `api/gl/` and drop GLEW and OpenGL from every link list once nothing references
  them.

Done when: pong plays, and no GL remains anywhere in the tree.

### Phase 4 — tetris

The quad primitive already exists from Phase 3, so what tetris adds is the sprite atlas.

- Pack the seven piece textures into one `v3d::image::TextureAtlas` at load time, so the
  board draws as a single batch instead of flushing per block.
- Replace the fixed-function draw path wholesale — `glBegin`/`glEnd`, `glTranslatef`,
  `glPushMatrix`, `glOrtho`. This is the oldest rendering code in the repo and none of it
  survives; rewrite against the Canvas equivalent rather than porting call by call.
- Rewrite the debug text against `TextureFont`.
- Load piece textures through `asset::Manager` instead of `image::Factory` with hardcoded
  relative paths.

Done when: tetris plays.

### Phase 5 — voxel and odyssey, and the engine consolidation

Voxel needs a 3D scene path — shaders, meshes, camera — that pong and tetris never exercised,
so it is the app that will drive real api growth. It needs its own survey before it can be
scoped.

Odyssey is where the consolidation actually lands. It should come out of Phase 1 building on
`SDL_Renderer`; whether it *runs* is a separate question nobody has asked yet. Once the
Vulkan path has textured-quad batching from Phase 4, port it:

- Replace its `Blit2DTexture` usage with the batched textured quad. That is close to the
  whole port — see the state notes above.
- Move it from `Feature::Window2D` to the single window and engine.
- Delete `api/render/realtime/2D/` and `operation/2D/`, collapse `Window2D`/`Window3D` into
  one `Window`, and drop the `Window2D`/`Window3D` feature flags for a single windowing flag.
- Add the sprite/orthographic pass properly, so a 2D game gets painter ordering and no depth
  buffer without special-casing the engine.

### Phase 6 — the Vertical3D editor

The app the repository is named after, and the largest piece of work here. It is a desktop
3D editing tool, which means it needs things no game in this repo does: multiple viewports,
manipulator gizmos, a construction plane, selection, and an undoable command model.

- Survey `rigel/` and decide what to fold in. It holds the working prototype of most of the
  above — `ViewLayout`, `ViewPort`, `RenderView`, `ConstructionPlane`, `ArcBall`, the
  `manipulators/` and the `commands/` sets. `api/type` has already absorbed `ArcBall`,
  `Camera` and `CameraProfile`, and `api/brep` mirrors `libv3dcore/brep`, so the fold-in is
  partly done and partly duplicated. Two items on `docs/TODO.md` track the brep and command
  library merges specifically.
- Rewrite `vertical3d/` — `Controller`, `ViewPort`, `CameraControlTool`, `HWRenderContext`
  — onto the current api. `HWRenderContext` is the GL render context and does not survive
  [ADR-0001](../adr/0001-vulkan-replaces-opengl.md).
- Multiple viewports are the feature that will push hardest on the pass model from
  [ADR-0003](../adr/0003-one-realtime-engine.md). Four views of one scene is four passes
  with four cameras against one device, which the model should already express — this is
  the app that proves whether it does.
- Delete `rigel/` once the fold-in is complete, and not before.

Done when: the editor opens a project, draws a scene from multiple viewports, and rigel has
nothing left worth taking.

### Ongoing — tests

Deliberately not last. This is independent of the render rewrite and blocked by nothing.

There is no working test suite today: Boost.Test sources sit in `v3dlibs/tests/`, and
`pong/run-unit-tests.sh` and `tetris/run-unit-tests.sh` invoke a `unit_tests` binary that no
CMakeLists builds. The existing tests cover `api/type` (AABBox, ArcBall, Bound2D, Camera),
`api/brep` (Face, HalfEdge, Vertex), `api/image`, `api/font`, `api/input` and the old command
layer.

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

- Split `v3dlibs/tests/` by target library and move each file to its api home before the
  legacy tree is deleted. Drop the ones covering the removed command layer
  (`BindTest`, `CommandTest`, `CommandDirectoryTest`, `CommandInfoTest`, `EventInfoTest`,
  `InputEventAdapterTest`) unless the behaviour survived into `api/config` or `api/event`,
  in which case rewrite them against the new interfaces.
- `enable_testing()` at the root plus `add_test` per target, so one `ctest` run covers
  everything and individual suites stay separately runnable.
- Add ctest to CI alongside cpplint.
- Start coverage on the libraries that need neither a window nor a GPU: `type`, `brep`,
  `dag`, `image`, `asset`, `config`, `event`, `input`, `font`. Those can run in CI from day
  one, which the render libraries cannot — see open question 4.

Two items on `docs/TODO.md` — "Get tests working again" and "integrate tests into github
actions" — are this workstream.

### Ongoing — documentation

`CLAUDE.md` now covers build, lint, architecture and conventions. The gaps worth filling,
in rough order of value:

- The rationale for the Vulkan move. Nothing anywhere records why, and the same is true of
  the SDL3 upgrade. This does not need to be a formal ADR, but the reasoning should exist
  somewhere before it is forgotten.
- `docs/RenderingPipeline.md` and `docs/ECSDesign.md` are design notes written as open
  questions. Once Phase 2 settles the `Frame`/`Operation` question, rewrite the first to
  describe what exists.
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

## Open questions

None outstanding. The last one — how render tests run in CI — is settled in
[ADR-0001](../adr/0001-ci-rendering-tests.md), pending acceptance.
