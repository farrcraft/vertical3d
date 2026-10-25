# The reyes hider

moya's default hider, `"hidden"`. It splits surfaces into grids of micropolygons, shades each
grid, and hides the result into the pixel samples bucket by bucket. Its classes are in
`moya::reyes`, in `moya/libmoya/reyes/`. The polygons and vertices it dices are shared with the
ray hider, and are in `moya/libmoya/`.

## The reyes hider

The reyes hider renders in two passes. The first pass runs as each primitive arrives. The second
runs at `WorldEnd`.

### First pass: bound, cull and bucket

`RenderContext::addPolygon` does the following for each polygon:

1. Adds the polygon to the traced scene, then stores on it the state it was submitted under:
   the object to eye transformation, the colour, the geometric normal and the shading state.
   A polygon with more than four vertices is then broken into a fan of quadrilaterals about
   its first vertex, with a triangle last when the count is odd, because dicing interpolates
   four corners. Each piece goes through these steps in turn.
2. Gives each vertex without a `"Cs"` the current colour, and each vertex without an `"N"` the
   geometric normal.
3. Bounds the polygon in object space and moves the bound to eye space. All eight corners of
   the box are moved, since under a rotation any of them can hold the extreme on an axis. A
   moving primitive's bound covers both ends of the shutter, bounded the same way.
4. Culls the polygon if its bound is entirely beyond the far clipping plane or entirely before
   the near one.
5. Marks it undiceable if it crosses the near plane and reaches behind the eye (z below zero).
6. Culls it against the view frustum, testing the eye space bound against the projection.
7. Projects all eight corners of the eye space bound into raster space. A corner at or behind
   the eye has no raster position: it is left out of the raster bound and the polygon is marked
   undiceable. If the raster bound is wider or taller than `sqrt(gridsize * ShadingRate)`
   pixels, marks it undiceable. `ShadingRate` is the area of one micropolygon in pixels, as RI
   defines it.
8. If it is diceable, moves its vertices and normals into eye space.
9. Files it in the bucket that holds the upper left corner of its raster bound. A corner off
   the image is clamped to the nearest bucket.

Only `Polygon` and `PointsPolygons` reach this pass. Spheres are not diced.

### Second pass: split, dice, shade and hide

`moya::FrameBuffer::render` sweeps the buckets. For each primitive in a bucket:

- **An undiceable primitive is split** into up to four pieces, by two planes through the centre
  of its bound, perpendicular to its plane and to each other. Each piece goes back through the
  first pass, which bounds, culls and buckets it again. A piece with fewer than three vertices,
  or one no smaller than its parent on any axis, is dropped, so splitting terminates.
- **A split carries `"st"`, colour and shading normal.** A corner of a piece keeps the values of
  the vertex it came from. A vertex made where a plane cuts an edge takes each value as far
  along the edge as it lies, when both ends of the edge have one, and its normal is
  renormalised. A value a vertex does not get is filled from the primitive's colour and plane,
  as on a primitive the scene gave none.
- **A diceable primitive is diced into one grid** of `sqrt(gridsize)` by `sqrt(gridsize)`
  micropolygons, 16 by 16 at the default. Dicing interpolates position, colour, shading normal
  and `"st"` bilinearly over the first four vertices. A triangle's fourth corner is its third. A
  polygon with more than four vertices loses the rest, which gives a wrong grid for a concave
  polygon.
- **The grid is displaced**, when the primitive has a displacement shader.
  `GridShader::displace` runs it once over every vertex and leaves the `P` and `N` it wrote on
  each, so the surface shader sees the moved surface.
- **The grid is shaded**: `moya::reyes::GridShader` runs the surface shader once over all of the
  grid's vertices and leaves `Ci` and `Oi` on each. A shader that never writes `Oi` leaves the
  primitive's `Os`. It answers `Renderer::shape()` with the grid, so a
  derivative differences a vertex with the next one along u or v, or the previous one on the
  last row or column. Vertex (i, j) is batch point `i * size + j`, at u = i / (size - 1).
- **I follows the projection.** Under a perspective camera I is the shading point, since the eye
  is the origin of camera space. Under an orthographic camera I is (0, 0, 1). E is the origin
  under both. A traced hit takes the same values, because I is its ray's direction.
- **The grid is hidden into the samples.** Each micropolygon takes the colour and opacity
  shaded at its first corner. It is tested against each sample in the pixels its bound
  touches, as two triangles, and its depth is interpolated at the sample. The sample keeps it
  as a layer unless an opaque surface is already as near.

Rules:

- **A `reyes::Primitive` carries the transformation, colour, normal and shading state it was
  submitted under.** A split resubmits pieces through the first pass during the second, when
  none of that state is current. The colour and normal fill a piece's vertices that carried
  none, and the normal is every piece's `Ng`.
- **The sweep repeats while any bucket split something.** A piece is bucketed where it lands,
  which can be a bucket the sweep has already passed. A primitive already diced yields no more
  grids, so a repeated sweep costs one pass over the buckets.
- **The samples are one store for the whole frame**, `moya::reyes::Samples`, filtered through
  the film once the last bucket is done. A sample is not finished until every grid that could
  reach it is hidden, and the sweep can return to any bucket, so no bucket is finished early.
  Bucket edges never show in the image.
- **A sample keeps every surface nearer than its nearest opaque one.** A surface is opaque when
  all three channels of its `Oi` are one. An opaque surface drops every layer at its depth or
  behind it, and a surface that arrives behind it is not kept.
- **A sample keeps one layer from each grid, the nearest.** A point on an edge two
  micropolygons share is inside both, so without the rule a translucent surface would be
  composited twice along its micropolygons' edges. One diced polygon is planar, so a grid
  cannot cover a sample twice at two depths.
- **A sample's layers are composited front to back when the film resolves them**, as a ray
  composites what it passes through: each adds its `Ci` scaled by what the nearer ones let
  through, `1 - Oi`. The order the layers arrived in does not matter. The sample's depth is its
  nearest layer's.
- **A primitive's bound grows by its `displacementbound`** before it is culled and bucketed, so
  a surface the displacement moves into view is not culled. A displacement that goes past its
  bound is clipped, and a primitive with no bound does not grow. The split test measures the
  primitive without the bound: a grid covers the surface, not its bound, and a bound wider than
  a grid would otherwise be split without end.
- **The traced scene holds a displaced primitive where its displacement moved it**, as
  [RayTracing.md](RayTracing.md) describes, so its shadows and reflections follow its moved shape
  under this hider too.
- **A grid of a polygon with no `st` takes its own parameters as s and t**, so the pieces of a
  split polygon each run from 0 to 1. A displacement that should be continuous across a
  polygon reads a position, not s.
- **A grid is shaded once for all its samples**, so its shadow and traced rays see the scene at
  the shutter's open time.
- **`--grid` and `--bucket` on the command line, and `Option "limits"`**, set the grid size and
  bucket size. The command line wins: a size it names is pinned, and a scene's `Option "limits"`
  leaves a pinned size as it is. `ShadingRate` sets the split threshold through its area.
