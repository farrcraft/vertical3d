# ADR-0087: Grid: a region is a grid placed by its corner

**Status**: accepted
**Date**: 2026-10-16
**Documented in**: [docs/api/Grid.md](../api/Grid.md)

## Context

A world of regions loads and releases each region with its own picture, terrain and sprite
sheets. Each region needs a place in the world, and today every `TileGrid` is centred on the
world origin. Pathfinding, line of sight and the overlay each take one grid. A game decides when a
region loads and what crosses between regions. The grid has to place a region without taking on
either decision.

## Decision

A `TileGrid` can be given the world position of its minimum corner on the ground plane, and is
centred on the origin when it is not. A region is one grid placed this way. The grid holds no
list of regions, and a search runs within one grid.

## Alternatives

### A grid of grids
- **For**: one object answers which region holds a point, and could run a search across a seam.
- **Against**: the library would decide how regions are loaded and released, and every search
  would have to deal with a region that is not loaded.
- **Rejected because**: loading belongs to the game, and no game yet searches across regions.

### A world offset kept by the game
- **For**: no change to the grid at all.
- **Against**: every call to `tileToWorld`, `worldToTile` and the overlay has to add or subtract
  the offset, and one call site that forgets places things in the wrong region.
- **Rejected because**: the grid already turns tiles into world positions, so it is the one place
  that should know where it is.

## Consequences

- **Gains**: a region's tiles convert to world positions with no code at the call site, and
  existing grids are unchanged.
- **Costs**: a path or a line of sight that crosses regions is the game's to stitch together.
  Two regions placed so that they overlap are not detected.
- **Revisit when**: a game needs a route that crosses regions, which would bring a grid of grids
  back into consideration.
