# Plans

A workstream earns a plan document when it spans several phases or several apps and the
ordering between the pieces is not obvious — when the interesting question is *what blocks
what*, not *what needs doing*. See [sdlc.md](../sdlc.md).

A plan is a living document while it is open, so update its state notes as things land. When
every phase is closed it moves to [completed/](completed/), and any open item it was carrying
moves to [TODO.md](../TODO.md). The plan itself stays, because the reasoning behind an ordering
outlives the schedule.

[GameLoopFoundations.md](GameLoopFoundations.md) **is open**, drafted on 2026-09-05 against this
tree from outside it and staged here on 2026-09-06. It takes up three gaps in the game loop that
every app subclassing `v3d::engine::Engine` has worked around separately or has failed to: a fixed
simulation step ([ADR-0032](../adr/0032-the-loop-simulates-at-a-fixed-step.md)), window focus as an
event, and the camera profile loader moving out of the editor into `api/config`. The ordering
matters because the capability and the two-app bug fix behind it are separate commits.

[OfflineRenderingPhase3.md](OfflineRenderingPhase3.md) **is open**, drafted on 2026-09-05. It
takes up phase 3 of [the offline rendering roadmap](../roadmap/OfflineRendering.md) — light and
surface — and answers the question that roadmap left open: shading is a language rather than a
fixed set of shaders. That answer makes it a subsystem rather than a weekend, and phases 4 and
5 sit behind it.

[completed/ExternalApiConsumption.md](completed/ExternalApiConsumption.md) was drafted and
closed on 2026-09-05. It made the `api/` libraries buildable inside another repository's tree,
by the route [ADR-0027](../adr/0027-the-api-is-consumed-as-source.md) settles: as source,
through a root that nests, rather than as an installed package. Most of it was a correction the
tree needed regardless, since a library now states its own include root and propagates its own
dependencies where both used to come off globals in the root `CMakeLists.txt`.

[completed/OfflineRenderingPhase2.md](completed/OfflineRenderingPhase2.md) and
[completed/OfflineRenderingPhase1.md](completed/OfflineRenderingPhase1.md) were both drafted
and closed on 2026-09-05, taking up the first two phases of that roadmap: the shared
`api/render/offline` library and each renderer computing a pixel that came from geometry, then
one RIB reader dispatching onto an interface both renderers implement, and the editor's export
to the same format.

[completed/Modernization.md](completed/Modernization.md) closed on 2026-09-04, covering the
overlapping rewrites: SDL3, the legacy tree migration, Vulkan, the engine consolidation, and
the per-app ports.
