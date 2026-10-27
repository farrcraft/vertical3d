# Plans

This directory holds the plans for larger pieces of work, open and finished. It is for anyone
who wants to know what is being worked on, or how a past piece of work was ordered.

A plan is written when a piece of work spans several phases or several apps, and the order of
the pieces matters. The question a plan answers is "what blocks what", not "what needs doing".
Smaller, independent items go in [TODO.md](../TODO.md) instead.

- **While a plan is open**, its state notes are kept current as each step lands.
- **When every step is closed**, the plan moves to [completed/](completed/). Any item it did
  not finish moves to [TODO.md](../TODO.md).
- **A completed plan stays in the repository.** It records why the work was ordered as it was.
  Its final "Outcome" section says what was delivered, what turned out differently from the
  plan, and what was left open.
- **A completed plan is frozen.** It is not updated when the code moves, and its links and line
  numbers are not repaired. It describes the tree as it stood when the plan closed.

[sdlc.md](../sdlc.md) describes how plans fit with ADRs, roadmaps and TODO.md.

## Open plans

| Plan | Area | What it delivers |
|---|---|---|

## Completed plans

Newest first.

| Plan | Area | Closed | What it delivered |
|---|---|---|---|
| [EditorScene](completed/EditorScene.md) | Editor, brep, curve, interchange, moya | 2026-10-27 | Cameras, lights, groups and curves in the scene, a SQLite project store with autosave, shaded views, glTF and USD import, rendering with moya, keyed playback, and curve points and handles edited through the manipulators |
| [Reshaping](completed/Reshaping.md) | All of the tree, documents | 2026-10-27 | `RenderContext` as a facade, an engine without Vulkan, an editor document layer, library boundaries redrawn, the demos in `demos/` with a lit one, and each reference fact in one place |
| [ScaleAndReach](completed/ScaleAndReach.md) | Realtime rendering, offline rendering, grid, audio, imagetool | 2026-10-25 | Cascaded shadows, uploads and texture requests that do not block, routes and sight across regions, a placed voice and placed events, sprite sheet packing, and ray hits shaded in batches |
| [SettingsAndPortability](completed/SettingsAndPortability.md) | Build, realtime rendering, audio, engine, event, UI | 2026-10-25 | Settings widgets and a text field in `Immediate`, a canvas segment, key bindings an app can read, reset and save, FMOD bus volume and pause, a requested present mode, wheel zoom and edge pan, a portability driver, and the platform-free libraries built with Clang on Linux |
| [Hardening](completed/Hardening.md) | moya, shading language, image, voxel, apps | 2026-10-24 | RI's C interface and the reyes hider following RI, `output`, `faceforward` and message passing as the shading language defines them, safe image writers, a voxel camera that turns correctly, and the known review findings fixed |
| [EditorModelling](completed/EditorModelling.md) | Editor, brep, ui, engine | 2026-10-24 | A dirty flag that asks before discarding work, selection sets and a rubber band, component moves, split, extrude, inset and merge with undo, fixed views and resizable panes |
| [OfflineShading](completed/OfflineShading.md) | Offline rendering, moya | 2026-10-18 | Rotations checked against the specification, a bounding volume hierarchy, derivatives under both hiders, area lights, reyes opacity, and displacement under both hiders |
| [ToolingDebt](completed/ToolingDebt.md) | All of the tree, tests | 2026-10-18 | The thirteen disabled clang-tidy checks enabled, a stronger frame test, a pipeline cache test, the glTF generators in TypeScript, and exact grade tests |
| [ReleaseHardening](completed/ReleaseHardening.md) | Engine, input, ui, assets, rendering, moya, apps | 2026-10-17 | UTF-8 paths, a minimised window that waits, a command registry every app's bindings are checked against, and the known review findings fixed |
| [LitScenesAtScale](completed/LitScenesAtScale.md) | Realtime rendering, audio, pong | 2026-10-17 | Lit objects in a storage buffer, instanced lit and shadow draws, a grade that reads its table as shown, and a panned voice |
| [Regrouping](completed/Regrouping.md) | Realtime rendering, image, UI, moya | 2026-10-16 | `render/realtime`, `image` and `libmoya` split into directories by purpose, every namespace in `api/` following its directory, and a sweep of libmoya's comments |
| [StreamingWorlds](completed/StreamingWorlds.md) | Assets, realtime rendering, grid, voxel, odyssey | 2026-10-16 | A worker thread for loading, mesh requests under an upload budget, uploads that do not drain the queue, grids placed as regions, odyssey as a world of two regions, and a shadow map fitted to the view |
| [RetconUpstream](completed/RetconUpstream.md) | Realtime rendering, UI, ECS, engine, audio | 2026-10-08 | Frame capture on `Engine3D`, four renderer seams a game on the other camera hand or an sRGB chain needs, five small helpers, and audio events behind an interface |
| [ReviewGates](completed/ReviewGates.md) | Process, CI | 2026-10-07 | A changeset reviews clean: a finding is what the change introduced, and four classes a tool can find are gated in CI |
| [ReviewFixes](completed/ReviewFixes.md) | All of `api/`, moya, pong, voxel, docs | 2026-10-05 | Fixed the findings of a code review of `feat/motion-and-queries`, each with a test that failed first, and recorded the four that were not defects |
| [DocumentationRefresh](completed/DocumentationRefresh.md) | Documentation | 2026-10-05 | Plain-language docs split by reader, ADRs renamed and rewritten with a qualification test, and comments that state their rules without citing ADRs |
| [ApiDesignDebt](completed/ApiDesignDebt.md) | All of `api/` | 2026-10-05 | Fixed the twelve defects found by the api design review, and gave each rule that was written in several places one implementation |
| [OfflineRenderingPhases4To6](completed/OfflineRenderingPhases4To6.md) | Offline rendering | 2026-10-04 | Seeded samples in a shared film, depth of field, motion blur, recursive ray tracing, and one ray tracer for both renderers |
| [ShellAndShipping](completed/ShellAndShipping.md) | UI, engine, apps | 2026-10-04 | Game engine milestone 7: one shared screen for the UI renderers, held commands, relative mouse, document migration, a file chooser, pass timings and a slider |
| [Effects](completed/Effects.md) | Realtime rendering, ECS | 2026-10-04 | Game engine milestone 6: sprite clips, a seeded random source, particles, weather, a world tint and a colour grade |
| [SkeletalAnimation](completed/SkeletalAnimation.md) | Models, animation | 2026-10-03 | Game engine milestone 5: models in parts, skins and clips from glTF, and skinning in both lit passes |
| [LitScene](completed/LitScene.md) | Realtime rendering | 2026-10-03 | Game engine milestone 4: images and samplers, a mesh registry, a lit pass, shadows and a post-processing chain |
| [RenderableComponent](completed/RenderableComponent.md) | ECS, realtime rendering | 2026-10-03 | Game engine milestone 3: a transform component, a sprite component and a function that draws every sprite |
| [LargeWorlds](completed/LargeWorlds.md) | Realtime rendering, grid | 2026-10-03 | Game engine milestone 2: releasable textures, ordered world quads, chunk culling in voxel, and a grid built from a map picture |
| [MotionAndQueries](completed/MotionAndQueries.md) | Types, ECS | 2026-10-03 | Game engine milestone 1: `Plane` and `Frustum` in `api/type`, ray and box queries, and interpolation between steps |
| [RealtimeGoldenImage](completed/RealtimeGoldenImage.md) | Testing | 2026-09-12 | Four device-test pictures compared byte for byte against committed reference images |
| [RenderTestsInCI](completed/RenderTestsInCI.md) | Testing, CI | 2026-09-12 | A device test suite for `api/render/realtime` that runs on a software Vulkan driver in CI |
| [OfflineRenderingPhase3](completed/OfflineRenderingPhase3.md) | Offline rendering | 2026-09-10 | A shading language, with surface and light shaders |
| [ApiOrganisation](completed/ApiOrganisation.md) | `api/` layout | 2026-09-08 | Includes named from the repository root, and six libraries reorganised into subdirectories |
| [EmbeddingSeams](completed/EmbeddingSeams.md) | Engine, UI, image, rendering | 2026-09-07 | Seven changes that let an app bring its own UI, renderer and `main` |
| [GameFoundations](completed/GameFoundations.md) | Files, rendering, UI, audio | 2026-09-07 | Atomic document writes, textured quads in world space, and the missing halves of `api/ui` and `api/audio` |
| [UiConsolidation](completed/UiConsolidation.md) | UI | 2026-09-07 | A restructured `api/ui` draw path, a style resolver, and three UI defects fixed |
| [UiFoundations](completed/UiFoundations.md) | UI, text | 2026-09-06 | Signed distance field text, menu input capture, a user settings path and a window size an app can set |
| [GameLoopFoundations](completed/GameLoopFoundations.md) | Engine | 2026-09-06 | A fixed simulation step, a window focus event, and the camera profile loader in `api/config` |
| [ExternalApiConsumption](completed/ExternalApiConsumption.md) | Build | 2026-09-05 | The `api/` libraries built as source inside another repository, shown by `examples/starter` |
| [OfflineRenderingPhase2](completed/OfflineRenderingPhase2.md) | Offline rendering | 2026-09-05 | One RIB reader shared by both renderers, and RIB export from the editor |
| [OfflineRenderingPhase1](completed/OfflineRenderingPhase1.md) | Offline rendering | 2026-09-05 | The shared `api/render/offline` library, and each renderer computing pixels from geometry |
| [Modernization](completed/Modernization.md) | Whole tree | 2026-09-04 | SDL3, Vulkan, the engine consolidation, the per-app ports, and the removal of the legacy trees |
