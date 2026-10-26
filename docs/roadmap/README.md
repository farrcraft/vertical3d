# Roadmaps

This directory holds roadmaps: descriptions of what a feature area needs, and in what order.
It is for anyone deciding what to work on next.

A roadmap covers an area nobody has started. It answers "if someone picked this up tomorrow,
where would they start?", and it is written while the relevant code is still fresh in someone's
mind.

## How a roadmap relates to a plan

A [plan](../plans/) is work that has been started. It has steps, state notes kept current as
work lands, and an end, after which it moves to [plans/completed/](../plans/completed/). A
roadmap has none of these. It is not a schedule and it has no dates.

- **When a section of a roadmap is started, that section gets a plan**, and the roadmap links to
  it.
- **When the plans have done everything a roadmap asked for**, or the rest is recorded elsewhere,
  the roadmap moves to [completed/](completed/). It stays there as the reasoning the plans were
  drafted from.

## Rules

- **An open roadmap describes the tree as it is.** Update it when the code it describes changes.
  A completed roadmap is a frozen record, and is never updated.
- **A roadmap does not record decisions.** It may say that a decision is needed and what depends
  on it. The decision itself goes in an ADR in [adr/](../adr/).
- **A roadmap is not [TODO.md](../TODO.md).** TODO.md lists independent loose ends. A roadmap
  exists because the order of its items matters.

[sdlc.md](../sdlc.md) describes how roadmaps fit with plans, ADRs and TODO.md.

## Open roadmaps

| Roadmap | Area |
|---|---|

## Completed roadmaps

| Roadmap | What it covered | Done by |
|---|---|---|
| [EditorModelling.md](completed/EditorModelling.md) | The editor: a dirty flag, multiple selection, modelling operations, the fixed views and resizable panes. The other menu command groups are in [TODO.md](../TODO.md#editor) | [EditorModelling](../plans/completed/EditorModelling.md), 2026-10-24 |
| [OfflineShading.md](completed/OfflineShading.md) | The remaining RenderMan features: verified transformations, an acceleration structure, area lights, reyes opacity, displacement and bump mapping. Batched hits and a lazily filled traced scene are in [TODO.md](../TODO.md#offline-rendering) | [OfflineShading](../plans/completed/OfflineShading.md), 2026-10-18 |
| [LitScenesAtScale.md](completed/LitScenesAtScale.md) | Instanced lit and shadow draws, a grade authored for display, and a panned voice. A voice with no position is in [TODO.md](../TODO.md#audio) | [LitScenesAtScale](../plans/completed/LitScenesAtScale.md), 2026-10-17 |
| [StreamingWorlds.md](completed/StreamingWorlds.md) | Loading while a game runs: a worker thread, uploads that do not drain the queue, regions, the movement filter and a shadow map that follows the camera. Remembered sight in `api/grid` is in [TODO.md](../TODO.md#tile-grids) | [StreamingWorlds](../plans/completed/StreamingWorlds.md), 2026-10-16 |
| [OfflineRendering.md](completed/OfflineRendering.md) | Offline rendering: a RIB reader, a shading language, sampling through one film, recursive ray tracing, textures, and one ray tracer. Area lights, displacement and acceleration are in [TODO.md](../TODO.md#offline-rendering) | Three phase plans and [OfflineRenderingPhases4To6](../plans/completed/OfflineRenderingPhases4To6.md), 2026-09-05 to 2026-10-04 |
| [GameEngine.md](completed/GameEngine.md) | The realtime api, made suitable for games rather than demos. It indexes the seven milestones below; what each left open is in [TODO.md](../TODO.md) | The seven plans below, 2026-10-03 to 2026-10-04 |
| [m7-ShellAndShipping.md](completed/m7-ShellAndShipping.md) | Milestone 7: strips that respect `pickable()`, held commands, relative mouse mode, document migration, one shared screen for the UI renderers, a canvas with its own coordinate space, wrapping text boxes, a file chooser, pass timings and a slider. Asynchronous loading is done by [StreamingWorlds](../plans/completed/StreamingWorlds.md) | [ShellAndShipping](../plans/completed/ShellAndShipping.md), 2026-10-04 |
| [m6-Effects.md](completed/m6-Effects.md) | Milestone 6: sprite clips, a seeded random source, particles on the fixed step, weather, a world tint, and a lit scene's colour over time. The panned voice is in [TODO.md](../TODO.md#audio) | [Effects](../plans/completed/Effects.md), 2026-10-04 |
| [m5-SkeletalAnimation.md](completed/m5-SkeletalAnimation.md) | Milestone 5: models in parts, skins and clips from glTF, playback on the fixed step, and skinning in both lit passes. Instancing is in [TODO.md](../TODO.md#lit-scenes) | [SkeletalAnimation](../plans/completed/SkeletalAnimation.md), 2026-10-03 |
| [m4-LitScene.md](completed/m4-LitScene.md) | Milestone 4: images and samplers, models on the device, a lit pass, shadows and a post-processing chain. A shadow fit that follows the camera is in [TODO.md](../TODO.md#lit-scenes) | [LitScene](../plans/completed/LitScene.md), 2026-10-03 |
| [m3-RenderableComponent.md](completed/m3-RenderableComponent.md) | Milestone 3: a transform component, a sprite component, and the function that draws them. The mesh component was built in [m4](completed/m4-LitScene.md) | [RenderableComponent](../plans/completed/RenderableComponent.md), 2026-10-03 |
| [m2-LargeWorlds.md](completed/m2-LargeWorlds.md) | Milestone 2: releasing a resource, ordering world quads, culling, and a grid built from a map picture. Regions, remembered sight and the movement filter are in [TODO.md](../TODO.md#tile-grids) | [LargeWorlds](../plans/completed/LargeWorlds.md), 2026-10-03 |
| [m1-MotionAndQueries.md](completed/m1-MotionAndQueries.md) | Milestone 1: interpolation between steps, a ground pick, overlap tests, `Plane` and `Frustum`. The sprite clip was later built in [Effects](../plans/completed/Effects.md) | [MotionAndQueries](../plans/completed/MotionAndQueries.md), 2026-10-03 |
