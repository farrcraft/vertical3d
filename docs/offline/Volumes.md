# Volumes

Light in moya travels through what lies between surfaces, not only from one surface to the next.
A medium is the inside of a closed surface whose material names one, or a volume read from
OpenVDB grids, which an instance places with no surface around it. A path carries the media it
is inside and tracks through them at its four wavelengths. It scatters and is absorbed in them,
and gathers what they emit.

- [Scene.md](Scene.md) names the scene's `volume` and `openVDBAsset`, and a material's volume
  terminal.
- [Materials.md](Materials.md#compiling-a-volume) compiles a volume shader into a medium, and
  gives the media an OpenPBR surface encloses: its transmission's and its subsurface's.
- [Integrator.md](Integrator.md#walking-a-subsurface) walks the medium below a subsurface. It
  also says which collision is a camera sample's first hit, and what alpha a medium leaves.
- [Rib.md](Rib.md) reads RenderMan's `Volume`, and [Compositing.md](Compositing.md) names a
  medium's events and its deep samples.

## Nested media

A path carries the closed surfaces it is inside, in `kernel/integrate/media/Media.h`, so a
dielectric bends light by the media on its two sides and overlapping surfaces give one medium.

- **A material encloses a medium** when a path may cross its surface into it. A layered material
  does when it is not thin-walled and its transmission's or its subsurface's weight may be more
  than zero. Any other
  does when it names a dielectric closure that is not thin. A material with a volume terminal and
  no surface terminal is a boundary, which encloses its volume and has no BSDF.
- **A path enters a surface it crosses from the side its normal faces, and leaves one it crosses
  back.** It records each surface by the chain of instances that placed it, its priority, and the
  index of refraction of the medium inside. A layered material's index is its specular index,
  scaled by its specular weight. A boundary's is one. It records too whether it walks the
  surface's subsurface, which it does when it entered by the subsurface's closure. A path that
  enters a surface any other way walks it when its material's subsurface weight may be more than
  zero and its transmission's may not.
- **The medium a path is in** is that of the surface of highest priority it is inside, and of
  those the one it entered last. An instance's `priority` is 0 unless a client sets it. A hit
  takes the priority of the innermost instance in its chain whose priority is not zero. A scene
  that sets no priority behaves as a plain stack.
- **A path passes by a surface of lower priority than its medium**, and a boundary, as no bounce.
  It enters or leaves the surface's medium as it passes, and goes on from just past the hit. A
  path passes by at most 256 surfaces.
- **A dielectric's index is relative to the medium beyond it**: the medium the path is in, leaving
  out the surface it met. A ratio below one turns the dielectric's frame over, so a glass of index
  1.5 in water of 1.33 bends by their ratio from either side.
- **A path holds at most eight surfaces.** One that enters a ninth leaves the one of lowest
  priority first, and of those the one it entered first.
- **A camera ray finds the surfaces it starts inside** when any material encloses a medium. It is
  traced on through everything it meets, and it is inside each surface whose first face it meets
  is a back face.
- **A shadow ray carries a copy of the media of the path it leaves**, when any material encloses
  a medium. One that crosses the surface it leaves first enters or leaves the medium that surface
  encloses, as a path that scatters across it does. It passes by the surfaces a path would pass
  by, entering and leaving their media as a path does, and they cast no shadow. Any other surface
  it meets casts its shadow as it would outside a medium.

Background: [ADR-0243](../adr/offline/0243-offline-nested-media-are-resolved-by-instance-priority.md)

## Tracking through a medium

The medium a path is in is its winning surface's material's, in `kernel/volume/`. A path in no
medium, or in one of no coefficients, carries its light unchanged from one surface to the next.

- **A medium's coefficients** are absorption and up to two lobes of scattering, each with an
  anisotropy, in ACEScg. A path reads each as the unbounded spectrum of its colour at its four
  wavelengths, `volume::coefficientsOf()`. The extinction is the absorption and every lobe's
  scattering.
- **A homogeneous medium is tracked by its own extinction.** The distance to a collision is drawn
  from the exponential distribution of the extinction at the tracked wavelength. A collision
  scatters with the chance of that wavelength's scattering over its extinction, and is otherwise
  absorbed, which ends the path. A path whose collision lies past the surface it meets reaches the
  surface.
- **The tracked wavelength** is a spectral path's first. Its four are drawn as one evenly spaced
  set, so any of them is equally likely to come first. An RGB path is tracked by a channel drawn
  from a hash of its pixel and sample.
- **Every wavelength is weighed by spectral multiple importance sampling**, `rescale()`. An
  event's value at each wavelength is also the density with which a path tracked there would have
  made it. That is the transmittance for a path that reaches the surface, and the scattering
  times the transmittance for one that scatters. The path's throughput is its value
  over the average of those densities over the whole path, so a medium whose coefficients differ
  by wavelength stays unbiased at each. A path that crosses no medium keeps chances of one, and
  its throughput is what it would be without them.
- **A scattering event takes a light sample**, `directInMedium()`, as a shading point does, with
  the phase function in place of the BSDF and no normal. Its light is multiplied by the
  transmittance of the media its shadow ray crosses, by Beer and Lambert's law. It is weighed
  against the phase function's sample by the power heuristic, where the phase function's density
  is multiplied by the chance a tracked path would have passed those media. A light a path meets
  after a phase function's sample is weighed the same way, since a path's density grows by that
  chance along every ray. The point is lit by the lights that light an instance of no links.
- **The phase function** is each lobe's Henyey and Greenstein's, weighed at each wavelength by its
  share of the scattering there, `volume::phaseValue()`. A direction is drawn from one lobe, picked
  by its share of the scattering over all four wavelengths together, so its density is the same
  whichever wavelength is tracked. The throughput is multiplied by the phase function over that
  density.
- **The ray that leaves a scattering event** starts at the point itself, its cone spread as a lobe
  of roughness one spreads it. The event gives a denoiser nothing past it.

Background: [ADR-0244](../adr/offline/0244-offline-media-are-tracked-by-null-scattering-with-spectral-mis.md)

## Tracking through volumes

A volume is a medium read from grids, which an instance places and no surface bounds. It stays out
of a path's media and adds itself to whatever medium the path is in. The gather turns each chain
of instances that places a volume into a `kernel::volume::Volume`, and `volume::Volumes` holds
them on the device beside the NanoVDB buffer of each grid they read.

- **A volume's medium** is its material's volume program, run with the values of its fields at
  the point, `program::mediumWithFields()`. A field is read as the primvar its name gives, so
  `geompropvalue` reads it. A volume of no material, or whose material has no volume program,
  scatters its field named `density`, the same at every wavelength and every way. A coefficient
  below zero is taken as zero. The innermost instance that names a material gives it.
- **The fields a volume reads** are its density and each field its program reads, in the order the
  volume names them, at most `kernel::volume::kMostFields`. Each is read by trilinear
  interpolation between the eight voxels around the point, `volume::valueAt()`. A volume that
  would read more is reported and reads the first of them. One that would read none, such as a
  volume of no program and no density, is reported and holds nothing.
- **A volume's bounds** are those of the grids it reads, one voxel past their active voxels, in
  its own space. A ray is clipped to them as its volume's instances stand at the ray's time, so a
  volume moves with its instances over the shutter. A volume whose grids hold no active voxel is
  left out. A ray sees a volume only when every instance in its chain is visible to rays of its
  kind.
- **A majorant bounds the extinction**, cell by cell of the volume's walked field: its density, or
  its first field when it has none. A cell is the node or tile of the grid's tree around a voxel,
  no smaller than a leaf, `volume::cellAt()`. A point reads voxels up to one past its floor, so a
  cell's most is taken over the cell and the seven cells of its side beyond it. The majorant is
  the program's extinction with the walked field at the cell's most and every other field at its
  grid's most. A program that reads anything but its fields and constants, such as a position or
  an image, cannot be bounded that way. Its volume takes that majorant over the whole volume, with
  the walked field at its grid's most too. A node's most counts its active values alone.
- **A path crossing volumes is tracked by null scattering**, `trackCrossed()`. The majorant is the
  homogeneous medium's extinction and the sum of each crossed volume's over the cell the ray is
  in, and a distance is drawn again wherever it changes. At a collision the coefficients of every
  medium there are joined, each lobe joining one of the same anisotropy, an empty one, or the one
  of nearest anisotropy, `volume::join()`. The collision scatters, is absorbed or is null with the
  chance of the tracked wavelength's scattering, absorption or what is left of the majorant, over
  the majorant. A null collision weighs each wavelength by what is left of the majorant times the
  majorant's transmittance, by `rescale()`, and the path goes on. A stretch of ray that crosses
  more than `kMostCrossed` volumes is tracked through the first of them, and a path that collides
  more than `kMostCollisions` times on one stretch ends.
- **A shadow ray takes a volume's transmittance by ratio tracking**, `passVolumes()`. It draws
  collisions against the volumes' majorant alone, since it takes the homogeneous medium's by Beer
  and Lambert's law. At each, every wavelength lets through what is left of the majorant there.
  Its numbers come from the bounce's collision draws, counted on from `kShadowCollisions`.
- **Multiple importance sampling counts the null collisions as part of a path.** A shadow ray
  would have drawn a null collision with the density of the volumes' majorant, and a tracked path
  with that of what is left of the whole majorant. The path's density is divided by the growth of
  its lightChances, which grow by the shadow ray's densities, and a light sample's chance is
  divided by the same. Both weigh the same path alike, so the weights sum to one. A path that
  crosses no volume keeps its density as before.

Background: [ADR-0243](../adr/offline/0243-offline-nested-media-are-resolved-by-instance-priority.md),
[ADR-0244](../adr/offline/0244-offline-media-are-tracked-by-null-scattering-with-spectral-mis.md),
[ADR-0245](../adr/offline/0245-offline-grids-are-held-as-nanovdb-on-every-device.md)

## Grids

A volume's fields are grids of floats, which kernel code reads on every device.

- **A grid is a NanoVDB buffer of floats.** The host reads it from its OpenVDB file and converts it
  with NanoVDB's own tools, `volume::read()`, which keep each node's least and most. Each buffer
  is held as `kernel::volume::Block`s, runs of NanoVDB's 32-byte alignment, and every grid's
  follows the one before in one array.
- **A lookup is trilinear**, `volume::valueAt()`, between the eight voxels from a point's floor to
  one past it, in the grid's index space.
- **A cell bounds the lookups in it**, `volume::cellAt()`, as
  [tracking through volumes](#tracking-through-volumes) says.
- **A grid is read once while it is kept.** `geometry::Cache` keeps each field's grid, and reads
  it again only when its field changed. A grid's majorants come from its nodes' least and most,
  so a kept grid keeps them.
- **`volume::Volumes` can be kept across versions.** `update(device, prototypes)` lays the
  blocks out again only when the prototypes' grids are not the ones laid out before, in the same
  order. `laidOut()` says whether the last update did.

Background: [ADR-0245](../adr/offline/0245-offline-grids-are-held-as-nanovdb-on-every-device.md)

## Emission in media

A medium that emits adds a radiance over each unit of length, `Medium::emission`, in ACEScg,
which a path reads as an illuminant's spectrum at its four wavelengths. A path finds it by
colliding in the medium as it tracks through, and by a light sample of a volume light, as
[volume lights](#volume-lights) say.

- **A path gathers emission at every collision**, `glowAt()`, as pbrt-v4 gathers it: real or
  null, before the collision scatters, absorbs or lets the path go on. It gathers its throughput
  times the majorant's transmittance since the last collision times the emission, over the
  average of its chances times that transmittance times the majorant. A homogeneous medium's
  majorant is its extinction. Each source is gathered apart: the homogeneous medium the path is
  in, and each volume it crosses.
- **A medium that neither absorbs nor scatters at the tracked wavelength is never collided in**,
  so no path gathers what it emits there.
- **A light sample of a volume light** draws a point in it and sends a shadow ray to the point.
  It gathers the emission there times what passes, as at a shading point or a point in a medium.
  A closed surface's light emits at the point only where the shadow ray ends in that surface's
  medium, as the medium a path would be in there.
- **The two are weighed by the power heuristic.** A light sample's point has a density by solid
  angle and by length. A path that collides at the same point does so with its own density, times
  the majorant there and the majorant's transmittance since its last collision. That is over a
  shadow ray's density for the volumes on the way, as for the null collisions before it. A light
  sample's weight takes that from `passing()`'s `colliding`: the chance a tracked path would have
  passed the media, times the majorant at the point. A collision's weight takes the chance of the
  light, times its brick's, times the point's density, drawn from where the path last scattered,
  of the set of linked lights there.
- **A point in no brick of its light is found by collisions alone**, and they gather all of it. A
  camera ray, a ray after a specular bounce and every ray under `bsdf` gather all of what they
  collide in, and a ray under `light` gathers none of a volume light's emission.
- **Each collision that gathers records the event of an emitting object**, after the path's
  events so far, labelled as its light is.

Background: [ADR-0244](../adr/offline/0244-offline-media-are-tracked-by-null-scattering-with-spectral-mis.md),
[ADR-0248](../adr/offline/0248-offline-an-emitting-volume-is-a-subtree-of-the-light-tree.md)

## Volume lights

A volume light emits from the points of a medium, along every direction. `light::Lights` makes a
`kernel::light::Light` of shape `Volume` for each chain of instances that places a volume or a
closed surface that emits, after the mesh lights. What it emits at a point is the medium's
emission there, a radiance added over each unit of length.

- **A volume whose program emits makes one.** Its bricks are the leaf nodes of its walked field's
  grid, each eight voxels a side, and each voxel of a brick is the corner of a cell one voxel a
  side. An active tile of a lower node is a brick too, and an active tile of an upper node is cut
  into bricks of a leaf's size. A tile of the root makes none, and its points are found by
  collisions alone. The host runs the volume's program at the centre of each cell, with each field
  looked up there, as the volume stands when the shutter opens. Each cell is weighed by its
  emission's luminance there. A brick whose cells emit nothing is left out, and a volume none of
  whose bricks emits makes no light. A volume of no volume program makes none.
- **A closed surface whose material encloses a medium that emits makes one** for each chain of
  instances that places it. Its one brick is its geometry's box over the shutter, a mesh's or a
  quadric's. A point of the box that lies outside the surface emits nothing.
- **A light's own space** is its grid's index space for a volume, which its `transform` takes
  into the volume's own space, and its geometry's own space for a closed surface. Its chain takes
  that into the world.
- **It is an emitting object**, in no light group, as an emitting material's light is. It lights
  everything and every instance casts its shadows. Camera rays see it as far as its volume's
  visibility lets them.
- **Its power** is four pi times its emission's luminance summed over its volume in the world as
  the shutter opens. A volume's sums its cells' weights, each times a cell's volume in the world.
  A closed surface's is its emission's luminance times its box's volume in the world.
- **Its bricks are sorted by their lower corners**, z first, then y, then x, so
  `media::brickAt()` finds the brick that holds a point by halving.

`Lights::glowing` holds the light of each volume, by the volume's place among the volumes, and
`enclosing` the lights of the closed surfaces. A path in a closed surface's medium finds its light
by the surface's key. `bricks` holds every light's bricks, from its own `bricks`, and `cells` the
sums of each brick's cells' chances, from the brick's own `cells`.

Background: [ADR-0248](../adr/offline/0248-offline-an-emitting-volume-is-a-subtree-of-the-light-tree.md)
