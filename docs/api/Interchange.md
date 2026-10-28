# Interchange

`api/interchange` (`v3dlib_interchange`, namespace `v3d::interchange`) reads a scene from a file
another tool wrote, and holds it in terms of no format. It reads glTF.
`v3dlib_interchange_usd`, in `api/interchange/usd`, reads USD into the same scene, and is the one
library that links OpenUSD. The editor imports through both. A game that only draws a model
loads glTF through [asset_media](Models.md#loading-gltf) instead, which merges a file into one
`model::Model`.

The library also owns cgltf's implementation, which is compiled once for every program.
`asset_media` links it for that.

Background: [ADR-0123](../adr/0123-interchange-file-readers-fill-a-neutral-scene.md)

## The scene

`interchange::Scene` ([api/interchange/Scene.h](../../api/interchange/Scene.h)) holds six lists.
Up is +y.

- **Nodes.** Each has a name, a parent, a placement and up to one mesh, one camera, one light and
  one set of curves, each by its index in the scene. A node with none of them only groups its
  children.
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
  - Both give their near and far clipping distances, in that order, as `clipping`. The far one is
    zero when there is no far plane.
- **Lights.** A point, distant or spot light, with a colour and an intensity. The intensity is
  in watts per steradian for a point or spot light and watts per square metre for a distant
  one. A distant or spot light shines along its node's -z axis. A spot light gives two half
  angles: `outerCone`, where its light ends, and `innerCone`, where it starts to fade toward
  that edge.
- **Curves.** `curve::Curve` values in the node's space, each of any type, and one width for
  all of them in the node's units. The width is zero when the file does not give one.
  [Curves.md](Curves.md) describes the types.
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
  distant one. glTF gives a light in candela or lux, which the reader divides by 683 lumens per
  watt.
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

## Reading USD

`interchange::readUsd(path)` ([api/interchange/usd/Usd.h](../../api/interchange/usd/Usd.h)) opens
a `.usd`, `.usda`, `.usdc` or `.usdz` stage, and returns an `Imported` as `readGltf()` does.

What it reads:

- **Which prims.** Every prim that can be placed, and every `Scope`, which groups without
  placing. Instanced prims are read as copies. A prim that cannot be placed, such as a material
  or a shader, is left out with everything beneath it.
- **Up and units.** The scene is y up and in metres. A z up stage, or one in another unit, is
  turned and scaled at its top. A still prim at the top takes the turn into its own placement. A
  keyed one goes beneath a group named `stage` that holds the turn, so its keys keep their
  meaning.
- **Placement.** A prim's local transform at the stage's start time, taken apart as a matrix
  is. A keyed placement is sampled at every time the stage keys it, as linear keys of its
  translation, its rotation and its scale. A part that keeps one value is not keyed. Times are
  in seconds, by the stage's time codes per second.
- **Meshes.** Polygons as the file gives them, so `triangulated` is false. A left handed mesh
  has each face turned round.
- **Cameras.** The lens at the start time. Apertures are in tenths of the stage's unit.
- **Lights.** A `SphereLight` is a point light, or a spot light when it is shaped to a cone
  narrower than a quarter turn. Its outer angle is the cone's angle, and its inner angle is that
  less the cone's softness. A `DistantLight` is a distant light. The power is worked out as
  Blender writes and reads it. A sphere light gives intensity times two to its exposure times pi
  watts when it is normalized or has no radius, and that times its surface area otherwise,
  spread over a sphere of directions. A distant light's intensity is watts per square metre.
- **Curves.** A `BasisCurves` or `NurbsCurves` prim gives its curves at the start time, at the
  mean of its widths. Each is converted to the type that is the same curve:

  | USD | Curve |
  |---|---|
  | `linear` | Poly, closed when periodic |
  | `bezier` | Bezier, every third point on the curve and the two between its handles, all free |
  | `catmullRom`, periodic | Catmull-Rom, closed |
  | `catmullRom`, not periodic | Bezier, from the second point to the second to last, each segment's handles a sixth of the way between its neighbours |
  | `bspline`, periodic | NURBS, cubic, closed |
  | `bspline`, not periodic | NURBS, cubic, over evenly spaced knots |
  | `NurbsCurves` | NURBS, with each curve's order, run of the knots and weights |

  A pinned curve gains each end point reflected through its neighbour beyond its end, so it
  starts and ends on them.

Left out, each with a note:

- a light of any other kind, whose prim becomes a node that only groups
- a mesh's keyed points, which are read at rest
- curves of a `hermite` basis, whose prim becomes a node that only groups
- a curve whose count or knots make no curve, such as a Bezier curve of five points
- curves' keyed points, which are read at rest
- a prim's choice to discard its parent's placement, which is placed beneath its parent anyway
