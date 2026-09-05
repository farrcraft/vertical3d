# Dependencies

Managed by `vcpkg`, through the manifest in [vcpkg.json](../vcpkg.json):

- boost
- entt
- freetype
- glm
- libjpeg-turbo
- libpng
- sdl3, **with its `vulkan` feature** - without it SDL builds with `SDL_VULKAN=OFF` and `SDL_Vulkan_LoadLibrary` fails at startup with "No dynamic Vulkan support in current SDL video driver (windows)"
- sdl3-mixer - what `v3dlib_audio` is built on, per [adr/0021-sdl3-mixer-replaces-soloud.md](adr/0021-sdl3-mixer-replaces-soloud.md). It needs SDL >= 3.4.0, which is why the vcpkg baseline moved
- spdlog
- vulkan

Not from vcpkg:

- **The Vulkan SDK**, which every configure needs whether or not it will draw: the root CMakeLists calls `find_package(Vulkan)` and looks for `glslc` with a `FATAL_ERROR`, because shaders are compiled at build time and embedded as SPIR-V. `VULKAN_SDK` has to point at an install.
- [libnoise](https://github.com/eXpl0it3r/libnoise) - an unofficial fork that adds CMake support. A git submodule, built separately; only voxel links it, and it is the only submodule left.

Submodules are configured in the `vendor/` directory, and need to be cloned and built individually. See the Submodules section below. `link_directories` expects their artefacts under `vendor/*/Debug`.

There is no OpenGL: `api/gl` was deleted on 2026-09-01, and the `find_package(OpenGL)` and `find_package(GLEW)` calls and the `glew` port went with it. See [adr/0001-vulkan-replaces-opengl.md](adr/0001-vulkan-replaces-opengl.md).


# Packages

There are some weird boost log ABI compatibility issues out of the box and vcpkg needs a minor patch to make it work.
For more information about the bug and the origin of this workaround, see the comments at: https://github.com/microsoft/vcpkg/discussions/22762

Grab a copy of vcpkg:
```
mkdir vendor
cd vendor
git clone https://github.com/Microsoft/vcpkg.git
```


Before doing anything else, apply this diff in the vcpkg directory:

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

Continue as normal with running:

```
# Prepare to use vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
cd ..
.\vendor\vcpkg\vcpkg.exe install

# Integrate this installation of vcpkg with Visual Studio (CMake won't work in VS otherwise)
.\vendor\vcpkg\vcpkg.exe integrate install
```

## Adding New Dependencies

To add a new dependency, open a developer command prompt and run, e.g. for boost:
```
.\vendor\vcpkg\vcpkg.exe add port boost
```

Packages are automatically installed during CMake generation.

## Updating Dependency Versions

To update package versions, e.g. to get a newer boost version, update the baseline and then re-run install:

```
.\vendor\vcpkg\vcpkg.exe x-update-baseline
.\vendor\vcpkg\vcpkg.exe install
```

# Submodules

To add a new submodule:

```
git submodule add https://github.com/eXpl0it3r/libnoise vendor/libnoise
```

## Building libnoise

Build it **out of source**. The `CMakeCache.txt` libnoise commits names a "Visual Studio 17 2022"
generator that is not necessarily installed, and reusing that cache is the usual reason a build of it
fails. `CMAKE_POLICY_VERSION_MINIMUM` is needed because its `cmake_minimum_required(VERSION 3.0)`
predates what current CMake accepts.

```
cmake -S vendor/libnoise -B vendor/libnoise/build-ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL \
  -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=<repo>/vendor/libnoise/Debug
cmake --build vendor/libnoise/build-ninja
```

libnoise is not prebuilt in the tree, and `voxel` will not link without it.
