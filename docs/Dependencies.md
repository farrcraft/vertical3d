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

To add a new dependency, open a developer command prompt and run, e.g. for boost:
```
.\vendor\vcpkg\vcpkg.exe add port boost
```

Packages are automatically installed during CMake generation.
