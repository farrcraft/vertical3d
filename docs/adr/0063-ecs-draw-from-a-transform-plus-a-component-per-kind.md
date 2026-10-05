# ADR-0063: ECS: draw from a transform plus a component per kind

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

Amended in place: `PositionFixed2D` is gone, and `Transform` is `type::Transform`, the value an
editor mesh's `dag::Transform` holds as well.

## Context

Nothing in the api says what an entity carries so that something can draw it, and
[RenderingPipeline.md](../api/Rendering.md#how-this-meets-the-ecs) has called
that open since before either game existed. `api/ecs` has four 2D components, and neither game
uses them. retcon has written `Transform` (position, yaw, scale) and `MeshRenderer` (a mesh
handle and a shadow flag), with a scene renderer that walks both. cozy has a `Position` at its
feet and draws its two world sprites by hand. [Milestone 4](../roadmap/completed/m4-LitScene.md)'s mesh
pass walks entities, so this has to be settled before that pass is written to retcon's shape by
default. [Renderable component](../plans/completed/RenderableComponent.md) is the plan that needs it, and
its survey of both games is the evidence here.

## Decision

**Where a thing stands is `v3d::ecs::component::Transform`**: a position, a quaternion and a
scale, with `aboutY(radians)` for a game whose world turns about one axis. It lives in `api/ecs`
because simulation writes it. **What a thing looks like is a component per kind of drawing,
beside the handle it names in `api/render/realtime/component/`**: `Sprite` for an upright
billboard drawn into one world canvas, and a mesh component that milestone 4 builds with its
mesh registry. **The api walks them**, with a function called from `render()` that reads
`interpolated<Transform>` ([ADR-0060](0060-ecs-interpolate-from-a-previous-step-component.md)), so an
entity with a previous step is drawn between steps. No parent component exists until something
is carried.

## Alternatives Considered

### Alternative 1: A yaw rather than a quaternion
- **Pros**: retcon's shape, four bytes rather than sixteen, and a placement can be written from
  a tile and a facing without building a rotation.
- **Cons**: A float angle interpolated between steps turns the long way round: 170° to -170° is
  20 degrees, and a lerp sweeps 340. Anything that ever pitches, or a skinned character in
  [milestone 5](../roadmap/completed/m5-SkeletalAnimation.md), changes the component's representation,
  which breaks every game that reads it.
- **Why not**: The cost is twelve bytes per entity at a scale of hundreds, and `aboutY` keeps
  the convenience.

### Alternative 2: A sprite is a mesh whose mesh is a quad
- **Pros**: One renderable component, and one walk.
- **Cons**: cozy's sprites are one stream of quads cut by texture
  ([ADR-0042](0042-rendering-world-space-sprites.md)). An item per sprite, each with its own
  mesh and push constants, is the opposite of that batching.
- **Why not**: The kinds draw differently, so they are described differently.

### Alternative 3: The api provides the components and each game writes the walk
- **Pros**: Nothing in the api decides how anything is drawn, and both games already walk their
  own registries.
- **Cons**: The walk is the part that is the same in every game: read the transform,
  interpolate it, emit. A component with no walk behind it is a struct a game could have
  written.
- **Why not**: The walk is what makes the component worth having in the api at all.

### Alternative 4: The renderable components in `api/ecs`
- **Pros**: Every component in one place.
- **Cons**: Each names a texture or mesh handle that `api/render` defines, so `api/ecs`, which
  links glm and EnTT, would link Vulkan. A headless test of a game's rules would then link a
  device to place a unit.
- **Why not**: A component lives beside what it names.

### Alternative 5: A transform, a component per kind beside its handle, and a walk in the api — **chosen**
- **Pros**: retcon's components map onto it field for field, and its walk keeps its shape.
  cozy's two sprites become two entities and one call, still drawn as one batch.
- **Cons**: `api/render` links `api/ecs`, and a game that already has its own position keeps a
  `Transform` in sync with it, as retcon already does from its tiles.
- **Why chosen**: It is the shape both games had arrived at, with the walk moved to where both
  can share it.

## Consequences

### Positive
- `alpha()` has a reader for anything the api draws, and interpolating is one
  `snapshot<Transform>` call at the top of `simulate()`.
- Milestone 4's mesh pass knows what it walks: `view<const Transform, const Mesh>()`, with the
  material on the registry entry where retcon has it.
- retcon's two components map across with one line changed. `matrix()` keeps its order and its
  callers, its shadow fit reads only `position`, `MeshRenderer` is the mesh component under
  another name with the same two fields, and the line that writes a facing becomes
  `rotation = aboutY(facingYaw(facing))`.

### Negative
- **A sprite ignores its transform's rotation.** An upright billboard faces the camera, so a
  game chooses a facing by choosing the uvs. A sprite that lies on the ground is not covered.
- A sprite holds resolved uvs rather than a region name, so a game resolves again when a sheet
  is reloaded.
- `Position1D`, `Position2D` and `PositionFixed2D` stay beside `Transform`, so the api has two
  ways to say where a thing is, split by whether it is drawn in the world.

### Risks
- **The walk's depth key is an axis, `dot(position, axis)`.** That serves an orthographic ground
  plane and a perspective view, and not an order that depends on a sprite's footprint. The
  escape hatch is `DepthOrder` itself, which takes any key from a game that walks its own.
- A parent, when it comes, is a component the walk resolves before it reads `Transform`, so
  `Transform` does not change shape for it.
