# ADR-0065: Mesh Registry — A Model Is Registered Once By Path, Named By Handle, And Released Explicitly

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

Amends [ADR-0010](0010-meshes-are-owned-by-the-app.md).

## Context

[ADR-0063](0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md) gives a lit
entity a mesh component that names a mesh by handle, and nothing in the api hands one out.
[ADR-0010](0010-meshes-are-owned-by-the-app.md) kept geometry out of `Resources`, because the
registry never freed and the sort key has no geometry field. It named a cache built over
app-owned meshes as the way to add sharing later, and called it premature until an app needed
it. Since then retcon's `MeshRegistry` has shown the need: ten props drawn from six files
share one upload each. [ADR-0061](0061-a-resource-is-released-explicitly.md) has also given
the api a handle that can be released. Nothing in the tree puts a `type::Model` on the device
at all. [LitScene](../plans/completed/LitScene.md) is the plan that needs this settled.

## Decision

**`realtime::MeshRegistry` turns a model into a `memory::Mesh` once per path and hands out a
`MeshHandle`**, a slot and a generation as ADR-0061's handles are. Its entry holds the mesh,
the albedo's texture and material from `renderer::Quad`, white for a flat surface, and the
base colour. An albedo is shared by every entry naming the same image. A handle is released
explicitly: the mesh goes to the ring, and the albedo goes when the last entry naming it does.
The vertex layout is `type::Model::Vertex`. The registry is not in `Resources`, and `DrawItem`
still carries raw buffers, filled from the entry when an entity is walked.

## Alternatives Considered

### Alternative 1: A fourth registry in `Resources`
- **Pros**: Every handle a draw names resolves through one owner.
- **Cons**: The sort key has no geometry field, so a mesh handle in `Resources` sorts nothing,
  which was ADR-0010's objection and is still true. A model also brings an albedo with it,
  which `Resources` cannot load.
- **Why not**: It is a slot in a registry for something the registry does nothing with.

### Alternative 2: Leave meshes to the app, as ADR-0010 has it
- **Pros**: No new type. The step from a model to a mesh is four lines an app can write.
- **Cons**: A component naming a mesh needs something to name it by. Every game would write
  the de-duplication, the albedo lookup and the release, which is the part that goes wrong.
- **Why not**: The walk ADR-0063 puts in the api has nothing to read without it.

### Alternative 3: retcon's registry as it is
- **Pros**: It works at retcon's scale.
- **Cons**: Its handle is a bare index that never goes stale because nothing is ever released,
  and it gives every textured mesh a new albedo slot, up to a fixed ceiling of 64.
- **Why not**: Released and shared are the two properties a world loaded by region needs.

### Alternative 4: A registry beside `Resources`, keyed by path, releasable — **chosen**
- **Pros**: One upload per file and one texture per image, and a stale handle resolves to
  nothing. Meshes an app builds itself, a voxel chunk say, are untouched by it.
- **Cons**: Two lifetimes for geometry: a registered mesh lives until it is released, and an
  app's own lives as long as its owner.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- `component::Mesh` names something, and a lit walk can resolve it.
- Props sharing an atlas share a material, so their draws sort together.

### Negative
- A model's vertex layout is the api's now. A game wanting another layout builds its own
  `memory::Mesh` and walks its own components.
- A model is one surface, per [ADR-0030](0030-a-model-is-an-interleaved-array-that-names-its-texture.md).
  A file whose parts need different materials is several registrations until milestone 5 splits
  a file by material.

### Risks
- A released handle still named by a component draws nothing, and nothing reports it. That is
  ADR-0061's stale handle in a new place, and the escape hatch is the same: resolve returns null,
  and a walk that wants to know can ask.
