# Rigel Survey

**Closed 2026-09-04. `rigel/` is deleted.** The eleven-item list is worked off; what follows
is the record of what the tree held and where each piece went, kept because it is the only
account of that. Recover a source with `git show 54d79e8^:rigel/...` — the four PNGs in
`rigel/icons/` are the only thing a later change is likely to want, and they were already
recovered into `vertical3d/data/icons/`.

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
| `libv3dcore/brep/HalfEdgeBRep` | `v3d::brep::BRep` | Modernised and renamed. **Complete as of 2026-09-02** — the `dag::Node`/`dag::Transform` base and the `selected` flag are back. |
| `libv3dcore/brep/{Vertex,Face,HalfEdge}` | `v3d::brep::{Vertex,Face,HalfEdge}` | Migrated. All three carry `selected()` again as of 2026-09-02. |
| `libv3dcore/brep/{Edge,WingedEdgeBRep}` | `v3d::brep::{Edge,WingedEdgeBRep}` | Ported and built 2026-09-04, the way `HalfEdgeBRep` became `BRep`. |
| `libv3dcore/Scene` | `v3d::editor::Scene` in `vertical3d/src` | **Moved 2026-09-02**, and reduced to meshes: the views own the cameras and `CameraProfiles` owns the profile table. |
| `vertical3d/commands/CreatePolyCommandSet` | `v3d::editor::create_poly_*` in `vertical3d/src` | **Moved 2026-09-02.** Four free functions returning a `brep::BRep`; the command wrapper is gone. The cone and the cylinder were rebuilt about +y, centred like the cube and the plane, and the cone's ring had an uninitialised third coordinate. |
| `ViewPort`'s camera modes | `vertical3d/CameraControlTool` | **Ported, with two things dropped** — see the defects. |
| `Window`'s keybinding table | `api/config` + `api/input` + `api/event` | Covered and better: `mappings.json` and `event::Mapper` do what `load_keybindings` did without the hardcoded key switch. |
| `libv3dcommand/Tool` | `v3d::editor::Tool` in `vertical3d/src` | **Ported bar `draw`.** `activate`, `deactivate`, `motion` and `button`; a tool that draws feedback does it through the manipulator it holds rather than through the interface. |

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

3. ~~**Picking.**~~ **Fixed 2026-09-02** —
   [ADR-0014](adr/0014-picking-is-a-cpu-ray-cast.md). It is a cpu ray cast against the brep
   rather than an id-buffer pass: an object and a face are hit by the ray meeting a triangle
   of a fan over the face's loop, and a vertex and an edge — which are drawn one pixel wide
   and have no area to rasterise or to hit exactly — by screen space proximity, nearest to
   the camera winning, which is what the depth sorted hit buffer below did.
   `ViewPort::selection_hit_test` was `glRenderMode(GL_SELECT)` with a name stack and a
   hand-rolled `gluPickMatrix`, deprecated in GL 3 and nonexistent in Vulkan. Its behaviour
   came across and its encoding did not: an object still has to be selected before any of
   its components may be, a miss still deselects, and a second click on a component still
   toggles it — but the name-space encoding (ids below 16,777,216 objects, the next 32
   manipulator axes, everything above a component index) existed only because a GL name stack
   carries one `GLuint`. A `Hit` is a struct and says what kind of thing it holds, so the
   manipulators will be picked by their own test against the ray rather than by reserving a
   range of the same integer.

4. ~~**A mesh has no identity and no transform.**~~ **Fixed 2026-09-02** -
   [ADR-0013](adr/0013-mesh-is-a-dag-node.md). What follows is what the gap was. Rigel's `HalfEdgeBRep` derives from
   `DAG::Node` and `DAG::Transform`, which is where `id()`, `matrix()`, `translation()`,
   `rotation()` and `scale()` come from — the selection model keys on the id and all three
   manipulators write through the transform. `api/dag::Node` and `api/dag::Transform` exist
   and carry exactly those members. `api/brep::BRep` derives from neither.

5. ~~**Selection state on three of the four brep types.**~~ **Fixed 2026-09-02.** All four
   carry it - and `Face`'s had been commented out rather than kept, so it was four of four
   rather than three. `BRep::deselectComponents()`, `Scene::deselect()` and
   `Scene::deselectComponents()` clear them. The select mask that decides which of the three
   a click writes landed with picking the same day: `v3d::editor::SelectMask` is object,
   vertex, edge or face, held by `SelectTool`, and changing it clears the component
   selection.

6. ~~**A per-pass camera on an orthographic pass, and more than one viewport.**~~
   **Landed 2026-09-01.** The editor draws four passes over one frame, each with its own
   region and its own camera at set 0, and three of the four cameras are orthographic. The
   frame model expressed it unchanged - what had to be fixed was underneath it:
   `v3d::type::Camera` built OpenGL clip space and looked down the wrong axis
   ([ADR-0012](adr/0012-camera-builds-vulkan-clip-space.md)), `CameraProfile::lookat()`
   stored the transpose of its rotation, and both geometry renderers wrote every submission
   of a frame into the same buffer, so a second canvas overwrote the first.

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

9. ~~**Undo.**~~ **Designed 2026-09-02**, as
   [ADR-0016](adr/0016-undo-records-what-has-already-happened.md). Rigel has none — a
   case-insensitive search for undo or redo over the whole tree returns nothing — so this one
   was scoped independently rather than folded in, and the plan's claim that rigel holds "the
   working prototype of most of the above" never covered it. A command is a record of a
   change already made, one gesture is one command, and `TransformTool` is where a gesture's
   ends are known.

Items 1, 2 and 6 are the ones that gate everything else - a modeller that cannot draw a line,
cannot configure a camera and cannot show four views is not a modeller. Two of the three
landed on 2026-09-01: the editor builds, runs, and draws a construction grid through four
viewports of one scene. Items 4, 5, 3, 6 and 9 landed on 2026-09-02, and the editor draws a
scene, selects what is in it, moves what is selected and takes it back. What is left of this
list is items 7 and 8 - the interactive command model and project persistence. Item 7 is most
of the way there: `SelectTool` and `TransformTool` are `Tool`s that receive motion and button
events, `TransformTool` draws its own feedback the way `SplitEdgeTool` did, and there is a
`Command` now - but it is the undoable half of one per ADR-0016 rather than the invocable
half rigel's `CommandDirectory` held, and there is still no tool map.

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

- ~~**`TranslateManipulator::transform` throws away the x component of a free drag, and treats
  a frame delta as an absolute position.**~~ **Fixed in the port, 2026-09-02.** The unconstrained branch reads
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

- ~~**`TransformManipulator` leaves `_coordinateSpace` uninitialised.**~~ **Fixed in the
  port, 2026-09-02.** The constructor
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

- ~~**`api/brep` already contains rigel's `Edge`, `HalfEdgeBRep` and `WingedEdgeBRep`, and
  does not build them.**~~ Settled 2026-09-04. All six were rigel's, reformatted to the house
  brace style and otherwise unchanged: still `namespace v3D`, still including
  `libv3dtypes/AABBox.h`, `libv3dgraph/Node.h` and `libv3dgraph/Transform.h`, none of which
  exist, so none of them compiled. `Edge` and `WingedEdgeBRep` are ported and built, with
  suites of their own; `HalfEdgeBRep` is deleted, because `BRep` is what it became and
  building it would have put a second copy of that class in the same library.

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
  already in `vertical3d/CameraControlTool`. Its picking behaviour came across on 2026-09-02
  and its name-space encoding deliberately did not, per
  [ADR-0014](adr/0014-picking-is-a-cpu-ray-cast.md). What is left to take from `ViewPort` is
  the decoration and handle geometry, which is a description rather than code.

## What has to happen before `rigel/` can be deleted

In dependency order. Nothing here is blocked by the Vulkan work, which landed for the four
games; it is blocked by the api never having had a customer that draws lines or picks.

1. ~~Translate `docs/xml/gui.xml` into the repo's JSON config form — menus, toolbar,
   bindings, camera profiles, viewport layout — and put it in `vertical3d/data/`.~~
   **Half done 2026-09-01.** The camera profile table is `data/cameras.json`, the viewport
   layout is `data/layout.json`, and the camera bindings are `data/mappings.json`; both new
   files load through `api/config`, which gained a `camera` and a `layout` type for them.
   The command strings behind the menus are translated as of 2026-09-02 — item 8 registers
   nineteen of them under gui.xml's own names, and item 9 adds `project::load` and
   `project::save`, so a menu item has something to invoke. **The menus themselves landed
   2026-09-02**, as `vertical3d/data/vgui.json` and
   [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md): nine menus over 75 commands,
   drawn by `api/ui` as a bar with dropped panels and flyouts, hit tested against the bounds
   the renderer leaves on each component. **The two toolbars landed 2026-09-04**, which
   closes this item: `ui::component::Toolbar` is a strip of `Button`s on the top or the left
   edge of the window, and both are more of `vertical3d/data/vgui.json`. Two departures from
   what gui.xml says, both forced: its top toolbar gives its nine buttons neither a name nor
   an icon, so the four masks the editor has are labelled with the mask's own name and the
   five with no node type to select are left out, as ADR-0014 left them out of the mask; and
   the left toolbar's four buttons were labelled rather than drawn from `rigel/icons/`, there
   being no image path in `api/ui` at the time. **The second departure closed the same day**,
   with [ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md): a `Button`
   names an image, the ui engine's image pass resolves it through the app, and the four PNGs
   were recovered into `vertical3d/data/icons/` before this tree was deleted. The top toolbar
   is still labelled, gui.xml naming nothing to draw there.

   **gui.xml has 79 distinct command strings, not the 51 counted below**; 73 of them are on
   a menu, and six — `view::camera::{pan,truck,zoom}`, `view::display::{shaded,wireframe}`
   and `view::pop` — appear only on a toolbar or a binding. The editor answers to 24, 20 of
   which the menu names.
2. ~~Give `api/type::CameraProfile` back its accessors, including the viewport size that
   `orthoFactor()` divides by, and decide whether adaptive projection and position come
   back.~~ Done 2026-09-01, both options included.
3. ~~Give the api a line primitive: geometry on `Canvas` or a sibling of it, and a line
   pipeline through `PipelineBuilder`.~~ Done 2026-09-01 as
   [ADR-0011](adr/0011-lines-are-the-second-primitive.md), and first drawn the same day.
4. ~~Give `api/brep::BRep` a `dag::Node`/`dag::Transform` base and put `selected()` back on
   `Vertex`, `HalfEdge` and `BRep`.~~ Done 2026-09-02, recorded as
   [ADR-0013](adr/0013-mesh-is-a-dag-node.md). `Face` got it back too - its accessors were
   commented out rather than kept. `dag::Transform` had to be made to compile first: it
   named members its header does not declare and called three glm methods that do not
   exist, and its `CMakeLists.txt` listed the header twice and the implementation not at
   all, so nothing had ever built it.
5. ~~Decide picking — ray cast or id buffer — in an ADR, and port the name-space scheme onto
   it.~~ Done 2026-09-02: [ADR-0014](adr/0014-picking-is-a-cpu-ray-cast.md), a ray cast. The
   name-space scheme is not ported — a typed `Hit` replaces it.
   `Camera::project()` and `::unproject()` are inverses of each other as of 2026-09-01,
   which a ray cast would be built on; they were not before.
6. ~~Port the three manipulators onto lines and the picking decision, fixing the translate
   defect and the uninitialised coordinate space on the way.~~ Done 2026-09-02, recorded as
   [ADR-0015](adr/0015-manipulators-write-the-object-transform.md). A handle writes the
   mesh's `dag::Transform`, is drawn at the object's own origin because that is where the
   transform pivots, and is picked by projecting itself to the screen. Both defects are
   fixed and the coordinate space is initialised. Rigel's centring on the selected
   components is not ported: rotation and scale pivot at the origin, so a gizmo drawn at a
   face's centre would turn the object about a point it is not drawn at.
7. ~~Port `ConstructionPlane` onto lines, either implementing autoscale or dropping its
   interface.~~ Done 2026-09-01, in `vertical3d/src/ConstructionPlane.cxx`. Autoscale and
   `infinite` are dropped: neither had a reader in rigel either — `_autoscale`, `_infinite`
   and `_scaleFactor` were set and never used. Line width goes with them, per ADR-0011;
   colour carries the emphasis the origin lines had.
8. ~~Port the five command sets onto `api/event` contexts, and decide what a `Tool` is in
   the api now that `event::Engine` dispatches by name.~~ Done 2026-09-02, recorded as
   [ADR-0017](adr/0017-a-command-is-a-name-in-a-context.md). A command is identified by its
   context and name together, which is what `Event::str()` returns and what a binding and a
   menu item both carry, so `v3d::editor::CommandDirectory` maps that string to a handler
   and `Controller::handleEvent` is a lookup. `data/mappings.json` was rewritten onto
   gui.xml's own command names, so the menu translation of item 1 now has commands to name.
   `Tool` stays in the editor: no game holds a gesture open across events, and one consumer
   is not a library. 21 of gui.xml's 51 commands have handlers, out of 24 registrations; the
   other 30 log themselves as unregistered rather than being dropped, which is what makes
   item 1 checkable.
9. ~~Decide the project file format and port `ProjectCommandSet::read`/`write` onto it.~~
   Done 2026-09-02, recorded as
   [ADR-0018](adr/0018-a-project-is-json-and-stores-topology-verbatim.md). The shape is
   rigel's - a project of meshes, each a transform and flat lists of vertices, half edges
   and faces addressed by index - in JSON rather than XML, because the library rigel parsed
   with is gtkmm's and `vault/quantumxml` was archived when the JSON config work replaced
   it. `v3d::editor::Project` is the reader and the writer. Three departures: the topology
   is written index for index rather than rebuilt through `BRep::addFace`, which welds and
   pairs by search and would renumber the mesh; the selection rigel stored on every mesh,
   vertex, edge and face is not stored, being where the user is rather than what the
   document holds; and there is no scene array, the editor having one `Scene` and the
   `version` field being what makes adding the level cheap. Rigel's `Window::fileChooser`
   has no counterpart, so both commands work on one document at a fixed path.
10. Settle `api/brep`'s three unbuilt files — port or delete. **Still open**, and it did not
    hold the deletion up: `Edge`, `HalfEdgeBRep` and `WingedEdgeBRep` are in `api/brep/` and
    absent from its `add_library` list, which is a question about that library rather than
    about this tree. It is the one item on [TODO.md](TODO.md) that came from here.
11. ~~Multiple viewports, which is the phase's headline feature and wants items 2, 3 and 6
    first.~~ Done 2026-09-01, and it wanted 2 and 3 but not 6. Four passes over one frame,
    each with its region and its camera.

~~Undo is on the phase 6 list and rigel contributes nothing to it; it should be scoped
independently rather than treated as a fold-in.~~ Done that way on 2026-09-02, as ADR-0016.

**`rigel/` was deleted on 2026-09-04**, with 1 through 9 and 11 landed. Item 10 went with it
unanswered, which was the right call — it is a question about `api/brep`, not about anything
this tree held.

Of the three things that were to outlive it, two did: the four icons are in
`vertical3d/data/icons/`, and this document. **`docs/xml/gui.xml` did not** — it is in git
history only, at `git show 54d79e8^:rigel/docs/xml/gui.xml`. Its content survives translated,
across `vertical3d/data/`'s `vgui.json`, `cameras.json`, `layout.json` and `mappings.json`,
but a question about what the original said has to go to git.
