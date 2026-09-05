# Plans

A workstream earns a plan document when it spans several phases or several apps and the
ordering between the pieces is not obvious — when the interesting question is *what blocks
what*, not *what needs doing*. See [sdlc.md](../sdlc.md).

A plan is a living document while it is open: update its state notes as things land. When
every phase is closed it moves to [completed/](completed/), and any open item it was carrying
moves to [TODO.md](../TODO.md) — a finished plan is a bad place to leave one. The plan itself
stays, because the reasoning behind an ordering is worth more than the schedule was.

**None is open.**

[completed/OfflineRenderingPhase1.md](completed/OfflineRenderingPhase1.md) was drafted and closed
on 2026-09-05, taking up phase 1 of
[the offline rendering roadmap](../roadmap/OfflineRendering.md): the shared `api/render/offline`
library, talyn's split into a library and a driver, and each renderer computing a pixel that came
from geometry. Phase 2 of that roadmap is the next thing to earn a plan.

[completed/Modernization.md](completed/Modernization.md) closed on 2026-09-04, covering the
overlapping rewrites: SDL3, the legacy tree migration, Vulkan, the engine consolidation, and the
per-app ports.
