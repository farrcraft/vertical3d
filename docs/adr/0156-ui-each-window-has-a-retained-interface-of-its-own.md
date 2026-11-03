# ADR-0156: UI: each window has a retained interface of its own

**Status**: accepted
**Date**: 2026-11-02
**Documented in**: [docs/api/ui/README.md](../api/ui/README.md), [docs/editor/Scene.md](../editor/Scene.md)

## Context

The editor's areas each have a header built in the retained ui, and an area in a second window
needs one too. An interface keeps one focus, one hover and one tooltip, and drawing a container
records where its widgets are for the cursor to test. A window's menus and tooltips belong in that
window, and the platform gives each window its own focus. Qt, GTK and Blender give each top-level
window a tree of its own, while Dear ImGui keeps one context and assigns its windows to platform
windows.

## Decision

Each window an app draws to has a retained interface of its own, with a screen, a cursor and the
containers that window shows. The interfaces share their themes and send their commands to the
one router. In the editor, each window has its own areas, headers and pointer.

## Alternatives

### One interface whose containers each belong to a window
- **For**: one tree, so a container moves between windows by changing its window.
- **Against**: focus, hover, tooltips, drawing and hit-testing each need a version for every
  window inside the interface, and every caller has to say which window it means.
- **Rejected because**: it changes the core of the ui to serve what separate interfaces already do.

### The second window's header drawn by the immediate layer
- **For**: nothing changes in the retained ui.
- **Against**: an area in the second window would look and behave unlike every other area, and
  its menus would be a second implementation of the header's.
- **Rejected because**: the header belongs to the area, wherever the area is.

## Consequences

- **Gains**: each window's menus, tooltips and focus stay in that window, and the retained ui needs
  no notion of windows. A window's areas behave as the main window's do.
- **Costs**: an app repeats its per-window setup for each window: an interface, a screen, a
  cursor, and in the editor the areas and their pointer. Area and editor ids have to be unique
  across every window's layout, since commands carry them.
- **Revisit when**: a container has to move between windows while it is open, such as a dragged
  tab carried into another window.
