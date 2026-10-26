# Tile grids

This page is for someone writing an app against the api. It covers `api/grid`: tile boards,
coordinates, movement and pathfinding, line of sight and fog of war, building a board from a picture, and the
geometry for drawing an overlay. Terms are defined in the [glossary](README.md#glossary).

- [What the library is](#what-the-library-is)
- [The board and its coordinates](#the-board-and-its-coordinates)
- [Movement](#movement)
- [Line of sight](#line-of-sight)
- [Remembered sight](#remembered-sight)
- [Building a board from a picture](#building-a-board-from-a-picture)
- [Drawing an overlay](#drawing-an-overlay)
- [An example: odyssey](#an-example-odyssey)

## What the library is

`v3dlib_grid`, namespace `v3d::grid`, depends on glm only. It names no device, window or
renderer, so it runs and is tested anywhere.

The grid holds terrain and nothing else. It does not record what stands on a tile. When a query
needs to know about occupants, the caller passes a predicate:

- `TileFilter` (`std::function<bool(TileCoord)>`) says whether a tile may be entered, on top of
  the grid's own passability. Close it over whatever the app tracks: occupancy, ownership, a
  zone a unit will not cross. A search calls it for every neighbour it visits. On a 256 by 256
  board, in an optimised build, that adds a few percent to a search, so it is a function object
  rather than a template parameter.
- `SightBlocker` (`std::function<bool(TileCoord)>`) says whether a tile stops sight, on top of
  the grid's own cover. Use it for smoke or a closed shutter.

A default-constructed predicate adds no restriction. Because the grid never learns about
occupants, the same type serves map generation, which has none, and a running game.

## The board and its coordinates

`TileGrid` ([api/grid/TileGrid.h](../../api/grid/TileGrid.h)) is a rectangle of tiles lying flat
on the `y = 0` plane, centred on the world origin unless it is placed by its corner.

- `TileGrid(width, height, tileSize)` throws `std::invalid_argument` if a dimension is not
  positive, the tile size is not a finite positive number, or the tile size makes a side too long
  to hold in a float. `TileGrid::refusal()` gives the same reason without throwing. The default
  tile size, `TileGrid::DEFAULT_TILE_SIZE`, is 1.5 world units, which suits a figure about 1.8
  units tall.
- `TileGrid(width, height, tileSize, corner)` places the grid's minimum corner, the outer corner
  of tile (0, 0), at a world position. The corner's `y` is ignored. A corner whose `x` or `z` is
  not finite throws `std::invalid_argument`. `worldMin()` returns the corner either way.
- A `TileCoord` is `{x, y}`. **`x` runs along world `+X` and `y` along world `+Z`**, so the
  second axis is depth, not height.
- `tileToWorld(tile)` gives the centre of a tile on the ground. `worldToTile(position)` gives the
  tile a point falls in, ignoring `y`. It floors, so a point on a boundary belongs to the higher
  tile. It does not clamp: a point off the board gives a coordinate off the board.
- `contains(tile)` says whether a coordinate is on the board. Coordinates off the board are
  allowed everywhere so that picking can tell "off the board" from "the edge tile".
- `index(tile)` is `y * width + x`, for any per-tile array the app keeps. Check `contains()`
  first.

Each tile has two independent properties:

- **Passability**, through `passable()` and `setPassable()`. A tile off the board is impassable,
  so a path search needs no bounds check.
- **Cover**, through `cover()` and `setCover()`: `Cover::None`, `Half` or `Full`, declared in
  increasing height. Only `Full` blocks sight. A tile off the board has no cover.

Neither property implies the other. Typical combinations are open ground (passable, `None`), a
doorway (passable, `None`), a crate or low wall (impassable, `Half`) and a solid wall
(impassable, `Full`). Placing a wall means setting both.

Reading off the board is allowed. **Writing off the board throws `std::out_of_range`.**

### Regions

A world larger than one board is made of regions, and **a region is a grid placed by its
corner**. The game loads and releases each region, and decides which one a point is in. Two
grids placed side by side share a seam. `fromPicture(rows, legend, tileSize, corner)` places
a grid read from a picture the same way. A point on the seam belongs to the higher tile, which is
the first column or row of the grid beyond it. The grid before the seam puts the point just off
its board.

Two grids placed so that they overlap are not detected by the grids themselves. A `GridSet`
refuses them.

### Searching across regions

`GridSet` ([api/grid/GridSet.h](../../api/grid/GridSet.h)) is the grids a game has loaded, which
a search treats as one board in world tiles. `findPath`, `reachableTiles`, `DistanceField` and
`hasLineOfSight` each take one in place of a grid.

```cpp
v3d::grid::GridSet loaded;
for (const TileGrid& region : regionsLoadedNow) {
    loaded.add(region);   // throws std::invalid_argument with refusal()'s reason
}
std::vector<TileCoord> route = v3d::grid::findPath(loaded, playerTile, goalTile);
```

- **World tile (x, y) is the tile whose minimum corner is at world X = x * tileSize,
  Z = y * tileSize.** `origin(grid)` is the world tile a grid's tile (0, 0) is, and the filter
  and the blocker are asked about world tiles.
- **A tile in no grid of the set is impassable and blocks sight.** A region that is not loaded
  is a wall: a route never plans through it and sight never sees into it. A route found over
  what is loaded therefore stays valid when another region arrives.
- **The rules are one grid's.** A diagonal step between two blocked tiles is refused when they
  lie in different grids, sight is symmetric across a seam, and a route may leave a region and
  come back into it, which two routes joined at the seam cannot find.
- **A grid joins only when it fits.** `refusal(grid)` says why one cannot: a tile size other than
  the set's, a corner that is not a whole number of tiles from the world's origin, or a tile a
  grid already in the set holds.
- **A set holds references.** Build it from what is loaded when a search is made, and do not keep
  it past a release. A `DistanceField` keeps where the grids lie and reads none of them after it
  is built, so it may outlive them.
- `GridSet::local(grid)` is one grid in its own coordinates. Every search over one grid runs as a
  search over that set, so the two give the same route, tie for tie.
- `Sight::look(set, board, from)` looks at one grid of a set from a world tile. Each region keeps
  its own `Sight`, and each is looked at in turn, so a viewer by a seam sees into the region beside
  it and remembers what it saw.

Background: [ADR-0087](../adr/0087-grid-a-region-is-a-grid-placed-by-its-corner.md),
[ADR-0111](../adr/0111-grid-a-search-may-span-a-set-of-placed-grids.md)

## Movement

The movement rules are in [api/grid/Pathfinding.h](../../api/grid/Pathfinding.h):

- **Movement is 8-way, and a diagonal step costs the same as an orthogonal one.**
  `ORTHOGONAL_STEP_COST` and `DIAGONAL_STEP_COST` are both 1. Distance on open ground is
  therefore Chebyshev distance: a budget of N reaches a square of side 2N + 1.
- **`tileDistance(a, b)` is the distance for anything measured in tiles.** It is the Chebyshev
  distance, ignoring obstacles. Use it rather than inventing another metric, so that ranges and
  movement costs agree.
- **A diagonal step may not pass between two blocked tiles.** A wall laid corner to corner is
  solid, and two units standing corner to corner cannot be slipped between. A diagonal may cut
  past a single blocked corner.
- **The tile the mover stands on is never tested** against the grid or the filter, so a unit is
  never trapped by its own tile.

The three queries share one rule set, so a tile one of them offers is a tile the others agree
on:

| Function | Answers |
|---|---|
| `findPath(grid, start, goal, filter)` | The cheapest route, start first and goal last. Its cost is `size() - 1`. A route to the tile you stand on is one tile long. An empty result means there is no route. |
| `reachableTiles(grid, start, budget, filter)` | Every tile reachable for at most `budget`, cheapest first, as `ReachableTile{tile, cost}`. Includes `start` at cost 0. Throws for a negative budget. |
| `DistanceField(grid, goal, filter)` | The cost of the cheapest route to `goal` from every tile. `cost(tile)` returns `DistanceField::UNREACHABLE` for a tile off the board or with no route; `reaches(tile)` is the same as a bool. |

- Ties are broken by a fixed neighbour order, so the same query on the same board always gives
  the same path.
- A movement highlight drawn from `reachableTiles()` never offers a tile `findPath()` then
  refuses, and the costs match.
- Use a `DistanceField` to move towards a goal. `tileDistance()` ignores walls, so the tile it
  calls closest to a goal behind a wall is the tile against the wall, and a unit that moves
  there is stuck. Building a field floods the whole board, so build it once outside a loop over
  candidate tiles.
- The goal of a `DistanceField` is not tested against the grid or the filter, in the same way
  `findPath()` does not test its start. An occupied goal is still reachable.

Background: [ADR-0029](../adr/0029-grid-8-way-movement-symmetric-line-of-sight.md)

## Line of sight

[api/grid/LineOfSight.h](../../api/grid/LineOfSight.h) has two functions.

`hasLineOfSight(grid, from, to, blocker)` says whether nothing blocks sight between two tiles:

- **Sight is symmetric.** `hasLineOfSight(a, b)` and `hasLineOfSight(b, a)` give the same
  answer, because the endpoints are put in a fixed order before the line is traced. Nothing can
  see something that cannot see it back.
- **Only `Cover::Full` blocks**, plus whatever the `SightBlocker` adds.
- **The endpoints are never tested.** A unit standing in cover can see out. Sight between a tile
  and itself, or between neighbours, is always clear.
- Where the line passes exactly through the point four tiles share, it is blocked only if both
  tiles it passes between block. This matches the diagonal movement rule.
- An endpoint off the board has no sight.
- A `SightBlocker` must give the same answer whoever is asking, or sight stops being symmetric.

`sightLine(grid, from, to)` returns the tiles the line passes through, `from` first and `to`
last. Consecutive tiles are one 8-way step apart, so it is the path a shot travels. Reversing
the arguments reverses the result exactly. It is empty when either endpoint is off the board.

**Do not test `sightLine()` tiles to decide whether sight is clear.** Where the line squeezes
through a corner, the two tiles beside it are not in the list. Call `hasLineOfSight()` instead.

### Remembered sight

`Sight` ([api/grid/Sight.h](../../api/grid/Sight.h)) is fog of war: what a viewer can see now,
and what it has seen before.

```cpp
v3d::grid::Sight sight(7);                  // the range, in tiles
sight.look(grid, player);                   // and a SightBlocker, if the game has one
if (sight.visible(tile)) { /* draw it */ }
else if (sight.remembered(tile)) { /* draw it dimmed */ }
```

- **Each tile is in sight, remembered, or neither.** `look()` replaces what is in sight and adds
  to what is remembered. `blind()` puts every tile out of sight and forgets nothing, for a board
  the viewer is not on.
- **The range is given to the constructor**, in Chebyshev steps, so the tiles within it form a
  square, as a movement budget does. A range below zero sees nothing.
- Sight within the range is `hasLineOfSight()`, so it is symmetric and takes the same blocker.
- Every `look()` traces again, even from the same tile, so a change to the board shows at once.
- A board of another size is a different board: the first look at it forgets everything.
- A tile off the board is neither in sight nor remembered, so no bounds test is needed first.

## Building a board from a picture

`fromPicture(rows, legend, tileSize)` in [api/grid/Picture.h](../../api/grid/Picture.h) builds a
`TileGrid` from rows of glyphs, one glyph per tile.

```cpp
std::map<char, v3d::grid::Terrain> legend{
    {'.', {true,  v3d::grid::Cover::None}},
    {'#', {false, v3d::grid::Cover::Full}},
    {'o', {false, v3d::grid::Cover::Half}},
};
v3d::grid::Picture picture = v3d::grid::fromPicture({"#####", "#.@o#", "#####"}, legend);
if (!picture.grid) {
    logger->get()->error("bad map: {}", picture.error);
}
for (const v3d::grid::Unknown& unknown : picture.unknown) {
    // unknown.glyph is '@', and unknown.tiles lists where it stands
}
```

- Row `y` of the picture is tile row `y`.
- A `Terrain` is `{passable, cover}`. The default is passable with no cover.
- **A glyph the legend does not name is returned, not refused.** `Picture::unknown` lists each
  one in the order it first appears, with every tile it stands on. Those tiles are left
  impassable with no cover, for the game to set. A game that treats an unknown glyph as an error
  checks the list itself.
- The picture is refused, with no grid and a reason in `error`, when the tile size is not
  positive, there are no rows, a row is empty, or rows differ in length. None of these throws.
  Pad a ragged map before the call.

**The grid parses terrain only.** The file the rows came from, its format, and everything else
a map means belong to the game: props, spawns, items, objectives and the start position. A
game finds those among the unknown glyphs or in its own records. Anything a glyph means beyond
passability and cover, such as its appearance, the game looks up again from the same picture.

Background: [ADR-0062](../adr/0062-grid-parse-terrain-not-map-files.md)

## Drawing an overlay

[api/grid/Overlay.h](../../api/grid/Overlay.h) computes overlay geometry and hands it to a
callback. It does not draw. The library therefore does not depend on the renderer, and every
case can be tested without a device.

| Function | Emits |
|---|---|
| `outlineTile(grid, tile, colour, lineSink)` | The four edges of one tile: a cursor highlight or a selection. |
| `outlineGrid(grid, interior, border, lineSink)` | Every tile boundary, with the outer edge in its own colour. A w × h board emits w + h + 2 segments. |
| `fillTile(grid, tile, colour, quadSink)` | One filled tile. |
| `fillTiles(grid, tiles, colour, quadSink)` | A run of filled tiles in one colour: a movement range or an area of effect. Tiles off the board are skipped. |
| `tileCorners(grid, tile)` | The four corners of a tile, in order around its edge, starting at the minimum corner and turning toward `+z`. |

- A `LineSink` receives `(from, to, colour)`. Forward it to the renderer's world-space line
  primitive.
- A `QuadSink` receives four corners and a colour. The corners arrive in the order
  `realtime::canvas::World` expects. The colour's alpha arrives unchanged, so a translucent fill
  blends if the pass blends.
- Everything is lifted `OVERLAY_LIFT` (0.01 world units) above the ground so it does not
  z-fight with ground drawn at `y = 0`. Draw every layer of one overlay with the same lift.
- A tile off the board emits nothing.

[rendering/](rendering/README.md) covers the line primitive and `canvas::World`.

## An example: odyssey

odyssey is the app that uses this library:

- Its world is two regions side by side. Each reads its board from a list of rows with one
  character per tile, and builds a grid placed by its corner with `fromPicture()`. Its own
  `tile::Kind` decides passability and cover.
- A region is loaded on a worker as the player approaches it and released once the player is
  far away. A step crosses the seam between two loaded regions.
- A click routes the player with `findPath()` over a `GridSet` of the loaded regions, so a route
  goes through the doorway into the next region. Sight is traced across the same set.
- Each region keeps a `Sight`, which outlives the region's map, so a region released and loaded
  again is remembered as it was.
- The map format and how far the player sees are odyssey's own.
