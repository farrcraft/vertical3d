# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A monorepo for the Vertical3D ecosystem: reusable C++ libraries under `api/` (targets named `v3dlib_*`,
one per subdirectory) plus the apps that consume them at the top level — pong, tetris, voxel, odyssey,
vertical3d, talyn, moya, imagetool, v3dshell. Every directory in the root builds.

Much of this code traces back to the early 2000s and is being modernised incrementally: C++17+, granular
namespaces, CMake replacing autotools and VS solutions. Expect wide variance in how modern any given file
is. MSVC/Windows only in practice; the root CMakeLists sets `CMAKE_CXX_STANDARD 23` — which CMake maps
to `/std:c++latest` here, and which is stated as a standard rather than as the flag because glm and EnTT
require `cxx_std_17` through their interfaces and MSVC reports D9025 for a command line naming two — plus
`/permissive-` and `/W4` unconditionally. Targets set `/EHsc` and `/utf-8` individually.

The legacy trees — `vault/`, `rigel/`, `luxa/`, `v3dlibs/` — are all deleted. [docs/audits/](docs/audits/)
is the only account of what they held and carries the `git show` incantation to recover a file from each.
Read [RigelSurvey.md](docs/audits/completed/RigelSurvey.md) before writing off a rigel feature as covered.

`vertical3d/` is the editor: four viewports of a scene over a construction grid, with manipulators, undo,
a command directory, menus, two toolbars and project persistence. Its sources group into `src/view/`,
`src/scene/`, `src/command/`, `src/tool/` and `src/manipulator/`, leaving `Controller`, `Renderer` and
`main` at the root of `src/` as the app shell; `tests/` mirrors those five. Includes are relative, so a
file under one of them reaches the api as `../../../api/`.

## Process

[docs/sdlc.md](docs/sdlc.md) describes how work moves through the repo — where plans live, when a decision
earns an ADR, and what "verified" currently means. Decisions are in [docs/adr/](docs/adr/), indexed in its
README; most of the first twelve cover the Vulkan rewrite and are worth reading before touching `api/render`.
Plans live in [docs/plans/](docs/plans/), surveys and audits in [docs/audits/](docs/audits/), roadmaps for
areas nobody has taken up in [docs/roadmap/](docs/roadmap/), and unphased work in
[docs/TODO.md](docs/TODO.md).
[docs/plans/OfflineRenderingPhase3.md](docs/plans/OfflineRenderingPhase3.md) is the open plan, taking up
light and surface in the two offline renderers as a shading language.

## Build

Visual Studio generates `out/build/x64-Debug` from [CMakeSettings.json](CMakeSettings.json). From a shell,
use a Developer environment (`vcvars64.bat`), then:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
```

- `/utf-8` is required, not cosmetic: spdlog's bundled fmt has a `static_assert` that fails without it.
- **Never delete `out/build/<config>/vcpkg_installed/`** — that directory *is* the dependency install.
  When CMake needs a fresh cache (typically after a VS toolset update leaves the cached
  `CMAKE_CXX_COMPILER` pointing at a version that no longer exists), delete only `CMakeCache.txt`,
  `CMakeFiles/`, `build.ninja`, `cmake_install.cmake` and `.ninja_*`, then reconfigure. Reconfiguring is
  fast; reinstalling is not.
- **Shaders are compiled at build time and embedded, not shipped as data.** `v3d_add_shader(<target>
  <source>)` runs the Vulkan SDK's `glslc` over a GLSL file and writes SPIR-V as a C initialiser list into
  `<binary dir>/shaders/<name>.inc`, which the source `#include`s into a `uint32_t` array. The engine's
  shaders are in [api/render/shaders/](api/render/shaders/). `VULKAN_SDK` must point at an SDK install or
  the first call to that function fails with "glslc was not found".
- **`sdl3` is requested with its `vulkan` feature**, and has to be. Without it SDL builds with
  `SDL_VULKAN=OFF` and `SDL_Vulkan_LoadLibrary` fails with "No dynamic Vulkan support in current SDL video
  driver (windows)" — which surfaces as an unhandled exception on startup, not as a build failure.
- **The vcpkg baseline is pinned in [vcpkg-configuration.json](vcpkg-configuration.json)**, not in
  `vcpkg.json`, and it is a commit of microsoft/vcpkg rather than the `vendor/vcpkg` ports tree on disk. A
  port missing from that commit fails with "the baseline does not contain an entry for port X" even when
  `vendor/vcpkg/ports/X` exists.
- Editing `vcpkg.json` re-runs the manifest install; a cold install builds boost from source and takes
  roughly 45 minutes. Changing the sdl3 feature set rebuilds SDL only, about five minutes.
- **boost 1.91 removed `boost::json::error_code` and `boost::json::system_error`.** Name `boost::system`
  and include `<boost/system/error_code.hpp>` and `<boost/system/system_error.hpp>` directly.
  [api/asset/JsonFile.h](api/asset/JsonFile.h) is where the tree's json error handling lives.
- `vendor/libnoise` is the only submodule, is not prebuilt, and `voxel` will not link without it. Build it
  out of source — see [docs/Dependencies.md](docs/Dependencies.md#building-libnoise); voxel's
  `target_link_directories` expects its artefacts under `vendor/libnoise/Debug`.
- **Assets shared by more than one app live in the root [data/](data/)**, copied next to an executable by
  `v3d_add_shared_data(<target>)`. An app's own `data/` is copied by `v3d_add_app_data(<target>)`, but
  **only tetris, voxel, odyssey and talyn call it** — everywhere else the `data/` under
  `out/build/<config>/<app>/` is a stale manual copy, so editing `pong/data/*.json` does not affect a run
  from the build tree until you copy it across.
- `VCPKG_ROOT` in CMakeSettings.json has a doubled path segment and points nowhere. vcpkg works through
  the toolchain file regardless.
- **The root is three files.** [cmake/v3dDependencies.cmake](cmake/v3dDependencies.cmake) holds every
  `find_package`, [cmake/v3dHelpers.cmake](cmake/v3dHelpers.cmake) the five `v3d_add_*` functions, and
  [CMakeLists.txt](CMakeLists.txt) the options and the subdirectory list. Paths into this repository go
  through `V3D_ROOT`, never `CMAKE_SOURCE_DIR`, which names the consumer's root in a nested build.
- **`V3D_BUILD_APPS` and `V3D_BUILD_TESTS` gate everything that is not the api**, and default to whether
  this project is the top level one. The tests guard is on each `add_subdirectory("tests")` rather than
  inside `v3d_add_test`, because a `tests/CMakeLists.txt` names its target again after calling it.
- **An application in another repository takes this one as source**, nested with `add_subdirectory`.
  [docs/NewProject.md](docs/NewProject.md) is the walkthrough and
  [docs/examples/starter/](docs/examples/starter/) is a working app that CI builds on every push. It is
  the only thing in the tree that can catch an api library relying on a global the root sets or on an
  app naming every library — which is how `api/engine` and `api/render` were found not declaring the
  api libraries they use.

### Linking rules

Per [ADR-0027](docs/adr/0027-the-api-is-consumed-as-source.md), an api library carries its own
dependencies — **including the other api libraries it uses**, so `v3dlib_engine` brings asset, config,
event, input and render with it. An app names the `v3dlib_*` targets it uses and nothing else, except
where the app itself uses a package directly — `Boost::program_options` in the three that parse a
command line. A library declared with `v3d_add_api_library` gets the include root, the `v3d::` alias an
external consumer links, `/EHsc` and `/utf-8` in its interface, and the boost winapi definitions. Adding a third-party package to an api
library means naming it PUBLIC when a header of that library names its types and PRIVATE otherwise.

- **Apps name neither spdlog nor fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC so the
  `SPDLOG_COMPILED_LIB` definition propagates. Every `api/` library whose sources compile
  [Logger.h](api/log/Logger.h) must link `v3dlib_log` PUBLIC for the same reason — without the definition
  it builds spdlog header-only and emits symbols the compiled library also defines, which surfaces as a
  duplicate-symbol link error in whichever app pulls the wrong object first.
- **Apps name neither the mixer nor `v3dlib_audio` unless they play a sound.** `v3dlib_asset` links
  `v3dlib_audio` PUBLIC and `v3dlib_audio` links `SDL3_mixer::SDL3_mixer` PUBLIC, so it propagates.
- **There is no OpenGL in the tree.** A target naming `OpenGL::GL`, `GLEW::GLEW` or `v3dlib_gl` will not
  configure.
- **glm and EnTT have to be linked, not assumed.** They resolved for years without a `find_package`,
  because `Boost_INCLUDE_DIRS` is the vcpkg installed include directory and the root put it on every
  target's include path. That line is gone; `glm::glm` and `EnTT::EnTT` are named by the libraries whose
  headers use them.

## Lint

```
cpplint --linelength=180 \
  --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=voxel/src/noise --recursive .
```

Run in CI by [.github/workflows/cpplint.yml](.github/workflows/cpplint.yml). **The tree is clean at
it** — every finding is a real one. The excludes matter only locally — CI checks out no submodules and
installs no ports — but a developer machine has all three trees, and `vcpkg_installed/` alone holds
80,000-odd third party headers.

**Nothing is suppressed** — there is no `--filter` at all. Keep it that way: cpplint accepts a name it
does not know and then silently suppresses nothing, so a filter entry that stops working looks exactly
like a tree that started failing.

## Tests

Boost.Test, one binary per api library from `api/<lib>/tests/`, plus one per app where the app has logic
worth covering. Registered with ctest and run in CI by
[.github/workflows/ctest.yml](.github/workflows/ctest.yml).

```
ninja -C out/build/x64-Debug                       # tests build with everything else
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite
out/build/x64-Debug/api/image/tests/v3dtest_image.exe --run_test=texture_test
```

`v3d_add_test(<lib> <sources>)` builds `v3dtest_<lib>`, links the framework, and adds the ctest entry with
the working directory beside the executable so a suite's fixtures resolve. Link the library under test
yourself in `api/<lib>/tests/CMakeLists.txt`. `TestMain` carries the `BOOST_TEST_MODULE` define and
nothing else.

- Everything is covered except `api/render` below the recorder, `Feature::Window`,
  `audio::Engine::initialize()` and `ui::TextRenderer` — those need a window, a GPU or a sound device,
  and wait on [ADR-0007](docs/adr/0007-ci-rendering-tests.md). `ctest -N` lists what exists; the test sources are the
  record of what each suite asserts.
- **The moya and talyn suites each render against a committed PNG** — `moya/tests/data/` and
  `talyn/tests/data/` — compared with `image::compare`, which reports the worst pixel and by how much
  rather than only that two images differ. A failing case, or a missing reference, writes what it
  rendered to `data_out/` beside the executable, which is also how a reference is regenerated when a
  change is meant to alter the picture. **Each PNG has a `.rib` beside it describing the same scene**,
  so the file path and the code path are pinned to one picture and a divergence between them fails.
- The api libraries are testable without a window because `ComponentRenderer` takes text measuring and
  writing as callbacks rather than depending on the font library, and because a strip is hit tested
  against the bounds a draw left on it per ADR-0019. Keep that seam when adding to `ui` or `render`.
- Voxel's suite must link `libnoise`: `Chunk` is built against a `TerrainMap`, and the vtable of the flat
  one a test supplies refers to the perlin implementation whether or not a case generates noise.
- `add_test` passes `--detect_memory_leaks=0` because Boost.Test reports a permanent false positive for
  any suite that builds a `Logger` — spdlog's registry outlives the report.
- **A round trip cannot see a symmetric orientation fault**: a writer and a reader that both reverse their
  rows return the image they were given. `imagewriter_jpeg_orientation_test` goes through libjpeg directly
  on one side of each check for exactly this reason.

## Architecture

**Two different classes named Engine.** `v3d::engine::Engine` (api/engine) is the *game* engine: main
loop, window, asset manager, config, input. Each app subclasses it as `Controller`.
`v3d::render::realtime::Engine` (api/render) is the *render* engine, subclassed as `Engine3D`. Apps hold
both.

**The shell around a game is the api's, per [ADR-0028](docs/adr/0028-an-apps-shell-belongs-to-the-api.md)**,
so an app carries only what makes it that game. `v3d::engine::run<T>(argv[0], "<name>")` *is* an app's
`main` — it derives the app path, runs initialize/eventLoop inside the try that logs what a renderer threw,
and shuts down outside it. `v3d::ui::TextRenderer` owns the font, the atlas and the glyphs, and hands
`ComponentRenderer` the `measure()`/`write()` pair ADR-0019 keeps it built from. `v3d::ui::GameMenu` is the
menu the escape key puts up, holding the pause as a `Suspend` callback. `Engine3D::beginFrame` is the
minimized-window rule a `draw()` opens with. An app that reimplements one of these has diverged, not
customised.

**Render pipeline.** Window → Engine3D → Context3D → Frame → Pass → DrawItem, one of each. An app fills a
`realtime::Canvas` during its tick, submits it, and calls `renderFrame()`. Drawing goes through dynamic
rendering — no `VkRenderPass`, no `VkFramebuffer`. [docs/RenderingPipeline.md](docs/RenderingPipeline.md)
describes the whole chain and is the reference for anything below this line.

**There are two primitives**: the batched quad ([ADR-0005](docs/adr/0005-one-batched-quad-primitive.md))
and the line ([ADR-0011](docs/adr/0011-lines-are-the-second-primitive.md)). A rectangle, a sprite, a glyph
and a menu panel are all the quad; text needs no separate path, and `ui::ComponentRenderer` draws onto the
same canvas. Lines are world space and read the pass camera at set 0.

**ECS.** entt. The `registry` lives on the app's `Controller` and is passed into the render engine as a
raw `entt::registry*`. [docs/ECSDesign.md](docs/ECSDesign.md) is largely aspirational.

### The offline renderers

`talyn` (raytracer) and `moya` (reyes, behind the RenderMan interface) are the *other* renderers, and
share nothing with the realtime stack. Each is a library, a driver and a suite —
`talyn/libtalyn` + `talyn/talyn` + `talyn/tests`, and the same three for moya — per
[ADR-0022](docs/adr/0022-offline-rendering-shares-an-api-library.md), with what they share in
`api/render/offline` (`v3dlib_render_offline`, namespace `v3d::render::offline`). That library **names
neither Vulkan nor SDL**, and neither renderer touches a window, a device or a swapchain, which is what
lets their suites render in CI where everything below the recorder in `api/render` cannot.
[docs/roadmap/OfflineRendering.md](docs/roadmap/OfflineRendering.md) is the account of where they stand.

- **`api/render/CMakeLists.txt` adds `offline` below its `set(CMAKE_CXX_FLAGS "/utf-8")` line**, where
  `tests` already is. A subdirectory added above it does not inherit the flag.
- **The plane count is not the channel count.** `offline::FrameBuffer` is a stack of float planes;
  `image(channels)` takes the leading planes as the picture and leaves the rest to the renderer.
  moya's are RGB plus a depth, named by `moya::FrameBuffer::Plane`; talyn's four are RGBA.
- **moya's raster space counts y downward from the upper left**, which is RI's convention and
  `image::Image`'s row order. `raster * screen` is the composition — a matrix applies to what is on its
  right — and reversing either would write a correct render upside down or in eye units.
- **A `RenderContext` writes a file only when `RiDisplay` named one with type `"file"`.** The RI token
  table in `RenderMan.cxx` is `RtToken`, i.e. pointers, and most of it is still uninitialised — a null
  one reaches the context as an empty string rather than as an error.
- **RIB is read by one reader for both renderers**, `offline::RIBReader`, dispatching onto
  `offline::RIBHandler` — a C++ interface with typed parameter lists, not the RI C ABI, per
  [ADR-0025](docs/adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md). Each renderer
  implements it as `<renderer>::RIBHandler`. **Every method has an empty body rather than being pure
  virtual**, because the RI standard asks a renderer to accept a request it does not support — so a
  misspelled override is silent, and every override carries `override`.
- **A matrix crosses the RIB boundary by being read in order, not transposed.** RIB writes row major
  under a row vector convention and glm stores column major under a column vector one, so
  `glm::make_mat4` over the sixteen floats *is* the conversion. This applies to `Transform`,
  `ConcatTransform`, `RtMatrix` and the editor's export alike.
- **A RIB `Polygon` carries no vertex count** — it is the length of `"P"`, which the reader divides
  out. So a parameter list is parsed before the count is known, and an unbracketed varying or vertex
  parameter ends the parse rather than being guessed at.
- **moya's world-to-camera transform applies as it stands.** `prepareWorld` saves the current
  transformation as the camera coordinate system, and by the RI standard that transformation *is* the
  world to camera one. A transpose and an inverse are each right only when it is a rotation.
- **talyn refuses a camera `type::CameraProfile` cannot hold** — an off centre `ScreenWindow`, a
  non-rigid matrix, or one that reverses handedness, which is what RI's camera basis is for a general
  lookat. `RIBHandler::error()` says which; the reader still succeeds, because the request was
  understood.
- **A `ReyesPrimitive` carries the transform and colour it was submitted under.** Splitting resubmits
  pieces through the first pass during the second one, when neither is current, and a split builds its
  pieces from intersection points that carry no colour at all.

### Invariants that bite

- **A quit command calls `Engine::quit()`, never `shutdown()`.** `eventLoop` ticks and renders after a
  handler returns, so tearing the window down inside one leaves the next frame drawing into a destroyed
  window. `quit()` sets a flag the loop breaks on; `main` calls `shutdown()` after `eventLoop()` returns.
- **An app's `shutdown()` must tear its renderer down before the base class runs.** The context owns the
  device that holds the window's surface alive, and `Window::destroy()` unloads the vulkan library — a
  surface released after it is never destroyed, and the instance reports it leaked.
- **Feature flags decide what exists.** `Engine::initialize(int features)` takes a bitmask of
  `v3d::engine::Feature` and only constructs what was asked for. `Feature::Config` loads
  `data/config.json`, which must use the indirect form — `{"configs": [{"type": "...", "file": "..."}]}`.
  Pong's `data/` is the reference. `Config::load` and `registerEventMappings` guard every lookup and log a
  `false`, but **a window config is not guarded**: `initialize` reads `width` and `height` with `at()`, so
  a `window.json` naming neither throws.
- **A frame may submit any number of canvases, and each takes a buffer of its own.** Appending into one
  buffer would not work: `vulkan::Buffer::grow` replaces the allocation, invalidating the handle every
  draw item recorded before it is holding.
- **The swapchain is UNORM, not sRGB** — colour is authored in display space
  ([ADR-0009](docs/adr/0009-colour-authored-in-display-space.md)). A lit 3D scene will revisit this.
- **`v3d::type::Camera` builds Vulkan clip space**, and `project()`/`unproject()` are inverses
  ([ADR-0012](docs/adr/0012-camera-builds-vulkan-clip-space.md)).
- **Meshes are owned by the app**, not by `Resources`
  ([ADR-0010](docs/adr/0010-meshes-are-owned-by-the-app.md)).
- **`image::Image` row 0 is the top of the picture.** Every consumer downstream — the canvas, the texture
  factory, the atlas packer — reads them that way. The jpeg reader also asks the decoder for RGB whatever
  the file holds, because it builds a 24 bit `Image` and copies three bytes a pixel.
- **The Khronos validation layer is enabled when installed**, and `vulkan::Instance` routes it through the
  logger. Without that messenger a loaded layer is silent, which looks exactly like a clean run.

### The editor

`api/brep` holds two mesh representations: `BRep` is half-edge and is what the editor models with;
`WingedEdgeBRep` is winged-edge and has no consumer yet. `Vertex`, `Face` and `Index` are common to both.
A mesh names its own parts with `brep::Index` — one `uint32_t` for a vertex, a half edge or a face, since
all three are offsets into a `BRep`'s arrays. `INVALID_ID` is `1 << 31` and **its value is part of the
project file format**, so it cannot be changed without invalidating documents already written.

Each of these is settled by an ADR; read the record rather than inferring the rule from the code.

| Area | Record |
|---|---|
| A mesh is a dag node with a transform; the scene belongs to the editor | [ADR-0013](docs/adr/0013-mesh-is-a-dag-node.md) |
| Picking is a CPU ray cast, with screen space proximity for components | [ADR-0014](docs/adr/0014-picking-is-a-cpu-ray-cast.md) |
| Manipulators write the object transform, and are an overlay pass | [ADR-0015](docs/adr/0015-manipulators-write-the-object-transform.md) |
| Undo records what has already happened; one gesture is one command | [ADR-0016](docs/adr/0016-undo-records-what-has-already-happened.md) |
| A command is a name in a context; the directory is the editor's | [ADR-0017](docs/adr/0017-a-command-is-a-name-in-a-context.md) |
| A project is JSON, and stores topology verbatim | [ADR-0018](docs/adr/0018-a-project-is-json-and-stores-topology-verbatim.md) |
| The ui is laid out by what draws it, and hit tested against those bounds | [ADR-0019](docs/adr/0019-the-ui-is-laid-out-by-what-draws-it.md) |
| A theme is data, and the app resolves the images it names | [ADR-0020](docs/adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) |

Three things the ADRs do not say. **Multiple viewports are several passes over one frame** — the editor's
`Renderer` builds one `Pass` per `ViewPort`, each with its own region, camera and clear; `data/layout.json`
decides the split. **There is no file chooser in the tree**, so `project::load`, `project::save` and
`project::export::rib` work on one document at a fixed `project.json` or `export.rib` beside the
executable. **`Tool` stays in the editor**: no game holds a gesture open across events.

The RIB export is one way, per [ADR-0023](docs/adr/0023-rib-is-the-offline-scene-description.md):
`RIBExportVisitor` writes topology and a placement per mesh from the active view's camera, and nothing
reads it back. The scene has no lights and no materials, so what it produces renders in one flat colour.

## Conventions

- Headers are `.h`; implementations are `.cpp` **or** `.cxx`, mixed even within a directory
  (`api/render/realtime/*.cpp` alongside `api/render/realtime/vulkan/*.cxx`). Match the immediate
  neighbours.
- Namespaces mirror the `api/` path: `v3d::asset`, `v3d::render::realtime`,
  `v3d::render::realtime::vulkan`. Closed with `};  // namespace <full name>` — the trailing semicolon is
  part of the style.
- 4-space indent; access specifiers indented one space into the class body (` public:`, ` private:`).
- **A namespace body is not indented**, which is Google's rule and what `whitespace/indent_namespace`
  enforces. That check cannot tell a continuation line from a declaration, so a continuation at namespace
  scope — a constructor's initialiser list, a string built from adjacent literals — sits at column 0 too.
  It only misreads a nested-class constructor (`Recorder::Target::Target()`), not `Plain::Plain()`.
- `boost::shared_ptr` / `boost::make_shared` throughout, not the `std` equivalents.
- Doc comments are `/** **/` blocks, frequently left empty above trivial members.
- Logging is spdlog through the wrapper: `logger_->get()->info("... {}", value)`. The older
  `LOG_INFO`/`LOG_ERROR` macros survive only in commented-out or non-compiling code — don't add new uses.
- [.gitattributes](.gitattributes) enforces LF (`* text=auto eol=lf`). Editors that save CRLF turn a small
  change into a whole-file diff; strip the CRs rather than committing them.
- **Comments explain the code, not the change.** No history ("this used to", "the block that was here"),
  no justification for why a commit exists, no roadmap for a later phase. Why a decision was made belongs
  in [docs/adr/](docs/adr/) and what is coming belongs in [docs/plans/](docs/plans/) — a comment that
  repeats either goes stale where nobody is looking. A non-obvious invariant, a trap, or a constraint the
  code satisfies is exactly what a comment is for. Three habits that keep reappearing:
  - **No provenance from another tree.** "which is rigel's rule", "what `v3dlibs` did here". Those trees
    are deleted, so every such reference names something no reader can open. Keep the rule, drop the
    attribution — the audits are where a port's lineage lives.
  - **Cite an ADR, do not summarise it.** "per ADR-00NN" followed by a paragraph re-deriving the argument
    is the restatement the ADR exists to prevent. Say which record settles it, then state only the
    invariant a caller has to honour.
  - **Plain register.** No conversational openers ("and ...", "so ..."), no personification, no
    editorialising about how bad the alternative would be. A comment is a note to the next reader, not
    narration.

**This file follows the same rule.** It describes the tree as it stands — not what changed, not when.
Dated narrative and per-commit history belong in git, the ADRs and the plans.
