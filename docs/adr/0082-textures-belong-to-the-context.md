# ADR-0082: Textures — Textures And Their Materials Belong To The Device Context, Not To The 2D Renderer

**Date**: 2026-10-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`renderer::Quad`, the batched 2D primitive, owned the texture factory, set 1's descriptor pool,
the white texture and the map from a texture to its material, because it was the first thing to
sample a texture. [ADR-0042](0042-a-textured-quad-in-world-space.md) and
[ADR-0065](0065-a-mesh-is-registered-by-path-and-released.md) then had the world quad, the lit
renderer and `MeshRegistry` take their textures and materials from it, so that one pool served
every primitive and an atlas was uploaded once. That reasoning holds; where it put the pool did
not. 3D code depended on the 2D renderer, and asking `quads()` for a texture built the quad's
pipelines, which `pipeline::Builder` refuses against an undefined colour format — so a headless
context, which [ADR-0051](0051-the-in-flight-ring-is-not-the-swapchain.md) says can build every
renderer, could not load a mesh. The factory also made its own uploader beside the context's.
The review is [R2](../audits/completed/ApiDesignReview.md).

## Decision

**A `realtime::Textures` on `DeviceContext` owns the texture factory, set 1's pool and layout,
the white texture and the material map**, and copies through the context's one uploader. It is
built with the context. `Quad`, `World`, `Lit` and `MeshRegistry` take it rather than `Quad`, and
an app asks `context->textures()` — or `Engine3D::textures()` — for what it asked `quads()` for.

## Alternatives Considered

### Alternative 1: Keep it on `Quad`, and build `Quad`'s pipelines lazily
- **Pros**: the smallest change; a headless context could load textures again.
- **Cons**: 3D code still depends on the 2D renderer, and a change to how textures work is still
  a change to the 2D primitive.
- **Why not**: it fixes the symptom and keeps the coupling that caused it.

### Alternative 2: Give each renderer a pool of its own
- **Pros**: no shared service at all.
- **Cons**: an atlas drawn by the quad and the world quad both would be a material twice, which
  is what ADR-0042 shared the pool to avoid.
- **Why not**: it undoes the one thing the old shape got right.

### Alternative 3: A `Textures` service on the context — **chosen**
- **Pros**: one pool, as before, owned by what owns the device; nothing 3D names the 2D renderer;
  a context that draws nothing loads textures and meshes.
- **Cons**: see below.

## Consequences

### Positive
- `MeshRegistryTest` registers a texture and a textured mesh against a context with no colour
  format, and the quad renderer is never built.
- One uploader per context, where the factory made a second.

### Negative
- Every context uploads a white texture as it is built, whether or not anything samples one.
- An app's `quads()->texture(...)` is `textures()->texture(...)` now; the change went through
  every app and test in the tree, and a consumer outside it makes the same one.

### Risks
- A second set 1 layout — a texture array, a second sampler — would be a second service or a
  second layout here. The escape hatch is that this is the one class to grow.
