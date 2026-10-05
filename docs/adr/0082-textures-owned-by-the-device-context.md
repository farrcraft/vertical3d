# ADR-0082: Textures: owned by the device context

**Status**: accepted
**Date**: 2026-10-05
**Amends**: [ADR-0042](0042-rendering-world-space-sprites.md), [ADR-0065](0065-meshes-shared-registry-keyed-by-path.md)
**Documented in**: [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md), [api/rendering/TexturesAndMeshes.md](../api/rendering/TexturesAndMeshes.md)

## Context

Every primitive that samples a texture shares one texture factory, one descriptor pool and
layout for set 1, one white texture and one map from texture to material, so that an atlas is
uploaded once and is one material ([ADR-0042](0042-rendering-world-space-sprites.md),
[ADR-0065](0065-meshes-shared-registry-keyed-by-path.md)). Whatever owns that pool is a
dependency of every 2D and 3D renderer and of `MeshRegistry`. If the 2D quad renderer owns it,
3D code depends on the 2D renderer, and reaching a texture builds the quad's pipelines, which
need a colour format. A context with no window has no colour format
([ADR-0051](0051-frames-in-flight-ring-separate-from-presenting.md)), yet still has to load
textures and meshes.

## Decision

`realtime::Textures`, owned by `DeviceContext` and built with it, holds the texture factory, set
1's pool and layout, the white texture and the material map, and uploads through the context's
one uploader. `Quad`, `World`, `Lit` and `MeshRegistry` take it, and an app reaches it through
`DeviceContext::textures()` or `Engine3D::textures()`.

## Alternatives

### Keep the pool on the quad renderer and build its pipelines lazily
- **For**: The smallest change, and a context with no window could load textures.
- **Against**: 3D code still depends on the 2D renderer, and any change to how textures work is
  a change to the 2D primitive.
- **Rejected because**: It removes the symptom and keeps the coupling that caused it.

### A pool per renderer
- **For**: No shared service at all.
- **Against**: An atlas drawn by both the quad and the world quad would be uploaded and bound as
  two materials, which ADR-0042 shares the pool to avoid.
- **Rejected because**: It gives up the sharing the pool exists for.

## Consequences

- **Gains**:
  - One pool, owned by what owns the device, and nothing 3D names the 2D renderer.
  - A context that draws nothing can load textures and meshes, so mesh tests need no colour
    format.
  - One uploader per context.
- **Costs**:
  - Every context uploads a white texture when it is built, whether or not anything samples it.
  - A second set 1 layout, such as a texture array or a second sampler, needs a second service
    or a second layout in this one.
- **Revisit when**: a renderer needs a set 1 layout this class does not provide.
