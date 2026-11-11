# Moya Path Tracer — The Integrator and Its Lights

Drafted 2026-11-09 against `92191de5`. This plan takes up section 4 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core lights the
scene that sections 2 and 3 built. The integrator builds paths through it in stages over an
explicit path state, fused on the CPU. Each bounce samples a light and the surface, and weighs the
two by multiple importance sampling. Every UsdLux light the scene holds is sampled by solid angle
where a method exists, a dome light through its image, and many lights through a light tree.
Light linking and IES profiles shape what each light reaches. A render stops each pixel at a noise
threshold, and a stopped render resumes where it left off. Surfaces are diffuse until section 5
brings the BSDF library. The reyes hider and the ray hider keep rendering every RIB scene and
every editor render, and no picture they make changes.

The rebuild's decisions are ADR-0186 to ADR-0194, the foundation's ADR-0195 to ADR-0199, the
scene's ADR-0200 to ADR-0203, and the geometry's ADR-0204 to ADR-0211, all accepted. This plan
follows five of them:

* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md):
  moya implements its own light sampling and light tree, and OpenEXR reads a light's image.
* [ADR-0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md): a path carries
  four sampled wavelengths.
* [ADR-0192](../adr/offline/0192-offline-path-stages-run-fused-on-the-cpu-queued-on-the-gpu.md):
  the integrator is stages over an explicit path state, run back to back on the CPU.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): samples
  come from ZSobol, and each device gives one picture on every run.
* [ADR-0204](../adr/offline/0204-offline-stages-take-their-traversal-as-a-type.md): each stage
  that traces takes its traversal as a type.

The plan keeps the roadmap's order, with five changes:

* **Five decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are what the path state holds and how it is laid out, how a ray meets a
  light, how the light tree is built, how adaptive sampling measures noise, and what a checkpoint
  holds. [Decisions](#decisions).
* **A statistical comparison comes first.** A path-traced picture is noisy, so no step can compare
  it byte for byte with an expected one. The film gains each pixel's second moment, and a test
  helper compares a picture with an expected value, or two pictures with each other, within a
  number of standard errors. It is the oracle every later step is held to, and section 10 compares
  a GPU with the CPU through it. [Step 6](#step-6--the-statistical-comparison).
* **Surfaces are diffuse until section 5.** The roadmap puts the BSDF library in section 5. This
  plan builds the BSDF interface the integrator calls, with one diffuse lobe whose colour comes
  from the surface node's `Cs` or a primvar. Section 5 puts the BSDF library behind the same
  interface. [Step 8](#step-8--the-bsdf-interface-and-a-diffuse-lobe).
* **The RIB client reads RenderMan's `Light` request.** RI's three standard lights and its area
  lights reach only point, spot, distant and mesh lights. A RIB file that names `PxrRectLight`,
  `PxrDomeLight` and their kin makes every UsdLux light, so `moya --paths` renders each from a
  file. [Step 18](#step-18--renderman-lights-from-rib).
* **Speed is measured with a baseline.** `moya_bench` gains a lit scene and reports samples a
  second, beside its traversal figures, so a later change can be held to it.
  [Step 21](#step-21--speed).

## What the survey found

The tree was read at `92191de5`.

**The foundation and the geometry:**

* **A hit holds no point, normal or differential** ([Traversal.md](../offline/Traversal.md#hits)).
  It holds its distance, time, primitive, `uv` and chain of instances. The stage that shades it
  computes the rest from the primitive.
* **A traversal gives `nearest(ray)` and `any(ray)`**, and a ray carries its kind: camera, shadow
  or indirect. An instance hidden from a kind is not hit.
* **Camera rays exist.** `camera::fromScene()` follows RI's camera, and its `ray()` draws a point
  on the lens and a time over the shutter. The hit view takes one ray through each pixel's
  centre.
* **The film places a camera sample by the pixel filter** and adds it to its own pixel alone,
  in index order ([Film.md](../offline/Film.md)). It holds weighted sums and one sum of weights.
  It holds no second moment.
* **ZSobol gives a sample from its pixel, index and dimension**, with no state
  ([Sampler.md](../offline/Sampler.md)). Nothing yet allocates dimensions to a path's draws.
* **Spectra are four wavelengths drawn by luminance.** `illuminant()` gives a light's colour as a
  spectrum, and a light of (1, 1, 1) shines with ACES white ([Spectra.md](../offline/Spectra.md)).
* **No geometry holds primvars on the device.** `subdivision::Surface::at()` evaluates a primvar,
  and `normals()` gives the limit surface's normals, but neither reaches a hit
  ([Geometry.md](../offline/Geometry.md)).
* **The CPU traversal traces 11 to 12.5 million rays a second** on a million triangles, several
  times slower than Embree ([TODO.md](../TODO.md#offline-rendering)). A path tracer traces at
  least two rays a bounce, so this sets its speed.

**The scene:**

* **Every UsdLux light is a kind** ([Scene.md](../offline/Scene.md#moyas-kinds)): rect, disk,
  sphere, cylinder, mesh, distant and dome. Each holds a transformation over the shutter,
  `colour`, `intensity`, `exposure` and `normalize`. A light with an area holds a cone angle and
  softness. Light linking is UsdLux's `includeRoot` and a list of `instances`.
* **No light has a colour temperature, a shadow link, an IES profile or a camera visibility.**
  UsdLux names all four.
* **A material is a network of nodes**, whose shaders the scene does not read. The RIB client
  makes a node of language `ri` for `Surface`, with the attribute state's colour as `Cs`.
* **No light reaches the gather.** `geometry::gather()` and `accel::Assembler` read geometry and
  instances alone, so nothing a ray meets emits.

**The RIB client:**

* **RI's `pointlight`, `spotlight` and `distantlight` become UsdLux lights**, and
  `AreaLightSource` makes a mesh light of each geometry after it. A point light is a sphere
  light of radius zero. [Rib.md](../offline/Rib.md#the-scene-client)
* **The RIB reader knows no `Light` request**, so a file names no rect, disk, cylinder or dome
  light.

**The tests:** moya's suite has 22 RIB scenes, each with a committed description and hit view.
Their PNGs are the old hiders' pictures. Nothing tests a picture within a statistical tolerance.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0212 | What the path state holds, and how each device lays it out | [1](#step-1--the-record-the-path-state) |
| 0213 | How a ray meets a light with a shape | [2](#step-2--the-record-meeting-a-light) |
| 0214 | How the light tree is built, and what its leaves are | [3](#step-3--the-record-the-light-tree) |
| 0215 | How adaptive sampling measures a pixel's noise | [4](#step-4--the-record-adaptive-sampling) |
| 0216 | What a checkpoint holds, and how a render resumes | [5](#step-5--the-record-checkpoints) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-the-path-state) | The record: the path state | `docs/adr` | **0212** | accepted |
| [2](#step-2--the-record-meeting-a-light) | The record: meeting a light | `docs/adr` | **0213** | accepted |
| [3](#step-3--the-record-the-light-tree) | The record: the light tree | `docs/adr` | **0214** | accepted |
| [4](#step-4--the-record-adaptive-sampling) | The record: adaptive sampling | `docs/adr` | **0215** | accepted |
| [5](#step-5--the-record-checkpoints) | The record: checkpoints | `docs/adr` | **0216** | accepted |
| [6](#step-6--the-statistical-comparison) | The statistical comparison | `moya/libmoya/film`, `moya/tests` | — | done |
| [7](#step-7--the-shading-point) | The shading point and primvars on the device | `moya/libmoya/kernel`, `geometry` | — | done |
| [8](#step-8--the-bsdf-interface-and-a-diffuse-lobe) | The BSDF interface and a diffuse lobe | `moya/libmoya/kernel` | — | done |
| [9](#step-9--the-integrator-stages) | The integrator's stages, and `moya --paths` | `moya/libmoya/kernel`, `moya` | 0212 | done |
| [10](#step-10--lights-with-a-shape) | Rect, disk, sphere and cylinder lights | `moya/libmoya`, `scene` | 0213 | done |
| [11](#step-11--next-event-estimation-and-mis) | Next event estimation and MIS | `moya/libmoya/kernel` | — | done |
| [12](#step-12--distant-lights-and-points) | Distant lights, and lights of no size | `moya/libmoya/kernel` | — | done |
| [13](#step-13--mesh-lights) | Mesh lights | `moya/libmoya` | 0213 | done |
| [14](#step-14--the-dome-light) | The dome light and an importance-sampled environment | `moya/libmoya`, `output` | — | done |
| [15](#step-15--the-light-tree) | The light tree | `moya/libmoya/light` | 0214 | done |
| [16](#step-16--light-linking-and-shadow-linking) | Light linking and shadow linking | `moya/libmoya`, `scene` | 0214 | done |
| [17](#step-17--ies-profiles) | IES profiles | `moya/libmoya/light`, `scene` | — | done |
| [18](#step-18--renderman-lights-from-rib) | RenderMan's lights from RIB | `api/render/offline/rib`, `moya/libmoya/ri` | — | open |
| [19](#step-19--adaptive-sampling) | Adaptive sampling to a noise threshold | `moya/libmoya/film`, `kernel` | 0215 | open |
| [20](#step-20--checkpoint-and-resume) | Checkpoint and resume | `moya/libmoya/film`, `output`, `moya` | 0216 | open |
| [21](#step-21--speed) | Speed: samples a second against a baseline | `moya/bench` | — | open |
| [22](#step-22--the-reference-renders) | The reference renders | `moya/tests` | — | open |
| [23](#step-23--the-documents) | The reference pages for the integrator and lights | `docs` | — | open |

## What blocks what

```
6 Statistics ─────────────────────────────────────┐
7 Shading point ──> 8 BSDF ──┐                    │
1 ADR ───────────────────────┴──> 9 Integrator ───┴──> 10 Shaped lights ──> 11 NEE, MIS ──┬──> 12 Distant, points
2 ADR ──────────────────────────────────────────────> 10, 13                              ├──> 13 Mesh lights
                                                                                           ├──> 14 Dome light
                                                                                           └──> 17 IES
3 ADR ──> 15 Light tree (after 12, 13, 14) ──> 16 Linking
4 ADR ──> 19 Adaptive (after 9)
5 ADR ──> 20 Checkpoint (after 9, and after 19)
14 ──> 18 RenderMan lights

(15 to 20) ──> 21 Speed, 22 References ──> 23 Documents
```

Steps 1 to 8 depend on nothing but each other. Step 8 needs step 7's shading point, and step 9
needs step 8 and the record of step 1. Step 6 needs only the film, and every step from 9 on is
tested through it. Step 10 needs the integrator and the record of step 2, and step 11 needs a
light to sample. Steps 12, 13, 14 and 17 each add to the light sampling of step 11, and can run
beside each other. Step 15 needs every kind of light, and step 16 needs the tree. Step 18 needs
the dome light. Step 19 needs the integrator and the record of step 4. Step 20 needs step 19,
since a checkpoint holds which pixels have stopped. Each step updates its own section of the
reference documents, and step 23 checks that they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **The statistical oracle.** From step 9 on, a picture is compared with its expected value
  through step 6's comparison. An expected value is a closed form where one exists, and an
  estimate by another technique where none does.
* **One picture on every run.** A render on one thread and on every thread gives the same bytes,
  as ADR-0194 requires.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build.
* **No old picture changes.** The old hiders read RIB as before, so every committed PNG in
  `moya/tests/data/` stays as it is, and the editor's render window shows the same picture.

---

### Step 1 — The record: the path state

**The question.** What a path carries between stages, and how each device lays it out. On the
CPU the stages run back to back, so the state can live on one thread's stack. On a GPU each stage
is a kernel over a queue of paths, so the state lives in device memory between kernels. Its
layout there sets how fast a GPU reads it.

**The options:**

* **The state declared once, as a list of fields, and laid out by each device**, as Cycles'
  integrator state is declared in one template. The CPU lays it out as one structure per path. A
  GPU lays it out as one array per field.
* **One structure per path on every device**, kept in an array on a GPU, and reached by a queue
  of indices. The simplest, and a GPU reads whole structures where it needs one field.
* **Arrays per field generated from a description**, as pbrt-v4's `soac` tool generates its
  layouts. A tool in the build, and stage code that reads a field through generated accessors.

The record also settles what the state holds: the ray, the throughput, the wavelengths, the
depth of each kind of bounce, the last BSDF sample's density for MIS, and the sample's pixel and
index. It also settles how a path's draws take ZSobol dimensions: a fixed number for each bounce,
as Cycles allots them, or the next free dimension, as pbrt-v4 takes them.

**Drafted** as
[ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md):
the state declared once as a list of fields, laid out as a structure per path on the CPU and an
array per field on a GPU, as Cycles' integrator state is. A shadow ray is a state of its own.
Each bounce owns a fixed block of dimensions, as Cycles and pbrt-v4's wavefront integrator
allot them.

### Step 2 — The record: meeting a light

**The question.** How a ray that a BSDF sampled finds the light it meets. MIS needs the light's
emission and its density at that point. A rect, disk, sphere or cylinder light has a shape, but no
geometry in the scene holds it. A mesh light's shape is a prototype that the hierarchy already
holds.

**The options:**

* **A light's shape is a primitive of the hierarchy**, as pbrt-v4 attaches an area light to its
  shape. A hit on it names the light. The traversal's visibility then hides a light from a kind of
  ray, and a GPU back end intersects it as a custom primitive.
* **Lights tested beside the traversal**, in a loop over the lights after each nearest hit. Each
  light's shape is moya's code, and the hierarchy holds no light. Its cost grows with the number
  of lights.
* **A hierarchy of the lights' shapes alone**, traced beside the scene's, so a change to a light
  rebuilds nothing else.

The record also settles whether camera rays see a light's shape. UsdLux names no camera
visibility for its lights, and renderers differ in their default.

**Drafted** as
[ADR-0213](../adr/offline/0213-offline-a-lights-shape-is-a-primitive-of-the-hierarchy.md): each
light with a shape is an instance of a light prototype in the world's hierarchy, as pbrt-v4 and
Mitsuba 3 hold an area light's shape. A light's shape is hidden from camera rays unless the scene
sets it, and a dome light is seen, as Arnold and RenderMan default them.

### Step 3 — The record: the light tree

**The question.** How the light tree is built and what it holds. Conty Estevez and Kulla's tree
picks a light by a bound on its contribution at the shading point. Its nodes hold a box, a cone of
emission directions and an energy. The roadmap names the method, and leaves open what a leaf is.

**The options:**

* **A leaf for each light, and one for each emitting triangle of a mesh light**, as Cycles'
  light tree has since Blender 3.5. A mesh light of many triangles is sampled triangle by
  triangle.
* **A leaf for each light alone**, and a mesh light's triangles picked by area within it. A
  smaller tree, which samples a large mesh light poorly.
* **A tree for each emitting prototype, placed by its instances**, as the scene's hierarchy is.
  An instanced mesh light costs one tree, and each pick walks into it through the instance.

The record also settles four things. How distant and dome lights are picked beside the tree's
lights. How a node bounds a light that moves over the shutter. How light linking is held in the
tree. How a BSDF sample that meets a light finds the density the tree would have picked it with.
pbrt-v4's `BVHLightSampler` and Cycles' tree are the references.

**Drafted** as
[ADR-0214](../adr/offline/0214-offline-the-light-tree-has-a-subtree-per-emitting-prototype.md): a
top tree over the lights and the instances of mesh lights, and a subtree per emitting prototype,
as Cycles' tree holds instances. A tree for each unique set of linked lights, as Cycles builds
them. Distant and dome lights picked beside the tree as pbrt-v4 picks them, and one light a
bounce.

### Step 4 — The record: adaptive sampling

**The question.** How moya decides that a pixel has enough samples. ADR-0194 requires the same
picture on every run, so the decision has to depend on the samples alone, not on which thread
finished first. ZSobol gives a pixel's samples as runs of a power of two.

**The options:**

* **An error from two half pictures**, as Cycles measures it: every other sample also lands in a
  second film, and a pixel's error is how far its two halves differ. It costs a second film.
* **The variance of a pixel's mean**, from its second moment, as RenderMan's `PixelVariance`
  and Arnold's adaptive threshold measure it. Step 6 adds the moment. Its estimate is poor while
  a pixel has few samples.
* **Either, filtered over a pixel's neighbours** before it is compared with the threshold, so an
  isolated pixel that converged by chance keeps sampling.

The record also settles when a pixel's error is checked, and whether the threshold is in the
working space or after a display transform.

**Drafted** as
[ADR-0215](../adr/offline/0215-offline-a-pixels-noise-is-the-difference-of-two-halves.md): a
second beauty takes the first half of each run of a pixel's samples, and the error is the
difference of the two estimates, normalised as Cycles normalises it. A pixel is checked between
passes and stops with its four neighbours, as Cycles spreads its check. The threshold is in the
working space.

### Step 5 — The record: checkpoints

**The question.** What a stopped render writes so that it can resume, and what makes a resumed
render the same as one that never stopped. A checkpoint has to hold each pixel's sums, its
weight, its second moment, and whether adaptive sampling stopped it.

**The options:**

* **An EXR of the film's sums, unresolved**, with each pixel's weight and state as channels and
  the pass count in the header, as RenderMan writes a checkpoint it can recover from. Any EXR
  reader opens it.
* **A file of moya's own**, holding the film's memory as it is. Smallest and fastest, and no
  other tool reads it.
* **No checkpoint**, and a render split into ranges of samples whose pictures are merged, as
  Cycles' sample offset splits one. A merge needs the second moment and the weights as well.

The record also settles how a checkpoint names the scene it belongs to, so that a resume against
a changed scene is refused.

**Drafted** as
[ADR-0216](../adr/offline/0216-offline-a-checkpoint-is-an-exr-of-the-films-sums.md): an OpenEXR
file of its own holding the film's unresolved sums as floats, as Karma writes a checkpoint apart
from its outputs. Its header holds the pass count and a hash of the scene's description and the
film's channels, and a resume whose hash differs is refused.

### Step 6 — The statistical comparison

The oracle every later step is held to. It needs only the film.

**Done.** `kernel::film::Pixels::add()` adds three moments beside each pixel's weight, and a
snapshot's `error(x, y)` gives each pixel's standard error of its luminance from them.
`film/Compare.h` gives `luminance()`, `mean()` over a region, `sigmas()` against a value or another
estimate, and `furthest()` over a picture against an expected value or another picture. Four
things the list below did not settle:

* **The moments are three sums, not one.** A filter's weights differ in sign, so a weighted
  mean's variance needs the squared weights times the squared luminance, times the luminance,
  and alone. The error is scaled by the effective count over one less, so samples of equal
  weight give the samples' variance over their count.
* **A pixel of fewer than two samples' weight is not measured.** Its error is infinite, a
  comparison counts it as unmeasured, and it lies no distance from anything.
* **The error treats ZSobol's samples as independent**, so it overstates their error, and a
  comparison errs towards passing. ADR-0215 measures adaptive sampling by two halves for that
  reason.
* **A NaN is furthest.** A pixel or an expected value that is not a number makes the comparison
  NaN, which fails any bound.

[Film.md](../offline/Film.md#a-pixels-noise) and
[Testing.md](../contributing/Testing.md#moya-references) state the rules.

* **The film holds each pixel's second moment**: a weighted sum of each sample's squared
  luminance, in a channel of its own. A snapshot resolves it with the beauty.
* **A pixel's standard error** follows from its mean, its second moment and its weight.
* **A comparison of a picture with an expected value**, per pixel or over a region, passes when
  the difference is within a number of standard errors. The number is set by the test, and a
  failure names the worst pixel.
* **A comparison of two pictures**, by the same rule over both errors. Section 10 compares a GPU's
  picture with the CPU's through it, as ADR-0194 requires.

**Tests:** a picture of known mean and variance passes against its mean. The same picture moved
by four standard errors fails. Two pictures of one scene with different seeds pass against each
other.

### Step 7 — The shading point

A hit holds no point, normal or position on the surface. The shade stage computes them once, for
the nearest hit.

**Done.** `kernel::surface::Surfaces::at()` turns a hit into a `ShadingPoint` as its surface stood
at the hit's time. It composes the chain of instances at that time, and gives the position, the
geometric and shading normals, the frame, the corners' weights and the face. `geometry::Surfaces`
holds a scene's prototypes on a device for it, with the primvars `N` and `Cs`. Since nothing
lights a scene yet, the terminator is tested by where the leaving point lies, not by a picture.
Four things the list below did not settle:

* **The surfaces are held apart from any traversal.** The CPU traversal's arrays are device code
  that kernel code cannot include, and a GPU's are inside HIP RT or OptiX. Shading reads its own
  copy of each geometry and instance, as Cycles keeps its objects and meshes apart from Embree
  and OptiX.
* **A primvar is spread over its primitives when it is gathered**: one value for the geometry, one
  per primitive, or three per triangle at its corners. A triangle's points are held that way
  already. Each primvar is read at its first sample.
* **A colour primvar is converted into ACEScg** from the space its value names, when the surfaces
  are made, through the colour configuration.
* **The leaving point lies on Phong's surface, not on the sphere a mesh's normals suggest.**
  Hanika's offset moves the point out of each corner's tangent plane, which reaches Phong's
  surface through the corners and their normals. An octahedron's bulges past its sphere at the
  middle of each edge, to a length of 1.0607.

[Surfaces.md](../offline/Surfaces.md) states the rules.

* **A shading point** holds the hit's position, its geometric normal, its shading normal, a frame
  around the shading normal, its `uv`, and the instance chain's transformation at the ray's time.
* **The shading normal** of a mesh is its `N` primvar where it has one, interpolated as its class
  says. A subdivision surface takes its limit surface's normals, and a quadric or a curve its
  own.
* **Primvars on the device.** Each geometry holds the primvars shading reads on the device: `N`
  and `Cs` now. The layout holds any primvar of any class, so section 5 adds texture coordinates
  without changing it.
* **A ray leaves a smooth surface from a shifted point**, by Hanika's offset of the shadow
  terminator, so a coarse mesh with smooth normals does not shade in facets at the shadow's
  edge.

**Tests:** a sphere's shading normal at every hit is its radius direction. A mesh with an `N`
primvar shades with the interpolated normal, and one without with its face's. A face-varying `Cs`
is read at its corner. A coarse sphere lit from the side shows no facets along the terminator.

### Step 8 — The BSDF interface and a diffuse lobe

The integrator calls a BSDF, and section 5 supplies the library. This step builds the interface
and one lobe.

**Done.** `kernel::bsdf::Bsdf` evaluates, gives a density and samples in world space, through a
`Model` it switches over. Its diffuse model is Lambert's, sampled by the cosine through the
concentric mapping. `bsdf::atSurface()` builds one at a shading point. The gather reads each
instance's material, and a shading point names its chain's. Three things the list below did not
settle:

* **The primvar comes before the surface node.** The list put the node's `Cs` first. RI's `Cs`
  on a primitive overrides the attribute colour, which the RIB client gives the surface node, so
  the primvar wins.
* **Materials reach the device.** A material holds its surface node's `Cs` in ACEScg. A shading
  point names the material of the innermost instance of its chain that names one, so a group's
  member takes the material of the instance that places the group.
* **The diffuse lobe reflects on either side** that light leaves towards, as pbrt's does. A
  reflection that would cross the geometric surface is refused on every model.

[Surfaces.md](../offline/Surfaces.md#the-bsdf) states the rules.

* **The interface evaluates, samples and gives a density**, in the shading frame, each for the
  path's four wavelengths. It is a tagged type with a `switch`, as kernel code has no virtual
  call.
* **A sample reports its kind**: diffuse, glossy, specular or transmission, so a depth limit can
  be kept for each.
* **A diffuse lobe** samples by the cosine of the angle from the normal. Its reflectance is the
  surface node's `Cs`, or the geometry's `Cs` primvar, or a mid grey.
* **The material's surface terminal chooses the lobe.** Any shader gives the diffuse lobe until
  section 5. A material with no surface node is grey.

**Tests:** the diffuse lobe's samples follow its density, by a chi-squared test over the
hemisphere. The lobe conserves energy: its reflectance integrated over the hemisphere is its
colour. A sample below the geometric surface is refused.

### Step 9 — The integrator stages

The first picture the new core lights. The scene's only light is a constant environment, which
the white furnace test needs.

**Done.** `kernel::integrate::paths` runs the generate, intersect, shade, scatter and accumulate
stages back to back over a `PathState`, whose fields `MOYA_PATH_FIELDS` declares once.
`integrate::render()` renders a scene on the CPU in passes, and `moya --paths` writes the picture.
Each case was run against broken code as well, one break at a time. Throughput not divided by the
density failed the furnace, the grey sphere and the roulette case. Survivors not weighed up failed
the roulette case. A limit off by one, in all or of a kind, failed the depth limit. A ray left
unoffset failed the furnace and the grey sphere. Four things the list below did not settle:

* **The state's list is an X-macro**, as Cycles' integrator state is. `MOYA_PATH(state, field)`
  reaches a field, and the CPU's layout defines it as a member. A GPU's layout defines it as an
  index into the field's array.
* **The integrator runs through the CPU's library.** Only the libraries for each instruction set
  define the CPU's traversal, so `paths` is an entry point of their table, whose version is now
  5.
* **The depth limits are Cycles'**: twelve bounces in all, four diffuse and four glossy, twelve
  specular and twelve transmission, and Russian roulette from the third bounce.
* **`moya --paths` writes one EXR of the beauty**, to `--output` or `paths.exr`, as `--hits`
  writes its view. Writing the outputs the scene names, with their channels, waits for the AOVs
  of section 6, which fill those channels.

[Integrator.md](../offline/Integrator.md) states the rules, and the moya manual's
[CommandLine.md](../manual/moya/CommandLine.md) names `--paths`.

* **The stages**, as ADR-0192 names them: generate, intersect, shade, scatter and accumulate,
  over the path state step 1 decides. The CPU runs them back to back for one path at a time.
  Sampling lights waits for step 11.
* **Generate** draws the wavelengths, the pixel filter's offset, the lens and the time, from
  ZSobol by the dimensions step 1 allots.
* **A depth limit for each kind of bounce**, diffuse, glossy, specular and transmission, and one
  for all of them, in the scene's `options`. Only diffuse bounces exist until section 5.
* **Russian roulette** ends a path by its throughput after a least depth, and divides the
  survivors' throughput by their chance of surviving.
* **A constant environment**: a path that leaves the scene meets a radiance given in the
  `options`, until step 14 brings the dome light.
* **`moya --paths`** reads a RIB file through the scene client and renders it through the new
  core, to the outputs the scene names. `moya` with no flag renders through the old hiders as
  before.

**Tests:** a white diffuse sphere inside a white environment is white at every pixel, within
three standard errors. A grey sphere of albedo a in a white environment is a at every pixel it
covers, since a convex surface never meets itself. A depth limit of zero shows the environment
and leaves the sphere black. A render on one thread and on eight gives the same bytes.

### Step 10 — Lights with a shape

**Done.** `geometry::gather()` places each rect, disk, sphere and cylinder light's shape through
an instance in the world, after every other, with the light's index in the scene. A disk, sphere
or cylinder is a quadric, and a rect is two triangles. `light::Lights` holds what each emits, and a
path that meets one gathers it and ends. `PathParams` gains the lights, so the CPU library's table
is now version 6. Each case was run against broken code as well, one break at a time. Radiance
not divided by the area failed both normalize cases, and a sphere's area 5% off failed the
second. A cylinder left along z, or its keys left unturned, failed the cylinder's cases. A light
seen by every camera ray failed the closed form and the visibility case, and a rect lit from
behind failed the visibility case. A cone ignored, or its edge moved, failed the cone's cases. A
radiator left at one at 560 nm failed the temperature's luminance at 1000 K, and a light's shape
not built again failed the rebuild. Five things the list below did not settle:

* **A light's shape is hidden from shadow rays**, as Arnold's and Cycles' are, so it casts no
  shadow from another light. Step 11's shadow ray towards a light then needs no end short of the
  shape.
* **A colour temperature tints a radiator of a luminance of one by the colour.** UsdLux names a
  colour for each temperature, white at 6500 K, which a spectral renderer has no use for. The
  radiator is Planck's itself, so 6504 K is near D65 and not on it.
* **A disk light emits from one face too**, the one its -z axis points to, as UsdLux says.
* **A cone of 90 degrees with no softness shapes nothing**, which is UsdLux's fallback. Taken
  literally, UsdLux's cone would cut a sphere light's back half at its fallback.
* **A sphere's and a cylinder's area in the world are summed over their normals**, since an
  ellipsoid's has no closed form. The sum is exact under a turn and a scale of the same size on
  every axis.

[Lights.md](../offline/Lights.md) states the rules, and [Scene.md](../offline/Scene.md) names the
lights' new `camera`, `enableColorTemperature` and `colorTemperature`.

* **Rect, disk, sphere and cylinder lights** as the record of step 2 decides, met by the paths a
  BSDF samples.
* **A light's radiance is UsdLux's**: its intensity times two to its exposure, times its colour
  as an illuminant spectrum. With `normalize`, the power is the same at any size.
* **A colour temperature**, as UsdLux's `enableColorTemperature` and `colorTemperature`. The
  scene's lights gain both. The spectrum is Planck's at that temperature, scaled to a luminance of
  one.
* **A cone of emission**, as UsdLux's `shaping:cone:angle` and `shaping:cone:softness`.
* **A rect light emits from one face**, and its texture waits for step 14, which reads images.
* **Camera visibility**, as the record of step 2 settles.

**Tests:** a diffuse plane under a disk light reaches the closed form of the disk's irradiance, by
BSDF sampling alone. A light with `normalize` at twice its size gives the same irradiance far
away. A colour temperature of 6504 K is near D65. A light outside its cone lights nothing.

### Step 11 — Next event estimation and MIS

**Done.** `integrate::direct` draws a light sample at each shading point, and `metShare` weighs a
BSDF sample that meets a light. `light::sample()` and `light::density()` in
`kernel/light/Sampling.h` draw a point on each shape and give its density by solid angle. The
options gain `lightSampling`, which is `mis`, `light` or `bsdf`, and the path state gains the
shading point a BSDF sample left. `PathParams` gains the cdf of the lights' powers, so the CPU
library's table is now version 7.

Each case was run against broken code as well, one break at a time. Either MIS weight left at
one failed the closed form and the MIS case. A shadow ray left out failed the shadow case. A light
sample past either depth limit failed the depth case. Always the first light, or a chance of one,
failed the case that compares light samples with BSDF samples over three shapes. A rectangle's
azimuth offset of the wrong sign, an ellipse inverted with no Newton step, a sphere's narrow cone
without its expansion, and an area density without its stretch each failed their shape's
chi-squared case. Six things the list below did not settle:

* **The spherical ellipse is exact.** Its solid angle comes from Legendre's third kind through
  Carlson's R_F and R_J, and each sample inverts it by Newton's method within a bracket. The
  paper's production form reads a table and drops samples outside the ellipse, which biases the
  light. The cone's frame and solid angle were first checked against a sum over the disk, and the
  sampler by a chi-squared test, outside the tree. The integral holds whichever semi-axis is the
  longer, so the axes are not sorted.
* **A shape its transformation shears or stretches is sampled by area.** A spherical rectangle
  needs right angles, a spherical ellipse a round disk, and a cone a round sphere. A rect or disk
  below a thousandth of a steradian is sampled by area too, since its solid angle loses precision
  in a float there.
* **Every light emits from one face.** A sphere or cylinder light emits from its outer face, so
  light sampling never brings in light from a face a BSDF sample could not meet.
* **A light's power for its pick leaves its cone out**: its radiance's luminance times its area
  times pi.
* **A light sample at a bounce a depth limit refuses is not drawn**, so MIS weighs two strategies
  that reach the same paths.
* **MIS's error is not the least of the three in every scene.** Under a small light it lies 1%
  above light sampling alone, and under a large one 60% above BSDF sampling alone, as Veach's
  bound allows. Its case holds it within twice the better strategy's error and below 0.6 of the
  worse one's.

[Lights.md](../offline/Lights.md) states how each light is drawn and sampled, and
[Integrator.md](../offline/Integrator.md#lights) the strategies.

* **Each bounce samples a light**, and casts a shadow ray through `any()`. One light is picked
  in proportion to its power until step 15.
* **Each light samples by solid angle where a method exists.** A rect light by Ureña, Fajardo
  and King's spherical rectangle. A sphere light by the cone it subtends. A disk light by Guillén
  and others' spherical ellipse. A cylinder light by area, converted to solid angle.
* **MIS by the power heuristic** weighs the light sample against the BSDF sample. A BSDF sample
  that meets a light finds the light's density through step 2's hit.

**Tests:** the disk light's plane of step 10 reaches the same closed form by light sampling
alone, by BSDF sampling alone, and by MIS. MIS's standard error is the least of the three for a
small light and for a large one. Each light's samples follow its density, by a chi-squared test.

### Step 12 — Distant lights and points

**Done.** `light::Lights` builds points and distant lights beside the lights with a shape, and
every light holds its own placement over the shutter. `light::sample()` draws a point where it
stands, a distant light of no angle along its direction, and one with an angle uniformly in its
cone. A light sample of either kind of delta light takes its whole weight, and a ray that leaves
the scene meets the distant lights with an angle. `PathParams` gains the lights' keys and their
distant lights, so the CPU library's table is now version 8. Six things the list below did not
settle:

* **A point is the sphere it stands for.** UsdLux leaves a point's units open, and calls
  `treatAsPoint` a hint. A point's radiant intensity is the sphere's, its radiance times pi r^2,
  or a quarter of its emission with `normalize`. A renderer that ignores the hint then gives the
  same brightness, and a shrinking normalised sphere converges to its point. Karma reads the
  intensity as candela instead, a factor of four from a small normalised sphere.
* **RI's `pointlight` and `spotlight` are normalised points at four times RI's intensity**, so
  their radiant intensity is RI's. A point of no radius without `normalize` emits nothing. The
  editor's translator makes its points the same way, and five committed scene descriptions gain
  the new values.
* **A distant light's size is UsdLux's**: pi sin^2 of its half angle, or pi (2 - sin^2) past a
  quarter turn. With `normalize`, its emission is the irradiance of a plane that faces it at any
  angle. Of no angle, its emission is that irradiance with `normalize` or without. Its intensity
  falls back to 1, as every light's does, not to OpenUSD's newer 50000.
* **A distant light's power is measured over the world's reach**: its irradiance times the area
  of a disk of the radius of a sphere about the world's bounds, as pbrt-v4 measures it.
* **A delta light is sampled whatever the strategy**, `bsdf` included, since nothing else finds
  it. Camera rays see no distant light.
* **A cone keeps its precision however narrow.** One less the cosine of the half angle is twice
  the sine squared of its half. A sample's sine comes from one less its cosine. A cone of the
  sun's half degree is then sampled as exactly as a wide one.

[Lights.md](../offline/Lights.md#points) states the rules, and
[Integrator.md](../offline/Integrator.md#lights) how a path finds each light.

Each case was run against broken code as well, one break at a time. A delta light weighed by MIS
failed both closed forms, the soft shadow and the shrinking sphere. A delta light left unsampled
under `bsdf` failed both closed forms. A normalised point not quartered failed the point's closed
form and its limit, and one not scaled by its sphere's area failed the limit. A shadow ray towards
a distant light stopped short failed the soft shadow. A distant light's size taken from its whole
angle, or a ray leaving the scene that met no distant light, failed the cone's closed form and the
soft shadow. A cone's sine taken from its cosine, or its density off by 5%, failed the cone's
chi-squared case. A point's cone ignored failed the cone case, a distant light shining the wrong
way failed five cases, and a point that kept its shape failed the gather's case.

* **A distant light with an angle** samples the cone its angle gives. An angle of zero is a
  single direction, which only light sampling reaches.
* **A sphere light of radius zero** is a point, which only light sampling reaches, as RI's
  `pointlight` maps to it. UsdLux's `treatAsPoint` makes any sphere light one.
* **MIS gives a light no BSDF sample can reach** its whole weight.

**Tests:** a diffuse plane of albedo ρ under a distant light of no angle and irradiance E reaches
ρ E cos θ / π. A sphere light shrinking towards zero converges to the point. A distant
light's angle softens a shadow's edge by the width the angle gives.

### Step 13 — Mesh lights

**Done.** `light::Lights` makes a light of each chain of instances from the world that places a
geometry a mesh light names, and draws its triangles in proportion to their area in the world.
`light::triangleSample()`, in `kernel/light/SphericalTriangle.h`, samples a triangle by Arvo's
method as pbrt-v4 gives it, within pbrt-v4's bounds on its solid angle, and by area outside them.
A path that meets an emitting geometry gathers its share of the light and scatters on.
`Surfaces::chainAt()` and `Surfaces::corners()` give a chain's transformation and a triangle's
corners at a time. `PathParams` gains the triangles' chances and the mesh lights by instance, so the
CPU library's table is now version 9. Five things the list below did not settle:

* **The geometry emits, wherever it is placed.** The scene's `meshLight` names a geometry, and
  RIB's `AreaLightSource` also places it through an ordinary instance. Each chain of instances
  that places it is a light of its own, as the record of step 3 holds them. The mesh light's
  `transform` and cone are not read.
* **A triangle emits from the side its winding faces**, the side a shading point's geometric
  normal faces, as every other light emits from one face.
* **With `normalize`, the radiance is divided by the chain's area in the world**, as UsdLux
  divides a mesh light by its surface area in the world, when the shutter opens.
* **Camera rays see the emission only while the light's `camera` is 1.** The geometry itself is
  always seen, by its material, since it is a surface of the scene.
* **A geometry with no triangles emits nothing**, and the logger reports it. A quadric, curves
  and points have no triangles to sample.

[Lights.md](../offline/Lights.md#mesh-lights) states the rules, and
[Integrator.md](../offline/Integrator.md#lights) how a path meets a mesh light.

Each case was run against broken code as well, one break at a time. A light sample or an MIS
weight that left out the triangle's chance failed the rect light's picture and four other cases.
Triangles drawn evenly, not by area, failed the rect light's picture, the instanced pair and the
normalised square, once the square was cut into triangles of unequal area. Emission seen from
behind failed the one-sided case, and a triangle sampled from behind the triangle's chi-squared
case. Camera rays that always saw the emission failed the camera case. A chain placed by its
outermost instance alone failed the instanced pair, by light samples alone, since under MIS the
BSDF still finds the light without bias. A mesh light placed only where it stood at the shutter's
opening failed the moving light. Normalising by the area in the geometry's own space failed the
normalised square. Arvo's second draw warped failed the chi-squared case and three pictures. No
emission gathered at a hit failed six cases.

* **A mesh light emits from its prototype's triangles**, placed by the instances the scene's
  hierarchy already holds, at the ray's time.
* **A triangle samples by its spherical triangle**, by Arvo's method, and by area when it
  subtends too small or too large an angle.
* **A triangle is picked in proportion to its power** until step 15.
* **Subdivision surfaces and moving meshes emit** from the triangles the gather made.

**Tests:** a mesh light of two triangles in a square gives the irradiance of a rect light of that
square. A mesh light instanced twice gives twice the light. A moving mesh light lights the
places it passes.

### Step 14 — The dome light

**Done.** `output::readExr()` reads an image's R, G and B as floats, with the space its
chromaticities name, and `light::internal::Textures` holds each image a light reads in ACEScg.
`kernel/light/Dome.h` maps a dome's image in latitude and longitude, gives its radiance, and
draws a texel by one of two ways, with Karlik and others' compensation under MIS. A ray that
leaves the scene meets every dome by its share, and the options' `environment` is gone.
`settingsOf()` no longer takes the colour configuration. `colour::Config::spaceOf()` finds a space
by its interop ID. `PathParams` gains the images, their texels and sums and the domes, so the CPU
library's table is now version 10. Six things the list below did not settle:

* **An image is converted from the space its chromaticities name**, among the six the EXR writer
  names, and is linear Rec. 709 with none, as OpenEXR takes a file. Chromaticities of another
  space are reported and read as linear Rec. 709.
* **An image is read as a texture over [0, 1]**, each texel the box it covers, as renderers read
  a latitude and longitude map. OpenEXR's own envmap functions put the texel centres on the
  image's edges instead. Texels are not filtered until section 5's texture cache.
* **A texel is drawn by its luminance times the solid angle it covers**, the exact band of
  latitude it spans, so a texel's chance is in proportion to its share of the dome's light.
* **The compensated way is drawn under MIS alone.** Light samples alone draw by the full way,
  since nothing else would find the directions compensation leaves out.
* **`format` is UsdLux's `texture:format`.** `automatic` reads the file's envmap attribute, and
  a cube map is reported and not lit, as is any layout but latitude and longitude. A dome's
  `poleAxis`, which UsdLux's newer schema adds, is not read: the dome's +y is its pole.
* **The environment option is gone**, so twenty-two committed scene descriptions lose it, and
  the furnace cases light their sphere by a white dome of no image.

[Lights.md](../offline/Lights.md#dome-lights) states the rules, and
[Integrator.md](../offline/Integrator.md#lights) how a path meets a dome.

Each case was run against broken code as well, one break at a time. Light samples alone drawn by
the compensated way failed the varied image. A density without the cosine of its latitude failed
the furnace. Longitude read the wrong way failed the orientation, the bright texel and the varied
image. A dome met whole under MIS failed the furnace, the bright texel and the varied image. A
dome read without its turn failed the bright texel, once that case turned the dome a quarter as
well as a half, since a half turn lights nothing either way. A rect's texture left out or turned
failed the rect's case. Every format read as latitude and longitude failed the layout case. A
texel left out of the light's colour failed the bright texel and the rect. An image's green read
as its red failed the reader's case and four others.

* **OpenEXR reads a light's image**, beside its writer, into device memory whole. Section 5's
  texture cache takes over, and a dome light then reads through it.
* **A dome light** is the environment, in UsdLux's latitude and longitude layout and orientation.
  Other layouts are reported and not lit. It replaces step 9's constant environment.
* **An importance-sampled environment**: a piecewise-constant distribution over the image's
  luminance and the solid angle of each texel. Karlík and others' MIS compensation takes the
  average from it.
* **A rect light's texture** reads through the same images, and is sampled by area.

**Tests:** a dome light of one constant colour gives step 9's furnace. A dome light of one bright
texel lights a plane as a distant light from that direction does. Light sampling, BSDF sampling
and MIS reach the same picture of a varied image.

### Step 15 — The light tree

**Done.** `kernel/light/Tree.h` holds the tree's nodes, the importance of a node at a shading
point, and the walks down and up. `light::internal::build()` builds a tree on the host by the
surface area and orientation heuristic. `light::Lights` builds a top tree over every light with a
place, and a subtree over the triangles of each geometry a mesh light makes emit, which every
light placing that geometry shares. `Lights::pickByTree()` draws a light and a triangle with one
random number, and `Lights::chanceByTree()` gives the chance. The path state gains the shading
normal, which a BSDF sample that meets a light is weighed by. `PathParams` gains the tree, so the
CPU library's table is now version 11. Five things the list below did not settle:

* **The options' `lightPicking` keeps drawing by power**, beside `tree`, the fallback, as pbrt-v4
  keeps its power light sampler. The tests compare the two.
* **What moves is bounded by every direction**: a light whose rotation or scale changes over the
  shutter, a mesh light whose chain turns, and a geometry whose points move. Their boxes are
  swept over the shutter, as the hierarchy sweeps them.
* **A node's distance is held to its box's half diagonal**, not to pbrt-v4's half the diagonal's
  length, which mixes a length with a length squared.
* **A rect or disk light's cone narrows its falloff**, and a sphere's, a cylinder's or a point's
  sets its spread, its softness the falloff past it.
* **The tree is built on one thread**, so it depends on the scene alone. Each render builds it
  again with the lights.

[Lights.md](../offline/Lights.md#the-light-tree) states the rules.

Each case was run against broken code as well, one break at a time. The walk up taking the left
child's chance for both children failed the chance the tree reports and the picture by power. A
rect light that emits along its axis alone failed the picture by power. It did so once that case
drew by light samples alone as well as by both, since BSDF samples find a light the tree leaves
out. A
mirrored chain's triangles facing the other way failed it too. A BSDF sample weighed without the
shading normal failed it by both. A node's importance without its distance failed the error under
many small lights. The tree's share left out of the count failed the case of distant and dome
lights alone, which the first build of the cases did not hold. Nothing in the build reads the
device's threads, so the case of two thread counts has no break to fail it.

* **The tree** as the record of step 3 decides, after Conty Estevez and Kulla: each node a box, a
  cone of directions and an energy, built by a surface area and orientation heuristic.
* **A light is picked by walking the tree**, choosing a child by a bound on its contribution at
  the shading point, with one random number.
* **A BSDF sample that meets a light** finds the density the tree would have picked it with.
* **The tree is a consumer of the scene**, rebuilt when a light changes, as the hierarchy is.
* **The same tree on every run**, whatever the threads do.

**Tests:** a scene of many small lights, of which one is near a surface, renders with a lower
error through the tree than by power at the same sample count. The tree's picks follow the
density it reports. Two builds on different thread counts are equal node for node.

### Step 16 — Light linking and shadow linking

**Done.** `light::internal::linking()` walks the chains of instances from the world once. It
finds each set of linked lights a shading point can lie in, and the steps from set to set at the
instances the lights name. `kernel::light::link::Links` follows the steps along a hit's chain.
Each set has a top tree of its own and a list of distant and dome lights, and the mesh lights'
subtrees are shared by every set. The path state gains the shading point's set, which a BSDF
sample that meets a light is weighed by. The scene's lights gain `shadowIncludeRoot` and
`shadowInstances`, so fifteen committed scene descriptions gain them. The CPU library's table is
now version 12.
Four things the list below did not settle:

* **A light names an instance at any depth of a chain**, and lights everything placed through
  it, as a UsdLux collection takes in a prim's descendants.
* **A shadow-linked light is found by light samples alone**, as a point is. A BSDF's direction
  towards it would stop at what its shadow ray passes through, so the two would not agree.
  Cycles traces a second ray for such a light, which this step does not.
* **A shadow ray passes through at most `kMostPassed` instances** that cast none of its light's
  shadows, and past that is blocked.
* **Drawn by power, a light that does not light the set gathers nothing**, rather than having a
  table of powers for each set. The tree is the fallback, and power is there to compare with.

[Lights.md](../offline/Lights.md#light-linking) states the rules.

Each case was run against broken code as well, one break at a time. Every set's tree holding
every light failed both cases of linked lights, as the list below asks of a tree that ignores
linking. Every set listing every distant and dome light failed the case of two spheres, through
its dome. A chain that never stepped out of set 0 failed both, and so did a BSDF sample that
ignored the links. A walk that did not enter groups failed the case of a group. A shadow cast by
every instance failed the shadow link, and so did a shadow-linked light weighed against the
BSDF.

* **Light linking** as the scene holds it: a light lights only the instances its `includeRoot`
  and `instances` allow. The tree leaves out what a shading point's instance is not linked to, as
  the record of step 3 decides.
* **Shadow linking**, as UsdLux's `shadow:include` collection names it. The scene's lights gain
  it. A shadow ray from a light passes through an instance the light's shadow link leaves out.

**Tests:** a light linked to one of two spheres lights that sphere alone. A sphere left out of a
light's shadow link casts no shadow from it, and still casts one from another light. A tree that
ignores linking fails the first case.

### Step 17 — IES profiles

**Done.** `light::parseIes()` reads LM-63 of any year, and `light::internal::Profiles`
reads each file once. `kernel/light/Profile.h` gives a profile's candela along a direction,
interpolated between its angles. `Lights::shaping()` shapes what a light sample brings in and
what a BSDF sample that meets the light gathers. A light's power is weighed by its profile's mean
over the sphere. The scene's area lights gain `iesFile`, `iesAngleScale` and `iesNormalize`, so
seven committed scene descriptions gain them. The CPU library's table is now version 13. Five
things the list below did not settle:

* **ADR-0187 needs no amending.** It names the reference library of each format that has one,
  and LM-63 has none. Reading it in moya rather than through a small library is easy to reverse,
  so it is not a record of its own.
* **A vertical angle is measured from the light's -z**, its nadir, and a horizontal angle from
  its +x towards its +y. UsdLux names neither axis. Its lights emit along -z, and it calls a
  profile of a positive angle scale one aimed down.
* **`iesNormalize` divides by the profile's mean over the sphere.** UsdLux writes it as a product
  with the profile's power, which would not keep the light's power as its own words say.
* **Only type C photometry with no tilt or an included one is read.** Types A and B, and a tilt
  in another file, are reported, and the light is not shaped.
* **A mesh light's profile is not read**, as its cone is not.

[Lights.md](../offline/Lights.md#ies-profiles) states the rules.

Each case was run against broken code as well, one break at a time. A vertical angle measured
from +z failed the profile's orientation and its angle scale. A horizontal angle measured from +y
failed the horizontal case, once that case lit a wedge of 44 degrees, since a quarter turn is
the same either way. A cylinder shaped in its prototype's space failed the orientation. The angle
scale left out failed its case, and so did normalisation left out. A light sample left unshaped
failed three cases, and a light a BSDF sample meets left unshaped failed the orientation. A
quarter span left unfolded failed the uniform profile, and an included tilt misread failed the
reader's case.

* **An IES file**, as UsdLux's `shaping:ies:file`, `shaping:ies:angleScale` and
  `shaping:ies:normalize` name it. The scene's lights gain the three.
* **moya parses LM-63 itself.** ADR-0187 names a reference library for each format that has one,
  and IES has none. The step checks whether the record needs amending.
* **A profile shapes the light's emission** by direction, and light sampling weighs it.

**Tests:** an IES file of uniform candela leaves a light as it was. A profile that is dark below
its horizon lights nothing below a sphere light. A malformed file is reported and the light is
unshaped.

### Step 18 — RenderMan lights from RIB

* **The RIB reader reads `Light`**, as RenderMan's RIB names a light and its handle.
* **The scene client maps RenderMan's lights to UsdLux's**: `PxrRectLight`, `PxrDiskLight`,
  `PxrSphereLight`, `PxrCylinderLight`, `PxrDistantLight`, `PxrDomeLight` and `PxrMeshLight`,
  with their intensity, exposure, colour, temperature, normalisation and texture.
* **Another light shader is logged** and makes no light, as `LightSource` does.
* **The old hiders ignore `Light`**, so no picture they make changes.

**Tests:** each RenderMan light makes its UsdLux light with its parameters, in the scene's
description. A `Light` inside an attribute block is turned off by `Illuminate`, as a
`LightSource` is.

### Step 19 — Adaptive sampling

* **A pixel stops sampling** when its error, as the record of step 4 measures it, falls below a
  threshold in the scene's `options`. The samples per pixel are the most it takes.
* **Each pixel's error is checked between passes**, from the samples the passes added, so the
  same pixels stop on every run.
* **A least number of samples** comes before the first check.
* **The film holds each pixel's sample count**, as a channel an output can write, and the half
  beauty the record of step 4 measures against.

**Tests:** a scene of a flat colour and a noisy region stops the flat colour's pixels first. A
picture sampled adaptively is within its threshold of one sampled to the most. Two runs on
different thread counts stop the same pixels.

### Step 20 — Checkpoint and resume

* **A render writes a checkpoint** as the record of step 5 decides, after each pass or at an
  interval the command line gives.
* **`moya --paths --resume`** reads a checkpoint and continues at the next pass.
* **A checkpoint of another scene is refused**, by the scene's name the record settles.

**Tests:** a render stopped after two passes and resumed to eight is the same, byte for byte, as
one of eight passes. A resume against a changed scene is refused. A checkpoint of a render that
adaptive sampling stopped keeps its stopped pixels.

### Step 21 — Speed

* **`moya_bench` gains a lit scene**: a diffuse room of a million triangles under a dome light
  and a few shaped lights. It reports samples a second, and rays a second over the paths.
* **The baseline** in `moya/bench/Baseline.md` gains the figures, from the same machine.
* **The share of time in traversal** is reported, so the open traversal item in
  [TODO.md](../TODO.md#offline-rendering) can be weighed against the integrator.

**Tests:** the benchmark's short pass times the lit scene.

### Step 22 — The reference renders

* **A set of scenes made for the path tracer**, as RIB files in `moya/tests/data`: the furnace, a
  room under a dome, every light kind, linked lights and an IES profile. Step 18 lets each name
  its lights in RIB.
* **Each is rendered and committed** as an EXR, at a sample count that keeps the files small.
* **The suite compares each render with its committed one** through step 6's comparison of two
  pictures, so a change of rounding between compilers passes and a change of light fails.

**Tests:** the committed renders. Each was compared with a broken light as well, to show that the
comparison fails it.

### Step 23 — The documents

* **New reference pages**: `docs/offline/Integrator.md` for the stages, the path state, depth
  limits, Russian roulette and adaptive sampling, and `docs/offline/Lights.md` for each light,
  its units, its sampling, the tree and linking. The docs site's summary lists them.
* **[Film.md](../offline/Film.md)** names the second moment, the sample count and checkpoints.
* **[Scene.md](../offline/Scene.md)** names the lights' new parameters and the options' depth
  limits and threshold.
* **[Rib.md](../offline/Rib.md)** names `Light` and RenderMan's lights.
* **[The offline README](../offline/README.md)**'s section on the new core says that it lights.
* **[Testing.md](../contributing/Testing.md)** names the statistical comparison and the reference
  renders.
* **The moya manual's [CommandLine.md](../manual/moya/CommandLine.md)** names `--paths` and
  `--resume`.
* **Each record's `Documented in` line** names the reference page that states its rule.
* **The roadmap's section 4** links here, and says so when this plan closes.
