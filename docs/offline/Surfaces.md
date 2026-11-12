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

## Materials

The gather reads the material each instance names, once per material. A `kernel::surface::Material`
holds the colour its surface node's `Cs` input gives, converted into ACEScg from the space the
value names, and whether it gives one. It also holds the closures its BSDF is built from, at most
`kMostMaterialClosures`. Each is a model and a scale, and a conductor's also holds its roughness,
its anisotropy and its F82 tint. The gather gives every
material one diffuse closure of scale one. Code can set others on the gathered prototypes, and
`integrate::render()` renders prototypes it is given, as
[Integrator.md](Integrator.md#rendering-a-scene) says. A shading point names the material of the
innermost instance of its chain that names one, so an instance in a group with no material takes
the material of an instance that places the group. The material program replaces this.

## The BSDF

A `kernel::bsdf::Bsdf` is how a shading point scatters light. Its directions are in world space and
point away from the surface: outgoing towards where light goes, and incoming towards where it
comes from.

- **It is a list of weighted closures**, at most `kMostClosures`. Each closure is one lobe: a
  `Model`, a weight at the path's wavelengths, and an estimate of its albedo, the mean of its
  weight. A closure of no albedo is left out. A full BSDF replaces its closure of the smallest
  albedo with a larger one, and keeps its closures when the new one is smaller.
- **It evaluates, gives a density and samples**, each at a path's four wavelengths. A sample
  holds the incoming direction, the value, the density it was drawn with, and its `Lobe`:
  diffuse, glossy, specular or transmission. A density of zero is no sample.
- **Its value is the sum of its closures' values**, each times its weight.
- **A sample picks one closure by its share of the albedo** and draws a direction from it. The
  first random number picks the closure, and is then stretched over the closure's share so the
  closure draws from it again. A BSDF of one closure draws from the same numbers as that closure
  alone.
- **The density of a direction** is each closure's density times the chance it is picked, summed.
- **A closure that scatters into a single direction is singular.** It has no value and no density
  for any direction it is given, so a light sample gathers nothing through it. Its sample is
  marked `specular`: its value is the closure's weight with no cosine, and its density is the
  chance the closure was picked.
- **Kernel code has no virtual call**, so each closure switches over its `Model`.
- **Every closure works in the frame around the shading normal.**
- **A reflection that would cross the geometric surface is refused.** A direction on the other
  side of the geometric surface from outgoing has no value, no density and no sample, whatever
  the shading normal says.

The library's models:

- **`Diffuse` is Lambert's**: its weight over pi, on whichever side light leaves towards. It
  samples by the cosine, through Shirley and Chiu's concentric mapping of the square onto the
  disk. Its bounces are diffuse.
- **`Mirror` is perfect specular reflection** about the shading normal, on either side. It is
  singular, and its bounces are specular.
- **`Conductor` is a metal's reflection**, on either side, and its bounces are glossy. It is
  singular, and its bounces specular, while both its alphas are below `kSmoothest`, 0.001.
  The parts are set out below.

A conductor is made of four parts:

- **GGX's distribution of microfacet normals**, by the closure's alphas along the tangent and
  the bitangent. Masking and shadowing are Smith's, height-correlated.
- **OpenPBR's roughness and anisotropy** give the alphas, through `Ggx::alphas()`. The alpha is
  the roughness squared, and an anisotropy stretches it along the tangent and shrinks it along
  the bitangent, keeping its area. No alpha falls below `kLeast`, 0.0001.
- **Its Fresnel factor is OpenPBR's F82 tint**, an `F82`: Schlick's from the reflectance at
  normal incidence, less Hoffman's term that brings it to the tint times Schlick's at a cosine of
  one seventh. Each wavelength is its own.
- **Kulla and Conty's compensation** gives back what a single scattering loses. It is a lobe of
  one minus each direction's albedo, over pi times one minus the mean albedo, coloured by the
  Fresnel factor's mean over the bounces light makes between microfacets. It reads the albedo
  table at the geometric mean of the two alphas, so an anisotropic conductor's compensation is an
  isotropic one's. Both parts are reciprocal.

A conductor samples a visible normal by Heitz's method with the chance of outgoing's albedo, and
reflects about it. Otherwise it samples by the cosine, as the compensation is shaped. Its
estimated albedo is its weight's mean times its Fresnel factor's.

**The albedo table is integrated when moya is built.** `moya_albedo`, in `moya/tools/albedo/`,
integrates a GGX reflection with a Fresnel factor of one at 32 cosines and 32 roughnesses, the
roughness being the square root of the alpha. Each point is the mean weight of reflections drawn
through visible normals, on a 128 by 128 grid of the random numbers, by the kernel's own GGX. The
mean albedo at each roughness is twice the integral of the cosine times the table. The table is
read between the cosines as the kernel reads it, so the compensation returns all of what the
table says is lost. The tool writes a C++ source, which libmoya compiles, and
`material::ggxAlbedos()` returns a `bsdf::Albedos` view of it. The source is generated, so
clang-tidy and `/analyze` skip it. A table of no points reads an albedo of one, which compensates
nothing.

`bsdf::atSurface(surfaces, point, table, albedos, wavelengths)` gives the BSDF at a shading
point: the closures its material names, or one diffuse closure for a surface with no material.
Its colour is the geometry's `Cs` primvar, which overrides its material's as RI's does, or its
material's colour, or a mid grey. The colour becomes a reflectance through the RGB-to-spectrum
table, as [Spectra.md](Spectra.md#rgb-to-spectrum) says. A diffuse closure's or a mirror's
weight is that reflectance times its scale. A conductor's weight is its scale, and that
reflectance is its reflectance at normal incidence. Its roughness, anisotropy and F82 tint are
its material closure's, the tint becoming a reflectance through the same table.

Background: [ADR-0222](../adr/offline/0222-offline-a-bsdf-is-a-list-of-weighted-closures.md)
