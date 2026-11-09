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
  third vertices, and for a quadric RI's parameters. A point has none, and its `uv` is zero.
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

## Points and quadrics

Points and RI's quadrics are intersected by moya's own kernel code, in
`kernel/geometry/Shapes.h`, so every device runs the same tests. Each is tested in its own space.
A `ShapeHit` gives the distance and the position on the surface.

- **A round point is a sphere** of half its width about its centre, and **a flat point a disk**
  of the same radius facing the ray, in the plane through its centre. `intersectSphere()` and
  `intersectFacing()` test them. A sphere's crossings come from the point on the ray nearest its
  centre, so a ray from far off keeps the radius.
- **A `Quadric` is one of RI's seven**, each a profile swept about z from +x by `thetamax`. Each
  reads RI's dimensions: a sphere cut at `zmin` and `zmax`, and a cone narrowing from its
  `radius` at z = 0 to its apex at `height`. A cylinder runs from `zmin` to `zmax`, and a
  hyperboloid is swept from the line `point1` to `point2`. A paraboloid has its `radius` at
  `zmax`, a disk lies at `height`, and a torus has a ring and a tube. Angles are radians.
- **`intersectQuadric()` keeps the nearer crossing within the quadric's sweep and limits.** The
  sphere, cone, cylinder, paraboloid and a rising hyperboloid are quadratics, whose roots are
  computed so that neither loses precision to the other. A disk, and a hyperboloid whose line
  lies flat, are planes.
- **The torus is a quartic**, solved in single precision. The ray first moves to where it passes
  nearest the torus's centre, with a direction of unit length, which keeps the coefficients
  small. `rootsIn()` then isolates each root between the polynomial's turning points, the roots of
  its derivative found the same way, and halves each interval that changes sign. A ray down the
  torus's axis passes through its hole.
- **The position on a quadric is RI's**: u along the sweep from 0 to 1, and v along the profile.
  v runs by latitude on a sphere, and by height on a cone, a cylinder or a paraboloid. It runs
  along the line of a hyperboloid, inwards on a disk, and around the tube of a torus. A
  hyperboloid's sweep starts at its line's own angle at the hit's height.
- **`quadricNormal()` gives the geometric normal** from the surface's implicit form.

A quadric's dimensions may each move over the shutter on their own times. `geometry::gather()`
samples them together at every time any of them is sampled at, which holds each between its own
samples exactly, and `sampledQuadric()` interpolates them. A point cloud's centres and widths
each keep their own samples.

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

- **A prototype placed many times is held once.** A `Geometry` is a mesh, a point cloud or a
  quadric, and says its `Form`: triangles, round points, flat points or a quadric.
  - A mesh's triangles are a run of the triangles every mesh holds, with the face each was cut
    from. Its points are three per triangle at each of its samples, one sample after another, and
    the faces are cut where the first sample's points lie.
  - A point cloud holds each point's centre and width at each sample. A cloud that gives a width
    for some points but not all takes its constant width, and is reported.
  - A quadric holds its dimensions at each sample.
- **An instance names its prototype**, a geometry or a group, and its transformation into the space
  of the group that holds it. It keeps its index in the scene. Its `camera`, `shadow` and
  `indirect` parameters give the kinds of ray it is visible to.
- **A group comes after every group it holds.** The world is the group of every instance in no
  group. A group's instances, and the world's, follow their indices.
- **Some instances are left out, and each reason is reported once**:
  - an instance of a group that holds the instance's own group, directly or through another
    group. A group that holds itself would otherwise have no bounds.
  - an instance that would be deeper in a chain than a hit records. It is still held where a
    shorter chain reaches it, and each traversal leaves it out where it is too deep.
  - an instance of a prototype of a kind not yet traced. Meshes, points and the quadrics are
    traced.

  `Prototypes::reports` holds the reports, and the log has them too.
- **An instance whose prototype was removed places nothing**, and one whose group was removed is
  placed by nothing.
- **The shutter is the camera's.** `Prototypes::open` and `close` hold it.

## Motion

Each instance's transformation and each mesh's points keep every sample the scene holds, at
their own times. Two objects need not share times or a count of samples. The interpolation is
kernel code in `kernel/geometry/Motion.h`, so every traversal and the placing below read one
motion.

- **A time is held to the shutter.** A ray before the shutter opens is traced as at the opening,
  and one after it closes as at the close. A hit keeps its ray's own time.
- **A mesh's points are interpolated linearly** between the two samples around a time.
  `sampleAt()` reads them, and a time outside the samples takes the nearer end.
- **A transformation is decomposed at each sample** by `geometry::decompose()`, by polar
  decomposition, into a translation, a rotation and a symmetric scale that holds any shear. A
  `kernel::geometry::Key` holds the three. A linear part that mirrors keeps a rotation, and its
  scale mirrors.
- **`transformAt()` interpolates between the keys around a time**: the translation and the scale
  linearly, and the rotation the shortest way at an even speed. Each key's rotation takes the
  sign nearer the key before it, so the shortest way is the one the samples meant.
- **An instance with one sample holds still**, and is traced by its matrix as it stands, with no
  decomposition.

Background: [ADR-0201](../adr/offline/0201-offline-each-scene-object-keeps-its-own-sample-times.md)

## Placing the scene

`geometry::flatten(prototypes, time)` places every primitive the prototypes' instances reach as
it stands at a time, in runs, one run for each chain of instances. Triangles are placed in world
space. Points and quadrics are held in their own space, each run as a `PlacedShapes` with the
transformation into it from the world, since a sphere stretched by a transformation is no longer
a sphere. With no time,
it places them as they stand when the shutter opens. `geometry::World` holds the result in a
device's memory, and `view()` gives it to kernel code as a `kernel::geometry::Placed`.
`flatten(scene, logger)` gathers the scene and places what it gathers.

- **Each instance in the world places its prototype**, and an instance of a group places every
  instance in the group inside its own transformation. Runs follow the instances' indices.
- **A run is visible to the kinds of ray every instance of its chain is visible to.**
- **A chain deeper than a hit records is left out.**
- **A primitive is its index within its geometry**, which is its index within its run. A hit then
  names the same primitive whichever traversal found it, and `Placed` gives the hit's geometric
  normal and its face from the chain, the primitive and the ray. A point's radius is half its
  width.

## The brute-force traversal

`kernel::geometry::BruteForce` tests every placed primitive for every ray, moving the ray into
each run of points or quadrics. It is slow, and it is
the reference every other traversal is compared with. Its `nearest()` keeps the nearest hit
within the ray's interval, and its `any()` stops at the first. A run hidden from the ray's kind
is passed over. It traces the triangles as they were placed, so it is right for a ray at the time
they were placed at.

## The hit view

The hit view is a picture of what rays hit, which lights nothing. `view::hits(device, scene,
logger, time, tracer)` traces one camera ray through the centre of each pixel and of the lens, at
a point over the shutter, through the hierarchy unless the brute-force traversal is asked for.
The scene is placed at that time, for the brute-force traversal and for each hit's normal and
face.
`view::writeHits()` writes it as an EXR file of float channels, and `moya --hits` writes the hit
view of a RIB file.

| Channel | A pixel whose ray hit | A pixel whose ray met nothing |
|---|---|---|
| `depth` | the hit's depth along the camera's view | infinity |
| `normal` | the geometric normal in world space: by a triangle's winding, from a round point's centre or a quadric's surface, and back along the ray for a flat point | zero |
| `instance` | the index of the innermost instance | -1 |
| `face` | the face of its mesh, the point it met, or 0 for a quadric | -1 |
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

## A hierarchy over motion

`accel::buildMoving(device, count, sweep, begin, end, options)` builds a hierarchy over
primitives that move, over a time range. `sweep` gives a primitive's bounds over any part of the
range as an `accel::LinearBox`: a box at the part's start and one at its end, with the box at a
time between interpolated linearly.

- **Each node covers a time range**, and bounds each of its primitives over that range.
- **A node is split by its primitives or by time**, whichever costs less by the surface area
  heuristic. A split by primitives bins their centroids at the middle of the range, as `build()`
  does. A split by time halves the range, keeps every primitive on both sides, and bounds each
  again over its half. It pays where motion turns back within the range, as a bounce or a turn
  does, so that bounds over the whole range are loose. A box's area is its area averaged over its
  range, which three points give exactly. There are no spatial splits.
- **A split by time is bounded**, because it doubles the references below it. It is taken only
  where it also costs less than a leaf, and no path from the root splits by time more than eight
  times. A node of more than eight primitives that no split helps, such as primitives that all
  move together, is halved in order, so the build always ends.
- **The binary hierarchy collapses to eight children** as a still one does, each node opening the
  child of the largest area times its share of the range. `Options::temporal` turns time splits
  off, and `accel::cost()` weights each child by its share of the range.

A `device::cpu::MotionNode` holds each child's box at the start and end of its range, and the
range. A ray outside a child's range does not reach it. It is twice the size of a `Node`.

The bounds that `sweep` gives are exact where the motion allows it.

- **A moving triangle's** points move along lines between samples. The bounds at the range's ends
  are pushed out until they hold each point at every sample inside the range, which is then
  enough to hold them between.
- **A box moved by an instance that moves** sweeps each corner along a curve. Between two keys,
  each coordinate of a corner is `a0 + a1 w + (b0 + b1 w) cos(θ w) + (c0 + c1 w) sin(θ w)` of the
  fraction `w` along the segment, where θ is the angle the rotation turns. `accel::sweepBox()`
  pushes the bounds out by how far each corner strays from them. That is found where the
  coordinate turns back, by halving the segment wherever a bound on how fast its slope changes
  allows a turn. A corner that swings outside the boxes at the range's ends stays inside.
- **Each bound is rounded outward** from double precision to single.

Background: [ADR-0207](../adr/offline/0207-offline-the-hierarchy-splits-motion-by-time.md)

## A hierarchy per prototype

`accel::assemble(device, prototypes)` builds a hierarchy for each geometry and each group of a
scene's prototypes, and one for the world. Still hierarchies are held in one array of nodes and
moving ones in one of moving nodes, with one array of references. An `accel::Assembly` gives each
one's root, and whether it moves.

- **A geometry's hierarchy is built once**, however many instances place it, and its leaves hold
  the indices of its primitives within it. One that holds still is built as `build()` builds one:
  a mesh over its triangles with spatial splits, and points over their boxes without. One that
  moves is built as `buildMoving()` does, over the shutter.
- **A point is bounded** by its centre and half its width, which `accel::pointSweep()` bounds
  over a time range as a moving triangle's points are bounded.
- **A quadric is one primitive.** `accel::quadricBounds()` gives the box of its whole surface of
  revolution from the least and greatest of each dimension, so it holds whatever the sweep. Over
  a time range, `accel::quadricSweep()` takes each dimension's least and greatest at the samples
  inside the range and at its ends, which hold the quadric throughout, and the bounds hold still.
- **A group's hierarchy is built over its instances**, with no spatial splits. A group moves when
  an instance in it moves, or places something that moves. One that holds still bounds each
  instance by its prototype's box with its eight corners moved by its transformation.
- **A moving group bounds each instance over each range its builder asks for**, by its
  prototype's bounds over that range. An instance that holds still moves those bounds at each
  end of the range. One that moves sweeps its prototype's box over the whole range along its
  keys, as `sweepBox()` does. Groups are built in their order, so each member's bounds are known.
- **The world's hierarchy is a group's**, over the instances in no group.

A `device::cpu::Instance` is an instance as the CPU traverses it. It holds the inverse of its
transformation while it holds still, and its keys while it moves. It names its prototype's root
and whether that moves, its geometry or `kGroup`, its index in the scene and its visibility. A
`device::cpu::Geometry` gives a geometry's form and where its samples are held.

## The CPU's traversal

`device::cpu::Traversal` traverses the hierarchies from the world's root, testing a node's eight
children at once: as one AVX register for AVX2 and AVX-512, and as two SSE registers for SSE4.2.

- **A ray's time is held to the shutter.** A moving node stands at that time as a still one, each
  child's box interpolated, and a child whose range does not hold the time left empty. A moving
  geometry, and a moving instance's transformation, are interpolated at that time.
- **A box is entered at its near planes and left at its far ones**, chosen per axis by the sign of
  the ray's direction. An empty child's near plane lies beyond its far one, so no ray reaches it.
  A distance that is not a number, from a ray along a box's plane, is passed over.
- **The far distance is widened** by Ize's bound of 1 + 2γ(3), so a ray that grazes a box is never
  lost to rounding.
- **`nearest()` tests the leaves it reaches nearest first**, and pushes the nodes it reaches
  farthest first, so the nearest is visited next. A node entered beyond the closest hit so far is
  passed over. `any()` stops at the first primitive it meets.
- **A leaf of a geometry's hierarchy tests each primitive by the geometry's form**: a triangle,
  a round or flat point, or the quadric, as it stands at the ray's time.
- **A leaf of a group's hierarchy enters each of its instances.** The ray is moved into the
  instance's prototype's space by the inverse of its transformation at the ray's time, and
  traverses the prototype's hierarchy there. A distance along the
  moved ray reaches the point the same distance along the ray it was moved from, so hits inside
  and outside an instance are compared by their distances alone.
- **A ray enters an instance only when the instance is visible to its kind**, and only when its
  chain holds fewer instances than a hit records.
- **A hit names the same primitive as the brute-force traversal's**: the primitive's index within
  its geometry, with the chain of instances the ray entered to reach it, outermost first.

`view::Tracing` holds a scene's prototypes on a CPU device: their primitives placed at a time for
the brute-force traversal, and the hierarchies when the hierarchy is traced through.
`view::trace(device, prototypes, rays, tracer)` traces a list of rays and gives each one's
nearest hit and whether anything lies on it. Through the brute-force traversal it places the
scene again for each time a ray holds, so each ray is traced against the scene at its own time.

The brute-force traversal is the oracle for instances, motion, points and quadrics too. Its
triangles are moved into the world and the hierarchy's rays into each instance, so the two round
apart, and their distances agree within a tolerance rather than exactly.
