# cmake

The CMake modules the root `CMakeLists.txt` includes.

- `v3dApiLibraries.cmake` is the manifest of api libraries: the directory of each, the api
  libraries it links, and the packages it names. A project that nests the tree selects libraries
  from it.
- `v3dDependencies.cmake` finds the packages the selected libraries need.
- `v3dHelpers.cmake` defines the functions every target is declared with, such as
  `v3d_add_api_library`.

[docs/contributing/Build.md](../docs/contributing/Build.md) covers the CMake layout, the options
and the helper functions. [docs/api/UsingTheApi.md](../docs/api/UsingTheApi.md) covers selecting
libraries from another project.
