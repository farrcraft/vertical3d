# ADR-0123: Interchange: file readers fill a neutral scene

**Status**: accepted
**Date**: 2026-10-27
**Documented in**: [api/Interchange.md](../api/Interchange.md)

## Context

The editor imports glTF and USD as scenes of items, and an app links only api libraries, so
the code that reads either format lives in the api. Both formats describe the same things: a
hierarchy of placed nodes, polygon meshes, cameras, lights and keyed values. The editor's items
are its own, and no api library can make them. OpenUSD is far larger than anything else in the
tree, and the api libraries that need neither Vulkan nor SDL also build on Linux with Clang.

## Decision

`v3dlib_interchange` holds a scene that belongs to no format, and a glTF reader that fills it.
`v3dlib_interchange_usd` holds the USD reader, fills the same scene, and is the only library that
links OpenUSD. The editor turns that scene into items, in one place for every format.

## Alternatives

### One library with both readers
- **For**: one library to link and to document.
- **Against**: anything that reads glTF builds and ships OpenUSD, and so does the Linux build.
- **Rejected because**: a dependency of that size belongs only where it is used.

### USD as the neutral model
- **For**: USD already describes everything the editor imports. A USD file-format plugin reads
  glTF into a stage, so the editor would map one format.
- **Against**: every import, glTF included, depends on OpenUSD and on a third-party plugin, and
  the api's own glTF reading goes unused.
- **Rejected because**: glTF import should not need the heaviest dependency in the tree.

### A scene kind loaded through the asset manager
- **For**: the manager already finds a loader by extension and runs it off the main thread.
- **Against**: `.gltf` already goes to the model loader, and a load picks its loader by
  extension, so the same file could not also be read as a scene.
- **Rejected because**: a file read two ways does not fit a manager that picks one loader per
  extension.

## Consequences

- **Gains**:
  - The editor has one mapping onto its items, whatever the format.
  - A further format is a reader that fills the scene, in a library of its own if its
    dependency is large.
  - Only the editor, and whatever else imports USD, carries OpenUSD.
- **Costs**:
  - The neutral scene holds only what both formats and the editor share, so a feature of one
    format waits until the scene can describe it.
  - cgltf's implementation is compiled once per program, so it moves into `v3dlib_interchange`,
    and the model loader in `v3dlib_asset_media` links interchange to reach it.
  - The games' glTF loader and the interchange reader both walk glTF nodes, for different ends.
- **Revisit when**: the scene has to carry most of what USD can say, at which point USD itself
  is the better neutral model.
