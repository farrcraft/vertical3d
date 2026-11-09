# Moya Geometry — The Hierarchy, Intersection and Tessellation

Drafted 2026-11-08 against `25ff996c`. This plan takes up section 3 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core traces rays
through the scene that section 2 built. A wide bounding volume hierarchy holds every geometry
the scene names, at every time of its motion and through nested instances, and the CPU
traverses it with SIMD. Triangles are intersected watertight, and curves, points and the
quadrics by moya's own code. Subdivision surfaces go through OpenSubdiv, and surfaces are
tessellated to an edge length in pixels and displaced within a bound. A change to the scene
rebuilds only the hierarchies it touched. Nothing is shaded yet. A hit view writes what each
camera ray hit, and section 4's path tracer is the first to light it. The reyes hider and the
ray hider keep rendering every RIB scene and every editor render, and no picture they make
changes.

The rebuild's decisions are ADR-0186 to ADR-0194, the foundation's ADR-0195 to ADR-0199, and the
scene's ADR-0200 to ADR-0203, all accepted. This plan follows five of them:

* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md):
  moya implements its own acceleration structures and tessellation, and OpenSubdiv evaluates
  subdivision surfaces.
* [ADR-0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md): one kernel
  source for every device, with ray traversal in the device layer.
* [ADR-0193](../adr/offline/0193-offline-moya-owns-its-ray-acceleration-structure.md): moya's
  own wide BVH, with motion over several segments and nested instances, and every primitive but
  the triangle intersected by moya's code on every device.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): one
  picture per device on every run.
* [ADR-0201](../adr/offline/0201-offline-each-scene-object-keeps-its-own-sample-times.md): each
  object keeps its own sample times, and a transformation is interpolated by decomposition.

The plan keeps the roadmap's order, with five changes:

* **Six decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are how kernel code reaches traversal, the hierarchy's width and nodes, how
  the CPU picks its instruction set, how motion is held in the hierarchy, how curves are
  intersected, and when surfaces are tessellated. [Decisions](#decisions).
* **A hit view comes first, with a brute-force traversal.** Nothing renders the scene until
  section 4, so every step needs a picture of its own. The hit view writes each camera ray's
  depth, normal and IDs into film channels. Its brute-force traversal tests every ray against
  every primitive, and is the oracle every later traversal is compared with.
  [Step 8](#step-8--the-hit-view-and-the-brute-force-oracle).
* **Camera rays are this plan's.** The hit view needs them, so this plan generates rays from the
  scene's camera, with a time over the shutter and a point on the lens. Section 4's generate
  stage builds on them. [Step 8](#step-8--the-hit-view-and-the-brute-force-oracle).
* **Displacement takes its height from a primvar until section 5.** The roadmap puts
  displacement here and materials in section 5. This plan builds the tessellator, the bound and
  the displaced surface with its height read from a primvar. Section 5 connects a material's
  displacement terminal in its place. [Step 17](#step-17--tessellation-and-displacement).
* **Speed is a step with a measured baseline.** ADR-0193's cost is that the CPU traversal has to
  reach Embree's speed. A benchmark of fixed scenes measures build time, memory and rays per
  second, so a later change can be held to it. [Step 19](#step-19--speed).

## What the survey found

The tree was read at `25ff996c`. Line numbers are from that commit.

**The device and kernel layers:**

* **A device holds memory and nothing else** (`moya/libmoya/device/Device.h:28`). ADR-0191 puts
  ray traversal in the device layer, and no interface for it exists yet.
* **Kernel code includes only kernel headers, glm and seven standard headers**
  ([Kernel.md](../offline/Kernel.md#the-rules)). No SIMD intrinsics header is among them, and the
  page says that the lanes of a SIMD traversal are moya's own types.
* **Nothing in the tree's CMake sets an instruction set.** No target names `/arch:AVX2` or
  `-mavx2`, so the CPU code is compiled for the compiler's default, SSE2 on x64.
* **The kernel target compiles as C++20 with no fused multiply and add**
  (`moya/libmoya/kernel/CMakeLists.txt`), and its contraction rule is PUBLIC. A traversal that
  inlines a kernel intersection routine rounds as the kernel does.
* **The CPU device runs tiles on oneTBB** behind `device/internal/Arena.h`, linked PRIVATE.
  [Devices.md](../offline/Devices.md#the-cpu-device).

**The scene:**

* **Every geometry this plan intersects is already a kind**
  ([Scene.md](../offline/Scene.md#moyas-kinds)): `mesh`, `subdivision`, `curves` in RI's bases
  with linear or cubic type and a flat or round profile, `points`, the seven quadrics, `group`
  and `instance`. An instance carries visibility to camera, shadow and indirect rays.
* **Each object keeps its own times over the shutter**, and points keep one count at every time
  ([Scene.md](../offline/Scene.md#samples-over-the-shutter)).
* **A nested instance has no single world transformation**, and a group that holds itself is
  stopped by whatever is built from the scene, not by the scene.
* **A consumer asks for changes since its own version** (`moya/libmoya/scene/Scene.h:148`),
  after the owner's `apply()` (`:106`). The hierarchy is the first consumer.
* **`points` has no profile.** Curves are flat or round, and points have only a width.

**The old renderer:**

* **`trace::Hierarchy` is binary**, built with a binned surface area heuristic over boxes
  (`api/render/offline/trace/Hierarchy.h:20`). A moving primitive is bounded by the union of its
  poses (ADR-0092). It does not carry over, and section 7 removes it.
* **The ray hider writes a depth plane** (`moya/libmoya/raytrace/Hider.cxx:76`,
  `state/Planes.h:31`). The new core's depth can be compared with the old one's for a scene that
  both read.
* **moya's suite has 22 RIB scenes** in `moya/tests/data`, each with a committed scene
  description in `moya/tests/data/scenes`. Ten are ray traced, ten are reyes, and two are
  `spec-cube` and `textured`.

**The dependencies:** OpenSubdiv is not installed. It appears only in the USD port's `imaging`
and `vulkan` features, with OpenGL (`ports/usd/vcpkg.json:26`). The `moya` feature holds Imath,
OpenColorIO, OpenEXR and oneTBB (`vcpkg.json:40`).

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0204 | How kernel code reaches ray traversal | [1](#step-1--the-record-how-kernel-code-reaches-traversal) |
| 0205 | The hierarchy's width and node layout | [2](#step-2--the-record-the-hierarchys-nodes) |
| 0206 | How the CPU picks its instruction set | [3](#step-3--the-record-the-cpus-instruction-set) |
| 0207 | How motion is held in the hierarchy | [4](#step-4--the-record-motion-in-the-hierarchy) |
| 0208 | How curves are intersected | [5](#step-5--the-record-curves) |
| 0209 | When surfaces are tessellated | [6](#step-6--the-record-tessellation) |
| 0210 | Each CPU instruction set is a library, amending 0206 | [11](#step-11--the-cpu-traversal) |
| 0211 | A round curve is swept by a sphere, amending 0208 | [15](#step-15--curves) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-how-kernel-code-reaches-traversal) | The record: how kernel code reaches traversal | `docs/adr` | **0204** | drafted |
| [2](#step-2--the-record-the-hierarchys-nodes) | The record: the hierarchy's nodes | `docs/adr` | **0205** | drafted |
| [3](#step-3--the-record-the-cpus-instruction-set) | The record: the CPU's instruction set | `docs/adr` | **0206** | drafted |
| [4](#step-4--the-record-motion-in-the-hierarchy) | The record: motion in the hierarchy | `docs/adr` | **0207** | drafted |
| [5](#step-5--the-record-curves) | The record: curves | `docs/adr` | **0208** | drafted |
| [6](#step-6--the-record-tessellation) | The record: tessellation | `docs/adr` | **0209** | drafted |
| [7](#step-7--rays-hits-and-triangles) | Rays, hits and watertight triangles | `moya/libmoya/kernel` | 0204 | done |
| [8](#step-8--the-hit-view-and-the-brute-force-oracle) | The hit view and the brute-force oracle | `moya/libmoya`, `moya` | 0204 | done |
| [9](#step-9--meshes-into-triangles) | Meshes into triangles | `moya/libmoya/geometry` | — | done |
| [10](#step-10--the-builder) | The builder: binned surface area with spatial splits | `moya/libmoya/accel` | 0205 | done |
| [11](#step-11--the-cpu-traversal) | The CPU traversal | `moya/libmoya/device` | 0204, 0205, 0206, **0210** | done |
| [12](#step-12--instances) | Instances, nested | `moya/libmoya/accel`, `device` | 0204 | done |
| [13](#step-13--motion) | Transformation and deformation motion | `moya/libmoya/accel`, `kernel` | 0207 | done |
| [14](#step-14--points-and-the-quadrics) | Points and the quadrics | `moya/libmoya/kernel` | — | done |
| [15](#step-15--curves) | Curves, round and flat | `moya/libmoya/kernel`, `geometry` | 0208, **0211** | done |
| [16](#step-16--subdivision-surfaces) | Subdivision surfaces through OpenSubdiv | `moya/libmoya/geometry`, `vcpkg.json` | 0209 | done |
| [17](#step-17--tessellation-and-displacement) | Tessellation and displacement | `moya/libmoya/geometry` | 0209 | done |
| [18](#step-18--rebuilding-what-changed) | Rebuilding what changed | `moya/libmoya/accel` | — | done |
| [19](#step-19--speed) | Speed: the benchmark and its baseline | `moya/bench` | — | open |
| [20](#step-20--the-rib-scenes-through-the-hit-view) | The RIB scenes through the hit view | `moya/tests` | — | open |
| [21](#step-21--the-documents) | The reference pages for geometry and traversal | `docs` | — | open |

## What blocks what

```
1 ADR ──> 7 Rays, triangles ──> 8 Hit view ──┬──> 9 Meshes ──> 10 Builder ──> 11 CPU traversal ──┬──> 12 Instances
2 ADR ───────────────────────────────────────┼──────────────────> 10, 11                         │
3 ADR ───────────────────────────────────────┼─────────────────────────────────> 11              │
4 ADR ───────────────────────────────────────┼───────────────────────────────────────────────────┴──> 13 Motion
                                             ├──> 14 Points, quadrics
5 ADR ───────────────────────────────────────┴──> 15 Curves (after 13)
6 ADR ──> 16 Subdivision (after 9) ──> 17 Tessellation, displacement

(12 to 17) ──> 18 Rebuilds, 19 Speed, 20 RIB scenes ──> 21 Documents
```

Steps 1 to 6 depend on nothing. Step 7 needs the record of step 1, and step 8 needs step 7.
Steps 9 and 14 can start once the hit view lands, since the brute-force traversal intersects any
primitive the kernel can test. Step 10 needs meshes to build over, and step 11 needs the builder
and the records of steps 2 and 3. Steps 12 and 13 need the CPU traversal, and can run beside each
other. Step 15 needs step 13 as well, since a curve's points move. Step 16 needs step 9's
triangles, and step 17 needs step 16. Steps 18, 19 and 20 need every primitive and both kinds of
motion. Each step updates its own section of the reference documents, and step 21 checks that
they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **The oracle.** From step 11 on, each traversal is compared with step 8's brute-force traversal
  over the same scene: every ray of a fixed set returns the same nearest primitive, and a
  distance within a tolerance.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build, and at every instruction set step 3 chooses
  that the runner supports.
* **No old picture changes.** The old hiders read RIB as before, so every committed PNG in
  `moya/tests/data/` stays as it is, and the editor's render window shows the same picture.

---

### Step 1 — The record: how kernel code reaches traversal

**The question.** How a stage written in kernel code traces a ray, when traversal belongs to
each device. On the CPU, section 4's stages run fused, so the shade stage calls traversal
directly. On a GPU, HIP RT and OptiX traverse, and call moya's kernel code back to intersect a
curve or a quadric. The record settles where the line between kernel code and device code
falls, and what a hit holds.

**The options:**

* **One function declared in kernel code, defined per device build**, as Cycles'
  `scene_intersect()` is: each back end compiles its own definition, chosen when it is built. The
  CPU's definition is SIMD host code. A GPU's calls its vendor's trace.
* **A traversal type passed to each stage as a template parameter.** Each device instantiates
  the stages with its own. The kernel never names a device, and each device's stages are a
  separate instantiation.
* **Traversal as a stage of its own over a queue of rays**, on every device, as pbrt-v4's
  wavefront integrator runs it on the CPU as well. Kernel code never calls traversal, and the CPU
  gives up the fused path ADR-0192 chose.

The record also settles what a hit holds: its distance, the primitive and the chain of instances
that placed it, the parametric position on the primitive, and what section 5 needs for texture
footprints. A hit that holds less is recomputed by the shade stage from the primitive.

**Drafted** as
[ADR-0204](../adr/offline/0204-offline-stages-take-their-traversal-as-a-type.md): each stage
that traces takes the traversal as a template parameter, as Mitsuba 3 does, so the reference and
every instruction set run in one binary. A hit holds its distance, primitive, position on it, time
and chain of instances, as HIP RT, OptiX and Embree return them. The shade stage computes the
rest.

### Step 2 — The record: the hierarchy's nodes

**The question.** How many children a node has, and how a node is laid out. ADR-0193 says four
or eight. The width fixes how many boxes one SIMD test covers, and the layout fixes memory per
primitive and what a GPU back end can reuse.

**The options:**

* **Eight children**, tested with AVX2's eight lanes or half of AVX-512's sixteen. Cycles' GPU
  layout and Ylitie and others' compressed wide BVH are eight wide.
* **Four children**, tested with SSE's four lanes or half of AVX2's eight. Shallower trees cost
  more nodes.
* **Both, chosen by instruction set**, as Embree builds BVH4 or BVH8 by what the CPU runs.
* **Full-precision boxes, or boxes quantized** to a few bits against the node's own bounds, as
  the compressed wide BVH and Embree's quantized nodes are.

**Drafted** as
[ADR-0205](../adr/offline/0205-offline-the-hierarchy-is-eight-wide-at-full-precision.md): eight
children per node, each box at full precision, built binary and collapsed. Embree's default
layout is at full precision, and the traversal is held to its speed.

### Step 3 — The record: the CPU's instruction set

**The question.** How the CPU traversal reaches AVX2 and AVX-512 on a machine that has them, and
still runs on one that has neither. The tree compiles for SSE2 today, and a build that assumes
AVX2 does not start on an older CPU.

**The options:**

* **Compile the traversal once for each instruction set and choose when the device is made**, by
  what the CPU reports, as Embree and OIDN do. A test can force each.
* **Choose when moya is built**, with one instruction set for the whole binary. The simplest, and
  a release build has to name the oldest CPU it runs on.
* **Write the lanes portably** through a library or the compiler's vector extensions, and let one
  of the two above choose the width.

The record also settles the oldest instruction set moya runs on, and whether a traversal
without SIMD remains for checking the others.

**Drafted** as
[ADR-0206](../adr/offline/0206-offline-the-cpu-path-is-built-per-instruction-set.md): the fused
CPU path compiled for SSE4.2, AVX2 and AVX-512, chosen when the device is made, with an option
that names a lower one for tests. Embree, Cycles and OIDN choose at start-up. The brute-force
traversal is the reference, so there is no scalar traversal.

### Step 4 — The record: motion in the hierarchy

**The question.** How the hierarchy bounds geometry that moves. Each object keeps its own times,
so two objects in one hierarchy may have different counts of samples. A box that bounds the whole
shutter is loose for anything that moves far.

**The options:**

* **Boxes at two times per node, interpolated by the ray's time**, as Embree's linear motion
  blur and OptiX's motion nodes hold them. Several segments become a subtree per segment.
* **Nodes split by time** as well as by space, so a subtree covers part of the shutter, as
  Embree's multi-segment motion builder splits them.
* **One box over the whole shutter**, as the old `trace::Hierarchy` holds a moving primitive.
  Simple, and slow for fast motion.

The record also settles how a moving instance is bounded. A transformation interpolated by
decomposition sweeps a rotating box through a curve, not along a line, so a box at each end does
not bound the middle. pbrt bounds it exactly from the motion's derivatives, and Embree adds a
margin.

**Drafted** as
[ADR-0207](../adr/offline/0207-offline-the-hierarchy-splits-motion-by-time.md): boxes at the two
ends of each node's time range, with time splits where they cost less than spatial ones, as
Embree's builder for several segments does. A rotating instance is bounded exactly, as pbrt and
Embree bound it.

### Step 5 — The record: curves

**The question.** How a round curve and a flat curve are intersected. A curve is thin, so the
method fixes how it looks when it is close, how it shades along its length, and whether the CPU
and a GPU agree. RI's curves come in five bases, linear or cubic.

**The options:**

* **Reshetov's phantom intersector** for round curves: a cone along the curve stepped towards
  the hit. It is exact to the curve's surface and runs on a GPU without a stack.
* **Recursive subdivision of the curve** until a piece is near straight, as pbrt does, for both
  profiles. Robust, and its depth sets its cost.
* **Converting each curve to segments of one basis**, cubic B-spline or Bézier, when the scene is
  built, so the intersector handles one basis. Cycles, Embree and OptiX each accept fewer bases
  than RI names.

Flat curves face the ray in every option, as RI's flat curves face the camera.

**Drafted** as
[ADR-0208](../adr/offline/0208-offline-curves-are-swept-and-converted-to-bezier.md): round curves
subdivided to a fixed depth and refined by Newton's method, as Embree does and Cycles runs on
GPUs. Flat curves face the ray. Every cubic basis converts exactly to Bézier when the scene is
built.

### Step 6 — The record: tessellation

**The question.** When a subdivision surface or a displaced surface becomes triangles, and what
its edge length in pixels is measured from. The roadmap asks for an edge length in pixels, so the
camera decides how finely a surface is cut.

**The options:**

* **All of it when the hierarchy is built**, from the render camera, as Cycles' adaptive
  subdivision and Arnold do. A GPU sees only triangles, and moving the camera tessellates again.
* **On demand into a cache of limited size**, as RenderMan RIS tessellates while it renders. Less
  memory for a large scene, and a cache that a GPU back end has to share.
* **A dicing camera of its own**, which the scene names separately from the render camera, so a
  moving camera does not tessellate again.

The record also settles what OpenSubdiv is asked for: the limit surface evaluated at the
tessellator's points, or a uniform refinement cut further by moya.

**Drafted** as
[ADR-0209](../adr/offline/0209-offline-surfaces-are-tessellated-when-the-hierarchy-is-built.md):
everything tessellated on the CPU when the hierarchy is built, measured from a dicing camera that
is the render camera unless the scene names another. A shared edge is cut alike on both sides,
and OpenSubdiv evaluates the limit surface, as Cycles does.

### Step 7 — Rays, hits and triangles

**Done.** `Ray`, `Hit`, `Shear` and `intersectTriangle`, `offsetStart`, and the concept
`Traversal` are kernel headers in `kernel/geometry/`. Each case was run against broken code as
well. With the test made to reject a ray exactly on an edge, 52 rays through the edges of a
closed octahedron missed it. With no offset, rays leaving a triangle met it again at each of the
three scales tried. Two things the list below did not settle:

* **A hit's chain holds four instances**, as `kInstanceDepth`, and its primitive is an index
  within the geometry the innermost instance places. Every geometry is placed by an instance, so
  no other index is needed.
* **The interval is open at its end**, so a shadow ray to a point on a light does not meet the
  light, and a distance that is not a number is no hit.

[Traversal.md](../offline/Traversal.md) states the rules.

* **`moya/libmoya/kernel/geometry/`**, in `v3d::moya::kernel::geometry`. A ray with an origin, a
  direction, an interval and a time. A hit as step 1 decides.
* **Triangles intersected watertight**, by Woop, Benthin and Wald's method: a ray through a
  shared edge or vertex hits one of the triangles, and never none.
* **Ray starts offset** by Wächter and Binder's method, from the hit point and the geometric
  normal, so a ray leaving a surface does not hit it again.
* **The traversal interface** as step 1 decides, with nothing behind it until step 8.

**Tests:** a ray through each edge and vertex of a closed mesh hits it. A ray offset from a hit
on a large and on a distant triangle does not hit that triangle again. A ray that misses within
its interval reports no hit.

### Step 8 — The hit view and the brute-force oracle

**Done**, after step 9, since `moya --hits` reads RIB's polygons and needs their triangles.
`camera::fromScene()` builds a `kernel::camera::Camera` with RI's derivations, and its `ray()`
draws a point on the lens and a time over the shutter. `geometry::flatten()` places every
triangle the scene's instances reach in world space, one run per chain of instances, and
`geometry::World` holds them on a device. `kernel::geometry::BruteForce` tests them all, and
`view::hits()` launches `kernel::view::hits` over the picture. Each case was run against broken
code as well. A screen read upside down failed the coverage case, and a traversal that kept the
last hit rather than the nearest failed the nearest-hit case once the near triangle was placed
first. Four things the list below did not settle:

* **The hit view takes one ray per pixel**, through the pixel's centre and the lens's, so its IDs
  are whole and its depth is exact. Its time over the shutter is a parameter.
* **Placement is the oracle's.** Flattening nested instances on the host is simple enough to
  trust, so step 12's instance traversal is checked against it. A placed run also gives a hit's
  normal and face from its chain and primitive, so the hit view works over any traversal.
* **A transformation is its first sample** until step 13. Meshes alone are placed. Every other
  prototype is reported once and left out until its step.
* **An instance whose group was removed is placed by nothing**, rather than falling into the
  world.

[Traversal.md](../offline/Traversal.md) states the rules, and the moya manual's
[CommandLine.md](../manual/moya/CommandLine.md) names `--hits`.

* **Camera rays** from the scene's `camera`: perspective and orthographic, the screen window, a
  time drawn over the shutter and a point drawn on the lens, from the ZSobol sampler. Section 4's
  generate stage builds on them.
* **A brute-force traversal**, which tests every primitive, behind step 7's interface. It is the
  oracle every later traversal is compared with.
* **The hit view**: a launch on the CPU device that writes each pixel's depth, geometric normal,
  instance and primitive into film channels, and coverage into the beauty.
* **`moya --hits`** reads a RIB file through the scene client and writes the hit view as EXR, in
  place of rendering it.

**Tests:** a triangle in front of a camera covers the pixels its projection covers. A pixel's
depth is its hit's distance along the camera's view. An orthographic camera's rays are parallel.
A ray drawn at a time inside the shutter carries that time.

### Step 9 — Meshes into triangles

**Done**, before step 8. `geometry::triangulate()` cuts a mesh's faces by ear clipping in each
face's own plane, and gives each triangle's corners as positions in the mesh's indices, so a
face-varying primvar is read at the corner. With the ear test's check for corners inside an ear
removed, the concave case failed. Two things the list below did not settle:

* **A corner in line with its neighbours is left out before any ear is cut**, so no triangle is
  a sliver along a straight edge.
* **A face whose edges cross is cut as a fan** once no ear can be found.

[Geometry.md](../offline/Geometry.md) states the rules.

* **`moya/libmoya/geometry/`**, in `v3d::moya::geometry`: host code that turns the scene's
  geometry into what the hierarchy holds.
* **Polygons with more than three vertices become triangles**, by ear clipping, so a concave
  polygon is cut inside itself. A face's triangles keep its index, so a hit names the face.
* **Primvars keep their interpolation** through triangulation: a uniform value stays the face's,
  and a face-varying value follows its corner.
* **Degenerate faces are dropped**, and reported once per mesh.

**Tests:** a concave polygon's triangles cover it and nothing outside it. A hit on any triangle
of a face names the face. A face of two vertices is dropped.

### Step 10 — The builder

**Done.** `accel::build()` builds a binary hierarchy by the surface area heuristic, with spatial
splits where a node's best halves overlap, and collapses it into `device::cpu::Node`s of eight
children at full precision. `CpuDevice::each()` runs host work on the device's threads, so the
builder reaches oneTBB only through the device. On long thin triangles along a diagonal, spatial
splits lower the cost by about two fifths. Three things the list below did not settle:

* **The budget is shared by counts.** Each node shares what is left of its budget of added
  references between its halves by their counts. A budget that threads drew from as they went
  would make the hierarchy depend on which thread split first, as Embree's spatial builder does.
* **The collapse is greedy by area**, as Embree's is, not the optimal collapse of the compressed
  wide hierarchy. Nodes average about four children over scattered triangles, since a leaf holds
  up to eight references and is never opened.
* **A primitive split in space is not unsplit.** Stich and others also try keeping a straddling
  primitive whole on one side when that costs less. The budget bounds what the extra references
  cost.

[Traversal.md](../offline/Traversal.md#the-hierarchy) states the rules.

* **`moya/libmoya/accel/`**, in `v3d::moya::accel`: host code that builds a hierarchy into
  device memory.
* **Binned surface area**, with spatial splits where a primitive's box overlaps its neighbours
  badly, as Stich, Friedrich and Dietrich's split hierarchy does. A primitive split this way is
  referenced from more than one leaf.
* **A wide hierarchy** as step 2 decides, collapsed from the binary build.
* **Built on the CPU device's threads**, through the arena, so a host that limits oneTBB limits
  the build.
* **The same hierarchy on every run**, whatever the threads do, so a picture is the same on every
  run as ADR-0194 requires.

**Tests:** every primitive is reachable from the root. The cost of a long, thin triangle scene
is lower with spatial splits than without. Two builds of one scene on different thread counts
are equal node for node.

### Step 11 — The CPU traversal

**Done.** `device::cpu::Traversal` tests a node's eight children at once, and lives in three
shared libraries, `moya_cpu_sse42`, `moya_cpu_avx2` and `moya_cpu_avx512`, each compiled whole
for its instruction set. The CPU device loads the newest the processor runs, from beside the
executable, and a test names each in turn. `view::trace()` traces a list of rays through either
traversal, and the hit view traces through the hierarchy. Each case was run against broken code
as well: with the box test blind to a direction's sign, every scene disagreed with the oracle at
both instruction sets this machine runs. Three things the list below did not settle:

* **Each instruction set is a library, by
  [ADR-0210](../adr/offline/0210-offline-each-cpu-instruction-set-is-a-library.md).** ADR-0206's
  sources compiled for each instruction set in one binary would share glm's and the kernel's
  inline functions, and the linker could keep an AVX2 copy for the SSE4.2 path. The owner chose a
  library per instruction set over an inline namespace per source or a traversal of intrinsics
  alone.
* **AVX-512 runs the AVX2 code compiled for AVX-512**, since a node has eight children. The
  machine this step was built on has no AVX-512, so that library is compiled and loaded by no test
  here. CI's runners test it where they have it.
* **The libraries are copied beside each executable on every build**, by a target of their own.
  A copy after the executable links missed a library that changed alone, and the first check
  against broken code passed for that reason.

[Devices.md](../offline/Devices.md#the-cpus-instruction-sets) and
[Traversal.md](../offline/Traversal.md#the-cpus-traversal) state the rules.

* **The CPU device's traversal** behind step 1's interface, testing a node's children at once, at
  each instruction set step 3 chooses.
* **Nearest hit, and any hit** for a shadow ray, which stops at the first.
* **The triangle test** of step 7, inlined, so the CPU's triangles are the kernel's.

**Tests:** over random rays through each of a set of scenes, the traversal returns the oracle's
nearest primitive, at every instruction set the machine runs. An any-hit ray reports a hit when
the oracle has one. A ray that starts inside a box still finds the nearest hit.

### Step 12 — Instances

**Done.** `geometry::gather()` gathers each prototype once, in its own space, with the instances
that place it, and `geometry::flatten()` now places what it gathers, so the oracle and the
hierarchy read one instancing. `accel::assemble()` builds a hierarchy per mesh, one per group over
its instances' moved boxes, and one for the world, in one array of nodes. The CPU's traversal
moves the ray into each instance it enters. Each case was run against broken code as well. With
the ray left unmoved, the cases of a prototype placed twice, groups in groups, visibility and the
random nested scene failed. With visibility ignored in the hierarchy or in the oracle, the
visibility case and the random scene failed. Four things the list below did not settle:

* **A loop of groups is broken where it closes.** An instance of a group that holds the
  instance's own group is left out, so each group has one set of members and finite bounds. The
  flattening used to stop where a chain came back to an open group, which depends on the path.
  Two groups that hold each other now keep only the one the world reaches.
* **The depth limit is the traversal's.** A group is held once and may be reached at more than
  one depth. An instance too deep on one chain is still held, and the traversal leaves it out
  where its chain is full. `gather()` reports it once.
* **A moved box has no margin.** Rays aimed at the corners and edges of 200 instances, randomly
  turned, scaled and moved, met 25,594 triangles, and the hierarchy lost none without a margin.
  The box test's widened far distance covers the rounding.
* **The oracle agrees within a thousandth.** The brute-force traversal's triangles are moved into
  the world and the hierarchy's rays into each instance. One ray of 3000 grazing a triangle
  differed in its distance by a ten-thousandth, with the same primitive and chain.

[Traversal.md](../offline/Traversal.md#a-hierarchy-per-prototype) states the rules.

* **A hierarchy per prototype**, and one over the instances, each holding its prototype's
  hierarchy and its transformation. A ray is moved into the prototype's space as it enters.
* **Nested instances**: an instance of a group enters the group's hierarchy, to a depth fixed
  when moya is compiled, since kernel code allocates nothing. A group that holds itself, or one
  nested deeper than the limit, is reported and left out.
* **Visibility**: a ray carries its kind, camera, shadow or indirect, and an instance hidden from
  that kind is not hit.
* **A hit names the chain of instances** that placed it, outermost first.

**Tests:** a prototype instanced twice is one hierarchy and two hits in two places. A group in a
group places its geometry at both transformations. A group that holds itself is reported and
left out. An instance hidden from shadows casts none.

### Step 13 — Motion

**Done.** `geometry::gather()` keeps every sample of each transformation and each mesh's points,
and decomposes each sampled transformation into keys. `kernel/geometry/Motion.h` interpolates
both, so the traversal and `geometry::flatten(prototypes, time)` read one motion.
`accel::buildMoving()` builds a hierarchy of `device::cpu::MotionNode`s over the shutter, split
by primitives or by time. `accel::sweepBox()` bounds a moving instance's box from where each
corner's coordinates turn back. Each case was run against broken code as well. With the sweep
left at the boxes of the range's ends, the rotating bar and the swept box failed. With the keys
ignored, the rotating bar and the random scene failed. With points read at the first sample, the
bending mesh and the random scene failed. With a ray's time left unheld, the ray after the
shutter failed. Five things the list below did not settle:

* **A time is held to the shutter.** A ray before the shutter opens is traced as at the opening,
  and one after it closes as at the close, through the hierarchy and the oracle alike.
* **A group moves when anything in it does.** A still group keeps still nodes, so a scene with no
  motion costs no more than it did. A moving group bounds a still instance by its prototype's
  bounds at each end of a range. A moving instance of a moving prototype sweeps the prototype's
  bounds over the whole range, which is looser, and time splits tighten it.
* **A split by time is tried at every node.** Motion that comes back has the same bounds at both
  ends of a range, so only bounding each half again shows whether a split pays. With triangles
  that go out and come back, splitting by time lowers the cost by more than a fifth.
* **A moving node has no margin.** Rays aimed at the corners and edges of moving triangles under
  moving instances, at random times, met 20,351 triangles. The hierarchy lost none without a
  margin.
* **The moving builder runs on the device's threads** as the still one does, and depends only on
  the primitives.

[Traversal.md](../offline/Traversal.md#motion) states the rules.

* **Transformation motion**: an instance's transformation at the ray's time, interpolated by
  decomposition, and bounded as step 4 decides.
* **Deformation motion**: a geometry's points at the ray's time, interpolated between its own
  samples, and bounded as step 4 decides.
* **Each object's own times.** Two objects with different samples sit in one hierarchy.

**Tests:** at each of several times, a moving scene's hit matches a still scene built from it at
that time. A rotating instance is hit at the middle of the shutter, where a box at each end
would miss it. A geometry at three times bends through the middle sample.

### Step 14 — Points and the quadrics

**Done.** `kernel/geometry/primitive/Shapes.h` intersects round points, flat points and RI's seven
quadrics, and `kernel/geometry/primitive/Polynomial.h` finds the torus's roots. `geometry::gather()`
gathers point clouds and quadrics as geometries beside meshes, each of a `Form`, and the
brute-force traversal and the CPU's both test every form. The scene's `points` gains a `profile`,
round by default. Each case was run against broken code as well. With the sweep's limit ignored,
every quadric's hits strayed outside it. With no root isolation in the quartic, the roots, the
torus and the random scene failed. With flat points tested as spheres, the flat point and the
random scene failed. With quadrics bounded at the start of a range, or points without their
width, the random scene failed. Four things the list below did not settle:

* **Points and quadrics are tested in their own space**, by the oracle as by the hierarchy. A
  sphere stretched by a transformation is no longer a sphere, so the oracle holds each run with
  the transformation into its space rather than placing it in the world.
* **A quadric's dimensions move**, each on its own times, as the scene declares. They are sampled
  together at every time any of them is, which reproduces each exactly between its own samples.
  A moving quadric's bounds come from each dimension's least and greatest over a range, since
  its box is not linear in its dimensions: a cut sphere's floor can dip lowest between samples.
* **A point's width moves on its own times**, apart from its centre, and the bounds hold at the
  samples of both.
* **A point has no position on its surface**, so its `uv` is zero, and its face in the hit view
  is its index. A quadric's position is RI's parameters.

[Traversal.md](../offline/Traversal.md#points-and-quadrics) states the rules.

* **The quadrics**: the sphere, cone, cylinder, hyperboloid, paraboloid, disk and torus,
  intersected in their own space with RI's sweep and height limits. The torus is a quartic,
  solved robustly in single precision.
* **Points**: a sphere of the point's width, or a disk facing the ray. The scene's `points` gains
  a `profile`, flat or round, as `curves` has. Round is RI's default.
* **Each is moya's own code on every device**, as ADR-0193 requires, so a GPU back end calls it
  from its custom primitives.

**Tests:** each quadric's hits lie on its surface, and none lies outside its sweep. A ray along a
torus's axis misses it. A flat point is hit from any direction at its width.

### Step 15 — Curves

**Done.** `geometry::cut()` cuts the scene's curves into segments, each cubic basis converted
exactly to Bézier, and `kernel/geometry/primitive/Curves.h` intersects them, linear and cubic,
round and flat. `geometry::gather()` gathers curves as geometries of four new forms, with their
points and widths sampled together. The brute-force traversal and the CPU's both test every form,
and `accel::curveSweep()` bounds a segment by its control points grown by their radii. Each case
was run against broken code as well, one break at a time. Every basis read as Bézier failed the
conversion and the basis cases. A periodic curve one segment short failed the counts. Widths
taken from a curve's first vertex failed the vertex widths. A circle's sweep, or a linear segment
without its start's sphere, failed the round case. A flat hit at the centre line's nearest point
failed the flat case. A piece culled without its radius failed the thin curve. A curve read at
the shutter's opening failed motion and the oracle, and so did a segment bounded without its
radius. A face that named the segment failed the hit view, and a segment that held its own end
failed the joint. Five things the list below did not settle:

* **A curve's width is exact**, by the owner's choice over a least width in pixels, which Embree
  4, Arnold and RenderMan offer. The hit view's one ray per pixel then misses most of a curve
  thinner than a pixel, and the thin curve's test spreads rays across it instead. A curve a
  tenth as wide as their spread is met by a tenth of them, at every traversal.
* **A round curve is swept by a sphere, by
  [ADR-0211](../adr/offline/0211-offline-a-round-curve-is-swept-by-a-sphere.md)**, where ADR-0208
  said a circle. A linear segment is then two spheres and the cone that touches both, in closed
  form, and a cubic segment's ends are the spheres there.
* **A flat curve is met where the round one is.** Its hit is at the depth of the point the ray
  passes deepest inside, by the square of the distance less the square of the radius. A hit at
  the centre line's nearest point missed rays where the radius grows along the segment.
* **A joint belongs to the segment after it.** Two segments that share an end gave one distance,
  and the brute-force traversal and the hierarchy kept different segments. Each segment holds its
  start, and only the segment that closes its curve holds its end.
* **A cubic piece's refinement starts where the ray enters its chord** as a round linear segment.
  A guess at the point nearest the chord led Newton's method to a farther surface for a ray
  running along the curve.

[Traversal.md](../offline/Traversal.md#curves) and
[Geometry.md](../offline/Geometry.md#curves-into-segments) state the rules.

* **Round and flat curves** as step 5 decides, in each of RI's five bases, linear or cubic,
  with a width at each vertex or one for the curve.
* **The scene's curves prepared** in `geometry`: bases converted if step 5 decides so, and
  bounded per segment.
* **Moving curves**, whose points move as step 13 moves a geometry's.

**Tests:** a round curve's hit is at its width from its centre line. A flat curve's hit faces the
ray. A curve in each basis is hit where the oracle hits it. A curve a fraction of a pixel wide
still covers the pixels along it.

### Step 16 — Subdivision surfaces

**Done.** OpenSubdiv 3.5.0 is in the manifest's `moya` feature with none of its features, linked
PRIVATE and static, and a module of the Flatpak. `subdivision::Surface` refines a `Topology`
adaptively and builds the table of patches over its limit surface. `cut()` cuts each patch face
into triangles at a rate, and `at()` evaluates a primvar of any class over them.
`geometry::gather()` cuts the scene's subdivision surfaces into a mesh's triangles, every sample's
points evaluated on the one surface. Each case was run against broken code as well, one break at
a time. Catmull-Clark refined as bilinear failed the cube and the traced surface. Creases of no
sharpness failed the crease, ignored holes the hole, and a boundary always interpolated the
boundary. Loop refined as Catmull-Clark failed Loop's limit. A quad's triangles wound backward, or
an n-sided face cut as one patch face, failed the winding. Values never welded, or legacy linear
patches, failed the face-varying case, and varying values read as vertex values the varying one.
A surface of no faces refined failed the empty surface, and no report the refused topology. Every
sample cut from the first, or every triangle naming face 0, failed the traced surface. Four things
the list below did not settle:

* **The rate is 8 pieces a side until step 17**, which measures an edge length in pixels. Places
  are not shared between patch faces, so two faces' points on a shared edge agree only to
  rounding until step 17 closes cracks.
* **Face-varying values are welded by value.** RI gives one per corner and no indices, so corners
  of one vertex with equal values share one of OpenSubdiv's values, and a seam is where they
  differ. OpenSubdiv's legacy linear face-varying patches are off. With them, a face-varying
  value with no seams strayed 0.04 from the same value interpolated by vertex on a cube.
* **Primvars are evaluated and not yet held.** No geometry holds primvars until section 5 shades,
  so `at()` is tested on its own.
* **A surface of no faces skips OpenSubdiv**, whose debug build asserts on an empty mesh. Every
  kind of geometry is now traced, so the hit view's case of a prototype that is not traced is
  gone. Its scene is the subdivision suite's case of a surface of no faces.

[Geometry.md](../offline/Geometry.md#subdivision-surfaces) states the rules, and
[Dependencies.md](../contributing/Dependencies.md) records OpenSubdiv.

* **OpenSubdiv as a dependency**: a `moya` feature dependency in `vcpkg.json` with no OpenGL,
  linked PRIVATE to libmoya, and built on Linux in CI.
  [Dependencies.md](../contributing/Dependencies.md) records it.
* **The scene's `subdivision`**: Catmull-Clark, Loop and bilinear schemes, creases, corners,
  holes and boundary interpolation, as RI names them, through OpenSubdiv's topology refiner.
* **Primvars across the surface**, face-varying ones included, through OpenSubdiv.
* **Triangles from the limit surface**, at the density step 6 decides, as step 17 asks.

**Tests:** a cube subdivided is within a tolerance of the sphere-like limit Catmull-Clark gives
it. A crease of infinite sharpness keeps its edge. A hole leaves its face open.

### Step 17 — Tessellation and displacement

**Done.** `subdivision::Surface::cut(points, measure)` cuts each edge into its own number of pieces,
measured once along the first face that reaches it, and both patch faces beside it cut it alike.
A place on an edge or a corner names the place whose point it takes, so the surface has no
cracks. A patch face whose sides differ has a grid inside it that each side is stitched to.
`subdivision::Dicing` measures an edge in pixels from the dicing camera, and `gather()` cuts each
surface once every instance is gathered, for its placement nearest that camera.
`Surface::normals()` gives the limit surface's normals, and a surface is displaced along them by
its `displacement` primvar, held within its `displacementBound`. Each case was run against
broken code as well, one break at a time. Places that shared no points failed the edges cut on
one side alone, the seams and the displaced hits. Odd halves of an n-sided face's edges failed
the edges and the winding. The first placement in place of the nearest failed the nearest
placement. The render camera in place of the dicing camera failed the finer cut near the camera,
and so did an edge length ignored. A pixel that did not widen with depth failed it and the
nearest placement. No clamp, a normal reversed, or displaced points that were not shared again
failed the displaced hits. Face-varying values shared failed the seams. A grid as coarse as its
coarser side failed the winding. A side joined to its grid's first place alone failed the cells,
and no report of a displacement that does not fit failed that report. Five things the list below
did not settle:

* **A pixel is measured as Cycles measures it**: a piece's length over a pixel's width at its
  depth, not its projection. An edge seen end on is cut as finely as one seen across, for the
  rays that meet it from elsewhere. An edge is cut into at most 64 pieces, which also bounds an
  edge near or behind the camera.
* **The boxes are not grown by the bound.** The plan had them grow by it, but every surface is
  displaced when it is gathered, so the hierarchy bounds the displaced triangles themselves. The
  bound holds the height, and grows the box that picks a surface's nearest placement.
* **The nearest placement weighs every placement** down the chains a traversal follows. A scene
  of many nested placements of one surface pays for each of them once.
* **The height is a float primvar named `displacement`**, of any interpolation, in the surface's
  own space. Section 5 connects a material's displacement terminal in its place. A polygon mesh
  is not tessellated or displaced: it is cut into its own triangles, as step 9 cuts it.
* **A control edge beside a face of other than four vertices is cut evenly**, so that its middle
  is a place that the face's patch faces meet at. `cut(rate)` follows the same rule, so a prism's
  triangles cut at a rate of 3 cut their edges into 4.

[Geometry.md](../offline/Geometry.md#tessellation-and-displacement) states the rules, and
[Scene.md](../offline/Scene.md) names the new parameters.

* **An edge length in pixels**, measured from the camera as step 6 decides, and set in the
  scene's `options`.
* **Displacement of the tessellated surface** along its normal, by a height read from a primvar.
  Section 5 connects a material's displacement terminal in its place.
* **A displacement bound** on each geometry, which its boxes grow by. A height beyond the bound
  is clamped and reported.
* **Cracks closed**: an edge shared by two patches is cut alike on both sides.

**Tests:** a patch nearer the camera is cut more finely than one further away. No two triangles
meet at an edge one cut and the other did not. A displaced sphere's hits lie at its height. A
height beyond the bound is clamped.

### Step 18 — Rebuilding what changed

**Done.** `accel::Assembler` keeps a scene's hierarchies, and `update()` brings them to the
scene's version. It forgets each changed object's geometry in a `geometry::Cache`, and gathers
again through it, so every other geometry is placed as it was made. A geometry's hierarchy is
built again only when its geometry was made again, and a group's when the group, its instances,
or one of their prototypes changed. `view::trace()` takes an assembly, so a test traces through
the assembler's and through one built afresh, and every ray agrees. Each case was run against
broken code as well, one break at a time. Every geometry built on every update failed every
case. A geometry made again but not built again failed the changed mesh and the surface cut
again. Changed objects left in the cache failed the changed mesh. A changed instance ignored
failed the moved instance, and a member's prototype built again but ignored failed it, the
changed mesh and the surface cut again. A group's members ignored failed the removed prototype.
A dicing camera ignored failed the surface cut again. A geometry placed from the cache without
moving its times, its points or its faces failed the removed prototype. The cache dropping
nothing it did not place failed no case: the assembler drops that geometry's hierarchy anyway,
so the drop only frees memory. Four things the list
below did not settle:

* **The gather runs again in full**, and places each geometry from the cache. Walking the
  instances again is cheap beside cutting and building, and keeps one gather for both uses.
* **A geometry keeps its hierarchy wherever it lands.** Its nodes count from its own first node
  and its leaves from its own primitives, so the assembly is appended again from the parts each
  update, and no node is moved in place.
* **A group's hierarchy names its members by their place in it**, and its references are turned
  into instances' indices as it is placed, since an instance's index moves as others come and go.
* **Tessellation is redone through the cache**: a subdivision surface keeps its limit surface and
  what it was measured from, and is cut again when its dicing camera, its edge length or its
  nearest placement moved.

[Traversal.md](../offline/Traversal.md#rebuilding-what-changed) states the rules.

* **The hierarchy is a consumer of the scene.** It keeps the version it was built from, and asks
  for the changes since.
* **A prototype's hierarchy is rebuilt only when its geometry changed.** A moved instance
  changes the hierarchy over the instances alone.
* **Tessellation is redone** for a geometry whose camera measure changed, as step 6 decides.

**Tests:** moving one instance rebuilds no prototype's hierarchy. Changing one mesh rebuilds that
mesh's alone. A removed prototype's instances are left out.

### Step 19 — Speed

* **A benchmark**, `moya/bench`, of fixed scenes: many small triangles, a few large ones, curves,
  instances and motion. It reports build time, memory per primitive, and rays per second for
  coherent and incoherent rays, at each instruction set.
* **A baseline**, committed beside it, from one machine, so a later change is measured against
  it.
* **A comparison with Embree's published numbers** for the same kinds of scene, recorded in the
  reference page, so ADR-0193's cost can be judged.

**Tests:** the benchmark builds and runs one short pass in the suite, so it does not rot.

### Step 20 — The RIB scenes through the hit view

* **Every one of the 22 scenes in `moya/tests/data`** renders through `moya --hits`, and its hit
  view is committed beside the scene's description.
* **Depth checked against the ray hider.** For each still scene with no displacement, the hit
  view's depth matches the ray hider's depth plane within a tolerance. The scene client and the
  traversal then agree with the old renderer about where things are.

**Tests:** the committed hit views, compared within a tolerance that absorbs rounding between
compilers. The depth comparison of each still scene.

### Step 21 — The documents

* **New reference pages**: `docs/offline/Geometry.md` for the primitives, tessellation and
  displacement, and `docs/offline/Traversal.md` for the hierarchy, the traversal interface,
  instances and motion. The docs site's summary lists them.
* **[Kernel.md](../offline/Kernel.md)** and **[Devices.md](../offline/Devices.md)** name the
  traversal interface and the CPU's instruction sets.
* **[Scene.md](../offline/Scene.md)** names the points' profile and the options' edge length.
* **[The offline README](../offline/README.md)**'s section on the new core names the hierarchy,
  the hit view and `moya --hits`.
* **[Testing.md](../contributing/Testing.md)** names the oracle, the hit views and the benchmark.
* **Each record's `Documented in` line** names the reference page that states its rule, and
  ADR-0193's names `Traversal.md` in place of the roadmap.
* **The roadmap's section 3** links here, and says so when this plan closes.
