# Tile grids

This page is for someone writing an app against the api. It covers `api/grid`: tile boards,
coordinates, movement and pathfinding, line of sight, building a board from a picture, and the
geometry for drawing an overlay. Terms are defined in the [glossary](README.md#glossary).

- [What the library is](#what-the-library-is)
- [The board and its coordinates](#the-board-and-its-coordinates)
- [Movement](#movement)
- [Line of sight](#line-of-sight)
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
  zone a unit will not cross.
- `SightBlocker` (`std::function<bool(TileCoord)>`) says whether a tile stops sight, on top of
  the grid's own cover. Use it for smoke or a closed shutter.

A default-constructed predicate adds no restriction. Because the grid never learns about
occupants, the same type serves map generation, which has none, and a running game.

## The board and its coordinates

`TileGrid` ([api/grid/TileGrid.h](../../api/grid/TileGrid.h)) is a rectangle of tiles lying flat
on the `y = 0` plane, centred on the world origin.

- `TileGrid(width, height, tileSize)` throws `std::invalid_argument` if any value is not
  positive. The default tile size, `TileGrid::DEFAULT_TILE_SIZE`, is 1.5 world units, which
  suits a figure about 1.8 units tall.
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

How far a unit can see, and what it remembers seeing, are the game's. The grid answers only
about two tiles.

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
  `realtime::WorldCanvas` expects. The colour's alpha arrives unchanged, so a translucent fill
  blends if the pass blends.
- Everything is lifted `OVERLAY_LIFT` (0.01 world units) above the ground so it does not
  z-fight with ground drawn at `y = 0`. Draw every layer of one overlay with the same lift.
- A tile off the board emits nothing.

[rendering/](rendering/README.md) covers the line primitive and `WorldCanvas`.

## An example: odyssey

odyssey is the app that uses this library:

- It reads its board from `data/map.json`, a list of rows with one character per tile, and
  builds the grid with `fromPicture()`. Its own `tile::Kind` decides passability and cover.
- A click routes the player with `findPath()`.
- `tile::Sight` calls `hasLineOfSight()` to find what the player can see.
- The map format, how far the player sees and what the player remembers are odyssey's own.
