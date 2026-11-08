# Traversal

This page is for anyone writing kernel code that traces rays, or a device's traversal. Rays,
hits and the tests that intersect primitives are kernel code, in `moya/libmoya/kernel/geometry/`
and the namespace `v3d::moya::kernel::geometry`. Every device compiles them. Which primitives a
ray reaches is each device's own traversal. The reyes and ray hiders use none of this.

## Rays

A `Ray` is the points `origin + t * direction` for `t` in `[start, end)`, at one `time` over
the shutter.

- **The direction need not be unit length.** A distance along a ray is measured in its
  direction's lengths, so a ray moved into an instance's space keeps its distances.
- **The interval is open at its end.** A primitive exactly at `end` is not met, so a shadow ray
  to a point on a light does not meet the light.
- **By default a ray starts at zero and has no end.**
- **A ray has a kind**: `kCameraRay`, `kShadowRay` or `kIndirectRay`, one bit each. An instance
  hidden from a ray's kind is not met, and neither is anything it places. A ray is a camera ray
  by default.

## Hits

A `Hit` is as small as every device can return it. HIP RT, OptiX and Embree each hand back the
same fields.

- **`distance`** along the ray, and the **`time`** of the ray.
- **`primitive`**, the index within the geometry the innermost instance places. `kNoPrimitive`
  means the ray met nothing, which `found()` reports.
- **`uv`**, the position on the primitive. For a triangle it is the weight of its second and
  third vertices.
- **`instances`**, the chain of instances that placed the primitive, outermost first, and
  **`depth`**, how many there are. The chain holds at most `kInstanceDepth`, which is 4.

A hit holds no point, normal or differential. The stage that shades it computes them from the
primitive, once, rather than for every hit a nearer one replaces.

## The traversal a stage takes

A stage that traces rays takes its traversal as a template parameter, and each device
instantiates the stages with its own. The concept `Traversal` names what one provides:

- **`nearest(ray)`**: the hit nearest the ray's start within its interval, or a hit that found
  nothing.
- **`any(ray)`**: whether anything lies within the interval. It may stop at the first primitive
  it meets, as a shadow ray needs.

Both are `const`, since a launch traces from every pixel at once. Kernel code names no device,
so a test runs the same stages over any traversal.

Background: [ADR-0204](../adr/offline/0204-offline-stages-take-their-traversal-as-a-type.md)

## Triangles

`intersectTriangle` decides whether a ray meets a triangle within an interval, and where. It
meets either side.

- **The test is watertight**, by Woop, Benthin and Wald's method. A ray through an edge or a
  vertex that triangles share meets at least one of them, so a closed mesh has no gaps.
- **A `Shear` holds the ray made ready for it.** The axis the direction is longest along becomes
  z, and a shear turns the direction into +z. `Shear::of(ray)` is computed once for every
  triangle the ray meets.
- **An edge that rounds to zero is computed again in double precision**, so both triangles that
  share it decide a ray exactly on it the same way.
- **A distance that is not a number is no hit.**

## Leaving a surface

`offsetStart(point, normal)` moves the start of a ray that leaves a surface off it, along the
geometric normal, so the ray does not meet the surface again where it starts. The normal points
to the side the ray leaves by, and need not be unit length.

- **Each coordinate moves by a count of representable floats** in proportion to the normal, so
  the move grows with the point's distance from the origin, as the hit's rounding does.
- **Near the origin, where floats are densest, it moves by a fixed fraction of the normal.**

This is Wächter and Binder's method. A ray that starts at an offset point needs no `start`
above zero.

## Camera rays

`kernel::camera::Camera` is RI's camera as kernel code reads it, looking down +z of its own
space with x to the right and y up. `camera::fromScene(scene)` builds it from the camera the
scene's options name.

- **A raster position maps onto the screen window.** The window spans the image, from its top
  left at the raster's origin. While the camera gives none, a frame at least as wide as it is
  tall spans `-aspect` to `aspect` across and -1 to 1 down. A taller one spans -1 to 1 across and
  `-1 / aspect` to `1 / aspect` down. The aspect is the options' `frameAspect`, or the
  resolution's times the pixel aspect while that is zero.
- **A perspective camera's screen is the plane z = 1**, scaled by the tangent of half the field
  of view. An orthographic camera's is the plane z = 0, in camera space's units.
- **A ray's direction has a z of one in camera space**, so a distance along it is the depth of the
  point it reaches. Its interval is the clipping planes'.
- **Depth of field draws a point on the lens.** The lens's radius is the focal length over twice
  the f-stop, and every ray through one pixel meets at the focal distance. A camera whose
  f-stop, focal length or focal distance is not positive and finite is a pinhole.
- **A ray's time is drawn over the shutter**, from its opening to its close.
- **The camera's transformation is its first sample.**

Options that name no camera take every camera parameter's fallback, which is RI's default:
orthographic, at the origin.

## Gathering the scene

`geometry::gather(scene, logger)` gathers every prototype the scene's instances reach from the
world into a `geometry::Prototypes`, each in its own space, with the instances that place them.

- **A prototype placed many times is held once.** Each mesh's triangles are a run of the
  triangles every mesh holds, three points each, with the face each was cut from.
- **An instance names its prototype**, a mesh or a group, and its transformation into the space
  of the group that holds it. It keeps its index in the scene. Its `camera`, `shadow` and
  `indirect` parameters give the kinds of ray it is visible to.
- **A group comes after every group it holds.** The world is the group of every instance in no
  group. A group's instances, and the world's, follow their indices.
- **Some instances are left out, and each reason is reported once**:
  - an instance of a group that holds the instance's own group, directly or through another
    group. A group that holds itself would otherwise have no bounds.
  - an instance that would be deeper in a chain than a hit records. It is still held where a
    shorter chain reaches it, and each traversal leaves it out where it is too deep.
  - an instance of a prototype of a kind not yet traced. Meshes alone are traced.

  `Prototypes::reports` holds the reports, and the log has them too.
- **An instance whose prototype was removed places nothing**, and one whose group was removed is
  placed by nothing.
- **A transformation is its first sample.**

## Placing the scene

`geometry::flatten(prototypes)` places every triangle the prototypes' instances reach in world
space, in runs, one run for each chain of instances. `geometry::World` holds the result in a
device's memory, and `view()` gives it to kernel code as a `kernel::geometry::Placed`.
`flatten(scene, logger)` gathers the scene and places what it gathers.

- **Each instance in the world places its prototype**, and an instance of a group places every
  instance in the group inside its own transformation. Runs follow the instances' indices.
- **A run is visible to the kinds of ray every instance of its chain is visible to.**
- **A chain deeper than a hit records is left out.**
- **A triangle's primitive is its index within its geometry**, which is its index within its
  run. A hit then names the same primitive whichever traversal found it, and `Placed` gives the
  hit's geometric normal and its mesh's face from the chain and the primitive.

## The brute-force traversal

`kernel::geometry::BruteForce` tests every placed triangle for every ray. It is slow, and it is
the reference every other traversal is compared with. Its `nearest()` keeps the nearest hit
within the ray's interval, and its `any()` stops at the first. A run hidden from the ray's kind
is passed over.

## The hit view

The hit view is a picture of what rays hit, which lights nothing. `view::hits(device, scene,
logger, time, tracer)` traces one camera ray through the centre of each pixel and of the lens, at
a point over the shutter, through the hierarchy unless the brute-force traversal is asked for.
`view::writeHits()` writes it as an EXR file of float channels, and `moya --hits` writes the hit
view of a RIB file.

| Channel | A pixel whose ray hit | A pixel whose ray met nothing |
|---|---|---|
| `depth` | the hit's depth along the camera's view | infinity |
| `normal` | the geometric normal in world space, by its triangle's winding | zero |
| `instance` | the index of the innermost instance | -1 |
| `face` | the face of its mesh | -1 |
| `coverage` | one | zero |

The beauty is black.

## The hierarchy

`accel::build(device, bounds, clip, options)` builds a bounding volume hierarchy over primitives
by their boxes, in host memory, in `moya/libmoya/accel/` and the namespace `v3d::moya::accel`.
`clip` gives the bounds of the part of a primitive inside a box. `accel::triangleClip` does that
for triangles of three points each.

- **Each node is split by the surface area heuristic**, over 32 bins of the centroids along each
  axis. A step costs as much as a test of one primitive.
- **Spatial splits are tried where a node's best halves overlap**, by more than 1e-5 of the root's
  area. 32 bins in space along each axis clip each primitive to the bins it crosses, and a
  primitive split this way is referenced from each leaf that holds part of it.
- **Spatial splits add at most a budget of references**, by default as many as there are
  primitives. Each node shares what is left of its part between its halves by their counts.
- **A node of eight references or fewer becomes a leaf** when that costs less than splitting it.
  A node 64 deep is a leaf however many it holds.
- **The binary hierarchy collapses to nodes of eight children.** A node opens its child of the
  largest area that is not a leaf until it holds eight, or every child is a leaf.
- **The hierarchy is the same on every run.** Splits depend only on the primitives. A node of 4096
  references or more builds its halves on separate threads, each into a part of its own, and
  joins them in order.

A `device::cpu::Node`, the layout the CPU traverses, holds up to eight children, packed from the
first slot, each with its box at full precision. The boxes are stored plane by plane, so a SIMD
test reads one plane of every child at once. A child is another node's index, `kLeaf` with its
first reference and a count, or `kEmpty` with an empty box. The root is the first node, and a
hierarchy of no primitives has one node of empty slots. `accel::cost()` gives a hierarchy's cost
by the surface area heuristic.

Background: [ADR-0205](../adr/offline/0205-offline-the-hierarchy-is-eight-wide-at-full-precision.md)

## A hierarchy per prototype

`accel::assemble(device, prototypes)` builds a hierarchy for each mesh and each group of a
scene's prototypes, and one for the world, all held in one array of nodes and one of references.
An `accel::Assembly` gives each one's root.

- **A mesh's hierarchy is built once**, however many instances place it, as `build()` builds one
  over triangles. Its leaves hold the indices of its triangles within the mesh.
- **A group's hierarchy is built over its instances**, with no spatial splits. Each instance is
  bounded by its prototype's box with its eight corners moved by its transformation. A group's
  leaves hold instances. Groups are built in their order, so each member's box is known.
- **The world's hierarchy is a group's**, over the instances in no group.

A `device::cpu::Instance` is an instance as the CPU traverses it: the inverse of its
transformation, its prototype's root, its index in the scene and its visibility. An instance of a
mesh names the mesh's first triangle, and one of a group names `kGroup`.

## The CPU's traversal

`device::cpu::Traversal` traverses the hierarchies from the world's root, testing a node's eight
children at once: as one AVX register for AVX2 and AVX-512, and as two SSE registers for SSE4.2.

- **A box is entered at its near planes and left at its far ones**, chosen per axis by the sign of
  the ray's direction. An empty child's near plane lies beyond its far one, so no ray reaches it.
  A distance that is not a number, from a ray along a box's plane, is passed over.
- **The far distance is widened** by Ize's bound of 1 + 2γ(3), so a ray that grazes a box is never
  lost to rounding.
- **`nearest()` tests the leaves it reaches nearest first**, and pushes the nodes it reaches
  farthest first, so the nearest is visited next. A node entered beyond the closest hit so far is
  passed over. `any()` stops at the first triangle it meets.
- **A leaf of a group's hierarchy enters each of its instances**, and the ray is moved into the
  instance's prototype's space, where it traverses the prototype's hierarchy. A distance along the
  moved ray reaches the point the same distance along the ray it was moved from, so hits inside
  and outside an instance are compared by their distances alone.
- **A ray enters an instance only when the instance is visible to its kind**, and only when its
  chain holds fewer instances than a hit records.
- **A hit names the same primitive as the brute-force traversal's**: the triangle's index within
  its mesh, with the chain of instances the ray entered to reach it, outermost first.

`view::Tracing` holds a scene's prototypes on a CPU device: their triangles placed in world space
for the brute-force traversal, and the hierarchies when the hierarchy is traced through.
`view::trace(device, prototypes, rays, tracer)` traces a list of rays and gives each one's
nearest hit and whether anything lies on it.

The brute-force traversal is the oracle for instances too. Its triangles are moved into the world
and the hierarchy's rays into each instance, so the two round apart, and their distances agree
within a tolerance rather than exactly.
