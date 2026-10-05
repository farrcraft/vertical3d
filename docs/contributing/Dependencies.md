# Dependencies

This document is for contributors. It lists the third-party packages the tree uses, says where
each one comes from, and explains how to add or update one. [Build.md](Build.md) covers what the
build does with them. A project in another repository that uses the api manages its own
packages, as described in [UsingTheApi.md](../api/UsingTheApi.md#2-the-vcpkg-manifest).

- [Packages from vcpkg](#packages-from-vcpkg)
- [The Vulkan SDK](#the-vulkan-sdk)
- [libnoise](#libnoise)
- [Setting up vcpkg](#setting-up-vcpkg)
- [Adding a package](#adding-a-package)
- [Updating versions](#updating-versions)

## Packages from vcpkg

vcpkg installs these in manifest mode, from [vcpkg.json](../../vcpkg.json). The packages are
installed into `out/build/<config>/vcpkg_installed/` during the first configure.

| Port | Notes |
|---|---|
| boost-filesystem, boost-foreach, boost-headers, boost-json, boost-lexical-cast, boost-optional, boost-program-options, boost-smart-ptr, boost-system, boost-test, boost-unordered | One port per boost library the tree uses. The manifest does not use the `boost` metapackage. See [Adding a boost library](#adding-a-boost-library) |
| cgltf | A single header with no CMake config. [Build.md](Build.md#traps) says how it is found and where its implementation is compiled |
| entt | |
| freetype | |
| glm | |
| libjpeg-turbo | |
| libpng | |
| sdl3 | **Must have its `vulkan` feature.** Without it SDL is built without Vulkan support, and `SDL_Vulkan_LoadLibrary` fails at startup with "No dynamic Vulkan support in current SDL video driver (windows)" |
| sdl3-mixer | `v3dlib_audio` plays sound through it. It needs SDL 3.4.0 or newer |
| spdlog | |
| vulkan | |
| vulkan-memory-allocator | Used by `memory::Allocator` when an app turns on suballocation. It is a single header. Its implementation is compiled once, in `api/render/realtime/vulkan/memory/VmaImpl.cxx`, and it is linked PRIVATE because no public header includes it |

The configure looks only for the packages that the selected api libraries need. A build of this
repository always selects every library, so it needs all of them. Boost is always looked for,
because every api library links `Boost::headers`.

**boost::json has no `error_code` or `system_error` of its own** in the boost version the tree
uses. Include `<boost/system/error_code.hpp>` and `<boost/system/system_error.hpp>` and use the
`boost::system` types. [api/asset/loader/Json.cpp](../../api/asset/loader/Json.cpp) parses with a
`boost::system::error_code` and is the example to copy.

There is no OpenGL, and no package for it.

Background: [ADR-0021](../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0053](../adr/0053-memory-optional-vma-suballocation.md)

## The Vulkan SDK

The [Vulkan SDK](https://vulkan.lunarg.com/) is installed separately, not through vcpkg. Its
installer sets `VULKAN_SDK`, and the build uses that variable to find it.

**Building this repository needs the SDK.** Two parts of the configure use it:

- `find_package(Vulkan)` runs when the selected libraries include `render`. A build of this
  repository selects every library, so it always runs.
- `glslc`, the SDK's shader compiler, compiles shaders at build time. `v3d_add_shader` looks for
  it on its first call and stops the configure if it is missing. `api/render`, the render device
  tests and voxel call it.

A project in another repository needs the SDK only if the api libraries it selects include
`render`, directly or through `engine` or `ui`. See
[UsingTheApi.md](../api/UsingTheApi.md#selecting-libraries).

The SDK also provides the Khronos validation layer, which the renderer turns on when it is
installed. [Testing.md](Testing.md#verifying-a-rendering-change) explains how to use it.

## libnoise

[libnoise](https://github.com/eXpl0it3r/libnoise) is an unofficial fork of libnoise with CMake
support. It is the tree's only git submodule, at `vendor/libnoise`. Only voxel and voxel's test
suite link it, and neither links until libnoise has been built.

### Building libnoise

Build it out of source, with the commands below, from the repository root in a developer
environment. Replace `<repo>` with the absolute path of your clone.

```
cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug
cmake --build vendor/libnoise/build-ninja
```

- **Do not build in the libnoise source directory.** libnoise commits a `CMakeCache.txt` that
  names a "Visual Studio 17 2022" generator. Reusing that cache is the usual cause of a failed
  libnoise build.
- `CMAKE_POLICY_VERSION_MINIMUM=3.5` is required because libnoise declares
  `cmake_minimum_required(VERSION 3.0)`, which current CMake rejects.
- The archive must land in `vendor/libnoise/Debug`, which is where voxel's
  `target_link_directories` looks.

## Setting up vcpkg

`vendor/vcpkg/` is listed in `.gitignore`, so a clone of this repository does not include
vcpkg. Clone and bootstrap it once:

```
git clone https://github.com/Microsoft/vcpkg.git vendor/vcpkg
.\vendor\vcpkg\bootstrap-vcpkg.bat
```

For Visual Studio's CMake integration, also run:

```
.\vendor\vcpkg\vcpkg.exe integrate install
```

You do not need to run `vcpkg install` yourself. The first configure installs the manifest's
packages into the build directory. A `vcpkg install` run from the repository root installs a
second copy into `vcpkg_installed/` at the root, which the build does not use.

CI uses the vcpkg that the GitHub Windows runner image provides, not `vendor/vcpkg`.

## Adding a package

From a developer command prompt:

```
.\vendor\vcpkg\vcpkg.exe add port libpng
```

The package is installed at the next configure. Then:

1. Add the `find_package` call to [cmake/v3dDependencies.cmake](../../cmake/v3dDependencies.cmake),
   inside `v3d_find_packages` if the package needs anything other than a plain
   `find_package(<name> REQUIRED)`.
2. Link it from the api library that uses it, PUBLIC or PRIVATE as
   [Build.md](Build.md#linking-rules) describes.
3. Add it to that library's `PACKAGES` list in
   [cmake/v3dApiLibraries.cmake](../../cmake/v3dApiLibraries.cmake). If the package provides an
   imported target, add the target to `V3D_PACKAGE_TARGETS` as well. The configure fails if the
   manifest and the link lines disagree.
4. Add the port to [examples/starter/vcpkg.json](../../examples/starter/vcpkg.json). That file is
   kept identical to the root one.

### Adding a boost library

The manifest lists one `boost-*` port per boost library, so a boost header with no port in the
manifest does not compile. To include `<boost/signals2.hpp>`, add `boost-signals2` to
[vcpkg.json](../../vcpkg.json) and to [examples/starter/vcpkg.json](../../examples/starter/vcpkg.json).

A compiled boost library needs one more step. Add it to the `COMPONENTS` list of the
`find_package(Boost ...)` call in [cmake/v3dDependencies.cmake](../../cmake/v3dDependencies.cmake).
That creates the `Boost::<component>` target that `target_link_libraries` can name. All the
components are found in that one call.

A header-only boost library needs no component. Every api library links `Boost::headers`, which
carries the include directory for all of boost.

Boost is resolved in config mode, using the `BoostConfig.cmake` that vcpkg installs. FindBoost's
input variables, such as `Boost_USE_STATIC_LIBS`, have no effect. The `x64-windows` triplet
builds boost as shared libraries.

## Updating versions

To move every package to newer versions, update the baseline and configure again:

```
.\vendor\vcpkg\vcpkg.exe x-update-baseline
```

**The baseline is in [vcpkg-configuration.json](../../vcpkg-configuration.json)**, not in
`vcpkg.json`. It names a commit of the microsoft/vcpkg repository, not the ports in your local
`vendor/vcpkg`. A port that the baseline commit does not contain fails with "the baseline does
not contain an entry for port X", even when `vendor/vcpkg/ports/X` exists.

Copy the new baseline into
[examples/starter/vcpkg-configuration.json](../../examples/starter/vcpkg-configuration.json) as well.

A new baseline usually reinstalls most packages, including boost. See [Build.md](Build.md#traps)
for the cost.
