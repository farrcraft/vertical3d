# Lights

This page is for anyone changing how moya's new core places lights and what they emit. The
kernel side is in `moya/libmoya/kernel/light/`, in the namespace `v3d::moya::kernel::light`. The
host side is `light::Lights`, in `moya/libmoya/light/`, and the walk that places each light's
shape is in `geometry::gather()`. [Scene.md](Scene.md) lists each light's parameters, and
[Integrator.md](Integrator.md) covers the paths that meet them.

## A light's shape

A rect, disk, sphere or cylinder light has a shape that no geometry in the scene holds. Its shape
is a prototype of the world's hierarchy, so the one traversal that finds geometry finds lights too.

- **Each light places its shape through an instance in the world**, after every other instance.
  The instance's index in the scene is the light's own, so a hit on the shape names the light.
- **Each shape is UsdLux's, in the light's own space.** A rect light is two triangles, its width
  along x and its height along y, centred on the origin and facing -z. A disk light is a disk of
  its radius facing -z. A sphere light is a sphere of its radius. A cylinder light is a cylinder of
  its radius and length along x, with no caps.
- **A disk, sphere or cylinder is one of RI's quadrics.** A quadric's cylinder lies along z, so a
  cylinder light's instance turns it onto x, as the light's transformation stands at every key.
- **A light's shape is built through the geometry cache**, keyed by the light's index. A change to
  a light builds its shape's hierarchy and the world's again, and no other.
- **A light's instance is visible to indirect rays.** Camera rays see it only while the light's
  `camera` parameter is 1, and shadow rays never meet it.

Background: [ADR-0213](../adr/offline/0213-offline-a-lights-shape-is-a-primitive-of-the-hierarchy.md)

## What a light emits

`light::Lights` holds each light with a shape as a `kernel::light::Light`, with its radiance's
parts worked out on the host. Its radiance is UsdLux's.

- **The scale** is the light's `intensity` times two to its `exposure`.
- **The spectrum** is its `colour` as an illuminant, converted into ACEScg from the space the
  colour names, so a white light shines with ACES white at a luminance of one.
- **A colour temperature** applies while `enableColorTemperature` is 1. The spectrum is then a
  Planckian radiator at `colorTemperature` kelvin, of a luminance of one, tinted by the colour as
  an unbounded spectrum. `light::planckian()` finds the radiator's scale and its ACEScg colour,
  summed at 1 nm through the observer. An RGB build uses the colour.
- **`normalize`** divides the radiance by the light's area in the world, as it stands when the
  shutter opens, so its power is the same at any size. A rect's and a disk's area are exact under
  any transformation. A sphere's and a cylinder's are summed over their normals, which is exact
  under a turn and a scale of the same size on every axis. A light of no area is not divided.
- **A light emits from one face.** A rect or disk light emits from the face its -z axis points
  to, and a sphere or cylinder light from its outer face. The inside of a cylinder light, seen
  through its open end, is dark.
- **A cone shapes what a light emits**, as UsdLux's shaping does. The angle off the light's -z
  axis is measured towards where the ray came from. Past `coneAngle` the light emits nothing, and
  from `coneAngle` times one less `coneSoftness` it falls to nothing along a smooth step. A cone of
  90 degrees or more with no softness shapes nothing, which is the fallback. The angle is held to
  [0, 180] degrees and the softness to [0, 1].

The light's axis is turned into the world as a normal is, by its instance's transformation at the
ray's time. A light that moves over the shutter therefore emits from where it stands then.

## Meeting a light

`kernel::light::Lights::at()` gives the place among the lights of the light whose shape a hit
met, from the hit's outermost instance, or none for a hit on anything else. `Lights::emitted()`
gives the radiance the light sends back along the ray. A path that meets a light gathers its share
of that radiance times its throughput, as [Integrator.md](Integrator.md#lights) says, and ends
there, since a light's shape does not scatter.

## Drawing a light

`Lights::pick()` draws a light in proportion to its power, and `Lights::chance()` gives the chance
it was drawn with. A light's power is its radiance's luminance, times its area in the world as it
stands when the shutter opens, times pi. Its cone is left out. `cdf` holds the sum of the chances up
to and including each light, and is empty when no light has any power.

## Sampling a light

`light::sample()`, in `kernel/light/Sampling.h`, draws a point on a light's shape from a shading
point, with its density by solid angle. `light::density()` gives the density with which it draws a
given point, and is zero for a point it never draws. The two always agree, since each decides the
method from the same light, transformation and point.

- **A rect light whose edges stay at right angles in the world** is sampled uniformly in the solid
  angle it subtends, by Ureña, Fajardo and King's spherical rectangle.
- **A disk light that stays round** is sampled uniformly in solid angle by Guillén and others'
  spherical ellipse, through its radial map. The directions to the disk form an elliptic cone.
  The cone's axis bisects the directions to the two points of the rim in the plane of the disk's
  normal and the direction to its centre. Its semi-axes are the tangents of its half angles across
  that plane and in it.
- **The spherical ellipse's solid angle** comes from Legendre's elliptic integral of the third
  kind, through Carlson's symmetric integrals R_F and R_J. A sample's azimuth inverts the solid
  angle swept from the major axis by Newton's method, held within a bracket that halves when a
  step leaves it. Its cosine to the axis is uniform between the cone's edge and one.
- **A sphere light that stays round** is sampled uniformly in the cone it subtends from a point
  outside it, as pbrt-v4 samples it. A narrow cone is sampled by its small-angle expansion.
- **Anything else is sampled uniformly by its area**, and its density converted to solid angle.
  That is a cylinder light, a point inside a sphere light, and a rect, disk or sphere that its
  transformation shears or stretches. A rect or disk that subtends less than `kSmallestSolidAngle`
  is sampled by area too, since its solid angle loses precision there.
- **A rect or disk light draws nothing for a point behind its emitting face**, and its density
  there is zero.
