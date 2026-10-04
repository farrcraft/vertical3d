# Roadmaps

A roadmap describes what a feature area needs and in what order, for an area nobody has taken
up. It answers "if this were picked up tomorrow, where would it start", written while the code
is still fresh in someone's head.

A [plan](../plans/) is the other half of that. A plan is work that has been taken on: it has
phases, state notes kept current as things land, and an end, after which it moves to
[plans/completed/](../plans/completed/). A roadmap has none of those. It is not scheduled and
it claims no date. **When a roadmap section is taken up it earns a plan**, and the roadmap
points at it. When the plans have done everything a roadmap asked for, or what is left of it is
recorded elsewhere, the roadmap moves to [completed/](completed/), where it stays as the
reasoning the plans were drafted from.

A roadmap shares two rules with a plan. It describes the tree as it stands, so update it when
the tree moves under it. And it does not restate decisions: a roadmap may say a decision is
needed and what turns on it, but the decision itself belongs in [adr/](../adr/). See
[sdlc.md](../sdlc.md).

A roadmap is also not [TODO.md](../TODO.md). That file collects loose ends, each independent of
the others; a roadmap exists because the ordering between its items is the interesting part.

## The roadmaps

| Roadmap | Area |
|---|---|
| [OfflineRendering.md](OfflineRendering.md) | `talyn` and `moya` — the raytracer and the reyes renderer |
| [GameEngine.md](GameEngine.md) | The realtime api, for games rather than demos — the index of seven milestones, each its own document |
| [m7-ShellAndShipping.md](m7-ShellAndShipping.md) | 7 — the shell's renderer setup, widgets, input, versioned documents, profiling, async loading |

## Completed

| Roadmap | Area | Done by |
|---|---|---|
| [m5-SkeletalAnimation.md](completed/m5-SkeletalAnimation.md) | 5 — a model in parts, skins and clips from glTF, playback on the step, skinning in both lit passes; instancing is in [TODO.md](../TODO.md#lit-scenes) | [SkeletalAnimation](../plans/completed/SkeletalAnimation.md), 2026-10-03 |
| [m4-LitScene.md](completed/m4-LitScene.md) | 4 — images and samplers, a model onto the device, a lit pass, shadows, a post chain; retcon adopts it after, and a fit that follows the camera is in [TODO.md](../TODO.md#lit-scenes) | [LitScene](../plans/completed/LitScene.md), 2026-10-03 |
| [m3-RenderableComponent.md](completed/m3-RenderableComponent.md) | 3 — a transform, a sprite and the walk that draws it, per [ADR-0063](../adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md); the mesh component is [m4](completed/m4-LitScene.md)'s to build | [RenderableComponent](../plans/completed/RenderableComponent.md), 2026-10-03 |
| [m2-LargeWorlds.md](completed/m2-LargeWorlds.md) | 2 — releasing a resource, ordering world quads, culling, a grid from a map's picture; regions, remembered sight and the movement filter are in [TODO.md](../TODO.md#tile-grids) | [LargeWorlds](../plans/completed/LargeWorlds.md), 2026-10-03 |
| [m1-MotionAndQueries.md](completed/m1-MotionAndQueries.md) | 1 — interpolation, a ground pick, overlap, `Plane` and `Frustum`; the sprite clip is in [TODO.md](../TODO.md#sprite-sheets) | [MotionAndQueries](../plans/completed/MotionAndQueries.md), 2026-10-03 |
