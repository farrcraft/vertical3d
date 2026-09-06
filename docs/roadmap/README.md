# Roadmaps

A roadmap describes what a feature area needs and in what order, for an area nobody has taken
up. It answers "if this were picked up tomorrow, where would it start" — written while the
code is fresh in someone's head, so the ordering survives the gap until it is.

A [plan](../plans/) is the other half of that. A plan is work that has been taken on: it has
phases, state notes kept current as things land, and an end, after which it moves to
[plans/completed/](../plans/completed/). A roadmap has none of those. It is not scheduled, it
claims no date, and it stays here whether or not anything happens. **When a roadmap section is
actually taken up it earns a plan**, and the roadmap points at it.

Two rules a roadmap shares with a plan. It describes the tree as it stands, so it is updated
when the tree moves under it — a roadmap describing code that no longer exists is worse than
no roadmap. And it does not restate decisions: a roadmap may say a decision is needed and what
turns on it, but the decision itself belongs in [adr/](../adr/). See [sdlc.md](../sdlc.md).

A roadmap is also not [TODO.md](../TODO.md). That file collects loose ends, each one
independent of the others; a roadmap exists because the ordering between its items is the
interesting part.

## The roadmaps

| Roadmap | Area |
|---|---|
| [OfflineRendering.md](OfflineRendering.md) | `talyn` and `moya` — the raytracer and the reyes renderer |
