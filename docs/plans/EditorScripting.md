# Editor Scripting — Scenes a Script Can Finish

Drafted 2026-11-10 against `48cc20b8`. This plan takes up every section of
[the editor scripting roadmap](../roadmap/EditorScripting.md). When it closes, a script can build
a whole scene and render it, with or without a window. Each item turns in an order of its own.
Meshes that share geometry stay shared when saved. A command places an item by value, and
another sets a key's interpolation. A headless run opens a project, runs a script against it and
quits, with no window or device. A batch renders several frames at once.

The plan runs while [the moya rebuild](../roadmap/MoyaRebuild.md) is in progress. Both change the
editor's rendering code, moya's command line and the scene document, and both commit to one
branch. [Working beside the moya rebuild](#working-beside-the-moya-rebuild) says where the two
meet and the rules that keep them apart.

The plan keeps the roadmap's order, with four changes:

* **Five decisions the roadmap leaves open are steps of their own**, each before the step that
  builds on it. They are the rotation orders, how shared geometry is stored and edited, and the
  space of a placement. The other two are the form of a headless run, and how a parallel batch
  shares the machine. [Decisions](#decisions).
* **Shared geometry reaches moya once.** The roadmap keeps meshes shared in the document. This
  plan also renders a shared mesh as one prototype with several instances, in the RIB export and
  in moya's new scene, so the scene a script builds is the scene moya holds.
  [Step 10](#step-10--shared-geometry-in-renders).
* **The roadmap's check of section 6 against the new core is done.** The old hiders run on one
  thread, so several processes use a machine that one cannot. The new core's CPU device takes
  every thread by default, so several processes would each take every thread. A batch in parallel
  therefore needs moya.exe to take a number of threads.
  [What the survey found](#what-the-survey-found), [step 16](#step-16--a-thread-count-for-moyaexe).
* **Every scriptable command is tested without views.** The roadmap names the rule. This plan
  makes it a test over the command table, so a command added later is held to it.
  [Step 13](#step-13--commands-with-no-views).

## What the survey found

The tree was read at `48cc20b8`.

**Rotations:**

* **Every item turns about x, then y, then z.** `fromEuler()` and `eulerNear()` in
  `src/scene/Properties.cxx` fix the order, and the properties editor, keyed rotation and the
  importer all go through them ([Animation.md](../editor/Animation.md#what-is-keyed)).
* **A rotation is keyed as three angles in degrees**, on the paths `rotation.x`, `.y` and `.z`.
  The item holds a quaternion, and a key takes the angles nearest the curves' values. The keys are
  therefore numbers in the one order, and a change of order has to say what becomes of them.
* **Every kind's payload is at version 1**, and the scene document's schema is at version 5
  (`store::Scene::SCHEMA_VERSION`). [Files.md](../editor/Files.md#scene-documents) states the
  rules for a new version and its migration.
* **The RIB export and moya's new scene take each item's world matrix**, so neither reads the
  order.

**Shared geometry:**

* **Import already shares.** `scene::Importer::mesh()` builds each glTF mesh once, and every node
  that names it holds the same `BRep`.
* **The document does not.** `MeshCodec` writes each item's geometry in full, so a saved import
  reads back as copies.
* **The translator makes one prototype for each mesh item**, named after the item
  (`render::SceneTranslator`). The RIB export writes each mesh in full where it stands.
* **Both RIB handlers read `ObjectBegin` and `ObjectInstance`**: the old hiders' `RIBHandler` and
  the new core's `SceneClient`.

**Commands and scripts:**

* **No command sets a placement by value.** The manipulators write one of translation, rotation
  and scale. `command::same()` in `src/command/Placement.h` compares two placements, and a
  gesture is undone by restoring the whole placement.
* **A script's parameter types** are `Boolean`, `Number`, `Integer`, `Text`, `Item`, `Items` and
  `Parts` ([Scripting.md](../editor/Scripting.md#commands-a-script-can-run)). None holds three
  numbers.
* **A key's interpolation** is `animation::Curve::Interpolation`: step, linear or Bezier. A key
  made in the editor is Bezier. No command sets it.

**Running without a window:**

* **The engine opens a window only for `Feature::Window`**, and `Engine::features()` asks for it
  by default. The editor does not override it.
* **`--script` runs at the end of `start()`**, in the window. A script that stops on a problem
  fails `start()`, so the editor does not open.
* **`app::Context` holds the views, the tools and the render view**, and includes
  `control/RenderView.h`. A command reads what it needs when it runs. `Context::atView()` runs
  its body against the active view.
* **`RenderView` holds the batch**, which the controller builds with the path of moya.exe.
  `render::Batch` itself depends on nothing in `control/`.
* **A batch from a scene with no camera is rendered from the active view.**

**Batches and the new core:**

* **A batch runs moya.exe on one frame at a time**, on a thread of its own, through
  `render::runProgram()`.
* **The old hiders run on one thread.** Nothing in `moya/libmoya` but the device starts a thread.
* **The new core's `CpuDevice` takes a thread count**, and 0 means as many as oneTBB gives.
  `moya.exe` makes its device with the default, and has no option for the count.
* **Each device gives one picture on every run**, whatever its thread count (ADR-0194).

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0217 | Which rotation orders an item takes, and what a change of order does to the item and its keys | [1](#step-1--the-record-rotation-order) |
| 0218 | How a document stores geometry several items share, and what an edit of shared geometry changes | [2](#step-2--the-record-shared-geometry) |
| 0219 | The space a placement command's values are in | [3](#step-3--the-record-the-space-of-a-placement) |
| 0220 | How the editor runs without a window: its program, how it names a project, and when it exits | [4](#step-4--the-record-a-headless-run) |
| 0221 | How a parallel batch shares the machine: processes, threads each, and where the count is kept | [5](#step-5--the-record-a-parallel-batch) |

Each record step researches how established tools make the choice, and the owner chooses among
the options before the record is drafted.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-rotation-order) | The record: rotation order | `docs/adr` | **0217** | accepted |
| [2](#step-2--the-record-shared-geometry) | The record: shared geometry | `docs/adr` | **0218** | accepted |
| [3](#step-3--the-record-the-space-of-a-placement) | The record: the space of a placement | `docs/adr` | **0219** | accepted |
| [4](#step-4--the-record-a-headless-run) | The record: a headless run | `docs/adr` | **0220** | accepted |
| [5](#step-5--the-record-a-parallel-batch) | The record: a parallel batch | `docs/adr` | **0221** | accepted |
| [6](#step-6--rotation-order-in-the-model) | Rotation order in the model | `src/scene`, `src/command` | 0217 | open |
| [7](#step-7--shared-geometry-in-the-model) | Shared geometry in the model | `src/scene`, `src/command` | 0218 | open |
| [8](#step-8--the-scene-document) | The scene document: orders and shared geometry | `src/store` | 0217, 0218 | open |
| [9](#step-9--changing-an-items-order) | Changing an item's order | `src/command`, `src/app`, `src/control` | 0217 | open |
| [10](#step-10--shared-geometry-in-renders) | Shared geometry in renders | `src/render` | 0218 | open |
| [11](#step-11--placing-an-item-by-value) | Placing an item by value | `src/script`, `src/command`, `src/app` | 0219 | open |
| [12](#step-12--a-keys-interpolation) | A key's interpolation | `src/command`, `src/app`, `src/control` | — | open |
| [13](#step-13--commands-with-no-views) | Commands with no views | `src/app` | — | open |
| [14](#step-14--the-batch-out-of-the-render-view) | The batch out of the render view | `src/app`, `src/control`, `src/Controller.*` | — | open |
| [15](#step-15--the-headless-run) | The headless run | `src/Controller.*`, `src/app` | 0220 | open |
| [16](#step-16--a-thread-count-for-moyaexe) | A thread count for moya.exe | `moya/moya.cxx` | 0221 | open |
| [17](#step-17--batches-in-parallel) | Batches in parallel | `src/render`, `src/control` | 0221 | open |
| [18](#step-18--a-script-that-finishes-a-scene) | A script that finishes a scene | `data/scripts/examples`, `tests` | — | open |
| [19](#step-19--the-documents) | The documents | `docs` | — | open |

Paths without a root are under `vertical3d/`.

## What blocks what

```
1 ADR ──> 6 Order in the model ──┐
2 ADR ──> 7 Shared in the model ─┴──> 8 Document ──┬──> 9 Changing the order
                                                    └──> 10 Shared in renders
3 ADR ──> 11 Placing by value (after 6)
12 Interpolation
13 No views ──> 14 Batch out of the view ──> 15 Headless (after 4, 9, 11, 12)
5 ADR ──> 16 moya --threads ──> 17 Parallel batch (after 14)

(8 to 17) ──> 18 Example script ──> 19 Documents
```

Steps 1 to 5 depend on nothing. Steps 6 and 7 each need their record, and step 8 saves what both
add, so the document changes once. Step 9 lets a user set what step 8 saves, and step 10 renders
what step 7 shares. Step 11 gives its rotation in step 6's order. Steps 12 and 13 depend on
nothing. Step 14 needs step 13, because a batch that no view owns runs where there are no views.
Step 15 needs its record and step 14, and comes after steps 9, 11 and 12 so a headless script can
do all a windowed one can. Step 16 needs its record, and step 17 needs step 16 and the batch that
step 14 moves. Step 18 runs everything before it, and step 19 checks that the documents read as
a whole.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze`,
  clang-tidy and every review gate stay clean.
* **The suites.** The editor's suite passes. A step that touches `src/render/` or `moya/` runs
  the offline suite and moya's suites as well. Each step with code adds cases, and `failsfirst.ts`
  is run on the change to show that each new case fails without it.
* **Old documents.** A document of every earlier schema version reads, and its items keep their
  placements, keys and geometry.
* **No picture changes.** Every committed PNG in `moya/tests/data/` stays as it is, and so does
  every scene description the translator's tests commit. The editor's render window shows the
  same picture of every scene.
* **A script and a menu agree.** Each new command is run from a script and from the command
  table, and both give the same scene and the same history.

---

## Working beside the moya rebuild

The rebuild's sections up to output for compositing are closed, the last by
[MoyaCompositing](completed/MoyaCompositing.md), which changed nothing the editor sends moya.
[MoyaMaterials](completed/MoyaMaterials.md) left the editor's surfaces on RI's `matte`. The
retirement of the old hiders (section 7) is open as [MoyaRetirement](MoyaRetirement.md), which
says where it meets this plan. Interactive rendering (section 9) reaches the editor too.
Giving the editor materials is work for a plan of its own.

**Where the two meet:**

| Shared ground | The rebuild changes it in | This plan changes it in |
|---|---|---|
| `src/render/SceneTranslator.*`, `RIBExportVisitor.*`, `tests/render/` | Section 7, and the plan that gives the editor materials | Step 10 |
| `src/render/Batch.*`, `Process.*` | Section 7, which renders a batch through the new core | Steps 14 and 17 |
| `src/control/RenderView.*`, `src/render/MoyaDelegate.*` | Sections 7 and 9 | Step 14 |
| `moya/moya.cxx`, `docs/manual/moya/CommandLine.md` | Nothing open. MoyaPathTracer added `--checkpoint`, `--checkpoint-interval` and `--resume`, MoyaMaterials `--maketexture`, and MoyaCompositing `--denoise` | Step 16 |
| The scene document's schema and kind versions | The plan that gives the editor materials, if it brings them into the document | Step 8 |
| `src/scene/RenderSettings.h` and its codec | Section 7, the hider list, and section 9, the AOVs the render view shows | Nothing |
| `docs/editor/Rendering.md` | Sections 7 and 9 | Steps 14, 17 and 19 |
| ADR numbers | MoyaRetirement, 0236 to 0240 | 0217 to 0221 |

**The rules:**

* **This plan changes nothing in `moya/libmoya` or `api/render/offline`.** Its one change to
  moya is step 16's option in `moya/moya.cxx`, which only passes a count to `CpuDevice`.
* **Step 10 is the only step that changes what the editor sends moya.** It groups shared meshes
  into one prototype or object. A scene with no shared mesh gives the same RIB and the same
  translator description as before, and no picture changes.
* **Nothing this plan adds is kept in the render settings.** A batch's process count and the
  interpolation of new keys belong to the machine and the user, as record 0221 and step 12
  settle. Sections 6 and 7 then change the render settings without this plan in the way.
* **The batch keeps its jobs' form.** A job stays a RIB file and the picture moya.exe writes
  from it. Section 7 changes which core renders the job and what file it writes, and step 17's
  processes and threads hold for either core.
* **This plan takes schema version 6 and version 2 of every kind.** A rebuild plan that brings
  materials into the document starts from them.
* **This plan takes ADR-0217 to ADR-0221.** The rebuild's next plan numbers from 0230. A record
  this plan does not need is listed as a number with no record, as the ADR index requires.
* **Each commit names its paths.** The rebuild's work is often staged while it is in progress,
  so a commit of this plan's work is made with `git commit -- <paths>` and takes nothing else.
* **Step 16's option sits beside the checkpoint options in the same two files.** `moya.cxx`
  refuses those options without `--paths` in `conflict()`, and keeps its option table in
  `optionsOf()`, where step 16 adds its own.

---

### Step 1 — The record: rotation order

**Done.** [ADR-0217](../adr/editor/0217-editor-a-change-of-rotation-order-keeps-the-angles-or-the-pose.md)
gives each item one of the six orders. One command changes it, keeping the angles by default or
the pose when asked, which re-keys every rotation key.

The record answers two questions.

* **Which orders an item takes.** The six orders of three axes are what Maya, USD and Blender's
  Euler modes offer. Blender also offers a quaternion mode and an axis and angle. The record
  says whether the editor offers more than the six, and what the keyed paths are when it does.
* **What a change of order does.** It can keep the item where it is and re-key its rotation in
  the new order, or keep the keys and move the item. Blender converts the current pose and
  leaves the keys. Maya's reorient keeps the orientation. The record says which the editor does,
  and whether a script can ask for the other.

### Step 2 — The record: shared geometry

**Done.** [ADR-0218](../adr/editor/0218-editor-shared-geometry-is-a-record-mesh-items-refer-to.md)
makes geometry a record mesh items refer to by id. An edit changes every item that shares it, a
command gives an item a copy of its own, and a geometry no item refers to is not saved.

* **How the document stores it.** The roadmap names geometry as a record of its own, which mesh
  items refer to by id, as Blender's objects refer to mesh data. Its rivals are a mesh item that
  refers to another mesh item, as Maya's instances share a shape, and copies found equal and
  merged on save. The record says which, and whether a geometry no item refers to is kept.
* **What an edit changes.** A modelling operation on one item of a shared mesh changes the
  geometry every sharing item shows, as Blender's linked duplicates do, or first gives that item
  a copy of its own. The record says which, and whether a command can make an item's geometry
  its own.

### Step 3 — The record: the space of a placement

**Done.** [ADR-0219](../adr/editor/0219-editor-a-placement-is-set-in-the-parents-space-or-the-scenes.md)
takes values in the parent's space by default, or the scene's when asked. A rotation or scale in
the scene's space under a parent that scales unevenly is refused.

The placement command of step 11 sets translation, rotation and scale to values. The record says
whether the values are in the item's parent's space, in the scene's space, or either by an
argument. Blender's `location` is in the parent's space and its `matrix_world` in the scene's.
Maya's `xform` takes `-objectSpace` and `-worldSpace`. The record also says what a value in the
scene's space does to an item under a scaled or sheared parent, which a placement cannot always
hold.

### Step 4 — The record: a headless run

**Done.** [ADR-0220](../adr/editor/0220-editor-a-headless-run-is-a-console-program-of-its-own.md)
makes a headless run a console program of its own over the editor's library. It takes a script
and an optional project, waits for any batch the script started, and fails when the script or a
frame does.

* **The program.** A headless run can be `vertical3d.exe` with a flag, as Blender's `-b` is, or a
  program of its own, as Houdini's `hython` is. The record weighs what each links, and what a
  farm installs.
* **The project.** A headless run can name its project on the command line, or leave the script
  to call `project.load`. The record says which, and whether a run may start with no project.
* **The end.** It says when the run exits: at the end of the script, or when the batch the
  script started has finished as well. It says what a run's exit status is when a frame fails.

### Step 5 — The record: a parallel batch

**Done.** [ADR-0221](../adr/editor/0221-editor-a-batch-runs-frames-at-once-each-on-a-share-of-threads.md)
runs a number of frames at once, each moya.exe on an equal share of the threads. The number is a
user setting a headless run can give, and a GPU takes one process.

* **Processes and threads.** A batch can run several moya.exe processes and give each a share of
  the machine's threads, or run one at a time on every thread. Under the old hiders only the first
  uses the machine. Under the new core both can, and the first also overlaps reading one scene
  with rendering another. Houdini's PDG and Deadline both size a local job by slots and threads.
  The record says how the editor sizes it, and what it does on a GPU, which one process holds.
* **Where the count is kept.** A count belongs to a machine more than to a scene. The record says
  whether it is a user setting or part of the document, and whether a headless run can set it
  from the command line.

### Step 6 — Rotation order in the model

* **Every item holds an order**, from the set record 0217 settles. A new item and every item read
  today take the order the editor uses now, so nothing moves.
* **`fromEuler()` and `eulerNear()` take an order.** The properties editor, keyed rotation,
  auto key and the importer pass each item's own.
* **`eulerNear()` locks per order.** A rotation that locks an order takes the angle that order
  leaves free as 0, as z is taken now.

**Tests:** for each order, angles turned into a quaternion and back give the same angles away
from the lock. A quaternion read at the lock of each order gives a pose that turns into the same
quaternion. An item in each order keyed and evaluated holds its rotation.

### Step 7 — Shared geometry in the model

* **Two mesh items share one `BRep`** as record 0218 says the scene holds it. Import keeps its
  sharing as it does now.
* **A modelling operation on a shared mesh** does what record 0218 settles, and undoes in one step.
* **The outliner and the properties editor show that a mesh is shared**, and with how many items.

**Tests:** an import of a glTF mesh on two nodes gives two items with one geometry. An edit of one
changes what the record says it changes, and an undo restores both.

### Step 8 — The scene document

* **Schema version 6** stores what record 0218 settles, and **version 2 of every kind** stores the
  item's order. A mesh at version 2 refers to its geometry as the record settles.
* **Migration.** A version 5 document reads as it does now. Each item takes the order the editor
  uses now, and each mesh its own geometry. The first write migrates it in the same transaction.
* **A newer document is refused**, as [Files.md](../editor/Files.md#scene-documents) says.

**Tests:** a document of each schema version from 1 to 5 reads, and writes as version 6. An item
of each order saves and reads back in its order. An import with a shared mesh saves and reads
back shared. A version 2 payload with a field it does not define is refused.

### Step 9 — Changing an item's order

* **A command sets the order of items**, as record 0217 says a change acts. It is one change in the
  history, and is scriptable with `items` and the order as text.
* **The properties editor shows the order** as a list beside the rotation, and choosing from it
  runs the command.

**Tests:** a change of order on an item with keys and one without, undone and redone. An order
the editor does not take is refused, and nothing changes.

### Step 10 — Shared geometry in renders

* **The translator makes one prototype for each geometry**, and one instance for each item that
  shows it. A prototype is named after the first item that shows it, by the scene's order.
* **The RIB export writes a shared geometry once**, as an `ObjectBegin` block, and places it with
  `ObjectInstance` for each item.
* **A geometry one item shows** is translated and written as it is now.

**Tests:** a scene with a shared mesh gives one prototype with two instances, and a RIB file with
one object placed twice. The old hiders' picture of it is the same as the picture of the two
copies. Every committed translator description stays as it is.

### Step 11 — Placing an item by value

* **A parameter type of three numbers.** A script writes an array of three finite numbers.
  `script::Parameter` gains the type, and `editor.d.ts` declares it as a tuple.
* **`transform::place`** takes `items`, `translation`, `rotation` and `scale`, each optional, and
  the space record 0219 settles. It sets what it is given as one change through the history.
  A rotation is given as angles in degrees, in each item's own order.
* **An item that cannot take a value is refused**, and nothing is set on any item.
* **With auto key on**, the command keys what it set through `command::Keyed::record()`, as the
  properties editor does.

**Tests:** each value and space, from a script and from the command table. A rotation in each
order sets the angles the properties editor then shows. Auto key keys what was set and nothing
else. A value of two numbers is refused.

### Step 12 — A key's interpolation

* **`timeline::interpolation`** sets step, linear or Bezier on the graph editor's chosen keys,
  or on keys named by `items`, a path and a `frame`. It is one change in the history.
* **New keys take an interpolation of their own**, from a user setting that `timeline::set` and
  auto key both read. The setting is the user's, as Blender's preference is, so the document does
  not change. It starts at Bezier, so a key made today is made the same.
* **The graph editor's key menu** runs the command on its chosen keys.

**Tests:** each interpolation on a chosen key and on a named one, undone and redone. A key made
under each setting takes it. A frame with no key is refused.

### Step 13 — Commands with no views

* **Each command a script can run works with no views**, or refuses with a notice. `Context`
  holds the views as optional, and `Context::atView()` refuses when there is none.
* **A test runs every command in the table** against a context with a document and no views. Each
  must act or refuse, and none may fail. A command added later is in the test without a change to
  it.

**Tests:** the test over the table, and the commands that need a view each refusing.

### Step 14 — The batch out of the render view

* **The batch is held where every command can reach it**, beside the document in `Context`, and
  the render view reads its status from there. `Context` stops including `control/RenderView.h`
  for it.
* **A batch with no scene camera and no view is refused**, since it has nothing to render from.
* **The caption and the render view** show a batch as they do now.

**Tests:** `render::batch` with no views and a scene camera writes its files. With no camera and no
view it refuses.

### Step 15 — The headless run

* **The editor chooses its features from its command line.** A headless run, in the form record
  0220 settles, asks for no window, and the editor builds no views, areas, ui or renderer.
* **The run opens its project, runs its script and exits** as the record settles. A script that
  stops on a problem exits with a failure. A run that started a batch waits for it as the record
  settles.
* **The log goes where it goes now**, and the journal's lines are printed to the standard output,
  so a farm can read what ran.

**Tests:** a headless run in ctest that creates and places items, keys them, saves, and exits 0.
A script that throws exits non-zero. The run opens no window.

### Step 16 — A thread count for moya.exe

* **`moya --threads <count>`** gives the new core's device at most that many threads, as
  `CpuDevice` takes them. With no count it takes as many as oneTBB gives, as now.
* **The old hiders take the option and run on one thread**, as now.
* **A count that is not a whole number above 0 is refused.**

**Tests:** a render on one thread and on four gives the same bytes. A count of 0 or of text is
refused.

### Step 17 — Batches in parallel

* **A batch runs its jobs in several processes at once**, sized as record 0221 settles, each
  given its share of threads with `--threads`.
* **Stop ends every running process**, and starts no other. The status counts frames from every
  process, and the caption shows how many are rendering.
* **A frame that fails fails alone**, as now.

**Tests:** a batch of eight frames in four processes writes eight pictures, the same as a batch in
one. A stop during it leaves no process running. A failing frame is counted once.

### Step 18 — A script that finishes a scene

* **An example script** in `data/scripts/examples/` builds a scene: it imports a mesh used twice,
  places its items, sets one item's order, keys a rotation with linear keys, saves, and starts a
  batch.
* **It runs headless in a test**, with the batch's frames checked for existence and the saved
  document read back.

**Tests:** the script itself, in the suite that runs the example scripts.

### Step 19 — The documents

* **[Scripting.md](../editor/Scripting.md)** names the new parameter type, the new commands,
  the headless run and its exit status, and the rule that every command works without views.
* **[Animation.md](../editor/Animation.md)** states rotation orders, what a change of order does,
  and the interpolation of new keys.
* **[Files.md](../editor/Files.md)** states schema version 6, version 2 of each kind, shared
  geometry and their migrations.
* **[Scene.md](../editor/Scene.md)** states what an edit of shared geometry changes.
* **[Rendering.md](../editor/Rendering.md)** states where the batch is held, parallel batches,
  and how shared geometry is rendered.
* **The editor manual** names the order list, the interpolation menu and the headless command
  line. **The moya manual's [CommandLine.md](../manual/moya/CommandLine.md)** names `--threads`.
* **`data/scripts/editor.d.ts`** is written again from the command table.
* **Each record's `Documented in` line** names the reference page that states its rule.
* **[The roadmap](../roadmap/EditorScripting.md)** moves to `completed/` when this plan closes.
