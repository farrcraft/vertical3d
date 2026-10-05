# Game Engine

What the realtime api would need for the games built on it to be finished games rather than
demos, and in what order. **As of 2026-10-03 the 2D and ui half of the api is mature and the 3D
half stops at the plumbing**: a fixed-step loop, a retained and an immediate ui, settings,
rebinding, sprite documents and atlases are all used by every consumer, while buffers,
pipelines, offscreen targets and a sampled depth target exist with nothing above them — no lit
mesh, no material beyond one texture, no shadow, no post-processing, no animation of any kind,
no culling, and no way to free a resource once it is registered.

**Every milestone has been done by a plan, the last on 2026-10-04**, and what each set aside
is in [TODO.md](../../TODO.md). The rest of this document is the reasoning they were drafted from,
and describes the tree as it was when it was written.

This roadmap is the index. Each milestone is a document of its own, because each is large
enough to earn a plan when it is taken up and the reasoning for one does not need the others.

## Who this was drawn from

The api is consumed as source ([ADR-0027](../../adr/0027-build-consume-the-api-as-source.md)) and
most of what consumes it is outside this tree, so the evidence came from four places:

* **The api itself** — what each library offers, and the gaps [TODO.md](../../TODO.md) and
  [RenderingPipeline.md](../../api/Rendering.md#what-is-not-built-yet) already record.
* **The apps in this tree** — what each implements locally that is engine-shaped: pong's
  hand-written box tests, voxel's collision stub and raw Vulkan, odyssey's map format and fog
  of war, moya's `Plane` and `Frustum`, and the renderer setup four apps write identically.
* **cozy**, surveyed at `3217bc2`. A narrative game drawn entirely in quads under a fixed
  three-quarter orthographic camera, between its M5 (player and inventory) and M6 (world and
  map). It links audio, config, ecs, engine, event, log, render, type and ui. Its upcoming
  needs are small and specific, and several have a named trigger in its own plans.
* **retcon**, surveyed at `3d22935` and pinned to this tree's `13a9557`. An isometric tactics
  and township game in low-poly 3D with cel shading, outlines, shadows and a colour grade, in
  its phase 5 (township). It takes this tree's device tier, loop, ui and grid, and **has
  written a whole lit 3D tier of its own in `engine/renderer/`** because nothing here offers
  one. Its largest gap, skeletal animation, exists in neither tree.

Both games author engine changes here rather than in their submodule — cozy's ADR-0002 and
retcon's ADR-0041 — so what follows is work for this tree whoever ends up wanting it first.

## The milestones

| # | Milestone | What it is | Drawn from |
|---|---|---|---|
| 1 | [Motion and queries](m1-MotionAndQueries.md) — **done by [a plan](../../plans/completed/MotionAndQueries.md)**, sprite clip aside | Interpolation, sprite clips, a ground pick, box overlap, `Plane` and `Frustum` in `api/type` | cozy M5–M6, pong, voxel, odyssey, moya |
| 2 | [A world larger than the screen](m2-LargeWorlds.md) — **done by [a plan](../../plans/completed/LargeWorlds.md)**, regions and remembered sight aside | Releasing a resource, depth-ordering world quads, culling, a grid map format, remembered sight | cozy M6, retcon phase 6, odyssey |
| 3 | [The renderable component](m3-RenderableComponent.md) — **done by [a plan](../../plans/completed/RenderableComponent.md)**, the mesh component built by milestone 4 | A decision: how the ECS meets the renderer | the open question in [ECSDesign.md](../../api/ECS.md), retcon's `ecs/` |
| 4 | [A lit scene](m4-LitScene.md) — **done by [a plan](../../plans/completed/LitScene.md)**, retcon's adoption aside | Image, sampler and texture classes, `type::Model` onto the device, a lit mesh pass, a shadow map, a post chain | retcon `engine/renderer/`, voxel |
| 5 | [Skeletal animation](m5-SkeletalAnimation.md) — **done by [a plan](../../plans/completed/SkeletalAnimation.md)**, instancing aside | Skins and clips from glTF, GPU skinning, a clip sampler shared with milestone 1, instancing | retcon phases 7–10 |
| 6 | [Effects](m6-Effects.md) — **done by [a plan](../../plans/completed/Effects.md)**, the panned voice aside | Particles, weather, a tint over the world, panned audio | cozy M6–M8, retcon phase 10 |
| 7 | [The shell, finished](m7-ShellAndShipping.md) — **done by [a plan](../../plans/completed/ShellAndShipping.md)**, asynchronous loading aside | The renderer setup into the shell, missing widgets, versioned documents, relative mouse, profiling, async loading | every app, cozy M7–M10, retcon phases 7 and 11 |

## The ordering

```
1 Motion and queries ─────────────────────────────┬──────────> 6 Effects, 2D half
                                                  │
2 A world larger than the screen ──┐              │
                                   ▼              ▼
3 The renderable component ──> 4 A lit scene ──> 5 Skeletal animation
                                   │
                                   └─────────────────────────> 6 Effects, 3D half

7 The shell, finished    (no order inside it, and none against the rest)
```

**Milestones 1 and 2 go first because they are what is due.** cozy's M6 needs a ground pick,
depth-ordered sprites and a texture that can be released, and its plans name M6 as the trigger
for the last of those. Neither waits on another milestone, though each carries a record of
its own to write — a resource's lifetime, and which part of a map is the grid's.

**Milestone 3 is a gate rather than work.** A lit scene built before it is settled what an
entity carries to be drawn is a lit scene built to one game's shape, and retcon's
`MeshRenderer` is the shape it would default to. The decision is small; deciding it after the
mesh tier exists is not.

**Milestone 4 no longer waits on milestone 2.** The texture class it moves here retires what it
owns through the in-flight ring, which milestone 2 decided
([ADR-0061](../../adr/0061-resources-explicit-release-generational-handles.md)).

**Milestones 4 and 5 are strictly ordered, and the order is the point of the roadmap.**
retcon's lit tier works today, so moving it here is not urgent for retcon on its own. But
skinning is a vertex layout, a pipeline and a pass, and it has to be written against some mesh
tier: written against retcon's, it is written twice. [What this needs decided](#what-this-needs-decided)
says what turns on that.

**Milestone 6 has a 2D half and a 3D half.** The 2D half needs milestone 1's clip and nothing
else, and it is the half cozy wants. The 3D half is billboards in a lit scene and waits on
milestone 4.

**Milestone 7 is a collection.** Each piece is taken when a consumer reaches it, and none of
them blocks another.

## What this needs decided

* **Where the lit tier lives** — whether retcon's `engine/renderer/gpu/` and `passes/` move here
  before skeletal animation is written, or animation is written in retcon and moved later.
  Moving it is a handoff from retcon under its ADR-0041, which is how every previous round
  reached this tree. [m4-LitScene.md](m4-LitScene.md) has what the move involves.
* **What a renderable component is** — milestone 3, in its own record.
* **Which part of a map is the grid's** — decided: a picture and a terrain legend, and nothing
  else ([ADR-0062](../../adr/0062-grid-parse-terrain-not-map-files.md)).

## Verification

The device suite and its golden images ([ADR-0054](../../adr/0054-testing-golden-images-hold-only-spec-exact-output.md))
are the only way this tree asserts a picture, and they hold only what the specification
determines pixel for pixel. Most of milestones 4 to 6 is lighting, filtering and blending,
which is exactly what a reference cannot pin — each milestone document says what *can* be
asserted for its own work, and where that is validation silence and a screenshot rather than
a test. Everything in milestones 1, 2 and 7 that is not drawing is headless and is tested the
way the rest of `api/type`, `api/grid` and `api/config` are.

## Not on this roadmap

Each has a trigger rather than a reason it can never happen.

* **Physics.** Neither game needs rigid bodies: retcon's rules are tiles and cover, and cozy
  needs overlap, which milestone 1 covers. A game that needs a body earns it.
* **Networking.** No consumer. pong's header lists it, and pong is not a reason.
* **Scripting.** No consumer. cozy's M10 cutscenes may want a timeline, which is smaller than a
  language and would belong beside milestone 1's clip.
* **Localisation.** Neither game schedules it. Text is UTF-8 through FreeType with no shaping,
  which is the first thing a string table would hit.
* **Gamepad.** retcon has ruled it out and cozy has not scheduled it; `api/input` is keyboard
  and mouse only.
* **The offline renderers.** [OfflineRendering.md](OfflineRendering.md) owns them, and they
  stay out of the realtime work.
