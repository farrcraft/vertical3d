# ADR-0062: Grid: parse terrain, not map files

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Two map formats exist, and they share almost nothing. odyssey's [`Map`](../../odyssey/tile/Map.h)
is JSON holding a `"tiles"` array of rows, with the glyphs fixed in code. retcon's `MapFile` is a
line-oriented text file: `legend` records mapping a glyph to passability, cover and a prop, a
`tiles`…`end` block, and records for spawns, a horde, items and objectives. retcon's ADR-0036
rejected Tiled because the non-terrain half costs too much to author there, and its phase 6
generator has to keep emitting the format with `std::format`. cozy has yet to choose between a
format of its own and Tiled. [Milestone 2](../roadmap/completed/m2-LargeWorlds.md) asks which part of a map
belongs to `api/grid`, and [Large worlds](../plans/completed/LargeWorlds.md#step-6--a-grid-from-a-picture-and-a-legend)
is the plan that answers it.

## Decision

**`api/grid` turns a picture and a terrain legend into a `TileGrid`, and owns nothing else about
a map.** The picture is rows of glyphs, one per tile, with picture row *y* as tile row *y*. The
legend maps a glyph to passability and cover. A glyph the legend does not name is handed back
with every position it appears at, rather than refused, and its tiles are left impassable with
no cover. The file, its container, and everything else in a map belong to the game: props,
spawns, items, objectives and start positions, keyed by glyph or by record.

## Alternatives Considered

### Alternative 1: A shared file format in `api/grid`
- **Pros**: One reader, and a map written for one game opens in another.
- **Cons**: The two containers share nothing, so a shared one replaces at least one of them.
  retcon has a recorded reason to keep its own, and odyssey's exists because a map is read far
  more often by a person than by the program.
- **Why not**: It would take a container away from a game that has a reason for it.

### Alternative 2: A Tiled reader in `api/grid`
- **Pros**: An editor nobody has to write, and cozy may choose it anyway.
- **Cons**: Nobody has chosen it yet, and retcon has rejected it. Once a layer's tile ids are
  mapped to glyphs, a Tiled tile layer is again rows plus a legend.
- **Why not**: Premature. If cozy chooses Tiled, that reader calls the function decided here.

### Alternative 3: A picture and a terrain legend, with unknown glyphs handed back — **chosen**
- **Pros**: It is the part both formats already share, after their own parsing. A game that
  hangs its own meaning on a glyph, like odyssey's `'@'`, gets the positions and sets the
  terrain itself. A game that wants an unknown glyph to be an error applies that rule on top.
- **Cons**: Each game still writes its own container and its own legend parsing.
- **Why chosen**: It shares exactly what both games have in common.

## Consequences

### Positive
- odyssey builds its grid through it, and retcon's terrain half can once its legend is parsed.
- Whichever format cozy picks, its terrain half arrives here as rows and a legend.

### Negative
- A start position is not the grid's, so every game finds its own among the unknown glyphs or
  in its own records.
- A legend is passability and cover only. Anything else a glyph means, a prop or a tile's
  appearance, is looked up again by the game, from the same picture.

### Risks
- A third terrain property, such as height or a movement cost, means widening `Terrain`. That
  touches every legend a game builds, and the escape hatch is a default for the new field.
- Ragged rows and an empty picture are still refused, because neither has a tile to hand back.
  A game wanting to pad a ragged map has to do it before the call.
