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
- **Never delete `out/build/<config>/vcpkg_installed/`** — that directory *is* the dependency install. When CMake needs a fresh cache (typically after a VS toolset update leaves the cached `CMAKE_CXX_COMPILER` path pointing at a version that no longer exists), delete only `CMakeCache.txt`, `CMakeFiles/`, `build.ninja`, `cmake_install.cmake` and `.ninja_*`, then reconfigure. Reconfiguring is fast; reinstalling is not.
- `vendor/libnoise` and `vendor/soloud` are git submodules built separately, not by this project. `link_directories` expects their artefacts under `vendor/*/Debug`.
- **Assets shared by more than one app live in the root [data/](data/)**, committed once. `v3d_add_shared_data(<target>)` — defined in the root CMakeLists — copies them next to that app's executable after it links, merging into whatever the app keeps in `<app>/data`. Currently only fonts, used by pong and voxel. Per-app data is *not* copied by CMake: the `data/` directories in `out/build/<config>/<app>/` are manual copies and are years stale, so editing `pong/data/*.json` does not affect a run from the build tree until you copy it across yourself.
- `VCPKG_ROOT` in CMakeSettings.json has a doubled path segment (`vertical3d/vertical3d/vendor/vcpkg`) and points nowhere. vcpkg works through the toolchain file regardless.

## Lint

```
cpplint --linelength=180 --filter=-runtime/indentation_namespace,-build/namespaces_literals \
  --exclude=vault --exclude=voxel/src/noise --exclude=v3dlibs --exclude=rigel --exclude=luxa --recursive .
```

Run in CI by [.github/workflows/cpplint.yml](.github/workflows/cpplint.yml). Current cpplint renamed the namespace-indent check to `whitespace/indent_namespace`, so the `-runtime/indentation_namespace` filter no longer suppresses it and **every file in the repo reports it**. Ignore those; treat anything else as a real finding.

## Tests

There is no working test suite. Boost.Test sources exist under `v3dlibs/tests/`, and `pong/run-unit-tests.sh` / `tetris/run-unit-tests.sh` invoke a `unit_tests` binary — but no CMakeLists builds one, and `v3dlibs/` is not in the build. Don't offer a test command; verify changes by building.

## Build health

Not everything compiles. Failing objects: three in `voxel`. Everything else builds.

- **Clean:** every `api/` library, plus `pong`, `tetris`, `odyssey`, `talyn`, `v3dshell`, `imagetool`.
- **`voxel`** — drifted behind API changes in the shared libraries (undeclared identifiers, calls to methods that no longer exist).

**Apps name neither spdlog nor fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC, so the `SPDLOG_COMPILED_LIB` definition and the spdlog/fmt link dependencies propagate to every library and app that consumes it. Every `api/` library whose sources compile [Logger.h](api/log/Logger.h) links `v3dlib_log` PUBLIC for the same reason — a target that compiles that header without the definition builds spdlog header-only and emits symbols the compiled spdlog library also defines, which surfaces as a duplicate-symbol link error in whichever app happens to pull the wrong object first. If you add an api library that logs, link `v3dlib_log`.

Check this list before assuming a build failure is yours.

## Architecture

**Two different classes named Engine.** `v3d::engine::Engine` (api/engine) is the *game* engine: main loop, window, asset manager, config, input. Each app subclasses it as `Controller`. `v3d::render::realtime::Engine` (api/render) is the *render* engine, subclassed as `Engine2D`/`Engine3D`. Apps hold both.

**Feature flags decide what exists.** `Engine::initialize(int features)` takes a bitmask of `v3d::engine::Feature` (Window2D, Window3D, Config, KeyboardInput, MouseInput) and only constructs what was asked for. `Feature::Config` loads `data/config.json`, which must use the newer indirect form — `{"configs": [{"type": "...", "file": "..."}]}` referencing separate mappings/window/ui/sound files. Pong's `data/` is the reference. Tetris still has the older inline `keys`/`menu` format, which `Config::load` rejects.

**Render pipeline.** Window → Engine2D/Engine3D → Context → Frame. Each frame the app builds a `Frame` from the engine's `Context`, adds `Operation`s to it, and calls `draw()`; operations are collected during the tick and executed in that final step. `v3d::gl::Canvas` accumulates 2D primitives into a vertex buffer that `operation::Canvas` uploads and draws — note its vertices carry position and rgba only, with **no texture coordinates**, so anything textured needs a different operation (`operation::GLTexturedQuad`) or an extension to Canvas. `Context2D` wraps an `SDL_Renderer`; `Context3D` owns the Vulkan device and swapchain. See [docs/RenderingPipeline.md](docs/RenderingPipeline.md).

**The Vulkan port is mid-flight, and nothing currently renders.** `Window3D` creates an `SDL_WINDOW_VULKAN` window and owns `vulkan::Instance` and `vulkan::Surface`; `Context3D` owns `vulkan::Device` and `vulkan::Swapchain`. Still missing: render pass, framebuffers, command pool and buffers, and the acquire/submit/present loop. Meanwhile `Engine3D::renderFrame()` and every app renderer still call OpenGL, but no GL context is created any more — so pong links yet has nothing to draw into. Treat "it builds" as the only available signal until the frame loop lands. The old GL setup in `Window3D::create` is commented out rather than deleted, kept as reference for what the Vulkan path still has to replace.

**ECS.** entt. The `registry` lives on the app's `Controller` and is passed into the render engine as a raw `entt::registry*`. [docs/ECSDesign.md](docs/ECSDesign.md) describes the intended design, which is largely aspirational.

## Conventions

- Headers are `.h`; implementations are `.cpp` **or** `.cxx`, mixed even within a directory (`api/render/realtime/*.cpp` alongside `api/render/realtime/vulkan/*.cxx`). Match the immediate neighbours.
- Namespaces mirror the `api/` path: `v3d::asset`, `v3d::render::realtime`, `v3d::render::realtime::vulkan`. Closed with `};  // namespace <full name>` — the trailing semicolon is part of the style.
- 4-space indent; access specifiers indented one space into the class body (` public:`, ` private:`).
- `boost::shared_ptr` / `boost::make_shared` throughout, not the `std` equivalents.
- Doc comments are `/** **/` blocks, frequently left empty above trivial members.
- Logging is spdlog through the wrapper: `logger_->get()->info("... {}", value)`. The older `LOG_INFO`/`LOG_ERROR` macros survive only in commented-out or non-compiling code — don't add new uses.
- [.gitattributes](.gitattributes) enforces LF (`* text=auto eol=lf`). Editors that save CRLF turn a small change into a whole-file diff; strip the CRs rather than committing them.
