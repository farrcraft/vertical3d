# Moya Compositing — Outputs, Light Paths, IDs, Deep Pixels and Denoising

Drafted 2026-11-13 against `7cc2313a`. This plan takes up section 6 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, the new core writes what a
compositor needs, not only a beauty. The scene's outputs decide the film's channels and the files
written. A channel collects the light a light path expression names, or a value at the first hit.
Light groups let a light be changed after rendering. Cryptomatte channels give each object's and
material's coverage, a deep file gives each pixel's samples in depth, and OIDN denoises the beauty
from its albedo and normal. The reyes hider and the ray hider keep rendering every RIB scene and
every editor render, and no picture they make changes.

The rebuild's decisions are ADR-0186 to ADR-0194, and the decisions of its first five plans are
ADR-0195 to ADR-0229, all accepted. This plan follows four of them:

* [ADR-0187](../adr/offline/0187-offline-moya-owns-its-algorithms-libraries-own-formats.md):
  moya implements its own light path expressions, coverage and deep samples. OpenEXR writes the
  files, and OIDN denoises.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): each device
  gives one picture on every run, so every channel, ID and deep sample is the same under any
  thread count.
* [ADR-0199](../adr/offline/0199-offline-a-sample-lands-in-the-one-pixel-it-was-drawn-for.md): a
  sample is added to the one pixel it was drawn for, so a pixel's coverage and deep samples come
  from its own samples alone.
* [ADR-0212](../adr/offline/0212-offline-the-path-state-is-declared-once-laid-out-per-device.md):
  what a light path expression adds to a path is declared once in the path state.

The plan keeps the roadmap's order, with five changes:

* **Five decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. Two are what a channel collects and how a light path expression runs on every
  device. Three are how a pixel's IDs are covered, how deep samples are kept, and what the
  denoiser reads and writes. [Decisions](#decisions).
* **The scene's outputs reach the render first.** The path tracer makes its film with no channels
  and `moya --paths` writes the beauty alone, whatever the scene names. Every later step needs a
  film made from the scene's outputs. [Step 7](#step-7--the-scenes-outputs-reach-the-render).
* **Coverage is a step of its own.** A compositor needs alpha, and the film has none. The roadmap
  does not name it. [Step 8](#step-8--alpha).
* **Values at the first hit come before light path expressions.** Depth, position, normal, albedo
  and a primvar are the simplest channels, and the denoiser and the deep file read them.
  [Step 9](#step-9--values-at-the-first-hit).
* **RIB names every output.** A RIB file names its channels, light groups, Cryptomatte layers and
  deep files, so `moya --paths` renders them and the reference renders are RIB files.
  [Step 17](#step-17--outputs-from-rib).

## What the survey found

The tree was read at `7cc2313a`.

**The film and its files:**

* **The film holds a beauty and named channels**
  ([Film.md](../offline/Film.md#the-films-pixels)). Each channel is a name and one to four floats.
  `Pixels::addChannel()` adds a sample's value with the beauty's weight, so every channel is
  filtered as the beauty is. Its sums are floats, added in index order.
* **A snapshot describes its channels**, and a checkpoint holds them, with the film's channels in
  its fingerprint.
* **`writeExr()` writes parts of chosen outputs**, each half or float and compressed as it says
  ([Output.md](../offline/Output.md#exr-files)). One part is a scanline file, and several a
  multi-part file. A channel of several floats is written as `name.R`, `name.G` and so on.
* **The beauty has no alpha**, and no file holds one.
* **OpenEXR writes no deep file** through moya, though the library reads and writes deep
  scanlines.

**The scene and the render:**

* **The scene holds `channel` and `output` objects** ([Scene.md](../offline/Scene.md)). A channel
  names its `source` and its `components`. An output names its `file`, its `format` of `exr`,
  `png` or `framebuffer`, the channels it writes, its `precision` and its `compression`.
* **The RIB client makes them from `Display`** ([Rib.md](../offline/Rib.md#the-scene-client)). A
  mode other than `rgb` or `rgba` adds a channel whose source is the mode.
* **`integrate::render()` reads none of them.** It makes its film with an empty list of channels.
* **`moya --paths` writes the beauty alone**, in floats, to `--output` or `paths.exr`.

**The integrator:**

* **A path gathers into one radiance** ([Integrator.md](../offline/Integrator.md#the-stages)). The
  shade stage adds what a light sample, a light's shape, a mesh light or a dome light gives, and
  accumulate adds it to the pixel. Nothing records which kind of bounce or light gave it.
* **A BSDF sample names its lobe**: diffuse, glossy, specular or transmission. The path keeps a
  count of each, and whether the last sample was specular.
* **A hit names the chain of instances that placed it**, up to four deep, and its primitive. The
  instances and lights are scene objects with names, but no name reaches the device.
* **Light linking keeps a set of linked lights on the path**, as one 32-bit field.

**The editor:** `MoyaDelegate` renders through the reyes and ray hiders, from a RIB recording of
the editor's scene. It writes 8-bit pictures.

**Dependencies:** OIDN is not a dependency. OpenEXR, oneTBB and OpenColorIO are, in the `moya`
feature of `vcpkg.json`.

**The tests:** moya's suite has 34 RIB scenes, each with a committed description and hit view,
and 11 reference renders of the path tracer held within five standard errors
([Testing.md](../contributing/Testing.md#moya-references)).

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0230 | What a channel collects, and how it is named | [1](#step-1--the-record-what-a-channel-collects) |
| 0231 | How a light path expression runs on every device | [2](#step-2--the-record-light-path-expressions) |
| 0232 | How a pixel's IDs are covered | [3](#step-3--the-record-cryptomatte-coverage) |
| 0233 | How a pixel's deep samples are kept | [4](#step-4--the-record-deep-samples) |
| 0234 | What the denoiser reads and writes | [5](#step-5--the-record-denoising) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-what-a-channel-collects) | The record: what a channel collects | `docs/adr` | **0230** | open |
| [2](#step-2--the-record-light-path-expressions) | The record: light path expressions | `docs/adr` | **0231** | open |
| [3](#step-3--the-record-cryptomatte-coverage) | The record: Cryptomatte coverage | `docs/adr` | **0232** | open |
| [4](#step-4--the-record-deep-samples) | The record: deep samples | `docs/adr` | **0233** | open |
| [5](#step-5--the-record-denoising) | The record: denoising | `docs/adr` | **0234** | open |
| [6](#step-6--oidn) | OIDN as a dependency | `vcpkg.json`, `ports` | — | open |
| [7](#step-7--the-scenes-outputs-reach-the-render) | The scene's outputs reach the render | `moya/libmoya/integrate`, `moya` | 0230 | open |
| [8](#step-8--alpha) | Alpha | `moya/libmoya/film`, `kernel` | — | open |
| [9](#step-9--values-at-the-first-hit) | Values at the first hit | `moya/libmoya/kernel/integrate` | 0230 | open |
| [10](#step-10--the-events-of-a-path) | The events of a path | `moya/libmoya/kernel/integrate` | 0231 | open |
| [11](#step-11--compiling-a-light-path-expression) | Compiling a light path expression | `moya/libmoya/lpe` | 0231 | open |
| [12](#step-12--light-path-expressions-on-the-device) | Light path expressions on the device | `moya/libmoya/kernel` | 0231 | open |
| [13](#step-13--light-groups) | Light groups | `moya/libmoya/light`, `scene` | 0231 | open |
| [14](#step-14--cryptomatte) | Cryptomatte | `moya/libmoya/film`, `output` | 0232 | open |
| [15](#step-15--deep-exr) | Deep EXR | `moya/libmoya/film`, `output` | 0233 | open |
| [16](#step-16--denoising) | Denoising through OIDN | `moya/libmoya/denoise`, `moya` | 0234 | open |
| [17](#step-17--outputs-from-rib) | Outputs from RIB | `api/render/offline/rib`, `moya/libmoya/ri` | — | open |
| [18](#step-18--the-reference-renders) | The reference renders | `moya/tests` | — | open |
| [19](#step-19--speed) | Speed: outputs against the baseline | `moya/bench` | — | open |
| [20](#step-20--the-documents) | The reference pages for outputs | `docs` | — | open |

## What blocks what

```
1 ADR ──> 7 Outputs ──┬──> 8 Alpha ──────────────────────┬──> 15 Deep (after 4)
                      ├──> 9 First hit ─────────────────┼──> 16 Denoise (after 5, 6)
                      │                                  └──> 14 Cryptomatte (after 3)
2 ADR ──> 10 Events ──> 11 Compile ──> 12 Device (after 7) ──> 13 Light groups
6 OIDN (after 5)

(7 to 16) ──> 17 RIB ──> 18 References, 19 Speed ──> 20 Documents
```

Steps 1 to 5 depend on nothing but each other, and step 6 needs the record of step 5. Step 7
needs the record of step 1, and every step after it needs a film made from the scene's outputs.
Steps 8 and 9 need only step 7. Step 10 needs the record of step 2, step 11 the events it names,
and step 12 both and a film of channels. Step 13 is a label of step 12's expressions. Step 14 needs
coverage and the record of step 3. Step 15 needs alpha, the first hit's depth and the record of
step 4. Step 16 needs the first hit's albedo and normal, and OIDN. Step 17 names every output in
RIB. Each step updates its own section of the reference documents, and step 20 checks that they
read as a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suites and the editor's suite pass. Each step with
  code adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **Channels that split the beauty add up to it.** A set of light path expressions that splits
  every path one way, by the lobe of the first bounce for one, sums to the beauty at each pixel
  within float rounding. A scene's light groups sum to it too.
* **The statistical oracle.** A channel is compared with its expected value, or with another
  picture, through the path tracer's comparison within standard errors.
* **One picture on every run.** A render on one thread and on every thread gives the same bytes in
  every channel, every ID and every deep sample.
* **The files open in other readers.** Each file's layout is checked against what OpenEXR's own
  readers give: its deep reader and compositor for a deep file, and the Cryptomatte
  specification's decoding for its channels and manifest.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build.
* **No old picture changes.** The old hiders read RIB as before, so every committed PNG in
  `moya/tests/data/` stays as it is, and the editor's render window shows the same picture. A
  render that names no channel writes the beauty it writes now, so the path tracer's reference
  renders stay as they are.

---

### Step 1 — The record: what a channel collects

**The question.** What a scene's channel can hold, how it names what it holds, and how its
samples are combined into a pixel. The scene's `channel` names a `source` string today, which
nothing reads.

**The options:**

* **USD's render vars**: a channel names a `sourceType` of `raw`, `primvar`, `lpe` or
  `intrinsic` and a `sourceName`, as UsdRender's `RenderVar` does, and as Karma and Hydra
  delegates read them. A Hydra delegate of section 12 then hands its vars through unchanged. RIB's
  `DisplayChannel` maps onto it, as RenderMan's own Hydra delegate maps them.
* **RenderMan's display channels**: a channel names its source as a string, `lpe:` followed by an
  expression or the name of a built-in value, as RenderMan's `DisplayChannel` does. RIB reads as
  written, and a USD var is translated.
* **A fixed set of passes**, as Cycles' view layer gives diffuse, glossy and transmission light,
  each direct and indirect, and its data passes. Each is cheap and known ahead, and none can be
  written that the set lacks.

The record also settles which values a channel can take at the first hit, such as depth, position,
normal, albedo and motion. It settles how a channel's samples are combined into its pixel. They may
be averaged by the pixel filter, or kept as the nearest, least or most, as RenderMan's channel
`filter` and Arnold's AOV filters keep depth and IDs.

### Step 2 — The record: light path expressions

**The question.** How a channel that names a light path expression collects only the light of
the paths it matches, on every device, within kernel code's rules: no allocation and no virtual
call.

**The options:**

* **A deterministic automaton per expression**, compiled on the host from OSL's grammar and run
  on the device, one state per expression held in the path state, as OSL's `accum` library,
  Arnold and RenderMan run them. Any expression a compositor writes works. Each expression costs a
  state on each path and a table lookup at each event.
* **One automaton for every expression**, its states the tuples of each expression's states,
  with each accepting state naming the channels it adds to. A path carries one state, and the
  table grows with the product of the expressions.
* **Fixed passes, recorded by the path's lobes**, as Cycles splits light by the first bounce's
  lobe and whether it was direct. A path carries a few flags, and no other split can be asked for.

The record also settles which events and labels a path reports, and the most expressions a render
takes. It settles how a light sample is matched, since its light has not yet been met. Last, it
settles how a light group is named: as a label in an expression, as OSL's grammar writes one, or
as an attribute of the light, as Cycles' and RenderMan's `lightGroup` is.

### Step 3 — The record: Cryptomatte coverage

**The question.** How a pixel's coverage by each object and material ID is kept while samples
arrive, within kernel code's rules, so that each pixel's IDs are ranked by coverage when it is
written. The Cryptomatte specification fixes the hash, the channels and the manifest, but not how
a renderer gathers coverage.

**The options:**

* **A bounded table of IDs in each pixel**, each ID with its weighted coverage, added to by the
  pixel's own samples in index order, as Cycles keeps one in its film. Memory is a fixed number of
  slots per pixel and layer. A pixel of more IDs than slots loses its smallest.
* **Each sample's ID written out**, and the pixel's table built on the host after each pass, as
  Arnold's Cryptomatte AOV shader gathers samples into a list. Nothing is lost, and memory grows
  with the samples of a pass.
* **The table kept on the host alone**, with the device reporting each camera hit's ID and
  weight through a queue. The device side is smallest, and each GPU pass copies every hit back.

The record also settles how many ranks a layer writes, and the names a layer takes its IDs from,
as the specification's object, material and asset layers do. It settles how coverage meets a
surface that is cut out or seen through glass.

### Step 4 — The record: deep samples

**The question.** How a pixel's samples along depth are kept and merged, so that a deep file
holds a bounded list of samples per pixel, each with its depth, colour, alpha and IDs. OpenEXR
fixes the file, and not how a renderer builds its lists.

**The options:**

* **A bounded list in each pixel, merged by depth and ID**, as Arnold's and RenderMan's deep
  outputs merge samples whose depths lie within a tolerance and whose IDs agree. Memory is fixed,
  and the tolerance decides what is merged.
* **Every camera hit kept, and merged on the host when the file is written.** Nothing is lost
  until writing, and memory grows with the samples.
* **The first hit alone, with volumes later adding their own samples.** One sample per hit
  surface and pixel, and no transparency through glass.

The record also settles what a sample's back depth is, and the channels a deep file holds beside
depth and alpha. It settles whether a surface seen through glass or a cut-out card adds samples.

### Step 5 — The record: denoising

**The question.** What OIDN reads and what it writes, so that a denoised picture keeps the
compositing outputs useful and meets ADR-0194's one picture per device.

**The options:**

* **The beauty alone, denoised into a channel of its own**, with the noisy beauty kept, as
  Cycles' denoising data and Karma's denoised beauty are written. A compositor can choose either.
* **Every light path expression and light group denoised as well**, as RenderMan's denoiser and
  V-Ray's denoise their light selects. Changing a light group after rendering stays clean, and each
  channel costs a filter run.
* **The beauty denoised in place.** One picture, and the noisy one is lost.

The record also settles whether OIDN cleans the albedo and normal before they guide the beauty,
as its `cleanAux` allows, and in what space the normal is given. It settles whether a picture shown
while sampling is denoised. Last, it settles whether OIDN on the CPU gives the same bytes under any
thread count, and what moya does if it does not.

### Step 6 — OIDN

* **OIDN joins the `moya` feature** in `vcpkg.json`, on its CPU device alone. Its GPU devices wait
  for sections 10 and 11.
* **The port is checked**: its version, that it builds against the tree's oneTBB, and the ISPC
  it needs. An overlay port in `ports/` is added only if the baseline's will not build.
* **Its weights are installed beside moya**, if the port keeps them apart from the library.
* **The Flatpak builds it** as a module.
* **[Dependencies.md](../contributing/Dependencies.md)** names the port and why.

**Tests:** a filter of OIDN's denoises a constant image to the same constant.

### Step 7 — The scene's outputs reach the render

* **`integrate::render()` makes its film from the scene's channels**, as the record of step 1
  decides, the channels every output names in the order the scene holds them.
* **A channel of a source the render cannot fill** is reported once, and holds zero.
* **`moya --paths` writes every output the scene names**, each to its file in its format and
  precision. `--output` replaces the first output's file, as it replaces the file `Display` names
  for the old hiders. A scene that names no output writes the beauty to `paths.exr`, as now.
* **A checkpoint holds the channels**, and its fingerprint names them, as it does now for a film's
  channels.

**Tests:** a scene of two outputs writes two files, with the channels each names. A scene with
no output writes what it writes now. A checkpoint of a render with channels resumes to the same
bytes.

### Step 8 — Alpha

* **The film holds a coverage beside the beauty**: each sample's weight times one when its camera
  ray met geometry or a light's shape, and zero when it left the scene.
* **Coverage is the beauty's `A`**, so the beauty is written as R, G, B and A.
* **A camera ray that passes a cut-out surface** counts the surface by the opacity it kept, as the
  ray's own passing decides.
* **A dome light the camera sees** leaves its pixels at zero coverage, so a compositor lays the
  render over a plate.

**Tests:** a sphere's coverage is one inside it, zero outside, and its share of the pixel at its
edge, within errors. A card of half opacity has a coverage of a half, within errors.

### Step 9 — Values at the first hit

* **Each value the record of step 1 names** is added at the camera ray's first hit: depth,
  position, normal and the others it settles.
* **A primvar's value at the first hit**, by name, as UsdRender's `primvar` source names one.
* **Albedo and normal for the denoiser**, from the first hit that is not a perfect mirror or a
  smooth transmission, as the roadmap names. The albedo is the BSDF's albedo at the outgoing
  direction.
* **Each value is filtered** as the record settles.

**Tests:** a plane square to the camera gives the depth a closed form gives at each pixel. A
mirror in front of a red wall gives the wall's albedo. A primvar of a constant reads back as
the constant.

### Step 10 — The events of a path

* **Each bounce reports its event** as the record of step 2 decides: the camera, a reflection or
  a transmission, with its lobe's kind of scattering, and a light, an emitting surface or the
  background.
* **A light reports its label**, and a light group, as the record settles, from its name on the
  host.
* **A light sample reports the event its light would give**, so the shade stage can match it.
* **Nothing is gathered differently.** The events are recorded beside the radiance, and the
  beauty is unchanged.

**Tests:** a path of known bounces in a scene built for it reports the events in order. Every
reference render of the path tracer is unchanged, byte for byte.

### Step 11 — Compiling a light path expression

* **OSL's grammar of light path expressions**: events, scattering types and labels, groups,
  alternatives, repetition and negation, as OSL's `accum` library reads them.
* **An expression compiles to an automaton** as the record of step 2 decides, by subset
  construction over the events step 10 names, and is minimised.
* **An expression that does not parse** is reported with its place, and its channel holds zero.

**Tests:** each construct of the grammar against strings of events it should and should not
match. Two expressions OSL's documentation says are equal compile to the same automaton.

### Step 12 — Light path expressions on the device

* **Each path carries its expressions' state**, declared once in the path state as the record
  decides, and advanced at each event.
* **Light the path gathers is added to each channel whose expression accepts** the path's events
  with the light's.
* **The most expressions a render takes** is the record's, and a scene of more is reported.

**Tests:** the lobes of the first bounce split the beauty into channels that sum to it at each
pixel. `C<RD>L` in a diffuse box under one light matches a direct render's light alone, within
errors. A render with five expressions gives the same beauty as one with none, byte for byte.

### Step 13 — Light groups

* **A light names its group** as the record of step 2 settles, in the scene and through UsdLux's
  names where they exist.
* **A channel of each group** collects the light of that group's lights.
* **A light with no group** belongs to none, and a channel of light no group names collects it.

**Tests:** a scene's groups sum to the beauty at each pixel. A group's channel scaled by two
matches a render whose lights in that group are twice as bright, within errors.

### Step 14 — Cryptomatte

* **IDs are hashed from names** with MurmurHash3 as the Cryptomatte specification says, and
  made a float that is not a NaN or an infinity, as it says.
* **Each pixel's coverage** is kept as the record of step 3 decides, from each camera sample's
  first hit and its weight.
* **A layer of each kind the record settles**, written as `<layer>00`, `<layer>01` and on, each
  holding two ranks of ID and coverage as `R`, `G`, `B` and `A`.
* **The manifest is in the header**, as `cryptomatte/<key>/name`, `hash`, `conversion` and
  `manifest`, with the key from the layer's name as the specification gives it.

**Tests:** a pixel inside one object holds its ID at a coverage of one. A pixel on the edge
between two holds both, with the shares their areas give, within errors. The manifest names every
object in the picture, and each hash matches the specification's examples.

### Step 15 — Deep EXR

* **An output of the format `deepexr`** writes a deep scanline file through OpenEXR.
* **Each pixel's samples** are kept and merged as the record of step 4 decides, each with its
  front and back depth, its colour, its alpha and its IDs.
* **A pixel with no sample** holds none.

**Tests:** flattening the deep file through OpenEXR's own compositing gives the flat beauty and
alpha, within float rounding. A pixel behind half-opaque glass holds a sample for the glass and
one for the surface behind it.

### Step 16 — Denoising

* **A denoised output** as the record of step 5 decides, after the last pass, through OIDN's
  `RT` filter, guided by the albedo and normal of step 9.
* **`moya --paths --denoise`** denoises a render whose scene names no denoised output, and the
  scene's options can ask for one.
* **A failed filter** is reported, and the noisy picture is written.

**Tests:** a denoised render of a few samples is nearer the converged picture than the noisy one,
in each pixel's mean squared error. A render denoised twice gives the same bytes, under one thread
and every thread, if the record settles that it must.

### Step 17 — Outputs from RIB

* **RIB's `DisplayChannel`** declares a channel, with its `source` of an expression or a value at
  the first hit, as RenderMan names one. A `Display` mode names those channels.
* **A light's group** is named as RenderMan's `lightGroup` parameter names it, on `Light` and on
  `LightSource`.
* **Cryptomatte and deep outputs** are named as RenderMan names them, through a `Display` of
  their type, and a denoised one through the scene's options.
* **`moya --scene` prints the outputs**, and their descriptions are committed with each scene.

**Tests:** a RIB file of each output gives the scene the channels and outputs it names. A RIB
file that names an expression and a light group renders both.

### Step 18 — The reference renders

* **A set of scenes made for compositing**, as RIB files in `moya/tests/data`. A room is split
  into channels by light path expressions, and another into light groups. Overlapping objects test
  Cryptomatte, and glass and cut-out cards in front of a wall test deep output. A render of few
  samples is denoised.
* **Each is rendered and committed** as the path tracer's references are, with each pixel's
  standard error, and compared within five of them. IDs and the manifest are compared exactly.

**Tests:** the committed renders, and a case that renders each with a light or an expression
changed and checks that the comparison fails.

### Step 19 — Speed

* **`moya_bench` gains a pass of outputs**: the lit room with five light path expressions, two
  light groups, both Cryptomatte layers and a deep output, beside the lit room alone. It reports
  samples a second, and the share of time in each kind of output.
* **The baseline** in `moya/bench/Baseline.md` gains the figures, from the same machine.

**Tests:** the benchmark's short pass times the room with its outputs.

### Step 20 — The documents

* **A new reference page**, `docs/offline/Compositing.md`, for light path expressions, light
  groups, Cryptomatte and deep output. The docs site's summary lists it.
* **[Film.md](../offline/Film.md)** names alpha, the filters of channels and each pixel's IDs.
* **[Output.md](../offline/Output.md)** names RGBA, deep files, Cryptomatte's channels and header,
  and the denoised output.
* **[Integrator.md](../offline/Integrator.md)** names the events of a path and the values at the
  first hit.
* **[Scene.md](../offline/Scene.md)** names a channel's sources, the outputs' formats and a light's
  group.
* **[Rib.md](../offline/Rib.md)** names `DisplayChannel`, a light's group and the outputs' types.
* **[The offline README](../offline/README.md)**'s section on the new core says it writes outputs
  for compositing.
* **The moya manual's [CommandLine.md](../manual/moya/CommandLine.md)** names `--denoise`.
* **[Testing.md](../contributing/Testing.md)** names the output references and the checks that
  channels add up.
* **Each record's `Documented in` line** names the reference page that states its rule.
* **The roadmap's section 6** links here, and says so when this plan closes.

---

## Working beside the editor scripting plan

[EditorScripting](EditorScripting.md) is open. It takes ADR-0217 to ADR-0221, which are below
this plan's records. Its step 16 adds a thread count to `moya/moya.cxx` and
`docs/manual/moya/CommandLine.md`, where steps 7 and 16 change how `--paths` writes and add
`--denoise`. Each lands as a commit of its own. This plan changes nothing the editor sends moya:
the editor renders through the old hiders until section 7, and showing outputs in its render view
is work for section 9.
