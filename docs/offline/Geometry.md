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

## Subdivision surfaces

`Surface(topology)` is a subdivision surface's limit, through OpenSubdiv. A `Topology` holds the
control mesh, its scheme and RI's tags. The patches are built once, and every sample of the
points and every primvar is evaluated over them.

- **The schemes are Catmull-Clark, Loop and bilinear.** A Loop surface's faces are triangles.
- **Creases, corners and holes follow RI's tags.** A crease of n vertices is n - 1 edges of its
  sharpness, and a sharpness of 10 or more is infinite. A hole's face is left open.
- **The boundary follows RI's interpolateboundary.** Where it is not interpolated, the faces
  along it are left open. `Edges` interpolates its edges, and `EdgesAndCorners` makes a corner
  of each vertex where two meet.
- **Refinement is adaptive**, isolating creases and extraordinary vertices to 6 levels, with
  Gregory patches where it stops.
- **`cut(rate)` cuts the surface into triangles** of a `Tessellation`, each side of each patch
  face into a rate of pieces. A face of four vertices is one patch face, a Catmull-Clark or
  bilinear face of n others is n, one at each corner, and a Loop face is one triangular patch
  face. Each triangle keeps its face's winding, and the face it was cut from.
- **A tessellation's places are not shared between patch faces**, so a face-varying value can
  differ on each side of an edge. Each place names its patch face and its position there.
- **`at(values, width, interpolation, tessellation)` evaluates a primvar at each place.** A
  vertex value is evaluated on the limit surface, as the points are, and a varying one
  bilinearly across each patch face. A uniform value is its face's, and a constant one is the
  same everywhere.
- **A face-varying value is given at each index**, as RI gives one. Corners of one vertex that
  give equal values share one, so a value that differs only across a seam keeps the seam. It is
  interpolated linearly at the corners of its seams, as RI's facevaryinginterpolateboundary does
  by default, and smoothly elsewhere.
- **A surface of no faces has no patches**, and is cut into nothing.
- **What cannot be refined is refused** with `std::invalid_argument`. A face may have fewer than
  three vertices, or a Loop face more. The counts may not add up to the indices, an index or a
  tagged vertex may lie outside the points, the tags' counts may not match, or a hole may name no
  face. Values of another count than their interpolation gives are refused too.

Background: [ADR-0209](../adr/offline/0209-offline-surfaces-are-tessellated-when-the-hierarchy-is-built.md)

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
