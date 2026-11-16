# Moya Retirement — One Core for Every Render

Drafted 2026-11-15 against `376b408a`. This plan takes up section 7 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md#7-the-retirement). When it closes, the path
tracer is moya's only renderer. Every RIB scene moya reads renders through the scene client and
the new core, and RI's built-in shaders become materials and lights. The editor's render view and
its batches render through the new core. The reyes hider, the ray hider, the old graphics state,
`offline::trace` and the shading language in `api/render/offline/sl` are deleted, with the tests,
fixtures and documents that describe them.

The rebuild's decisions are ADR-0186 to ADR-0194, and the decisions of its first six plans are
ADR-0195 to ADR-0216 and ADR-0222 to ADR-0235, all accepted. This plan follows three of them:

* [ADR-0186](../adr/offline/0186-offline-moya-renders-by-path-tracing-alone.md): the old hiders and
  the shading language are retired with no compatibility mode. A shader written in RSL stops
  working, and RIB stays as one way to describe a scene.
* [ADR-0188](../adr/offline/0188-offline-a-retained-scene-interface-moya-owns.md): the retained
  scene is moya's API, and RIB and the editor are its clients.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): each device
  gives one picture on every run, so the editor's render of a scene and moya.exe's render of the
  same scene can be compared byte for byte.

The plan keeps the roadmap's order, with five changes:

* **Four decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are what RI's built-in surfaces become, and what RI's ambient light and
  background imager become. The other two are whether RI's C interface stays, and what
  `api/render/offline` holds once the shading language and the ray tracer leave it.
  [Decisions](#decisions).
* **Every request the old handler reads gets a stated behaviour in the scene client**, before
  moya.exe changes its default. The roadmap names the built-in shaders. The survey found more:
  `Hider`, `ShadingRate`, `Imager`, `Opacity`, three `Attribute`s and two `Option`s.
  [Step 7](#step-7--every-request-the-old-handler-read).
* **The old test scenes are kept as path-traced references.** Their reyes and ray-traced PNGs are
  deleted. Each scene is rewritten where it names a user shader, and becomes a reference render
  held within its standard errors. [Step 8](#step-8--the-old-scenes-through-the-new-core).
* **The tests stand on the new core before anything is deleted.** Eleven new-core tests reach the
  old code through helpers, and one uses the ray hider as its reference.
  [Step 13](#step-13--tests-on-the-new-core-alone).
* **The shipped scenes and the manual's pictures are remade.** moya installs two RIB scenes that
  CI renders through the old core, and `pictures.ts` renders the manual's pictures through it,
  from two shaders of its own. [Step 12](#step-12--the-shipped-scenes-and-the-manuals-pictures).

## What the survey found

The tree was read at `376b408a`.

**The two RIB handlers:**

* **The reader recognises a fixed set of requests**, and reports any other once by name
  ([Rib.md](../offline/Rib.md)). `Atmosphere`, `Interior`, `Exterior`, the patches, `Blobby`,
  `Procedural`, `ReadArchive` and `Bound` reach neither handler.
* **`RIBHandler` drives the old graphics state** and the two hiders, and shades through the
  shading language. It draws no point, no quadric but the sphere, no subdivision mesh and no
  object instance.
* **`SceneClient` fills the retained scene.** It reads every kind of geometry the new core
  intersects, `Bxdf` and `Pattern` networks, RenderMan's `Light`s and the outputs.
* **These old requests have no behaviour in the scene client:** `Hider`, `ShadingRate` and
  `Imager` are not overridden. `Option "searchpath" "shader"` and `Option "limits"`'s bucket and
  grid sizes are passed over. Every `Attribute` but `identifier` and `displacementbound` is passed
  over in silence, among them `"light" "nsamples"` and `"trace" "bias"`.
* **A `Surface` becomes a shader node of language `ri`**, with its parameters, `Cs` and `Os`. The
  path tracer compiles only MaterialX surfaces. Any other is one diffuse closure in `Cs`. As a
  result `plastic`, `metal`, `shinymetal`, `glass`, `paintedplastic` and every user shader render
  as a diffuse surface, their parameters and `Os` are ignored, and nothing is reported.
* **A `Displacement` of language `ri` is ignored** by the path tracer, which displaces only by a
  MaterialX graph.
* **`LightSource` makes a light of `pointlight`, `distantlight` and `spotlight` alone**, as UsdLux
  sphere and distant lights. `ambientlight` is reported and makes no light. The spot light's
  default cone delta differs from the shading language's: 0.0872665 against 0.0698132.
  `AreaLightSource` makes a mesh light of any shader name.

**The built-in shaders** are one source string compiled into `api/render/offline/sl`. There are
seven surfaces (`constant`, `matte`, `metal`, `plastic`, `paintedplastic`, `shinymetal` and
`glass`), five lights (`ambientlight`, `distantlight`, `arealight`, `pointlight` and `spotlight`)
and one imager (`background`). A `.sl` file on the shader search path replaces a built-in of its
name. No mapping of a built-in surface onto OpenPBR exists.

**moya.exe:** `--scene`, `--hits`, `--paths` and `--maketexture` run on the new core. With none of
them it renders through `RIBHandler`, and the RIB's `Hider` picks reyes or the ray hider.
`--width`, `--height`, `--grid` and `--bucket` apply to the old path alone. The checkpoint options
and `--denoise` are refused without `--paths`.

**The editor:**

* **`MoyaDelegate` renders through the old core.** It records the shot in a `rib::Recorder` and
  replays it into a `RIBHandler` on a thread of its own. It keeps the new core's scene in step
  through `SceneTranslator`, but renders nothing from it.
* **The retained scene cannot be copied.** `scene::Scene` deletes its copy constructor, so a render
  thread cannot hold a copy while the editor commits the next change.
* **`integrate::render()` asks a `stop` function before each pass**, which can cancel a render and
  report its progress.
* **A batch runs `moya.exe --file <job>.rib --silent`**, the old path, and reads the PNG the job's
  `Display` names.
* **The RIB export writes `Hider` from the render settings** and `Surface "matte"` for every
  surface. The translator writes a `matte` node of language `ri`.
* **The render settings hold a hider**, `hidden` or `raytrace`, saved under the document's
  `render_hider` key. The load refuses any other value.

**What the new core takes from the old:** `offline::film::Sampling.h`'s filters, through the film,
the scene client, the texture maker and the RIB requests. `Executable.h`'s
`executableDirectory()`, through the CPU device and the material library. Nothing else. The old
core includes no new-core header.

**The tests and fixtures:**

* **`v3dtest_render_offline`** has 31 case files. 24 are old: the area, camera, film, framebuffer,
  moving-transform, noise and texture tests, 13 shading language tests and 6 trace tests. Five
  RIB tests stay.
* **`v3dtest_moya`** has 90 case files: 23 old and 56 new. Eleven new tests reach old code:
  `SceneHitsTest` compares depth with the ray hider's, `FoundationTest` compares through
  `Reference.h`, six include `Handling.h` only for its log catcher, and `SceneKindsTest` checks
  defaults against `offline::CameraOptions`.
* **`moya/tests/data` holds 39 RIB scenes**, and two tests count them. 22 were written for the old
  hiders, with 19 committed PNGs. Each of the 39 has a committed description and hit view in
  `scenes/`. Thirteen `.sl` files are there, and five more in `api/render/offline/tests/data`.
* **Two fuzzers**, `fuzz_rib` and `fuzz_sl`, replay their corpus in ctest and run under libFuzzer
  in the portable job.

**Outside the libraries:**

* **`moya/data/` installs two RIB scenes**, and `ctest.yml` renders `raytrace-scene.rib` from the
  install through the old path. It names the ray hider, `background`, `ambientlight`,
  `shinymetal` and `glass`.
* **`scripts/pictures.ts` renders four manual scenes through the old path**, two of them through
  `stripes.sl` and `ripple.sl`.
* **The Flatpak's metainfo** says moya reads RIB and its shading language.
* **Documents:** `Reyes.md`, `RayTracing.md`, `ShadingLanguage.md` and `CamerasAndSampling.md` in
  `docs/offline` describe only old code. The offline README, `RunningAndTesting.md` and `Rib.md`
  describe both. Thirteen of the moya manual's fourteen pages name the old core, and five of them
  name nothing else.
* **Records:** the roadmap names ADR-0026, 0078, 0093, 0094, 0095, 0113, 0114 and 0125 to supersede,
  and 0023, 0025, 0076 and 0092 to amend. ADR-0185 also sets a rule under both hiders, ADR-0174
  names the shading language's fuzzer, and ADR-0022 names the library this plan empties.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0236 | What RI's built-in surfaces, a user shader and `Opacity` become | [1](#step-1--the-record-ris-surfaces-as-materials) |
| 0237 | What RI's ambient light becomes | [2](#step-2--the-record-ris-ambient-light-and-imager) |
| 0238 | What RI's background imager becomes | [2](#step-2--the-record-ris-ambient-light-and-imager) |
| 0239 | Whether RI's C interface stays, and over what | [3](#step-3--the-record-ris-c-interface) |
| 0240 | What `api/render/offline` holds, and its name | [4](#step-4--the-record-the-offline-library) |
| 0241 | Which faces a material emits from, as RI's `Sides` says | [8](#step-8--the-old-scenes-through-the-new-core) |

Each record step researches how production renderers and tools make the choice. The owner
chooses among the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-ris-surfaces-as-materials) | The record: RI's surfaces as materials | `docs/adr` | **0236** | accepted |
| [2](#step-2--the-record-ris-ambient-light-and-imager) | The record: RI's ambient light and imager | `docs/adr` | **0237**, **0238** | accepted |
| [3](#step-3--the-record-ris-c-interface) | The record: RI's C interface | `docs/adr` | **0239** | accepted |
| [4](#step-4--the-record-the-offline-library) | The record: the offline library | `docs/adr` | **0240** | accepted |
| [5](#step-5--ris-surfaces-become-materials) | RI's surfaces become materials | `moya/libmoya/ri`, `material` | 0236 | done |
| [6](#step-6--ambient-light-and-the-background) | Ambient light and the background | `moya/libmoya/ri`, `light`, `output` | 0237, 0238 | done |
| [7](#step-7--every-request-the-old-handler-read) | Every request the old handler read | `moya/libmoya/ri` | — | done |
| [8](#step-8--the-old-scenes-through-the-new-core) | The old scenes through the new core | `moya/tests` | 0241 | done |
| [9](#step-9--moyaexe-renders-by-path-tracing) | moya.exe renders by path tracing | `moya/moya.cxx` | — | done |
| [10](#step-10--the-editors-render-view) | The editor's render view | `vertical3d/src/render` | — | done |
| [11](#step-11--the-editors-batch-export-and-settings) | The editor's batch, export and settings | `vertical3d/src` | — | done |
| [12](#step-12--the-shipped-scenes-and-the-manuals-pictures) | The shipped scenes and the manual's pictures | `moya/data`, `docs/manual/moya`, `scripts`, `.github` | — | done |
| [13](#step-13--tests-on-the-new-core-alone) | Tests on the new core alone | `moya/tests` | — | open |
| [14](#step-14--ris-c-interface) | RI's C interface | `moya/libmoya/ri` | 0239 | open |
| [15](#step-15--the-old-core-leaves-libmoya) | The old core leaves libmoya | `moya/libmoya`, `moya/tests` | — | open |
| [16](#step-16--the-offline-library) | The offline library | `api/render/offline` | 0240 | open |
| [17](#step-17--the-records) | The records the retirement ends | `docs/adr` | — | open |
| [18](#step-18--the-documents) | The documents | `docs` | — | open |

## What blocks what

```
1 ADR ──> 5 Surfaces ──┬──> 8 Old scenes ──> 13 Tests alone ──┐
2 ADR ──> 6 Ambient ───┤                                      │
7 Requests ────────────┴──> 9 moya.exe ──┬──> 11 Batch ───────┤
5 Surfaces ──> 10 Render view ───────────┤                    │
                                         └──> 12 Shipped ─────┤
3 ADR ──> 14 RI's C interface (after 7) ──────────────────────┤
                                                              └──> 15 Old core out
4 ADR ──> 16 Offline library (after 15) ──> 17 Records ──> 18 Documents
```

Steps 1 to 4 depend on nothing. Step 5 needs the record of step 1, and step 6 that of step 2.
Step 7 depends on nothing. Step 8 needs steps 5, 6 and 7, so the old scenes render as the scene
client will read them. Step 9 needs the same three, since moya.exe's default reads every RIB
file. Step 10 needs step 5, because the editor's surfaces are RI's `matte`. Step 11 needs step 9,
because a batch runs moya.exe. Step 12 needs step 9. Step 13 needs step 8's references in place of
the old PNGs. Step 14 needs its record, and step 7 if the interface moves onto the scene client.
Step 15 needs everything before it but the records of steps 3 and 4, so nothing still reads what
it deletes. Step 16 needs its record and step 15. Step 17 sets the records once the code they
describe is gone, and step 18 checks that the documents read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **The statistical oracle.** A scene that changes core is compared with a committed render
  through the path tracer's comparison, within five standard errors. A built-in's material is
  also compared with its expected value where one exists: a white furnace for `matte`, and
  energy conservation for `metal` and `glass`.
* **One picture on every run.** The editor's render of a scene and moya.exe's render of its RIB
  export give the same bytes, since the translator and the scene client describe alike.
* **Nothing reads what is deleted.** Before step 15, no file outside the old core includes one of
  its headers. After step 16, no header names a deleted path, and the include gate and the
  namespace gate pass.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build, and runs `fuzz_rib` under libFuzzer.
* **Pictures change once, by design.** The old hiders' pictures are not kept. Each scene the old
  hiders rendered is rendered once by the path tracer and committed, with its standard errors.
  The new core's reference renders, hit views and scene descriptions stay as they are, except
  where step 7 or step 8 changes what a scene holds. Each such change is committed with the step
  and named in it.

---

### Step 1 — The record: RI's surfaces as materials

**The question.** What a RIB `Surface` becomes in the retained scene: each of RI's seven built-in
surfaces, a user shader the core cannot run, and the `Color` and `Opacity` beside them. Today
each is one diffuse closure in `Cs`, with nothing reported.

**The options:**

* **Each built-in translated into OpenPBR inputs by the scene client**, in its code.
  `plastic`'s `Kd`, `Ks` and
  `roughness` become base and specular weights and a roughness, `metal` a conductor, `glass` a
  dielectric of its `eta`. The rules are code, tested one by one.
* **A library of MaterialX documents, one for each built-in**, installed beside moya and found by
  the shader's name on the shader search path. A user's own document of the same name replaces a
  built-in, as a `.sl` file replaced one before. Karma and RenderMan's Hydra delegate resolve a
  shader's name to a definition in the same way.
* **No translation.** A `Surface` RI names is reported once and takes a default material, as
  RenderMan 21 dropped RSL and asked for its scenes to be rewritten. RIB written for moya names
  `Bxdf` networks.

The record also settles what a user shader the core cannot run becomes: a default material,
reported once, or a refusal of the scene. It settles how `Os` meets OpenPBR's
`geometry_opacity`, how `paintedplastic`'s `texturename` becomes an image node, what becomes of
`Ka`, and what a `Displacement` of language `ri` becomes. It settles the default surface, now
`defaultsurface`, which RI defines as a matte of its own.

**Drafted** as
[ADR-0236](../adr/offline/0236-offline-ris-built-in-surfaces-are-materialx-definitions.md): each
built-in is a MaterialX node definition of RI's parameters and defaults, with a graph onto OpenPBR
Surface, as moya already turns Standard Surface, UsdPreviewSurface and glTF PBR onto it. A
surface of language `ri` compiles to the definition its name finds on the shader search path,
then in moya's own. A name that finds none is reported once and shades as RI's matte,
which is also the default surface. Step 5 settles the rest as rules of the definitions: `Ka` is
accepted and not used, `Os` becomes `geometry_opacity` by its mean, and `texturename` an image
node. A roughness follows Walter and others' conversion from a Phong exponent. A `Displacement`
finds a document's `displacementshader` in the same way, or is reported once.

### Step 2 — The record: RI's ambient light and imager

**The question.** What `LightSource "ambientlight"` and `Imager "background"` become. Both are in
most of the old scenes and in the scene moya installs. An ambient light lights every point by
the same amount, unshadowed, which no physical light does. An imager runs over the finished
pixels.

**The options for the ambient light:**

* **A dome light of constant radiance.** It lights as an overcast sky does: shadowed, and seen in
  reflections. The picture differs from the old one in every corner.
* **A term of its own added at each hit**, unshadowed and not sampled, as an ambient term in a
  game renderer adds. It keeps the old look and is the one light the integrator does not sample.
* **Reported once and dropped**, as RenderMan's RIS renders a scene with an ambient light.

**The options for the background:**

* **The beauty composited over the colour when an output is written**, since the beauty carries
  alpha, as RenderMan's `PxrBackgroundDisplayFilter` does. The film's channels and the deep and
  Cryptomatte outputs are unchanged.
* **A dome light the camera alone sees**, as a lighting artist sets a plate in Karma or RenderMan.
  A dome the camera sees leaves its pixels at zero alpha, the opposite of the imager.
* **Reported once and dropped.** A compositor lays the render over a plate.

The record also settles what any other imager becomes, and whether `AreaLightSource` makes a mesh
light of a shader name other than `arealight`, as it does now.

The research added a fourth option for the ambient light: a dome of constant radiance whose
shadow link names no instance, which the scene already expresses.

**Drafted** as two records, one decision each.
[ADR-0237](../adr/offline/0237-offline-ris-ambient-light-is-an-unshadowed-dome.md): an ambient
light is a dome of constant radiance whose shadow link names no instance, and camera rays do not
see it. On a diffuse surface it gives RI's ambient term, and it stays a light the integrator
samples.
[ADR-0238](../adr/offline/0238-offline-a-background-is-composited-when-an-output-is-written.md):
a background composites an output's beauty over its colour when the output is written, and sets
its alpha to one. `Imager "background"` and RenderMan's `DisplayFilter
"PxrBackgroundDisplayFilter"` both name it, and any other imager or display filter is reported
once. An `AreaLightSource` of a name other than `arealight` is reported once and makes no light,
as any other RSL light is.

### Step 3 — The record: RI's C interface

**The question.** Whether moya keeps RI's C entry points in `ri/RenderMan.cxx`, and what they
drive once `RIBHandler` is gone. They are an adapter over a `rib::Handler`, so the scene client
can take its place. The C interface cannot instance an object, as [TODO.md](../TODO.md) records.

**The options:**

* **An adapter over the scene client**, as it is over `RIBHandler` today. A C program renders
  through the new core, and the object table TODO.md asks for is built.
* **A RIB writer.** `RiBegin` given a file name writes RIB, as Pixar's `librib` and Aqsis's
  `ri2rib` did, and moya.exe renders the file. The C interface needs no scene of its own.
* **Retired.** The retained scene is moya's C++ API, as RenderMan made Riley its API and kept RIB
  as a file. The entry points, the converter and the test of them are deleted.

The record also settles where `RenderMan.h` lives if it stays, since it is moya's only C header.

**Drafted** as [ADR-0239](../adr/offline/0239-offline-ris-c-interface-is-retired.md): RI's C
entry points are deleted, with the converter and the stack of handlers. Nothing outside their own
tests calls them, and they have C++ linkage, so no C program links them now. moya's API is the
retained scene, and a C interface over it, if one is needed, is a decision of its own.

### Step 4 — The record: the offline library

**The question.** What `v3dlib_render_offline` holds once the shading language, the ray tracer,
the old film, the textures and the old camera leave it. What remains is the RIB reader, writer,
recorder and requests, the lexer it shares, and the pixel filters in `film/Sampling.h`, which the
RIB requests name. The editor writes RIB through it, and links libmoya as well.

**The options:**

* **A RIB library of its own**, with the filters' names in its requests and the filters'
  functions moved into libmoya's film. Its directory and namespace follow what it holds, so
  `api/render/offline` becomes `api/rib`, or stays with one subdirectory.
* **`api/render/offline` as it stands, smaller.** It keeps RIB, the lexer and the filters, and its
  name and namespace stay.
* **Folded into libmoya.** RIB is moya's format, as ADR-0195 put the core in libmoya, and the
  editor reaches RIB through libmoya, which it links anyway.

The record also settles whether ADR-0022 is amended or superseded, and how the rename moves the
fuzzer, the tests and the editor's includes.

**Drafted** as [ADR-0240](../adr/offline/0240-offline-rib-is-a-library-of-its-own.md): RIB is
`api/rib`, built as `v3dlib_rib` in `v3d::rib`, with the reader, writer, recorder, requests and
their lexer. A pixel filter is named by RI's name, and its weights move to moya's film. It
supersedes ADR-0022.

### Step 5 — RI's surfaces become materials

* **Each built-in surface becomes the material the record of step 1 names**, with its RI
  parameters read as their defaults say.
* **`Color` and `Opacity` reach the material**, as the record says.
* **A user shader** is reported once by name, and becomes what the record says.
* **A `Displacement` of language `ri`** is reported once, and becomes what the record says.
* **The scene descriptions** of every scene that names a built-in change once, and are committed
  with the step.

**Tests:** each built-in surface's material, read from RIB, holds the inputs its parameters give.
`matte` is white in a white furnace within errors. `metal` and `glass` conserve energy in a
furnace within errors. A user shader is reported once, however often it is named.

**Done.** `material/internal/RiSurfaces.cxx` defines the seven built-ins as
`ND_ri_<shader>_surfaceshader`, with translations onto OpenPBR. They are read with moya's other
translations, not installed as files. A surface of language `ri` compiles through them, and an RI
surface's translation is found by its name, so a user's shader needs no entry of its own.
`Option "searchpath" "shader"` sets the options' new `shaderSearchPath`, and `<shader>.mtlx` there
is read before moya's own. The colour is the geometry's `Cs` primvar or the node's. The opacity is
the mean of the node's `Os` alone, since a primvar read would leave every surface cut out. Every
reference render held within its errors as `matte` moved onto OpenPBR. Each scene description
gained `shaderSearchPath`. The cases that set a gathered material's closures now start from a
material of closures, and the BSDF's colour cases read OpenPBR's base. A `Displacement` found as
a document on the search path is not built, and is in [TODO.md](../TODO.md#offline-rendering).

### Step 6 — Ambient light and the background

* **`ambientlight` becomes what the record of step 2 names**, or is reported and dropped.
* **`Imager "background"` becomes what the record names**, with its `background` colour.
* **Any other imager** is reported once by name.
* **The spot light's default cone delta** follows the built-in's, 0.0698132, since a RIB scene
  that names `spotlight` meant the built-in.

**Tests:** a scene lit by an ambient light alone, and one with a background, each give the
expected value at a pixel within errors, or the report the record names. The spot light's
falloff at its default delta matches the built-in's within errors.

**Done.** `ambientlight` is a dome light of its colour and intensity, whose shadow link names no
instance, and a dome gained `camera`, 1 unless a client sets it, which the ambient dome sets to 0.
A diffuse wall under it shows its albedo times the light beside a card that hides half its sky.
RenderMan's `DisplayFilter` is a request of the RIB reader, writer and recorder now. Its
`PxrBackgroundDisplayFilter` and RI's `background` imager set the options' new `background`,
and `output::composited()` composites an output's beauty, and its denoised beauty, over it when
the output is written. Any other imager, display filter or area light shader is reported once.
The spot light keeps the scene client's default delta of five degrees, which is the RI
specification's. The shading language's built-in used four, so the plan's bullet was wrong. Each
scene that names an ambient light or a background describes them now. The hit views of the
twelve with an ambient light were made again, since the dome shifts every later object's index.

### Step 7 — Every request the old handler read

Every request and parameter `RIBHandler` reads has a stated behaviour in the scene client:
read, reported once, or passed over by a rule [Rib.md](../offline/Rib.md) states.

* **`Hider`.** `"raytrace"` and `"hidden"` are RI's and RenderMan's names for the renderer's
  default, and are read with no report. Any other name is reported once.
* **`ShadingRate`** is passed over. Tessellation follows the new core's edge length in pixels.
* **`Attribute "trace" "bias"`** is passed over, since a ray leaving a surface starts off it by
  its own rule. **`Attribute "light" "nsamples"`** is passed over, since the integrator samples a
  light once at each bounce. Every other attribute the scene client does not read is reported
  once by name.
* **`Option "searchpath" "shader"`** is passed over until step 5 reads it.
  **`Option "limits"`'s bucket and grid sizes** are passed over.
* **`Option "trace" "maxdepth"`** sets the options' `maxDepth`, the most bounces a path takes.
* **`CropWindow`** crops the picture the new core renders, as RI defines it.
* **`Display`'s parameters** the scene client does not read are reported once.
* **`moya --paths` reads every old test scene** without an error.

**Tests:** each request above, read from RIB, gives the scene or the report its rule names. The
cases of `RIBCameraTest`, `RIBGeometryTest`, `RIBOutputTest` and `RIBStateTest` that test what a
request means, rather than how the old hiders draw it, move to the scene client's suite.

**Done.** `Hider`, `ShadingRate`, `Option "trace" "maxdepth"`, the parameters passed over by
rule and the reports of every other parameter are done, and [Rib.md](../offline/Rib.md) states
them. A declaration of an array in a parameter's name, such as `"int bucketsize[2]"`, is not read
by the reader, so RI's standard declarations are used.

`CropWindow` is done. The film holds the window's pixels alone, and the sampler and the camera
place each at its pixel of the image. A crop is therefore the whole image's render there, bit for
bit, in every float and every deep sample. The snapshot names the image and the window's place in
it. An EXR or deep file then has the image as its display window and the crop as its data window,
and a PNG holds the crop alone.

Of the old handler's 87 cases, 31 had a case against the scene client already, and 31 test only
how the old hiders draw. Nine are moya.exe's or the editor's, for steps 9 and 10. The other
sixteen meanings have cases now. They are the camera's `Transform`, depth of field, the screen
window in any order, the specification's cube, transform and attribute blocks, and named spaces.
They are also a shader of no name, an RI displacement, motion flat at both ends, an area light
over a quadric, and RI's width of a curve. Each was broken in the scene client and its case failed.

### Step 8 — The old scenes through the new core

* **Each of the 22 scenes written for the old hiders renders through the scene client.** A scene
  that names a user shader is rewritten in MaterialX. `checked` becomes a checkerboard,
  and `bumpy` a bump from a noise. `lift` becomes a displacement of a subdivision surface, since
  the new core displaces only those.
* **Each pair of a reyes scene and a ray-traced one** that now describe the same scene becomes
  one scene.
* **Each scene is rendered and committed** as the path tracer's references are, with its
  standard errors, and compared within five of them.
* **The 19 PNGs and the scenes' old cases are deleted**: `ReyesReferenceTest`,
  `SurfaceReferenceTest` and `TracedReferenceTest`. The hard-coded count of scenes in
  `SceneWorldTest` and `SceneHitsTest` follows.
* **The scenes' descriptions and hit views** are committed again where the scene changed.

**Tests:** the committed renders, and a case for each that renders it with a light twice as
bright and checks that the comparison fails.

**Done.** Six pairs described the same scene, and two scenes differed from another only in
their sampling, so 22 scenes are 14. Each is rendered by the path tracer and compared within
five standard errors. A scene that took one sample a pixel takes 64, since one sample has no
error, and `textured.rib`'s dome needs 256. The old ray hider's depth check reads each scene at
pixel centres whatever sampling it names. `checked.mtlx` and `bumpy.mtlx` define those two RI
surfaces on the shader search path, and `reference-displaced.rib` is a subdivision surface that
`rippled.mtlx` displaces. `textured.rib`'s quad is diffuse, since the ambient light is a dome.
The brighter case doubles RI's `constant` colour too, the only light of four scenes.

RI's `constant` surfaces seen from behind rendered black, since a triangle emitted from its
front alone. [ADR-0241](../adr/offline/0241-offline-ris-sides-decides-which-faces-a-material-emits-from.md)
has RI's `Sides` decide: the reader reads it, the scene client sets an instance's new
`doubleSided`, and an emitting material on a two-sided instance emits from both faces. Every
scene description gained `doubleSided`. The 19 PNGs and the three old reference suites are
deleted, and `Reference.h` keeps what the foundation chart and the depth case use.

### Step 9 — moya.exe renders by path tracing

* **`moya --file <scene>.rib` renders through the new core.** The path tracer is the default, and
  `--paths` is removed.
* **`--width` and `--height`** replace the size the scene names, as they did for the old path.
  **`--grid` and `--bucket`** are removed.
* **The checkpoint options and `--denoise`** are taken without `--paths`.
* **An output the scene names** is written as it is now. A scene that names none writes the
  beauty to the file the old path wrote.
* **The changelog** names each change to the command line.

**Tests:** `CommandLineTest` renders a scene through the default and checks its output. A removed
option is refused with a message that names it.

**Done.** `moya --file <scene>.rib` renders through the scene client and the path tracer.
`--paths`, `--grid` and `--bucket` are not options any more, so each is refused by name as one
moya does not recognise. `--width` and `--height` set the scene client's new `resolution()`,
which a `Format` does not change. A side given alone, or out of range, is refused where the old
path printed a line and kept the scene's size. The checkpoint options and `--denoise` are
refused only with `--scene` or `--hits`, which render no picture.

The old path wrote no file for a scene with no `Display`, so the plan's bullet named nothing.
Such a scene writes its beauty to an EXR file named after it in the working directory, in place
of `paths.exr`. A render prints Pixar's notice, as the old handler did, and logs its time. A file
it cannot write is logged, the frames after it still render, and the file is printed once the
scene is read. The editor's batch and the install job run moya.exe as before, and render through
the new core. The command line page, the changelog and the reference pages that named the old
options state the new rules. The manual's pages on the hiders are step 18's. This step also makes
the editor scripting plan's verification name the path-traced references in place of the PNGs,
which step 8 left undone. Step 8 also left the editor's translator describing every instance as
one-sided, where the scene client reads its RIB export as RI's default of two, so two cases of
`SceneTranslatorTest` failed. The translator's instances are two-sided now.

### Step 10 — The editor's render view

* **`MoyaDelegate` renders its retained scene** through `integrate::render()` on its thread. The
  recorded shot and the `RIBHandler` go.
* **The scene is not changed while a render reads it.** An edit stops the running render at its
  next pass. The delegate then applies the update and starts again. Edits while a render runs are
  section 9's.
* **Progress** is the passes done over the passes the settings ask for, reported before each
  pass. **Cancel** is the `stop` function's answer.
* **The picture** is the snapshot through the default display and view, as an 8-bit output is
  written.

**Tests:** `DelegateTest` renders a scene through the new core, cancels one, and reports its
progress. Its picture is the same bytes as moya.exe's render of the scene's RIB export.

**Done.** `MoyaDelegate` renders its retained scene through `integrate::render()` on its thread,
and the recorded shot and the `RIBHandler` are gone from it. The delegate's base class cancels a
running render and waits for its thread before the next one starts, so the translator changes
the scene only while no render reads it. The progress is the passes done over
`integrate::passesOf()`, a new function that gives the passes a scene's samples take. The picture
is `output::displayed()` of the snapshot, composited over the first output's background where it
names one, as a PNG output is written. A render of a cube and a batch's moya.exe render of its
export give the same bytes. The editor and its suite now carry moya's CPU libraries and
MaterialX's standard libraries beside them, as moya.exe does. The render settings' hider changes
nothing now, and [Rendering.md](../editor/Rendering.md) and the editor manual's rendering page say
so until step 11 removes it.

### Step 11 — The editor's batch, export and settings

* **A batch runs moya.exe on its default path**, and reads the PNG each job names, as now.
* **The RIB export writes no `Hider`.**
* **The render settings lose the hider.** The settings editor drops its list. A document that
  holds `render_hider` loads, and the key is read and passed over. The document needs no new
  version if [Files.md](../editor/Files.md#scene-documents) allows a key to be passed over at
  every version. Otherwise the step takes the next schema version after the editor scripting
  plan's.
* **The samples along each side** stay as they are, and reach moya through `PixelSamples`.

**Tests:** `BatchTest` renders a frame through moya.exe's default. `RIBExportVisitorTest` writes
no `Hider`. A document saved with each hider loads with the settings it had otherwise.

**Done.** A batch already ran moya.exe's default once step 9 landed, so its code is unchanged.
`RenderSettings` holds the size and the samples alone, the settings editor drops its list, and
the RIB export writes no `Hider`. The store reads the meta table by a list of keys, so a key
outside the list is passed over at every version. `render_hider` left the list, and a document
that holds it loads at its own version, whatever it names, where one that named another hider
was refused. No new schema version is needed. A document the editor saves whole drops the row,
and one it saves in part keeps it, passed over. [Files.md](../editor/Files.md) now names the
render settings' keys and the rule.

### Step 12 — The shipped scenes and the manual's pictures

* **`moya/data/`'s two scenes** are rewritten for the new core, and the CI job that renders one
  from the install renders it through the default.
* **The manual's four scenes** are rewritten, `stripes.sl` and `ripple.sl` as MaterialX graphs
  beside them. `pictures.ts` renders them through the default, and the pictures are made again.
* **`pictures.test.ts`** follows the change.

**Tests:** the install job renders its scene. `pictures.test.ts` passes.

**Done.** The shipped scenes and the manual's four name no `Hider`. The manual's take 64
samples a pixel, and the shipped ones 16 and 64. `stripes.mtlx` and `ripple.mtlx` define RI's
`stripes` surface and `ripple` displacement beside their `.sl` files, which Shaders.md and
Displacement.md still show until step 18. `ripple` needed what ADR-0236 decided and step 5 left
out: an RI displacement now takes `ND_ri_<shader>_displacementshader` from the shader search
path, and its graph is flattened into the network. The panel it ripples is a subdivision
surface. The stripes run across a panel with its own `st`, since a quadric has none in the new
core, which [TODO.md](../TODO.md) records. The four pictures were made again. `pictures.ts` and
the install job needed no change.

`test-scene-01.rib` rendered black: RI's default surface is a matte now, and the scene had no
light, so it gained a distant light and an ambient one. In `raytrace-scene.rib` the glass lost
its `Opacity`, which the old ray hider read only as a shadow's and the new core reads as a cut.
Its metal sparkled under the ambient light, which only a light sample finds, so the scene lights
its sky with `PxrDomeLight`, and [Rib.md](../offline/Rib.md) states why.

### Step 13 — Tests on the new core alone

* **`Handling.h`'s log catcher** moves to a header of its own, which names no old code.
* **`FoundationTest`'s comparison** goes through the new core's comparison, or a helper of its
  own, in place of `Reference.h`.
* **`SceneHitsTest`'s depth case** takes the brute-force traversal as its reference in place of
  the ray hider.
* **`SceneKindsTest`** states RI's defaults as numbers, in place of `offline::CameraOptions`.
* **No new-core test includes an old-core header.**

**Tests:** each moved case still fails when the value it checks is broken.

**In progress.** The log catcher is in `Heard.h`, which names no old code, and `Handling.h`
includes it. The crop's case in `DeepTest.cxx` uses it. The six tests that include `Handling.h`
for it alone still do.

### Step 14 — RI's C interface

* **The C entry points become what the record of step 3 names.** They are moved onto the scene
  client, made a RIB writer, or deleted with `Renderer`, the converter and `RenderManTest`.
* **The TODO.md entry on `RiObjectBegin`** is done or deleted as the record decides.
* **The warning flags and the clang-tidy exemption** that `RenderMan.cxx` needs go if it goes.

**Tests:** as the record decides. An adapter renders a scene through the C interface and gives the
same scene description as its RIB.

### Step 15 — The old core leaves libmoya

* **Deleted:** `reyes/`, `raytrace/`, `state/`, `Hider.h`, `Curve`, `Polygon` and `Vertex`,
  `ri/RIBHandler`, and the 23 old test files with `Reference.*` and `Handling.h`.
* **The `.sl` fixtures** in `moya/tests/data` are deleted.
* **`Executable.h`** stays, as a helper the device and the material library use.
* **`v3dlib_moya`'s link to the offline library** becomes private if no public header of libmoya
  names it.

**Tests:** the suites pass with the old code gone.

### Step 16 — The offline library

* **Deleted:** `sl/`, `trace/`, `texture/`, `film/Film`, `FrameBuffer` and `Sampler`, `Area`,
  `Camera`, `CameraOptions`, `MovingTransform`, `Uniform` and `Units`, and their 24 test files.
* **`fuzz_sl`** and its corpus files are deleted. The portable job's script fuzzes what remains.
* **What remains moves** as the record of step 4 names, with its includes, namespaces, tests and
  fuzzer.
* **`dirsize.ts`'s exemption** for `sl/syntax` goes.

**Tests:** the suites pass, and `fuzz_rib` replays its corpus.

### Step 17 — The records

* **Superseded by ADR-0186:** ADR-0026, 0078, 0093, 0094, 0095, 0113, 0114, 0125 and 0185, each
  where the code it decides is gone.
* **Amended:** ADR-0023, 0025, 0076 and 0092, as the roadmap names, and ADR-0174 for the
  shading language's fuzzer. Each is read against what remains, and superseded where nothing of
  its rule is left.
* **ADR-0022** is superseded by ADR-0240, as the record of step 4 sets. ADR-0195 is read against
  it, since it names `v3dlib_render_offline` as the library that keeps RIB.
* **The index and the tags** are regenerated.

### Step 18 — The documents

* **Deleted:** [Reyes.md](../offline/Reyes.md), [RayTracing.md](../offline/RayTracing.md) and
  [ShadingLanguage.md](../offline/ShadingLanguage.md). Any rule in
  [CamerasAndSampling.md](../offline/CamerasAndSampling.md) that the new core keeps, RI's camera
  among them, moves to the page that owns it, and the page is deleted.
* **[The offline README](../offline/README.md)** describes one renderer: its glossary, its
  pipeline from RIB to the scene to the pictures, and what is not built.
* **[RunningAndTesting.md](../offline/RunningAndTesting.md)** and **[Rib.md](../offline/Rib.md)**
  describe the new core and the one handler.
* **The moya manual** is rewritten for the path tracer. Hiders, the shading language, shaders and
  displacement shaders give way to materials, lights and displacement through MaterialX.
  `HowMoyaRenders.md`, `Quality.md` and `CommandLine.md` describe the new core.
* **[Rendering.md](../editor/Rendering.md)** and the editor manual's rendering page describe the
  new core behind the render view and the batch, and settings with no hider.
* **[Testing.md](../contributing/Testing.md)** names the references that replace the PNGs.
* **`docs/SUMMARY.md`**, the root README, `moya/README.md`, the offline library's README,
  `GettingStarted.md` and the Flatpak's metainfo follow.
* **The changelog** says the old hiders and the shading language are gone.
* **The roadmap's section 7** links here, and says so when this plan closes. Its section on what
  carries over and what is retired is brought up to date.

---

## Working beside the editor scripting plan

[EditorScripting](EditorScripting.md) is open. It takes ADR-0217 to ADR-0221, and this plan
takes 0236 to 0240.

| Shared ground | This plan changes it in | The editor scripting plan changes it in |
|---|---|---|
| `src/render/MoyaDelegate.*`, `src/control/RenderView.*` | Step 10 | Step 14 |
| `src/render/Batch.*`, `Process.*` | Step 11 | Steps 14 and 17 |
| `src/render/RIBExportVisitor.*`, `SceneTranslator.*`, `tests/render/` | Steps 5 and 11 | Step 10 |
| `src/scene/RenderSettings.h`, its codec and its editor | Step 11 | Nothing |
| The scene document's schema version | Step 11, only if a new version is needed | Step 8 |
| `moya/moya.cxx`, `docs/manual/moya/CommandLine.md` | Step 9 | Step 16 |
| `docs/editor/Rendering.md` | Step 18 | Steps 14, 17 and 19 |

**The rules:**

* **Each commit names its paths**, with `git commit -- <paths>`, so neither plan's staged work
  lands in the other's commit.
* **Step 9 and the editor scripting plan's step 16 meet in `optionsOf()` and `conflict()`.**
  Whichever lands second adapts. Its thread count holds for the default path once step 9 lands,
  and the old hiders' single thread is no longer a case.
* **The editor scripting plan's check that no picture changes** holds until step 8. This plan
  updates that plan's verification when step 8 lands, so its rule names the path-traced
  references in place of the PNGs.
* **The batch keeps its jobs' form**: a RIB file, and the picture moya.exe writes from it.
