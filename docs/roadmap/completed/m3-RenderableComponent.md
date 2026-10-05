# The Renderable Component

Milestone 3 of [the game engine roadmap](GameEngine.md). **This milestone is a decision rather
than a body of code**: what an entity carries so that something can draw it, and where it
stands in the world. It is in the roadmap because [milestone 4](m4-LitScene.md) cannot be shaped
until it is answered, and because the question has been open since before either game existed.

**Done by [RenderableComponent](../../plans/completed/RenderableComponent.md)**, closed 2026-10-03. The
decision has a record of its own in
[ADR-0063](../../adr/0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md), and the
mesh component it shapes is built by [milestone 4](m4-LitScene.md). What follows is the
reasoning the plan was drafted from, as it stood then.

## What exists

* **The question is written down in two places.** [ECSDesign.md](../../api/ECS.md) was written
  around it, and [RenderingPipeline.md](../../api/rendering/Entities.md)
  states it: `Scene::collect()` returning a frame fits the pass model, a `Renderable` marker says
  nothing about how to draw, and components named after the old operation classes are the
  design [ADR-0004](../../adr/0004-rendering-submit-draw-items-as-data.md) moved away from. The likely answer it
  names is a component naming a material and a mesh or quad, with a system turning those into
  draw items — and nothing has been built to prove it.
* **[`api/ecs`](../../../api/ecs/)** is a `System` with `simulate(float)` and four components:
  `Color3`, `Position1D`, `Position2D` and `PositionFixed2D`. Nothing in it is 3D.
* **Neither game uses the api's positions, and both say why.** cozy's
  `src/Components.h` defines `Position` as the feet of a thing on the XZ ground plane, and its
  comment says a 2D position is either wrong for that world or a 3D one with an unwritten
  convention. retcon's `engine/ecs/Components.hpp` defines `Transform` as a position, a yaw about
  +Y and a scale.
* **retcon has answered the question for itself.** Its `MeshRenderer` is a handle into its mesh
  registry plus whether the entity casts a shadow; its scene renderer walks the registry and
  draws every entity carrying both. Nothing in that component owns GPU memory, which is the
  same rule as [ADR-0010](../../adr/0010-meshes-owned-by-the-app-that-built-them.md).
* **cozy draws without one.** Its world sprites are emitted into a `WorldCanvas` by the code
  that owns them, read from its own components, so what an entity looks like is a function in
  the game rather than data on the entity.
* **The editor is a scene graph.** A mesh is a dag node
  ([ADR-0013](../../adr/0013-editor-a-mesh-is-a-dag-node.md)) and nothing in it is an entity.

## What it needs decided

One record, answering these together because each constrains the next.

**Where a thing stands.** A 3D transform component in `api/ecs`, which both games have written
and which the api does not have. What turns on it is how much of a transform it is: retcon's is
yaw only, because its world turns about one axis, and cozy's is a point. A full rotation serves
both and costs a quaternion per entity that neither reads. A parent — anything that is carried
by something else, like a held item or a rider — is the other thing that turns on it, and
neither game has one yet.

**What a thing looks like.** A mesh and a material by handle, per the likely answer above, is
retcon's `MeshRenderer` with the shadow flag generalised. What has to be settled is whether a
world sprite is the same component with a quad for its mesh, or a component of its own. cozy's
sprites are one stream of quads cut by texture, and an item per sprite is the opposite of that
batching, which argues for a component of its own whose system writes into one canvas.

**Who turns it into draw items.** A system in the api walks the registry and fills a pass, or
the api provides the component and each game writes the walk. The first is what makes the
component worth having in the api at all; the second is where both games are today.

**What it means for [interpolation](m1-MotionAndQueries.md#interpolation).** If the api owns the
transform, the previous and current pair that milestone 1 blends is a second transform
component, copied by the api before each step, and the drawing system reads `alpha()`.

## What it does not decide

* **The scene graph.** `api/dag` is mostly stubs, and only the editor uses `Node` and
  `Transform`. Whether the editor ever becomes entities is the editor's question, and nothing
  in either game is waiting on it.
* **The lit pass.** What a material holds and how it is shaded is
  [milestone 4](m4-LitScene.md)'s. This record names a material handle and stops there.

## Verification

A record is verified by being used. The test of this one is that retcon's `MeshRenderer` and
`Transform` could be replaced by the api's without its scene renderer changing shape, and that
cozy's sprite emission could be written against it without an item per sprite. Both are checked
in this tree, against what the games do: the second as a test that reproduces cozy's scene
before the record is accepted, because it is the one the likely answer does not obviously fit.
The games adopt the result after it ships.
