# ADR-0127: Editor: tiled areas that hold editors as tabs

**Status**: amended
**Date**: 2026-10-30
**Tags**: editor
**Amended by**: ADR-0154
**Documented in**: [editor/Scene.md](../../editor/Scene.md#viewports)

## Context

The editor divides its window between viewports only, through a tree of splits read from
`data/layout.json`. Open work adds an outliner, a properties editor, a timeline, a graph editor, a
console and a render view, and each needs a place on screen. The viewports need most of the
window, and a layout the user rearranges has to stay predictable. Blender tiles the window into
areas that each show one editor. Unreal docks panels as tabs that can float, and Houdini tiles
panes that each hold tabs.

## Decision

The editor window is a tree of splits whose leaves are areas, and an area holds one or more
editors and shows one at a time. An area with one editor shows that editor's header and no tab
strip, and an area with two or more shows a strip of tabs above it. Areas split, join, swap,
resize and maximise, a tab dropped on an area's edge splits that area, and no area overlaps
another or floats.

## Alternatives

### Areas that show one editor each
- **For**: every area gives all of its room to its editor, and the model has no tabs at all.
- **Against**: changing what an area shows goes through a menu, and editors used together, such
  as a timeline, a graph editor and a console, cannot share one place a click apart.
- **Rejected because**: animation and scripting work keeps several editors in one place, and
  tabs on demand cost nothing where they are not used.

### Docks with tabs that can float
- **For**: every dock lists its editors by name, and a floating dock can move to a second
  monitor.
- **Against**: every dock spends a tab strip's height, including the one that holds the views. A
  free-form layout drifts into overlapping and lost panels, and needs a reset.
- **Rejected because**: the views need the room, and a tiling is a layout the user can always
  read at a glance.

### Fixed panels around the viewports
- **For**: no layout model beyond today's split tree.
- **Against**: each new editor changes the layout code, and the user cannot move anything.
- **Rejected because**: the editor is gaining several editors, and their placement belongs to
  the user.

## Consequences

- **Gains**:
  - Every planned editor has a home without a change to the layout code.
  - A viewport keeps its whole area, with its header drawn over the scene.
  - The layout stays a tree of splits, which is data the editor already reads.
- **Costs**:
  - There are no floating windows, so a second monitor needs a decision of its own.
  - Tabs, dragging a tab and splitting by a drop are new widgets in `api/ui`.
  - Every editor has to draw at any size an area can take.
- **Revisit when**: the editor needs a second window, or an editor that only works floating.
