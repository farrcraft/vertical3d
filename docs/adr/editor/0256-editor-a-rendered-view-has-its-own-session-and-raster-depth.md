# ADR-0256: Editor: a rendered view has its own session and raster depth

**Status**: accepted
**Date**: 2026-11-20
**Tags**: editor, offline
**Documented in**: [editor/Rendering.md](../../editor/Rendering.md)

## Context

A 3D view that shows moya's picture in place of its own shading must still show the grid, the
manipulators and the selection outlines. A manipulator behind an object should be hidden there as
it is in a shaded view. moya's picture arrives some time after an edit, coarse at first, and its
depth is blurred by the pixel filter. The render view already has a camera of its own, and the user
may look through a different one in each view. Blender gives each viewport in rendered shading its
own render engine, and tests the overlays against a depth the viewport rasterises itself. A Hydra
viewport composites the overlays against the depth the render delegate gives.

## Decision

Each 3D view in rendered shading has a session of its own, at the view's size and from the view's
camera, and a view that is not shown pauses its session. The view draws the scene's depth as it
rasterises it now, and the overlays are tested against that depth and drawn over moya's picture.

## Alternatives

### A session for each view, with overlays tested against moya's depth
- **For**: The overlays meet exactly what is rendered, including displacement and hair the
  rasteriser does not draw.
- **Against**: The depth arrives with the picture, so after each edit the overlays are wrong until
  the first full-resolution pass, and coarse pictures give no depth worth testing. The pixel filter
  blends depths at every edge.
- **Rejected because**: The overlays are what the user manipulates, and they must be right on the
  frame of the edit.

### One session, shared by the render view and the active view
- **For**: One render runs, and every thread goes to it.
- **Against**: The render view loses its own camera, and two views can never show two cameras'
  renders. The render view's picture changes when the user moves to another view.
- **Rejected because**: A user who frames a shot in the render view and works in another view needs
  both.

### Overlays drawn over the picture with no depth
- **For**: Nothing is rasterised beside the render, and the overlays are always visible.
- **Against**: A manipulator or an outline behind an object shows through it, so a rendered view
  reads differently from a shaded one.
- **Rejected because**: The rasterised depth is cheap and the view already draws it.

## Consequences

- **Gains**:
  - The overlays are right from the frame of an edit, before moya has a picture.
  - Each view and the render view keep their own camera, so a view's orbit is an edit of that
    view's session alone.
  - A shaded view and a rendered one hide the same overlays.
- **Costs**:
  - Several sessions share the machine's threads, and each renders more slowly than one would.
  - Each session holds its own film and its own copy of the structures a camera does not share.
  - Where moya renders what the rasteriser does not, such as displacement or a volume, the overlays
    meet the rasterised surface instead.
- **Revisit when**: moya gives a depth at full resolution within a frame of an edit.
