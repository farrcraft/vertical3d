# Building

This document is for contributors who build this repository. It covers the configure and build
commands, the CMake layout and options, the helper functions, shaders, the linking rules and the
traps. [GettingStarted.md](GettingStarted.md) walks through a first build from a fresh clone, and
[Dependencies.md](Dependencies.md) covers what has to be installed first. Building an app in
another repository against the api is covered in [UsingTheApi.md](../api/UsingTheApi.md).

- [Configure and build](#configure-and-build)
- [The CMake layout](#the-cmake-layout)
- [The options](#the-options)
- [The helper functions](#the-helper-functions)
- [Shaders](#shaders)
- [Linking rules](#linking-rules)
- [The starter example](#the-starter-example)
- [Traps](#traps)

## Configure and build

The tree builds with MSVC on Windows, using CMake and Ninja. There is no `CMakePresets.json`.
Visual Studio configures `out/build/x64-Debug` from [CMakeSettings.json](../../CMakeSettings.json)
when it opens the folder.

From a shell, enter a developer environment first (run `vcvars64.bat`), then configure and build:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
```

The first configure installs every vcpkg package into `out/build/x64-Debug/vcpkg_installed/`.
See [Traps](#traps) for how long that takes and why that directory must be kept.

`ninja`, `cmake` and `ctest` are on the PATH only inside a developer environment. Two scripts
enter one for you:

```
scripts\build.cmd                         # ninja: everything, or one target by name
scripts\test.cmd -R input                 # ctest, with any arguments passed through
```

- [scripts/build.cmd](../../scripts/build.cmd) runs `ninja -C out/build/x64-Debug` and passes its
  arguments on.
- [scripts/test.cmd](../../scripts/test.cmd) runs `ctest --test-dir out/build/x64-Debug
  --output-on-failure` and passes its arguments on.
- Neither one configures. Both stop with a message if the tree has not been configured yet.
- Both look for `vcvars64.bat` under Visual Studio 18 and then Visual Studio 2022 (Community
  edition). For any other install, set `V3D_VCVARS` to the full path of `vcvars64.bat`.

Every target in the tree compiles and links.

## The CMake layout

The build is defined by four files at the root:

| File | Contents |
|---|---|
| [CMakeLists.txt](../../CMakeLists.txt) | The options, the compiler flags, and the list of app subdirectories |
| [cmake/v3dApiLibraries.cmake](../../cmake/v3dApiLibraries.cmake) | The manifest: for each api library, the other api libraries it links (`REQUIRES`) and the third-party packages it uses (`PACKAGES`) |
| [cmake/v3dDependencies.cmake](../../cmake/v3dDependencies.cmake) | The `find_package` calls |
| [cmake/v3dHelpers.cmake](../../cmake/v3dHelpers.cmake) | The `v3d_add_*` helper functions |

[api/CMakeLists.txt](../../api/CMakeLists.txt) loops over the api libraries the root selected and
adds each one's directory. The selection comes from `V3D_LIBRARIES` (see
[the options](#the-options)).

**Name paths into this repository with `V3D_ROOT`, never `CMAKE_SOURCE_DIR`.** When another
project nests this repository with `add_subdirectory`, `CMAKE_SOURCE_DIR` is that project's root.
`V3D_ROOT` is always the root of this repository.

### Compiler settings

- The root sets `CMAKE_CXX_STANDARD 23`, which CMake maps to `/std:c++latest` for MSVC. Do not
  add `/std:c++latest` as a flag. glm and EnTT require `cxx_std_17` through their targets, so
  CMake adds `/std:c++17`, and MSVC reports warning D9025 when a command line names two
  standards.
- `/permissive-`, `/W4` and `/w14062` apply to every target. [Linting.md](Linting.md#the-compiler)
  explains `/w14062` and `/WX`.
- `/EHsc` and `/utf-8` are set per target. `v3d_add_api_library` puts both in each library's
  interface, so anything linking an api library gets them.
- **`/utf-8` is required.** spdlog's bundled fmt has a `static_assert` that fails without it.

### The manifest is checked on every configure

The manifest in `cmake/v3dApiLibraries.cmake` repeats what each library's `target_link_libraries`
says. It has to exist because the configure needs each library's dependencies before any library
has been added. Two checks run after every library is added, and either one stops the configure:

- **`v3d_api_verify_manifest`** compares the manifest with the link lines. It fails on a
  difference in either direction and prints the line that is wrong. When you add a package or an
  api library to a library's `CMakeLists.txt`, add it to the manifest too.
- **`v3d_api_verify_visibility`** reads every header of every api library. It fails when a header
  includes another api library that its own library links PRIVATE or not at all. It also fails
  when a PUBLIC link to another api library is not needed by any header. See
  [Linking rules](#linking-rules).

The checks run at configure time, so a header edited afterwards is checked at the next configure.
CI configures on every run.

`cgltf` is the one manifest entry these checks cannot confirm. It adds an include directory rather
than an imported target, so it never appears in a link line.

## The options

| Option | Default | Controls |
|---|---|---|
| `V3D_BUILD_APPS` | top level | Everything outside `api/` |
| `V3D_BUILD_TESTS` | top level | The Boost.Test suites and their ctest entries |
| `V3D_WARNINGS_AS_ERRORS` | top level | `/WX`, and whether an analyser finding stops the build |
| `V3D_ANALYZE` | `OFF` | MSVC `/analyze`. See [Linting.md](Linting.md) |
| `V3D_CLANG_TIDY` | `OFF` | clang-tidy. See [Linting.md](Linting.md) |
| `V3D_LIBRARIES` | `all` | Which api libraries to build. See [UsingTheApi.md](../api/UsingTheApi.md#selecting-libraries) |

"Top level" means the value of `PROJECT_IS_TOP_LEVEL`. It is on for every build of this
repository and off when another project nests it.

`V3D_LIBRARIES` must be `all` whenever `V3D_BUILD_APPS` or `V3D_BUILD_TESTS` is on, and the
configure stops if it is not. The apps link every api library between them, and each test
directory belongs to one library. In practice a build of this repository always builds the whole
api.

Each `tests/` subdirectory is added under its own `if(V3D_BUILD_TESTS)`, not inside
`v3d_add_test`. A `tests/CMakeLists.txt` names its test target again after creating it, so the
guard has to cover the whole file.

## The helper functions

| Function | What it does |
|---|---|
| `v3d_add_api_library(<name> <sources>)` | Declares `v3dlib_<name>` and its alias `v3d::<name>`. Adds the repository root as a PUBLIC include directory, puts `/EHsc` and `/utf-8` in the interface, and links `Boost::headers` PUBLIC |
| `v3d_add_shared_data(<target>)` | Copies the root [data/](../../data) directory beside the executable |
| `v3d_add_app_data(<target>)` | Copies `<app>/data` beside the executable |
| `v3d_add_test(<lib> <sources>)` | Builds `v3dtest_<lib>` and registers it with ctest. See [Testing.md](Testing.md#writing-a-test) |
| `v3d_add_shader(<target> <source> [OUTPUT <name>] [DEFINES <define>...])` | Compiles a GLSL shader into the target. See [Shaders](#shaders) |

### Data directories

The engine resolves every asset relative to the executable, so assets are copied beside it.
Assets used by more than one app live in the root [data/](../../data) directory. An app's own
assets live in `<app>/data`.

The two data helpers add a build rule whose inputs are the files in the source directory:

- Editing a file, or adding one, is copied at the next build. The file list is re-globbed at
  build time.
- **A deleted file is not removed** from the build tree, because the copy merges rather than
  mirrors. Delete the `data/` directory beside the executable and build again to clear it.
- An app that calls both helpers gets both directories copied into the same `data/`, shared
  data first.

These apps call `v3d_add_app_data`: pong, tetris, voxel, odyssey, moya and vertical3d. pong
also calls `v3d_add_shared_data`.

## Shaders

Shaders are compiled at build time and embedded in the binary. They are not shipped as data
files.

`v3d_add_shader(<target> <source>)` runs the Vulkan SDK's `glslc` on a GLSL file. glslc writes
the SPIR-V as a C initialiser list to `<binary dir>/shaders/<name>.inc`, and the C++ source
`#include`s that file into a `uint32_t` array. The generated file is outside the source tree,
which is why its include stays quoted (see [Conventions.md](Conventions.md#includes)).

- The engine's shaders are in [api/render/shaders/](../../api/render/shaders).
- `OUTPUT` gives the compiled module a name other than the source file's name.
- `DEFINES` are passed to the preprocessor. One source compiled twice with different defines
  gives two shaders. For example, a skinned lit shader is the rigid one compiled with `SKINNED`.
- A shader can `#include` another file, beside it or by a relative path, after
  `#extension GL_GOOGLE_include_directive : require`. glslc writes a depfile listing the
  included files, so editing a shared include rebuilds every shader that uses it. The lit
  shaders share [shaders/lit/lit.glsl](../../api/render/shaders/lit/lit.glsl) this way, and
  [shaders/lit/pose.glsl](../../api/render/shaders/lit/pose.glsl) includes `skin.glsl` when
  `SKINNED` is defined.

`v3d_add_shader` looks for `glslc` the first time it is called, using `VULKAN_SDK`. If it is not
found, the configure stops with "glslc was not found". Only `api/render`, the render device tests
and voxel call it, so a configure that includes none of them does not need glslc.

## Linking rules

**An api library links everything it uses, including the other api libraries.** For example,
`v3dlib_engine` links asset, asset_media, config, event, input, render and the rest of its
manifest entry, so an app that links `v3dlib_engine` gets them too.

**An app names the `v3dlib_*` targets it uses and nothing else.** The exception is a package the
app uses directly, such as `Boost::program_options` in the apps that parse a command line.

**PUBLIC or PRIVATE:** link a package or another api library PUBLIC when one of the library's
headers includes it, and PRIVATE when only its sources do. The configure enforces this for api
libraries (see [the manifest checks](#the-manifest-is-checked-on-every-configure)). A wrong
visibility can otherwise compile for as long as some other library happens to export the same
dependency.

Specific rules:

- **Apps do not name spdlog or fmt.** `v3dlib_log` links `spdlog::spdlog` PUBLIC so the
  `SPDLOG_COMPILED_LIB` definition reaches every user. Every api library whose sources include
  [Logger.h](../../api/log/Logger.h) must link `v3dlib_log` PUBLIC for the same reason. Without
  the definition, spdlog compiles header-only and defines symbols the compiled library also
  defines. The result is a duplicate-symbol link error in whichever app pulls in the wrong object
  first.
- **Only an app that plays sound links the mixer.** It names `v3dlib_audio`, which links
  `SDL3_mixer::SDL3_mixer` PRIVATE, because its headers declare the mixer's handles rather than
  including the mixer. The app calls `audio::registerLoaders()` on its asset manager. No other
  api library depends on audio.
- **An asset manager loads only the file types registered on it.** `engine::Engine` registers
  `v3dlib_asset_media`'s loaders on the manager it builds. A manager built anywhere else,
  including in a test, starts with JSON documents only and must call
  `asset::media::registerLoaders()` itself.
- **There is no OpenGL in the tree.** A target that names `OpenGL::GL`, `GLEW::GLEW` or
  `v3dlib_gl` will not configure.
- **glm and EnTT must be linked explicitly.** Each library whose headers use them names
  `glm::glm` or `EnTT::EnTT`. Do not rely on their headers being reachable through another
  package's include directory.

Background: [ADR-0027](../adr/0027-build-consume-the-api-as-source.md),
[ADR-0079](../adr/0079-assets-loaders-are-registered.md)

## The starter example

[examples/starter/](../../examples/starter) is a small windowed app that consumes the api the way a
project in another repository would. It is a root CMake project of its own, so the root
`CMakeLists.txt` does not add it, and a normal build of this repository does not build it.

CI builds it on every run, after the main build. It is the only build that can catch an api
library that depends on something only this repository's root provides, such as a global flag or
an app that happens to link every library. It sets `V3D_LIBRARIES` to `engine log render`, so it
also checks that the manifest's dependency lists are complete.

To build it locally against the packages your main build already installed:

```
cmake -S examples/starter -B out/build/starter -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  -DVCPKG_INSTALLED_DIR=out/build/x64-Debug/vcpkg_installed \
  -DVCPKG_MANIFEST_INSTALL=OFF
ninja -C out/build/starter
```

## Traps

- **Never delete `out/build/<config>/vcpkg_installed/`.** That directory holds every installed
  package. A cold install builds boost from source and takes about 45 minutes. When CMake needs
  a fresh cache, delete only these, then configure again:
  - `CMakeCache.txt`
  - `CMakeFiles/`
  - `build.ninja`
  - `cmake_install.cmake`
  - `.ninja_*`

  The usual reason is a Visual Studio toolset update, which leaves the cached
  `CMAKE_CXX_COMPILER` pointing at a compiler version that has been uninstalled.
- **Editing `vcpkg.json` re-runs the package install at the next configure.** Adding or removing
  a boost port can rebuild boost. Changing the `sdl3` feature list rebuilds SDL only, which takes
  about five minutes.
- **cgltf has no CMake config.** It is a single header. `cmake/v3dDependencies.cmake` finds it with
  `find_path(V3D_CGLTF_INCLUDE_DIR ...)`, and `v3dlib_asset_media` adds that directory to its own
  include path. Its implementation is compiled once, in
  [api/asset/media/loader/CgltfImpl.cpp](../../api/asset/media/loader/CgltfImpl.cpp). That file is
  excluded from both static analysers, as `voxel/src/noise/noiseutils.cpp` is.
- **The `VCPKG_ROOT` value in CMakeSettings.json is wrong.** It has a doubled path segment and
  points at nothing. vcpkg still works, because the build uses the toolchain file.
- **voxel will not link until libnoise is built.** `vendor/libnoise` is a git submodule and is
  not prebuilt. voxel and its test suite expect the library under `vendor/libnoise/Debug`. See
  [Dependencies.md](Dependencies.md#building-libnoise).
- **A test fixture or reference image is copied only when its suite relinks.** See
  [Testing.md](Testing.md#traps).
- **LNK1163 names a corrupt object file**, not a code problem. Delete the `.obj` it names and
  build again.
