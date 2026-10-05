# ADR-0062: Grid: parse terrain, not map files

**Status**: accepted
**Date**: 2026-10-03
**Documented in**: [api/Grid.md](../api/Grid.md)

## Context

Games that build a `TileGrid` from a map keep their maps in different containers. odyssey's map
is JSON holding rows of glyphs, and a line-oriented text format with legend records and records
for spawns, items and objectives is just as reasonable. Most of a map is game-specific: props,
spawns, items, objectives and start positions. The part every format shares, once parsed, is rows
of glyphs and a legend that says what terrain each glyph is.

## Decision

`api/grid` turns a picture, rows of glyphs with one glyph per tile, and a terrain legend into a
`TileGrid`, and owns nothing else about a map. A glyph the legend does not name is handed back
with every position it appears at, and its tiles are left impassable with no cover. The file,
its container and everything else in a map belong to the game.

## Alternatives

### A shared map file format in `api/grid`
- **For**: one reader, and a map written for one game opens in another.
- **Against**: the containers in use share nothing, so a shared one replaces at least one of them.
  A game may have good reason for its own, such as a format a person reads far more often than
  the program does.
- **Rejected because**: it would take a container away from a game that has a reason for it.

### A Tiled reader in `api/grid`
- **For**: an editor nobody has to write.
- **Against**: no game here has chosen Tiled, and its object layers are a poor fit for the
  non-terrain half of a map. Once a layer's tile ids are mapped to glyphs, a Tiled tile layer is
  rows and a legend again.
- **Rejected because**: it is premature. A Tiled reader, if one is written, can call the function
  decided here.

### Refuse a glyph the legend does not name
- **For**: a typo in a map is an error at load time rather than an impassable tile.
- **Against**: games hang their own meanings on glyphs, such as odyssey's `'@'` for the player's
  start, and those would all have to be in the terrain legend.
- **Rejected because**: handing unknown glyphs back serves both cases. A game that wants them to
  be errors applies that rule on top.

## Consequences

- **Gains**:
  - The terrain half of any game's format arrives at the grid the same way.
  - A game finds its own glyph meanings, such as a start position, among the unknown glyphs.
- **Costs**:
  - Each game still writes its own container and its own legend parsing.
  - A legend gives passability and cover only. Anything else a glyph means, such as a prop or a
    tile's appearance, the game looks up again from the same picture.
  - Ragged rows and an empty picture are refused, so a game that wants to pad a ragged map does it
    before the call.
- **Revisit when**: terrain needs a third property, such as height or a movement cost. That widens
  `Terrain` and touches every legend a game builds.
