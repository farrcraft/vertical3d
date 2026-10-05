# ADR-0010: Meshes: owned by the app that built them

**Status**: superseded
**Date**: 2026-08-31
**Superseded by**: [ADR-0065](0065-meshes-shared-registry-keyed-by-path.md)
**Documented in**: [api/rendering/TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md)

## Context

`pipeline::Resources` owns the pipelines, materials and textures a `DrawItem` names by handle.
A handle's slot is stable and comparable, so a sort key built from it means something
([ADR-0004](0004-rendering-submit-draw-items-as-data.md)). Geometry is the one thing a draw item
names that is not registered: it carries raw `VkBuffer`s. At the time, `Resources` never freed
an individual resource, while a streamed terrain chunk's mesh is built and discarded during
play. The sort key's fields are layer, pipeline, material and depth, so a mesh handle would sort
nothing.

## Decision

Geometry is owned by whatever built it. A mesh is a `memory::Mesh` held by a chunk, a model or
an app, and `DrawItem` keeps raw buffers. A draw item is valid only while the mesh it was filled
from is alive.

## Alternatives

### A mesh handle in `Resources`, beside the other three
- **For**: uniform. Everything a draw item names is a handle, resolved the same way.
- **Against**: `Resources` would have to free and reuse slots, which breaks the guarantee that a
  handle always means one thing. The sort key has no mesh field, so the handle buys nothing at
  record time.
- **Rejected because**: it pays the registry's whole cost for none of its benefit.

### A mesh cache in the api, keyed by what built the geometry
- **For**: removes duplicate meshes, and gives the api a place for a memory budget later.
- **Against**: a cache needs an eviction policy, and only the owner knows when a chunk mesh is
  dead. Building it first means guessing the policy.
- **Rejected because**: premature. A cache can be built later on top of owned meshes without
  changing this decision.

## Consequences

- **Gains**:
  - A chunk that goes out of range destroys its mesh and its device memory comes back.
  - `Resources` keeps every slot stable for the life of the context.
  - Nothing changes at record time.
- **Costs**:
  - The api cannot detect a draw item naming a mesh that has since been freed. This is safe only
    while items are built and recorded within one frame.
  - Two lifetime models in one frame: registered resources live as long as the context, meshes
    as long as their owner.
  - Nothing can sort or merge by geometry without a new sort key field.
- **Revisit when**: several owners load the same model and each builds its own copy.
  [ADR-0065](0065-meshes-shared-registry-keyed-by-path.md) answers that with a shared registry
  keyed by path, and [ADR-0061](0061-resources-explicit-release-generational-handles.md) removes
  the premise that `Resources` never frees.
