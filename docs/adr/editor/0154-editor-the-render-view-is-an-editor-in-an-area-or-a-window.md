# ADR-0154: Editor: the render view is an editor, in an area or a window

**Status**: accepted
**Date**: 2026-11-02
**Tags**: editor
**Amends**: ADR-0127
**Documented in**: [docs/editor/Rendering.md](../../editor/Rendering.md)

## Context

A render is watched while the scene is edited, often beside the view it comes from, and on a
second monitor where there is one. A Rendering workspace needs the render view and its settings in
areas of their own. The render view and the render settings are floating panels over the editor's
areas now, so neither can be part of a workspace. Houdini and Maya show a render in a pane that can
be torn off into a window. 3ds Max and Cinema 4D show it in a floating window that can be docked,
and Blender lets the user choose.

## Decision

The render view and the render settings are editors an area holds. Render shows the render view
in an area that holds one, and opens a window of its own holding an area with it when none does.
That window is a second window of the platform's, and its area behaves as any other.

## Alternatives

### An editor in an area only
- **For**: one window, and the render takes its place in a workspace.
- **Against**: a render can never leave the main window for a second monitor, and Render has to
  rearrange the user's areas when none holds the view.
- **Rejected because**: watching a render on its own monitor is ordinary look development.

### A floating panel only
- **For**: what the editor has now, and Render needs no area.
- **Against**: it covers the view being edited, and no workspace can hold it.
- **Rejected because**: the Rendering workspace needs the render view in an area.

## Consequences

- **Gains**: a render can sit beside the scene, in a workspace, or on another monitor, and it is
  the same editor in each place. Other editors can later move to a window by the same means.
- **Costs**: the editor draws to two windows, each with a swapchain, and an area tree is kept for
  each. A workspace has to say which windows it has. The floating render panels go.
- **Revisit when**: renders have to be compared against earlier ones. That needs a viewer of its
  own beside the render view, as Houdini keeps MPlay.
