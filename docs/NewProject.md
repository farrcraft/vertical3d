# Starting A Project Against The api

How to stand up an application in its own repository that uses the `api/` libraries.
[ADR-0027](adr/0027-the-api-is-consumed-as-source.md) settles the shape: the api is taken as
**source**, not as an installed package. There is no `find_package(vertical3d)` and nothing to
install. The consumer nests this repository with `add_subdirectory` and builds it with its own
compiler.

[examples/starter/](../examples/starter/) is the finished result of everything below, kept in
the tree and built by CI on every push. Copy it rather than typing this out. The walkthrough
exists to explain the pieces.

First, though: **every directory in the vertical3d root already builds**, so adding an app
there costs nothing. A second repository is worth it when the split itself matters: a
different release cadence, a different licence, a collaborator who should not have the editor
sources. If the reason is only tidiness, add a directory to the monorepo and stop here.

## What you need

- **MSVC.** Windows only, in practice. Work from a developer environment (`vcvars64.bat`).
- **CMake 3.21 or newer**, and **Ninja**.
- **The Vulkan SDK**, with `VULKAN_SDK` pointing at it. This is not optional even for an app
  that draws nothing: `add_subdirectory` builds the whole api, `api/render` is part of it, and
  the SDK's `glslc` compiles its shaders at build time.
- **A vcpkg clone of your own.** `vendor/vcpkg/` is *gitignored* in this repository, so cloning
  vertical3d does not bring you one.

## 1. The repository

```
git init myapp
cd myapp
git submodule add https://github.com/<you>/vertical3d vendor/vertical3d
git clone https://github.com/microsoft/vcpkg vendor/vcpkg
```

vertical3d has a submodule of its own, libnoise, which only `voxel` needs. `V3D_BUILD_APPS` is
off for you, so voxel is never configured and you can leave libnoise uncloned.

Your layout ends up as:

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

**Copy both files from the tree.** Manifest mode reads the manifest belonging to the *root*
project, which is now yours. vertical3d's own `vcpkg.json` is never consulted once it is
nested, so every package the api needs has to be in yours.

Copy [vcpkg.json](../vcpkg.json) as it stands, `sdl3`'s `vulkan` feature included. Without that
feature `SDL_Vulkan_LoadLibrary` fails at startup with "No dynamic Vulkan support in current
SDL video driver (windows)", which surfaces as an unhandled exception rather than a build
failure.

[vcpkg-configuration.json](../vcpkg-configuration.json) matters more, and getting it wrong
produces no clear error: **copy the `baseline` commit verbatim.** A boost library's file name
carries its version, so a baseline that has drifted from the tree's is a different boost, and
the result is a link error a long way from its cause. Nothing checks that the two agree.

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

Six things in that are worth knowing rather than copying.

**The standard is stated, not flagged.** `CMAKE_CXX_STANDARD 23` maps to `/std:c++latest` on
MSVC. Writing `add_compile_options("/std:c++latest")` instead gets you `warning D9025:
overriding '/std:c++17' with '/std:c++latest'` on every file, because glm and EnTT require
`cxx_std_17` through their interfaces and CMake answers that by putting `/std:c++17` on the
command line beside your flag. The standard and `/permissive-` are the only compiler settings
you supply. The include root, `/EHsc`, `/utf-8` and the boost winapi version definitions all
arrive through the `v3d::` targets.

**`V3D_BUILD_APPS` and `V3D_BUILD_TESTS` go off**, or you build pong, tetris, the editor, both
offline renderers and 22 test binaries alongside your app. They already default to off when
vertical3d is not the top level project; setting them explicitly documents the intent.

**`V3D_LIBRARIES` names the api libraries you link.** Their closure is what gets built and
what decides which packages are looked for, per
[ADR-0033](adr/0033-a-consumer-selects-the-api-libraries-it-wants.md), so an app that wants
only `v3d::image` needs no Vulkan SDK and no SDL3 installed. Leave it out and you get `all`,
which is every library and every package. It has to be `all` if you turned the apps or the
tests back on.

**Targets are `v3d::<library>`**, one alias per directory under `api/`. Name only what you use.
Each library declares what it needs, so `v3d::engine` brings `v3d::asset`, `v3d::config`,
`v3d::event`, `v3d::input` and `v3d::render` with it. The underlying `v3dlib_*` names are what
the tree links internally and are not the interface.

**A third party package you name yourself needs your own `find_package`, after the
`add_subdirectory`.** The list above has none, because everything these three targets need
arrives through them: boost, glm, EnTT, SDL and Vulkan included. Naming one directly is
different. `find_package` creates imported targets in the directory that called it and below,
so nothing vertical3d resolved is visible in your scope, and naming `Boost::program_options`
without your own `find_package(Boost)` fails with *"Target myapp links to
Boost::program_options but the target was not found"*. Make the call after the
`add_subdirectory`, so you resolve the boost the api already did: the tree's `find_package`
leaves `Boost_DIR` in the cache and yours then hits that same package. Nothing else about the
tree's resolution reaches you - its variables are set in its own directory scope, which is a
child of yours.

**`v3d_add_app_data` is available to you.** A CMake function is global once defined, so both
data helpers work in your project. This one copies your `data/` beside the executable, which is
where the engine looks: it resolves every asset relative to the exe, not to the working
directory. `v3d_add_shared_data(myapp)` adds the tree's own `data/`, the shared fonts and
themes, which you want as soon as you draw text and not before.

## 4. The application

An app is a subclass of `v3d::engine::Engine` that overrides `tick`, `render` and `shutdown`,
plus a `main` that drives it. [examples/starter/src/](../examples/starter/src/) is a complete
working one. The parts that are not obvious:

**`main` is one line.** `v3d::engine::run<AppEngine>(argv[0], "myapp")` from
`<api/engine/Application.h>` derives the path every asset resolves against from `argv[0]`,
drives `initialize()` and `eventLoop()` inside a try block that logs what a renderer threw, and
calls `shutdown()` outside it. A windowed app has no console, so an uncaught exception is
otherwise an abort dialog with nothing in it. [ADR-0028](adr/0028-an-apps-shell-belongs-to-the-api.md)
covers what else an app does not have to write: `v3d::ui::paint::TextRenderer` for a font and its
glyphs, and `v3d::ui::shell::GameMenu` for a menu the escape key puts up.

**Includes are angle-bracketed and start at `api/`**: `#include <api/engine/Engine.h>`. Files
inside the tree reach each other by relative path (`../../api/engine/Engine.h`), so the same
header has two spellings depending on which side of the boundary you are on. Yours is the
first. If you run cpplint over your own sources, note that it reads an angle-bracketed `.h` as
a C system header, so the api includes sort *above* `<string>` rather than below it.

**`initialize` takes a feature bitmask**, and constructs only what is asked for.
`Feature::Window | Feature::KeyboardInput | Feature::Config` is the useful minimum for
something with a window. `Feature::Config` is what reads `data/config.json`; without it you
still get a window, at its own default size rather than the configured one.

**The renderer is torn down before the base class.** `shutdown()` calls the render engine's
first, because the context owns the device that holds the window's surface alive and
`Window::destroy()` unloads the vulkan library. A surface released after that is never
destroyed, and the instance reports it leaked.

**A quit calls `quit()`, never `shutdown()`.** The loop ticks and renders after an event
handler returns, so tearing the window down inside one leaves the next frame drawing into a
destroyed window.

**Drawing is a canvas of quads submitted to a pass.** Open the frame with
`renderer_->beginFrame(&size)`, which returns false while the window has no area — it has
presented the frame empty, and a canvas with no area has no projection to build geometry
against. Then build a `realtime::Canvas`, submit it with
`renderer_->quads()->submit(canvas_, pass.get())`, and call `renderFrame()`. The engine has two
primitives, the batched quad and the line, and a rectangle, a sprite, a glyph and a menu panel
are all the first one.

## 5. The config

`data/config.json` uses the indirect form, and only the indirect form: a list of typed files
rather than the settings themselves.

```json
{
	"configs": [
		{ "type": "window", "file": "window.json" }
	]
}
```

```json
{
  "window": { "width": 1024, "height": 768 }
}
```

The recognised types are `window`, `binding`, `ui` and `sound`. Every lookup but one is guarded
and logs a `false` when it fails. **The window is the exception**: `initialize` reads `width`
and `height` with `at()`, so a `window.json` naming neither throws rather than falling back.

## 6. Build and run

```
cmake -S . -B out/build/x64-Debug -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE=%CD%/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug
```

The toolchain file has to be an absolute path. A relative one is resolved against the source
directory, and CMake reports "Could not find toolchain file" before it has a compiler to report
it with. The first configure installs the whole package set: a couple of minutes against a warm
vcpkg binary cache, and roughly 45 minutes against a cold one, most of it building boost.

**How to tell it worked.** CI renders nothing, so a rendering change in this tree is verified
by running it and reading the log. The Khronos validation layer is enabled when installed and
routed through the logger, so `v3d.log` beside the executable should read:

```
[info] Creating window 1024 x 768
[info] Created vulkan instance with 3 extension(s), validation on
[info] Using vulkan device <your gpu>
[info] Created a vulkan swapchain of 3 images at 1024 x 768
```

**A silent run is the signal.** Validation says nothing when it has nothing to say, and without
that messenger a loaded layer looks exactly like a clean run.

## Keeping up with the tree

The submodule sha is your version. There is no compatibility check and no release. Moving it
forward is `git -C vendor/vertical3d pull` and a rebuild, and a break shows up as a compile
error in your own build. Two things to re-check when you do:

- **The vcpkg baseline**, if `vcpkg-configuration.json` changed in the tree. Copy it again.
- **The package list**, if `vcpkg.json` changed. Yours is the one that gets installed.

The api is still moving — see [the roadmap](roadmap/) and whatever is open in
[plans/](plans/) — so expect to fix a call site occasionally rather than to pin a version.
