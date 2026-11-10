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
gathered so far. It also holds the wavelengths, the pixel filter's weight, the count of bounces
in all and of each kind, and the last BSDF sample's density.

Background: [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md)

## Where a path draws its numbers

Each draw takes its dimension of the sampler from a fixed place, in `kernel/integrate/Dimensions.h`.

- **The camera's draws come first**: the pixel filter's offset, the point on the lens, the time
  and the wavelengths.
- **Each bounce has a block of `kBounceDraws` dimensions**, and each draw has a fixed place in it:
  the BSDF's direction, then Russian roulette. The rest of the block is kept for a light's draws.
- **`bounceDimension(bounce, place)`** gives a draw's dimension. Any stage then draws its numbers
  from the pixel, the sample, the bounce and the place alone.

## The stages

`kernel::integrate::paths` is the kernel function a launch runs for each pixel. For each sample of
the pass, from `first`, it runs the stages back to back on one thread, and adds the sample to the
pixel in index order.

- **Generate** draws the sample's place in the pixel from the pixel filter, with the filter's
  weight, and its point on the lens, its time and its wavelengths. Its ray is the camera's.
- **Intersect** traces the ray through the traversal the launch was built with.
- **Shade** turns the hit into a shading point. A path whose ray leaves the scene gathers the
  environment: the options' colour as an illuminant, times its throughput. A path whose ray meets
  a light's shape gathers the radiance the light sends back along it, times its throughput, and
  ends there.
- **Scatter** draws a direction from the BSDF at the shading point. The throughput is multiplied by
  the BSDF's value and the cosine to the shading normal, over the density. The ray that leaves
  starts from the shading point's leaving point when it leaves on the geometric normal's side,
  offset off the surface as [Traversal.md](Traversal.md#leaving-a-surface) says. It is an
  indirect ray.
- **Accumulate** adds the light the path gathered to its pixel, with the filter's weight.

A path ends when it leaves the scene, when it meets a light, at a depth limit, when the BSDF
refuses a sample, or by Russian roulette. A path finds a light only by the direction its BSDF
draws.

## Depth limits and Russian roulette

The scene's options set the integrator's `Settings`, which `integrate::settingsOf()` reads.

- **`maxDepth`** is the most bounces a path makes, twelve by default. A path at its limit gathers
  nothing more where it next meets a surface. A limit of zero shows the environment where camera
  rays miss, and nothing where they meet a surface.
- **`diffuseDepth`, `glossyDepth`, `specularDepth` and `transmissionDepth`** each limit one kind of
  bounce: four diffuse and four glossy by default, and twelve of the others. A bounce of a kind
  at its limit ends the path.
- **`rouletteDepth`** is the bounce from which Russian roulette may end a path, the third by
  default. A path survives with the chance of its largest throughput, held to one, and a
  survivor's throughput is divided by that chance, so the picture's mean is unchanged.
- **A depth below zero is taken as zero.**
- **`environment`** is the radiance a path that leaves the scene meets, black by default. It is
  converted into ACEScg from the space its colour names.

## Rendering a scene

`integrate::render(device, scene, logger, config)` renders a scene through the integrator on the
CPU, through the hierarchy, and gives the film's snapshot.

- **The integrator runs through the CPU's library for its instruction set**, as its `paths` entry
  point, since only those libraries define the CPU's traversal.
- **Each pixel takes the samples the options name**, rounded up to a power of two, in passes of
  at most `kSamplesPerPass`. The options' pixel filter and frame are used.
- **The scene's lights with a shape** are lit, through `light::Lights`.
- **The picture is the same on any thread count** and however the samples are split into passes,
  since each pixel's samples are added in index order.
- **`moya --paths`** reads a RIB file through the scene client and writes the picture as an EXR
  file. [CommandLine.md](../manual/moya/CommandLine.md) lists its options.
