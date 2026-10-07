# Examples

This directory is for someone who wants to see how an application outside this repository uses
the Vertical3D api. Each example is a small, complete application.

## What an example is

- **Its own root CMake project.** An example has its own `CMakeLists.txt` with a `project()`
  call. It is configured on its own, not as part of the repository's build.
- **A consumer of the api as source.** The example sets `V3D_LIBRARIES` to the api libraries it
  needs and then calls `add_subdirectory` on the repository root. Only those libraries, and the
  libraries they depend on, are built. It links them as `v3d::<name>` targets.
- **Outside the root build.** The root `CMakeLists.txt` does not list `examples/`, so building
  the repository does not build the examples.
- **Built in CI.** The `ctest` workflow configures and builds each example on every push. This
  checks that each api library declares its own include path and dependencies. Inside the
  repository's own build, a missing declaration can go unnoticed, because the root and the other
  apps supply it.

[docs/api/UsingTheApi.md](../docs/api/UsingTheApi.md) explains how to set up a project of your
own in the same way.

## Building an example

Use a developer command prompt (`vcvars64.bat`), as for the main build. Run these commands from
the repository root.

To reuse the dependencies the main build already installed, point the configure at its vcpkg
install and turn off the manifest install:

```
cmake -S examples/starter -B out/build/starter -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE=%CD%/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_INSTALLED_DIR=%CD%/out/build/x64-Debug/vcpkg_installed ^
  -DVCPKG_MANIFEST_INSTALL=OFF
cmake --build out/build/starter
```

This is how CI builds the examples. Without the last two options, the first configure installs
every package in the example's own `vcpkg.json`, which takes as long as a cold install of the
main build. [docs/contributing/Build.md](../docs/contributing/Build.md) covers the toolchain and
the Vulkan SDK.

## The examples

| Example | Shows |
|---|---|
| [starter/](starter/) | The smallest windowed app: an `Engine` subclass that opens a window from config and draws one rectangle each frame |
