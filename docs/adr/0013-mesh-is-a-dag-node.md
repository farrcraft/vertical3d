# ADR-0013: Scene Model — A Mesh Is A dag Node With A Transform, And The Scene Belongs To The Editor

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor draws four viewports of a construction grid and nothing else, because there is
nothing to draw: `v3d::brep::BRep` is a bag of vertices, half edges and faces with no
identity, no placement and no selection state. Everything phase 6 has left needs all three.
Picking has to name what it hit. The translate, rotate and scale manipulators have to write
somewhere. Component selection has to be remembered between the click that makes it and the
operation that consumes it. Project persistence has to write out where each mesh sits.

[RigelSurvey.md](../RigelSurvey.md) records what rigel did: `HalfEdgeBRep` derived from
`DAG::Node` and `DAG::Transform`, the selection model keyed on `id()`, and all three
manipulators wrote through `matrix()`, `translation()`, `rotation()` and `scale()`.
`api/dag::Node` and `api/dag::Transform` are the survivors of that library and carry exactly
those members — but nothing in the repository had ever linked `v3dlib_dag`, and
`Transform.cxx` did not compile: it referred to members the header does not declare and
called `glm::mat4::identity()`, `::translate()` and `glm::quat::matrix()`, none of which
exist. Its own `CMakeLists.txt` listed `Transform.h` twice and the implementation not at all,
which is why nobody had noticed.

The other half of the question is where a scene lives. `v3dlibs/core/Scene` is a flat list of
meshes, cameras and camera profiles with a visitor over the meshes;
[V3dlibsAudit.md](../V3dlibsAudit.md) concluded it should not be folded into `api/dag` —
which is 374 lines of skeleton with no traversal, no visitor and no way to add a child — and
should move with the editor instead.

## Decision

`v3d::brep::BRep` derives from `v3d::dag::Node` and `v3d::dag::Transform`, so a mesh has a
process-unique id and a placement, and `v3dlib_brep` links `v3dlib_dag` PUBLIC. Selection is
a flag on the mesh and a flag on each `Vertex`, `HalfEdge` and `Face`.

The scene is the editor's. `v3d::editor::Scene` holds meshes and nothing else, keyed by node
id; the views own their cameras and `CameraProfiles` owns the table those are built from.

## Alternatives Considered

### Alternative 1: The mesh is a dag node with a transform — **chosen**
- **Pros**: It is what rigel did, so the manipulator, selection and persistence behaviour
  still to be folded in transfers without translation. `api/dag::Node` and `::Transform` gain
  the consumer they were written for. One object is passed around: a function that has a
  mesh has its id and its placement, and cannot be handed one without the other.
- **Cons**: `api/brep`, a geometry library, now links a scene-graph one. Multiple
  inheritance, and a `BRep` that is copied duplicates the geometry under a second id.
- **Why not**: n/a — chosen.

### Alternative 2: Keep `BRep` pure geometry, wrap it in an editor-side scene node
- **Pros**: The clean layering — `api/brep` stays about topology, and identity and placement
  are the scene's concern rather than the mesh's. `api/dag` stays unused and can be deleted.
- **Cons**: Every operation that touches both geometry and placement takes two arguments, and
  every one that takes only the mesh loses the ability to say which mesh it was. Rigel's
  manipulators, its selection encoding and its project writer all assume the mesh knows, so
  each would have to be rewritten rather than folded in — and the survey's whole point is
  that behaviour is the only thing rigel has left to give.
- **Why not**: It buys a layering the repository does not otherwise enforce, at the cost of
  rewriting the six items the survey still has open.

### Alternative 3: The mesh is an entt entity, with the brep and a transform as components
- **Pros**: The repository already has entt and `api/ecs`, the registry already lives on the
  `Controller`, and entity ids are exactly the stable handle picking wants. Selection becomes
  a tag component and a query rather than a flag and a walk.
- **Cons**: [ECSDesign.md](../ECSDesign.md) is aspirational — nothing in the tree stores
  anything meaningful in the registry. Component selection is the harder half of the editor's
  selection model and it is over the parts *inside* one mesh, which entt does not help with:
  a vertex is not going to be an entity. So the flags stay wherever the mesh is, and the ecs
  would carry the object half only, splitting the model across two mechanisms.
- **Why not**: It answers the easy half of selection and not the half that is actually hard,
  and it would be the first real use of an ecs design that has never been exercised.

### Alternative 4: Grow `api/dag` into the scene graph its skeleton promises, and put the scene there
- **Pros**: `Root`, `Group`, `Switch`, `State` and `View` become something. A scene graph is
  what a modeller's group, instance and layer features eventually want.
- **Cons**: Nothing knows what those classes are for; `Group` holds `std::vector<Node*>` with
  no way to add a child and no traversal. Designing a scene graph before the editor has a
  scene is designing against no requirements, and the audit says as much.
- **Why not**: Premature. The editor's scene is a flat list because that is what the editor
  currently needs; when grouping arrives it can push back into `api/dag` with a caller to
  answer to.

## Consequences

### Positive
- The editor draws a scene. `create_poly_cube`/`plane`/`cylinder`/`cone` move out of
  `v3dlibs/core` into `vertical3d/src`, a `WireframeVisitor` turns a scene into a
  `LineCanvas`, and every view draws the same scene through its own camera — four passes over
  one frame, unchanged from [ADR-0011](0011-lines-are-the-second-primitive.md) and
  [ADR-0003](0003-one-realtime-engine.md).
- Picking has something to return: a node id, which is what rigel's name-stack encoding
  reserved its low range for. That decision is still open, and this is the half of it that
  had to land first either way.
- `api/dag::Transform` compiles and is tested. It also composes correctly now — translation *
  rotation * scale, so a non-uniform scale acts along the object's own axes rather than
  shearing whatever the rotation turned.
- `Transform::translation(v)` sets where it used to accumulate, and `translate(v)` is the
  accumulating one. A setter that silently added was going to be found by a manipulator drag.
- `v3dlibs/core/` is empty and gone, which is one of the five items on the `v3dlibs/`
  deletion list.

### Negative
- `v3dlib_brep` links `v3dlib_dag`, so anything wanting brep geometry gets the scene-graph
  base with it. That is one static library of about four hundred lines, but it is a
  dependency in the direction a purist would not draw it.
- `dag::Node`'s id comes from a process-wide counter that is incremented in the constructor,
  never reused, and not thread safe. Ids are unique within a run and mean nothing between
  runs, so project persistence will have to write its own and remap on load rather than
  trusting these.
- `BRep` has no const accessors — `edge()`, `face()` and `vertex()` are all non-const — so a
  visitor that only reads a scene still takes a non-const mesh.

### Risks
- Selection is now four flags in three places and nothing yet clears them together except
  `BRep::deselectComponents()` and `Scene::deselect()`. A select mask — vertex, edge or face
  mode — is the thing that decides which of them a click writes, and it does not exist yet;
  until it does, nothing enforces that only one kind is selected at a time.
