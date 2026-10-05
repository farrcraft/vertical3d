# ADR-0015: Editor: manipulators edit the object transform

**Status**: accepted
**Date**: 2026-09-02
**Documented in**: [editor/](../editor/README.md)

## Context

The editor needs translate, rotate and scale handles for the selected mesh. A mesh has a
`dag::Transform` for its placement, per [ADR-0013](0013-editor-a-mesh-is-a-dag-node.md), and
the transform pivots about the mesh's own origin. Handles must stay grabbable when geometry is in
front of them, and they share a plane with the construction grid's axis lines. The render pass
model already provides overlay passes without depth, per
[ADR-0011](0011-rendering-lines-as-a-world-space-primitive.md).

## Decision

A manipulator drag changes the object's transform and never its vertices. The handles are drawn
at the object's origin, where the transform pivots, in an overlay pass per viewport with no depth
test.

## Alternatives

### Handles move the selected vertices
- **For**: one mechanism for object and component modes; an object drag moves every vertex, a
  vertex drag moves one.
- **Against**: placement disappears into geometry, so it can no longer be saved, undone or shared
  as a placement, and a rotation must be applied to every point. Two meshes could never share
  geometry. Modelling operations rebuild the vertex arrays, so a drag in progress would hold
  indices that could become invalid.
- **Rejected because**: it merges placement into geometry, the distinction ADR-0013 makes.

### Handles drawn at the centre of the selected components
- **For**: the handles follow the face or vertices the user is working on.
- **Against**: rotation and scale pivot at the object's origin, so a handle drawn elsewhere turns
  the object about a point it is not drawn at.
- **Rejected because**: a handle must be drawn where its pivot is, or a drag does not do what
  the drawing shows.

### Handles in the scene pass, offset toward the camera to beat the grid
- **For**: one pass per viewport instead of two.
- **Against**: the offset is a constant that must work for every camera, zoom and distance, and
  the handles are still hidden by geometry in front of them.
- **Rejected because**: an overlay pass expresses the requirement directly, at the cost of one
  pass.

## Consequences

- **Gains**:
  - A mesh has one placement to save and to undo, instead of a changed vertex list.
  - What a handle shows matches what a drag does, because both use the same pivot.
  - Handles can always be grabbed, whatever is in front of them.
  - Manipulators are arithmetic over a camera and a transform, testable without a window or a
    device.
- **Costs**:
  - In a component mode, a drag still moves the whole object. Moving part of a mesh needs a
    modelling operation.
  - Every viewport has a handle pass in its frame, whether or not anything is selected.
- **Revisit when**: modelling operations arrive and component editing needs handles of its own.
