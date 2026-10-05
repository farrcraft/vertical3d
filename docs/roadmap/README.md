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
| — | No roadmap is open. |

## Completed

| Roadmap | Area | Done by |
|---|---|---|
| [OfflineRendering.md](completed/OfflineRendering.md) | `talyn` and `moya` — a RIB reader, a shading language, sampling through one film, a trace that recurses, textures, and one ray tracer both reach; area lights, displacement and acceleration are in [TODO.md](../TODO.md#offline-rendering) | three phase plans and [OfflineRenderingPhases4To6](../plans/completed/OfflineRenderingPhases4To6.md), 2026-09-05 to 2026-10-04 |
| [GameEngine.md](completed/GameEngine.md) | The realtime api, for games rather than demos — the index of the seven milestones below; what each left is in [TODO.md](../TODO.md) | the seven plans below, 2026-10-03 to 2026-10-04 |
| [m7-ShellAndShipping.md](completed/m7-ShellAndShipping.md) | 7 — strips that respect `pickable()`, a held command, a relative mouse, a document read forward, one screen for the ui's renderers, a game space on a canvas, a wrapping box, a file chooser, pass timings and a slider; asynchronous loading is in [TODO.md](../TODO.md#loading) | [ShellAndShipping](../plans/completed/ShellAndShipping.md), 2026-10-04 |
| [m6-Effects.md](completed/m6-Effects.md) | 6 — a sprite clip, a seeded random source, particles on the step, weather, a tint over the world, and a lit scene's colour over time; the panned voice is in [TODO.md](../TODO.md#audio) | [Effects](../plans/completed/Effects.md), 2026-10-04 |
| [m5-SkeletalAnimation.md](completed/m5-SkeletalAnimation.md) | 5 — a model in parts, skins and clips from glTF, playback on the step, skinning in both lit passes; instancing is in [TODO.md](../TODO.md#lit-scenes) | [SkeletalAnimation](../plans/completed/SkeletalAnimation.md), 2026-10-03 |
| [m4-LitScene.md](completed/m4-LitScene.md) | 4 — images and samplers, a model onto the device, a lit pass, shadows, a post chain; retcon adopts it after, and a fit that follows the camera is in [TODO.md](../TODO.md#lit-scenes) | [LitScene](../plans/completed/LitScene.md), 2026-10-03 |
| [m3-RenderableComponent.md](completed/m3-RenderableComponent.md) | 3 — a transform, a sprite and the walk that draws it, per [ADR-0063](../adr/0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md); the mesh component is [m4](completed/m4-LitScene.md)'s to build | [RenderableComponent](../plans/completed/RenderableComponent.md), 2026-10-03 |
| [m2-LargeWorlds.md](completed/m2-LargeWorlds.md) | 2 — releasing a resource, ordering world quads, culling, a grid from a map's picture; regions, remembered sight and the movement filter are in [TODO.md](../TODO.md#tile-grids) | [LargeWorlds](../plans/completed/LargeWorlds.md), 2026-10-03 |
| [m1-MotionAndQueries.md](completed/m1-MotionAndQueries.md) | 1 — interpolation, a ground pick, overlap, `Plane` and `Frustum`; the sprite clip is in [TODO.md](../TODO.md#sprite-sheets) | [MotionAndQueries](../plans/completed/MotionAndQueries.md), 2026-10-03 |
