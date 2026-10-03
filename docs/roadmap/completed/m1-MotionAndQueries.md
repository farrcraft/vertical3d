# Motion and Queries

Milestone 1 of [the game engine roadmap](../GameEngine.md). Five small pieces with little order
between them: drawing between simulation steps, playing a sprite's frames, finding where a
click lands on the ground, asking whether two boxes overlap, and the plane and frustum maths
that two of those need. Each is something a consumer has written for itself or stubbed.

**Done by [MotionAndQueries](../../plans/completed/MotionAndQueries.md)**, closed 2026-10-03. The
sprite clip is held in [TODO.md](../../TODO.md#sprite-sheets) until it has a second consumer, and
interpolation has a record of its own in
[ADR-0060](../../adr/0060-a-moving-thing-keeps-its-previous-step.md); what follows is the
reasoning the plan was drafted from, as it stood then.

## What exists

* **The loop simulates at a fixed step and renders at a variable one**
  ([ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md)).
  [`Engine::alpha()`](../../../api/engine/Engine.h) is the fraction between the last completed
  step and the next, and [`Accumulator.h`](../../../api/engine/Accumulator.h) says of it that
  nothing reads it yet. Every world drawn in this tree is snapped to the last 60 Hz step.
* **A sprite sheet names regions.** [`config::SpriteSheets`](../../../api/config/SpriteSheets.h)
  reads and writes the document, and `image::TextureAtlas` places what it names. Nothing plays
  a sequence of them: cozy's M5 plan states that the engine has no animation of any kind, and
  defers its walk cycle for want of art rather than of code.
* **A click is already a ray.** [`Camera::ray()`](../../../api/type/camera/Camera.h) turns a window
  point into a world-space ray, orthographic included. What is missing is the last step: where
  that ray meets the ground.
* **[`type::geometry`](../../../api/type/geometry/)** has `Ray` (slab test against an `AABBox`,
  Möller-Trumbore against a triangle), `AABBox`, which can be built and extended but not tested
  against anything, and `Bound2D`, whose only test is a point
  ([`Bound2D.h:64`](../../../api/type/geometry/Bound2D.h)).
* **moya has a `Plane` and a `Frustum`** (`moya/libmoya/Plane.h` and `Frustum.h`, since moved to
  [`api/type/geometry/`](../../../api/type/geometry/) by [the plan](../../plans/completed/MotionAndQueries.md)): classifying a point or a box against a plane,
  intersecting a ray or an edge with one, and extracting six planes from a matrix. They are
  general geometry living in a renderer.

## What it needs

### Interpolation

Something that reads `alpha()`. The shape is a previous and a current state per moving thing,
the previous copied before each `simulate()` and the two blended when drawing. What decides
where it lives is what a state is: a position alone is a helper in `api/type`, while a whole
transform held per entity is an `api/ecs` component and a system that copies it. Milestone 3
settles what an entity carries to be drawn, but this does not have to wait for it — a pair of
positions is useful to pong today and is what a transform pair would generalise.

cozy's `Movement` integrates a velocity on the fixed step and is the consumer that would see it
first. odyssey is not one: its [path follower](../../../odyssey/system/Movement.cpp) replaces a
tile coordinate every 0.15 seconds, so what it would want is a position between two tiles,
which is its own path's elapsed time rather than the loop's.

### A sprite clip

A list of region names, a duration per frame or for the whole, a loop mode, and a time advanced
on the fixed step that answers "which region now". That is the whole of a walk cycle as cozy's
M5 plan describes it.

**It is the first half of milestone 5's clip sampler, so its shape is worth getting right
once.** A skeletal clip is a set of tracks sampled at a time; a sprite clip is one track of
discrete values. Advancing time, looping, clamping and reporting that a named point was passed
are the same in both, and an event on a frame — a footstep, the moment a swing lands — is what
both games will ask for. The time-keeping belongs in a type neither kind of track owns.

Whether the document that defines clips is `sprites.json` itself or one beside it is the
step's to decide; `config::SpriteSheets` already reads and writes the former.

**It has one consumer, and that consumer disagrees.** cozy's M5 plan calls its walk cycle thirty
lines that are entirely the game's, and defers it for want of art. The case for the api is the
clock shared with milestones 5 and 6, which is a case that exists only once one of them is taken
up — so the plan holds this piece until then.

### A ground pick

`Camera::ray()` and a plane intersection. cozy's ground is XZ at height zero under a fixed
orthographic camera (its ADR-0001), and retcon's is the same plane under
`type::camera::Isometric`, with a `screenRay` and an `intersectHorizontalPlane` of its own in
`engine/view/Picking`. Once there is a `Plane`, both are one call, and a tile is that call
followed by `TileGrid`'s world-to-tile, which `api/grid` already has.

### Overlap

Box against box, for `Bound2D` and for `AABBox`, and a containment test for each. pong writes
its own ([`PongScene.cxx:95-112`](../../../pong/src/PongScene.cxx)), voxel's
`Player::checkWorldCollision` is a stub that always answers no
([`Player.cxx:94`](../../../voxel/src/game/Player.cxx)), and cozy will need the same at M6 when
something first walks into something.

This is overlap and not collision response. Pushing a body back out of what it walked into is
a game's rule — cozy and retcon would answer it differently — and is
[not on this roadmap](../GameEngine.md#not-on-this-roadmap).

### `Plane` and `Frustum` in `api/type`

Moved from moya, which keeps using them. They are the ground pick above and the culling in
[milestone 2](../m2-LargeWorlds.md#culling).

**The frustum's plane extraction has to be checked against the clip space it is handed.** moya
builds an offline camera and the realtime camera builds Vulkan clip space
([ADR-0012](../../adr/0012-camera-builds-vulkan-clip-space.md)), whose depth runs 0 to 1 rather
than −1 to 1. By [ADR-0024](../../adr/0024-api-type-serves-both-renderers.md) a convention one
consumer needs becomes a parameter rather than a second copy, so the extraction takes it.

## Verification

All of it is headless. The geometry is tested the way `Ray` is, with cases at the edges —
touching boxes, a ray parallel to the plane. The clip is a time advanced by fixed steps and a
region named at each, which a table of steps asserts exactly. Interpolation is asserted at
`alpha()` of zero, one and between.

The frustum is the one place a wrong answer looks right: a frustum extracted in the wrong clip
space culls the near half of what it should keep and draws the rest. Its tests build a camera,
place boxes known to be inside, outside and straddling each plane, and assert all three.

## Not in this milestone

* **A camera that moves itself.** voxel's flight camera and the editor's orbit tool are each
  one consumer's controller; [milestone 7](../m7-ShellAndShipping.md) has the relative mouse mode
  they both work around.
* **Collision response and a broadphase.** See above.
