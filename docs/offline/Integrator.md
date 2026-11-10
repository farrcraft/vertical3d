# The integrator

This page is for anyone changing how moya's new core builds paths and gathers light. The kernel
side is in `moya/libmoya/kernel/integrate/`, in the namespace `v3d::moya::kernel::integrate`. The
host side is `integrate::render()`, in `moya/libmoya/integrate/`. [Surfaces.md](Surfaces.md)
covers the shading point and the BSDF a path scatters by, and [Lights.md](Lights.md) the lights a
path meets.

## The path state

A path's state is what one stage hands the next. Its fields are declared once, in
`kernel/integrate/PathState.h`, as `MOYA_PATH_FIELDS`, a list of each field's type and name.

- **Each device lays the state out from the list.** The CPU makes it one structure per path,
  `PathState`, on the thread that runs the path. A GPU makes it one array per field.
- **Stage code reaches a field through `MOYA_PATH(state, field)`**, which each layout defines, so
  the same stage reads either.
- **A field is added to the list**, and every layout gains it.

The state holds the pixel and the sample, the ray traced next, the throughput and the light
gathered so far. It also holds the wavelengths, the pixel filter's weight, and the count of
bounces in all and of each kind. Last, it holds the last BSDF sample's density, and the shading
point it was drawn at with its shading normal.

Background: [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md)

## Where a path draws its numbers

Each draw takes its dimension of the sampler from a fixed place, in `kernel/integrate/Dimensions.h`.

- **The camera's draws come first**: the pixel filter's offset, the point on the lens, the time
  and the wavelengths.
- **Each bounce has a block of `kBounceDraws` dimensions**, and each draw has a fixed place in it.
  The draws are the BSDF's direction, Russian roulette, the light a light sample draws, the point
  on it, and a mesh light's triangle. The rest of the block is kept for draws to come.
- **`bounceDimension(bounce, place)`** gives a draw's dimension. Any stage then draws its numbers
  from the pixel, the sample, the bounce and the place alone.

## The stages

`kernel::integrate::paths` is the kernel function a launch runs for each pixel. For each sample of
the pass, from `first`, it runs the stages back to back on one thread, and adds the sample to the
pixel in index order.

- **Generate** draws the sample's place in the pixel from the pixel filter, with the filter's
  weight, and its point on the lens, its time and its wavelengths. Its ray is the camera's.
- **Intersect** traces the ray through the traversal the launch was built with.
- **Shade** turns the hit into a shading point. A path whose ray leaves the scene gathers its share
  of the dome lights and the distant lights it meets, as [Lights](#lights) says, times its
  throughput. A path whose ray meets a light's shape gathers its share of the radiance the light
  sends back along it, times its throughput, and ends there. A path whose ray meets a geometry a
  mesh light makes emit gathers its share of what the light emits, and goes on to scatter there. At
  a shading point, a path gathers a light sample, as [Lights](#lights) says.
- **Scatter** draws a direction from the BSDF at the shading point. The throughput is multiplied by
  the BSDF's value and the cosine to the shading normal, over the density. The ray that leaves
  starts from the shading point's leaving point when it leaves on the geometric normal's side,
  offset off the surface as [Traversal.md](Traversal.md#leaving-a-surface) says. It is an
  indirect ray.
- **Accumulate** adds the light the path gathered to its pixel, with the filter's weight.

A path ends when it leaves the scene, when it meets a light, at a depth limit, when the BSDF
refuses a sample, or by Russian roulette.

## Lights

A path finds a light in two ways: by a light sample at each shading point, and by the direction its
BSDF draws meeting the light. A light with a shape is met where a ray hits its shape, and a mesh
light where a ray hits its geometry from the side it emits from. A distant light with an angle is
met where a ray that leaves the scene lies inside its cone. The options'
`lightSampling` chooses between the two ways.

- **A light sample** draws one light, by the light tree or in proportion to its power as the
  options' `lightPicking` says, then a point on it, as [Lights.md](Lights.md#drawing-a-light)
  and [Lights.md](Lights.md#sampling-a-light) say. The tree draws by the shading point and its
  shading normal, which the path keeps for a BSDF sample that meets a light. A shadow ray from
  the shading point towards the point says whether anything lies between. It stops short of the
  point by `kShadowShort` of its length, and a light's shape never stops one. A shadow ray
  towards a distant light runs on past everything in the scene. The sample gathers the BSDF's
  value times what the light sends to the shading point and the cosine to the shading normal,
  over the chance of the light times the point's density.
- **A point and a distant light of no angle are found by light samples alone**, since no
  direction a BSDF draws meets them. A light sample of either takes its whole weight whatever the
  strategy, and is drawn under `bsdf` too.
- **`mis`**, the fallback, weighs the two by multiple importance sampling with the power
  heuristic. A light sample is weighed by its density against the BSDF's density for its
  direction. A BSDF sample that meets a light is weighed by its density against the chance of the
  light times the light's density for the point it met, seen from the shading point it left.
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

## Depth limits and Russian roulette

The scene's options set the integrator's `Settings`, which `integrate::settingsOf()` reads.

- **`maxDepth`** is the most bounces a path makes, twelve by default. A path at its limit gathers
  nothing more where it next meets a surface. A limit of zero shows the dome lights where camera
  rays miss, and nothing where they meet a surface.
- **`diffuseDepth`, `glossyDepth`, `specularDepth` and `transmissionDepth`** each limit one kind of
  bounce: four diffuse and four glossy by default, and twelve of the others. A bounce of a kind
  at its limit ends the path.
- **`rouletteDepth`** is the bounce from which Russian roulette may end a path, the third by
  default. A path survives with the chance of its largest throughput, held to one, and a
  survivor's throughput is divided by that chance, so the picture's mean is unchanged.
- **A depth below zero is taken as zero.**

## Rendering a scene

`integrate::render(device, scene, logger, config)` renders a scene through the integrator on the
CPU, through the hierarchy, and gives the film's snapshot.

- **The integrator runs through the CPU's library for its instruction set**, as its `paths` entry
  point, since only those libraries define the CPU's traversal.
- **Each pixel takes the samples the options name**, rounded up to a power of two, in passes of
  at most `kSamplesPerPass`. The options' pixel filter and frame are used.
- **The scene's lights** are lit, through `light::Lights`, with the world's reach from the box
  that bounds its hierarchy.
- **The picture is the same on any thread count** and however the samples are split into passes,
  since each pixel's samples are added in index order.
- **`moya --paths`** reads a RIB file through the scene client and writes the picture as an EXR
  file. [CommandLine.md](../manual/moya/CommandLine.md) lists its options.
