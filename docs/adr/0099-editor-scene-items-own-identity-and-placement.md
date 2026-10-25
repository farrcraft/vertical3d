# ADR-0099: Editor: scene items own identity and placement

**Status**: accepted
**Date**: 2026-10-24
**Supersedes**: ADR-0013
**Documented in**: [editor/Scene.md](../editor/Scene.md#the-scene)

## Context

The scene has to hold cameras and lights as well as meshes, and later groups whose children are
of any kind. Every kind needs an id, a placement and an object selection flag, and grouping and
keying will add a parent and keyed tracks to each. A mesh has been a `brep::BRep` that derives from
`dag::Node` and `dag::Transform`, so the geometry carried the identity. That rules out two objects
sharing one mesh, and makes an importer mint an id and a placement for geometry that may never
enter a scene.

## Decision

A scene item is an editor class that derives from `dag::Node` and `dag::Transform` and holds the
object selection flag. A mesh item, a camera and a light each derive from it, and a mesh item holds
a shared `brep::BRep`, which is geometry alone with the selection flags of its own parts. A visitor
has one pure `visit` per kind, and the scene keeps every item in one list in insertion order.

## Alternatives

### A `std::variant` of the kinds, with the mesh still the `BRep`
- **For**: the set of kinds is closed, each `std::visit` fails to compile when a kind is missing,
  and a typed lookup needs no cast.
- **Against**: the state every kind shares, now and later, is repeated in each kind or embedded
  in each. Geometry keeps its identity, so two items cannot share a mesh.
- **Rejected because**: the shared state grows with grouping and keying, and a base class is
  the one place it belongs.

### The scene holds `dag::Node`, and a kind is found by `dynamic_cast`
- **For**: no new type, and the kinds stay open.
- **Against**: a kind a caller forgets is skipped at run time without a word, the placement is
  reached by a cross cast that can fail, and the selection flag has no common home.
- **Rejected because**: it gives up every check the other designs have, for openness nothing needs.

### One list per kind in the scene
- **For**: each list stays typed, and no kind depends on another.
- **Against**: the order across kinds is lost. Selection, deletion, moving and keying are each
  written once per list, and a group cannot hold children of mixed kinds.
- **Rejected because**: most of what the editor does to an item does not depend on its kind.

### The mesh stays a `dag::Node`, and the other kinds join it under a common base
- **For**: a function given a `BRep` has its id and its placement as well.
- **Against**: `BRep` would need the editor's base, which the api cannot name, or the other kinds
  would have no common base with it.
- **Rejected because**: identity belongs to the scene, and the geometry is reused without it.

## Consequences

- **Gains**:
  - Id, placement, selection, and later the parent and the keys, live in one class.
  - Two mesh items may share one `BRep`, and an importer builds geometry with nothing attached.
  - `api/brep` no longer depends on `api/dag`, and a `BRep` can be copied.
  - A kind added to the visitor is a compile error in every visitor that does not handle it.
- **Costs**:
  - Code that needs the geometry and the placement holds the mesh item, and reaches the geometry
    through it.
  - Getting one kind from an item is a downcast, guarded by the item's kind.
  - Component selection is on the shared `BRep`, so two items sharing a mesh share it.
- **Revisit when**: a kind is wanted that the editor cannot define, or two items sharing a mesh
  need separate component selections.
