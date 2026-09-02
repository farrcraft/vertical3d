# Rigel Survey

Phase 6 of [plans/Modernization.md](plans/Modernization.md). Surveyed against the tree on
2026-09-01.

`rigel/` is the earlier prototype of the Vertical3D editor, and the plan has always said the
rewrite has to absorb it before it can go. This is the record of what is actually in there,
what has already landed in `api/`, what has not, and what the fold-in costs — so phase 6 can
be scoped rather than guessed at.

**Verdict up front: nothing in rigel can be ported, and one file in it is worth more than the
other fifty-three.** It is a gtkmm 2 / gtkglextmm / libxml++ application. Every line of
drawing is immediate-mode OpenGL and every line of windowing is GTK, so
[ADR-0001](adr/0001-vulkan-replaces-opengl.md) and SDL3 between them take all of it. What
survives is behaviour, six or seven algorithms, and `docs/xml/gui.xml` — the editor's whole
menu, toolbar, keybinding, camera-profile and viewport-layout definition, which is a data
asset and needs translating, not rewriting.

The fold-in is also smaller than the plan implies, because more of it has already happened
than the plan records — and larger in one place nobody has costed, because the api has no way
to draw a line.

## Method

`rigel/` is 9,026 lines across 53 files, plus `docs/xml/gui.xml` (205 lines), four toolbar
icons and eight screenshots. All 53 were read, and `gui.xml` with them.

| Subtree | Files | Lines |
|---|---|---|
| `vertical3d/` | 13 | 3,337 |
| `libv3dcore/brep/` | 12 | 1,755 |
| `vertical3d/commands/` | 10 | 1,481 |
| `libv3dcore/` | 8 | 1,279 |
| `vertical3d/manipulators/` | 8 | 1,068 |
| `libv3dcommand/` | 2 | 106 |

Everything rigel duplicates was diffed against its `api/` counterpart — `api/type`,
`api/brep`, `api/dag`, `api/event` — and against `v3dlibs/core`, so that "already migrated"
is a claim about behaviour rather than about a class of the same name existing.

**Nothing was run, and nothing could be.** There is no `CMakeLists.txt` anywhere in `rigel/`,
and six of its include roots — `libv3dtypes/`, `libv3dgraph/`, `libv3dcommand/Command.h`,
`libv3dcommand/CommandDirectory.h`, `libv3dcore/log/logstream.h`, `libmoya/` — do not exist
in the repository. `libv3dcommand/` still stands as a directory but holds only `Tool.{h,cxx}`;
the `Command` and `CommandDirectory` that every command set in rigel inherits from and calls
went to `v3dlibs/command` and from there to `api/event`. The last commit to touch the tree is
`0650e7d`, "move rigel/libv3dgraph to v3dlibs/dag", 2022-07-22 — the commit that took the
last of rigel's own foundations out from under it. Nothing outside `rigel/` references it.

Treat the tree as a reference text, the way [LuxaAudit.md](LuxaAudit.md) treats `luxa/`.

## What rigel is

A GTK desktop modeller: a quad-split viewport layout over one scene, orthographic and
perspective cameras driven by an arcball, a construction-plane grid, click-to-select down to
faces, edges and vertices, three transform manipulators, four primitive creators, three poly
tools, and an XML project file.

| Piece | Files | What it does |
|---|---|---|
| `Window` | `vertical3d/Window.{h,cxx}` | 783 lines. `Gtk::Window` singleton. Builds menus, toolbars, keybindings, camera profiles and the view layout out of `gui.xml`; owns the active viewport, the active tool mode, the select mask, and the one live manipulator. |
| `ViewLayout` | `vertical3d/ViewLayout.{h,cxx}` | Recursively builds a tree of `Gtk::Paned` from nested `<viewgroup>` elements, one `ViewPort` per `<viewport>`. `pop()` maximises the active view and restores it. Ten layout types are declared and none is used — the tree in the XML decides the layout. |
| `ViewPort` | `vertical3d/ViewPort.{h,cxx}` | 1,381 lines, the largest file. A GL drawing area: camera modes (zoom/truck/pan), the draw of scene and decorations, `GL_SELECT` hit testing, unproject/project, per-face/edge/vertex handles, and the manipulator draw. |
| `ConstructionPlane` | `vertical3d/ConstructionPlane.{h,cxx}` | A 60x60 line grid with major intervals and a thicker origin line, oriented to the camera's own axes under an orthographic camera. |
| `ArcBall` | `vertical3d/ArcBall.{h,cxx}` | NeHe lesson 48 arcball. Click, drag to a quaternion, sphere mapping. |
| `RenderView` | `vertical3d/RenderView.{h,cxx}` | 37 lines. A window containing an empty 640x480 `Gtk::DrawingArea`. Nothing ever draws into it. |
| `commands/` | 10 files | Five command sets registering 51 command strings with `CommandDirectory`: viewport render/cull/shade/show/camera modes, transform tool modes and select masks, project load/save, four poly primitives, three poly tools. |
| `manipulators/` | 8 files | A base holding an axis constraint and a coordinate space, plus translate (line and cone per axis), rotate (a circle per axis) and scale (line and cube per axis). Each draws itself and turns a mouse delta into a transform on the selection. |
| `libv3dcore/` | 8 files | `Project` (singleton: scenes, selection, active tool), `Scene` (named list of meshes), `Camera` and `CameraProfile`. |
| `libv3dcore/brep/` | 12 files | Half-edge and winged-edge boundary representations, with `Vertex`, `Face`, `HalfEdge` and `Edge`. `HalfEdgeBRep` is typedef'd `Mesh` and is what the editor models. |
| `libv3dcommand/` | 2 files | `Tool`: a `Command` that also takes `motion`, `button` and `draw`. The interactive half of the command model. |

`docs/xml/gui.xml` is the other half of the app: a five-menu tree with 60-odd items, two
toolbars, thirteen key bindings, eight camera profiles (Front/Back/Left/Right/Top/Bottom and
two perspectives, with eye, up, right, direction, orientation, zoom, clipping and adaptive
flags) and a quad viewport layout. It is the specification for what the editor's UI *is*,
and it is the piece of rigel with the longest useful life.

## What has already landed

More than the plan says. `api/type` has all three of the pieces the plan names, and it has
them completely.

| rigel | api equivalent | Verdict |
|---|---|---|
| `vertical3d/ArcBall` | `v3d::type::ArcBall` | **Complete.** Same four methods, same semantics, glm for the vector types. |
| `libv3dcore/Camera` | `v3d::type::Camera` | **Complete and then some.** Same projection, view, unproject, project and the six camera moves; gained `orthoFactor()` and `rotate()`. Composes a profile rather than deriving from one. |
| `libv3dcore/CameraProfile` | `v3d::type::CameraProfile` | **Data complete, interface gone.** See gap 2. |
| `libv3dcore/brep/HalfEdgeBRep` | `v3d::brep::BRep` | Modernised and renamed. Loses `DAG::Node`/`DAG::Transform` and the `selected` flag — see gaps 4 and 5. |
| `libv3dcore/brep/{Vertex,Face,HalfEdge}` | `v3d::brep::{Vertex,Face,HalfEdge}` | Migrated. Only `Face` kept `selected()`. |
| `libv3dcore/brep/{Edge,WingedEdgeBRep}` | — | Copied into `api/brep` and **not built** — see the corrections below. |
| `libv3dcore/Scene` | `v3d::core::Scene` in `v3dlibs/core` | Already modernised there, waiting on the editor. |
| `vertical3d/commands/CreatePolyCommandSet` | `v3d::core::create_poly_*` in `v3dlibs/core` | Already reduced to four free functions returning a `brep::BRep`. The command wrapper is gone; the geometry survived. |
| `ViewPort`'s camera modes | `vertical3d/CameraControlTool` | **Ported, with two things dropped** — see the defects. |
| `Window`'s keybinding table | `api/config` + `api/input` + `api/event` | Covered and better: `mappings.json` and `event::Mapper` do what `load_keybindings` did without the hardcoded key switch. |
| `libv3dcommand/Tool` | `v3d::Tool` in `vertical3d/` | **Half.** `activate`/`deactivate` only; `motion`, `button` and `draw` unported. |

The other thing already done is the one the plan does not mention at all: `vertical3d/` is not
an empty shell. It has a `ViewPort`, a `Controller` with a tool map and camera-profile
loading, a `CameraControlTool`, and an `HWRenderContext` — 792 lines, of which `Controller`,
`ViewPort` and `CameraControlTool` are commented out of its `CMakeLists.txt` because they
still include `v3dlibs/hookah`, `v3dlibs/gui` and `luxa/`. It is a partial rigel port that
stalled on the legacy trees, not a blank page.

## What the api does not have

This is the list that makes phase 6 a phase. Each item is something no game in the repo has
ever asked for.

1. ~~**A line primitive.**~~ **Landed 2026-09-01** as
   [ADR-0011](adr/0011-lines-are-the-second-primitive.md) -
   `realtime::LineCanvas` and `vulkan::LineRenderer`, world space and through the pass
   camera. What follows is what the gap was.
   [ADR-0005](adr/0005-one-batched-quad-primitive.md) made the batched
   quad the one primitive, which was right for four games and is wrong for a modeller.
   Rigel's viewport draws, by line count, mostly lines: the construction grid, the axis
   decoration in the corner, wireframe and shaded-wireframe mesh display, selected-edge
   highlighting, the translate and scale manipulator shafts and the rotate manipulator's
   three circles. `realtime::Canvas` offers `rect`, `circle` and `text`, all triangles.
   `vulkan::PipelineBuilder` already takes `topology()` and `polygon()`, so a line pipeline
   is a few chained calls — what is missing is anything that produces line geometry and a
   `DrawItem` path to carry it. **This is the largest single api gap in the survey and the
   plan does not list it.**

2. ~~**`CameraProfile` is write-only.**~~ **Fixed 2026-09-01** - every field has an
   accessor pair, the ortho factor is split into a horizontal and a vertical one, and both
   guard the unset viewport size. What follows is what the gap was.
   `v3d::type::CameraProfile` holds every field rigel's does — name, eye, direction, right, up, orthoZoom, pixelAspect, near, far, fov, rotation,
   options, size — and exposes `clipping`, `eye`, `lookat`, `clone` and `operator=`. The
   other twenty-odd accessors are gone, the rest of the state is `protected` behind
   `friend class Camera`, and the adaptive-projection and adaptive-position options were
   dropped outright. Three consequences:
   - **`gui.xml`'s camera profile table cannot be loaded.** Every attribute in it except
     `near`, `far`, `eye` and `lookat` has no setter to write to.
   - **`Camera::orthoFactor()` divides by zero.** It reads `profile_.size_[0]`, which the
     constructor zeroes and which nothing anywhere ever writes — there is no setter for it.
     `CameraControlTool::zoom` and `::truck` both use it, so the ported camera control is
     dead on arrival whether or not anything calls it yet.
   - **`fov` is unreachable**, which the perspective profiles in `gui.xml` set and rigel's
     own loader also failed to read (see the defects).

3. **Picking.** `ViewPort::selection_hit_test` is `glRenderMode(GL_SELECT)` with a name stack
   and a hand-rolled `gluPickMatrix` — deprecated in GL 3 and nonexistent in Vulkan. The
   name-space encoding it wraps around that is worth keeping: ids below 16,777,216 are
   objects, the next 32 are manipulator axes, everything above is a face, edge or vertex
   index resolved against the active select mask. The mechanism has to be rebuilt as either
   a ray cast against the brep or an id-buffer pass, and that decision is worth an ADR.

4. **A mesh has no identity and no transform.** Rigel's `HalfEdgeBRep` derives from
   `DAG::Node` and `DAG::Transform`, which is where `id()`, `matrix()`, `translation()`,
   `rotation()` and `scale()` come from — the selection model keys on the id and all three
   manipulators write through the transform. `api/dag::Node` and `api/dag::Transform` exist
   and carry exactly those members. `api/brep::BRep` derives from neither.

5. **Selection state on three of the four brep types.** `api/brep::Face` kept `selected()`.
   `Vertex`, `HalfEdge` and `BRep` did not, and component selection needs all four.

6. **A per-pass camera on an orthographic pass, and more than one viewport.** Phase 5 already
   deferred giving `Pass` an orthographic camera to this phase, on the grounds that multiple
   viewports are the first thing that needs one. Four views of one scene is four passes with
   four cameras against one device, which [ADR-0003](adr/0003-one-realtime-engine.md) says
   the model expresses; this is the app that finds out.

7. **The interactive half of the command model.** `api/event` maps input to named events and
   dispatches them by context; it has no `Command`, and nothing in it corresponds to rigel's
   `Tool` — a command that also receives motion and button events and draws its own feedback
   while active. `SplitEdgeTool` is the worked example: it tracks the nearest edge under the
   cursor every motion event and draws a vertex handle there until a click commits.
   `vertical3d/Tool.h` has the activate/deactivate half and nothing else.

8. **Project persistence.** `ProjectCommandSet::read`/`write` is a libxml++ DOM walk over a
   `<project><scene><mesh>` tree with transforms, vertices, edges and faces. It is the only
   scene serialisation anywhere in the repository. `api/config` and `api/asset` are JSON, so
   this is a format decision as much as a port.

9. **Undo.** Rigel has none — a case-insensitive search for undo or redo over the whole tree
   returns nothing. The plan lists an undoable command model among the things phase 6 needs
   and lists rigel as holding "the working prototype of most of the above"; on this one it
   holds nothing.

Items 1, 2 and 6 are the ones that gate everything else - a modeller that cannot draw a line,
cannot configure a camera and cannot show four views is not a modeller. Two of the three
landed on 2026-09-01; item 6, the per-pass camera and the multiple viewports over it, is what
is left of the gate.

## Defects found

All in `rigel/` unless marked, all independent of the port, and none of them observable —
the tree has not compiled since 2022.

- **`ViewPort::VisibleFilter` is not a bit field but is used as one.** It declares
  `SHOW_GRID = 1` and lets the rest default, so the five values are 1, 2, 3, 4, 5. The
  constructor ORs all five into `_showFlags` and gets 7; `view::show::handle` does
  `_showFlags ^= SHOW_HANDLE`, which is `^= 3`, and toggles grid and camera instead. Only
  `SHOW_GRID` and `SHOW_MESH` are ever tested, and `SHOW_MESH` is 5, so hiding the grid also
  hides the meshes. `Window::SelectMasks` in the same tree gets this right with `(1 << n)`.
  `vertical3d/ViewPort.h` had copied the broken enum verbatim; **fixed there 2026-09-01**,
  since that file is the one that survives.

- **`TranslateManipulator::transform` throws away the x component of a free drag, and treats
  a frame delta as an absolute position.** The unconstrained branch reads
  `t = camera->right() * mouse_delta[0];` and then `t = camera->up() * -mouse_delta[1];` —
  an assignment, not an accumulation — so horizontal movement is discarded. It then calls
  `_selection->translation(t)`, setting the object's translation to one frame's delta rather
  than adding to it, so a drag snaps the object to the origin and jitters. `ScaleManipulator`
  gets both halves right (`s += …` from the current scale) and `RotateManipulator` composes
  onto the current rotation, which is what makes the translate case clearly a defect rather
  than a convention.

- **`ConstructionPlane` calls `glLineWidth` between `glBegin` and `glEnd`.** It does this
  twice per grid draw, once per axis loop, to thicken the origin line. `glLineWidth` is not
  one of the calls allowed inside a Begin/End block, so every one of them raises
  `GL_INVALID_OPERATION` and the origin line is never thicker. Keep the intent, drop the
  mechanism.

- **`ConstructionPlane`'s size, infinite and autoscale settings are dead.** Five setters,
  five getters, and `draw()` reads none of them — it hardcodes `max = 60` and uses only
  `_spacing` and `_majorIntervals`. `_size`, `_infinite`, `_autoscale` and `_scaleFactor` are
  also uninitialised, the constructor's initialiser list covering only the two fields that
  are used.

- **`TransformManipulator` leaves `_coordinateSpace` uninitialised.** The constructor
  initialises `_axisConstraint` and nothing else, so a manipulator is in global or local
  space depending on what was on the heap. Nothing reads it yet, which is the only reason it
  does not show.

- **`ViewLayout::build` and `::build_children` both dereference a loop variable that may not
  have been assigned.** Each scans a child list for the first `viewgroup` or `viewport`
  element, then calls `view_elem->get_name()` without checking that the scan found one.
  `build_children` does check `it == child_elems.end()` — three statements *after* the
  dereference. A `<viewgroup>` with fewer than two view children takes the process down.

- **`SplitEdgeTool::motion` reports edge 0 for the first edge it considers.** The
  `first_edge` branch records `nearest_distance` and `nearest_t` but not `nearest_edge`,
  which stays 0 from its initialiser. Since edges shorter than a pixel in screen space are
  skipped, the first *considered* edge is often not edge 0, and the tool then splits the
  wrong edge unless something later beats it on distance.

- **`load_camera_profiles` never reads `fov`.** It reads twelve attributes and `fov` is not
  among them, so both perspective profiles in `gui.xml` — the only two that set it — fall
  back to whatever `CameraProfile`'s constructor leaves. Carry the attribute across when the
  file is translated.

- **24 of the 51 commands `gui.xml` names are registered nowhere.**
  `dispatch_menu_command` handles `quit`, `render::view` and `render::batch` inline;
  `CommandDirectory` covers 27; the remaining 24 — the timeline transport, the four
  importers, camera creation, grouping, render settings and five bare `camera::*` bindings —
  reach the "Unhandled Command!" branch. The menu tree is a design document as much as a
  working UI, which is worth knowing before anyone treats it as a checklist.

- **The key bindings in `gui.xml` only work for thirteen keys.** `Window::on_key_press_event`
  is a hardcoded `switch` translating GDK keyvals into `Event("Keyboard::X")` strings, so a
  `<bind>` naming any other key is loaded, stored and never fired. `api/input` and
  `api/config` already solve this properly; the fix is not to port the switch.

- **`Window::activeView(id)` clears the active flag before matching.** Its loop is
  `if (active) clear; else if (id matches) set`, so asking for the view that is already
  active leaves no view active at all. The one caller guards against it with `!_active`,
  which is why it has never mattered.

- **`Window::renderScene` has never compiled.** It sits behind `#ifdef USE_MOYA` and calls
  `mesh->polygons()` and `mesh->vertices()` expecting `std::vector<Polygon>` and
  `std::vector<Vector3>`; `HalfEdgeBRep` has neither. It is the sketch of an editor-to-moya
  bridge and the `render::batch` command's only body.

### One already-migrated piece lost behaviour

`vertical3d/CameraControlTool` is rigel's `ViewPort` camera-mode block, ported and tidied,
and two things did not come across:

- ~~**The arcball is never clicked.**~~ Fixed 2026-09-01. Rigel called `_arcball.click()`
  on button press to set the drag reference point and `_arcball.bounds(w, h)` on configure
  to size the sphere. `CameraControlTool::buttonPressed` was empty and nothing called
  `bounds`, so `pan` dragged against an unset start point on a zero-sized sphere.
- ~~**Vertical truck uses the horizontal factor.**~~ Fixed 2026-09-01. Rigel scaled a
  pedestal by `(orthoZoom * 2) / height` and a truck by
  `(orthoZoom * 2 * pixelAspect) / width`; `orthoFactor()` was only the second, and
  `CameraControlTool::truck` used it for both. On a 4:3 view that is a 1.77x error on
  vertical drags.
- **`CameraControlTool::pan` drags the arcball to the point the gesture came from**, not the
  one it has reached - `motion()` records `last_` only after `pan()` returns, so every
  rotation is one event stale and the first after a click is identity. Rigel passed the
  current event position. Fixed 2026-09-01 with the two above.

## Corrections

- **`docs/TODO.md`'s two rigel items both name directories that do not exist.** "make sure
  rigel libv3dcore/brep doesn't have anything missing from v3dlibs/brep" — there is no
  `v3dlibs/brep`; the brep tree went to `api/brep`. "merge rigel libv3dcommand with
  v3dlibs/command" — `rigel/libv3dcommand` is `Tool` and nothing else, and `v3dlibs/command`
  is itself being replaced by `api/event`, which has no `Tool`. Both items should be rewritten
  against `api/`.

- **`api/brep` already contains rigel's `Edge`, `HalfEdgeBRep` and `WingedEdgeBRep`, and does
  not build them.** All six files are rigel's, reformatted to the house brace style and
  otherwise unchanged: still `namespace v3D`, still including `libv3dtypes/AABBox.h`,
  `libv3dgraph/Node.h` and `libv3dgraph/Transform.h`, none of which exist. `api/brep`'s
  `CMakeLists.txt` names only `BRep`, `Face`, `HalfEdge` and `Vertex`. Nothing anywhere
  includes the other three. So the brep merge is not "check nothing is missing and delete" —
  it is a decision about whether the winged-edge representation is wanted at all, and if it
  is, porting it the way `HalfEdgeBRep` became `BRep`. If it is not, six files should go from
  `api/brep`, not just from rigel.

- **`v3dlib_brep` has no consumer.** Only its own test binary links it.

- **`RenderView` is worth nothing.** The plan lists it among the working prototypes to fold
  in. It is 37 lines that put an empty `Gtk::DrawingArea` in a window; the render view was
  never implemented. Drop it from the list.

- **`vertical3d/` is further along than the plan records.** The plan says to "rewrite
  `vertical3d/` — `Controller`, `ViewPort`, `CameraControlTool`, `HWRenderContext`", which
  reads as four things to write. Three of the four exist and are written against the current
  `api/type` and glm; what stops them building is `v3dlibs/hookah`, `v3dlibs/gui` and
  `luxa/ComponentManager`, all of which are already scheduled for replacement elsewhere.
  `HWRenderContext` is the only one that has to go outright.

- **CLAUDE.md says rigel's "viewport, manipulator and modelling-command work has to be folded
  into the rewrite before it can go".** That is right about the manipulators and the command
  sets and wrong about the viewport: `ViewPort` is 1,381 lines of GTK signal handlers and
  immediate-mode GL, and the part of it that had portable value — the camera modes — is
  already in `vertical3d/CameraControlTool`. What is left to take from `ViewPort` is the
  picking name-space scheme and the decoration and handle geometry, both of which are
  descriptions rather than code.

## What has to happen before `rigel/` can be deleted

In dependency order. Nothing here is blocked by the Vulkan work, which landed for the four
games; it is blocked by the api never having had a customer that draws lines or picks.

1. Translate `docs/xml/gui.xml` into the repo's JSON config form — menus, toolbar, bindings,
   camera profiles, viewport layout — and put it in `vertical3d/data/`. This is the single
   highest-value item and it depends on nothing except gap 2.
2. Give `api/type::CameraProfile` back its accessors, including the viewport size that
   `orthoFactor()` divides by, and decide whether adaptive projection and position come back.
3. Give the api a line primitive: geometry on `Canvas` or a sibling of it, and a line
   pipeline through `PipelineBuilder`.
4. Give `api/brep::BRep` a `dag::Node`/`dag::Transform` base and put `selected()` back on
   `Vertex`, `HalfEdge` and `BRep`.
5. Decide picking — ray cast or id buffer — in an ADR, and port the name-space scheme onto it.
6. Port the three manipulators onto lines and the picking decision, fixing the translate
   defect and the uninitialised coordinate space on the way.
7. Port `ConstructionPlane` onto lines, either implementing autoscale or dropping its
   interface.
8. Port the five command sets onto `api/event` contexts, and decide what a `Tool` is in the
   api now that `event::Engine` dispatches by name.
9. Decide the project file format and port `ProjectCommandSet::read`/`write` onto it.
10. Settle `api/brep`'s three unbuilt files — port or delete.
11. Multiple viewports, which is the phase's headline feature and wants items 2, 3 and 6 first.

Undo is on the phase 6 list and rigel contributes nothing to it; it should be scoped
independently rather than treated as a fold-in.

Delete `rigel/` when 1 through 10 have landed. `docs/xml/gui.xml`, the four icons and this
document are what should outlive it.
