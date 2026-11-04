# ADR-0169: Editor: navigating a camera view frees it unless locked

**Status**: accepted
**Date**: 2026-11-04
**Documented in**: [docs/editor/Scene.md](../editor/Scene.md)

## Context

A viewport can look through a scene camera, to judge a shot as the render frames it. Orbiting,
panning or zooming such a view has to do one of two things: move the view away from the camera,
or move the camera with the view. A camera is part of the scene, so moving it changes the
document and the render, and it has to be undoable like any other edit. Moving a view changes
nothing saved. Blender frees the view unless Lock Camera to View is on, Maya moves the camera,
and Houdini offers a lock on each view.

## Decision

Navigating a view that looks through a scene camera frees it: the view keeps showing what it
showed, as a free perspective view, and the camera stays where it is. A lock in the camera menu
makes navigation move the camera instead, and each drag is one change in the history. The lock
belongs to the view and is saved in its layout.

## Alternatives

### Navigation always moves the camera
- **For**: one behaviour, and framing a shot by orbiting is direct.
- **Against**: every glance around a camera's view edits the scene, fills the history and dirties
  the document, and a keyed camera is moved off its keys.
- **Rejected because**: looking is far more common than framing, and an accidental edit of a
  camera is easy to miss.

### Navigation always frees the view, with no lock
- **For**: a view can never change the scene.
- **Against**: a camera can only be framed through the Properties numbers or the transform tools,
  which work in the world's axes rather than the camera's own.
- **Rejected because**: framing a shot from the camera's own view is how every tool it is
  compared with lets a camera be aimed.

### The lock on by default
- **For**: a camera view behaves as Maya's does from the start.
- **Against**: the first orbit of a new camera view edits the scene.
- **Rejected because**: the safer default costs one click to change, and the other costs an undo
  every time it surprises.

## Consequences

- **Gains**: a camera view can be explored freely, and a camera can still be aimed from its own
  view, with undo, when asked for.
- **Costs**: two behaviours to learn, and a lock to notice. A view freed by accident has to be set
  back to its camera from the menu.
- **Revisit when**: the editor gains a way to frame a camera that does not use the view, or users
  aim cameras through their views more than they look around them.
