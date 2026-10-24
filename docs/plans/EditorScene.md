# Editor Scene — Cameras, Lights, Groups, Shading, Import, Render and Playback

Drafted 2026-10-24 against `ab4bdea1`. This plan takes the six editor groups in
[TODO.md](../TODO.md#editor). When it closes, every one of the menu's 80 commands has a handler.
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
another directory is named.

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
  `type::Model` (`api/render/realtime/MeshRegistry.h:177`) and `vulkan::renderer::Lit` draws it
  (`vulkan/renderer/Lit.h:49`). Lit is cel shading with fixed cull and polygon modes, and needs
  an sRGB target. `pipeline::Builder` can set both modes (`vulkan/pipeline/Builder.h:92,98`).
  `Engine3D::context()` gives an app the device context to build its own pass.
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
| Select masks | `select::mask::curve`, `mesh`, `light`, `camera`, `handle` | 4 |
| Scene cameras and groups | `create::ortho::camera`, `create::perspective::camera` (step 4), `create::group` (step 9) | 4, 9 |
| Camera Manipulators | `camera::truck`, `dolly`, `zoom`, `pan`, `pitch` | 5 |
| Shading | `view::shade::flat`, `smooth`; `view::cull::none`, `front`, `back`, `both`; `view::rendermode::front::*`, `back::*` (points, lines, fill) | 6 |
| Import | `project::import::lwo`, `dxf`, `3ds`, `milkshape` | 7 |
| Render | `render::settings`, `render::view`, `render::batch` | 8 |
| Timeline | `timeline::set`, `timeline::playback::*` (ten) | 10 |

The curve mask has no curves to select. Step 4 decides whether the item goes, or waits on a
curve kind the plan does not build.

## Decisions

| Decision | Step |
|---|---|
| Editor: what a scene item is, and how the scene holds kinds other than meshes. Amends ADR-0013. | 1 |
| Editor: where hierarchy lives, in `api/dag` or in the editor, and how a world matrix is composed. Amends ADR-0013. | 9 |
| Editor: how the editor renders with moya, by linking `v3dlib_moya` or by running `moya.exe`. | 8 |
| Editor: which import formats the menu offers. glTF has a reader. The four legacy formats each need one written. | 7 |
| Editor: what the editor animates, and how a key is stored. | 10 |

## What blocks what

- **Step 1 blocks steps 2, 3, 4 and 9.** Every kind other than a mesh needs a scene item to be.
- **Step 2 goes with step 1.** A scene that holds a camera has to save one.
- **Step 3 comes before step 4's show toggles**, so that a toggle changes what is drawn.
- **Step 7 needs the builder in step 7's first half** before any importer can make a mesh.
- **Step 8 works on today's export.** It renders better after step 4, which gives the export
  lights and a scene camera to render from.
- **Step 9 needs step 1, and touches picking, the manipulators, the export and the file.**
- **Step 10 needs its ADR, and is best last**, so that a group and a camera can be keyed.
- **Step 5 needs [Hardening](Hardening.md) step 6.** Step 6 is independent of everything else.

## Steps

### Step 1 — A scene of items

**Open.** ADR first. A scene item is a `dag::Node` and a `dag::Transform`, and a mesh is one
kind of item. The ADR weighs a common base class with a kind, against a variant of the kinds.
It also says how a visitor reaches each kind.

- `Scene` holds items. `selection()`, `mesh()`, `add()` and `remove()` work on items, with a
  typed lookup for a mesh.
- `Picker`, `Target`, `CreateCommand` and `RIBExportVisitor` take items, and handle only the
  meshes until step 4 gives them other kinds.
- No visible change. The editor suite's existing cases pass unchanged.

### Step 2 — Project version 2

**Open.** `Project::VERSION` becomes 2, with the first entry in the migration chain: a version 1
file reads as a version 2 file with no cameras, lights or groups. The new sections are written
even when empty. A read stays all or nothing for the new kinds.

- Tests: a version 1 file reads under version 2. A file with a camera and a light round-trips.
  A malformed light fails the whole read.

### Step 3 — Cameras and lights drawn

**Open.** A camera item draws as a frustum glyph and a light item as a glyph by its type, in
lines, through the existing line canvas. A light is a point, distant or spot light, matching the
standard RI lights moya reads.

### Step 4 — Cameras and lights in the scene

**Open.** `create::ortho::camera` and `create::perspective::camera` add a camera at the view's
position, through the command stack. A light is created the same way, from a new create item.
`view::show::camera` and `view::show::light` toggle the existing flags.

- `SelectMask` gains `Mesh`, `Light`, `Camera` and `Handle`, and the picker honours them. Decide
  whether `curve` is removed from the menu.
- The RIB export writes each light as a `LightSource`, and a chosen scene camera as the camera.
- Tests: create, undo and redo for each kind. The mask decides what a pick returns. The export
  writes a light.

### Step 5 — Camera Manipulators

**Open.** The `camera::*` items latch a camera move on the left button, so a mouse with no
modifier can truck, dolly, zoom, pan and pitch. `CameraControlTool` gains dolly and pitch. On a
perspective view zoom changes the field of view and dolly moves the camera.

- Needs [Hardening](Hardening.md) step 6, which makes `CameraControlTool::deactivate` reset only
  the mode it was given. A latched move and a held move would otherwise cancel each other.

### Step 6 — Shaded views

**Open.** A mesh is triangulated by fanning each face loop, with a flat normal from the face or a
smooth normal averaged at each vertex, and its winding reversed to counter-clockwise. A shaded
pass per view draws the triangles before the lines and shares the depth buffer.

- A mesh carries a revision that every edit bumps, so the renderer re-uploads only what changed.
- The pass is the editor's own: a simple lit shader with pipeline variants for the cull and
  polygon modes. Lit's cel shading and fixed modes do not fit a modeller's views.
- `view::shade::*`, `view::cull::*` and `view::rendermode::*` set view state, and each view keeps
  its own.
- Tests: the triangulation of a quad, a pentagon and a concave face; normals flat and smooth;
  the revision bumps on each command. A validation log that stays silent in every mode.

### Step 7 — Import

**Open.** ADR first, on the formats.

- `brep` gains a builder from an indexed triangle list. It welds positions through a hash, pairs
  edges through a map, and joins coplanar triangles that share an edge back into faces. The
  builder is linear in the triangle count.
- An importer reads through `api/asset`, builds meshes, and adds them through one command, so an
  import undoes in one step.
- Tests: a cube in triangles builds six quads. A non-manifold input is refused with a reason. A
  large mesh builds in time linear in its size.

### Step 8 — The Render menu

**Open.** ADR first, on how the editor reaches moya.

- `render::view` renders the active view, or the chosen scene camera, and shows the picture in a
  window of the editor's ui. The render runs off the main thread, so the editor stays live.
- `render::settings` sets the picture size, the pixel samples and the hider.
- `render::batch` renders each scene camera to a file.
- The export gains a `Display` line and a default light when the scene has none.

### Step 9 — Groups

**Open.** ADR first, on hierarchy. An item may have a parent. Its world matrix composes its
parents' matrices, and picking, the manipulators and the export use the world matrix.

- `create::group` groups the selection under a new group item, through the command stack.
- The project file stores the hierarchy by array index, since ids are not saved.
- The RIB export nests `AttributeBegin` blocks.
- Tests: a group moves its children. Undo restores the parents. A grouped project round-trips.

### Step 10 — Playback

**Open.** ADR first, on what the editor animates. The proposal is the transform of any item,
keyed per channel with `Track`, and stored in the project.

- The editor gains `simulate()`, which advances the current time while playing.
- `timeline::set` keys the selection's transform at the current time, through the command stack.
- `timeline::playback::*` play, stop and step between frames and keys.
- The export writes the current frame. `render::batch` can render a frame range.
- Tests: a key sets and undoes. A track samples between two keys. A keyed project round-trips.

## Outcome

Written when the plan closes.
