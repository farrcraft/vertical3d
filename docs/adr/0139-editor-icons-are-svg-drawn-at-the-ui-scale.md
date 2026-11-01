# ADR-0139: Editor: icons are SVG, drawn at the ui's scale

**Status**: accepted
**Date**: 2026-10-31
**Documented in**: [docs/editor/README.md](../editor/README.md), `vertical3d/src/control/IconSheet.h`

## Context

The editor draws its tools, editor kinds, item kinds, mouse buttons and marks as icons, and the
set is growing. The ui is redrawn at the monitor's scale times a user preference, so an icon is
needed at sizes that are not known in advance. An icon takes its colour from where it is drawn:
the text colour on a toolbar, an item kind's colour in the outliner. Established editors keep
their icons as vector files and rasterise them at the size the ui needs.

## Decision

Each icon is an SVG file drawn in white. The editor rasterises every icon at the ui's icon size
in device pixels into one image, and rasterises them again when the scale changes. Whatever
draws an icon tints it.

## Alternatives

### SVG files rasterised to a sheet by a script
- **For**: no rasteriser at runtime, and one texture made ahead of time.
- **Against**: the sheet is drawn at one size, so an icon is filtered down at any other scale and
  softens between whole scales. The script needs an SVG rasteriser of its own, since the tree's
  scripts use only Node's built-in modules.
- **Rejected because**: the ui is drawn at scales not known when the sheet is made, and an icon
  drawn at its size is sharper.

### An icon font drawn through the text renderer
- **For**: icons scale and colour exactly as text does, through the distance fields already used
  for glyphs.
- **Against**: every icon added means building a font, with a toolchain the tree does not have,
  and the editor's own icons, such as extrude and inset, exist in no published font.
- **Rejected because**: adding an icon should be adding a file.

### A hand-drawn image of cells
- **For**: nothing more to build or depend on.
- **Against**: there is no source to edit, an icon is drawn by hand at one size, and it softens at
  every other scale.
- **Rejected because**: it does not scale, and a set this size needs a source.

## Consequences

- **Gains**: icons are sharp at every scale, adding one is adding a file, and each takes the
  colour of where it is drawn.
- **Costs**: `v3dlib_image` depends on NanoSVG, which draws only a subset of SVG. Every icon is
  rasterised again when the scale changes.
- **Revisit when**: an icon needs SVG that NanoSVG does not draw, or rasterising the set at a scale
  change is slow enough to notice.
