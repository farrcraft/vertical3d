# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A monorepo for the Vertical3D ecosystem: reusable C++ libraries under `api/` (targets named `v3dlib_*`, one per subdirectory) plus the apps that consume them at the top level — pong, tetris, voxel, odyssey, talyn, moya, imagetool, v3dshell.

Much of this code traces back to the early 2000s and is being modernised incrementally: C++17+, granular namespaces, CMake replacing autotools and VS solutions. Expect wide variance in how modern any given file is.

`v3dlibs/`, `rigel/`, `luxa/`, `vault/` and `vertical3d/` are absent from the root `add_subdirectory` list, excluded from lint, and not built — but "not built" does not mean disposable, and they are four different situations:

- `vertical3d/` is the desktop 3D editing app the repo is named after, to be rewritten onto the new api. Not dead code.
- `rigel/` is the earlier prototype of that app; its viewport, manipulator and modelling-command work has to be folded into the rewrite before it can go.
- `luxa/` and `v3dlibs/` are migrations in progress, and both audits are now written up: [docs/LuxaAudit.md](docs/LuxaAudit.md) and [docs/V3dlibsAudit.md](docs/V3dlibsAudit.md). Each lists what has to land before its tree can be deleted, plus the `api/` regressions the audit turned up — the most consequential being that `Menu::activate()` dispatches nothing and `api/input/Mouse::handleEvent` swallows every mouse event. Read the relevant one before deleting anything from either tree.
- `vault/quantumxml` is genuinely archived, superseded by the JSON config work.

MSVC/Windows only in practice. The root CMakeLists passes `/std:c++latest` and `/permissive-` unconditionally, and targets set `/EHsc` and `/utf-8` individually.

## Process

[docs/sdlc.md](docs/sdlc.md) describes how work moves through the repo — where plans live,
when a decision earns an ADR, and what "verified" currently means. Decisions are recorded in
[docs/adr/](docs/adr/), indexed in its README — seven of them cover the Vulkan rewrite and
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
- Editing `vcpkg.json` re-runs the manifest install. A cold install builds boost from source and takes roughly 45 minutes.
- **`sdl3` is requested with its `vulkan` feature**, and has to be. Without it SDL builds with `SDL_VULKAN=OFF`, the windows video driver leaves `Vulkan_LoadLibrary` unset, and `SDL_Vulkan_LoadLibrary` fails with "No dynamic Vulkan support in current SDL video driver (windows)" — which surfaces as an unhandled exception on startup, not as a build failure. Changing it rebuilds SDL only, about five minutes.
- **Never delete `out/build/<config>/vcpkg_installed/`** — that directory *is* the dependency install. When CMake needs a fresh cache (typically after a VS toolset update leaves the cached `CMAKE_CXX_COMPILER` path pointing at a version that no longer exists), delete only `CMakeCache.txt`, `CMakeFiles/`, `build.ninja`, `cmake_install.cmake` and `.ninja_*`, then reconfigure. Reconfiguring is fast; reinstalling is not.
- `vendor/libnoise` and `vendor/soloud` are git submodules built separately, not by this project. `link_directories` expects their artefacts under `vendor/*/Debug`.
  soloud is prebuilt in the tree; libnoise is not, and `voxel` will not link without it. Its committed `CMakeCache.txt` names a
  "Visual Studio 17 2022" generator that no longer exists here, so configure it out of source instead of reusing that cache:
  `cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug`. The
  `CMAKE_POLICY_VERSION_MINIMUM` is needed because its `cmake_minimum_required(VERSION 3.0)` predates what current CMake accepts.
- **Assets shared by more than one app live in the root [data/](data/)**, committed once. `v3d_add_shared_data(<target>)` — defined in the root CMakeLists — copies them next to that app's executable after it links, merging into whatever the app keeps in `<app>/data`. Currently only fonts, used by pong and voxel. Per-app data is *not* copied by CMake: the `data/` directories in `out/build/<config>/<app>/` are manual copies and are years stale, so editing `pong/data/*.json` does not affect a run from the build tree until you copy it across yourself.
- `VCPKG_ROOT` in CMakeSettings.json has a doubled path segment (`vertical3d/vertical3d/vendor/vcpkg`) and points nowhere. vcpkg works through the toolchain file regardless.

## Lint

```
cpplint --linelength=180 --filter=-runtime/indentation_namespace,-build/namespaces_literals \
  --exclude=vault --exclude=voxel/src/noise --exclude=v3dlibs --exclude=rigel --exclude=luxa --recursive .
```

Run in CI by [.github/workflows/cpplint.yml](.github/workflows/cpplint.yml). Current cpplint renamed the namespace-indent check to `whitespace/indent_namespace`, so the `-runtime/indentation_namespace` filter no longer suppresses it and **every file in the repo reports it**. Ignore those; treat anything else as a real finding.

## Tests

Boost.Test, one binary per api library, built from `api/<lib>/tests/` and registered with ctest:

```
ninja -C out/build/x64-Debug                       # tests build with everything else
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite
out/build/x64-Debug/api/image/tests/v3dtest_image.exe --run_test=texture_test
```

`v3d_add_test(<lib> <sources>)` in the root CMakeLists builds `v3dtest_<lib>`, links the framework, and adds the ctest entry with the working directory set beside the executable so a suite's data files resolve. Link the library under test yourself in `api/<lib>/tests/CMakeLists.txt`. `TestMain` (one per target) carries the `BOOST_TEST_MODULE` define and nothing else.

Covered as of 2026-08-31: `type`, `brep`, `image`, `font`, `input`, `event`, `render` — 63 cases, migrated out of `v3dlibs/tests/` (the command-layer tests were rewritten against `api/event`, and the two input tests against `Keyboard`/`Mouse`). `render` covers only the parts that need neither a window nor a GPU — the frame and pass model, the draw item's sort key, and the handle registry. Not covered: `asset`, `config`, `dag`, `ecs`, `audio`, `log`, `ui`, and everything in `api/render` below the recorder — that needs a window and a GPU, so it waits on [ADR 0007](docs/adr/0007-ci-rendering-tests.md). `moya/tests/` and `tetris/tests/` still build nothing; `pong/run-unit-tests.sh` and `tetris/run-unit-tests.sh` still invoke a `unit_tests` binary that does not exist. Boost.Test's leak check reports a permanent false positive for any suite that builds a `Logger` (spdlog's registry outlives the report), which is why `add_test` passes `--detect_memory_leaks=0`.

## Build health

Everything compiles and links as of 2026-08-31.

- **Clean:** every `api/` library, plus `pong`, `tetris`, `voxel`, `odyssey`, `talyn`, `v3dshell`, `imagetool`.
- **`voxel`** was the last holdout — it had drifted behind API changes in the shared libraries, and its
  `target_link_libraries` still named only `libnoise`, so it could not have linked even once the objects
  compiled. It now links the same `v3dlib_*` set as tetris. `src/game/Player.cxx` and
  `src/noise/noiseutils.cpp` were also missing from the target's source list.

**Apps name neither spdlog nor fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC, so the `SPDLOG_COMPILED_LIB` definition and the spdlog/fmt link dependencies propagate to every library and app that consumes it. Every `api/` library whose sources compile [Logger.h](api/log/Logger.h) links `v3dlib_log` PUBLIC for the same reason — a target that compiles that header without the definition builds spdlog header-only and emits symbols the compiled spdlog library also defines, which surfaces as a duplicate-symbol link error in whichever app happens to pull the wrong object first. If you add an api library that logs, link `v3dlib_log`.

Check this list before assuming a build failure is yours.

## Architecture

**Two different classes named Engine.** `v3d::engine::Engine` (api/engine) is the *game* engine: main loop, window, asset manager, config, input. Each app subclasses it as `Controller`. `v3d::render::realtime::Engine` (api/render) is the *render* engine, subclassed as `Engine2D`/`Engine3D`. Apps hold both.

**Feature flags decide what exists.** `Engine::initialize(int features)` takes a bitmask of `v3d::engine::Feature` (Window2D, Window3D, Config, KeyboardInput, MouseInput) and only constructs what was asked for. `Feature::Config` loads `data/config.json`, which must use the newer indirect form — `{"configs": [{"type": "...", "file": "..."}]}` referencing separate mappings/window/ui/sound files. Pong's `data/` is the reference; tetris was migrated to the same shape on 2026-08-31. No app still uses the older inline `keys`/`menu` format, which `Config::load` rejects.

**Render pipeline.** Window → Engine2D/Engine3D → Context → Frame. Each frame the app builds a `Frame` from the engine's `Context`, adds `Operation`s to it, and calls `draw()`; operations are collected during the tick and executed in that final step. `v3d::gl::Canvas` accumulates 2D primitives into a vertex buffer that `operation::Canvas` uploads and draws — note its vertices carry position and rgba only, with **no texture coordinates**, so anything textured needs a different operation (`operation::GLTexturedQuad`) or an extension to Canvas. `Context2D` wraps an `SDL_Renderer`; `Context3D` owns the Vulkan device and swapchain. See [docs/RenderingPipeline.md](docs/RenderingPipeline.md).

**The Vulkan frame loop is in, and a window clears to a colour.** `Window3D` creates an `SDL_WINDOW_VULKAN` window and owns `vulkan::Instance` and `vulkan::Surface`; `Context3D` owns `vulkan::Device`, `vulkan::Swapchain`, `vulkan::Presenter` (command pool, per-frame command buffers, semaphores and fences, and the acquire/submit/present loop), `vulkan::PipelineCache` and `vulkan::Resources`. `Engine3D::renderFrame()` records the frame it has been given and presents it, rebuilding the swapchain when acquiring or presenting reports it out of date. Drawing goes through **dynamic rendering** — there is no `VkRenderPass` and no `VkFramebuffer` — and layout transitions use synchronization2 barriers. A frame is a list of `Pass`es holding `DrawItem`s; the recorder walks them in submission order and nothing sorts yet. See [docs/RenderingPipeline.md](docs/RenderingPipeline.md), which describes what exists rather than what was planned.

**No pipeline exists yet, so nothing but the clear is drawn.** Every app renderer still calls OpenGL against a context nothing creates, and the first real pipeline — the batched quad of [ADR-0005](docs/adr/0005-one-batched-quad-primitive.md) — lands with pong's port. The old GL setup in `Window3D::create` is commented out rather than deleted, kept as reference for what the Vulkan path still has to replace.

**ECS.** entt. The `registry` lives on the app's `Controller` and is passed into the render engine as a raw `entt::registry*`. [docs/ECSDesign.md](docs/ECSDesign.md) describes the intended design, which is largely aspirational.

## Conventions

- Headers are `.h`; implementations are `.cpp` **or** `.cxx`, mixed even within a directory (`api/render/realtime/*.cpp` alongside `api/render/realtime/vulkan/*.cxx`). Match the immediate neighbours.
- Namespaces mirror the `api/` path: `v3d::asset`, `v3d::render::realtime`, `v3d::render::realtime::vulkan`. Closed with `};  // namespace <full name>` — the trailing semicolon is part of the style.
- 4-space indent; access specifiers indented one space into the class body (` public:`, ` private:`).
- `boost::shared_ptr` / `boost::make_shared` throughout, not the `std` equivalents.
- Doc comments are `/** **/` blocks, frequently left empty above trivial members.
- Logging is spdlog through the wrapper: `logger_->get()->info("... {}", value)`. The older `LOG_INFO`/`LOG_ERROR` macros survive only in commented-out or non-compiling code — don't add new uses.
- [.gitattributes](.gitattributes) enforces LF (`* text=auto eol=lf`). Editors that save CRLF turn a small change into a whole-file diff; strip the CRs rather than committing them.
