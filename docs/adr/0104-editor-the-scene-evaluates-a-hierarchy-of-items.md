# ADR-0104: Editor: the scene evaluates a hierarchy of items

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0099
**Documented in**: [editor/Scene.md](../editor/Scene.md#hierarchy)

## Context

A group places its children, so an item's place in the world depends on its ancestors. Keys
sampled at a time, constraints that read another item's placement, and parametric operations
will each change where an item ends up, and each needs its inputs settled before it runs. The
viewports, picking, the manipulators, the export and a batch render have to agree on that one
result, and a render of frame N must not overwrite what the user is editing. glTF, USD and RIB
all compose a child's matrix onto its parent's, and glTF requires the node hierarchy to be a tree.

## Decision

The scene holds the hierarchy as a parent for each item, named by document id, and any item may
be a parent. The scene evaluates each item's world matrix as its parent's world matrix times its
own local one, and every reader takes placement from that evaluation rather than from the item.
Deleting an item deletes everything beneath it.

## Alternatives

### Each item holds its parent and composes its own world matrix
- **For**: an item gives its world matrix to any caller that holds it, with no scene in reach.
- **Against**: the world matrix is fixed as a product of local matrices, so a key, a constraint
  or a history node has nowhere to change it. Nothing owns the order in which items are evaluated,
  and nothing separates the evaluated placement from the edited one.
- **Rejected because**: every later source of placement would need this to be reopened.

### The hierarchy in `api/dag`
- **For**: a reusable graph in the library whose name promises one, tested in an api suite.
- **Against**: a link between `dag::Node`s returns a `dag::Node`, so every walk casts. A reparent
  moves a revision and is saved by document id, which the api cannot name. The games place
  entities through ECS, and nothing else in the tree would use it.
- **Rejected because**: its only user is the editor, and every walk would cast.

### A general graph, with parenting one kind of connection
- **For**: one structure for parenting, constraints, history and material assignment, with an
  evaluation order for them all.
- **Against**: placement is a tree with one parent per item, which a general graph enforces as a
  special case. Parenting and the flow of data mean different things, and editors that have both
  keep them apart.
- **Rejected because**: a dependency graph is a structure of its own, and can feed this
  evaluation when it exists.

### A child does not inherit its parent's scale
- **For**: a child under a rotated parent with a non-uniform scale is never sheared.
- **Against**: glTF, USD and RIB inherit the whole matrix, so import and export would each bake
  the difference.
- **Rejected because**: every interchange format composes the whole matrix.

### An item may have several parents
- **For**: one group could appear in several places.
- **Against**: glTF forbids it, and two mesh items can already share one geometry.
- **Rejected because**: geometry is already instanced, and nothing else needs to be.

## Consequences

- **Gains**:
  - One place where keys, constraints and history will change placement, in a defined order.
  - Every reader sees the same placement, and a render of any frame leaves the edit state alone.
  - The hierarchy, import and export compose matrices the same way.
  - A camera rig, or a lamp with its light, needs no extra group, because any item can be a
    parent.
- **Costs**:
  - An item cannot say where it is in the world. Every reader needs the scene's evaluation, and
    has to read it after the edit that changed it.
  - A child under a rotated parent with a non-uniform scale can be sheared, and a manipulator
    edits its local transform in its parent's space.
  - Deleting a group removes its children, and an item cannot be kept by deleting its parent.
- **Revisit when**: one item has to appear in several places, or an item's placement has to be
  read where no scene is in reach.
