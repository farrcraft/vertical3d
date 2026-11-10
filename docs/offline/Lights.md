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
- **A rect or disk light emits from one face**, the one its -z axis points to. A sphere or
  cylinder light emits from all of it.
- **A cone shapes what a light emits**, as UsdLux's shaping does. The angle off the light's -z
  axis is measured towards where the ray came from. Past `coneAngle` the light emits nothing, and
  from `coneAngle` times one less `coneSoftness` it falls to nothing along a smooth step. A cone of
  90 degrees or more with no softness shapes nothing, which is the fallback. The angle is held to
  [0, 180] degrees and the softness to [0, 1].

The light's axis is turned into the world as a normal is, by its instance's transformation at the
ray's time. A light that moves over the shutter therefore emits from where it stands then.

## Meeting a light

`kernel::light::Lights::at()` gives the light whose shape a hit met, from the hit's outermost
instance, or none for a hit on anything else. `Lights::emitted()` gives the radiance the light
sends back along the ray. A path that meets a light gathers that radiance times its throughput,
and ends there, since a light's shape does not scatter.
