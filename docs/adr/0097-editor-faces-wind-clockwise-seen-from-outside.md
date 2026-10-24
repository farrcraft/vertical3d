# ADR-0097: Editor: faces wind clockwise seen from outside

**Status**: accepted
**Date**: 2026-10-20
**Documented in**: [editor/Scene.md](../editor/Scene.md#meshes)

## Context

A face stores a normal, and a face's half edges run round it in an order. Moving a vertex makes
the stored normal stale, so it is recomputed from the points, and a normal found from points
points to whichever side the winding says. The winding must therefore say which side is outside,
or a recomputed normal turns a face inside out. The editor's primitives wind every face the same
way, and project files and the RIB export carry that order as it is.

## Decision

A face's loop runs clockwise when seen from outside, and its normal points outward. A normal
computed from a face's points follows that rule.

## Alternatives

### Counter-clockwise seen from outside
- **For**: it is the glTF and OpenGL convention, and the one most geometry code assumes.
- **Against**: every primitive would be rewound, and the RIB export would have to reverse its
  order or set an orientation. Projects saved with the present winding would have their normals
  turned over at their first edit.
- **Rejected because**: it changes the files and output that exist for no gain inside the editor.

### No convention: a recomputed normal keeps the side of the old one
- **For**: nothing changes in the primitives or in any file.
- **Against**: a move that really does fold a face over is hidden, and a face with no normal yet
  has no side to keep. The winding would carry no meaning at all.
- **Rejected because**: topology operations need a winding they can rely on.

## Consequences

- **Gains**:
  - A normal can always be recomputed from the points, so no operation has to carry one.
  - The winding matches RenderMan's default orientation, so the RIB export needs no orientation
    of its own.
- **Costs**:
  - An importer of glTF or of any counter-clockwise format has to reverse each face's loop.
  - Code written against the common convention gets the wrong side, and has to be checked
    against this one.
- **Revisit when**: the editor draws shaded faces with back-face culling, or imports a
  counter-clockwise format as its main source of geometry.
