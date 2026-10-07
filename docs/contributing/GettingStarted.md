# Getting Started

This page takes a new contributor from a fresh clone to a working build: install the tools,
configure, build, run the tests and run an app. Each step links to the document that covers it in
full.

- [1. Install the tools](#1-install-the-tools)
- [2. Clone the repository](#2-clone-the-repository)
- [3. Set up vcpkg](#3-set-up-vcpkg)
- [4. Build libnoise](#4-build-libnoise)
- [5. Configure](#5-configure)
- [6. Build](#6-build)
- [7. Run the tests](#7-run-the-tests)
- [8. Run an app](#8-run-an-app)
- [Where to go next](#where-to-go-next)

## 1. Install the tools

The tree builds on Windows with MSVC. Install:

- **Visual Studio 2022 or newer**, with the "Desktop development with C++" workload. It provides
  MSVC, CMake and Ninja.
- **The [Vulkan SDK](https://vulkan.lunarg.com/).** Its installer sets `VULKAN_SDK`. The build
  needs it to compile shaders, and the renderer uses its validation layer.
- **Git.**
- **Python 3 and cpplint**, for the lint check: `pip install cpplint==2.0.2`.

`cmake`, `ninja` and `ctest` are on the PATH only inside a developer environment. Open the
"x64 Native Tools Command Prompt", or run `vcvars64.bat` from your Visual Studio install, before
running the commands below. The scripts in step 6 enter that environment for you.

## 2. Clone the repository

```
git clone --recurse-submodules https://github.com/farrcraft/vertical3d.git
cd vertical3d
```

`--recurse-submodules` also clones `vendor/libnoise`, the tree's only submodule. If you cloned
without it, run `git submodule update --init`.

## 3. Set up vcpkg

vcpkg is not committed, so clone it into `vendor/vcpkg` and bootstrap it:

```
git clone https://github.com/Microsoft/vcpkg.git vendor/vcpkg
.\vendor\vcpkg\bootstrap-vcpkg.bat
```

If you will build from Visual Studio, also run `.\vendor\vcpkg\vcpkg.exe integrate install`.

You do not install packages yourself. The first configure installs them.
[Dependencies.md](Dependencies.md) lists the packages.

## 4. Build libnoise

The voxel app and its tests link libnoise, and the full build fails until it is built. From the
repository root, in a developer environment, replace `<repo>` with the absolute path of your
clone and run:

```
cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug
cmake --build vendor/libnoise/build-ninja
```

[Dependencies.md](Dependencies.md#building-libnoise) explains the options.

## 5. Configure

There is no `CMakePresets.json`. Visual Studio configures from
[CMakeSettings.json](../../CMakeSettings.json) when you open the folder. From a developer
environment:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
```

**The first configure is slow**, because vcpkg builds every package from source. The packages go
into `out/build/x64-Debug/vcpkg_installed/`. Never delete that directory: read
[Build.md](Build.md#traps) before you reset a build directory.

[Build.md](Build.md#the-options) lists the configure options.

## 6. Build

From a developer environment:

```
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
```

Or from any shell, with the script that enters the developer environment first:

```
scripts\build.cmd
scripts\build.cmd pong
```

The build treats warnings as errors. The tree has no warnings, so any warning comes from your
change. [Linting.md](Linting.md) covers the other checks.

## 7. Run the tests

```
ctest --test-dir out/build/x64-Debug --output-on-failure
```

Or, from any shell:

```
scripts\test.cmd
scripts\test.cmd -R image                 # one suite
```

Every suite should pass. `render_device` needs a Vulkan device. On a machine without one, ctest
reports it as skipped. [Testing.md](Testing.md) explains the suites and how to write a test.

## 8. Run an app

Each app is built into its own directory under `out/build/x64-Debug/`, with its data copied
beside it:

| App | Executable |
|---|---|
| pong | `out/build/x64-Debug/pong/pong.exe` |
| tetris | `out/build/x64-Debug/tetris/tetris.exe` |
| voxel | `out/build/x64-Debug/voxel/voxel.exe` |
| odyssey | `out/build/x64-Debug/odyssey/Odyssey.exe` |
| vertical3d (the editor) | `out/build/x64-Debug/vertical3d/vertical3d.exe` |
| moya | `out/build/x64-Debug/moya/moya/moya.exe` |
| imagetool | `out/build/x64-Debug/imagetool/imagetool.exe` |

Each app writes a log, `v3d.log`, in the same directory as its executable. Open it after a run.
With the Vulkan SDK installed, the renderer turns on the validation layer and writes its messages
to this log. A run with no validation messages is a clean run.
[Testing.md](Testing.md#verifying-a-rendering-change) explains how to check a rendering change.

moya renders a RIB file from the command line. Run it from its own directory:

```
cd out/build/x64-Debug/moya/moya
moya.exe --file data/raytrace-scene.rib --output scene.png
```

## Where to go next

- [Conventions.md](Conventions.md): code style, and how to write comments and documents.
- [Build.md](Build.md): the CMake layout, linking rules and traps.
- [Testing.md](Testing.md): the test suites and rendering checks.
- [Linting.md](Linting.md): cpplint and the static analysers.
- [CONTRIBUTING.md](../../CONTRIBUTING.md): the branch and pull request workflow.
- [docs/api/README.md](../api/README.md): the libraries, and a glossary of terms.
- [docs/adr/README.md](../adr/README.md): the design decisions and their reasons.
