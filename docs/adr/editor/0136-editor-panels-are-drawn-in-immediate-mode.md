# ADR-0136: Editor: panels are drawn in immediate mode

**Status**: accepted
**Date**: 2026-10-30
**Tags**: editor
**Documented in**: [editor/Scene.md](../../editor/Scene.md#panels)

## Context

An area can hold editors other than a viewport, and the outliner and the properties editor are
the first two. Both show the scene as it is at that moment: a row for each item, the selection,
and numbers that change while a manipulator drags. The menus, headers and tool shelf are retained
documents, which show whatever they were last told and are rebuilt when a summary of what they
show changes. A panel that lists every item and shows live numbers would need that summary to
cover nearly the whole scene, and would rebuild on every frame of a drag.

## Decision

An editor that is not a viewport is drawn with the immediate layer, from the scene, every frame,
inside its area below the header. The header, the tabs and the editor's menus stay retained. The
immediate layer gains what such panels need: a region pinned to a rectangle with no title bar, a
tree row that folds, and a number field that reports when an edit ends.

## Alternatives

### Retained documents, rebuilt on a signature
- **For**: every panel is styled and laid out from the same `ui` theme class as the chrome, and
  can be looked up by name.
- **Against**: the retained ui has no scrolling list of components and no wheel, so the editor
  would place and scroll the rows itself. Every value shown would need a rebuild or a write each
  frame to stay current.
- **Rejected because**: the panels show the scene's live state, and the immediate layer exists
  to show live state.

## Consequences

- **Gains**:
  - A panel cannot show a stale value, and has no summary of the scene to keep in step.
  - Scrolling, the wheel, clipping and the text field come with the layer.
  - The layer's additions serve any later panel, in the editor or a game's tools.
- **Costs**:
  - Panels are styled from the `tools` class and the chrome from `ui`. Both derive from one
    palette, but a size set in one class does not reach the other.
  - Hover runs a frame behind the cursor in a panel, as it does everywhere in the layer.
  - A panel's widgets cannot be named in a document or found by a test through the ui tree.
- **Revisit when**: a panel has to be authored or rearranged by a user, as a document would let
  them.
