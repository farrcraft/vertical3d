# ADR-0096: Editor: handles pivot at the mean of the selection

**Status**: accepted
**Date**: 2026-10-18
**Amends**: [ADR-0015](0015-editor-manipulators-edit-the-object-transform.md)
**Documented in**: [editor/PickingAndManipulators.md](../editor/PickingAndManipulators.md#manipulators)

## Context

The manipulators draw their handles at the selected object's origin, and turn and scale about
it. Two kinds of selection have no single origin: several objects, and vertices, edges or faces
inside one or more objects. A handle must be drawn where its pivot is, or a drag does not do
what the drawing shows. The pivot also decides where a rotation or a scale leaves each object
or vertex, so a user learns it once and expects it everywhere.

## Decision

A set of objects pivots at the mean of their origins. A component selection pivots at the mean
of the distinct vertices it moves, in world space, and may span several objects; each vertex is
moved in world space and written back in its own object's space. A selection of one object
pivots at its origin, as before.

## Alternatives

### The centre of the selection's bounds
- **For**: it does not depend on how densely a mesh is divided, so a face with many vertices on
  one side does not pull the pivot towards them.
- **Against**: for one object it is not the origin, so turning a single object would no longer
  turn it about the point its transform pivots on. Objects and components would also follow two
  different rules if components used it and objects did not.
- **Rejected because**: it breaks the single-object case that ADR-0015 already fixes.

### The origin of the most recently selected object
- **For**: one obvious point that the user chose, and the single-object case is unchanged.
- **Against**: the selection order becomes state the user cannot see, and a shift-click to take
  an object away moves the pivot. It has no answer for components.
- **Rejected because**: it needs a second rule for components and hidden state for objects.

### Components keep the owning object's origin
- **For**: ADR-0015 holds unchanged, and the handles never move.
- **Against**: the handles sit away from the face being worked on, and rotating or scaling a
  face turns it about a point outside it.
- **Rejected because**: it makes component rotation and scale useless for modelling.

### Components only within one object
- **For**: a component command holds one mesh, and the move stays in that mesh's own space.
- **Against**: two objects cannot be welded or aligned by moving their parts together.
- **Rejected because**: the cost of a command holding several meshes is small, and the
  restriction would be visible to the user.

## Consequences

- **Gains**:
  - One rule, the mean, for objects and components, which reduces to the origin for one object.
  - A rotation or scale of a face turns about the face.
  - Components in several objects move together.
- **Costs**:
  - A rotate or scale of several objects changes each object's translation, not only its
    rotation or scale, so the transform command records every placement in full.
  - The pivot moves when a densely divided part is selected beside a sparse one.
  - A move of components in several objects converts each vertex between spaces twice, once in
    and once out, and so a command holds positions per mesh.
- **Revisit when**: users need to place a pivot by hand, or choose between pivot rules.
