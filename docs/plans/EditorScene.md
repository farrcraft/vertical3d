# Editor Scene — Cameras, Lights, Groups, Shading, Import, Render and Playback

Drafted 2026-10-24 against `ab4bdea1`. This plan takes the six editor groups in
[TODO.md](../TODO.md#editor). When it closes, every one of the menu's 83 commands has a handler.
The scene holds cameras, lights and groups as well as meshes. The views draw shaded as well as
in wireframe, a project imports glTF, the Render menu renders the scene with moya, and the
timeline keys and plays back transforms.

The editor is described in [editor/](../editor/README.md). It edits half-edge meshes
(`brep::BRep`) in a `Scene`, records each completed change as a `Command` on a `CommandStack`,
and draws lines only.

Each entry this plan takes stays in TODO.md until the step that does it lands, and is then
deleted there. A step with a testable CPU half brings test cases with it.

## What the survey found

The tree was read at `ab4bdea1`. Line numbers are from that commit, under `vertical3d/` unless
another directory is named. The Reshaping plan has since moved the editor's sources into
directories by layer, folded `api/dag` into `Item`, and moved animation out of `api/type`, so
the paths below are where things were. The open steps name where things are now.

**The scene:**

* **The scene is a flat list of meshes.** `Scene` holds a `std::vector` of `brep::BRep`
  (`src/scene/Scene.h:84`), and `SceneVisitor` has one `visit(shared_ptr<BRep>)`
  (`src/scene/SceneVisitor.h:10-23`).
* **`api/dag` is two small classes.** `Node` gives an id (`api/dag/Node.h:15-28`) and `Transform`
  holds a local translation, rotation and scale (`api/dag/Transform.h:51-83`). There are no
  parent links and no world matrix. ADR-0013 rejected a full scene graph, and names grouping as
  the point to revisit it.
* **Everything that reads the scene is typed on `BRep`:** `Picker` (`src/tool/Picker.h:34`),
  `Target::mesh` (`src/manipulator/Target.h:57`), `CreateCommand`
  (`src/command/CreateCommand.h:32`), `Scene::selection()` (`src/scene/Scene.h:54`), `Project`,
  and `RIBExportVisitor::visit` (`src/scene/RIBExportVisitor.h:55`).
* **`ViewPort` already has `SHOW_CAMERA` and `SHOW_LIGHT`** (`src/view/ViewPort.h:40-44`). They
  have no command because nothing draws either (`src/Controller.cxx:318-319`).
* **`SelectMask` has four values**, `Object`, `Vertex`, `Edge` and `Face`
  (`src/tool/SelectMask.h:20-24`). The menu also offers curve, mesh, light, camera and handle.

**The project file:**

* **`Project::VERSION` is 1** (`src/scene/Project.cxx:271`), read through `asset::readForward`
  with an empty migration chain (`Project.cxx:311-324`). The file holds meshes only, and no ids
  (`docs/editor/Files.md:9-26`). A read is all or nothing (`Project.cxx:331-345`).

**Drawing:**

* **A mesh is drawn as lines, every frame, on the CPU.** `WireframeVisitor` appends each edge to
  a line canvas (`src/view/ViewPort.cxx:131`). No mesh becomes GPU geometry.
* **`api/render/realtime` has a lit pass that no app uses.** `MeshRegistry` uploads a
  `type::Model` (`api/render/realtime/MeshRegistry.h:177`) and `renderer::Lit` draws it
  (`renderer/Lit.h:49`). Lit is cel shading with fixed cull and polygon modes, and needs
  an sRGB target. `pipeline::Builder` can set both modes (`vulkan/pipeline/Builder.h:92,98`).
  `WindowRenderer::context()` gives an app the device context to build its own pass.
* **BRep faces wind clockwise from outside**, and Lit and glTF expect counter-clockwise.

**Import:**

* **The menu's importers are LightWave, DXF, 3DS and MilkShape** (`data/vgui.json:503-521`).
  Nothing in the tree reads any of them. glTF is the only mesh format the api reads, and the
  menu has no item for it.
* **The glTF loader flattens a scene** into one indexed triangle list in world space
  (`api/asset/media/loader/Gltf.cpp:182-184`).
* **`BRep::addFace` welds by exact float equality with a linear scan**, and pairs edges with a
  linear search (`api/brep/BRep.cxx:52-107`). Building a large mesh face by face is quadratic.

**Rendering:**

* **The RIB export writes the active view's camera and the meshes**, and no lights, materials or
  `Display` (`src/scene/RIBExportVisitor.cxx:39-112`). It writes `export.rib` beside the
  executable (`src/Controller.cxx:482`).
* **moya is both a library and an executable.** `v3dlib_moya` (`moya/libmoya/CMakeLists.txt`)
  and `moya/moya/moya.cxx`, which reads a RIB file and writes the picture at `RiWorldEnd`. It
  takes `LightSource` and `AreaLightSource`.

**Time:**

* **The editor has no `simulate()` or `tick()`** (`src/Controller.h:58-109`), and nothing in the
  editor or `api/brep` keeps a time.
* **`api/type/animation` keys joints for skins**: `Channel`, `Clip`, `Pose`, `Clock` and
  `Track<T>`. `Track<glm::vec3>` is the nearest fit for keying a whole object.

**Cameras:**

* **The `camera::*` items are a menu, "Camera Manipulators"** (`data/vgui.json:369-399`), not a
  toolbar. They name truck, dolly, zoom, pan and pitch. The working camera's moves are held
  commands under `view::camera::*` (`src/Controller.cxx:335-347`), and `CameraControlTool` has
  zoom, truck and pan only (`src/tool/CameraControlTool.h:31-36`).

## The 45 commands

| Group | Commands | Step |
|---|---|---|
| Show | `view::show::light`, `view::show::camera` | 4 |
| Select masks | `select::mask::mesh`, `light`, `camera` (step 4), `curve` (step 13), `handle` (step 14) | 4, 13, 14 |
| Scene cameras and groups | `create::ortho::camera`, `create::perspective::camera` (step 4), `create::group` (step 9) | 4, 9 |
| Camera Manipulators | `camera::truck`, `dolly`, `zoom`, `pan`, `pitch` | 5 |
| Shading | `view::shade::flat`, `smooth`; `view::cull::none`, `front`, `back`, `both`; `view::rendermode::front::*`, `back::*` (points, lines, fill) | 6 |
| Import | `project::import::lwo`, `dxf`, `3ds`, `milkshape`, replaced by `gltf` and `usd` | 7 |
| Render | `render::settings`, `render::view`, `render::batch` | 8 |
| Timeline | `timeline::set`, `timeline::playback::*` (ten) | 10 |

The curve and handle masks need kinds of item the editor does not have. Steps 13 and 14 build
them.

## Decisions

| Decision | Step |
|---|---|
| Editor: what a scene item is, and how the scene holds kinds other than meshes. Decided in ADR-0099, which supersedes ADR-0013. | 1 |
| Editor: how a project is laid out on disk. Decided in ADR-0100: a directory with a manifest. | 2 |
| Editor: what a scene document is. Decided in ADR-0101, which supersedes ADR-0018: a SQLite database, saved incrementally, with autosave to a separate recovery database. | 2 |
| Editor: how records are encoded and versioned. Decided in ADR-0102: typed columns for the core, JSON payloads, a schema version and a version per record type. | 2 |
| Editor: how a saved record refers to an item. Decided in ADR-0103: a persistent document id. | 2 |
| Editor: where hierarchy lives, in `api/dag` or in the editor, and how a world matrix is composed. Decided in ADR-0104, which amends ADR-0099: the scene holds the links and evaluates every placement. | 9 |
| Editor: how the editor renders with moya, by linking `v3dlib_moya` or by running `moya.exe`. Decided in ADR-0106: a renderer interface for interactive renders, with moya linked behind it, and `moya.exe` for batches. | 8 |
| Editor: which import formats the menu offers, and what an import makes. Decided in ADR-0122: glTF and USD, each read as a scene of items. | 7 |
| Editor: what a curve is, whether it is a kind of item, and whether moya renders one. | 13 |
| Editor: what a handle is. | 14 |
| Editor: what the editor animates, and how a key is stored. Decided in ADR-0105: a scalar curve per numeric property, keyed on frames. | 10 |

## What blocks what

- **Step 1 blocks steps 2, 3, 4 and 9.** Every kind other than a mesh needs a scene item to be.
  Step 1 is closed.
- **Step 2 goes with step 1.** A scene that holds a camera has to save one.
- **Step 11 needs step 2**, and gives step 6 the revision it re-uploads by. **Step 12 needs
  step 11**, so that an autosave writes only what changed.
- **Step 3 comes before step 4's show toggles**, so that a toggle changes what is drawn.
- **Step 7 needs the builder in step 7's first half** before any importer can make a mesh.
- **Step 8 works on today's export.** It renders better after step 4, which gives the export
  lights and a scene camera to render from.
- **Step 9 needs step 1, and touches picking, the manipulators, the export and the file.** Step 9
  is closed.
- **Step 10 needs its ADR, and is best last**, so that a group and a camera can be keyed.
- **Step 13 needs step 1**, and **step 14 needs its ADR**, which decides what else it waits on.
- **Step 5 is independent** of everything else, and can land at any point. Step 6 needs only
  step 11.

## Steps

### Step 1 — A scene of items

**Closed.** ADR-0099 decided it, and supersedes ADR-0013. It weighed a variant of the kinds, a
`dag::Node` found by cast, and a list per kind, against an editor base class.

- `editor::Item` is a `dag::Node` and a `dag::Transform` with the object selection flag.
  `Mesh`, `Camera` and `Light` derive from it, and `as<T>()` casts by the item's kind.
- `brep::BRep` is geometry alone. It lost its `dag` bases and its object flag, `v3dlib_brep` no
  longer links `v3dlib_dag`, and a `BRep` can be copied. A `Mesh` holds a shared `BRep`, so two
  items can share one, and a modelling operation runs once on a shared mesh.
- `Scene` holds items of every kind in one list. `selection()` returns items, `selected<T>()`
  the items of one kind, `item()` any kind, and `mesh()` a mesh.
- `SceneVisitor` has a pure `visit` per kind. `Picker`, `WireframeVisitor`, `RIBExportVisitor`
  and the project writer do nothing with a camera or a light until steps 2 to 4.
- `CreateCommand` and `TransformCommand` take any item, and an object `Target` holds items, so
  step 4's cameras and lights move and undo through the code that moves a mesh.
- No visible change. The editor suite's cases changed with the types, since a mesh is now an
  item that holds geometry, and kept what they check. `ItemTest` and `SceneTest` cover the
  kinds, the casts, shared geometry and the order across kinds.

### Step 2 — The project store

**Closed.** ADR-0100 to ADR-0103 decided it, after weighing JSON, a JSON document with binary
chunks, LMDB, LevelDB, a file format of our own, glTF and USD, and FlatBuffers and Cap'n Proto
for the payloads. The JSON project file is gone, and a project written by an earlier build is not
read.

- A new api library, `v3dlib_database`, wraps SQLite: a connection, statements, transactions and
  the two pragmas a document carries. No other library or app includes SQLite.
- A project is a directory: `project.json`, a manifest written atomically, and the `scenes`,
  `assets`, `cache`, `renders` and `autosave` directories. The manifest names the scenes, and
  the editor opens the first.
- A scene document is a database with an application id, a schema version, an `items` table of
  typed core columns and a JSON payload, and a `buffers` table of blobs. A mesh's topology is
  three blobs.
- Every item has a document id, assigned by the scene and never reused within a document.
- A save writes the whole scene in one transaction. A read is all or nothing, and refuses a
  newer schema, a newer record type, an unknown kind and a database another program wrote.
- Open picks a project's manifest, and Save As makes a new project directory.
- Tests: a scene of every kind round-trips, in order and with its ids. A malformed light, a
  newer type, a newer schema and a foreign database each fail the whole read and leave the scene
  alone. A failed save leaves the last save whole. The manifest round-trips.
- What landed matches the list above. A payload must hold exactly the fields its version
  defines, so any change to what a kind stores is a new version of the kind. An older build
  then refuses a newer document rather than dropping what it cannot read. The store writes each mesh
  item's geometry in full, so two items that share a `BRep` read back with a copy each. Nothing
  makes such items yet, and TODO.md holds the work.

### Step 3 — Cameras and lights drawn

**Closed.** A camera item draws as a frustum glyph and a light item as a glyph by its type, in
lines, through the existing line canvas. A light is a point, distant or spot light, matching the
standard RI lights moya reads.

- `src/scene/Glyphs.h` describes each glyph in its item's own space, one unit long along +z.
  `WireframeVisitor` draws them through the item's translation and rotation, and takes which
  kinds to draw from the view's flags.
- `SHOW_CAMERA` and `SHOW_LIGHT` start on. Their toggles, `view::show::camera` and
  `view::show::light`, landed here rather than in step 4, because once the glyphs draw a toggle
  changes what is drawn.
- Tests: each glyph's shape for every projection and light type, its placement without the
  item's scale, the colour of a selected item, and each flag leaving its kind out.

### Step 4 — Cameras and lights in the scene

**Closed.** `create::ortho::camera` and `create::perspective::camera` add a camera at the view's
eye, facing the way it looks, through the command stack. Create > Light gives `light::point`,
`light::distant` and `light::spot`, which add a light the same way.

- `SelectMask` gained `Mesh`, `Light` and `Camera`. Object takes every kind, and each of the
  three takes only its own. The picker hits a camera or a light by its glyph, by screen distance,
  and a band holds one when its whole glyph is inside. A target under one of the three moves only
  its kind.
- The Curve and Handle masks stay in the menu with no handler, and steps 13 and 14 build them.
- The RIB export writes each light as a `LightSource` of RI's standard shader, before every mesh.
  It renders from the selected camera when exactly one is selected, and from the active view
  otherwise.
- `Camera::Lens::valid()` and `Light::Emission::valid()` state the ranges once, for the store and
  for making a camera from a view.
- Tests: create, undo and redo for each kind. The mask decides what a pick and a band return, and
  what a target moves. The export writes every light, before the meshes. A camera made from a view
  is seen through the same view again, in either hand.

### Step 5 — Camera Manipulators

**Closed.** The `camera::*` items latch a camera move on the left button, so a mouse with no
modifier can truck, dolly, zoom, pan and pitch. `CameraControlTool` gained dolly and pitch. On a
perspective view zoom changes the field of view and dolly moves the camera.

- The tool keeps a held move and a latched one. The held move wins while it is held, and the
  latch lasts until its item is chosen again or a transform mode is chosen. The menu items are
  checks, and show the latch.
- Left Alt was bound to `zoom`, which dollied a perspective view. Zoom now changes the angle of
  view, so Left Alt is bound to the new held `view::camera::dolly`, and does what it did.
- An orthographic dolly zooms, and an orthographic pitch does nothing, as an orthographic pan
  does.
- Tests: the latch and the held move together, zoom and dolly in each projection, and pitch.

### Step 6 — Shaded views

**Closed.** A mesh is triangulated by fanning each face loop, with a flat normal from the face or a
smooth normal averaged at each vertex, and its winding reversed to counter-clockwise. A shaded
pass per view draws the triangles before the lines and shares the depth buffer.

- The renderer re-uploads a mesh only when step 11's revision has moved.
- The pass is the editor's own: a simple lit shader with pipeline variants for the cull and
  polygon modes. Lit's cel shading and fixed modes do not fit a modeller's views.
- `view::shade::*`, `view::cull::*` and `view::rendermode::*` set view state, and each view keeps
  its own.
- Tests: the triangulation of a quad, a pentagon and a concave face; normals flat and smooth;
  the revision bumps on each command. A validation log that stays silent in every mode.
- What landed differs in four ways. Faces are cut by ear clipping, not fanned, because a fan
  covers area outside a concave face. The surfaces go into each view's scene pass ahead of its
  lines rather than into a pass of their own, which shares the pass's clear and its depth
  exactly. The device turns on `fillModeNonSolid` where it is offered, and a view on a device
  without it draws a points or lines side filled. A new suite, `vertical3d_device`, draws the
  pass on a headless device. It checks which side is the front under the editor's camera, that
  every fill and shading draws without a validation message, and that an upload is made again
  only when the geometry changes. The editor started with validation on and logged nothing.

### Step 7 — Import

**Open.** ADR-0122 decided it: glTF and USD, each read as a scene of items.
LightWave, DXF, 3DS and MilkShape leave the Import menu, and `project::import::gltf` and
`project::import::usd` replace them.

- `api/brep` gains a builder from polygons over an indexed point list, beside its primitives
  and its triangulation. It welds positions through a hash and pairs edges through a map, so it
  is linear in the size of the input. For triangles it can join coplanar neighbours back into
  faces, which a glTF import asks for. Each face's loop is reversed to the editor's clockwise
  winding.
- A glTF import walks the file's nodes itself, through cgltf, rather than the game loader's
  merged `model::Model`. Each node becomes a group, a mesh, a camera or a light
  (`KHR_lights_punctual`), parented and placed as the file places it. Its animation becomes
  keys on the items' properties.
- A USD import walks the stage's prims the same way. `Xform` becomes a group, `Mesh` a mesh with
  its polygons kept, `Camera` a camera, and `SphereLight`, `DistantLight` and a shaped
  `SphereLight` a point, distant or spot light. Time samples become keys. OpenUSD enters the
  vcpkg manifest without its imaging feature, and the build places its plugin files beside the
  editor.
- An import is one operation on `Document`, so it undoes in one step. The command table supplies
  the file, as it supplies every other operation's arguments. A file that cannot be read, or a
  node that cannot be made into an item, is reported in the operation's outcome.
- Tests: a cube in triangles builds six quads. A non-manifold input is refused with a reason. A
  large mesh builds in time linear in its size. A glTF fixture and a USD fixture, each written
  by Blender with a parented mesh, a camera, a light and a keyed move, import as those items,
  parents, placements and keys, and undo in one step.
- The builder has landed, as `brep::build()`. It differs in one way. It joins pairs of
  triangles into quads, across an edge that is the longest of both, rather than every coplanar
  neighbour. Joining a whole flat region would turn a grid of squares into one face.

### Step 8 — The Render menu

**Closed.** ADR-0106 decided it, after looking at how Blender, Houdini, Katana, Arnold and
RenderMan join a renderer to an editor. It weighed linking moya for everything and running
`moya.exe` for everything against an editor-owned renderer interface with a batch path beside it.

- `render::view` renders the active view, or the chosen scene camera, and shows the picture in a
  window of the editor's ui. The render runs off the main thread, so the editor stays live.
- `render::settings` sets the picture size, the pixel samples and the hider.
- `render::batch` renders each scene camera to a file.
- The export gains a `Display` line and a default light when the scene has none.
- What landed differs in five ways.
  - Render View goes through `Delegate`, an interface the editor owns, which also brings a render
    up to date as the scene changes. While the window is open an edit renders again.
  - moya gained a progress callback that can stop a render, per bucket and per ray-traced row.
    Its pictures still arrive only when a frame finishes, which TODO.md records.
  - `render::batch` renders every frame of the timeline's range, not one frame, from each
    camera, through `moya.exe`, which the build copies beside the editor with its libraries.
  - The export writes `Surface "matte"`, because RI's default surface ignores lights, so the
    lights step 4 exported had no effect on a render.
  - The render settings are saved in the document, and a change to them goes through the
    history.

### Step 9 — Groups

**Closed.** ADR-0104 decided it, and amends ADR-0099. It weighed a parent link on each item, the
hierarchy in `api/dag`, and a general graph with parenting as one kind of connection, against the
scene holding the links and evaluating every placement. It also weighed a child that does not
inherit its parent's scale, and an item with several parents.

- An item may have a parent. Its world matrix composes its parents' matrices, and picking, the
  manipulators and the export use the world matrix.
- `create::group` groups the selection under a new group item, through the command stack.
- The RIB export nests `AttributeBegin` blocks.
- Tests: a group moves its children. Undo restores the parents. A grouped project round-trips.
- What landed differs in five ways.
  - Any item can be a parent, and a group is a fourth kind that holds nothing of its own. It
    is drawn and picked as a small box, since it has no geometry to click.
  - The scene document stores each parent by document id, in a `parent` column, which is schema
    version 2. A version 1 document reads with no parents, and its first write migrates it.
  - The scene holds the links, and `Scene::evaluate()` gives every reader its placement. A
    visitor is handed each item's evaluated placement, and the walk is depth first.
  - Removing an item removes everything beneath it.
  - A light is written at world level rather than inside its parent's block, because RI turns
    a light off when its block ends.

### Step 10 — Playback

**Closed.** ADR-0105 decided it. It weighed a track of whole values per property, a key that
snapshots the whole placement, placement alone, and keys in seconds against a scalar curve per
numeric property, keyed on frames.

- The editor gains `simulate()`, which advances the current time while playing.
- `timeline::set` keys the selection's transform at the current time, through the command stack.
- `timeline::playback::*` play, stop and step between frames and keys.
- The export writes the current frame. `render::batch` can render a frame range.
- Tests: a key sets and undoes. A track samples between two keys. A keyed project round-trips.
- What landed differs in five ways.
  - A key is a `Curve` in `api/type/animation`, not a `Track`, because a track lerps whole
    values and has no slopes. Set Key keys every property an item declares: its placement, and
    a camera's lens or a light's emission.
  - Rotation is keyed as Euler angles in one fixed order. A per-item order is in TODO.md.
  - The scene is posed by writing the curves' values into the items when the frame changes.
    An edit holds until the next pose, and the export writes the posed scene.
  - The scene document is schema version 3, with a `curves` table and the timeline in its meta
    table. Older documents read with no curves, and their first write migrates them.
  - `render::batch` is part of step 8, and has nothing to render a range with until then.

### Step 11 — Incremental save

**Closed.** An item carries a revision that every change to it moves. The store remembers the
revision it last wrote for each item, and a save writes only the rows of items that changed,
were added or were removed.

- `dag::Transform` and `brep::BRep` count their changes. A mesh counts an added part and a
  restore, so every operation counts. A part changed in place needs `touch()`, which the
  component move and the vertex command call. `Item::revision()` sums the counts.
- The store remembers the items of the one document it last wrote or read. An item that only
  changed place has its `position` updated. A write to any other document, or the first after a
  failed write, is written whole.
- Tests: the counts for a transform, a mesh and each kind. A save that changes nothing writes
  nothing, and a move writes one row. A removal deletes one row and moves the rest, and a redone
  item is written again. A document read and then saved writes only its change. Another document
  is written whole, and so is the first write after a failed one.

### Step 12 — Autosave

**Closed.** On a timer, the editor writes the changes since the last autosave into a recovery
database in the project's `autosave` directory, never into the document. Opening a project whose
recovery database is newer than its scene offers to recover it. A save or a clean close removes
the recovery database.

- The timer runs in `Controller::tick()`, every 60 seconds while the history is dirty. It is wall
  time rather than simulation, so it is not in `simulate()`.
- Recovery is also offered at startup for the default project, which the editor starts on
  without reading. A crash in a project never saved is otherwise lost. Discard at the prompt
  that asks before discarding removes the recovery database as well, because those changes were
  thrown away on purpose.
- A recovered scene leaves the history dirty, through the new `CommandStack::markUnsaved()`.
- Tests: an autosave leaves the document as it was, and the second writes only its change.
  Recovery reads back the autosaved scene. A recovery database older than the document is not
  offered, one with no document is, and a save removes it.

### Step 13 — Curves

**Open.** ADR first, on what a curve is: its form (polyline, Bezier, B-spline or NURBS),
whether it is a kind of item of its own, and whether moya renders one. moya's `RiCurves` has an
empty body, so a curve that is only exported shows in neither Render View nor a batch. The
menu's `select::mask::curve` waits on it.

- A curve kind of item. `SceneVisitor` gains a `visit` for it, which every visitor implements,
  and the wireframe draws it in lines from its evaluated placement. A new Create item makes one
  through an operation on `Document`.
- The store saves a curve through a new `Codec` in `store/`.
- The export sends a curve as RI `Curves` through `rib::Handler`, which gains a `curves`
  request. `rib::Writer` writes it and `rib::Recorder` replays it. If the ADR has moya render
  curves, moya's handler draws them as well.
- `SelectMask::Curve` picks a curve by screen distance, as a glyph is picked.
- Tests: a curve draws, picks, saves and exports, and the writer writes a `Curves` request that
  reads back the same.

### Step 14 — Handles

**Open.** ADR first, on what a handle is. Nothing in the editor defines one: a manipulator's
handles are drawn by the renderer, are not items, and cannot be selected. The candidates are a
group's selection handle, which step 9's groups make possible, and an IK handle, which needs a
skeleton in the scene. The menu's
`select::mask::handle` waits on it.

- Whatever the ADR decides is built as an item kind or a part of one, with the handle mask
  picking it.

## Outcome

Written when the plan closes.
