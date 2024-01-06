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


# Packages

Install vcpkg from the installation instructions: https://vcpkg.io/en/getting-started

```
mkdir vendor
cd vendor
git clone https://github.com/Microsoft/vcpkg.git
```

Apply this diff in the vcpkg directory:

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

Condinue with running:

```
# Prepare to use vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
cd ..
.\vendor\vcpkg\vcpkg.exe install

# Integrate this installation of vcpkg with Visual Studio (CMake won't work in VS otherwise)
.\vendor\vcpkg\vcpkg.exe integrate install
```

## Adding Dependencies

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
