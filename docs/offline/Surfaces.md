# Surfaces

This page is for anyone changing how moya's new core turns a hit into the point a path shades.
The kernel side is in `moya/libmoya/kernel/surface/`, in the namespace `v3d::moya::kernel::surface`.
The host side is `geometry::Surfaces`, in `moya/libmoya/geometry/`. [Traversal.md](Traversal.md)
covers the hits, and [Geometry.md](Geometry.md) the geometry they are met on.

## The surfaces apart from a traversal

`geometry::Surfaces` holds a scene's prototypes in a device's memory as shading reads them. It
holds them apart from any device's traversal, so a hit from the brute-force traversal, the CPU's
or a GPU's is shaded alike.

- **It holds what the gather gives.** Every geometry is in its own space at each of its samples,
  with the instances that place it and their keys, and the primvars shading reads.
- **A hit names its instances by their index in the scene.** `byId` gives each instance's place
  among the instances by that index.
- **`view()` gives `kernel::surface::Surfaces`**, which kernel code reads. The device outlives
  the surfaces.

## The shading point

`Surfaces::at(hit, ray)` turns a hit into a `ShadingPoint`, as the surface stood at the hit's
time. The time is held to the shutter, as a traversal holds a ray's.

- **The chain of instances is composed at that time**, outermost first. A moving instance's
  transformation is interpolated between its keys, as [Traversal.md](Traversal.md#motion) says.
- **The position** is a triangle's corners weighted by the hit's `uv`, at the triangle's points at
  that time. On any other primitive it is the ray's point at the hit's distance, in the
  primitive's own space.
- **The geometric normal** is a triangle's by its winding, a round point's from its centre, a
  quadric's from its surface and a round curve's from its centre line. A flat point's and a flat
  curve's face back along the ray. It is the hit view's normal.
- **The shading normal** is a mesh's `N` interpolated across the triangle, a subdivision surface's
  limit normal, or the geometric normal. It is turned onto the geometric normal's side.
- **The frame** is three directions at right angles around the shading normal, by Duff and
  others' construction.
- **`front`** says whether the ray met the side the geometric normal faces.
- **`weights`** are the triangle's corners' weights at the position, which a primvar read at its
  corners is interpolated by.
- **`face`** is the face of its mesh, the curve of its segment, the point it met, or 0 for a
  quadric, as the hit view gives it.

A hit that found nothing, or names an instance or a geometry the surfaces do not hold, gives a
point whose `found()` is false.

## Leaving a smooth surface

A coarse mesh with smooth normals shows its triangles' facets along a shadow's edge, since a ray
that leaves the flat triangle towards a light near the horizon meets the next one.

- **`leave` is where such a ray starts**, before it is offset as
  [Traversal.md](Traversal.md#leaving-a-surface) says.
- **On a triangle with a normal at each corner it is Hanika's point.** The position is moved out
  of the tangent plane at each corner that it lies below, by that corner's weight. The point then
  lies on Phong's curved surface through the corners and their normals, or outside it.
- **Everywhere else it is the position.**

## Primvars

The gather reads the primvars shading reads from each geometry's object: `N` as
`kernel::surface::kNormal` and `Cs` as `kColour`. Each is of three floats, as its first sample
holds it.

- **A primvar is spread over its geometry's primitives.** A `kernel::surface::Primvar` holds its
  name, its floats per value, its first value, and its values per primitive:
  - none, for one value for the whole geometry;
  - one, for a primitive's own value;
  - three, for a triangle's corners, in the order of its points.
- **A mesh's primvar of any class** is spread so. A constant value is the mesh's, a uniform one
  each triangle's face's, and a varying, vertex or face-varying one is read at each corner.
- **A subdivision surface's primvar** is evaluated by the surface at each corner of its
  triangles. An undisplaced surface takes its limit normals as its `N`, and a displaced one
  shades with its triangles' own normals.
- **A point cloud's** constant or uniform value is the cloud's, and a varying or vertex one each
  point's.
- **A quadric holds constant and uniform values**, and curves constant values and a uniform
  value per curve. Their other classes are reported and left out.
- **A primvar of another width than three, or of a count its class does not give**, is reported
  and left out.
- **A colour is converted into ACEScg** from the space its value names, through
  `colour::Config::input()`, when the surfaces are made.
- **A cached geometry keeps its primvars**, which a later gather places again with the rest of
  it.

`Surfaces::read(point, name, out, components)` reads a primvar at a shading point, interpolated by
its corners' weights. It is false for a geometry without that primvar.
