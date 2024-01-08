# Dependencies

- [libnoise](https://github.com/eXpl0it3r/libnoise) - This is an unofficial fork that adds CMake support.
- glm
- boost
- entt
- libjpeg
- libpng
- sdl2
- SoLoud
- FreeType2

Dependencies are managed using `vcpkg` when possible.  Otherwise they are configured as git submodules in the `vendor/` directory.
These submodules will need to be cloned and manually built individually.  See the Submodules section below for build instructions.


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


# Submodules

To add a new submodule:

```
git submodule add https://github.com/jarikomppa/soloud vendor/soloud
```

## Building SoLoud

SoLoud has a dependency on SDL2 which means that vcpkg-base dependencies need to be installed first.

```
cd vendor/soloud/contrib
cmake -B . -DSDL2_INCLUDE_DIR="..\..\..\vcpkg_installed\x64-windows\include" -DSDL2_LIBRARY="..\..\..\vcpkg_installed\x64-windows\lib"
MSBuild SoLoud.sln /p:IncludePath="..\..\..\vcpkg_installed\x64-windows\include\SDL2;$(IncludePath)"
```

## Building libnoise

```
cd vendor/libnoise
cmake -B .
MSBuild libnoise.sln
```
