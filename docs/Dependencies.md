# Dependencies

## From vcpkg

Through the manifest in [vcpkg.json](../vcpkg.json):

| Port | |
|---|---|
| boost | See the note below on `boost::json` |
| cgltf | A single header the port copies into `include/`, with no CMake config of its own. [Build.md](Build.md#traps) covers how it is found and where its implementation half is compiled |
| entt | |
| freetype | |
| glm | |
| libjpeg-turbo | |
| libpng | |
| sdl3 | **With its `vulkan` feature**, which is required. Without it SDL builds with `SDL_VULKAN=OFF` and `SDL_Vulkan_LoadLibrary` fails at startup with "No dynamic Vulkan support in current SDL video driver (windows)" |
| sdl3-mixer | What `v3dlib_audio` is built on, per [ADR-0021](adr/0021-sdl3-mixer-replaces-soloud.md). It needs SDL >= 3.4.0, which is why the vcpkg baseline moved |
| spdlog | |
| vulkan | |

**boost 1.91 removed `boost::json::error_code` and `boost::json::system_error`.** Name
`boost::system` and include `<boost/system/error_code.hpp>` and
`<boost/system/system_error.hpp>` directly. [api/asset/JsonFile.h](../api/asset/JsonFile.h) is
where the tree's json error handling lives.

## Not from vcpkg

- **The Vulkan SDK.** Every configure needs it whether or not the build will draw:
  `find_package(Vulkan)` is unconditional, and `v3d_add_shader` looks for `glslc` with a
  `FATAL_ERROR` on the first shader it is asked to compile, because shaders are compiled at
  build time and embedded as SPIR-V. `VULKAN_SDK` has to point at an install.
- **[libnoise](https://github.com/eXpl0it3r/libnoise)**, an unofficial fork that adds CMake
  support. It is a git submodule, built separately, and it is the only submodule left. Only
  voxel links it. See [Building libnoise](#building-libnoise) below; voxel's
  `target_link_directories` expects its artefacts under `vendor/libnoise/Debug`.

There is no OpenGL. `api/gl` was deleted on 2026-09-01, and the `find_package(OpenGL)` and
`find_package(GLEW)` calls and the `glew` port went with it. See
[ADR-0001](adr/0001-vulkan-replaces-opengl.md).

## Setting up vcpkg

vcpkg needs a small triplet patch first, for a boost log ABI problem. The bug and the origin of
the workaround are discussed at https://github.com/microsoft/vcpkg/discussions/22762.

Clone it:

```
mkdir vendor
cd vendor
git clone https://github.com/Microsoft/vcpkg.git
```

Apply this diff in the vcpkg directory before anything else:

```
diff --git a/triplets/x64-windows.cmake b/triplets/x64-windows.cmake
index d0be7297f..86fcd4207 100644
--- a/triplets/x64-windows.cmake
+++ b/triplets/x64-windows.cmake
@@ -2,3 +2,5 @@ set(VCPKG_TARGET_ARCHITECTURE x64)
 set(VCPKG_CRT_LINKAGE dynamic)
 set(VCPKG_LIBRARY_LINKAGE dynamic)

+set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} /DBOOST_ALL_DYN_LINK /DBOOST_USE_WINAPI_VERSION=0x0A00 /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00")
+set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} /DBOOST_ALL_DYN_LINK /DBOOST_USE_WINAPI_VERSION=0x0A00 /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00")
```

Then carry on as normal:

```
# Prepare to use vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
cd ..
.\vendor\vcpkg\vcpkg.exe install

# Integrate this installation with Visual Studio (CMake will not work in VS otherwise)
.\vendor\vcpkg\vcpkg.exe integrate install
```

## Adding a dependency

Open a developer command prompt and run, for example:

```
.\vendor\vcpkg\vcpkg.exe add port boost
```

Packages are installed during CMake generation.

## Updating versions

To get a newer boost, say, update the baseline and re-run the install:

```
.\vendor\vcpkg\vcpkg.exe x-update-baseline
.\vendor\vcpkg\vcpkg.exe install
```

**The baseline is pinned in [vcpkg-configuration.json](../vcpkg-configuration.json)**, not in
`vcpkg.json`, and it names a commit of microsoft/vcpkg rather than the `vendor/vcpkg` ports
tree on disk. A port missing from that commit fails with "the baseline does not contain an
entry for port X" even when `vendor/vcpkg/ports/X` exists.

Editing `vcpkg.json` re-runs the manifest install. A cold install builds boost from source and
takes roughly 45 minutes; changing the sdl3 feature set rebuilds SDL only, about five minutes.

## Submodules

Submodules live in `vendor/` and are cloned and built individually. To add one:

```
git submodule add https://github.com/eXpl0it3r/libnoise vendor/libnoise
```

### Building libnoise

Build it **out of source**. The `CMakeCache.txt` libnoise commits names a "Visual Studio 17
2022" generator that is not necessarily installed, and reusing that cache is the usual reason a
build of it fails. `CMAKE_POLICY_VERSION_MINIMUM` is needed because its
`cmake_minimum_required(VERSION 3.0)` predates what current CMake accepts.

```
cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug
cmake --build vendor/libnoise/build-ninja
```

libnoise is not prebuilt in the tree, and `voxel` will not link without it.

## Consuming the api from another repository

An application outside this tree takes it as source, per
[ADR-0027](adr/0027-the-api-is-consumed-as-source.md), and [NewProject.md](NewProject.md) is
the walkthrough. Two points about it belong here, because they are about dependencies:

- **The consumer's `vcpkg.json` is the one that gets installed.** Manifest mode reads the root
  project's manifest, and once vertical3d is nested that is the consumer's. Copy
  [vcpkg.json](../vcpkg.json) across; the api's dependencies are not resolved from the tree's
  own.
- **`vendor/vcpkg/` is gitignored here**, so cloning this repository does not bring a vcpkg with
  it. A consumer clones its own.

The baseline in [vcpkg-configuration.json](../vcpkg-configuration.json) has to be copied
verbatim into the consumer's, and nothing checks that it was. A boost library's file name
carries its version, so a drifted baseline shows up as a link error rather than a warning.
