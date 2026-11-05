# ADR-0141: Text: OpenType shaping with HarfBuzz

**Status**: accepted
**Date**: 2026-11-01
**Tags**: text
**Documented in**: [api/Fonts.md](../../api/Fonts.md#shaping)

## Context

The fonts the api ships keep their kerning only in OpenType's GPOS table, and their tabular
figures, case forms and alternates only in GSUB. Placing a combining mark, joining Arabic, and
reordering an Indic syllable are done by the same tables. Laying text out by adding each code
point's advance reads none of them. Games load fonts the api did not choose, from mods and user
files, so the reader has to be safe with a malformed font. The api already holds each face open
in FreeType.

## Decision

Text is shaped by HarfBuzz, through its FreeType integration, over the faces the api holds open.
A run of one face, script and direction becomes glyphs with indices, advances, offsets and the
clusters they came from. Segmentation and bidirectional reordering are separate decisions.

## Alternatives

### kb_text_shape
- **For**: a single header that shapes and also segments text by direction, script, line, word
  and grapheme, which would cover more than shaping.
- **Against**: no published conformance results, coverage of variable fonts and Apple's AAT
  tables is not stated, and it reads font files itself rather than sharing FreeType's faces. Its
  author states it is not safe with untrusted fonts.
- **Rejected because**: a game's fonts are not all trusted, and HarfBuzz is the reference other
  shapers are measured against.

### FreeType's GPOS kerning
- **For**: no new dependency.
- **Against**: off by default, basic pair kerning only, and no substitutions, marks or complex
  scripts.
- **Rejected because**: it gives kerning and nothing else a modern font defines.

### The platform's shaper, DirectWrite or Core Text
- **For**: shapes as the operating system's own text does.
- **Against**: a different shaper on each platform, and none for the libraries built on Linux.
- **Rejected because**: the api lays text out the same way on every platform.

## Consequences

- **Gains**:
  - Kerning, marks, ligatures and contextual forms are applied as each font defines them.
  - An OpenType feature such as tabular figures is a choice in a style, with no font edited.
  - Complex scripts, variable fonts and AAT fonts are shaped by the same code.
- **Costs**:
  - A dependency with a large API, linked into every app that draws text.
  - Segmentation and bidi still need libraries of their own.
  - A layout can no longer find a caret by measuring a prefix, because a glyph's advance depends
    on its neighbours. Carets and hit tests go through the shaped clusters.
- **Revisit when**: a shaper with HarfBuzz's coverage and robustness also segments text, or the
  api stops drawing text from fonts it did not choose.
