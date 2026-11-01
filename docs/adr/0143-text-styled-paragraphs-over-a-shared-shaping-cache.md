# ADR-0143: Text: styled paragraphs over a shared shaping cache

**Status**: accepted
**Date**: 2026-11-01
**Documented in**: [api/Fonts.md](../api/Fonts.md#paragraphs)

## Context

Text has to be shaped, divided into lines, placed, and asked where a caret goes and what a point
is over. Shaping is the costly part, and it does not depend on the width the text is laid out in.
The ui has a retained component tree, whose widgets can hold an object across frames, and an
immediate mode that builds its widgets again every frame and holds nothing. Editors, consoles and
game dialogue colour or embolden words inside one block of text.

## Decision

The ui and the games hold a paragraph: a text with styles over its ranges, laid out at a width,
which keeps its shaping when its width or its colours change. Its shaping comes from one cache
that every paragraph shares. The cache keeps the most recently used texts by their string and the
styles that decide their glyphs, so text built again each frame is found and not shaped.

## Alternatives

### An immutable laid out text
- **For**: the simplest contract, with nothing to invalidate. A change builds a new one.
- **Against**: a change of width shapes everything again. Immediate mode still needs a cache to
  find last frame's text.
- **Rejected because**: it repeats the costly work on every change that does not need it.

### A paragraph with no shared cache
- **For**: each widget owns its text outright, as DirectWrite's text layouts and Godot's text
  paragraphs do.
- **Against**: immediate mode holds no widget, so it needs its own map from strings to
  paragraphs, and two widgets showing one label shape it twice.
- **Rejected because**: the cache serves both kinds of ui in one place, as Skia's paragraph cache
  and Slate's shaped text cache do.

### A cache with no object
- **For**: callers pass strings and get layouts, which suits immediate mode.
- **Against**: a retained widget looks its text up every frame, and a key holding the width
  misses on each resize.
- **Rejected because**: a widget that holds its text should not pay to find it again.

### One style for each text
- **For**: a smaller object, with no ranges to split and join.
- **Against**: a coloured or bold word becomes several objects placed by hand, which loses
  kerning and line breaking across their edges.
- **Rejected because**: DirectWrite, Skia, Core Text, Slate and Godot all style ranges of one
  text, and every rich text use needs it.

## Consequences

- **Gains**:
  - A resize or a recolour costs no shaping.
  - Retained and immediate ui share one path, and text shown twice is shaped once.
  - Carets, hit tests and selections are answered from the same layout that is drawn.
  - The paragraph needs no GPU, so it lives in the font library and is tested without a device.
- **Costs**:
  - A paragraph has to track which changes undo which stage, and a mistake there shows stale
    text.
  - The cache holds memory for texts no longer shown until newer ones push them out.
  - Drawing is separate from the paragraph, so a renderer has to walk its lines and runs.
  - A line break does not shape the text again, so a ligature or a joining form across it keeps
    its shape.
- **Revisit when**: text needs a layout across several paragraphs or shapes, such as text flowed
  around an image or through columns.
