# The integrator

This page is for anyone changing how moya builds paths and gathers light. The kernel
side is in `moya/libmoya/kernel/integrate/`, in the namespace `v3d::moya::kernel::integrate`. The
host side is `integrate::render()`, in `moya/libmoya/integrate/`. [Surfaces.md](Surfaces.md)
covers the shading point, [Materials.md](Materials.md) the BSDF a path scatters by, and
[Lights.md](Lights.md) the lights a path meets.

## The path state

A path's state is what one stage hands the next. Its fields are declared once, in
`kernel/integrate/PathState.h`, as `MOYA_PATH_FIELDS`, a list of each field's type and name.

- **Each device lays the state out from the list.** The CPU makes it one structure per path,
  `PathState`, on the thread that runs the path. A GPU makes it one array per field.
- **Stage code reaches a field through `MOYA_PATH(state, field)`**, which each layout defines, so
  the same stage reads either.
- **A field is added to the list**, and every layout gains it.

The state holds the pixel and the sample, the ray traced next and its cone, the throughput and
the light gathered so far. It also holds the wavelengths, the pixel filter's weight, and the
count of bounces in all and of each kind, a scattering event in a medium being a volume bounce.
Its subsurfaceBounces count the events of its walks below surfaces. The count of bounces in all
holds them too, and no other depth limit counts them.
Its chances are the densities with which it would have been tracked through media at each of its
wavelengths, as [tracking through a medium](#tracking-through-a-medium) says. Its lightChances
are those with which a shadow ray would have tracked it through volumes, as
[tracking through volumes](#tracking-through-volumes) says. Last, it holds the
last BSDF or phase function sample's density and whether it was specular, and the point it was
drawn at with its shading normal and its set of linked lights. Its coverage is one once the
camera ray has met something, as [Coverage](#coverage) says. Its media are the closed surfaces
it is inside, as [nested media](#nested-media) says.

Background: [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md)

## Where a path draws its numbers

Each draw takes its dimension of the sampler from a fixed place, in `kernel/integrate/Dimensions.h`.

- **The camera's draws come first**: the pixel filter's offset, the point on the lens, the time
  and the wavelengths.
- **Each bounce has a block of `kBounceDraws` dimensions**, and each draw has a fixed place in it.
  The draws are the BSDF's direction, Russian roulette, the light a light sample draws, the point
  on it, a mesh light's triangle, and the BSDF's choice between reflecting and transmitting. The
  last decides what the bounce's ray passes through where surfaces are cut out, and the camera's
  ray takes the first bounce's.
- **`bounceDimension(bounce, place)`** gives a draw's dimension. Any stage then draws its numbers
  from the pixel, the sample, the bounce and the place alone.
- **A scattering event in a medium draws from its bounce's block** as a shading point does. The
  phase function's direction takes the BSDF's direction, and its lobe the BSDF's choice. Its
  light sample and Russian roulette take their own places.
- **Each event of a walk below a surface draws from a block of its own**, since the count of
  bounces in all counts it. A walk's events take the blocks after the bounce that entered the
  surface, so a walk of any length draws numbers apart.
- **A collision in a medium takes its numbers from a hash**, `collisionDraw()`, of the pixel in
  the image, the sample, the bounce and the collision's count within the bounce. A path meets a
  count of collisions no block could hold a place for. The numbers are the same on every run, but
  they do not gain from the sampler's stratification.

## The stages

`kernel::integrate::paths` is the kernel function a launch runs for each pixel. For each sample of
the pass, from `first`, it runs the stages back to back on one thread, and adds the sample to the
pixel in index order.

- **Generate** draws the sample's place in the pixel from the pixel filter, with the filter's
  weight, and its point on the lens, its time and its wavelengths. Its ray and its cone are the
  camera's.
- **Intersect** traces the ray through the traversal the launch was built with, passing through
  what is cut out as [Surfaces that are cut out](#surfaces-that-are-cut-out) says. A path in a
  medium is then tracked through it up to the hit, as
  [tracking through a medium](#tracking-through-a-medium) says, and may scatter or end before it.
- **Shade** turns the hit into a shading point, with the footprint the path's cone makes there.
  A path whose ray leaves the scene gathers its share of the dome lights and the distant lights
  it meets, as [Lights](#lights) says, times its throughput. A path whose ray meets a light's
  shape gathers its share of the radiance the light sends back along it, times its throughput,
  and ends there. A path whose ray meets a geometry a mesh light makes emit gathers its share of
  what the light emits, and goes on to scatter there. At a shading point, a path gathers a light
  sample, as [Lights](#lights) says.
- **Scatter** draws a direction from the BSDF at the shading point. The throughput is multiplied by
  the sample's projected value over its density, both of the closure that drew it alone, as
  [Materials.md](Materials.md) says. A specular sample's holds no cosine, and the
  path marks the sample specular. The ray that leaves
  starts from the shading point's leaving point when it leaves on the geometric normal's side,
  and from the shading point on the other side, offset off the surface as
  [Traversal.md](Traversal.md#leaving-a-surface) says. It is an indirect ray, and its cone is the
  one [A ray's footprint](#a-rays-footprint) says leaves.
- **Accumulate** adds the light the path gathered to its pixel, with the filter's weight, and its
  coverage to the pixel's alpha.

A path ends when it leaves the scene, when it meets a light, at a depth limit, when the BSDF
refuses a sample, when a medium absorbs it, or by Russian roulette.

## A ray's footprint

A path carries a cone, `geometry::Cone`: how wide the area its camera sample covers is where its
ray starts, and how much wider it grows over each unit of distance along the ray. A texture
lookup filters over the area the cone covers where it meets a surface.

- **A pinhole camera's cone** starts with no width, and spreads by a pixel's width over each unit
  of depth. A pixel's width is the screen window's width over the image's width in pixels, at a
  depth of one. A plane square to the camera therefore takes a footprint a pixel wide at every
  pixel.
- **A lens's cone** starts as wide as the lens, and narrows to a pixel's width at the depth in
  focus. It widens again beyond, as a defocused point's blur does.
- **An orthographic camera's cone** is a pixel wide and does not spread.
- **At a hit** the cone's width there is projected onto the plane of the shading normal, as an
  ellipse. Its minor axis lies across the ray and is the cone's width. Its major axis lies along
  the ray as the surface sees it, and is the width over the cosine between the ray and the normal.
  The cosine is held to at least `kLeastCosine`, so a grazing footprint is at most a hundred
  times longer than wide.
- **The cone that leaves a bounce** starts as wide as the footprint. Its spread grows by the
  arctangent of the sampled lobe's roughness, a tuned rule, so a diffuse bounce adds an eighth of
  a turn. A reflection's spread also grows by twice the surface's curvature times the width, so a
  convex surface spreads it and a concave one focuses it. A transmission keeps the curvature out.
- **A lobe's roughness** is its GGX alpha, or the geometric mean of an anisotropic lobe's two. It
  is one for a diffuse, Oren-Nayar or fuzz lobe, and zero for a specular sample.

The cone is round, so a pixel stretched along one axis gains its stretch only where the cone is
projected. A shading point holds its footprint in the world, and
[Surfaces.md](Surfaces.md#the-footprint) says how it is given in texture coordinates.

Background: [ADR-0224](../adr/offline/0224-offline-a-rays-footprint-is-a-cone.md)

## Lights

A path finds a light in two ways: by a light sample at each shading point, and by the direction its
BSDF draws meeting the light. A light with a shape is met where a ray hits its shape, and a mesh
light where a ray hits its geometry from the side it emits from. A distant light with an angle is
met where a ray that leaves the scene lies inside its cone. The options'
`lightSampling` chooses between the two ways.

- **A light sample** draws one light that lights the shading point, as
  [Lights.md](Lights.md#light-linking) says. It draws by the light tree or in proportion to its
  power as the options' `lightPicking` says, then a point on it, as
  [Lights.md](Lights.md#drawing-a-light) and [Lights.md](Lights.md#sampling-a-light) say. The tree
  draws by the shading point and its shading normal, which the path keeps for a BSDF sample that
  meets a light. A shadow ray from the shading point towards the point says how much of the light
  passes what lies between. It stops short of the point by `kShadowShort` of its length, and a
  light's shape never stops one. A shadow ray towards a distant light runs on past everything in the
  scene. The sample gathers the BSDF's projected value times what the light sends to the shading
  point, over the chance of the light times the point's density.
- **A point and a distant light of no angle are found by light samples alone**, since no
  direction a BSDF draws meets them. A light whose shadow link leaves an instance out is found by
  light samples alone too. A light sample of any of them takes its whole weight whatever the
  strategy, and is drawn under `bsdf` too.
- **A BSDF sample that meets a light gathers nothing of a light that does not light the shading
  point it left**, which the path keeps the set of linked lights of.
- **A specular sample that meets a light gathers all of it**, whatever the strategy, since no
  light sample could have drawn its direction. A light sample at a shading point of singular
  closures alone gathers nothing, since they have no value for its direction.
- **`mis`**, the fallback, weighs the two by multiple importance sampling with the power
  heuristic, closure by closure. A light sample gathers each closure's part apart, weighed by the
  light's density against that closure's density for its direction. A BSDF sample that meets a
  light is weighed by the density of the closure that drew it. That is weighed against the chance
  of the light times the light's density for the point it met, seen from the shading point it
  left.
- **`light`** finds the lights by light samples alone, and a BSDF sample that meets a light
  gathers nothing.
- **`bsdf`** finds them by BSDF samples alone. A light sample gathers nothing but the light of a
  point or a distant light of no angle.
- **A camera ray that meets a light's shape gathers all of it**, whatever the strategy, since
  nothing else samples the light for it. A camera ray sees no distant light, and sees what a mesh
  light emits only while its `camera` is 1.
- **A mesh light's triangle is part of its density.** A light sample of a mesh light, and a BSDF
  sample that meets one, are weighed by the chance of the light, times the chance of the
  triangle, times the point's density on the triangle.
- **A bounce that a depth limit refuses draws no light sample.** A shading point at `maxDepth`
  bounces, or at the diffuse limit, draws none, since the BSDF's sample there would end the path.
- **A dome light is met by every ray that leaves the scene**, and camera rays see it. Its light
  sample is drawn by its image's compensated way under `mis`, and by its other way under
  `light`, as [Lights.md](Lights.md#dome-lights) says, so each strategy draws by the density it
  weighs by.
- **A volume light is found by a light sample, or where a path collides in its medium**, as
  [Emission in media](#emission-in-media) says.

## Surfaces that are cut out

A surface whose material is cut out lets a ray through where its opacity is less than one, as
OpenPBR's `geometry_opacity` says. Each device's traversal asks the integrator's filter about a
hit on such a surface, as [Traversal.md](Traversal.md#the-traversal-a-stage-takes) says, so a ray
passes any number of them in one traversal, with no limit on how many.

- **The opacity at a hit** is what the material's program gives there, read with its images at
  their finest level, held to between zero and one. A material whose program does not give it
  holds a constant one. A surface whose material is not cut out is opaque. `opacityAt()` gives it.
- **A camera or an indirect ray keeps a hit with a chance of its opacity**, and passes through
  it otherwise. `Keeping` decides by a number hashed from the bounce's own draw, the primitive and
  its chain of instances. Each surface along a ray takes a number apart from the others, and the
  picture is the same on every run. A kept hit is shaded in full, so on average the surface shows
  in the share its opacity gives.
- **A shadow ray carries one minus the opacity of each surface it meets.** `Passing` multiplies
  them, and stops the traversal once nothing passes. The light sample gathers what passes, so a
  shadow through a half-opaque surface is smooth rather than drawn at random.
- **A light with a shadow link** passes through what casts none of its shadows, as
  [Lights.md](Lights.md#light-linking) says, and carries one minus the opacity of each surface
  that does. A ray from such a light meets each surface by a traversal of its own.

Background: [ADR-0227](../adr/offline/0227-offline-opacity-is-tested-inside-the-traversal.md)

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

## Emission in media

A medium that emits adds a radiance over each unit of length, `Medium::emission`, in ACEScg,
which a path reads as an illuminant's spectrum at its four wavelengths. A path finds it by
colliding in the medium as it tracks through, and by a light sample of a volume light, as
[Lights.md](Lights.md#volume-lights) says.

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

## Walking a subsurface

A path that enters a surface through its subsurface's closure walks the medium below it, as
[Materials.md](Materials.md#the-medium-below-a-subsurface) gives it, until it leaves the surface
again. The walk is tracking through a medium, as [the section above](#tracking-through-a-medium)
says, with these differences.

- **The medium is the subsurface's**, not the one the material's transmission gives, while the
  winning surface is one the path walks.
- **A scattering event takes no light sample** and no Russian roulette. Its direction counts as
  specular for a light the path meets next, since no light sample could have drawn it.
- **The walk ends where it meets its surface from inside.** `bsdf::Layering::walk()` gives the
  surface there as a white Lambertian lobe that goes through it, whatever the surface's
  roughness and Fresnel factor. Its weight is the square of the subsurface's index relative to
  the medium beyond the surface. A refraction into the surface divides radiance by that square,
  so the lobe gives it back, and a white walk keeps a furnace's light. Its light sample is taken
  as at any diffuse bounce, and its sample leaves the surface as a transmission, which counts
  against the depth limits as any bounce does. The light that enters still passes the
  subsurface's dielectric, so the walk is not reciprocal.
- **Each scattering event of the walk counts against `subsurfaceDepth`**, and against no other
  limit. A walk at its limit is absorbed.

Background: [ADR-0246](../adr/offline/0246-offline-subsurface-is-a-random-walk-inside-the-surface.md),
[ADR-0252](../adr/offline/0252-offline-a-walk-leaves-its-surface-by-a-lambertian-lobe.md)

## Depth limits and Russian roulette

The scene's options set the integrator's `Settings`, which `integrate::settingsOf()` reads.

- **`maxDepth`** is the most bounces a path makes, twelve by default. A path at its limit gathers
  nothing more where it next meets a surface. A limit of zero shows the dome lights where camera
  rays miss, and nothing where they meet a surface.
- **`diffuseDepth`, `glossyDepth`, `specularDepth` and `transmissionDepth`** each limit one kind of
  bounce: four diffuse and four glossy by default, and twelve of the others. A bounce of a kind
  at its limit ends the path. A bounce that crosses the surface is a transmission, smooth or
  rough. One that stays on its side is specular from a singular closure, glossy from a
  microfacet lobe, and diffuse from a diffuse one.
- **`volumeDepth`** is the most scattering events a path makes in media, four by default. A
  scattering event counts against `maxDepth` too. One a limit refuses ends the path there,
  before its light sample. Cycles' limit of zero lets a path scatter once in a medium, and moya
  counts that event, so its default is four rather than Cycles' zero.
- **`subsurfaceDepth`** is the most events a walk below a surface makes, 256 by default, as
  Cycles' walk makes. A walk's events count against no other limit, and `maxDepth` and
  `rouletteDepth` count the bounces of a path but those events.
- **`rouletteDepth`** is the bounce from which Russian roulette may end a path, the third by
  default. A scattering event in a medium counts as any bounce. A path survives with the chance
  of its largest throughput, held to one, and a
  survivor's throughput is divided by that chance, so the picture's mean is unchanged.
- **A depth below zero is taken as zero.**

## Rendering a scene

`integrate::render(device, scene, logger, config, checkpoints)` renders a scene through the
integrator on the CPU, through the hierarchy, and gives the film's snapshot. Its overload that
takes `geometry::Prototypes` renders those in place of gathering them from the scene, so a case
can change what was gathered, such as a material's closures, before it renders.

- **The integrator runs through the CPU's library for its instruction set**, as its `paths` entry
  point, since only those libraries define the CPU's traversal.
- **Each pixel takes the samples the options name**, rounded up to a power of two, in passes of
  at most `kSamplesPerPass`. The options' pixel filter and frame are used.
- **The scene's lights** are lit, through `light::Lights`, with the world's reach from the box
  that bounds its hierarchy.
- **Adaptive sampling** stops each pixel at the options' `noiseThreshold`, as
  [the next section](#adaptive-sampling) says.
- **A channel that reads the first hit** is filled where each camera ray first meets the scene,
  past any surface it passes as cut out, as [the next section](#values-at-the-first-hit) says.
- **The picture is the same on any thread count** and however the samples are split into passes,
  since each pixel's samples are added in index order.
- **The options' `crop` renders a window of the image.** Its pixels are those RI's crop window
  names: on each axis, from `ceil(side * least)` up to and not including `ceil(side * most)`. The
  film holds the window's pixels alone. Each draws the samples of its place in the image, and the
  camera sees it there, so a pixel of a crop is the same, bit for bit, as that pixel of the whole
  image. The snapshot names the image and the window's place in it. A window of no pixels is
  logged, and the whole image is rendered.
- **`moya`** reads a RIB file through the scene client and writes each of the scene's
  outputs, as [Output.md](Output.md#a-scenes-outputs) says.
  [CommandLine.md](../manual/moya/CommandLine.md) lists its options.

## Coverage

A camera sample's coverage is what the beauty's alpha adds. It is one once the camera ray meets a
surface, a light's shape, or a collision in a medium that scatters or absorbs. A camera ray that
leaves the scene meeting nothing covers its sample by one less the mean of its throughput over
its wavelengths. Tracking weighs that throughput by what each wavelength passed, so the expected
alpha is one less the mean transmittance along the ray, and a grey fog over nothing composites
over a plate. A camera ray that passes a cut-out surface or a medium's boundary covers nothing
there.

Background: [ADR-0251](../adr/offline/0251-offline-alpha-through-a-medium-is-its-mean-transmittance.md)

## Values at the first hit

A raw channel names a value by its `sourceName`, which a camera ray reads where it first meets
the scene. Each is added to its channel by the channel's combine, as
[Film.md](Film.md#the-films-pixels) says. A closest channel keeps the sample of the least depth.

| Name | Components | Value | Where the ray meets nothing |
|---|---|---|---|
| `depth` | 1 | the hit's depth along the camera's view | infinity |
| `position` | 3 | the hit's point in the world | zero |
| `normal` | 3 | the shading normal in the world, on the geometric normal's side | zero |
| `st` | 2 | the `st` primvar, or the hit's place on its primitive without one | zero |
| `instance` | 1 | the innermost instance that placed the hit, or -1 for none | -1 |
| `face` | 1 | the face or curve of the mesh the hit's primitive was cut from | -1 |
| `motion` | 2 | how far the hit moves across the picture while the shutter is open, in pixels | zero |

- **A light's shape the ray meets** has its depth and position, a zero normal, `st` and motion,
  and an instance and a face of -1.
- **A collision in a medium is a first hit** when it comes before any surface: one that scatters
  or absorbs, not a null collision. Its depth and position are the collision's. Its instance is
  the volume's innermost instance, or that of the closed surface whose medium it is. Its normal,
  `st` and motion are zero, its face -1, and a primvar reads nothing. Tracking picks the medium a
  collision names with the chance its extinction at the tracked wavelength is of the whole there,
  the closed surface's medium first and then each volume.
- **A motion** is the hit's raster position as the shutter closes less its position as it opens,
  with y down the picture. The hit is found again at each time, as its geometry and its instances
  stand then. A triangle's hit follows its corners. A point's or a curve's follows its primitive's
  first point or control. A quadric's own motion is not followed.
- **A primvar channel** reads the primvar its `sourceName` names, of the channel's components,
  and zero on a surface without it. The gather reads the primvar for the channel, though no
  material reads it.
- **A denoiser's `albedo` and `denoisingNormal`**, of three components each, are read where
  the path first meets a surface whose singular closures hold less than half of its estimated
  albedo. A path passes mirrors and smooth glass by, and the albedo it then gives is the
  surface's times what they let through. It gives nothing once it leaves a surface through a lobe
  that is not singular, or meets no surface. The albedo is that of the surface's closures that
  are not singular, under ACES white, so a white surface's is one. The normal is the surface's
  shading normal.
- **A collision in a medium gives a denoiser its albedo** when the path has given none yet: the
  medium's scattering over its extinction there, times the path's throughput, under ACES white.
  Its normal points back along the ray. The path gives nothing past it.
- **Any other raw or primvar channel holds zero, and is reported**: a raw name not in the
  table, and a raw value of other components than its channel's. A light path expression's
  channel is filled as [Compositing.md](Compositing.md#channels-of-light-path-expressions)
  says.

Background: [ADR-0249](../adr/offline/0249-offline-a-collision-in-a-medium-is-a-first-hit.md)

## Light path expressions

A path reports each of its events, and a channel may take the light of the paths a light path
expression matches. Both, with light groups, are in [Compositing.md](Compositing.md).

## Adaptive sampling

A render stops each pixel once its noise falls below the options' `noiseThreshold`. The samples
the options name are the most a pixel takes. A threshold of zero, the fallback, samples every
pixel to the most. [Film.md](Film.md#stopping-a-pixel) says how a pixel's noise is measured.

- **The threshold is in the working space**, ACEScg, so it means the same under every view.
- **A pixel is checked only between passes**, from the samples of the passes before. The same
  pixels then stop on every run and on any thread count.
- **The first check comes once a pixel holds `minSamples`**, 16 unless the options give
  another, so both halves hold a few samples before they are compared. A check comes before each
  pass from then on.
- **A render ends early** when a check leaves no pixel sampling.
- **RI's `PixelVariance`** sets the threshold, as RenderMan's path tracer reads it.

Background: [ADR-0215](../adr/offline/0215-offline-a-pixels-noise-is-the-difference-of-two-halves.md)

## Checkpoint and resume

`render()` takes an `integrate::Checkpoints` as its last argument. With its `path` empty, the
fallback, a render keeps no checkpoint.

- **A checkpoint is written after each pass**, or after the first pass once `interval` seconds
  have passed since the last write. The last pass a render runs is always written.
  [Film.md](Film.md#checkpoints) describes the file.
- **`resume` continues the render the checkpoint holds**, at the pass after its last. Its
  picture is the same, byte for byte, as that of a render that never stopped, on any thread
  count. A pixel that adaptive sampling stopped stays stopped, and the noise check before the
  next pass is made again from the same sums.
- **A checkpoint of another scene or film is refused** with `output::Refused`, by its
  fingerprint. The sampler's samples depend on the samples per pixel, so a render resumed to more
  samples than it began with is another render, and is refused too.
- **`stop` is asked before each pass** with the passes the film holds. A render it answers true
  for writes its checkpoint and returns the snapshot of what it holds.

## Profiling a render

`render()` fills an `integrate::Profile` when it is given one, for a benchmark.

- **`seconds` is always measured**: the time the passes took, summed, without the noise checks
  and checkpoints between them.
- **`coverage` and `deep` are always measured too.** They are the seconds the host took after
  each pass, summed, to merge its samples' Cryptomatte coverage and to join their deep lists.
  Each is part of `seconds`, and each is zero for a render with neither output.
- **A counted render**, with `counted` true, runs the CPU library's `profiled` entry point in
  place of `paths`. It wraps the traversal in `device::cpu::Profiled`, which counts each ray into
  its pixel's `device::cpu::Tally` and reads the processor's time-stamp counter around it, and the
  whole pixel's time around that. Each pixel has a tally of its own, so no two threads write one.
- **The textures are wrapped too**, in `device::cpu::ProfiledTextures`. The kernel tells a texture
  access when a material's program and a texture lookup start and end, through
  `kernel::texture::started()` and `finished()`. Only a texture access that defines both is told,
  so an ordinary render reads no counter.
- **The counts are the rays traced** for their nearest hit, `nearest`, and for whether they meet
  anything, `any`. `traversal` is the share of every thread's time in the passes that traversal
  took, from 0 to 1. `programs` and `lookups` are the shares that materials' programs and
  texture lookups took. A program's share holds the lookups it makes.
- **A counted render gives the same picture** as one that is not counted. Reading the counter
  around every ray makes it slower, so its `seconds` is not the render's speed. A benchmark times
  one render and counts another.

`moya_bench` times the integrator this way over a lit room, a textured one and one with outputs, as
[Traversal.md](Traversal.md#speed) describes.
