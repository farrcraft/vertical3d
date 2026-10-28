# ADR-0121: Editor: component selection belongs to the item

**Status**: amended
**Date**: 2026-10-26
**Amended by**: ADR-0126
**Documented in**: [editor/PickingAndManipulators.md](../editor/PickingAndManipulators.md)

## Context

The editor selects vertices, edges and faces of a mesh item, and its modelling operations and
manipulators act on what is selected. Two mesh items can share one `brep::BRep`, and instancing
and glTF imports that share buffers make that common. `api/brep` is a geometry library that the
editor uses now and that imports and other tools will use later. An operation that changes a
mesh's topology can renumber its parts. Named selection sets and soft selection weights are
features the editor is likely to grow.

## Decision

The scene holds which parts of each mesh item are selected, as indices into the item's
geometry, keyed by the item. `api/brep` holds no selection. An operation that renumbers a mesh
says what it made, and the editor replaces the item's selection from that.

## Alternatives

### A flag on each vertex, half edge and face
- **For**: a topology change carries the flags with the parts by itself, as Blender's BMesh
  does, and a snapshot restores them.
- **Against**: two items sharing one mesh select its parts together, and every user of
  `api/brep` carries an editor's state.
- **Rejected because**: selection belongs to what the user picked, which is an item, not to
  the geometry it shows.

### A per-part attribute layer on the mesh, of which selection is one
- **For**: the same layers would hold UVs, creases and weights, as Blender's `Mesh` attributes
  do, and they carry over a topology change.
- **Against**: the layer is still shared by every item that shows the mesh, and still lives in
  the geometry library.
- **Rejected because**: it fixes how the state is stored but not whose state it is.

## Consequences

- **Gains**:
  - Two items sharing a mesh select its parts independently.
  - `api/brep` is geometry alone.
  - A named set or a weighted selection is another entry of the same kind.
- **Costs**:
  - Every operation that renumbers a mesh has to report the mapping or what it made, and a
    selection that is not replaced afterwards can name the wrong part.
  - An operation on a mesh two items share runs once, and clears the component selection of the
    items it did not run for.
- **Revisit when**: per-part data other than selection has to live on the mesh, which would
  bring a general attribute layer.
