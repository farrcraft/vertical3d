# Geometry

This page is for anyone changing how the scene's geometry becomes what a ray traversal holds.
That code is host code, in `moya/libmoya/geometry/` and the namespace `v3d::moya::geometry`. It
reads the scene, and what it builds is copied to a device. [Traversal.md](Traversal.md) covers
the rays and the tests kernel code runs against it.

## Meshes into triangles

`triangulate(points, vertices, indices)` cuts a mesh's faces into triangles.

- **A corner is a position in the mesh's indices.** A triangle's points are
  `P[indices[corner]]`, and a face-varying primvar is read at the corner itself. `Triangles`
  holds three corners per triangle and the face each was cut from.
- **Each triangle keeps its face's winding.**
- **A face of more than three vertices is cut by ear clipping** in its own plane, so a concave
  face is cut inside itself. A face that is not planar is cut in the plane it is nearest, which
  Newell's method finds.
- **A corner in line with its neighbours is left out** before any ear is cut, so no triangle is a
  sliver along a straight edge.
- **A face whose edges cross** is cut as far as ears can be found, and what remains as a fan.
- **A face of fewer than three vertices, or whose points cover no area, is dropped** and counted.
- **Counts that do not fit are refused** with `std::invalid_argument`: a negative count, counts
  that do not add up to the indices, or an index outside the points.
