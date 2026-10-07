# The reyes hider

moya's default hider, `"hidden"`. It splits surfaces into grids of micropolygons, shades each
grid, and hides the result into the pixel samples bucket by bucket.

## The reyes hider

The reyes hider renders in two passes. The first pass runs as each primitive arrives. The second
runs at `WorldEnd`.

### First pass: bound, cull and bucket

`RenderContext::addPolygon` does the following for each polygon:

1. Adds the polygon to the traced scene, then stores on it the state it was submitted under:
   the object to eye transformation, the colour, the geometric normal and the shading state.
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
   undiceable. If the raster bound is wider or taller than `sqrt(gridsize) * ShadingRate`
   pixels, marks it undiceable.
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
- **The grid is shaded**: `moya::GridShader` runs the surface shader once over all of the
  grid's vertices and leaves `Ci` on each.
- **The grid is hidden into the samples.** Each micropolygon takes the colour shaded at its
  first corner. It is tested against each sample in the pixels its bound touches, as two
  triangles, and its depth is interpolated at the sample. The nearest micropolygon wins the
  sample.

Rules:

- **A `ReyesPrimitive` carries the transformation, colour, normal and shading state it was
  submitted under.** A split resubmits pieces through the first pass during the second, when
  none of that state is current. The colour and normal fill a piece's vertices that carried
  none, and the normal is every piece's `Ng`.
- **The sweep repeats while any bucket split something.** A piece is bucketed where it lands,
  which can be a bucket the sweep has already passed. A primitive already diced yields no more
  grids, so a repeated sweep costs one pass over the buckets.
- **The samples are one store for the whole frame**, `moya::Samples`, filtered through the film
  once the last bucket is done. A sample is not finished until every grid that could reach it
  is hidden, and the sweep can return to any bucket, so no bucket is finished early. Bucket
  edges never show in the image.
- **Every sample is opaque.** The reyes hider keeps the nearest surface and sets its opacity to
  one, ignoring the shader's `Oi`.
- **A grid is shaded once for all its samples**, so its shadow and traced rays see the scene at
  the shutter's open time.
- **`--grid` and `--bucket` on the command line, and `Option "limits"`**, set the grid size and
  bucket size. `ShadingRate` scales the split threshold.
