# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A monorepo for the Vertical3D ecosystem: reusable C++ libraries under `api/` (targets named `v3dlib_*`, one per subdirectory) plus the apps that consume them at the top level — pong, tetris, voxel, odyssey, vertical3d, talyn, moya, imagetool, v3dshell.

Much of this code traces back to the early 2000s and is being modernised incrementally: C++17+, granular namespaces, CMake replacing autotools and VS solutions. Expect wide variance in how modern any given file is.

`v3dlibs/`, `rigel/`, `luxa/` and `vault/` are absent from the root `add_subdirectory` list, excluded from lint, and not built — but "not built" does not mean disposable, and they are three different situations:

- `rigel/` is the earlier prototype of the editor; its manipulator, construction-plane and modelling-command work has to be folded into the rewrite before it can go. [docs/RigelSurvey.md](docs/RigelSurvey.md) is the record of what is in there: none of it is portable — it is gtkmm 2 and immediate-mode GL with no build files — so the fold-in is a harvest of behaviour plus `rigel/docs/xml/gui.xml`, which is the editor's whole UI definition and the most valuable thing in the tree. Read it before deleting anything from `rigel/` or writing off a rigel feature as already covered. `vertical3d/` itself is built again as of 2026-09-01 — it is the editor, rewritten onto the current api, and it opens and draws four viewports of a construction grid.
- `luxa/` and `v3dlibs/` are migrations in progress, and both audits are now written up: [docs/LuxaAudit.md](docs/LuxaAudit.md) and [docs/V3dlibsAudit.md](docs/V3dlibsAudit.md). Two further surveys scope the app ports: [docs/VoxelSurvey.md](docs/VoxelSurvey.md) for voxel and [docs/RigelSurvey.md](docs/RigelSurvey.md) for the editor. Each lists what has to land before its tree can be deleted, plus the `api/` regressions the audit turned up. The two most consequential — `Menu::activate()` dispatching nothing and `api/input/Mouse::handleEvent` swallowing every mouse event — were both fixed on 2026-08-31 and are struck through in their audits; check there rather than assuming an item is still open. Read the relevant one before deleting anything from either tree.
- `vault/quantumxml` is genuinely archived, superseded by the JSON config work.

MSVC/Windows only in practice. The root CMakeLists passes `/std:c++latest` and `/permissive-` unconditionally, and targets set `/EHsc` and `/utf-8` individually.

## Process

[docs/sdlc.md](docs/sdlc.md) describes how work moves through the repo — where plans live,
when a decision earns an ADR, and what "verified" currently means. Decisions are recorded in
[docs/adr/](docs/adr/), indexed in its README — nine of them cover the Vulkan rewrite and
are worth reading before touching `api/render`. Phased plans live in
[docs/plans/](docs/plans/).

## Build

Visual Studio generates `out/build/x64-Debug` from [CMakeSettings.json](CMakeSettings.json). From a shell, use a Developer environment (`vcvars64.bat`), then:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
ninja -C out/build/x64-Debug -k 0         # keep going past the broken targets (see below)
```

- `/utf-8` is required, not cosmetic: spdlog's bundled fmt has a `static_assert` that fails without it.
- **Shaders are compiled at build time and embedded, not shipped as data.** `v3d_add_shader(<target> <source>)`
  in the root CMakeLists runs the Vulkan SDK's `glslc` over a GLSL file and writes SPIR-V as a C initialiser
  list into `<binary dir>/shaders/<name>.inc`, which the source `#include`s into a `uint32_t` array. The
  engine's shaders live in [api/render/shaders/](api/render/shaders/). `VULKAN_SDK` has to point at an SDK
  install, or configuration fails with "glslc was not found".
- Editing `vcpkg.json` re-runs the manifest install. A cold install builds boost from source and takes roughly 45 minutes.
- **`sdl3` is requested with its `vulkan` feature**, and has to be. Without it SDL builds with `SDL_VULKAN=OFF`, the windows video driver leaves `Vulkan_LoadLibrary` unset, and `SDL_Vulkan_LoadLibrary` fails with "No dynamic Vulkan support in current SDL video driver (windows)" — which surfaces as an unhandled exception on startup, not as a build failure. Changing it rebuilds SDL only, about five minutes.
- **Never delete `out/build/<config>/vcpkg_installed/`** — that directory *is* the dependency install. When CMake needs a fresh cache (typically after a VS toolset update leaves the cached `CMAKE_CXX_COMPILER` path pointing at a version that no longer exists), delete only `CMakeCache.txt`, `CMakeFiles/`, `build.ninja`, `cmake_install.cmake` and `.ninja_*`, then reconfigure. Reconfiguring is fast; reinstalling is not.
- `vendor/libnoise` and `vendor/soloud` are git submodules built separately, not by this project. `link_directories` expects their artefacts under `vendor/*/Debug`.
  soloud is prebuilt in the tree; libnoise is not, and `voxel` will not link without it. Its committed `CMakeCache.txt` names a
  "Visual Studio 17 2022" generator that no longer exists here, so configure it out of source instead of reusing that cache:
  `cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug`. The
  `CMAKE_POLICY_VERSION_MINIMUM` is needed because its `cmake_minimum_required(VERSION 3.0)` predates what current CMake accepts.
- **Assets shared by more than one app live in the root [data/](data/)**, committed once. `v3d_add_shared_data(<target>)` — defined in the root CMakeLists — copies them next to that app's executable after it links, merging into whatever the app keeps in `<app>/data`. Currently only fonts, used by pong, tetris, voxel and the font test suite. An app's own `data/` is copied the same way by `v3d_add_app_data(<target>)`, but **only tetris, voxel and odyssey call it**: everywhere else the `data/` directories in `out/build/<config>/<app>/` are manual copies and are years stale, so editing `pong/data/*.json` does not affect a run from the build tree until you copy it across yourself.
- `VCPKG_ROOT` in CMakeSettings.json has a doubled path segment (`vertical3d/vertical3d/vendor/vcpkg`) and points nowhere. vcpkg works through the toolchain file regardless.

## Lint

```
cpplint --linelength=180 --filter=-runtime/indentation_namespace,-build/namespaces_literals \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=vault --exclude=voxel/src/noise --exclude=v3dlibs --exclude=rigel --exclude=luxa --recursive .
```

Run in CI by [.github/workflows/cpplint.yml](.github/workflows/cpplint.yml). The first three excludes matter only locally - CI never builds, checks out no submodules and installs no ports, so it has none of those trees - but a developer machine has all three, and they hold two orders of magnitude more lintable files than the project does. `vcpkg_installed/` at the repo root is the worst of them at 80,000-odd third party headers. Note that `--exclude` filters what is linted and not what is walked, so the run still costs an `os.walk` of the whole tree either way. Current cpplint renamed the namespace-indent check to `whitespace/indent_namespace`, so the `-runtime/indentation_namespace` filter no longer suppresses it and **every file in the repo reports it**. Ignore those; treat anything else as a real finding.

## Tests

Boost.Test, one binary per api library, built from `api/<lib>/tests/` and registered with ctest — plus one per app, where the app has logic worth covering:

```
ninja -C out/build/x64-Debug                       # tests build with everything else
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite
out/build/x64-Debug/api/image/tests/v3dtest_image.exe --run_test=texture_test
```

`v3d_add_test(<lib> <sources>)` in the root CMakeLists builds `v3dtest_<lib>`, links the framework, and adds the ctest entry with the working directory set beside the executable so a suite's data files resolve. Link the library under test yourself in `api/<lib>/tests/CMakeLists.txt`. `TestMain` (one per target) carries the `BOOST_TEST_MODULE` define and nothing else.

Covered as of 2026-09-01: `type`, `brep`, `image`, `font`, `input`, `event`, `render`, `ui`, plus the app suites `tetris`, `voxel` and `vertical3d` — 140 cases, migrated out of `v3dlibs/tests/` (the command-layer tests were rewritten against `api/event`, and the two input tests against `Keyboard`/`Mouse`). `render` and `ui` cover only the parts that need neither a window nor a GPU — the frame and pass model including the camera and the record ordering `Pass::ordered()` produces, the draw item's sort key, the handle registry, the canvas's batching, transform stack and projection, and the line canvas's geometry and transform stack; and, for `ui`, menu layout, submenu descent and visibility, which are testable because `ComponentRenderer` takes text measuring and writing as callbacks rather than depending on the font library. Not covered: `asset`, `config`, `dag`, `ecs`, `audio`, `log`, and everything in `api/render` below the recorder — that needs a window and a GPU, so it waits on [ADR 0007](docs/adr/0007-ci-rendering-tests.md). `tetris/tests/`, `voxel/tests/` and `vertical3d/tests/` are the app suites — `v3d_add_test` is not api-only, and they build against `<app>/src/*.cxx` directly: the board and the tetrad, voxel's chunk, mesh cache, Morton code and chunk-seam face culling, and the editor's view layout, camera profile table and construction grid all need neither a window nor a device. Voxel's suite links `libnoise`, because `Chunk` is built against a `TerrainMap` and the vtable of the flat one a test supplies refers to the perlin implementation whether or not any case generates noise. `moya/tests/` still builds nothing; `pong/run-unit-tests.sh` and `tetris/run-unit-tests.sh` still invoke a `unit_tests` binary that does not exist. Boost.Test's leak check reports a permanent false positive for any suite that builds a `Logger` (spdlog's registry outlives the report), which is why `add_test` passes `--detect_memory_leaks=0`.

## Build health

Everything compiles and links as of 2026-09-01.

- **Clean:** every `api/` library, plus `pong`, `tetris`, `voxel`, `odyssey`, `vertical3d`, `talyn`, `v3dshell`, `imagetool`.
- **`voxel`** was the last holdout — it had drifted behind API changes in the shared libraries, and its
  `target_link_libraries` still named only `libnoise`, so it could not have linked even once the objects
  compiled. It now links a `v3dlib_*` set of its own, which since the port on 2026-09-01 is tetris's plus
  `libnoise` and minus `v3dlib_audio`. `src/game/Player.cxx` and `src/noise/noiseutils.cpp` were also
  missing from the target's source list.
- **There is no OpenGL in the tree.** `api/gl` was deleted on 2026-09-01 with the last thing that held
  it, and the root `find_package(OpenGL)` and `find_package(GLEW)` calls and the `glew` port in
  [vcpkg.json](vcpkg.json) went with it. A target that names `OpenGL::GL`, `GLEW::GLEW` or `v3dlib_gl`
  will not configure.

**Apps name neither soloud nor `v3dlib_audio` unless they play a sound.** `v3dlib_asset`'s Wav loader calls `v3d::audio::AudioClip::load`, so any app linking asset needs audio whether or not it makes noise; `v3dlib_asset` links `v3dlib_audio` PUBLIC and `v3dlib_audio` links `soloud` PUBLIC, so the dependency propagates rather than being named again by every consumer. Pong names `v3dlib_audio` because it really does play sounds.

**Apps name neither spdlog nor fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC, so the `SPDLOG_COMPILED_LIB` definition and the spdlog/fmt link dependencies propagate to every library and app that consumes it. Every `api/` library whose sources compile [Logger.h](api/log/Logger.h) links `v3dlib_log` PUBLIC for the same reason — a target that compiles that header without the definition builds spdlog header-only and emits symbols the compiled spdlog library also defines, which surfaces as a duplicate-symbol link error in whichever app happens to pull the wrong object first. If you add an api library that logs, link `v3dlib_log`.

Check this list before assuming a build failure is yours.

## Architecture

**Two different classes named Engine.** `v3d::engine::Engine` (api/engine) is the *game* engine: main loop, window, asset manager, config, input. Each app subclasses it as `Controller`. `v3d::render::realtime::Engine` (api/render) is the *render* engine, subclassed as `Engine3D`. Apps hold both.

**A quit command calls `Engine::quit()`, never `shutdown()`.** `eventLoop` ticks and renders after an event handler returns, so a handler that tears down the window and calls `SDL_Quit` leaves the next frame drawing into a destroyed window — which surfaces as a throw out of `Presenter::acquire` on a lost surface, or as a process that keeps spinning with no window, depending on what the driver returns. `quit()` sets a flag the loop breaks on; `main` calls `shutdown()` once, after `eventLoop()` returns. **Pong and tetris still call `shutdown()` from their `quit` handlers** and have the same defect.

**Feature flags decide what exists.** `Engine::initialize(int features)` takes a bitmask of `v3d::engine::Feature` (Window, Config, KeyboardInput, MouseInput) and only constructs what was asked for. `Feature::Config` loads `data/config.json`, which must use the newer indirect form — `{"configs": [{"type": "...", "file": "..."}]}` referencing separate mappings/window/ui/sound files. Pong's `data/` is the reference; tetris and voxel were migrated to the same shape on 2026-08-31 and odyssey on 2026-09-01, and no app is on the older inline `keys` format any more. `config::Type` also carries `camera` and `layout`, which only the editor uses — its camera profile table and its viewport split, both translated from rigel's `gui.xml`. `Config::load` guards every lookup with a `contains()`, so a config it does not understand is a logged `false` rather than an exception out of engine startup. A window config is not guarded the same way: `Engine::initialize` reads `width` and `height` with `at()`, so a `window.json` naming neither throws. An app with no window config at all gets a window at the default size rather than none.

**Render pipeline.** Window → Engine3D → Context3D → Frame → Pass → DrawItem, and there is exactly one of each. An app fills a `realtime::Canvas` during its tick, hands it to `Engine3D::quads()->submit(canvas, pass)`, and calls `renderFrame()`, which records and presents. `Context3D` owns the Vulkan device, swapchain and quad renderer. The `SDL_Renderer` path — `Engine2D`, `Context2D`, `Scene2D`, `Texture2D`, `Window2D`, `Blit2DTexture` — and the `Operation` submission path it used were deleted on 2026-09-01 with odyssey's port, which was their last consumer. See [docs/RenderingPipeline.md](docs/RenderingPipeline.md).

**The Vulkan frame loop is in, and a window clears to a colour.** `realtime::Window` creates an `SDL_WINDOW_VULKAN` window and owns `vulkan::Instance` and `vulkan::Surface` — there is one window class, not a 2D and a 3D one, and one `Feature::Window` flag that asks for it; `Context3D` owns `vulkan::Device`, `vulkan::Swapchain`, `vulkan::Presenter` (command pool, per-frame command buffers, semaphores and fences, and the acquire/submit/present loop), `vulkan::PipelineCache`, `vulkan::Resources`, `vulkan::FrameUniforms`, `vulkan::Uploader` and — lazily — `vulkan::DepthBuffer`. `Engine3D::renderFrame()` records the frame it has been given and presents it, rebuilding the swapchain when acquiring or presenting reports it out of date. Drawing goes through **dynamic rendering** — there is no `VkRenderPass` and no `VkFramebuffer` — and layout transitions use synchronization2 barriers. A frame is a list of `Pass`es holding `DrawItem`s, recorded in submission order unless the pass asks to be sorted. See [docs/RenderingPipeline.md](docs/RenderingPipeline.md), which describes what exists rather than what was planned.

**What phase 5 added to the api, all of it 2026-08-31 and none of it with a 3D consumer yet.** `vulkan::DepthBuffer` is owned by `Context3D` and **allocated the first frame a pass asks for depth**, so a 2D app never pays for it; `Recorder` attaches it, clears it exactly when the pass clears colour, and transitions it from `UNDEFINED` once a frame. Because dynamic rendering matches a pipeline to its pass's attachments, `QuadRenderer` compiles its pipeline **twice** — with and without a depth format — and picks between them from `Pass::depth()`. `vulkan::PipelineBuilder` describes a pipeline a chained call at a time and is what the quad pipeline is now built through. `vulkan::FrameUniforms` is set 0 of [ADR-0008](docs/adr/0008-binding-by-update-frequency.md) — the layout every pipeline declares, plus a camera slot per pass per frame in flight that `Recorder` writes and binds. `vulkan::DeviceBuffer` and `vulkan::Mesh` are device-local geometry filled through `vulkan::Uploader`, the one-shot record/submit/wait that `TextureFactory` also uses. Sorting is `Pass::sort(true)`, opt-in because the key groups by pipeline and material within a layer and a canvas of batches has to stay in submission order. **Meshes are not `Resources` handles** — the app owns them, per [ADR-0010](docs/adr/0010-meshes-are-owned-by-the-app.md).

**There are two primitives: the batched quad and the line.** The quad is what pong and tetris both draw through. `realtime::Canvas` accumulates 2D quads on the CPU — position, uv and colour, with a batch cut only where the bound texture changes — and `vulkan::QuadRenderer` owns the two pipelines it compiles, the descriptor pool, the 1x1 white texture an untextured quad is drawn against, and a vertex and index buffer per frame in flight. A rectangle, a sprite, a glyph and a menu panel are all the same primitive ([ADR-0005](docs/adr/0005-one-batched-quad-primitive.md)). Text needs no separate path: the font library lays glyphs out and `Canvas::text` copies them in against the atlas, whose single channel is swizzled into alpha by its image view. `v3d::ui::ComponentRenderer` draws the ui onto the same canvas. **Lines are the second primitive**, added 2026-09-01 for the editor ([ADR-0011](docs/adr/0011-lines-are-the-second-primitive.md)): `realtime::LineCanvas` accumulates segments on the cpu and `vulkan::LineRenderer` draws a whole canvas as one non-indexed line list. Unlike the quad, positions are in world space and read the pass camera at set 0, and the pipeline built for a pass with depth tests and writes it. `Context3D::lines()` builds the renderer on first use, so a 2D app pays nothing. The editor is the first consumer and drew through both pipelines on 2026-09-01, validation clean.


**The swapchain is a UNORM format, not sRGB** — colour is authored in display space and written out unchanged, per [ADR-0009](docs/adr/0009-colour-authored-in-display-space.md). An `_SRGB` target encodes on write, which brightens every colour in the tree; that was the phase 2 default and it was wrong. A lit 3D scene will have to revisit this.

**A frame may submit any number of canvases, and each takes a buffer of its own.** `QuadRenderer` and `LineRenderer` keep a ring of geometry buffers per frame in flight rather than one, and `Engine3D` returns the ring to its start once the frame has been recorded. Appending several canvases into one buffer would not work: `vulkan::Buffer::grow` replaces the allocation, which invalidates the handle every draw item recorded before it is holding. Until 2026-09-01 there was one buffer per frame and a second `submit()` silently overwrote the first, which four games that each submit once never saw.

**Multiple viewports are several passes over one frame.** The editor's `Renderer` builds one `Pass` per `ViewPort`, each carrying that view's pixel region as its viewport and scissor, its own camera at set 0, and its own clear of both attachments within that region. Nothing in `Frame`, `Pass`, `Recorder` or `FrameUniforms` had to change for it — `FrameUniforms` already kept a camera slot per pass per frame in flight. `data/layout.json` is what decides the split, translated from rigel's `gui.xml` viewgroup tree.

**`v3d::type::Camera` builds Vulkan clip space** — y down, depth zero at the near plane and one at the far one — and the camera looks along its own `direction`, which is +z of the basis its three normals define. See [ADR-0012](docs/adr/0012-camera-builds-vulkan-clip-space.md). It built OpenGL projections until 2026-09-01, which nothing had noticed because nothing had ever drawn through the class; voxel carries a second camera of its own that had already made the same choice privately. `Camera::project()` and `::unproject()` are inverses of each other, which picking will need and which they were not before.

**Voxel is ported, and no app calls OpenGL any more.** As of 2026-09-01 it runs and draws terrain through Vulkan: a depth tested, sorted scene pass of its own pipeline built with `vulkan::PipelineBuilder`, one `DrawItem` per meshed chunk over a `vulkan::Mesh` the app owns per [ADR-0010](docs/adr/0010-meshes-are-owned-by-the-app.md), and a second painter ordered pass of batched quads for the debug overlay and the game menu. Its shaders are `voxel/shaders/voxel.{vert,frag}`, embedded by `v3d_add_shader`, and are the first in the tree to read set 0. See [docs/VoxelSurvey.md](docs/VoxelSurvey.md) for what it was, and the phase 5 groups in [docs/plans/Modernization.md](docs/plans/Modernization.md) for what changed.

**Odyssey runs and is ported.** As of 2026-09-01 it opens a 1280x768 window and draws one 64x64 sprite through the batched quad, at the tile its entity's `PositionFixed2D` names. That is the whole of what it draws: `Movement::tick` returns true and does nothing, and `Sprite`, `SpriteSheet`, `Actor`, `Tile` and `ui::Screen` are empty declarations. It was the last consumer of the `SDL_Renderer` path, so porting it is what let that tree and `api/gl` go.

**Images are top down, and `image::Image` row 0 is the top of the picture.** Every consumer downstream — the canvas, the texture factory, the atlas packer — reads them that way. The png reader and writer both reversed their rows until 2026-09-01, which cancelled out on a round trip and handed every *displayed* png an upside down picture; the tga writer left its origin bit clear, so its own reader turned a file it had just written over. `imagewriter_orientation_test` in [api/image/tests/ImageWriterTest.cxx](api/image/tests/ImageWriterTest.cxx) pins both, with rows that differ — the older uniform test image could not see a flip. **The jpeg reader and writer still reverse their rows** and nothing displays a jpeg, so the pair is self consistent and untested; fix them together if anything ever does.

**The Khronos validation layer is enabled when it is installed**, and `vulkan::Instance` routes its warnings and errors through the logger. Without that messenger a loaded layer is silent, which looks exactly like a clean run — so treat any earlier claim of "validation clean" that predates it as unverified.

**ECS.** entt. The `registry` lives on the app's `Controller` and is passed into the render engine as a raw `entt::registry*`. [docs/ECSDesign.md](docs/ECSDesign.md) describes the intended design, which is largely aspirational.

## Conventions

- Headers are `.h`; implementations are `.cpp` **or** `.cxx`, mixed even within a directory (`api/render/realtime/*.cpp` alongside `api/render/realtime/vulkan/*.cxx`). Match the immediate neighbours.
- Namespaces mirror the `api/` path: `v3d::asset`, `v3d::render::realtime`, `v3d::render::realtime::vulkan`. Closed with `};  // namespace <full name>` — the trailing semicolon is part of the style.
- 4-space indent; access specifiers indented one space into the class body (` public:`, ` private:`).
- `boost::shared_ptr` / `boost::make_shared` throughout, not the `std` equivalents.
- Doc comments are `/** **/` blocks, frequently left empty above trivial members.
- **Comments explain the code, not the change.** No history ("this used to", "the block that was here"), no
  justification for why a commit exists, no roadmap for a later phase. Why a decision was made belongs in
  [docs/adr/](docs/adr/) and what is coming belongs in [docs/plans/](docs/plans/) — a comment that repeats either
  goes stale where nobody is looking. A non-obvious invariant, a trap, or a constraint the code satisfies is
  exactly what a comment is for.
- Logging is spdlog through the wrapper: `logger_->get()->info("... {}", value)`. The older `LOG_INFO`/`LOG_ERROR` macros survive only in commented-out or non-compiling code — don't add new uses.
- [.gitattributes](.gitattributes) enforces LF (`* text=auto eol=lf`). Editors that save CRLF turn a small change into a whole-file diff; strip the CRs rather than committing them.
