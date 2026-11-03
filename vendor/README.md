# vendor

Third-party code that does not come from vcpkg. Nothing here is edited as part of the tree.

- `libnoise/` is a git submodule, which only voxel links. It is built separately, before voxel.
- `vcpkg/` is a clone of vcpkg, which git ignores. The build uses its toolchain file.

[docs/contributing/Dependencies.md](../docs/contributing/Dependencies.md) covers both, with the
steps to build libnoise.
