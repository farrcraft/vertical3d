# Building

Visual Studio configures `out/build/x64-Debug` directly from
[CMakeSettings.json](../CMakeSettings.json). From a shell, use a developer environment
(`vcvars64.bat`), then:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
```

[scripts/build.cmd](../scripts/build.cmd) and [scripts/test.cmd](../scripts/test.cmd) are the
same two commands with the developer environment already entered, for a shell that is not one:

```
scripts\build.cmd                         # everything; or one target by name
scripts\test.cmd -R input                 # ctest, arguments passed through
```

They build a tree that is already configured rather than configuring one, and they find
`vcvars64.bat` under Visual Studio 18 or 2022. Set `V3D_VCVARS` to it for anything else.

Everything in the tree compiles and links. MSVC and Windows in practice.
[Dependencies.md](Dependencies.md) covers what has to be installed first and how a dependency
is added or updated. This document covers what the build does with them.

## The CMake layout

**The root is four files.** [cmake/v3dApiLibraries.cmake](../cmake/v3dApiLibraries.cmake)
holds the manifest of what each api library depends on,
[cmake/v3dDependencies.cmake](../cmake/v3dDependencies.cmake) the `find_package` calls,
[cmake/v3dHelpers.cmake](../cmake/v3dHelpers.cmake) the five `v3d_add_*` functions, and
[CMakeLists.txt](../CMakeLists.txt) the options. `api/CMakeLists.txt` is a loop over the
libraries the root selected — see [Selecting the api](#selecting-the-api).

**Paths into this repository go through `V3D_ROOT`, never `CMAKE_SOURCE_DIR`.** Once another
project has nested this one, `CMAKE_SOURCE_DIR` names the consumer's root — see
[ADR-0027](adr/0027-the-api-is-consumed-as-source.md).

The root sets `CMAKE_CXX_STANDARD 23`, which CMake maps to `/std:c++latest` here. It is stated
as a standard rather than as the flag because glm and EnTT require `cxx_std_17` through their
interfaces, and MSVC reports D9025 for a command line naming two standards. `/permissive-` and
`/W4` are unconditional. `/EHsc` and `/utf-8` are set per target.

**`/utf-8` is required, not cosmetic.** spdlog's bundled fmt has a `static_assert` that fails
without it, so `v3d_add_api_library` puts it in the interface as well as on the library
itself.

### The options

| Option | Default | |
|---|---|---|
| `V3D_BUILD_APPS` | top level | Everything that is not the api |
| `V3D_BUILD_TESTS` | top level | The Boost.Test suites and their ctest entries |
| `V3D_WARNINGS_AS_ERRORS` | top level | `/WX`, and whether an analyser finding stops the build |
| `V3D_ANALYZE` | `OFF` | MSVC `/analyze` — see [Linting.md](Linting.md) |
| `V3D_CLANG_TIDY` | `OFF` | clang-tidy — see [Linting.md](Linting.md) |
| `V3D_LIBRARIES` | `all` | Which api libraries to build — see below |

"top level" means `PROJECT_IS_TOP_LEVEL`: on for every build of this repository, off for a
consumer that has nested it.

The tests guard is written on each `add_subdirectory("tests")` rather than inside
`v3d_add_test`, because a `tests/CMakeLists.txt` names its target again after calling it.

### Selecting the api

Per [ADR-0033](adr/0033-a-consumer-selects-the-api-libraries-it-wants.md), **a consumer names
the libraries it links and the tree works out the rest**:

```cmake
set(V3D_BUILD_APPS OFF)
set(V3D_BUILD_TESTS OFF)
set(V3D_LIBRARIES image log)      # or leave it at "all"
add_subdirectory("${V3D_TREE}" v3d)
```

The closure of that set decides two things: which subdirectories `api/CMakeLists.txt` adds, and
which packages `v3d_find_packages` looks for. `image log` needs Boost, JPEG, PNG, glm and
spdlog, and **configures with no Vulkan SDK, no SDL3 and no Freetype installed**. `v3d::render`
is what wants Vulkan, and `v3d::render_offline` — added by `api/CMakeLists.txt` rather than by
`api/render`, so it is takeable on its own — wants none of it.

Two things are always resolved whatever is selected. **Boost**, because
`v3d_add_api_library` links `Boost::headers` into every library. And **glslc**, but only if
`api/render` is in the closure, because that is the only thing calling `v3d_add_shader`.

`V3D_LIBRARIES` must be `all` when `V3D_BUILD_APPS` or `V3D_BUILD_TESTS` is on, and the
configure stops if it is not: the apps name every library between them, and each `tests/`
directory belongs to one library.

**The manifest is checked against the link graph on every configure.**
[cmake/v3dApiLibraries.cmake](../cmake/v3dApiLibraries.cmake) states each library's `REQUIRES`
and `PACKAGES` a second time, because the closure has to be known before any library has been
configured. `v3d_api_verify_manifest` reads what the targets actually linked and fails on a
difference in either direction, so the two cannot drift. Adding a package to an api library
means editing its `CMakeLists.txt` **and** the manifest; forgetting the second stops the next
configure with the exact line that is wrong.

`cgltf` is the one entry nothing can check — it contributes an include directory rather than an
imported target, so it has no link line to appear in.

### The helper functions

- `v3d_add_api_library(<name> <sources>)` declares `v3dlib_<name>`, the `v3d::<name>` alias an
  external consumer links, the include root, `/EHsc` and `/utf-8` in the interface, and the
  boost winapi definitions.
- `v3d_add_shared_data(<target>)` copies the root [data/](../data/) beside the executable.
- `v3d_add_app_data(<target>)` copies `<app>/data` beside the executable.
- `v3d_add_test(<lib> <sources>)` — see [Testing.md](Testing.md).
- `v3d_add_shader(<target> <source>)` — see below.

**Assets shared by more than one app live in the root [data/](../data/)**, and an app's own
live in `<app>/data`. Both are copied next to the executable, because the engine resolves an
asset relative to the exe.

**Only tetris, voxel, odyssey and talyn call `v3d_add_app_data`.** Everywhere else the `data/`
under `out/build/<config>/<app>/` is a stale manual copy, so editing `pong/data/*.json` does
not affect a run from the build tree until you copy it across.

## Shaders

**Shaders are compiled at build time and embedded, not shipped as data.**
`v3d_add_shader(<target> <source>)` runs the Vulkan SDK's `glslc` over a GLSL file and writes
SPIR-V as a C initialiser list into `<binary dir>/shaders/<name>.inc`. The source `#include`s
that into a `uint32_t` array. The engine's shaders are in
[api/render/shaders/](../api/render/shaders/).

`VULKAN_SDK` must point at an SDK install, or the first call to that function fails with
"glslc was not found". The tool is looked for at that first call rather than at configure
time, so a build that compiles no shader is not stopped by its absence.

## Traps

- **Never delete `out/build/<config>/vcpkg_installed/`.** That directory *is* the dependency
  install. When CMake needs a fresh cache — usually after a VS toolset update leaves the cached
  `CMAKE_CXX_COMPILER` pointing at a version that no longer exists — delete only
  `CMakeCache.txt`, `CMakeFiles/`, `build.ninja`, `cmake_install.cmake` and `.ninja_*`, then
  reconfigure. Reconfiguring is fast; reinstalling is not.
- **Editing `vcpkg.json` re-runs the manifest install.** A cold install builds boost from source
  and takes roughly 45 minutes. Changing the sdl3 feature set rebuilds SDL only, about five
  minutes.
- **cgltf has no CMake config.** It is a single header the port copies into `include/`, found
  with `find_path(V3D_CGLTF_INCLUDE_DIR ...)` in
  [cmake/v3dDependencies.cmake](../cmake/v3dDependencies.cmake) and put on `v3dlib_asset`'s own
  include path. ADR-0027 removed the alternative, a header reachable only through the vcpkg
  include directory. Its implementation half is compiled once, in
  [api/asset/loader/CgltfImpl.cpp](../api/asset/loader/CgltfImpl.cpp), which is exempt from both
  analysers the same way `voxel/src/noise/noiseutils.cpp` is.
- `VCPKG_ROOT` in CMakeSettings.json has a doubled path segment and points nowhere. vcpkg works
  through the toolchain file regardless.
- `vendor/libnoise` is the only submodule, is not prebuilt, and `voxel` will not link without
  it. See [Dependencies.md](Dependencies.md#building-libnoise). voxel's
  `target_link_directories` expects its artefacts under `vendor/libnoise/Debug`.

## Linking rules

Per [ADR-0027](adr/0027-the-api-is-consumed-as-source.md), an api library carries its own
dependencies, **including the other api libraries it uses**. So `v3dlib_engine` brings asset,
config, event, input and render with it.

An app names the `v3dlib_*` targets it uses and nothing else. The exception is an app that uses
a package directly, such as `Boost::program_options` in the three that parse a command line.

Adding a third-party package to an api library means naming it PUBLIC when a header of that
library names its types, and PRIVATE otherwise.

- **Apps name neither spdlog nor fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC so the
  `SPDLOG_COMPILED_LIB` definition propagates. Every `api/` library whose sources compile
  [Logger.h](../api/log/Logger.h) must link `v3dlib_log` PUBLIC for the same reason. Without
  the definition it builds spdlog header-only and emits symbols the compiled library also
  defines, which surfaces as a duplicate-symbol link error in whichever app pulls the wrong
  object first.
- **Apps name neither the mixer nor `v3dlib_audio` unless they play a sound.** `v3dlib_asset`
  links `v3dlib_audio` PUBLIC and `v3dlib_audio` links `SDL3_mixer::SDL3_mixer` PUBLIC, so it
  propagates.
- **There is no OpenGL in the tree.** A target naming `OpenGL::GL`, `GLEW::GLEW` or `v3dlib_gl`
  will not configure.
- **glm and EnTT have to be linked, not assumed.** They resolved for years without a
  `find_package`, because `Boost_INCLUDE_DIRS` is the vcpkg installed include directory and the
  root put it on every target's include path. That line is gone. `glm::glm` and `EnTT::EnTT`
  are now named by the libraries whose headers use them.

## Building from another repository

**An application in another repository takes this one as source**, nested with
`add_subdirectory`. [NewProject.md](NewProject.md) is the walkthrough, and
[examples/starter/](../examples/starter/) is a working app that CI builds on every push.

The starter is the only thing in the tree that can catch an api library relying on a global
the root sets, or on an app naming every library. That is how `api/engine` and `api/render`
were found not declaring the api libraries they use. It names its three libraries in
`V3D_LIBRARIES` rather than taking `all`, so the closure of
[ADR-0033](adr/0033-a-consumer-selects-the-api-libraries-it-wants.md) is exercised on every
push as well.

[examples/](../examples/) is the exception to "every directory here builds with the tree":
nothing in the root's `add_subdirectory` list names it, because each example is a root project
of its own, configured separately. That is the only way to be a consumer of this repository
from inside it.
