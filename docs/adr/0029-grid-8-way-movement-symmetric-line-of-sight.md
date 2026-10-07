# ADR-0029: Grid: 8-way movement, symmetric line of sight

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [api/Grid.md](../api/Grid.md)

## Context

Tile games need to know how something gets from one tile to another and whether it can see one
tile from another. Three rules decide the answers: what a step costs, what a diagonal may pass
between, and whether sight is the same from both ends. When a movement highlight and a path search
read different rules, the highlight offers a move the search then refuses. The grid also has to
stay ignorant of what stands on it, because occupancy belongs to the simulation and map
generation has no occupants at all.

## Decision

Tile grids are `api/grid`, a library with no device or windowing types. Three rules are fixed in
it. Movement is 8-way at a flat cost per step. A diagonal may not pass between two blocked tiles.
Sight between two tiles gives the same answer from either end. The grid does not hold what may be
entered or what blocks a line, so it asks the caller for those through a predicate. It is never
told what an occupant is.

## Alternatives

### Put it in `api/type` beside `Camera` and `Ray`
- **For**: no new library, and `api/type` is already the shared geometric vocabulary.
- **Against**: `api/type` holds value types; this is algorithms over a container. An app that
  wants a camera would take a pathfinder with it, and the offline renderer links `api/type`.
- **Rejected because**: the two have no consumer in common.

### Leave it to each app
- **For**: nothing to design before there are consumers, and each game prices a diagonal its own
  way.
- **Against**: the corner rule and sight symmetry are easy to get subtly wrong and hard to notice.
  An asymmetric trace lets a unit be shot from a tile it cannot shoot back at, which players
  report as a balance problem rather than a bug.
- **Rejected because**: those two rules are worth one implementation with a test over every
  ordered pair of tiles.

### 4-way movement, or a diagonal priced at about 1.41
- **For**: 4-way has no corner rule to get wrong. A weighted diagonal makes distance Euclidean, so
  a route's cost matches the ground it covers.
- **Against**: 4-way makes a diagonal approach cost twice an orthogonal one, and puts four movers
  rather than eight in contact with a target. A weighted diagonal needs a non-integer cost, so
  every tie in the search becomes a float comparison.
- **Rejected because**: a flat 8-way cost keeps the search in integers and its ties exact, so a
  query gives the same answer every time.

## Consequences

- **Gains**:
  - Each of the three rules has one implementation, tested once.
  - The library names no device, so its tests run in CI unchanged.
  - Terrain cover is readable by both line of sight and an app's own rules, without the grid
    knowing what either does with it.
- **Costs**:
  - A wall costs nothing extra to walk around until the way round leaves the diagonal envelope,
    so 8-way distance under-reports some detours.
  - The caller's predicate is a `std::function` called for every neighbour a search visits.
- **Revisit when**: a board is large enough for the predicate call to matter, which would want it
  templated. Changing the step cost is the hardest change to make later, because every movement
  budget an app authors is measured in it.
