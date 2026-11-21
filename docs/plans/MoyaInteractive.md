# Moya Interactive — Edits While Rendering, and Pictures as They Improve

Drafted 2026-11-20 against `caaef331`. This plan takes up section 9 of
[the moya rebuild roadmap](../roadmap/MoyaRebuild.md). When it closes, a render in the editor keeps
running while the user works. An edit reaches the render between two passes, the render rebuilds
only what the edit touched, and sampling starts again. The render view shows a picture within a
moment of each edit and a better one as samples arrive, denoised if the user asks. A 3D view can
show moya's picture in place of its own shading.

The rebuild's decisions are ADR-0186 to ADR-0194, and the decisions of its plans are ADR-0195 to
ADR-0252, all accepted. This plan follows five of them:

* [ADR-0188](../adr/offline/0188-offline-a-retained-scene-interface-moya-owns.md): the editor is a
  client of moya's retained scene, so an edit is a change to that scene and nothing else.
* [ADR-0203](../adr/offline/0203-offline-scene-changes-are-versioned-and-committed.md): a render
  takes a commit between passes, never during one. Nothing does so yet.
* [ADR-0194](../adr/offline/0194-offline-stateless-samples-one-picture-per-device.md): each device
  gives one picture on every run, so a render that took edits must end on the bytes a fresh render
  of the edited scene gives.
* [ADR-0106](../adr/editor/0106-editor-a-renderer-interface-for-interactive-a-process-for-batch.md):
  the editor drives an interactive render through `Delegate`, and a batch through moya.exe. The
  batch is not changed.
* [ADR-0154](../adr/editor/0154-editor-the-render-view-is-an-editor-in-an-area-or-a-window.md): the
  render view is an editor in an area or a window.

The plan keeps the roadmap's order, with five changes:

* **Four decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are how an edit reaches a running render, how the first pictures are made,
  when a picture is denoised, and how a 3D view shows a render. [Decisions](#decisions).
* **A pass stops between tiles.** A cancel waits for the pass in progress, and at the editor's
  default of 2 by 2 samples a render is one pass, so a cancel waits for the whole render. An edit
  cannot reach a render faster than its pass ends. [Step 5](#step-5--a-pass-that-stops-between-tiles).
* **Every structure a render builds is kept across versions**, not only the hierarchies. The
  roadmap names rebuilding the touched structures, and only the hierarchies can do so today.
  [Steps 6](#step-6--materials-textures-and-volumes-kept-across-versions) and
  [7](#step-7--lights-kept-across-versions).
* **Denoised pictures while sampling continues.** moya denoises through OIDN when a render ends.
  Every production renderer's interactive view denoises as it goes, and the roadmap does not name
  it. [Step 10](#step-10--denoised-pictures).
* **A rendered view.** `Delegate`'s own comment names a live render in a viewport as its purpose,
  and the roadmap names only the render view. A 3D view gains a third shading, moya's picture.
  [Step 13](#step-13--a-rendered-view).

## What the survey found

The tree was read at `caaef331`.

**The editor's render:**

* **`Delegate::update()` compares a signature of the scene each tick.** On a difference it cancels
  the running render and starts another. The signature is each item's id, revision and parent, the
  evaluated values of the current frame, the camera and the settings
  ([Rendering.md](../editor/Rendering.md#the-two-ways-in)).
* **`MoyaDelegate::start()` brings the retained scene in step on the caller's thread**, through the
  `SceneTranslator`, applies it, and starts a thread that calls `integrate::render()`. The scene is
  never changed while a render reads it, because a running render is cancelled first.
* **A cancel waits for the pass in progress.** A pass adds up to `kSamplesPerPass`, 16 samples, to
  every pixel. The editor's default is 2 by 2 samples, so its render is one pass.
* **A picture arrives when the render finishes.** `picture()` gives the last finished render's
  picture, and a stopped render's samples are dropped. TODO.md holds this as "A render shows
  nothing until it finishes".
* **The translator writes edits and commits them, and reads only the schema.** The scene takes
  edits from any thread and holds them until its owner applies them
  ([Scene.md](../offline/Scene.md#edits-commits-and-versions)). The translator could therefore edit
  while a render runs, and only the delegate's cancel keeps it from doing so.
* **`RenderView::tick()` calls `update()` each tick while any area shows a render view**, and
  uploads a picture when the count of pictures moves.
* **A 3D view's shading is `Flat` or `Smooth`** (`src/view/Shading.h`). No view shows moya's
  picture.

**moya's render:**

* **`integrate::render()` builds every structure from nothing on each call.** It builds the
  hierarchies, the tracing view, the surfaces and their compiled materials, the volumes, the
  textures, and the lights and their tree. It makes the filter, the film, and the records of first
  hits, light paths, coverage and deep samples. Nothing is kept from one call to the next.
* **`accel::Assembler` keeps the hierarchies across versions**, through a `geometry::Cache` that
  places each unchanged geometry as it was made. Only tests and the benchmark use it.
* **No render calls `apply()`.** moya.exe applies the scene once, before it renders.
* **The film can be read while it samples.** `snapshot()` can be called from any thread, and waits
  for a pass in progress to end ([Film.md](../offline/Film.md#passes-and-snapshots)).
* **The picture is the same whatever the passes hold**, and on any thread count (`Render.h`).
* **A launch runs in tiles** of 32 by 32 pixels, in any order on oneTBB's threads, and has no way
  to stop part way ([Devices.md](../offline/Devices.md)).
* **`Checkpoints::stop` is asked before each pass**, and is how the delegate cancels.
* **Denoising** is `denoise::denoise()` over a finished snapshot, from the beauty, albedo and
  normal through OIDN ([Output.md](../offline/Output.md#denoised-channels)).

**The tests:** moya's suite holds the path tracer's, compositing's and volumes' reference renders,
each a RIB file in `moya/tests/data/` with its committed EXR. `AssemblerTest.cxx` checks that an
updated assembly agrees with one assembled from nothing. The editor's suite renders through
`MoyaDelegate` and compares it with moya.exe's render of the RIB export. `moya_bench` has no
measure of how long a render takes to build its structures, or of the time from an edit to a
picture.

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0253 | How an edit reaches a running render, what it rebuilds, and how a render that takes edits relates to `render()` | [1](#step-1--the-record-an-edit-in-a-running-render) |
| 0254 | How an interactive render makes its first pictures, and how often it makes one | [2](#step-2--the-record-the-first-pictures) |
| 0255 | When a picture is denoised while sampling continues | [3](#step-3--the-record-denoising-while-sampling) |
| 0256 | How a 3D view shows moya's picture and draws its overlays on it | [4](#step-4--the-record-a-rendered-view) |

Each record step researches how production renderers make the choice, and the owner chooses
among the options before the record is drafted. A record that a later step shows is not needed is
listed as a number with no record, as the ADR index requires.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-an-edit-in-a-running-render) | The record: an edit in a running render | `docs/adr` | **0253** | accepted |
| [2](#step-2--the-record-the-first-pictures) | The record: the first pictures | `docs/adr` | **0254** | accepted |
| [3](#step-3--the-record-denoising-while-sampling) | The record: denoising while sampling | `docs/adr` | **0255** | accepted |
| [4](#step-4--the-record-a-rendered-view) | The record: a rendered view | `docs/adr` | **0256** | accepted |
| [5](#step-5--a-pass-that-stops-between-tiles) | A pass that stops between tiles | `moya/libmoya/device`, `film` | 0194 | done |
| [6](#step-6--materials-textures-and-volumes-kept-across-versions) | Materials, textures and volumes kept across versions | `moya/libmoya/geometry`, `texture`, `volume` | 0203 | done |
| [7](#step-7--lights-kept-across-versions) | Lights kept across versions | `moya/libmoya/light` | 0203, 0214 | open |
| [8](#step-8--the-session) | The session | `moya/libmoya/integrate` | 0253 | open |
| [9](#step-9--pictures-while-sampling-continues) | Pictures while sampling continues | `moya/libmoya/integrate`, `film` | 0254 | open |
| [10](#step-10--denoised-pictures) | Denoised pictures | `moya/libmoya/denoise`, `integrate` | 0255 | open |
| [11](#step-11--the-editors-delegate-on-a-session) | The editor's delegate on a session | `vertical3d/src/render` | 0106, 0253 | open |
| [12](#step-12--the-render-view-as-it-improves) | The render view as it improves | `vertical3d/src/control` | 0154 | open |
| [13](#step-13--a-rendered-view) | A rendered view | `vertical3d/src/view`, `src/render` | 0256 | open |
| [14](#step-14--speed) | Speed: the time from an edit to a picture | `moya/bench` | — | open |
| [15](#step-15--the-documents) | The documents | `docs` | — | open |

## What blocks what

```
5 Stop between tiles ─────────┐
6 Materials, textures ────────┤
7 Lights ─────────────────────┤
1 ADR ────────────────────────┴──> 8 Session ──> 9 Pictures (after 2) ──> 10 Denoised (after 3)
                                                      │
                                                      └──> 11 Delegate ──> 12 Render view (after 10)
                                                                       └──> 13 Rendered view (after 4, 10)

(8 to 13) ──> 14 Speed ──> 15 Documents
```

Steps 1 to 4 depend on nothing but each other. Steps 5, 6 and 7 depend on nothing, and can run
beside the records. Step 8 needs its record and the three steps before it, since a session is
those structures kept together. Step 9 needs the session and the record of step 2, and step 10
needs step 9 and the record of step 3. Step 11 puts the editor on a session that makes pictures
as it goes. Step 12 shows them, and needs step 10 for the denoised channel. Step 13 needs the
delegate, the denoised pictures and the record of step 4. Step 14 measures everything before it,
and step 15 checks that the documents read as a whole. Each step updates its own section of the
reference documents as it lands.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean. Kernel code passes `kernel.ts`.
* **The suites.** The offline suite, moya's suite and the editor's suite pass. Each step with code
  adds cases, and `failsfirst.ts` is run on the change to show that each new case fails without
  it.
* **An edited render ends where a fresh one does.** A session that takes an edit, and samples to
  its end, gives the same bytes in every channel as a render of the edited scene from nothing.
  This is the plan's oracle, and ADR-0194 makes it exact. It is held for each kind of
  edit: a camera, an instance's transformation, a geometry, a material, a light, an option and an
  output, and an object created and one removed.
* **A kept structure agrees with a built one.** Each structure step 6 and step 7 keep across
  versions is compared, after each kind of edit, with the same structure built from nothing, as
  `AssemblerTest.cxx` compares assemblies.
* **One picture on every run.** A render on one thread and on every thread gives the same bytes,
  with and without a stop part way through a pass.
* **No picture changes.** `render()` and moya.exe go through the session once step 8 lands, and
  every committed reference render in `moya/tests/data/` gives the same bytes as before. The
  editor's render of a scene still matches moya.exe's render of its RIB export.
* **Linux.** portable.yml's clang job builds `v3dlib_moya` and runs moya's suite under the
  sanitizers, in the spectral and the RGB build. The thread sanitizer job runs the session's cases,
  since a session is edited on one thread and read on another.

---

### Step 1 — The record: an edit in a running render

**The question.** How an edit reaches a render that is running, what it rebuilds, and how a render
that takes edits relates to `render()`, which moya.exe and the tests call. The answer decides the
shape of moya's interactive interface, which the editor uses now and a Hydra delegate uses in
section 12.

**The options:**

* **A session that lives across edits.** moya keeps a render's structures from one version of the
  scene to the next. Between passes it applies the batches committed since the last, asks the
  scene what changed, rebuilds what the changes touched, and starts the film again. Cycles'
  `Session` resets on a scene update this way, RenderMan's Riley takes edits while it renders,
  and Karma and Arnold's interactive renders restart sampling after an edit. `render()` becomes a
  session that takes no edits and runs to its end.
* **A render started again, with structures the caller keeps.** `render()` keeps its shape and
  takes an object of kept structures. The caller cancels and starts a new render for each edit,
  as the delegate does now, and only the build is saved.
* **A render started again from nothing**, as now, made faster by building each structure in
  parallel. Nothing is kept, so nothing can be kept wrongly.

The record also settles which thread a session runs on and who owns it, and whether an edit that
changes nothing a picture shows restarts the film. It settles what a stop and a resume mean for a
session, and whether a session that took an edit writes a checkpoint. It settles whether the
film is kept at its resolution and channels when neither changed, and what is released when a
session ends.

**Drafted** as
[ADR-0253](../adr/offline/0253-offline-a-render-is-a-session-that-takes-edits-between-passes.md): a
render is a session that keeps every structure it builds across versions. Between two passes it
applies the batches committed since the last, rebuilds only what they changed, and starts the film
again unless nothing a picture shows was changed. `render()` is a session that takes no edits and
runs to its end. The session runs its passes on a thread of its own, and its caller owns it. The
film keeps its storage while its resolution and channels are unchanged. A checkpoint describes the
version the session is rendering, counted from the edit that last started the film, so a resume
ends on the bytes of a fresh render of that version. Ending a session releases every structure it
holds.

### Step 2 — The record: the first pictures

**The question.** How soon after an edit the first picture comes, what it holds, and how often a
picture is made after it.

**The options:**

* **Coarse pixels first.** The first passes render at a fraction of the resolution, each pixel
  standing for a block, and the full resolution follows. Cycles' viewport starts at a resolution
  divided by a power of two, and Arnold's progressive render refines from coarse samples upward.
  The first picture comes in a fraction of a pass's time, and is blocky.
* **Full resolution, in passes that grow.** The first pass takes one sample a pixel, and each pass
  after it takes more, up to `kSamplesPerPass`. Karma XPU and Cycles' render window work this way.
  The first picture is noisy and sharp.
* **Fixed passes, with pictures by time.** Passes stay at `kSamplesPerPass`, and a picture is made
  as each ends, as now but before the render finishes.

The record also settles whether a coarse pass's samples count toward the film, so that the final
bytes stay those of a render that never stopped. It settles the rate pictures are made at and who
sets it. It settles which thread turns a snapshot into a displayed picture, and how long a stop or
an edit can wait.

**Drafted** as
[ADR-0254](../adr/offline/0254-offline-first-pictures-are-coarse-then-passes-grow.md): after an
edit a session renders coarse pictures first, halving the block each pixel stands for until it
reaches full resolution, and the coarse samples are never added to the film. The full-resolution
passes then grow from one sample a pixel, doubling up to `kSamplesPerPass`. The caller sets the
least time between two pictures, and a session makes one after a pass once that time has passed.
The session's own thread turns the snapshot into the displayed picture, and a stop or an edit waits
for one tile on each thread.

### Step 3 — The record: denoising while sampling

**The question.** When a picture is denoised while its render continues, and at what cost to the
render.

**The options:**

* **Each picture denoised once the pixels hold enough samples.** Cycles' viewport denoising starts
  at a set sample and denoises every picture after it. OIDN's fast quality keeps it brief.
* **Denoised when sampling ends or pauses**, so a render that is still improving shows its noise,
  and one that has settled shows the denoised result.
* **Denoised when the caller asks**, picture by picture, so the render view decides and moya holds
  no rule.

The record also settles whether albedo and normal are cleaned before the beauty is denoised, as
OIDN's `cleanAux` does. It settles OIDN's quality for an interactive picture, and whether
denoising runs between passes or on a thread of its own while the next pass samples.

**Drafted** as
[ADR-0255](../adr/offline/0255-offline-each-picture-is-denoised-once-it-holds-enough-samples.md): a
session whose caller turns denoising on denoises every picture whose pixels hold at least a start
number of samples, which the caller sets, and gives it beside the raw picture. An interactive
picture is denoised at OIDN's fast quality from the albedo and normal as they are. The picture that
ends a render is denoised as a batch render's is, with the albedo and normal filtered first.
Denoising runs between passes on the session's thread.

### Step 4 — The record: a rendered view

**The question.** How a 3D view shows moya's picture in place of its own shading, and how the grid,
the manipulators and the selection outlines are drawn on it.

**The options:**

* **A session for each rendered view**, at the view's size and from the view's camera. The
  overlays are drawn over the picture against the depth of the scene rasterised as the view
  draws it now, so a manipulator behind an object is hidden. Blender's rendered shading works this
  way.
* **One session, shared by the render view and the active view.** The render view shows what the
  active view shows. Only one render runs, and the render view loses its own camera.
* **Overlays drawn on top**, with no depth, over a picture from a session of the view's.

The record also settles the resolution of a rendered view's picture against the view's own. It
settles what a rendered view that is not the active one does, and how many sessions can run at
once on one machine's threads.

**Drafted** as
[ADR-0256](../adr/editor/0256-editor-a-rendered-view-has-its-own-session-and-raster-depth.md): each
3D view in rendered shading has a session of its own, at the view's size and from the view's
camera, and a view that is not shown pauses its session. The view draws the scene's depth as it
rasterises it now, and the overlays are tested against that depth and drawn over moya's picture.
A rendered view's picture is the view's size divided by a pixel size, which the view's state holds.
Every session runs on oneTBB's one pool of threads, so a rendered view that is shown, whether it is
active or not, renders beside the others.

### Step 5 — A pass that stops between tiles

* **A launch can stop.** `CpuDevice::launch()` takes a stop that each tile reads before it starts,
  and a launch that stopped returns once its running tiles end.
* **A stopped pass is not in the film.** Its samples are dropped, so the film holds the passes
  before it and a snapshot shows them.
* **`Checkpoints::stop` is also asked during a pass**, so a cancel waits for one tile on each
  thread, not for the pass.

**Tests:** a launch stopped at its start runs no tile. A pass stopped part way leaves the film's
bytes as they stood before it. A render stopped part way through a pass, and resumed from its
checkpoint, ends on the bytes of one that never stopped.

**Done.** `CpuDevice::launch()` takes a stop asked before each tile, and says whether every pixel
ran. `Film::attempt()` runs a pass that may stop, and puts the film's floats back when it does. A
render asks `Checkpoints::stop` before each tile as well as each pass, one thread at a time, and a
render with no stop keeps no copy of the film. A counted render's tallies and a render's trails
keep what a dropped pass's tiles added.

### Step 6 — Materials, textures and volumes kept across versions

* **`geometry::Surfaces` keeps each material's compiled program**, and compiles again only the
  materials whose object, or one of whose nodes, changed.
* **`texture::Textures` keeps its cache across versions.** A texture no material reads any more is
  released, and its tiles leave the cache.
* **`volume::Volumes` keeps each grid's device layout and majorants**, through the grids the
  geometry cache keeps, and builds again only the volumes whose fields changed.
* **Each keeps the version it last built from**, and asks the scene for what changed since, as
  `accel::Assembler` does.

**Tests:** after an edit to one material, one texture's file name and one volume's field, each
kept structure agrees with one built from nothing, and the unchanged materials and grids are not
built again.

**Done.** A material is compiled in the gather, not in `Surfaces`, so `geometry::Cache` keeps each
material's programs, and the assembler's forgetting of changed objects drives them. A material is
compiled again when its object, its network's nodes or connections, or the shader search path
changed, or when the primvars read before it differ. `Surfaces` still uploads its buffers on each
build. `Textures::update()` keeps the textures read before and closes the rest, and
`Volumes::update()` keeps its blocks while its grids are the same. A grid's majorants are worked
out in kernel code from its nodes, so a kept grid keeps them. A kept texture is read from its file
as it was opened.

### Step 7 — Lights kept across versions

* **`light::Lights` keeps its lights and the light tree across versions.** A light whose object or
  instance changed is made again. The light tree is built again when a light was made, moved or
  removed, or when an emitting geometry or its instance changed.
* **A camera's edit, an option's that does not touch lights, and a material's that does not emit
  rebuild nothing of the lights.**

**Tests:** after each kind of edit, the kept lights agree with lights built from nothing, and the
tree is rebuilt only for the edits that touch it.

**Done.** `Lights::update()` makes a light again only when its object changed, and every light
when the shutter changed. It places the lights and builds the tree again when a light was made,
created or removed, when the reach or shutter changed, or when the bytes recorded for the emitting
geometries differ. While the scene has a volume or a linked light, any edit but a camera's, an
option's, an output's or a channel's places them again; that is coarser than the step asks.
Images and IES profiles are read once and held for the life of the lights. Every update still
converts the materials.

### Step 8 — The session

Under the shape step 1 settles.

* **`integrate::Session` holds a render's structures**: the assembler, the tracing view, the
  surfaces, the volumes, the textures, the lights, the film and the records of the outputs.
* **Between passes, it applies what was committed**, rebuilds what the changes touched and starts
  the film again, as the record settles. A pass never sees half a batch.
* **`render()` is a session run to its end**, so moya.exe, the tests and the editor render through
  one path.
* **`Scene::forget()` is called** once the session has built past a version, so the scene's record
  of removals does not grow for the length of an editing session.

**Tests:** for each kind of edit the oracle names, a session edited after its first pass ends on
the bytes of a render of the edited scene from nothing. A session edited many times before a pass
ends takes every batch. A commit made during a pass is not in that pass's samples. Every
reference render gives the same bytes as before.

### Step 9 — Pictures while sampling continues

Under the schedule step 2 settles.

* **A session makes pictures as it samples**, at the rate its caller sets, from the film's
  snapshot through the display and view the caller names.
* **The first passes after an edit** are made as the record settles, so the first picture comes
  within a short time of the edit.
* **A picture says what it holds**: the samples its pixels hold, and the version of the scene it
  shows.

**Tests:** a session gives a picture before its last pass, and each picture holds more samples than
the last. A picture after an edit names the new version. The final picture's bytes are those of a
render that made no pictures.

### Step 10 — Denoised pictures

Under the rule step 3 settles.

* **A session can denoise its pictures** through `denoise::denoise()`, from the beauty, albedo and
  normal, at the quality the record settles.
* **A denoised picture is a second picture**, beside the raw one, so the raw picture's bytes do
  not depend on whether denoising runs.

**Tests:** a denoised picture of a noisy scene is closer to a converged reference than the raw
picture at the same samples. Turning denoising on changes no byte of the raw picture.

### Step 11 — The editor's delegate on a session

* **`MoyaDelegate` keeps one session** for as long as the editor renders. `update()` hands the
  translator's edits to it and commits them, and no longer cancels and starts again.
* **`picture()` gives the latest picture**, not the last finished one, and `status()` gives the
  samples it holds beside the state and how much is done.
* **A stop keeps the latest picture.** The render view shows what was sampled before the stop.
* **`Delegate`'s contract says an edit reaches a running render.** ADR-0106's decision is unchanged.
  The rule that a render reads the scene when it starts is removed from the header and from
  [Rendering.md](../editor/Rendering.md).

**Tests:** an edit to an item while a render runs reaches its picture, without a cancel. The
delegate's final picture of an edited scene matches moya.exe's render of the scene's RIB export
after the edit. A stop during a render keeps a picture. TODO.md's "A render shows nothing until it
finishes" is closed by this step, and its entry deleted.

### Step 12 — The render view as it improves

* **The render view shows each picture as it comes**, with the samples it holds in the status row.
* **The render view sets the rate** of pictures it is given, from how often it can show one.
* **The render view chooses a channel**: the beauty, the denoised beauty, the albedo or the normal.
  The choice is the render view's own state, kept with the workspace as an editor's state is, and
  is not a render setting.

**Tests:** a render view's picture count moves before its render ends. Choosing a channel shows
that channel's picture. A workspace saved with the denoised channel chosen reads back with it.

### Step 13 — A rendered view

Under the shape step 4 settles.

* **A 3D view's shading gains `Rendered`**, beside `Flat` and `Smooth`, with its name in the view's
  saved state.
* **A rendered view shows moya's picture from its own camera**, at the resolution the record
  settles, and an orbit, a pan or a zoom is an edit of that camera.
* **The overlays are drawn on the picture** as the record settles.

**Tests:** a view switched to `Rendered` shows a picture from moya, and switched back shows its
own shading. Orbiting a rendered view makes a new picture from the new camera. A manipulator behind
an object is hidden in a rendered view as it is in a shaded one, if the record chooses depth.

### Step 14 — Speed

* **`moya_bench` gains an interactive pass** over the lit room. It times the build of each
  structure from nothing. It times the first picture after a camera move, an instance move, a
  material change and a light change.
* **The baseline** in `moya/bench/Baseline.md` gains the figures, from the same machine.

**Tests:** the benchmark's short pass times each edit, and checks that a camera move rebuilds no
structure but the film.

### Step 15 — The documents

* **A new reference page**, `docs/offline/Interactive.md`, for the session, edits between passes,
  the first pictures and denoising while sampling. The docs site's summary lists it.
* **[Integrator.md](../offline/Integrator.md)** names the session and a pass that stops.
* **[Film.md](../offline/Film.md)** names a stopped pass and pictures made while sampling.
* **[Scene.md](../offline/Scene.md)** names the session as the owner that applies, and when it
  forgets.
* **[Devices.md](../offline/Devices.md)** names a launch that stops.
* **[Rendering.md](../editor/Rendering.md)** names edits reaching a running render, the render
  view's pictures and channels, and the rendered view.
* **The editor's user manual**, [Rendering.md](../manual/editor/Rendering.md) and
  [Views.md](../manual/editor/Views.md), names the render view's pictures and channel and the
  rendered shading.
* **[The offline README](../offline/README.md)** lists the new page.
* **Each record's `Documented in` line** names the reference page that states its rule.
* **The roadmap's section 9** links here, and says so when this plan closes.

---

## Working beside the editor scripting plan

[EditorScripting](EditorScripting.md) is open. It takes ADR-0217 to ADR-0221, and this plan takes
ADR-0253 to ADR-0256. The two meet in the editor's render code:

| Shared ground | This plan changes it in | EditorScripting changes it in |
|---|---|---|
| `src/render/MoyaDelegate.*`, `Delegate.*` | Step 11 | Nothing. Its step 14 moves the batch, which this plan does not touch |
| `src/control/RenderView.*` | Step 12 | Step 14, which moves the batch out of the render view |
| `src/render/SceneTranslator.*` | Nothing. The translator already sets only what changed | Step 10, which shares geometry in renders |
| `src/view/Shading.*` and a view's saved state | Step 13 | Nothing |
| `src/scene/RenderSettings.h` and its codec | Nothing. The render view's channel is its own state | Nothing |
| `docs/editor/Rendering.md` | Steps 11, 12, 13 and 15 | Steps 14, 17 and 19 |

EditorScripting's step 15 runs the editor with no window, and so with no renderer. A session is
made by the delegate, so a headless run makes none. Each commit names its paths, with
`git commit -- <paths>`, so neither plan's staged work lands in the other's commit.
