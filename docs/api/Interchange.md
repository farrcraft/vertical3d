# Interchange

`api/interchange` (`v3dlib_interchange`, namespace `v3d::interchange`) reads a scene from a file
another tool wrote, and holds it in terms of no format. The editor imports through it. A game
that only draws a model loads glTF through [asset_media](Models.md#loading-gltf) instead, which
merges a file into one `model::Model`.

The library also owns cgltf's implementation, which is compiled once for every program.
`asset_media` links it for that.

Background: [ADR-0123](../adr/0123-interchange-file-readers-fill-a-neutral-scene.md)

## The scene

`interchange::Scene` ([api/interchange/Scene.h](../../api/interchange/Scene.h)) holds five lists.
Up is +y.

- **Nodes.** Each has a name, a parent, a placement and up to one mesh, one camera and one light,
  each by its index in the scene. A node with none of them only groups its children.
  - The placement is a `type::Transform` in the parent's space, or the scene's when the node has
    no parent.
  - A parent always comes before its children in the list, so a walk in order meets every parent
    first.
- **Meshes.** A list of points, a count of corners for each face, and every face's corners as
  indices into the points, one face after another. A face runs counter-clockwise seen from
  outside. `triangulated` says that the format stores only triangles, so a quad the author made
  arrives cut in two. `brep::build()` with `joinTriangles` puts such quads back together.
- **Cameras.** A camera looks along its node's -z axis, with +y up.
  - A perspective camera gives its full angle of view from the bottom of the picture to the top
    (`yfov`), in radians, and its aspect when the file states one.
  - An orthographic camera gives half the width and half the height it sees (`magnification`).
  - Both give their near and far clipping distances. `far` is zero when there is no far plane.
- **Lights.** A point, distant or spot light, with a colour and an intensity in the file's own
  units. A distant or spot light shines along its node's -z axis. A spot light gives two half
  angles: `outerCone`, where its light ends, and `innerCone`, where it starts to fade toward
  that edge.
- **Channels.** The keyed values of one part of one node's placement: its translation, its
  rotation or its scale.
  - Times are in seconds, rising.
  - A value is a `vec4` for every part: x, y and z for a translation or a scale, and a
    quaternion's x, y, z and w for a rotation.
  - Interpolation is step, linear or cubic spline. A cubic spline key also has a tangent on each
    side, in value per second.

## Reading glTF

`interchange::readGltf(path)` ([api/interchange/Gltf.h](../../api/interchange/Gltf.h)) reads a
`.gltf` or `.glb` file. It returns an `Imported`:

- `scene` holds the scene, or is empty when the file cannot be read, parsed or validated, and
  `problem` then says why.
- `notes` lists, a line each, what the file held that the scene cannot describe.

What it reads:

- **Which nodes.** The scene the file names as its default, or its first scene, or every root
  node when the file has no scene.
- **Placement.** A node's translation, rotation and scale. A node placed by a matrix is placed
  by that matrix taken apart: the last column is the translation, the length of each other
  column is a scale, and what is left is the rotation. A matrix that mirrors puts the mirror in
  its x scale.
- **Meshes.** All of a mesh's primitives join into one mesh of triangles. Strips and fans are cut
  into triangles, and a primitive with no indices takes its points in order. A mesh that two
  nodes use is read once, and both nodes name it.
- **Cameras and lights.** Lights come from `KHR_lights_punctual`. A directional light is a
  distant one.
- **Animation.** Every channel of every animation that moves, turns or scales a read node. A
  file whose animations each move one node, as Blender writes one, plays as one scene.

Left out, each with a note:

- a primitive of points or lines, or one with no positions
- a skin, whose mesh is read at rest, and morph targets and their keyed weights
- a second channel for a part of a node that an earlier channel already keys

```cpp
const v3d::interchange::Imported imported = v3d::interchange::readGltf("scene.glb");
if (!imported.scene) {
    logger_->get()->error("{}", imported.problem);
    return;
}
for (const std::string& note : imported.notes) {
    logger_->get()->warn("{}", note);
}
```
