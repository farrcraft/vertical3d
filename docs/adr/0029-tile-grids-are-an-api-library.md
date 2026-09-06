# ADR-0029: Tile Grids — A Library Of Their Own, 8-Way On The Ground Plane, Asked Rather Than Told What Blocks

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Nothing in the tree can answer "how does something get from this tile to that one, and can it
see it from here". `odyssey` is a tile roguelike with no pathfinder, `voxel` addresses a chunk
by integer coordinate and computes nothing over it, and the editor's picking
([ADR-0014](0014-picking-is-a-cpu-ray-cast.md)) is a ray against a mesh rather than against a
board.

A tile grid is where three decisions meet that are cheap to make once and expensive to disagree
about later: what a step costs, what a diagonal may pass between, and whether sight between two
tiles is the same question from both ends. An app that answers them for itself answers them
twice — a movement highlight and a path search that read different rules offer a move the
search then refuses — and an app that answers them differently from another app has no shared
vocabulary at all.

The grid also has to stay ignorant of what is standing on it. Occupancy changes every turn or
every frame, belongs to whatever is simulating, and putting it in the grid would make the type
useless for map generation, which has no occupants at all.

## Decision

Tile grids are `api/grid` — `v3dlib_grid`, namespace `v3d::grid` — holding `TileGrid`, the
pathfinding over it, and line of sight across it, and nothing that names a Vulkan, SDL or
windowing type.

Three rules are fixed in it rather than left to a caller:

- **Movement is 8-way at a flat cost per step**, so distance on open ground is Chebyshev and a
  budget of N reaches a square of side 2N + 1. `tileDistance()` is that metric and is the
  admissible heuristic the search uses.
- **A diagonal may not pass between two blocked tiles**, and may pass a single blocked corner.
  One predicate serves the path search, the reachable set and the distance field, so a tile the
  highlight offers is a tile the move can reach for the cost shown.
- **Sight is symmetric.** The two endpoints are put in a fixed order before anything is traced,
  so one line answers both directions; only `Cover::Full` blocks, and neither endpoint is
  tested.

What the grid cannot know it is **asked** for, as a caller-supplied predicate: `TileFilter` for
what may be entered and `SightBlocker` for what stops a line. A default-constructed one imposes
nothing, and neither is ever told what an occupant is.

## Alternatives Considered

### Alternative 1: A library of its own, with caller predicates — **chosen**
- **Pros**: Device-free, so it is covered by ctest in CI where most of `api/render` is not. The
  three rules are asserted once. An app supplies occupancy without the grid learning what
  occupancy is.
- **Cons**: A fifteenth api library for 500 lines, and `std::function` in the inner loop of a
  search.
- **Why not**: n/a — chosen.

### Alternative 2: Fold it into `api/type` beside `Camera` and `Ray`
- **Pros**: No new target. `api/type` is already the shared geometric vocabulary
  ([ADR-0024](0024-api-type-serves-both-renderers.md)).
- **Cons**: `api/type` holds value types that carry a convention; this holds algorithms over a
  container. An app that wants a camera would take a pathfinder with it, and the offline
  renderers link `api/type` for its maths.
- **Why not**: The two have no consumer in common. A library nothing forces together is not a
  library.

### Alternative 3: Leave it to each app
- **Pros**: Nothing to design for consumers that do not exist yet. Each app prices a diagonal
  the way its own game wants.
- **Cons**: The corner rule and the sight symmetry are the two things easiest to get subtly
  wrong and hardest to notice — an asymmetric trace shows up as being shot from somewhere you
  cannot shoot back at, which reads as a balance complaint rather than as a bug.
- **Why not**: Rejected on that asymmetry alone. It is worth one implementation with a sweep
  over every ordered pair behind it.

### Alternative 4: 4-way movement, or a diagonal priced at 1.41
- **Pros**: 4-way has no corner rule to get wrong. A weighted diagonal makes the distance metric
  Euclidean, so a route's cost matches the ground it covers.
- **Cons**: 4-way makes a diagonal approach cost twice an orthogonal one and puts four movers
  rather than eight in contact with a target. A weighted diagonal needs a non-integer cost, and
  with it every tie in the search becomes a float comparison.
- **Why not**: The flat 8-way cost is what makes the search integral and its ties exact, which
  is what makes a query reproducible. The price is that a wall costs nothing until the way round
  leaves the diagonal envelope, which the tests state outright.

## Consequences

### Positive
- The three rules have one implementation and 73 cases over it, including a sweep of sight over
  every ordered pair of a scattered board, which is what an asymmetric trace fails.
- Nothing in the library names a device, so its suite runs in CI unmodified.
- `Cover` gives terrain a height that both sight and an app's own rules can read, without the
  grid knowing what either does with it.

### Negative
- `TileFilter` is a `std::function` called for every neighbour of every tile a search visits. A
  board large enough for that to matter would want the predicate templated instead.
- The default tile size is a number in a header. An app authoring against a different scale
  passes its own and the constant is only a default.

### Risks
- The 8-way flat cost is the hardest of the three rules to change later, because a route's
  length is what every consumer's movement budget is denominated in. Changing it is a change to
  `tileDistance()` and to the two step costs together, and to whatever an app has authored
  against them.
