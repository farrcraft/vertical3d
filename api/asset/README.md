# asset

Finds files under a data directory and loads them through registered loaders. Holds the JSON and
text loaders, the manifest an app lists its documents in, atomic file writes and document
migration.

Its CMake target is `v3dlib_asset`. A project outside the tree links it as `v3d::asset`.

- [api/Assets.md](../../docs/api/Assets.md) describes how to use it.
- [api/README.md](../../docs/api/README.md) lists every library and what each one links.
