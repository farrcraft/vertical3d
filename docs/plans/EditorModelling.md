# Editor Modelling — Selection Sets, Component Moves and Topology Operations

Drafted 2026-10-18 against `0bb29eb9`. This plan takes up sections 1, 2, 3 and 5 of
[the editor modelling roadmap](../roadmap/EditorModelling.md), and the camera-view part of its
section 4. Its steps fall in `vertical3d`, `api/brep`, `api/ui` and `api/engine`. When it closes,
the editor warns before it discards work, selects several objects or components at once, moves
vertices, edges and faces, and splits, extrudes, insets and merges faces with undo. Its views
switch between the fixed cameras, and its panes can be resized.

The editor is described in [editor/](../editor/README.md). It edits half-edge meshes
(`brep::BRep`) in a `Scene`, records each completed change as a `Command` on a `CommandStack`,
and draws lines only.

The plan keeps the roadmap's order, with five changes:

* **A step of corrections goes first.** The survey found the roadmap wrong about the tree in
  several places. [Step 1](#step-1--what-the-roadmap-got-wrong).
* **Asking before a quit needs two api changes.** The engine quits without asking the app, and
  `api/ui` has no prompt. [Steps 3 and 4](#step-3--a-prompt-in-the-ui-shell).
* **Shift-click needs Shift back.** Left Shift is the camera's pan modifier, and a held modifier
  sends the press to the camera. [Step 6](#step-6--shift-is-freed-for-selection).
* **Where handles pivot is one record, before either use of it.** Moving a set of objects and
  moving components both move the handles off the object's origin, which ADR-0015 forbids.
  [Step 7](#step-7--the-record-where-the-handles-pivot).
* **The mesh gets its checks before anything changes its topology.** `BRep::validate` cannot
  tell a broken mesh from a sound one. [Step 11](#step-11--brep-checks-and-walks-a-mesh-fully).

Out of this plan, and left open in the roadmap: shading and culling modes, cameras and lights in
the scene, the importers, playback, the Render menu and `create::group`.
[Step 1](#step-1--what-the-roadmap-got-wrong) says why each waits.

## What the survey found

The tree was read at `0bb29eb9`. Line numbers are from that commit.

**History and files:**

* **The stack has no notion of a saved position.** `CommandStack` holds two vectors, done and
  undone (`command/CommandStack.h:74-76`). `push` trims the oldest commands past the capacity
  (`CommandStack.cxx:25-34`).
* **Opening a project clears the history** (`Controller.cxx:405`). Saving touches nothing but the
  file (`Controller.cxx:411-424`).
* **The caption is fixed** at `"Vertical|3D"` (`Controller.cxx:78`).

**Quitting:**

* **The engine registers `ui::quit` itself** and calls `quit()` from it (`api/engine/Engine.cpp:51`).
  A second registration of the same name is refused, so the editor cannot take the command over.
* **A closed window quits directly.** `SDL_EVENT_QUIT` and `SDL_EVENT_WINDOW_CLOSE_REQUESTED` both
  call `quit()` (`Engine.cpp:331-335`). No app is asked first.

**The ui:**

* **`api/ui/shell` has no prompt.** It holds `FileChooser`, `GameMenu`, `Keyboard`, `Screen` and
  `StatisticsOverlay`. `FileChooser` asks before replacing a file by waiting for the same name a
  second time (`FileChooser.h:123-133`). That is a yes or no, and a dirty project needs three
  answers: save, discard, cancel.
* **`FileChooser` is the shape to follow.** It drives a container the app's config lays out, and
  the app routes the container's commands to it.

**Selection:**

* **Left Shift is bound to `view::camera::pan`** in `data/mappings.json`. While any camera modifier
  is held, `Controller::drag` sends the press to the camera tool (`Controller.cxx:493`). A
  shift-click therefore never reaches `SelectTool`.
* **The bindings cannot express a chord**, so "Shift and the left button" cannot be bound as one
  source. A held modifier has to be a command of its own, as the camera moves are.
* **`Scene::selection()` returns the first selected mesh** (`Scene.cxx:54-61`). `SelectTool`
  clears every other mesh when it selects one (`SelectTool.cxx:138-148`), and clears every other
  component of a kind (`SelectTool.cxx:172`).
* **The screen projection a rubber band needs is private to `Picker`** (`Picker::screen`,
  `Picker.h:111`).

**Manipulators:**

* **`TransformTool` drags one mesh** (`dragged_`, `TransformTool.h:128`), and records one
  `TransformCommand` holding one mesh's two placements (`TransformTool.cxx:178-189`).
* **`Manipulator::placement` puts the handles at the mesh's origin** (`Manipulator.h:125`), and
  `apply()` writes the mesh's transform. ADR-0015 says to revisit this "when modelling operations
  arrive and component editing needs handles of its own".

**Meshes:**

* **`BRep` has no operation that changes an existing mesh.** It appends vertices, half edges and
  faces, and nothing removes one.
* **`BRep::validate` checks only that references are in range** (`BRep.cxx:181-201`). It does not
  check that pairs are mutual, that a `next` chain closes, or that every half edge in a loop names
  that loop's face. A topology operation can break all three and still validate.
* **A face stores its normal** (`Face.h:33`), and the project file writes it. Moving a vertex
  leaves the normals of its faces stale.
* **Nothing sets a vertex's `edge`**, and the project file does not store it. There is no way to
  walk the faces around a vertex short of scanning every half edge.
* **`BRep::addEdge(const glm::vec3&)` is declared and never defined** (`BRep.h:74`). TODO.md lists
  it as known debt.
* **A `BRep` cannot be copied**, because `dag::Node` cannot. A record of a mesh's topology cannot be
  a copy of the mesh.

**The menu:**

* **The menu already names three modelling operations**: `transform::poly::extrude_face`,
  `transform::poly::split_edge` and `transform::poly::split_face`. None has a handler.
* **Every fixed view has a camera profile.** `data/cameras.json` holds Front, Back, Left, Right,
  Top, Bottom, Perspective and Perspective2, and the menu's `view::camera::*` items name the first
  seven.
* **The editor draws lines only.** The `view::shade::*`, `view::rendermode::*` and `view::cull::*`
  items need a shaded draw of the mesh, which the renderer does not have.
* **The roadmap's five groups miss four** that the menu carries: `render::view`, `render::batch` and
  `render::settings`; `create::group`; `create::ortho::camera` and
  `create::perspective::camera`; and the camera toolbar's `camera::*` buttons.

**Layout:**

* **A split divides its area evenly** (`ViewLayout.h:26`), and the tree's nodes record no share
  (`ViewLayout.h:96-102`).

---

## Decisions

| ADR | Decides | Step |
|---|---|---|
| 0096 | Where the handles pivot, for a set of objects and for components. Amends 0015 | [7](#step-7--the-record-where-the-handles-pivot) |
| 0097 | How a topology change is recorded for undo | [13](#step-13--the-record-how-a-topology-change-is-undone) |

Each goes in as `proposed` and is accepted when the step that builds it lands.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--what-the-roadmap-got-wrong) | The roadmap and TODO.md corrected | `docs` | — | done |
| [2](#step-2--the-stack-knows-where-it-was-saved) | The stack knows where it was saved, and the caption shows it | `vertical3d` | — | done |
| [3](#step-3--a-prompt-in-the-ui-shell) | A prompt in the ui shell | `api/ui` | — | done |
| [4](#step-4--an-app-can-hold-a-quit) | An app can hold a quit | `api/engine` | — | done |
| [5](#step-5--load-and-quit-ask-first) | Load and quit ask before discarding a dirty project | `vertical3d` | — | done |
| [6](#step-6--shift-is-freed-for-selection) | Shift is freed for selection | `vertical3d` | — | done |
| [7](#step-7--the-record-where-the-handles-pivot) | The record: where the handles pivot | `docs/adr` | **0096** | done |
| [8](#step-8--a-selection-set) | A selection set, and shift-click | `vertical3d` | — | done |
| [9](#step-9--a-rubber-band) | A rubber band | `vertical3d` | 0096 | open |
| [10](#step-10--manipulators-move-a-set) | Manipulators move a set about a shared pivot | `vertical3d` | 0096 | open |
| [11](#step-11--brep-checks-and-walks-a-mesh-fully) | `BRep` checks and walks a mesh fully | `api/brep` | — | open |
| [12](#step-12--components-move) | Components move | `vertical3d`, `api/brep` | 0096 | open |
| [13](#step-13--the-record-how-a-topology-change-is-undone) | The record: how a topology change is undone | `docs/adr` | **0097** | open |
| [14](#step-14--split-edge-and-split-face) | Split edge and split face, through the record 0097 decides | `api/brep`, `vertical3d` | 0097 | open |
| [15](#step-15--extrude-and-inset) | Extrude and inset a face | `api/brep`, `vertical3d` | 0097 | open |
| [16](#step-16--merge) | Merge vertices | `api/brep`, `vertical3d` | 0097 | open |
| [17](#step-17--fixed-views-and-the-projection) | The fixed views and the projection switch | `vertical3d` | — | open |
| [18](#step-18--resizable-panes) | Resizable panes | `vertical3d` | — | open |
| [19](#step-19--the-documents) | The editor documents state the new rules | `docs` | — | open |

## What blocks what

```
1 Corrections
2 Save marker ──┐
3 Prompt ───────┼──> 5 Load and quit ask
4 Quit hold ────┘
6 Shift freed ──> 8 Selection set ──> 9 Rubber band
7 ADR-0096 ─────┴──────────────────> 10 Set moves
11 BRep checks ──> 12 Component moves (after 7, 8)
13 ADR-0097 ──> 14 Split ──> 15 Extrude, inset ──> 16 Merge   (each after 11)
17, 18          (no order)
19 Documents    (last; each step also updates its own section)
```

Step 5 waits on 2, 3 and 4, and comes before any step that adds a new way to change the scene,
because each of those is a new way to lose work. Step 8 needs Shift (6) and the set's pivot (7).
Step 12 needs the full checks (11), because a component move recomputes normals through the walks
it adds. The topology operations run in order of difficulty: a split appends and rewires, an
extrude appends more, and a merge removes and renumbers. Steps 17 and 18 can land at any point.

## How every step is verified

* **The build and the gates.** Everything compiles at `/W4 /WX`, and cpplint, `/analyze` and
  clang-tidy stay clean.
* **The suites.** The `vertical3d`, `brep`, `ui` and `engine` suites pass. Each step with a CPU
  half adds cases, and `failsfirst.ts` is run on the change to show that each new case fails
  without it.
* **A mesh operation** is tested by applying it, checking the result with the strict validation of
  step 11, undoing it, and comparing the mesh with the original index for index. A redo is
  compared with the first result the same way.
* **Drawing and dragging** are checked by running the editor with the validation layer on, and
  reading the log for validation messages.

---

### Step 1 — What the roadmap got wrong

**Done.** The roadmap states each point below, links this plan, and keeps the groups this plan
leaves. [plans/README.md](README.md), the roadmap index and TODO.md's editor section name the plan.

**Documents only.**

* The roadmap's section 2 says shift-click adds to the selection. Left Shift is the pan modifier,
  and a held modifier takes the press. [Step 6](#step-6--shift-is-freed-for-selection).
* The roadmap's section 1 says load and quit ask first. The engine quits without asking the app,
  and the ui has no prompt to ask with. [Steps 3 and 4](#step-3--a-prompt-in-the-ui-shell).
* The roadmap's section 4 says shading and culling modes "are view state". They need a shaded draw
  of the mesh, which the renderer does not have.
* The roadmap's section 4 lists five groups of unhandled commands. The menu carries four more: the
  Render menu, `create::group`, the camera-creation items and the camera toolbar.
* The roadmap's section 3 does not say that `BRep::validate` is too weak to test a topology change
  against, or that a `BRep` cannot be copied. Both shape step 3's ADR.
* The roadmap links this plan, and [plans/README.md](README.md) lists it.
* TODO.md's editor section names this plan.

**What stays in the roadmap**, and why:

* **Shading and culling** need a shaded mesh pass in the editor's renderer. That is a rendering
  piece of its own, and the modelling steps do not need it.
* **Cameras and lights in the scene** need the scene to hold something other than meshes. Nothing
  in this plan does.
* **The importers** each need a format reader. None is needed to model.
* **Playback** waits on a decision about what the editor animates.
* **The Render menu** would hand the scene to moya. The RIB export already does the first half.
* **`create::group`** needs a scene that holds a hierarchy.

**Tests:** none.

### Step 2 — The stack knows where it was saved

**Done.** `CommandStack::markSaved()` and `dirty()` hold the saved position as a count of done
commands, shifted when the stack drops its oldest and lost when a push discards it.
`Controller::syncCaption()` runs after every command. Four cases are in `CommandStackTest.cxx`.
[Commands and undo](../editor/CommandsAndUndo.md#the-saved-position) states the rule.

* **`CommandStack` records the position of the last save.** `markSaved()` records the current
  position. `dirty()` is true at any other position.
* **A save marker trimmed off the far end stays lost.** Once a push has dropped the command the
  marker named, the stack is dirty until the next `markSaved()`.
* **A push after an undo past the marker** discards the redo side that held the saved position, so
  the stack is dirty until the next save.
* **`clear()` marks the empty stack as saved**, because opening a project clears the history.
* **The controller marks the stack** after a successful Save, Save As and Open.
* **The caption shows the project's name and a mark when it is dirty.** The caption is set after
  every command, from the same place `syncUi()` runs.

**Tests**, headless: push and mark, then undo and redo across the marker; a push after an undo past
the marker; the marker trimmed by a full stack; `clear()`.

### Step 3 — A prompt in the ui shell

**Done.** `ui::shell::Prompt` is in `api/ui/shell`, with six cases in `PromptTest.cpp`.
[ShellComponents.md](../api/ui/ShellComponents.md#prompt) states its rules. The prompt does not
hold the keyboard or the pointer: the chooser does not either. Each is a pickable panel, so a
click on it goes to the ui and a click beside it goes to the app. Step 5 decides whether the editor
ignores the tools while a prompt is up.

* **`ui::shell::Prompt` asks one question with up to three answers.** Like `FileChooser`, it drives
  a container the app's config lays out: a label for the question and one button for each answer.
  The app routes the buttons' commands to it.
* **Each answer runs a handler** given when the prompt is opened. Closing the prompt by any path
  other than an answer runs the cancel handler.
* **The prompt closes before a handler runs**, so a handler can open another prompt.

**Tests**, headless in the ui suite: each answer runs its handler and closes the prompt; a config
with no container still opens and answers; opening a second prompt while one is open replaces it
and cancels the first.

### Step 4 — An app can hold a quit

**Done.** The seam is a protected virtual, `Engine::quitRequested()`, beside `start()` and
`release()`. `ui::quit`, `SDL_EVENT_QUIT` and `SDL_EVENT_WINDOW_CLOSE_REQUESTED` all reach it
through `requestQuit()`. Three cases are in `EngineTest.cpp`.
[Lifecycle.md](../api/engine/Lifecycle.md#rules) states the rule.

* **Every path to quitting asks the app first.** `ui::quit`, `SDL_EVENT_QUIT` and
  `SDL_EVENT_WINDOW_CLOSE_REQUESTED` call one engine function, which asks the app whether to quit
  now. The default answers yes, so every other app behaves as it does today.
* **An app that holds a quit calls `Engine::quit()` later**, from the answer it was waiting for.
* **The step decides the seam's shape**: a virtual function on `Engine`, or a handler an app sets.
  Either is a seam on one class, and the engine document states it.

**Tests**, headless in the engine suite: a quit with no override ends the loop; a quit an app holds
does not; the held quit ends the loop when the app calls `quit()`.

### Step 5 — Load and quit ask first

**Done.** `Controller::askBeforeDiscarding()` runs Open's chooser and the quit, and
`Controller::quitRequested()` holds a dirty quit. The prompt is laid out in `data/vgui.json`.
`BindingsTest.cxx` checks that every button of the chooser and the prompt sends a registered
command. Running the editor showed the prompt over the views, the caption marked dirty, and two
close requests held with no validation message. The answer buttons were not clicked: posted mouse
messages do not reach SDL at the posted position, so the answers are covered by the prompt's own
cases. [Files.md](../editor/Files.md#project-files) states the rule.

* **A dirty project asks before Open and before a quit**: save, discard, or cancel.
* **Save** writes the project and then continues. A save that fails stops the open or the quit, and
  the failure is logged.
* **Discard** continues without writing. **Cancel** does nothing.
* **A clean project does not ask.**

**Tests**, headless where the controller allows it: the decision of whether to ask, against a
clean and a dirty stack. The prompt itself is checked by running the editor.

### Step 6 — Shift is freed for selection

**Done.** `view::camera::pan` is bound to the middle mouse button, and holding it is the drag.
Left Shift is bound to `select::add`, which sets `SelectTool::adding()`. Nothing reads it until
step 8. The editor README's binding table states both.

* **The pan modifier moves off Left Shift.** The step picks its new binding. The middle mouse button
  is the likeliest, since no other move uses it.
* **Left Shift becomes `select::add`**, a held command, as the camera moves are. While it is down, a
  click adds to or removes from the selection instead of replacing it.
* **The bindings test still passes**: every binding names a registered command.
* **The editor document's binding table** and `data/mappings.json` change together.

**Tests:** the bindings test. The held state is tested with step 8.

### Step 7 — The record: where the handles pivot

**Done.** ADR-0096 is accepted: the handles pivot at the mean of the selected objects' origins,
or of the distinct vertices a component selection moves. A component selection may span several
objects. ADR-0015 is marked as amended by it. The manipulators document states the rule when
step 10 builds it.

**ADR-0096**, amending ADR-0015.

The question: where are the handles drawn, and about what point do they turn and scale, when the
selection is several objects, or is components inside one or more objects?

* **The candidates** for a set of objects: the centre of the set's bounds, the mean of the
  objects' origins, or the origin of the most recently selected object.
* **The candidates** for components: the centre of the selected components' bounds, their mean, or
  the owning object's origin as today.
* **The constraint ADR-0015 still holds**: a handle is drawn where its pivot is.
* **Whether components in several objects move together** is part of the decision.

**Tests:** none.

### Step 8 — A selection set

**Done.** `Scene::selection()` returns every selected mesh in insertion order, and
`Scene::firstSelected()` keeps the single case for the manipulators until step 10. With
`select::add` held, a click toggles what it hit and a miss changes nothing. A plain click on a
component clears the components of every mesh first. The renderer already drew each mesh by its
own flag, so it did not change. Three cases are in `SelectToolTest.cxx` and `SceneTest.cxx`, and
`create_command_selection_test` covers an undo with two meshes selected.
[Picking](../editor/PickingAndManipulators.md#picking) states the rules.

* **`Scene::selection()` returns every selected mesh**, in scene order. A function for the single
  case is kept only if a caller still needs one after this step.
* **A click with `select::add` held toggles** the object or component under the cursor, and leaves
  the rest of the selection alone.
* **A click without it** replaces the selection, as today.
* **Component masks test every selected mesh**, as today, and a component selection may span
  several meshes if ADR-0096 allows it.
* **Selection is not history**, and stays so. `CreateCommand::undo` stops assuming that at most one
  mesh is selected.
* **The renderer draws every selected mesh as selected.**

**Tests**, headless: add and remove objects; add and remove components; a plain click replaces a
set; a mask change clears components in every mesh; undoing a create with two meshes selected.

### Step 9 — A rubber band

* **A drag that starts on nothing draws a rectangle**, and its release selects everything inside
  it, under the select mask. With `select::add` held it adds to the selection.
* **The test is the picker's screen projection**, made reachable for a rectangle. An object is
  inside when its bound's projected corners are. A vertex is inside when it projects inside. An edge
  is inside when both ends are. A face is inside when every vertex is.
* **A rubber band selects hidden components** as a click does, because picking tests geometry and
  not pixels.
* **The rectangle is drawn in screen space**, in whichever existing pass needs no new one.
* **A drag of a few pixels is a click**, so a shaky click does not become an empty band.

**Tests**, headless: each mask against a known camera and rectangle; the add case; the click
threshold.

### Step 10 — Manipulators move a set

* **The handles sit at the pivot ADR-0096 decides**, for every selected object.
* **A translate moves every object by the same offset.** A rotate or a scale turns each object
  about the shared pivot, which changes each object's translation as well as its rotation or scale.
* **One gesture is one command.** `TransformCommand` holds a placement pair for each mesh it moved.
* **A drag keeps the set it grabbed**, as it keeps one mesh today, so a selection change mid-drag
  cannot split the record from the movement.

**Tests**, headless: a two-object translate, rotate and scale about the pivot, each undone and
redone; a grab and release that moves nothing records nothing.

### Step 11 — `BRep` checks and walks a mesh fully

* **A strict validation** checks what `validate` does not: every pair is mutual, every `next` chain
  closes, every half edge in a loop names that loop's face, and no two half edges of one face run
  between the same pair of vertices. `validate` keeps its present rule for project files, which
  may hold an open chain.
* **Walks around a vertex.** `Topology.h` gains the half edges leaving a vertex and the faces
  around it. They scan the edges, because nothing keeps a vertex's `edge` current, and the step
  says so on the function.
* **A face's normal from its points.** `Topology.h` gains Newell's normal, which is right for a
  face that is not quite planar.
* **`BRep::addEdge(const glm::vec3&)` is deleted**, and its TODO entry with it.

**Tests**, headless in the brep suite: each primitive passes the strict check; a mesh broken in
each of the four ways fails it with a problem naming the part; the walks around a cube's corner;
Newell's normal for a planar and a bent quad.

### Step 12 — Components move

* **In a component mode, the manipulator moves the selected components' vertices**, in the object's
  own space, about the pivot ADR-0096 decides. An edge moves its two vertices and a face its loop.
  A vertex shared by two selected components moves once.
* **The face normals around every moved vertex are recomputed** with step 11's normal.
* **A component move is recorded as a command** that holds the mesh, the moved vertices' indices,
  and their positions before and after the gesture.
* **Undo restores positions and normals exactly**, not by applying the inverse offset.
* **The object's transform is untouched.**

**Tests**, headless: a vertex, an edge and a face moved, each undone and redone and compared exactly;
a shared vertex moved once; normals after a move match Newell's.

### Step 13 — The record: how a topology change is undone

**ADR-0097.** The roadmap names this decision.

The requirement: an undo restores the mesh **index for index**. A later command in the history,
such as a component move, holds indices into the mesh, and its undo runs after the topology undo
has restored the older layout.

* **The candidates**: a snapshot of the mesh's three arrays before and after; the operation and its
  inverse; or a change record of the arrays' lengths and the old and new values of every element
  the operation touched.
* **A snapshot cannot be a copy of the `BRep`**, which cannot be copied. It would be a copy of the
  arrays alone, which `BRep` would have to expose.
* **An inverse has to restore the numbering** as well as the shape. That rules out an inverse built
  from the same append and rewire operations.
* **The decision also says** what a topology command holds, and whether the selection flags inside
  the arrays are part of the record. Selection is not history, so they should not be restored.

**Tests:** none. The ADR is `proposed` until step 14 lands.

### Step 14 — Split edge and split face

* **The topology command** that 0097 decides is built, with split edge as its first user.
* **Split edge** adds a vertex at an edge's middle, and splits both half edges, keeping the pairs
  mutual.
* **Split face** joins two vertices of one face with a new edge, making two faces. The two
  vertices must not be adjacent.
* **`transform::poly::split_edge` and `transform::poly::split_face`** get handlers. Each acts on the
  selection under the matching mask and does nothing, with a log line, on a selection it cannot use.
* **The new parts are selected** after the operation, so it can be repeated.

**Tests**, headless: each operation on a cube's edge and face, with the strict check, undo compared
index for index, and redo compared with the first result; an edge on an open boundary, such as the
plane's; a face split between adjacent vertices refused.

### Step 15 — Extrude and inset

* **Extrude face** duplicates a face's loop, joins the old and new loops with a ring of quads, and
  leaves the new face selected so that the manipulator can pull it out. Several selected faces
  extrude together, and faces that share an edge extrude as one region.
* **Inset face** makes a smaller copy of the face inside it, joined by a ring of quads. The step
  picks how the inset distance is set: a fixed default, or a drag.
* **`transform::poly::extrude_face`** gets a handler, and inset gets a command, a binding and a
  menu item.

**Tests**, headless: each operation on a single face and on two adjacent faces, with the strict
check and the undo and redo comparisons; the cylinder's open boundary.

### Step 16 — Merge

* **Merge vertices** collapses the selected vertices to their centre. Faces that shrink below three
  vertices are removed, and so are the half edges and vertices nothing references.
* **Removal renumbers.** The arrays are compacted, and every index in the mesh is rewritten. The
  undo record of 0097 must restore the old numbering exactly.
* **A merge that would leave a non-manifold edge is refused** with a log line, and changes nothing.
* The merge gets a command, a binding and a menu item.

**Tests**, headless: an edge collapsed on a cube; a face collapsed to a point; a refused merge
leaves the mesh unchanged; every case with the strict check and the undo and redo comparisons.

### Step 17 — Fixed views and the projection

* **`view::camera::front` and the other six fixed views** set the active view's camera to that
  profile. Each profile is already in `data/cameras.json`.
* **The projection switch** toggles the active view between perspective and orthographic.
* **The menu marks the active view's camera**, in `syncUi()`.

**Tests**, headless: each command sets the profile; the bindings test.

### Step 18 — Resizable panes

* **A split records each child's share**, and a missing share means an even one, so
  `data/layout.json` keeps working.
* **Dragging a divider changes the two shares beside it.** A drag that starts within a few pixels
  of a divider takes the press before the tools see it.
* **A share has a floor**, so no pane can be dragged to nothing.
* **The step decides** whether the shares are written back to the layout's config.

**Tests**, headless: a layout with shares divides by them; a missing share is even; a divider drag
changes two shares and respects the floor; `viewAt` finds the divider.

### Step 19 — The documents

* [editor/](../editor/README.md) states the dirty flag, the prompt, the selection set, the rubber
  band, component moves and each topology operation, in the page that owns each.
* The engine document states how an app holds a quit. The ui document states the prompt.
* The roadmap's sections 1, 2, 3 and 5 are marked as done by this plan. Its section 4 keeps what
  step 1 left in it, so the roadmap stays open.
* ADR-0096 and ADR-0097 are `accepted`, and ADR-0015 says `Amended by` 0096.
* TODO.md's editor entries this plan closed are deleted.
