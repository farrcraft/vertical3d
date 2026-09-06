# ADR-0010: Geometry Ownership — Meshes Belong To The App, Not To Resources

**Date**: 2026-08-31
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`vulkan::Resources` owns the pipelines, materials and textures a `DrawItem` names by handle,
and [ADR-0004](0004-operations-as-draw-data.md) gives that indirection its reason: a handle's
registry slot is stable and comparable, so a sort key built out of one means something, where
a pointer sorts by whatever the allocator happened to return. Geometry is the one thing a
draw item names that is not in there — `DrawItem` carries raw `VkBuffer`s. Voxel is the first
app with geometry worth talking about, one mesh per chunk, so the question has to be settled
before the port rather than by whichever way the port happens to go.

The two properties that decide it are not the same for a mesh as for the other three.
`Resources` never frees an individual resource — the header says so — because a pipeline or a
texture is built at load time and used until the process exits. A chunk mesh is built when the
player walks towards it and is dead when they walk away. And the sort key has four fields —
layer, pipeline, material, depth — none of which is geometry, so a mesh handle would sort
nothing.

## Decision

Geometry is owned by whatever built it. A mesh is a `vulkan::Mesh` held by a chunk, a model or
an app, `DrawItem` keeps its raw `VkBuffer`s, and a draw item is valid only while the mesh it
was filled from is alive. `Resources` gains no fourth registry.

## Alternatives Considered

### Alternative 1: The app owns meshes, the api gives it an RAII type — **chosen**
- **Pros**: Lifetime is where the knowledge is. A registry that never frees cannot hold
  something created and destroyed during play without leaking device memory for the whole
  session. `Mesh` is still api code, so no app writes its own staging upload.
- **Cons**: A draw item can outlive the mesh it names, and nothing catches that — the symptom
  is a use-after-free the validation layer reports, or a crash. Frame lifetime is what makes
  it survivable: a draw item is built during a tick and recorded at the end of the same one.
- **Why not**: n/a — chosen.

### Alternative 2: A fourth registry, `MeshHandle`, alongside the other three
- **Pros**: Uniform. Everything a draw item names is a handle, resolved the same way, and the
  recorder has one kind of lookup rather than two.
- **Cons**: `Resources` would have to learn to free, which none of it does today: slots are
  never reused, so a handle can never come to mean something else. Making meshes freeable
  means either leaking their slots or reusing them, and reusing them breaks exactly the
  guarantee the registry exists to give. It buys nothing at record time — the key has no mesh
  field, and the recorder would resolve a handle where it currently reads a pointer.
- **Why not**: It pays the registry's whole cost for none of its benefit.

### Alternative 3: A mesh cache in the api, keyed by whatever built the geometry
- **Pros**: Deduplicates identical meshes and gives the api somewhere to put a residency
  budget later, which a streaming voxel world will eventually want.
- **Cons**: A cache needs an eviction policy, and the only thing that knows when a chunk mesh
  is dead is the chunk. Building it now means guessing the policy before there is an app to
  check the guess against.
- **Why not**: Premature. It can be built on top of alternative 1 without changing anything
  decided here, since a cache would own `Mesh` objects exactly the way a chunk does.

## Consequences

### Positive
- Chunk streaming works: a chunk that goes out of range destroys its mesh and the device
  memory comes back, which under a never-freeing registry it would not.
- `Resources` keeps its invariant intact — every slot is stable for the life of the context —
  and that invariant is what makes handles comparable at all.
- Nothing changes at record time. `DrawItem` already carried `VkBuffer`s; this says that was
  right rather than provisional.

### Negative
- The api cannot tell an app it has submitted a draw item naming a mesh it has since freed.
  There is no handle to invalidate and no registry to check against.
- Two lifetime models in one frame: pipelines and textures live until the context does, meshes
  live as long as their owner. A reader has to know which is which.
- Anything that later wants to sort or merge by geometry has no field to do it on. Adding one
  means adding it to the sort key, which is a change to every submitter.

### Risks
- The dangling draw item is the real one. It stays theoretical only while draw items are built
  and recorded inside one tick; anything that starts caching draw items across frames makes it
  a live bug. If that happens, the answer is a residency check at record time rather than
  reversing this decision.
