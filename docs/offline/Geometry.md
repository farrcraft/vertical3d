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

## Curves into segments

`cut(points, widths, vertices, cubic, basis, periodic)` cuts a set of curves into segments, and
`varyingCount()` counts the varying values they take.

- **A segment's control points are a centre and a radius each**: two to a linear segment, and
  four to a cubic one. `Segments` holds them, the curve each segment was cut from, and which
  segments close their curves.
- **A curve's segments follow RI.** A linear curve has one between each pair of vertices, and a
  periodic one a last that closes the loop. A cubic curve has one at every step of its basis that
  four vertices from there reach, and a periodic one wraps back to its first vertices. The bezier
  basis steps by 3, b-spline and catmull-rom by 1, hermite by 2, and power by 4.
- **A varying value sits at each end of each segment**, shared between neighbours, so a periodic
  curve takes one per segment. A linear curve's are its vertices.
- **Every cubic segment becomes a Bézier segment exactly.** RI's matrix for its basis takes its
  four vertices to the coefficients of a cubic, and the inverse of the Bézier basis's matrix takes
  those to control points.
- **A width is the curve's diameter**, one per varying value or one per vertex, and a radius is
  half of it. A varying width runs linearly along its segment. A vertex's is converted by the
  basis as the points are. Where the two counts agree, the widths are varying, as RI declares
  them. A radius that the basis takes below zero is held at zero.
- **The last segment of a curve that is not periodic closes it.** Every other segment shares its
  end with the next segment's start.
- **Counts that do not fit are refused** with `std::invalid_argument`. A curve may have fewer
  than two vertices, or vertices that make no whole segments. The counts may not add up to the
  points, or the widths may be of neither count.
