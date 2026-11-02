# ADR-0148: UI: text is paragraphs from one text renderer

**Status**: accepted
**Date**: 2026-11-01
**Documented in**: [api/ui/README.md](../api/ui/README.md#text-is-supplied-by-the-app)

## Context

The retained components and the immediate layer measure and draw text through a pair of
callbacks that take a string, so that neither names a font type and both test without a device.
A text field finds its caret by measuring the string before it, which is wrong once glyphs are
kerned, joined or reordered. A caret, a selection, a hit test and an input method's composition
each need the laid out text, not a width. The text library and its glyph store need no GPU,
and the device side of text already sits behind callbacks of its own.

## Decision

The ui's components, its immediate layer and its text fields hold font paragraphs, laid out and
drawn by one text renderer that the screen owns and passes to them. They measure, wrap, place a
caret, hit test and draw through the paragraph. The string callbacks are removed.

## Alternatives

### An abstract text interface
- **For**: the ui names no concrete text type, and a test can supply a fake with fixed widths,
  as Godot's text server allows.
- **Against**: every query the text field makes, from a caret to a selection to a composition,
  becomes a method on the interface, which repeats the paragraph's own. A fake cannot shape, so
  tests through kerned and right to left text need the real one anyway.
- **Rejected because**: there is one text engine, and nothing to swap behind the interface.

### Callbacks that return paragraphs
- **For**: the smallest change to how the ui is built today.
- **Against**: each new query is another callback, and the ui still holds no text of its own
  between frames.
- **Rejected because**: it keeps the indirection while giving up what a held paragraph offers.

## Consequences

- **Gains**:
  - A caret, a selection and a hit are exact through kerned, joined and reordered text.
  - Wrapping follows Unicode's line breaks and the paragraph's own layout.
  - Every part of the ui shares one shaping cache and one glyph store.
- **Costs**:
  - The ui's tests load a real face, and their expected widths come from it.
  - A test can no longer record the strings drawn through a callback, and reads them from what
    the components hold instead.
- **Revisit when**: a second text engine is wanted beside the font library.
