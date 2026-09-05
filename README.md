# Vertical3D

[![ctest](https://github.com/farrcraft/vertical3d/actions/workflows/ctest.yml/badge.svg)](https://github.com/farrcraft/vertical3d/actions/workflows/ctest.yml)
[![cpplint](https://github.com/farrcraft/vertical3d/actions/workflows/cpplint.yml/badge.svg)](https://github.com/farrcraft/vertical3d/actions/workflows/cpplint.yml)

A monorepo for the Vertical3D ecosystem: a set of C++ libraries for building 3D applications and
games, and the applications I have written against them over the years. Rendering is Vulkan 1.3,
windowing and input are SDL3, and the whole tree builds with CMake.

Windows and MSVC in practice. Nothing here is packaged or released — it is built from source.

## Layout

The libraries live under [api/](api/), one target per subdirectory, each named `v3dlib_<name>` and
namespaced to match its path (`v3d::render::realtime`, `v3d::asset`, and so on).

| | |
|---|---|
| [`render`](api/render/) | The Vulkan realtime renderer — frames, passes, draw items, and the quad and line primitives |
| [`engine`](api/engine/) | The game engine: main loop, window, asset manager, config and input |
| [`ui`](api/ui/) | Menus, toolbars, themes and the component renderer |
| [`brep`](api/brep/) | Boundary representation meshes, half-edge and winged-edge |
| [`type`](api/type/) | Math and geometry — vectors, cameras, rays |
| [`dag`](api/dag/) | Scene graph nodes and transforms |
| [`image`](api/image/) | Image readers and writers — png, jpeg, tga |
| [`font`](api/font/) | Glyph layout and atlas packing |
| [`asset`](api/asset/) | Loading and resolving files by type |
| [`config`](api/config/) | JSON configuration documents |
| [`event`](api/event/) | Input mapping and command dispatch |
| [`input`](api/input/) | Keyboard and mouse state |
| [`audio`](api/audio/) | Sound playback over SDL3_mixer |
| [`ecs`](api/ecs/) | Components and systems over entt |
| [`log`](api/log/) | A thin wrapper over spdlog |

The applications sit at the top level, each consuming some subset of those.

## Building

You will need Visual Studio 2022 or newer with the MSVC toolchain, the
[Vulkan SDK](https://vulkan.lunarg.com/) with `VULKAN_SDK` set, CMake, and Ninja. Dependencies come
from vcpkg through a manifest, so the first configure installs them — budget around 45 minutes for a
cold install, most of it building boost.

Visual Studio configures the tree directly from [CMakeSettings.json](CMakeSettings.json). From a
shell, use a developer environment (`vcvars64.bat`) and then:

```
cmake -S . -B out/build/x64-Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=vendor/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows
ninja -C out/build/x64-Debug              # everything
ninja -C out/build/x64-Debug pong         # one target
```

Shaders are compiled to SPIR-V at build time by `glslc` and embedded into the binaries rather than
shipped alongside them, which is why the Vulkan SDK is needed to configure and not just to run.

`voxel` additionally needs libnoise, the one vendored submodule, built separately. See
[docs/Dependencies.md](docs/Dependencies.md) for that and for adding or updating a dependency.

### Tests

Boost.Test, one binary per library and per app that has logic worth covering, registered with ctest
and run in CI on every push.

```
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite
```

### Lint

cpplint, also run in CI. The invocation and the one check whose filter no longer applies are
described in [docs/Linting.md](docs/Linting.md).

## Applications

| | |
|---|---|
| **Vertical3D** | A 3D modelling tool. Four viewports over a construction grid, with primitives, picking, manipulators, undo, menus, toolbars and project persistence. The most complete thing here. |
| **Voxel** | A Minecraft-style voxel terrain generator, with chunked meshing over perlin noise. |
| **Pong** | Pong, with sound and single-player AI. |
| **Tetris** | Tetris. |
| **Odyssey** | A tile-based roguelike. Opens a window and draws a sprite; the game itself is not written yet. |
| **Moya** | A RenderMan-compatible Reyes renderer — micropolygon grids, buckets and framebuffers. Builds as a library with tests, and has no driver. |
| **imagetool** | A small CLI over `api/image`. Reads an image and reports its dimensions and depth; it parses an `--outfile` but the write path is unreachable. |
| **Talyn** | An intended raytracer. Currently a framebuffer, a render context, a RIB reader and a long list of unstarted work. |
| **v3dshell** | An intended REPL for driving Vertical3D. Currently an empty `main`. |

Pong, Tetris, Voxel, Odyssey and Vertical3D all run.

## Documentation

* [docs/sdlc.md](docs/sdlc.md) — how work moves through the repo, and what "verified" means here
* [docs/adr/](docs/adr/) — architecture decision records, and why things are shaped as they are
* [docs/RenderingPipeline.md](docs/RenderingPipeline.md) — the render chain from window to draw item
* [docs/Dependencies.md](docs/Dependencies.md) — vcpkg, submodules, and adding a dependency
* [docs/TODO.md](docs/TODO.md) — what is loose
* [CLAUDE.md](CLAUDE.md) — orientation for coding agents, and a fair summary of the tree's traps

## History

Much of this code traces back to the early aughts, before what we would now call modern C++. There
was a single namespace, every library was its own repo and package, and the build used autotools on
Unix and Visual Studio solutions on Windows. That is a modular design, and it has worked well for
larger OSS projects that split across many small packages — but it is a great deal more to maintain.

The current iteration pulls everything into one repo, replaces both build systems with CMake, makes
the namespaces granular, and targets C++17 or newer. Rendering moved from OpenGL to Vulkan and
windowing from SDL2 to SDL3. That work is recorded in
[docs/plans/completed/Modernization.md](docs/plans/completed/Modernization.md), whose six phases
closed on 2026-09-04 along with the last of the four legacy trees.

Expect wide variance in how modern any given file is.
