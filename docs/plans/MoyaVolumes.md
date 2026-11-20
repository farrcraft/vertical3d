# Moya Volumes — Media, Subsurface, Hair and Volumetric Lights

Drafted 2026-11-16 against `da1fdf28`. This plan takes up section 8 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, light in moya travels
through what lies between surfaces, not only from one surface to the next. A path scatters and is
absorbed inside a medium: a homogeneous one, such as fog or coloured glass, or a grid read from an
OpenVDB file, such as smoke or a cloud. Media nest, so a liquid in a glass is lit as both. A skin
or a marble scatters below its surface by a random walk, a hair scatters by its own model, and a
fire lights the scene from its volume. RIB names all of it, and the compositing outputs see it.

The rebuild's decisions are ADR-0186 to ADR-0194, and the decisions of its plans are ADR-0195 to
ADR-0241, all accepted. This plan follows six of them:

* [ADR-0186](../adr/offline/0186-offline-moya-renders-by-path-tracing-alone.md): moya is a
  unidirectional path tracer, so a medium is lit by paths through it, not by a pass of its own.
* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md): moya
  implements its own tracking, phase functions, random walk and hair model. OpenVDB reads the
  grid files.
* [ADR-0189](../adr/offline/0189-offline-light-is-carried-as-sampled-spectra.md): a path carries
  four wavelengths, so a medium whose density differs by wavelength is tracked at all four.
* [ADR-0191](../adr/offline/0191-offline-one-c-kernel-source-for-every-device.md): tracking and
  grid lookups are kernel code, so a grid's layout is one every device can read.
* [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md):
  the media a path is inside are fields of the path state, declared once.
* [ADR-0222](../adr/offline/0222-offline-a-bsdf-is-a-list-of-weighted-closures.md): the hair model
  is a closure of the BSDF library, beside OpenPBR's.

The plan keeps the roadmap's order, with five changes:

* **Seven decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. Four are about volumes: how the scene names a medium, which medium a path is
  in, how a medium is tracked, and how a grid is held on a device. The other three are how
  subsurface scatters, the model a hair takes, and how a volume's light is sampled.
  [Decisions](#decisions).
* **Homogeneous media come before grids**, as the roadmap orders them, and they include
  OpenPBR's coloured transmission. A glass whose `transmission_depth` is set is a homogeneous
  medium inside a dielectric, and the roadmap does not name it.
  [Step 11](#step-11--homogeneous-media).
* **A curve's hit gains a tangent along the curve.** A hair is shaded in a frame along its
  fibre, and a curve's hit gives no tangent today. [Step 16](#step-16--a-curves-frame).
* **Volume events reach light path expressions.** OSL's grammar names `V`, and moya's paths have
  no volume event for it to match. [Step 12](#step-12--volume-events).
* **The compositing outputs see volumes.** Alpha, the values at the first hit, deep samples and
  Cryptomatte each need a rule for a ray that crosses a medium. The roadmap does not name them.
  [Step 18](#step-18--volumes-in-the-outputs).

## What the survey found

The tree was read at `da1fdf28`.

**Media:**

* **moya has no medium.** Nothing in `moya/libmoya` or `api/rib` tracks, absorbs or scatters
  between surfaces. A path carries light unchanged from one surface to the next.
* **A dielectric's closure holds one index**, the one inside, with the outside's index being one
  ([Materials.md](../offline/Materials.md#the-bsdf)). Nothing records what a path has entered,
  so a glass in water bends light as a glass in air does.
* **The scene has no volume.** A `material` holds `surface` and `displacement` terminals alone,
  and no geometry kind is a volume ([Scene.md](../offline/Scene.md#moyas-kinds)).
* **The options have no volume depth.** The depth limits are `maxDepth`, `diffuseDepth`,
  `glossyDepth`, `specularDepth`, `transmissionDepth` and `rouletteDepth`.
* **The path state has no medium field** ([Integrator.md](../offline/Integrator.md#the-path-state)).
* **A shadow ray is stopped or passed by surfaces alone.** `Passing` multiplies the opacity of
  the cut-out surfaces it meets.

**Materials:**

* **The BSDF library's models** are `Diffuse`, `Mirror`, `Conductor`, `Dielectric`, `OrenNayar`,
  `Specular` and `Fuzz`, in `kernel/bsdf/Model.h`. A sample's `Lobe` is diffuse, glossy, specular
  or transmission.
* **Subsurface scatters as the diffuse base.** OpenPBR's `subsurface_weight` and
  `subsurface_color` are read, and `bsdf::Layering` mixes the colour into the Oren-Nayar base
  by the weight. `subsurface_radius`, `subsurface_radius_scale` and
  `subsurface_scatter_anisotropy` are not read.
* **Transmission is a tint at the surface.** `transmission_weight` and `transmission_color` make
  a `Dielectric` closure's transmitted colour. `transmission_depth`, `transmission_scatter` and
  `transmission_scatter_anisotropy` are not read, and an input outside `kOpenPbrInputs` is left
  out with no report.
* **Only `open_pbr_surface` reaches the BSDF library.** Standard Surface, UsdPreviewSurface, glTF
  PBR and RI's surfaces are translated onto it. A surface of any other definition takes
  OpenPBR's defaults and is reported.
* **MaterialX 1.39.5 ships what this plan needs.** Its `pbrlib` defines `ND_subsurface_bsdf`,
  `ND_chiang_hair_bsdf`, `ND_chiang_hair_roughness`, `ND_chiang_hair_absorption_from_color` and
  `ND_deon_hair_absorption_from_melanin`. It defines `ND_absorption_vdf`, `ND_anisotropic_vdf`,
  `ND_add_vdf`, `ND_mix_vdf`, `ND_layer_vdf` and `ND_volume`, and its `stdlib` defines
  `ND_volumematerial`. moya copies the libraries beside each executable, and compiles none of
  them.

**Curves:**

* **A curve's hit gives `uv` across the width and along the segment**, u from 0 to 1 with a half
  on the centre line ([Traversal.md](../offline/Traversal.md#curves)). That is the offset across
  the fibre a hair model reads.
* **A curve's shading frame is built around its normal.** A round curve's normal points away from
  the centre line, and a flat one's back along the ray. No tangent along the curve reaches the
  shading point, unless a `tangent` primvar gives one.
* **A curve takes its instance's material**, so it is shaded as an OpenPBR surface.
* **A set of curves is flat unless it says otherwise**, as RI's are.

**Lights:**

* **A light is a `Shape`**: rect, disk, sphere, cylinder, point, distant, mesh or dome
  ([Lights.md](../offline/Lights.md)). An emitting material makes a mesh light of its triangles.
* **The light tree holds lights with a place**, with a subtree over each emitting geometry's
  triangles. Distant and dome lights are drawn beside it.

**Light path expressions:** an event is the camera, a reflection, a transmission, a light or an
emitting object, and its scattering is none, diffuse, glossy or singular. The parser reads OSL's
`V`, and `lpe/Expression.h` says it matches no event.

**RIB:** `Volume`, `Interior`, `Exterior` and `Atmosphere` are reported as requests the reader does
not recognise, and skipped. A `Bxdf` names a MaterialX node definition, so `Bxdf
"ND_chiang_hair_bsdf"` makes a node today. It reaches no BSDF, since the material's surface is not
OpenPBR's. RenderMan's own `PxrVolume`, `PxrSurface` and `PxrMarschnerHair` are not read.

**Dependencies:** OpenVDB is not a dependency. vcpkg's baseline holds OpenVDB 12.0.1, whose
`nanovdb` feature depends on CUDA. The Flatpak builds neither OpenVDB nor Blosc, and the USD port
is built with OpenVDB off.

**The tests:** moya's suite holds 25 reference renders of the path tracer and 5 of compositing
outputs, each a RIB file in `moya/tests/data/` with its committed EXR
([Testing.md](../contributing/Testing.md#moya-references)). `moya_bench` times a scene of 32768
round curves for the hierarchy, and the integrator over a lit room, a textured room and a room
with outputs.

**The records this plan meets:** ADR-0233 already says a volume adds a deep sample for each
stretch of a ray inside it. ADR-0222 says the library will hold hair and volume models. ADR-0212
says volumes and subsurface grow the path state.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0242 | How the scene names a medium: a volume's geometry, its fields and its material | [1](#step-1--the-record-media-in-the-scene) |
| 0243 | Which medium a path is in where media and dielectrics nest | [2](#step-2--the-record-nested-media) |
| 0244 | How a path is tracked through a medium, at four wavelengths, and its majorants | [3](#step-3--the-record-tracking) |
| 0245 | How a grid is held where kernel code reads it | [4](#step-4--the-record-a-grid-on-the-device) |
| 0246 | How subsurface scatters, and how OpenPBR's parameters become a medium | [5](#step-5--the-record-subsurface) |
| 0247 | The model a hair takes, and how a material names it | [6](#step-6--the-record-a-hairs-model) |
| 0248 | How a volume's light is sampled | [7](#step-7--the-record-a-volumes-light) |
| 0249 | Whether a collision in a medium is a camera sample's first hit, amending ADR-0230 | [18](#step-18--volumes-in-the-outputs) |
| 0250 | How a volume's deep samples are cut, amending ADR-0233 | [18](#step-18--volumes-in-the-outputs) |
| 0251 | How alpha through a medium is made grey | [18](#step-18--volumes-in-the-outputs) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted. A record that a later step shows is not needed is
listed as a number with no record, as the ADR index requires.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-media-in-the-scene) | The record: media in the scene | `docs/adr` | **0242** | accepted |
| [2](#step-2--the-record-nested-media) | The record: nested media | `docs/adr` | **0243** | accepted |
| [3](#step-3--the-record-tracking) | The record: tracking | `docs/adr` | **0244** | accepted |
| [4](#step-4--the-record-a-grid-on-the-device) | The record: a grid on the device | `docs/adr` | **0245** | accepted |
| [5](#step-5--the-record-subsurface) | The record: subsurface | `docs/adr` | **0246** | accepted |
| [6](#step-6--the-record-a-hairs-model) | The record: a hair's model | `docs/adr` | **0247** | accepted |
| [7](#step-7--the-record-a-volumes-light) | The record: a volume's light | `docs/adr` | **0248** | accepted |
| [8](#step-8--openvdb) | OpenVDB as a dependency | `vcpkg.json`, `ports`, `flatpak` | 0245 | done |
| [9](#step-9--media-in-the-scene) | Media in the scene, and volume shaders compiled | `moya/libmoya/scene`, `material` | 0242 | done |
| [10](#step-10--the-medium-a-path-is-in) | The medium a path is in | `moya/libmoya/kernel/integrate` | 0243 | done |
| [11](#step-11--homogeneous-media) | Homogeneous media | `moya/libmoya/kernel/volume`, `integrate` | 0244 | done |
| [12](#step-12--volume-events) | Volume events | `moya/libmoya/kernel/integrate`, `lpe` | 0231 | done |
| [13](#step-13--grids) | Grids | `moya/libmoya/volume`, `kernel/volume` | 0244, 0245 | done |
| [14](#step-14--volumetric-lights) | Volumetric lights | `moya/libmoya/light`, `kernel/light` | 0248 | done |
| [15](#step-15--random-walk-subsurface) | Random-walk subsurface | `moya/libmoya/kernel/bsdf`, `integrate` | 0246 | done |
| [16](#step-16--a-curves-frame) | A curve's frame | `moya/libmoya/kernel/surface`, `geometry` | — | done |
| [17](#step-17--the-hair-bsdf) | The hair BSDF | `moya/libmoya/kernel/bsdf`, `material` | 0247 | done |
| [18](#step-18--volumes-in-the-outputs) | Volumes in the outputs | `moya/libmoya/kernel/integrate`, `integrate` | 0233, **0249**, **0250**, **0251** | done |
| [19](#step-19--media-and-hair-from-rib) | Media and hair from RIB | `api/rib`, `moya/libmoya/ri` | 0242, 0243 | open |
| [20](#step-20--the-reference-renders) | The reference renders | `moya/tests` | — | open |
| [21](#step-21--speed) | Speed: volumes, subsurface and hair against the baseline | `moya/bench` | — | open |
| [22](#step-22--the-documents) | The reference pages for volumes, subsurface and hair | `docs` | — | open |

## What blocks what

```
1 ADR ──> 9 Scene ──┐
2 ADR ──────────────┴──> 10 Medium ──> 11 Homogeneous (after 3) ──┬──> 12 Events
                                                                  ├──> 13 Grids (after 4, 8) ──> 14 Lights (after 7)
                                                                  ├──> 15 Subsurface (after 5)
                                                                  └──> 18 Outputs
4 ADR ──> 8 OpenVDB
16 Frame ──> 17 Hair (after 6)

(9 to 18) ──> 19 RIB ──> 20 References, 21 Speed ──> 22 Documents
```

Steps 1 to 7 depend on nothing but each other. Step 8 needs the record of step 4, which says
whether NanoVDB is part of the dependency. Step 9 needs the record of step 1. Step 10 needs the
scene's media and the record of step 2. Step 11 needs the medium a path is in and the record of
step 3. Steps 12, 15 and 18 need a medium a path scatters in, which step 11 gives. Step 13 needs
homogeneous tracking, OpenVDB and the record of step 4, and step 14 needs grids and the record
of step 7. Step 16 depends on nothing, and step 17 needs it and the record of step 6. Step 19
names in RIB what the steps before it built. Each step updates its own section of the reference
documents, and step 22 checks that they read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suite, the RIB library's suite and the editor's suite
  pass. Each step with code adds cases, and `failsfirst.ts` is run on the change to show that each
  new case fails without it.
* **Closed forms.** A homogeneous medium's transmittance is Beer and Lambert's. A slab's single
  scattering under a distant light has a closed form. Tracking, a shadow ray's transmittance and a
  grid's lookups are held to them, through the path tracer's statistical comparison.
* **Furnaces.** A medium that scatters and does not absorb, inside a white furnace, returns the
  furnace's light at every pixel, within errors. A white subsurface material and a white hair of
  no absorption return it too.
* **Models are checked as the BSDF library's are.** A phase function and the hair model are held
  to a chi-squared test of their samples against their densities, and to their albedo. A
  reciprocal model is checked for reciprocity.
* **One picture on every run.** A render on one thread and on every thread gives the same bytes in
  every channel, as ADR-0194 requires.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build. OpenVDB builds there through vcpkg.
* **No picture changes without a reason.** A scene with no medium, no subsurface and no hair
  renders the same bytes as before, until step 15. Step 15 changes every reference render whose
  material sets `subsurface_weight`, once, and each is committed again with the reason in its
  commit.

---

### Step 1 — The record: media in the scene

**The question.** How the scene interface names a medium: what geometry bounds a volume, where
its grids come from, and what material says how it absorbs, scatters and emits. The choice
constrains the RIB client, the editor's client and the Hydra delegate of section 12.

**The options:**

* **UsdVol's shape**: a `volume` geometry kind whose fields each name a file and a grid, as
  UsdVol's `Volume` prim names `OpenVDBAsset` fields. The material gains a `volume` terminal, as
  MaterialX's `volumematerial` and USD's `outputs:volume` give one. A Hydra delegate hands its
  volumes through unchanged. A homogeneous medium is a volume terminal on a closed surface's
  material.
* **RenderMan's shape**: a volume is a closed geometry, or RenderMan's `Volume` primitive, whose
  `Bxdf` is a volume shader such as `PxrVolume`. Grids are read by the primitive's plugin. RIB
  reads as written, and a USD volume is translated.
* **pbrt-v4's shape**: a medium is an object of its own, and every surface names the medium inside
  and outside it, as `MediumInterface` does. Nothing is inferred, and every client has to say what
  each surface separates.

The record also settles how a volume shader reads a field by name, and what a volume's fields
are called: density, temperature, emission and velocity, as Houdini and Blender name them. It
settles whether a volume's bounds come from its grids or from the geometry, and how OpenPBR's
`transmission_depth` and `subsurface_*` inputs become media under the chosen shape.

**Drafted** as
[ADR-0242](../adr/offline/0242-offline-media-follow-usdvols-volumes-and-fields.md): a volume is a
geometry that names its fields by the names its shader reads them by, as UsdVol's `field:`
relationships do. Each field is an object of a file and a grid. A material gains a volume
terminal, a MaterialX volume shader that reads a field through `geompropvalue`. It fills the
volume, or the inside of a closed surface the material is given to. A closed surface whose
material has no volume terminal takes the medium its OpenPBR transmission depth and subsurface
inputs give. A volume with no material reads its `density` field as its density. A volume's
bounds are its fields' bounds, and the usual fields are `density`, `color`, `temperature`,
`emission` and `velocity`, as Houdini and Blender name them.

### Step 2 — The record: nested media

**The question.** Which medium a path is in when surfaces overlap: a liquid in a glass, ice in
water, a cloud through a mountain's fog. The answer decides which index a dielectric bends by and
which medium a path tracks through.

**The options:**

* **A stack with priorities**, after Schmidt and Budge: each closed surface has a priority, and
  where two overlap the higher one's medium fills the overlap. A surface of lower priority inside
  a higher one's medium is skipped, and the index of a dielectric is the ratio of the two media on
  either side. Arnold, RenderMan and Octane work this way, and Cycles has a patch for it. The path
  state carries a stack of bounded depth.
* **A stack without priorities**: a path pushes a medium on entering a surface and pops it on
  leaving, and the top of the stack is the medium. Overlaps are the user's to avoid.
* **Media named on each surface**, as pbrt-v4's `MediumInterface` names them. The path state
  carries one medium, and nothing nests unless the scene says so surface by surface.

The record also settles how deep the stack is, what a camera inside a medium starts in, and how a
shadow ray finds the media it crosses. It settles where a priority is set: on the instance, as
RenderMan's intersection priority is an attribute, or on the material, as Arnold's is.

**Drafted** as
[ADR-0243](../adr/offline/0243-offline-nested-media-are-resolved-by-instance-priority.md): a path
carries a stack of the closed surfaces it is inside, eight deep, and each instance has a
`priority`, 0 by default. A hit on a surface of lower priority than the path's medium is passed
as no bounce. A dielectric bends light by the indices of the winning media on its two sides.
Equal priorities let the medium entered last win, so a scene of no priorities is a plain stack.
A ninth medium drops the one of lowest priority. A volume not bounded by a surface stays out of
the stack and adds its density to the path's medium. A camera finds the media it is inside by
tracing out at the start of each path, as Cycles does. A shadow ray carries a copy of the stack.

### Step 3 — The record: tracking

**The question.** How a path finds where it scatters or is absorbed in a medium, and how a shadow
ray finds what passes, at the four wavelengths a path carries.

**The options:**

* **Null-scattering tracking with spectral MIS**, as Miller, Georgiev and Jarosz formulate it and
  pbrt-v4's `VolPathIntegrator` runs it. A path tracks against a majorant at the hero wavelength,
  and the other wavelengths are weighed by the chance each would have sampled the same path. A
  medium whose density differs by wavelength stays unbiased, and light samples are weighed
  against tracked ones by MIS.
* **Delta and ratio tracking at the hero wavelength alone**: the other wavelengths are dropped once
  a path enters a chromatic medium, as pbrt-v4's `SimpleVolPathIntegrator` does. It is simpler, and
  noisier in colour.
* **Ray marching at a fixed step**: a medium is sampled at intervals. It is biased by the step,
  and its cost follows the step rather than the medium's density.

The record also settles the majorant: one per medium, or a coarse grid of majorants traversed by a
DDA, as Kettunen and others give it and pbrt-v4's `DDAMajorantIterator` runs it. It settles the
phase functions, Henyey and Greenstein's at least, and how a volume event's draws take their place
among a bounce's dimensions.

**Drafted** as
[ADR-0244](../adr/offline/0244-offline-media-are-tracked-by-null-scattering-with-spectral-mis.md):
a path is tracked by null scattering at its hero wavelength, and the other wavelengths are weighed
by spectral MIS, as pbrt-v4's `VolPathIntegrator` runs it. A shadow ray takes the transmittance by
ratio tracking inside the same MIS. A homogeneous medium is its own majorant. A grid's majorants
are each cell's most in a coarse grid walked by DDA, and step 4 settles where the coarse grid
comes from. The phase function is Henyey and Greenstein's, as MaterialX's `anisotropic_vdf`
defines it. A scattering event's direction, light sample and roulette draw from a bounce's
dimensions. The distances between collisions come from a hash of the pixel, the sample, the
bounce and the collision. A scattering event counts against `volumeDepth` and `maxDepth`, and a
null collision counts for nothing.

### Step 4 — The record: a grid on the device

**The question.** How a grid read from OpenVDB is held where kernel code reads it, within kernel
code's rules: only kernel headers and glm included, no allocation and no virtual call.

**The options:**

* **NanoVDB**, the linearised read-only VDB that Cycles converts OpenVDB grids into for every
  device. Its layout is NVIDIA's and its access is a header of its own, so the kernel's include
  rule would gain it. vcpkg's port builds it behind a feature that pulls in CUDA.
* **moya's own sparse layout**, converted from OpenVDB on the host: a two-level grid of bricks, or
  a tree whose depth moya fixes, in device buffers that `kernel::Array` views. moya owns its
  lookup, as ADR-0187 says of its algorithms.
* **Dense bricks through the texture cache**: a grid becomes tiles of a 3D texture, loaded on
  demand within the cache's memory limit, as the cache loads image tiles today. Grids larger than
  memory stream, and a sparse grid's empty space costs tiles.

The record also settles how a lookup is filtered, trilinear or by a stochastic choice of voxel,
and whether the majorant grid of step 3 is built from the same structure.

**Drafted** as
[ADR-0245](../adr/offline/0245-offline-grids-are-held-as-nanovdb-on-every-device.md), which
amends ADR-0187: each grid is converted on the host from OpenVDB to NanoVDB by NanoVDB's own
tools, and kernel code reads its buffer through NanoVDB's headers. Those headers join what kernel
code may include, beside glm, with NanoVDB's own host and device markers mapped onto
`MOYA_KERNEL`. A lookup is trilinear. A grid's majorants are the most its nodes store, walked by
NanoVDB's hierarchical DDA. A volume shader whose density is not a scale of one field is bounded
by its program's value at each node's most. One that cannot be bounded is reported and takes one
majorant over the volume. An overlay port builds NanoVDB's headers without CUDA.

### Step 5 — The record: subsurface

**The question.** How light scatters below a surface, and how OpenPBR's `subsurface_color`,
`subsurface_radius`, `subsurface_radius_scale` and `subsurface_scatter_anisotropy` become what the
path tracer runs. The roadmap names a random walk, and the choice is the record's.

**The options:**

* **A random walk in the medium inside the surface**, as Chiang and Burley give it and Cycles,
  Arnold's `randomwalk` and RenderMan's path-traced subsurface run it. A path enters through the
  dielectric, walks the medium the parameters give, and leaves where it meets the surface again.
  It needs a closed surface, and it shares step 11's tracking.
* **A diffusion profile**, Christensen and Burley's, sampled by probe rays to find an exit point
  near the entry. It works on any surface and blurs thin features less correctly.
* **Both**, chosen per material, as Cycles and Arnold offer.

The record also settles how the multiple-scattering colour becomes a single-scattering albedo,
by van de Hulst's inversion or Chiang and Burley's fit, and how the radius becomes the medium's
mean free path. It settles what a thin-walled or open surface does, and whether a walk's medium
nests with step 2's media.

**Drafted** as
[ADR-0246](../adr/offline/0246-offline-subsurface-is-a-random-walk-inside-the-surface.md): a surface
whose subsurface weight is above zero holds a homogeneous medium inside it, which a path walks by
ADR-0244's tracking. The medium enters ADR-0243's stack as any interior medium does. The colour
becomes a single-scattering albedo by the mapping OpenPBR's companion paper gives. The radius
times its scale is the mean free path in each ACEScg channel, made a spectrum through the
unbounded table. A walk counts its events against `subsurfaceDepth`, 256 by default, apart from
`volumeDepth`. It draws no light sample and is absorbed at its limit. A thin-walled surface takes
OpenPBR's thin-walled rule with no walk, which step 15 checks against the specification's text.

### Step 6 — The record: a hair's model

**The question.** The model a hair fibre scatters by, and how a material names it.

**The options:**

* **Chiang and others' far-field model**, as MaterialX's `chiang_hair_bsdf`, pbrt-v4 and
  Cycles' principled hair give it. Its lobes are R, TT, TRT and a residual. It takes a
  longitudinal and an azimuthal roughness, a cuticle tilt, and an absorption from colour or
  melanin. MaterialX defines its node and its helpers, so a material names it directly.
* **Huang and others' near-field microfacet model**, which Cycles 4.0 added beside Chiang's. It
  handles elliptical fibres and close views, and has no MaterialX node.
* **d'Eon and others' energy-conserving model**, of which Chiang's is a fit. It is more exact and
  costlier to sample.

The record also settles how a curve's frame gives the model its tangent and its offset across the
fibre, and what a flat curve does. It settles how a hair's material is told apart from a
surface's when it is compiled.

**Drafted** as [ADR-0247](../adr/offline/0247-offline-hair-scatters-by-chiangs-model.md): a hair
scatters by Chiang and others' far-field model, a closure of the BSDF library. A material names it
by MaterialX's `surface` node whose `bsdf` is `chiang_hair_bsdf`, and its inputs and its three
helpers compile through the node program. `curve_direction` is the curve's tangent, which step 16
adds. The offset across the fibre, from -1 to 1, is the hit's place across the width, on round and
flat curves alike, as pbrt-v4 takes it on flat ones. On a surface that is not a curve, the model is
reported once and shades with the surface's tangent and an offset of zero. R and TRT are glossy
bounces and TT a transmission.

### Step 7 — The record: a volume's light

**The question.** How light a volume emits reaches the points it lights: by light samples, by
paths that track into it, or both.

**The options:**

* **Emitting volumes in the light tree**: each emitting volume, or each brick of its emission, is
  a leaf bounded by its box and its power, as Conty Estevez and Kulla's tree takes volumes. A
  light sample draws a point inside by its emission.
* **Emission gathered by tracking alone**, as pbrt-v4 gathers it: a path collects what a medium
  emits along the stretch it tracks through. Nothing is built, and a small hot volume is noisy.
* **A distribution over each volume's voxels**, with MIS against tracking, as Villemin and Hery
  give it. It samples a volume's emission well, with no tree.

The record also settles how a temperature field becomes emission, by Planck's law as
`blackbody` gives it, and how a light sample into a volume carries the transmittance on its way
out.

**Drafted** as
[ADR-0248](../adr/offline/0248-offline-an-emitting-volume-is-a-subtree-of-the-light-tree.md): each
emitting volume has a subtree of the light tree over its NanoVDB leaf nodes, as a mesh light has
one over its triangles. Each brick's box and summed emission come from running the shader on the
host. A light sample draws a brick by the tree, a voxel by its emission and a point in the voxel.
It carries the transmittance on its way out by ratio tracking, and is weighed against tracked
emission by MIS. An emitting medium inside a closed surface is one leaf, drawn uniformly in the
surface's box, and a point outside the surface emits nothing. Emission is the shader's ACEScg
colour made a spectrum, and a temperature reaches it through MaterialX's `blackbody` node. A
volume's light is an emitting object in light path expressions, labelled as an emitting
material's light is.

### Step 8 — OpenVDB

* **OpenVDB joins the `moya` feature** in `vcpkg.json`, with Blosc, and with NanoVDB only if the
  record of step 4 chose it.
* **An overlay port in `ports/` builds it**, if vcpkg's port cannot be had without a feature the
  record does not need, such as CUDA.
* **The Flatpak builds it** as a module, with Blosc.
* **[Dependencies.md](../contributing/Dependencies.md)** names it and why.

**Tests:** a grid written through OpenVDB and read back holds the same values.

**Done.** `ports/openvdb` is vcpkg's port of OpenVDB 12.0.1 with its features removed. It always
builds NanoVDB 32.7, with CUDA off and its conversion from OpenVDB on, and none of the two
libraries' tools. OpenVDB joins the `moya` feature and brings Blosc and boost's iostreams, which
the Flatpak builds as modules beside OpenVDB's. libmoya does not link OpenVDB until step 13 reads
a grid, so the moya suite links `OpenVDB::openvdb` and `OpenVDB::nanovdb` for `GridTest.cxx`. A
grid written and read back holds every value, and its NanoVDB conversion holds them too, with its
root's most the grid's most. NanoVDB chooses between the standard's old and new type traits by
`__cplusplus`, which MSVC reports as C++98 unless it is given `/Zc:__cplusplus`, so the suite is
compiled with it. MSVC also reports code it cannot reach in NanoVDB's headers once it inlines
them, which its external-header level does not silence, so C4702 is switched off around them.

### Step 9 — Media in the scene

* **The scene holds what the record of step 1 decides**: a volume's geometry and its fields, and
  the material's volume terminal.
* **`material::compile()` compiles a volume shader**: MaterialX's `volume`, `absorption_vdf`,
  `anisotropic_vdf`, `add_vdf`, `mix_vdf` and `layer_vdf`, into a medium's absorption,
  scattering, anisotropy and emission. A pattern feeding them runs in the node program, as a
  surface's does, at the point being tracked.
* **OpenPBR's `transmission_depth`, `transmission_scatter`, `transmission_scatter_anisotropy` and
  `subsurface_*` inputs are read** into `kOpenPbrInputs`, so a program fills them, and the medium
  they give is made as the record of step 1 says.
* **An input left out of `kOpenPbrInputs` is reported**, as any other input moya does not read is.
* **`moya --scene` prints a volume and its fields**, and the committed descriptions gain them.

**Tests:** a volume's description names its fields. A volume shader of constant inputs compiles
to the medium they give. An OpenPBR glass with a depth gives a medium of its colour at that depth.

**Done.** The scene holds UsdVol's `openVDBAsset`, a `filePath` and a `fieldName`, and `volume`,
whose `fields` and `fieldNames` pair each field with the name its shader reads it by. An instance
places a volume, which the gather reports and leaves out until a step traces one, and a mesh light
takes none. A material gains `volume` and `volumeOutput`, so every committed description of a
scene with a material gains the two lines. `material::compileVolume()` compiles an `ND_volume`.
`internal::rewrittenMedium()` turns its VDF and EDF trees into `add` and `mix` nodes of
`vector3` and `color3`, which feed a node of a medium's values. A pattern feeding a closure then
runs in the program. A `kernel::volume::Medium` holds an absorption, two lobes of Henyey and
Greenstein's phase function, from the first two `anisotropic_vdf` leaves, and an emission. A third
leaf joins the second lobe and is reported. Every input of `open_pbr_surface` but its thin wall now
reaches the program, and a dispersion that may be set is reported. `kernel::volume::interior()`
gives the medium inside a glass, as OpenPBR's text gives it. The medium below a subsurface
surface is step 15's, since ADR-0246 settles its mapping.

### Step 10 — The medium a path is in

* **The path state carries the media a path is in**, as the record of step 2 decides, declared
  in `MOYA_PATH_FIELDS`.
* **A path enters and leaves media at their surfaces.** A dielectric bends by the indices on its
  two sides, which the media give, in place of an outside of one.
* **A surface skipped by the record's rule** is passed through, as a cut-out surface is, and
  counts as no bounce.
* **A camera inside a medium** starts in it, as the record settles.
* **A volume's boundary scatters nothing.** It is a surface of no BSDF, which a path passes.

**Tests:** a glass of index 1.5 in water of index 1.33 bends by their ratio. Two overlapping
spheres give the overlap the medium the record says. A scene with no medium gives the same bytes
as before.

**Done.** `kernel/integrate/media/Media.h` holds the closed surfaces a path is inside, eight deep,
each by a key of its chain of instances, its priority and its medium's index. The path state carries
them as `media`. An instance gains `priority`, and a hit takes its innermost instance's that is not
zero, so every committed scene description gains a line for each instance. A material encloses a
medium when it is layered, not thin-walled and its transmission's weight may be more than zero, or
names a dielectric closure that is not thin. A material of a volume terminal and no surface terminal
is a boundary: it encloses, has no BSDF and is clear to shadow rays. A path passes by a boundary,
and a surface of lower priority than its medium, as no bounce, and enters or leaves it as it does. A
path that crosses a surface as it scatters enters or leaves its medium. A dielectric's index is
taken over the index beyond it, and a ratio below one turns its frame over, so the albedo tables and
the closure keep a ratio of at least one. A camera ray finds the surfaces it starts inside by
tracing on through every hit, when any material encloses a medium. A shadow ray's copy of the media
is step 11's, since nothing it carries depends on them until transmittance does.

### Step 11 — Homogeneous media

* **A path is tracked through a homogeneous medium** as the record of step 3 decides, at its four
  wavelengths, and scatters by its phase function or is absorbed.
* **A phase function is a model of its own** in `kernel/volume/`: Henyey and Greenstein's, and any
  other the record names, each evaluated, sampled and given a density.
* **A point in a medium draws a light sample**, weighed against the phase function's sample by
  MIS, as a shading point's is against its BSDF.
* **A shadow ray carries a copy of the path's media and the transmittance of those it crosses**,
  beside the opacity of the
  cut-out surfaces it passes.
* **The options gain `volumeDepth`**, the most scattering events in media a path makes, and a
  volume bounce is counted against `maxDepth` too. Russian roulette treats it as any bounce.
* **Coloured glass**: an OpenPBR dielectric with a `transmission_depth` absorbs over the distance a
  path travels inside it, and scatters where `transmission_scatter` is set.

**Tests:** a slab's transmittance matches Beer and Lambert's at each wavelength. A slab's single
scattering under a distant light matches its closed form. A non-absorbing medium in a white
furnace returns the furnace's light. Henyey and Greenstein's samples pass chi-squared at several
anisotropies. A chromatic medium renders its expected colour.

**Done.** Each material holds the medium it encloses: its volume's, compiled from its volume
terminal, or the one `volume::interior()` gives a layered material, and an enclosure names its
material. `kernel/volume/` gains `Phase.h`, `PhaseSample.h`, `Coefficients.h` and `Homogeneous.h`.
`kernel/integrate/media/` holds `Media.h` and `Enclosure.h`, and `Tracking.h` with the collision
draws, `rescale()` and `track()`. A path is tracked by the
extinction at its first wavelength, or at a hashed channel in the RGB build, and weighed at every
wavelength by spectral MIS. The ADR's two rescaled spectra are one here, the path's `chances`:
the light strategy's side of the MIS is carried by the scalar `density`, which grows along each
ray, and the power heuristic is kept. A path that crosses no medium keeps chances of one, so its
bytes are unchanged. A phase function's direction is drawn by each lobe's share of the
scattering over all four wavelengths, so its density does not depend on the tracked wavelength. A
shadow ray carries a copy of the media and passes by the surfaces a path passes by. A glass that
is not thin-walled and has a `transmission_depth` is not tinted by its transmission colour at its
surface. Each committed scene description gains `volumeDepth`, which defaults to four. Three
things are left to the steps that own them. A scattering event in a medium records no event for
light path expressions until step 12. A medium's emission is not gathered until step 14. A camera
ray that scatters before any surface gives the outputs nothing until step 18. The cases are
in `moya/tests/MediaTest.cxx`, where the slab of the closed forms is a sphere seen along its
diameter by an orthographic camera.

### Step 12 — Volume events

* **An event in a medium is a volume event**, `V` in OSL's grammar, with its scattering as the
  record of step 3 names it.
* **`kernel::integrate::symbolOf()` gives it a symbol**, and an expression of `V` matches it.
  `lpe/Expression.h` no longer says `V` matches nothing.
* **The trail records it** in order with the surface events.

**Tests:** `C<V.>L` in a fog lit by one light matches a render of single scattering. A set of
expressions split by whether the first event is a volume's sums to the beauty at each pixel.

**Done.** ADR-0244 names no scattering for an event in a medium, so it has none, as the camera, a
light and an object have none. Karma also reads `V` apart from `D`, as its `C[DV]A` shows.
`EventType` gains `Volume`, and `symbolOf()` gives it `kVolumeSymbol`, one symbol after the
transmissions. The object's, the light's and the labels' symbols each move up by one. `V`,
`<V.>` and `.` match it, and `D`, `<.D>` and `<VD>` do not. `track()` advances the path by the
event once the phase function draws a direction. `directInMedium()` gathers its light after the
event, as `direct()` gathers a closure's after its bounce. A trail then reads `C V* L* V` for a
fog's single scattering. The cases are in `LpeTest.cxx`, `EventsTest.cxx` and
`LightPathsTest.cxx`. They use `Staged::fog()`, a sphere of one anisotropic VDF. `C<V.>L`
matches a render whose `volumeDepth` is one at each pixel, within rounding. The room's splits by
the first bounce's scattering, with `C<V.>.*` added, sum to the beauty with a fog in front of it.

### Step 13 — Grids

* **A volume's fields are read through OpenVDB on the host**, from the files the scene names, and
  held on the device as the record of step 4 decides.
* **A medium's density comes from its field**, looked up at a tracked point, and a volume shader
  reads any field by name, as the record of step 1 settles.
* **Majorants come from the grid**, as the record of step 3 decides.
* **A grid moves with its instance** over the shutter, as a geometry's hierarchy does.
* **A grid that cannot be read is reported**, and its volume holds nothing.
* **A change to a grid's file or field** rebuilds that grid and its majorants alone, through the
  scene's versions.

**Tests:** a grid of a constant density renders as the homogeneous medium of that density, within
errors. A sphere of fog made through OpenVDB's own tools at run time matches its closed-form
transmittance along rays through its centre. One grid exported by Blender 5.2 renders, to check a
file another tool wrote.

**Done.** `volume::read()` reads a field's grid through OpenVDB and converts it with NanoVDB's own
tools, and `volume::Volumes` holds every grid's buffer on the device as blocks of NanoVDB's
alignment. The kernel reads them through `kernel/volume/Nano.h`, and `kernel.ts` lets kernel code
include `nanovdb/NanoVDB.h`. The kernel target links NanoVDB's headers and compiles with
`/Zc:__cplusplus` publicly, so the suite no longer names either. An instance of a volume is
gathered as `Prototype::Volume`, which no hierarchy is built over. Each chain of instances that
reaches it becomes a `kernel::volume::Volume`, which holds the chain, so a volume moves with its
instances over the shutter. A volume whose material has a volume program runs it with its fields,
read as the primvars their names give. A material's volume program is appended after its
surface's, and a closed surface's medium is folded from its converted values. A volume of no
program scatters its density. A grid that cannot be read is reported and its volume holds
nothing, and `geometry::Cache` keeps each grid by its field, so a changed field reads its grid
again alone.

The record leaves one thing open, which this step settles. A trilinear lookup reads one voxel past
a node, so a cell's majorant is taken over the cell and the seven cells of its side beyond it,
found by descending NanoVDB's tree. The walk steps from cell to cell by moya's own code rather
than NanoVDB's HDDA class, since it needs each cell's bound as it enters it. A program that reads
anything but its fields and constants takes one majorant over its volume, as the record says.
Tracking against a majorant that sums the homogeneous medium's and every crossed volume's is
`trackCrossed()`, and a ray that crosses no volume takes the homogeneous tracking of step 11, to
the byte. A shadow ray takes a volume's transmittance by ratio tracking, `passVolumes()`. Null
collisions are part of a path for multiple importance sampling, as pbrt-v4's are. The path state
gains `lightChances`, the densities with which a shadow ray would have tracked the path. A path's
density and a light sample's chance are each divided by their growth, so both strategies weigh a
path alike. Overlapping media join their lobes by anisotropy, `volume::join()`.

The cases are in `VolumeTest.cxx`. A cell bounds every lookup in a grid of scattered leaves and
tiles. A box of constant density, read by a program that absorbs it times a colour, lets through
Beer and Lambert's share. A scattering density returns a furnace's light. OpenVDB's fog sphere
lets through e to the minus its diameter less its band. The smoke `make_blender_smoke.py` has
Blender simulate and write lets through what OpenVDB's own sampler integrates along the axis.
An unreadable grid renders the bare picture to the byte. A volume in a group placed twice is
placed through both chains, and a cache reads a forgotten field's grid alone. The furnace and
closed-form cases take 4096 samples, since a path's estimate of transmittance is one or zero.

### Step 14 — Volumetric lights

* **A volume emits** by its shader's emission, and by its temperature through Planck's law where
  the shader reads one.
* **Its light reaches the scene** as the record of step 7 decides.
* **Camera rays see it**, and light path expressions name it as an emitting object.

**Tests:** a homogeneous emitting sphere's radiance seen through its centre matches its closed
form. A scene lit by an emitting volume alone matches a render of tracking alone, within errors.

**Done.** A volume light is a `kernel::light::Light` of shape `Volume`, which emits from bricks of
its own space, `kernel::light::Brick`. A volume whose program emits makes one, and so does each
chain that places a closed surface whose medium emits. `light::internal::gridGlow()` runs the
volume's program on the host at each cell's centre, through `light::Shading`, the CPU device's
views. A grid's bricks are its leaf nodes, its lower nodes' active tiles, and its upper nodes'
active tiles cut to a leaf's size. A root's tiles make none, and their points are found by
collisions alone. A closed surface's one brick is its box, and a light sample there emits only
where its shadow ray ends in the surface's medium. `kernel/integrate/media/Glowing.h` draws a
point in a brick and gives its density by solid angle and length, so it weighs against a
collision. `glowAt()` gathers emission at every collision, as pbrt-v4 does, and weighs it
against the light sample by the power heuristic. A shadow ray gives `Passed::colliding`, the
density a tracked path would have had of colliding at its end, for the light sample's side.
MaterialX's `blackbody` is `Op::Blackbody`, `spectrum::blackbody()`. It sums Planck's law through
the observer at 5 nm, as MaterialX's OSL implementation does, since its GLSL implementation's
cubic strays far from the locus below 1667 K. `chains()` in `light/Lights.cxx` called its callback
for a volume's chain too, which then read a geometry of the volume's index. It now stops at a
geometry. The cases are in `VolumeLightsTest.cxx`. An emitting sphere and a grid that glows by
`blackbody` of a temperature field each match the closed form along their depth. A floor lit by
either alone renders the same light under `bsdf`, `light` and `mis`, drawn by the tree and by
power. `blackbody` matches a light's colour temperature from 1000 K to 12000 K.

### Step 15 — Random-walk subsurface

* **A material whose `subsurface_weight` is above zero scatters below its surface** as the record
  of step 5 decides, with the medium its colour, radius, scale and anisotropy give.
* **The diffuse base no longer stands in for it.** `bsdf::Layering` stops mixing the subsurface
  colour into the Oren-Nayar base.
* **A walk's events are volume events** below a transmission, so light path expressions and
  depth limits see them.
* **The denoiser's albedo** at such a surface is its subsurface colour, as OIDN expects.

**Tests:** a white subsurface sphere in a white furnace returns the furnace's light. A slab's
reflectance for a set radius matches the record's mapping within errors. Every reference render
whose material sets `subsurface_weight` is committed again, once.

**Done.** `kernel::volume::openpbr::walked()` gives the medium below a surface by OpenPBR's mapping,
in `kernel/volume/openpbr/Subsurface.h`, and a material holds it as `walked` beside the medium its
transmission gives. A material encloses a medium when its subsurface's weight may be more than zero,
`subsurfaces`, as when its transmission's may. `openpbr::interior()` and `walked()` now run in
`ConvertedMaterials`, once the surface's colours are in ACEScg. Before, they read a colour in its
document's space, which only the default space hid. `material::enclose()` makes both, and a
subsurface input that varies over the surface takes OpenPBR's default in the medium and is reported.
An enclosure records `walk`, whether the path walks the subsurface's medium, and `media::mediumOf()`
gives the medium a path is in. `bsdf::Layering` gives the subsurface's share of the opaque base, and
that share of its specular reflection, to a `Dielectric` that walks. A path that crosses by it
enters the walk. From inside, `Layering::walk()` makes the surface that dielectric alone. A walk's
scattering events take no light sample and no roulette, and count as specular for the light met
next. A light inside the surface is then found only by a walk's own directions, where ADR-0246's
costs say it is not found at all. A reflection off its own boundary is an event of the walk too, not
a glossy bounce. Its events count against `subsurfaceDepth`, 256 by default, and the other limits
read `depthOf()`, the bounces less `subsurfaceBounces`. The count of bounces in all still counts
them, so each event draws from a block of its own. A shadow ray that crosses the surface it leaves
enters or leaves that surface's medium first. Without that, a light sample at a walk's exit is
dimmed by the walk's medium all the way to the light. A walk's dielectric gives a denoiser the
subsurface colour, and is not counted singular when smooth. A thin wall takes OpenPBR's sheet, a
reflection and a transmission of Oren-Nayar's lobe split by the anisotropy. The transmission is a
closure that goes `through`, which `Bsdf` mirrors through the surface. No reference render sets
`subsurface_weight`, so none is committed again, and every one renders as before within its errors.
Each committed description gains `subsurfaceDepth`.

The test of a slab's reflectance compares with the closed form of a half-space rather than with
the colour. The camera sees the half-space along its normal, where it reflects one less the root
of one less the albedo times Chandrasekhar's H at one. OpenPBR fits its mapping to the albedo
under diffuse light, which is larger, so a grey of a half reads 0.445 there. A white surface of
a partial subsurface keeps a furnace's energy at the BSDF, counting the walk's light by its
energy, since the renders of a full weight cannot see a share counted twice. The cases are in
`SubsurfaceTest.cxx`. The subsurface's header took `kernel/volume` past the directory limit, so
it and `Interior.h`, the media an OpenPBR surface gives, move to `kernel/volume/openpbr`.

### Step 16 — A curve's frame

* **A curve's shading point holds a tangent along the curve**, from the segment's derivative at
  the hit's v, in the world.
* **A curve's frame is built around its tangent**, so a model that reads a fibre's direction
  gets it.
* **A hit's offset across the fibre**, from -1 to 1, is kept with the shading point, from the
  hit's u.

**Tests:** a straight curve's tangent lies along it at every hit. The offset is zero on the
centre line and runs to the edges. An OpenPBR surface on a curve gives the same picture as
before.

**Done.** `geometry::primitive::curveDirection()` gives the direction a segment's centre line runs
at the hit's v, and `surface::Surfaces::at()` holds it on the shading point as `fibre`, turned
into the world. A cubic segment whose derivative vanishes where control points meet runs along
its chord. `across` is twice the hit's u less one. A curve's frame keeps its shading normal and
takes the fibre less its part along that normal as its tangent, by `Surfaces::alongside()`. A
`tangent` primvar still overrides it where `tangentFrame()` reads one. No reference render holds
a curve, so the picture before is checked where it would change. An isotropic OpenPBR surface at
a curve's point gives the same values in the new frame as in the old, which turns about the same
normal. The new frame changes a curve's noise and, with an anisotropic specular, the direction
the highlight stretches, which now follows the fibre. The cases are in `CurvesTest.cxx`.

### Step 17 — The hair BSDF

* **The model the record of step 6 decides** is a closure of the BSDF library, in
  `kernel/bsdf/scattering/`, evaluated, sampled and given a density, with an estimate of its
  albedo.
* **A material whose surface is that model's node compiles to it**: `chiang_hair_bsdf` with
  `chiang_hair_roughness`, `chiang_hair_absorption_from_color` and
  `deon_hair_absorption_from_melanin`, if the record chose Chiang's model.
* **A hair's material on a surface that is not a curve** is reported and shades as the record
  says.
* **A hair's bounces are counted** by the kind of lobe each sample takes.

**Tests:** samples pass chi-squared against the density at several roughnesses and offsets. A white
hair of no absorption in a white furnace returns its light. The model's values match pbrt-v4's
published implementation at fixed directions.

**Done.** `bsdf::scattering::Hair` is pbrt-v4's model with MaterialX's tints and a roughness for
each lobe, of the `scattering::Fibre` the BSDF holds, and `Model::Hair` names it. A `surface` node
over `chiang_hair_bsdf` compiles to a hair's program, laid out by `kernel/program/Hair.h`, and the
material holds the folded `surface::Hair`. MaterialX implements the three helper nodes only in its
shader generators, so `material::Library` reads moya's graphs of them, from standard nodes, in
`HairGraphs.cxx`. MaterialX's cuticle angle a tilts by pi (1/2 - a) in pbrt-v4's terms, which
places R's peak where MaterialX's GLSL places it. The model's offset is minus the shading point's
`across`, as MaterialX derives it from a round curve's normal. A hair's material on what is not a
curve is reported once, by the gather. A sample's bounce is TT's transmission or a glossy
reflection, and a light sample's part takes the kind of the lobe that carries most of its light.
The cases are in `HairTest.cxx`, against a double-precision port of pbrt-v4's `HairBxDF`. No
reference render holds hair, so no picture changes.

### Step 18 — Volumes in the outputs

* **Alpha through a medium** is one less the transmittance along the camera ray, so a render of
  fog over a plate composites.
* **The values at the first hit** in a medium take a rule: whether a volume event is a first hit
  for depth, position and normal, and what the denoiser's albedo and normal are there.
* **A deep file holds a sample for each stretch of a ray inside a volume**, as ADR-0233 says,
  with its front and back depths.
* **Cryptomatte names a volume** by its instance and material, as a surface's are named.
* **A rule ADR-0233 or ADR-0230 lacks** is an amendment of that record, drafted before the code.

**Tests:** a fog's alpha matches one less its transmittance. A deep file of a fog flattens through
OpenEXR's compositing to the flat beauty and alpha. A volume's Cryptomatte coverage matches its
alpha.

**Done.** Three records settle what the step's rules left open.
[ADR-0251](../adr/offline/0251-offline-alpha-through-a-medium-is-its-mean-transmittance.md) makes
alpha one less the mean transmittance over a path's wavelengths, as Cycles does, estimated from
the throughput of a camera path that leaves the scene.
[ADR-0249](../adr/offline/0249-offline-a-collision-in-a-medium-is-a-first-hit.md), amending
ADR-0230, makes a camera ray's collision in a medium its first hit and what its sample covers, and
gives a denoiser the medium's albedo and a normal back along the ray.
[ADR-0250](../adr/offline/0250-offline-a-volumes-deep-samples-are-cut-by-the-tolerance.md),
amending ADR-0233, joins a medium's collisions by the deep tolerance into spans. Tracking records a
collision that scatters or absorbs through `media::collided()`, and in volumes picks the medium it
names by its share of the extinction, from the number that chose the collision. A closed surface's
medium names its innermost and outermost instances. Each camera sample's record now starts as one
that met nothing, so a path that ends in a medium before any record no longer leaves the slot of a
pass before. A checkpoint's deep part gains `back` and `volume`, and a deep file's `ZBack` is each
sample's back depth. The cases are in `VolumeOutputsTest.cxx`, and one in `VolumeTest.cxx` checks
the share two volumes take. No reference render holds a medium, so no picture changes.

### Step 19 — Media and hair from RIB

* **RIB's `Volume` request** is read, written and recorded by the RIB library, and a handler
  ignores it by default.
* **The scene client makes a volume** from RenderMan's `Volume` and its OpenVDB plugin's
  arguments, as RenderMan names a file and its grids.
* **A volume shader's `Bxdf`** makes the material's volume terminal, as a surface's `Bxdf` makes its
  surface.
* **A priority**, if the record of step 2 keeps one, is read from RenderMan's attribute of it.
* **`Interior`, `Exterior` and `Atmosphere`** are read as the record of step 1 maps them, or stay
  reported as requests moya does not take.
* **A hair's `Bxdf`** makes its material's surface.

**Tests:** a RIB file of each request gives the scene the objects it names. A RIB file of a fog
and a light renders.

### Step 20 — The reference renders

* **A set of scenes for volumes, subsurface and hair**, as RIB files in `moya/tests/data`. They
  are a fog under a spot light, a chromatic medium, a glass of liquid, a grid of smoke and a
  fire. A marble and a skin-toned figure scatter below their surfaces, and a lock of hair lies
  under a key light.
* **Each is rendered and committed** as the path tracer's references are, with each pixel's
  standard error, and compared within five of them. Each holds its committed description and hit
  view, as the other references do.

**Tests:** the committed renders, and a case that renders each with a light or a medium changed
and checks that the comparison fails.

### Step 21 — Speed

* **`moya_bench` gains three passes**: the lit room filled with fog, the lit room with a
  subsurface statue, and a lock of hair under a key light. Each reports samples a second, and
  the share of time in tracking, in grid lookups and in the hair model.
* **The baseline** in `moya/bench/Baseline.md` gains the figures, from the same machine.

**Tests:** the benchmark's short pass times each scene.

### Step 22 — The documents

* **A new reference page**, `docs/offline/Volumes.md`, for media, nesting, tracking, grids and
  volumetric lights. The docs site's summary lists it.
* **[Materials.md](../offline/Materials.md)** names volume shaders, subsurface and the hair model.
* **[Integrator.md](../offline/Integrator.md)** names the medium in the path state, volume
  bounces and `volumeDepth`.
* **[Lights.md](../offline/Lights.md)** names volumetric lights.
* **[Traversal.md](../offline/Traversal.md)** names a curve's tangent and offset.
* **[Scene.md](../offline/Scene.md)** names the volume kind, the material's volume terminal and
  the new options.
* **[Rib.md](../offline/Rib.md)** names `Volume` and the requests the records map.
* **[Compositing.md](../offline/Compositing.md)** names volume events and volumes' deep samples.
* **[The offline README](../offline/README.md)** no longer lists volumes as not built.
* **[Testing.md](../contributing/Testing.md)** names the volume references.
* **Each record's `Documented in` line** names the reference page that states its rule.
* **The roadmap's section 8** links here, and says so when this plan closes.

---

## Working beside the editor scripting plan

[EditorScripting](EditorScripting.md) is open. It takes ADR-0217 to ADR-0221, and this plan takes
ADR-0242 to ADR-0248. Its one change to moya is step 16's thread count in `moya/moya.cxx`, which
this plan does not touch. This plan changes nothing the editor sends moya: the editor has no
volumes or hair to send, and its materials are a plan of their own. Each commit names its paths,
with `git commit -- <paths>`, so neither plan's staged work lands in the other's commit.
