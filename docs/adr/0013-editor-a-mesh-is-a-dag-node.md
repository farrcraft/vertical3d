# ADR-0013: Editor: a mesh is a dag node

**Status**: accepted
**Date**: 2026-09-02
**Documented in**: [editor/](../editor/README.md)

## Context

`v3d::brep::BRep` is a half-edge mesh with no identity, no placement and no selection state.
Picking has to name the mesh it hit, manipulators have to write a placement, a selection has to
persist between the click that makes it and the operation that uses it, and a project file has
to store where each mesh sits. `api/dag` already provides `Node`, which gives an object an id,
and `Transform`, which gives it a placement. The editor also needs somewhere to hold its meshes.

## Decision

`v3d::brep::BRep` derives from `v3d::dag::Node` and `v3d::dag::Transform`, so a mesh has a
process-unique id and a placement, and `v3dlib_brep` links `v3dlib_dag` publicly. Selection is a
flag on the mesh and on each of its vertices, half edges and faces. The scene belongs to the
editor: `v3d::editor::Scene` holds meshes and finds them by node id.

## Alternatives

### Keep `BRep` pure geometry, and wrap it in an editor-side scene node
- **For**: clean layering. `api/brep` stays about topology, and identity and placement belong to
  the scene.
- **Against**: every operation that touches geometry and placement takes two arguments, and one
  given only the mesh cannot say which mesh it is.
- **Rejected because**: the separation adds an argument everywhere, and nothing else in the
  repository enforces that layering.

### The mesh is an EnTT entity, with the brep and a transform as components
- **For**: EnTT and `api/ecs` already exist, entity ids are stable handles for picking, and
  selection becomes a tag component and a query.
- **Against**: the harder half of selection is over the parts inside one mesh, and a vertex is
  not going to be an entity. Selection would be split across two mechanisms, and this would be
  the first real use of the ecs design.
- **Rejected because**: it helps with object selection and not with component selection, which
  is the hard part.

### Grow `api/dag` into a full scene graph, and keep the scene there
- **For**: groups, switches and views would become real, and a modeller's grouping and
  instancing features eventually want a scene graph.
- **Against**: nothing yet needs those classes, so they would be designed with no requirements.
- **Rejected because**: the editor's scene is a flat list because that is what it needs. Grouping
  can move into `api/dag` when it has a caller.

## Consequences

- **Gains**:
  - One object carries geometry, identity and placement, so a function given a mesh has all
    three.
  - Picking can return a node id, and manipulators and project files have one transform per
    mesh to work with.
- **Costs**:
  - A geometry library links a scene-graph library, so anything that wants brep geometry gets
    the dag base too.
  - `BRep` uses multiple inheritance and cannot be copied, because a copy would share its id.
  - Ids come from a process-wide counter and mean nothing in another run, so project files must
    not store them.
  - Selection is flags in several places, kept consistent only by the scene and the select tool.
- **Revisit when**: the editor needs grouping or instancing, or another consumer wants brep
  geometry without the dag dependency.
