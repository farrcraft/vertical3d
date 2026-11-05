# ADR-0151: UI: Noto faces for each script, shipped in the tree

**Status**: accepted
**Date**: 2026-11-02
**Tags**: ui
**Amends**: [ADR-0129](0129-ui-inter-in-two-weights-noto-sans-as-fallback.md)
**Documented in**: [api/ui/README.md](../../api/ui/README.md#text-is-supplied-by-the-app)

## Context

Text is shaped, reordered and segmented for every script, but Inter and Noto Sans draw only
Latin, Greek and Cyrillic, so Arabic, Hebrew, Indic, Thai and CJK text draws as missing glyphs.
Apps and their tests have to show the same glyphs on every machine, and CI has no fonts of its
own. Han characters shared by Chinese, Japanese and Korean are drawn in a different form in each
region. A font committed to the repository stays in its history for good, and a CJK face is
several megabytes.

## Decision

The shared data carries a Noto face for each of Arabic, Hebrew, Devanagari and Thai, and
Noto Sans CJK in the build holding every region's forms. The ui's text renderer tries them after
Noto Sans and before the emoji face. A text's language, from its style or the app's locale, picks
a region's forms through OpenType's language systems.

## Alternatives

### The system's fonts, found by the platform's font fallback
- **For**: nothing added to the tree, every script the machine supports, and the forms its
  users expect. Browsers and Godot do this.
- **Against**: each platform has its own fallback API, the glyphs differ from machine to
  machine, and a CI runner or a fresh container has few or no fonts.
- **Rejected because**: tests and screenshots have to draw the same glyphs everywhere, and the
  tree would need a fallback search per platform.

### No script faces; an app supplies its own
- **For**: the tree stays small, and only an app that shows a script pays for it. Unity's text
  works this way.
- **Against**: every app and every test that shows other scripts finds and licenses faces
  itself, and the shaping, bidi and line breaking already built would have nothing to draw in.
- **Rejected because**: the stack exists to draw every script, and a face per script is the
  smallest thing that lets it.

### One region's CJK subset
- **For**: a half or a quarter of the size, as Noto Sans SC or JP.
- **Against**: text in the other languages draws in the wrong region's forms, or not at all for
  Hangul in the Japanese subset.
- **Rejected because**: a language picking the right forms from one face is worth the size.

## Consequences

- **Gains**:
  - Text in any of these scripts draws the same on every machine and in CI, with no setup.
  - Japanese, Chinese and Korean text each take their own forms from one face.
- **Costs**:
  - The CJK face is the largest file in the shared data, and every clone carries it.
  - Each script face is another OFL licence file beside it.
  - Scripts beyond these, such as Bengali, Tamil or Ethiopic, still draw as missing glyphs until
    a face is added.
- **Revisit when**: an app needs a script these faces lack, or the repository's size matters
  more than drawing CJK with no setup.
