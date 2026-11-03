# api

The Vertical3D libraries. Each directory here is one library, except that `asset/media`,
`interchange/usd` and `render/offline` are libraries inside another one's directory. A library's
CMake target is `v3dlib_<name>`, and a project outside the tree links it as `v3d::<name>`.

- [docs/api/README.md](../docs/api/README.md) lists every library, what it is for and what it
  links, and says what to read next.
- [docs/api/UsingTheApi.md](../docs/api/UsingTheApi.md) covers using the libraries from another
  repository.
- [cmake/v3dApiLibraries.cmake](../cmake/v3dApiLibraries.cmake) is the manifest that names them.
