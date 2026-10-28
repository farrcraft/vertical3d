# ADR-0126: Editor: the handle mask selects Bezier curve handles

**Status**: accepted
**Date**: 2026-10-27
**Amends**: ADR-0121
**Documented in**: [editor/PickingAndManipulators.md](../editor/PickingAndManipulators.md#component-selection)

## Context

The select menu offers a Handle mask, and nothing in the editor is a handle that can be
selected. Curve items hold Bezier points whose two handles each keep a type, and other curves
whose control points lie off the curve, yet a curve can only be selected and moved whole. Maya
gives the word two meanings, a selection handle that picks a node and an IK handle that drives a
chain of joints. Blender and Houdini select a Bezier curve's handles in its edit mode, beside its
points. The editor has no joints and no plan for them, and it draws a glyph for every kind of item
that has no geometry to click.

## Decision

A curve item's control points and its Bezier handles are its parts, selected and held as a mesh
item's are. The Vertex mask selects control points of every curve type, and the Handle mask
selects Bezier handles alone. Moving a handle retypes it as dragging one does in Blender, and
moving a point carries its handles with it.

## Alternatives

### A selection handle on each item
- **For**: Maya has one, and it would pick an item from a marker anywhere in its space.
- **Against**: every kind of item already has geometry or a glyph to click, and a group is
  picked by its box.
- **Rejected because**: it adds nothing the glyphs do not already give.

### An IK handle
- **For**: it is the other meaning Maya gives the mask.
- **Against**: it needs joints, a skeleton and a solver in the scene, and none of them is planned.
- **Rejected because**: the mask would wait on a feature nobody has asked for.

### No Handle mask, with handles under the Vertex mask
- **For**: one mask fewer, as Blender's curve edit mode picks points and handles together.
- **Against**: the handles of many points cannot be selected without their points, so they cannot
  be retyped, aligned or scaled as a set.
- **Rejected because**: a separate selection is what both Blender's handle overlay and Maya's
  mask exist to give.

## Consequences

- **Gains**:
  - A curve can be edited after it is made, point by point, in every type.
  - The handle types the curve library keeps become something a user changes.
  - A component selection, a manipulator drag and its undo work the same on meshes and curves.
- **Costs**:
  - A part of a curve is named by its position counted across every curve of the item, so
    adding or removing a curve renumbers the parts after it and clears the item's selection.
  - The word handle names both a manipulator's handles and a curve's, and `view::show::handle`
    shows both.
  - Edge and Face select nothing on a curve.
- **Revisit when**: the editor gains joints, which would bring an IK handle and a second kind
  of handle to select.
