# ADR-0065: Meshes: shared registry keyed by path

**Status**: amended
**Date**: 2026-10-03
**Amended by**: [ADR-0082](0082-textures-owned-by-the-device-context.md)
**Supersedes**: [ADR-0010](0010-meshes-owned-by-the-app-that-built-them.md)
**Documented in**: [api/rendering/TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md)

## Context

[ADR-0063](0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md) gives a lit entity a
mesh component that names a mesh by handle, and something has to hand those handles out.
[ADR-0010](0010-meshes-owned-by-the-app-that-built-them.md) kept geometry out of `Resources`,
because that registry never freed and the sort key has no geometry field. Many props are drawn
from the same few files, and each file should be uploaded once and its textures shared.
[ADR-0061](0061-resources-explicit-release-generational-handles.md) gives the api handles that
can be released and that go stale safely.

## Decision

`realtime::MeshRegistry` uploads a model once per path and hands out a `MeshHandle`, a slot and
a generation as in ADR-0061. An albedo texture is shared by every entry that names the same
image; on release the mesh is retired through the ring, and an albedo goes with the last entry
that names it. The registry sits beside `Resources`, not in it, and a `DrawItem` still carries
raw buffers, filled from the entry.

## Alternatives

### A fourth registry in `Resources`
- **For**: Every handle a draw names resolves through one owner.
- **Against**: The sort key has no geometry field, so a mesh handle in `Resources` sorts
  nothing. A model also brings its albedo with it, which `Resources` cannot load.
- **Rejected because**: It is a slot in a registry that does nothing with it.

### Meshes stay the app's, as ADR-0010 has it
- **For**: No new type. Turning a model into a mesh is a few lines an app can write.
- **Against**: A mesh component needs something to name its mesh by. Every game would write the
  de-duplication, the albedo lookup and the release, which is the part that goes wrong.
- **Rejected because**: The draw walk ADR-0063 puts in the api would have nothing to read.

### A registry with plain index handles and nothing released
- **For**: Simple, and sufficient when everything loads at startup.
- **Against**: An index is only safe while nothing is released, and a texture per mesh
  duplicates shared images.
- **Rejected because**: A world loaded by region needs both release and sharing.

## Consequences

- **Gains**:
  - A mesh component names something, and a lit walk resolves it.
  - One upload per file and one texture per image. Props that share an atlas share a material,
    so their draws sort together.
  - Meshes an app builds itself, such as voxel chunks, are untouched.
- **Costs**:
  - The vertex layout of a registered model is the api's. A game wanting another layout builds
    its own `memory::Mesh` and walks its own components.
  - Geometry has two lifetimes: a registered mesh lives until released, and an app's own mesh
    as long as its owner.
  - A component naming a released handle draws nothing and nothing reports it, as with any
    stale ADR-0061 handle.
- **Revisit when**: draws need to sort or batch by geometry, as instancing would, which would
  give a mesh handle a reason to live in `Resources`.
