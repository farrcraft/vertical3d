# Using The api From Another Repository

This is a walkthrough for building an application in its own repository on top of the `api/`
libraries. It covers the repository layout, the vcpkg manifest, the CMake setup, a minimal app,
and how to build, run and update it. [docs/api/README.md](README.md) describes the libraries
themselves.

[examples/starter/](../../examples/starter) is the finished result of this walkthrough. CI builds
it on every run. Copy it rather than typing the steps out; this page explains each piece.

- [Before you start](#before-you-start)
- [What you need](#what-you-need)
- [1. The repository](#1-the-repository)
- [2. The vcpkg manifest](#2-the-vcpkg-manifest)
- [3. The CMakeLists](#3-the-cmakelists)
- [4. The application](#4-the-application)
- [5. The config](#5-the-config)
- [6. Build and run](#6-build-and-run)
- [Keeping up with the tree](#keeping-up-with-the-tree)
- [Changing the tree from a consumer](#changing-the-tree-from-a-consumer)

## Before you start

**Consider adding your app to the vertical3d repository instead.** Every top-level directory
there builds with the tree, so a new app directory needs no extra setup. A separate repository is
worth it when the separation matters: a different release schedule, a different licence, or
collaborators who should not have the rest of the source. If the only reason is tidiness, add a
directory to the vertical3d repository.

The api is used as **source**, not as an installed package. There is no
`find_package(vertical3d)` and nothing to install. Your project adds this repository with
`add_subdirectory` and builds it with your compiler.

Background: [ADR-0027](../adr/0027-build-consume-the-api-as-source.md)

## What you need

- **MSVC** on Windows. Work in a developer environment (run `vcvars64.bat`).
- **CMake 3.21 or newer**, and **Ninja**. Both come with Visual Studio.
- **Your own vcpkg clone.** vertical3d does not commit one (`vendor/vcpkg/` is in its
  `.gitignore`), so cloning vertical3d does not give you vcpkg.
- **The [Vulkan SDK](https://vulkan.lunarg.com/), if you draw anything.** You need it when the
  libraries you select include `render`, directly or through `engine` or `ui`. The SDK provides
  Vulkan itself and `glslc`, which compiles the renderer's shaders at build time. An app that
  uses only libraries such as `image`, `log` or `grid` does not need it. See
  [Selecting libraries](#selecting-libraries).

## 1. The repository

```
git init myapp
cd myapp
git submodule add https://github.com/<you>/vertical3d vendor/vertical3d
git clone https://github.com/microsoft/vcpkg vendor/vcpkg
.\vendor\vcpkg\bootstrap-vcpkg.bat
```

vertical3d has one submodule of its own, libnoise, which only the voxel app uses. Apps are not
built when vertical3d is nested in another project, so you do not need to clone libnoise.

The layout ends up as:

```
myapp/
  CMakeLists.txt
  vcpkg.json
  vcpkg-configuration.json
  data/
    config.json
    window.json
  src/
    AppEngine.h
    AppEngine.cxx
    main.cxx
  vendor/
    vertical3d/     <- submodule
    vcpkg/
```

## 2. The vcpkg manifest

**Copy both [vcpkg.json](../../vcpkg.json) and
[vcpkg-configuration.json](../../vcpkg-configuration.json) from vertical3d into your root.**

vcpkg's manifest mode reads the manifest of the top-level project, which is yours. vertical3d's
own `vcpkg.json` is not read once it is nested, so your manifest must list every package the api
needs.

- **Keep `sdl3`'s `vulkan` feature.** Without it, `SDL_Vulkan_LoadLibrary` fails at startup with
  "No dynamic Vulkan support in current SDL video driver (windows)". The app fails with an
  unhandled exception, not a build error.
- **Copy the `baseline` commit in `vcpkg-configuration.json` exactly.** The baseline fixes every
  package's version. A boost library's file name includes its version, so a different baseline
  gives a different boost, and the result is a link error far from its cause. Nothing checks that
  your baseline matches vertical3d's.

## 3. The CMakeLists

```cmake
cmake_minimum_required(VERSION 3.21)

project("myapp" CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("/permissive-")

set(V3D_BUILD_APPS OFF)
set(V3D_BUILD_TESTS OFF)
set(V3D_LIBRARIES engine log render)
add_subdirectory("vendor/vertical3d" v3d)

add_executable(myapp
	"src/AppEngine.h" "src/AppEngine.cxx"
	"src/main.cxx")

v3d_add_app_data(myapp)

target_link_libraries(myapp PRIVATE
	v3d::engine
	v3d::log
	v3d::render)
```

The rest of this section explains each part.

### The language standard

Set `CMAKE_CXX_STANDARD 23`, which CMake maps to `/std:c++latest` for MSVC. Do not add
`/std:c++latest` with `add_compile_options`. glm and EnTT require `cxx_std_17` through their
targets, so CMake adds `/std:c++17` as well, and MSVC then reports warning D9025 on every file.

The standard and `/permissive-` are the only compiler settings you supply. The include root,
`/EHsc` and `/utf-8` come from the `v3d::` targets you link.

### Apps and tests off

`V3D_BUILD_APPS` and `V3D_BUILD_TESTS` default to off when vertical3d is not the top-level
project. Setting them explicitly makes the intent clear. If they were on, your build would also
build every vertical3d app and every test suite.

### Selecting libraries

`V3D_LIBRARIES` names the api libraries you link. vertical3d adds those libraries, plus every
api library they depend on, and looks only for the third-party packages that set needs.

| You select | You need installed |
|---|---|
| `image log` | Boost, libpng, libjpeg-turbo, glm, spdlog |
| `render`, `engine` or `ui` | Everything above, plus the Vulkan SDK, vulkan-memory-allocator, SDL3, Freetype, EnTT and cgltf |
| (left unset, `all`) | Every package in the manifest |

- **Boost is always required**, because every api library links `Boost::headers`.
- **`render_offline` needs no Vulkan and no SDL.** It is a separate library from `render`, so
  you can select it alone.
- **`V3D_LIBRARIES` must be `all` if you turn the apps or the tests back on**, and the configure
  stops otherwise.

[cmake/v3dApiLibraries.cmake](../../cmake/v3dApiLibraries.cmake) lists each library's
dependencies. The configure prints the resolved set as `vertical3d api:` and
`vertical3d packages:` lines.

Background: [ADR-0033](../adr/0033-build-select-api-libraries-through-a-manifest.md)

### Link `v3d::` targets

Each directory under `api/` provides a `v3d::<library>` target. Link only the ones you use. Each
library links what it needs, so `v3d::engine` brings `v3d::asset`, `v3d::asset_media`,
`v3d::config`, `v3d::event`, `v3d::input`, `v3d::render` and others with it.

Sound is the exception. An app that plays sound links `v3d::audio` and calls
`v3d::audio::registerLoaders()` on its asset manager.

The `v3dlib_*` names are the tree's internal target names. Use the `v3d::` aliases.

### Find your own packages after `add_subdirectory`

The example names no third-party package directly. Everything the three targets need, including
boost, glm, EnTT, SDL and Vulkan, comes through them.

If you name a package yourself, call `find_package` for it in your own `CMakeLists.txt`, after the
`add_subdirectory` line. `find_package` creates imported targets only in the directory that
calls it and that directory's children. vertical3d's directory is a child of yours, so none of its
targets are visible to you. Naming `Boost::program_options` without your own `find_package(Boost)`
fails with "Target myapp links to Boost::program_options but the target was not found".

Calling `find_package` after `add_subdirectory` finds the same boost that vertical3d found,
because vertical3d's call leaves `Boost_DIR` in the CMake cache.

### Data helpers

vertical3d's CMake functions are global once defined, so you can call its two data helpers:

- `v3d_add_app_data(myapp)` copies your `data/` directory beside the executable. The engine
  resolves every asset relative to the executable, not to the working directory.
- `v3d_add_shared_data(myapp)` also copies vertical3d's own `data/` directory, which holds the
  shared fonts and UI themes. Add it once you draw text.

## 4. The application

An app is a subclass of `v3d::engine::Engine`, plus a `main` that runs it.
[examples/starter/src/](../../examples/starter/src) is a complete example.
[engine/](engine/README.md) describes the app lifecycle, the loop, input and config in full. The
points below are the ones a first app needs.

**`main` is one line:**

```cpp
#include <api/engine/Application.h>

#include "AppEngine.h"

int main(int /* argc */, char* argv[]) {
    return v3d::engine::run<AppEngine>(argv[0], "myapp");
}
```

`run()` does the following:

- Works out the executable's directory from `argv[0]`. Every asset path is resolved against it.
- Opens the log, `v3d.log`, in that directory.
- Calls the engine's startup and runs the loop, inside a `try` block that logs any exception.
  A windowed app has no console, so the log is the only place an error appears.
- Shuts the engine down.

Do not write your own `main` loop, menu or text renderer. The api provides
`v3d::ui::paint::TextRenderer` for text and `v3d::ui::shell::GameMenu` for a menu that the Escape
key opens. See [ui/](ui/README.md).

**Includes start at `api/` and use angle brackets:** `#include <api/engine/Engine.h>`. Files
inside vertical3d use the same form, so a header has one spelling everywhere. If you run cpplint
on your own sources, note that it treats an angle-bracket `.h` include as a C system header, so
the `<api/...>` includes go above `<string>` and the other C++ standard headers.

**Override these members of `Engine`:**

| Member | When it runs | Use it for |
|---|---|---|
| `features()` | Before startup | The engine parts to set up. The default is all four: `Feature::Window`, `Feature::Config`, `Feature::MouseInput` and `Feature::KeyboardInput`. Override it only to ask for fewer |
| `start()` | Once, after every feature is set up | Building the renderer and the scene |
| `simulate(float step)` | At a fixed step, in seconds | Game state and physics |
| `tick(unsigned int delta)` | Once per frame, with milliseconds since the last frame | Work that is not simulation |
| `render()` | Once per frame | Drawing |
| `release()` | Once, before the engine destroys the window | Releasing the renderer |

- `Feature::Config` reads `data/config.json`. Without it you still get a window, at the
  window's default size rather than the configured one.
- **Release the renderer in `release()`.** The engine calls it before destroying the window. The
  renderer's device keeps the window's surface alive, and destroying the window unloads the
  Vulkan library. A renderer released later never destroys its surface, and the Vulkan instance
  reports a leak.
- **To quit, call `quit()`.** The loop still ticks and renders after an event handler returns, so
  the window must outlive the handler. `run()` calls `shutdown()`; an app cannot.

**Drawing uses a canvas of quads submitted to a pass:**

1. Call `renderer_->beginFrame(&size)`. It returns false while the window has no area, such as
   when it is minimized. The renderer has already presented an empty frame, so skip drawing.
2. Fill a `v3d::render::realtime::Canvas` with rectangles.
3. Submit it with `renderer_->quads()->submit(canvas_, pass.get())`.
4. Call `renderer_->renderFrame()`.

[rendering/](rendering/README.md) covers drawing in full.

## 5. The config

`data/config.json` lists typed config files. It does not hold settings itself.

```json
{
	"configs": [
		{ "type": "window", "file": "window.json" }
	]
}
```

`data/window.json`:

```json
{
  "window": { "width": 1024, "height": 768 }
}
```

The api reads these types: `window`, `binding`, `ui`, `sound`, `camera`, `layout` and `sprite`.
A type the api does not read is kept for the app to look up by name. An entry with no `type` or
no `file`, or a file that does not load, stops startup with an error in the log.
[engine/](engine/README.md) describes each type.

## 6. Build and run

```
cmake -S . -B out/build/x64-Debug -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE=%CD%/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug
```

- **Give the toolchain file as an absolute path.** Otherwise CMake can report "Could not find
  toolchain file" before it has found a compiler.
- **The first configure installs every package in your manifest.** With a warm vcpkg binary
  cache this takes a couple of minutes. With a cold cache it takes about 45 minutes, mostly
  building boost.

### Checking that it works

Run the executable and open `v3d.log` beside it. When the Khronos validation layer from the
Vulkan SDK is installed, the renderer turns it on and writes its messages to this log. A clean
start looks like this:

```
[info] Creating window 1024 x 768
[info] Created vulkan instance with 3 extension(s), validation on
[info] Using vulkan device <your gpu>
[info] Created a vulkan swapchain of 3 images at 1024 x 768
```

**No validation messages after these lines means a clean run.** The layer writes nothing when it
finds nothing.

vertical3d's own CI draws with a software Vulkan driver and checks some output exactly, but it
cannot check your app. Run it and read the log.

## Keeping up with the tree

The submodule commit is your version of vertical3d. There are no releases and no compatibility
checks. To update:

```
git -C vendor/vertical3d pull
```

Then rebuild. A breaking change appears as a compile error in your build. After updating, check
two files:

- **`vcpkg-configuration.json`.** If vertical3d's baseline changed, copy it again.
- **`vcpkg.json`.** If vertical3d's package list changed, update yours. Yours is the one that is
  installed.

The api is still changing (see [the roadmap](../roadmap) and [plans/](../plans)), so expect to
update a call site now and then.

## Changing the tree from a consumer

**Do not commit inside `vendor/vertical3d`.** A submodule is a full clone and will accept a
commit, but the change is hidden from both projects, and the commit you record may not exist
anywhere else.

The change is also untested there. Your build adds only the api libraries you selected. It does
not build vertical3d's apps or any of its test suites.

Make the change in the vertical3d repository, on a branch, and build and test it there. To try the
branch against your app, check it out in your submodule and leave the submodule change
uncommitted. [CONTRIBUTING.md](../../CONTRIBUTING.md) gives the steps, including how to move a
commit between two local clones.
