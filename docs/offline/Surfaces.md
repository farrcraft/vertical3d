# Surfaces

This page is for anyone changing how moya's new core turns a hit into the point a path shades.
The kernel side is in `moya/libmoya/kernel/surface/`, in the namespace `v3d::moya::kernel::surface`.
The host side is `geometry::Surfaces`, in `moya/libmoya/geometry/`. [Traversal.md](Traversal.md)
covers the hits, and [Geometry.md](Geometry.md) the geometry they are met on.
[Materials.md](Materials.md) covers the material a shading point is shaded by.

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
- **`toWorld` and `time`** are the transformation from the geometry's own space into the world,
  and the time it was composed at, which a program's geometric nodes read.
- **`front`** says whether the ray met the side the geometric normal faces.
- **`footprint`** is where the path's cone met the surface, as
  [Integrator.md](Integrator.md#a-rays-footprint) says. A point no path reached has a footprint of
  no area.
- **`weights`** are the triangle's corners' weights at the position, which a primvar read at its
  corners is interpolated by.
- **`face`** is the face of its mesh, the curve of its segment, the point it met, or 0 for a
  quadric, as the hit view gives it.

A hit that found nothing, or names an instance or a geometry the surfaces do not hold, gives a
point whose `found()` is false.

### The tangent frame

`Surfaces::tangentFrame(point)` gives the frame a layered BSDF builds its frames from, and the
frame a program's tangent and bitangent nodes read.

- **The tangent** is the geometry's `tangent` primvar, turned into the world, where it has one.
  Otherwise it is the direction the point moves in as `s` increases, on a triangle whose corners
  each hold `st`. It is laid flat against the shading normal.
- **The bitangent** is the normal's cross product with the tangent, turned to the side `t`
  increases on.
- **A point with neither keeps its frame**, as does one where the tangent lies along the normal.

### A material's normals

OpenPBR's `geometry_normal` and `geometry_tangent` give the frame every closure but the coat
works in, and `geometry_coat_normal` and `geometry_coat_tangent` the coat's. Each is a direction
in world space that the program gives. `surface::frameAbout()` makes each frame from the tangent
frame.

- **A direction of no length is the shading point's own**, so an input that is not connected
  changes nothing. A material that gives neither a normal nor a tangent keeps the tangent frame.
- **A tangent is laid flat against the normal**, or is the frame's own around the normal where it
  lies along it. The bitangent is the normal's cross product with the tangent, turned to the
  tangent frame's bitangent's side.
- **A normal is bent back where it faces away.** `surface::visible()` turns a normal towards the
  geometric normal, in the plane the two share. It turns just far enough that outgoing's mirror
  about it keeps a cosine to the geometric normal of `kLeastRise`. Where outgoing grazes, the
  mirror keeps `kRiseShare` of outgoing's own cosine instead, where that is less. A normal whose
  mirror keeps that cosine is left as it is. The normal sought solves a quadratic in the square
  of its cosine to the geometric normal, as Cycles bends a normal-mapped one, so a bumped surface
  shows no black.

`Surfaces::shifted(point, axis)` moves a shading point by a step along an axis of its footprint,
which a program's height differences read.

- **Each step is the axis's whole length**, so the differences are taken across the area the
  footprint covers. A point whose footprint has no area steps `kShiftStep` along `s` and along
  `t`, on a triangle whose `st` varies, and otherwise does not move.
- **The position moves by the step.** On a triangle the corners' weights and `uv` move by the
  step's nearest sum of its edges, so every primvar reads as it would where the point moved to.
  The normals, the frame and the footprint stay as they were.

### The footprint

- **`Surfaces::textureFootprint(point)`** gives the footprint's two axes as changes in `s` and
  `t`. Each axis is written as a sum of the directions the point moves in as `s` and as `t`
  increase, the nearest such sum, on a triangle whose corners each hold `st`. A point with no
  `st` that varies across its triangle gives a footprint of no area.
- **`Surfaces::curvature(point)`** gives a triangle's mean curvature at the point, as one over a
  radius, from the normals its corners hold. It averages how the normal turns along the frame's
  tangent and bitangent. It is above zero where the surface is convex on the side the ray met,
  and below where it is concave. A triangle without normals at its corners is flat, and so is
  any other primitive.

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

The gather reads the primvars shading reads from each geometry's object, as their first sample
holds them. Each is named by a number, which `Prototypes::primvarNames` maps back to its name.

- **Four are read whatever a program does**: `N` as `kernel::surface::kNormal`, `Cs` as
  `kColour` and `tangent` as `kTangent`, each of three floats, and `st` as `kTexture`, of two.
- **Every other primvar a program reads** is numbered from `kFirstNamed`, in the order the
  programs first read them, and is of one to four floats.
- **Every MaterialX material is compiled before any geometry is gathered**, so the names are
  known first. A cached geometry holds only the primvars that were read when it was made, so the
  cache is emptied when the names change.

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
- **A primvar of another width than shading reads it at, or of a count its class does not give**,
  is reported and left out. A float primvar's width is its declaration's.
- **A colour primvar is converted into ACEScg** from the space its value names, through
  `colour::Config::input()`, when the surfaces are made. A primvar of another type has no space.
- **A cached geometry keeps its primvars**, which a later gather places again with the rest of
  it.

`Surfaces::read(point, name, out, components)` reads a primvar at a shading point, interpolated by
its corners' weights. It is false for a geometry without that primvar.
