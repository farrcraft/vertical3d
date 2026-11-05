# ADR-0138: Editor: ui text is 12 pixels

**Status**: accepted
**Date**: 2026-10-31
**Tags**: editor
**Documented in**: `vertical3d/src/Renderer.h`

## Context

The editor's rows, headers, bars, padding and icons are all multiples of one text size. The
editor holds dense panels: a properties editor, an outliner, and a timeline and graph editor to
come. Established editors set their ui text between 11 and 13 pixels at a display scale of one,
and let the user enlarge it with a scale preference. The editor already multiplies its sizes by
the monitor's scale and by a user preference. Its text is drawn from distance-field glyphs,
which can soften at small sizes.

## Decision

The editor's ui text is 12 pixels at a scale of one. Rows, headers, bars and icons are set from
it to the design's measurements: 24 pixel rows, 28 pixel headers and 18 pixel icons on 30 pixel
buttons. A user who wants larger text raises the scale preference.

## Alternatives

### 15 pixels
- **For**: the easiest to read at a scale of one, and what the editor drew before.
- **Against**: every row, header and bar is about a quarter taller, so a panel holds less. It is
  larger than every established editor's default, and every measurement of the design has to be
  rescaled.
- **Rejected because**: density matters most in the panels the editor is growing, and the scale
  preference already serves anyone who wants larger text.

### 13 pixels
- **For**: a middle ground that matches some established tools, and a margin against soft
  glyphs.
- **Against**: the design's measurements do not divide evenly by it, and it gives up density
  without a reason the glyphs supply. At 12 pixels they read sharply.
- **Rejected because**: 12 pixels reads well and matches the design exactly.

## Consequences

- **Gains**: the editor matches its design's density, and the panels to come fit the room drawn
  for them.
- **Costs**: text is smaller at a scale of one, so a user on a low-density display may need to
  raise the scale preference.
- **Revisit when**: the editor gains a ui where small text loses meaning, such as long prose in a
  panel, or users report the default too small at a scale of one.
