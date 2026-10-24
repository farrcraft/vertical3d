# ADR-0098: Editor: topology changes undone from snapshots

**Status**: accepted
**Date**: 2026-10-20
**Documented in**: [editor/CommandsAndUndo.md](../editor/CommandsAndUndo.md)

## Context

A split, an extrude, an inset or a merge changes which vertices, half edges and faces a mesh
holds, and a merge renumbers them. Later commands in the history hold indices into the mesh, so
an undo has to put back the old numbering exactly, not only the old shape. A `BRep` cannot be
copied, because each one carries a unique node id. The arrays also carry the component selection
flags, and selection is not part of the history.

## Decision

A topology command holds a snapshot of the mesh's vertex, half edge and face arrays from before
the operation and from after it, and undo and redo put one or the other back whole. Undo and redo
then clear the component selection of that mesh, and leave the object selected.

## Alternatives

### A record of the elements the operation touched
- **For**: a command holds only the arrays' old lengths and the old and new value of each element
  written, which is far smaller than the mesh.
- **Against**: every operation has to log every write it makes, and one missed write corrupts an
  undo without any sign until a later command acts on the wrong part.
- **Rejected because**: the snapshot is exact by construction, and memory is not yet a constraint.

### An inverse operation
- **For**: the smallest record, and undo reads as the opposite of the operation.
- **Against**: an inverse built from the same append and rewire steps leaves the parts in a new
  order, and a merge's inverse would have to invent the numbering it removed.
- **Rejected because**: it cannot meet the index for index requirement.

### Keeping the selection flags whose index survives
- **For**: more of the user's selection outlasts an undo.
- **Against**: after a merge renumbers the mesh, a kept flag can name a different part than the one
  the user chose.
- **Rejected because**: a selection that silently moves is worse than one that is cleared.

## Consequences

- **Gains**:
  - Undo and redo restore a mesh exactly, whatever the operation did.
  - An operation is written once, forwards, with no undo logic of its own.
- **Costs**:
  - Each topology command holds two copies of its mesh's arrays, so the history's memory grows
    with the size of the meshes edited.
  - An undo or redo of a topology change loses the component selection of that mesh.
  - `BRep` exposes its arrays as a snapshot, which other code could misuse to bypass its rules.
- **Revisit when**: meshes grow large enough that a full history of snapshots costs real memory.
