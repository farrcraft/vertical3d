# ADR-0063: ECS: draw from a transform plus a component per kind

**Status**: accepted
**Date**: 2026-10-03
**Documented in**: [api/ECS.md](../api/ECS.md), [api/Rendering.md](../api/Rendering.md)

## Context

A game draws an entity by reading where it stands and what it looks like, and the api has to say
which components carry each. Placement is written by simulation, so it belongs with the ECS, which
links only glm and EnTT. What a thing looks like names a texture or mesh handle that `api/render`
defines. Sprites and meshes draw differently: sprites are one stream of quads batched by texture
([ADR-0042](0042-rendering-world-space-sprites.md)), and meshes are a draw each.

## Decision

Where an entity stands is `v3d::ecs::component::Transform`, a position, a quaternion rotation and
a scale. What it looks like is one component per kind of drawing, such as `Sprite` or `Mesh`, kept
in `api/render/realtime/component/` beside the handle it names. The api walks them, with a
function per kind called from `render()` that reads the transform through `interpolated<Transform>`
([ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md)).

## Alternatives

### A yaw angle rather than a quaternion
- **For**: four bytes rather than sixteen, and a placement can be written from a tile and a facing
  without building a rotation.
- **Against**: an angle interpolated between steps can turn the long way round: 170° to -170° is
  20 degrees, and a plain blend sweeps 340. Anything that pitches, or a skinned character, would
  change the component's representation and break every game that reads it.
- **Rejected because**: twelve bytes per entity is cheap, and `aboutY(radians)` keeps the
  convenience of a yaw.

### A sprite is a mesh whose mesh is a quad
- **For**: one renderable component and one walk.
- **Against**: an item per sprite, each with its own mesh and push constants, is the opposite of
  batching sprites as one stream of quads.
- **Rejected because**: the kinds draw differently, so they are described differently.

### The api provides the components, and each game writes the walk
- **For**: nothing in the api decides how anything is drawn.
- **Against**: the walk is the part every game writes the same way: read the transform,
  interpolate it, emit a draw. A component with no walk behind it is a struct a game could have
  written.
- **Rejected because**: the walk is what makes the components worth having in the api.

### The renderable components in `api/ecs`
- **For**: every component in one place.
- **Against**: each names a handle `api/render` defines, so `api/ecs` would link Vulkan, and a
  headless test of a game's rules would link a device to place a unit.
- **Rejected because**: a component lives beside what it names.

## Consequences

- **Gains**:
  - Anything the api draws is drawn between steps, for one `snapshot<Transform>()` call at the top
    of `simulate()`.
  - A game with several sprites makes them entities and calls one walk, still drawn as one batch.
- **Costs**:
  - `api/render` links `api/ecs`.
  - A game that keeps its own position, such as a tile coordinate, has to keep a `Transform` in
    sync with it, so the api has two ways to say where a thing is.
  - A sprite ignores its transform's rotation. An upright billboard faces the camera, so a game
    picks a facing by picking the uvs, and a sprite lying on the ground is not covered.
  - The sprite walk sorts by distance along one axis. That serves an orthographic ground plane and
    a perspective view, not an order that depends on a sprite's footprint.
- **Revisit when**: something is carried by something else. A parent component would be resolved
  by the walk before it reads `Transform`, so `Transform` would not change shape.
