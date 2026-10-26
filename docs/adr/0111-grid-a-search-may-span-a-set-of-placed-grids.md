# ADR-0111: Grid: a search may span a set of placed grids

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0087
**Documented in**: [docs/api/Grid.md](../api/Grid.md)

## Context

A world of regions is a set of grids, each placed by its corner, and a game loads and releases
them as a player moves. A mover that follows the player, or a route clicked into the next region,
has to cross a seam, and a viewer by a seam has to see past it. Joining two searches made within
one grid each misses a route that leaves a region and comes back into it. Two games need this,
and each hands the search the grids it has loaded rather than copies of them.

## Decision

`findPath`, `reachableTiles`, `DistanceField` and `hasLineOfSight` also take a set of placed
grids and work in world tiles, with the movement and sight rules of one grid. The set holds
references to the grids the game says are loaded, refuses grids whose tile sizes differ, whose
corners are off the tile lattice or that overlap, and treats a tile in no grid as impassable and
opaque. One grid is a set of one, so there is one search.

## Alternatives

### Stitch the loaded grids into one
- **For**: the searches stay as they are, and a stitched grid is searched as fast as any other.
- **Against**: the copy is made again at every load and release, and every game with regions
  writes the stitching.
- **Rejected because**: it copies what the game already holds, for a seam the library can cross.

### Searches over world tiles through callbacks
- **For**: the most general form, for a world of any shape or one made as it is searched.
- **Against**: every caller writes the lookup from a world tile to its region, and the checks on
  tile size and alignment, and a search pays a call for every question it asks.
- **Rejected because**: the set is the one lookup every game with regions needs, and a made world
  still joins it as one more grid.

### A region not loaded is unknown ground a route may plan through
- **For**: a long route can head for a region before it loads.
- **Against**: the route has to be planned again once the region arrives, and may not exist.
- **Rejected because**: a route is never invalidated by a region arriving if the region was a wall
  until then, and a mover that follows the player only needs what is loaded.

## Consequences

- **Gains**: a route, a reachable set, a distance field and a line of sight cross a seam with the
  same rules as within one grid, and a route may leave a region and come back.
- **Costs**: a search looks each tile up among the grids of the set, which costs a little over
  one grid. A set names grids by reference, so it is built for a search and not kept past a
  release. A route stops at the edge of what is loaded, however far the goal is.
- **Revisit when**: a game needs a route to a region before it loads, or a set grows to more
  grids than a lookup among them can bear.
