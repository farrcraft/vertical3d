# Reshaping — Boundaries, Decomposition, Demos and Documents

Drafted 2026-10-25 against `2d33df79`. The tree was read whole, area by area, to find what has
grown out of shape. At file and function level it is healthy. Few functions pass 100 lines, the
libraries have no cycles, and the suites are thorough. The debt is one level up. Some library
boundaries fall in the wrong place, a few classes hold several jobs, and two mechanisms are built
twice. The reference documents name things correctly, but a dozen statements of behaviour are
stale and several facts are written in more than one place.

When this plan closes, `RenderContext` is a facade over classes with one job each. The game
engine builds without Vulkan. The editor has a document layer that menus, bindings and a console
can all call. The four games sit in `demos/`, and each reference fact has one home.

The open plan [EditorScene](EditorScene.md) also changes the editor. Steps 13 to 16 here touch
the same files. Neither plan waits for the other; whichever step lands second is written against
the tree the first one left.

## What the survey found

Line numbers are from `2d33df79`.

**Defects:**

* **Five RI tokens are null.** `RI_RASTER`, `RI_SCREEN`, `RI_CAMERA`, `RI_WORLD` and `RI_OBJECT`
  are declared without a value (`moya/libmoya/RenderMan.cxx:141-145`). `RiCoordinateSystem` and
  `RiCoordSysTransform` build a `std::string` from the token (`RenderMan.cxx:684-690`), so
  `RiCoordSysTransform(RI_WORLD)` is undefined behaviour.
* **A scene can overwrite the camera.** The predefined spaces live in the same map as the
  systems a scene names (`moya/libmoya/RenderContext.cxx:84-91,163`). `CoordinateSystem
  "raster"` replaces the matrix the reyes buckets read (`reyes/Bucket.cxx:337`).
* **ScaleAndReach left two items out of TODO.md.** Its Outcome lists the FMOD placement's
  missing CI gate and the traced ray cast for every hit of a run. Neither is in TODO.md.

**moya and the offline renderer:**

* **`RenderContext` holds seven jobs** in 1242 + 631 lines. It holds the RI options, the camera
  and projection rules, the transform and attribute stacks, the light registry, the reyes front
  end, the traced scene and the output stage.
* **Control runs backwards.** `reyes::Hider::render()` calls `RenderContext::bucket()`, and
  `reyes::Primitive::split` re-enters `RenderContext::addPolygon` (`Polygon.cxx:435`).
* **The camera exists three times:** in `RenderContext`, as `RayHider::Camera`, and as
  `trace::Scene::view()`. The raster product is rebuilt in `Bucket.cxx:337`,
  `RayHider.cxx:205-208` and `GridShader.cxx:75-90`.
* **`attributeBegin` and `attributeEnd` copy eleven fields one by one**
  (`RenderContext.cxx:380-415`). An attribute missed there is silently not restored.
* **Two RI front ends apply RI's rules, and they disagree.** `RiOption`, `RiSphere` and
  `RiPointsPolygons` are empty in `RenderMan.cxx`, while `RIBHandler` implements all three.
* **`GridShader` and `HitShader` each build the shader's space table** (`GridShader.cxx:60-102`,
  `HitShader.cxx:48-90`). Neither answers a space the scene named.
* **The two largest test files mirror `RenderContext`.** `ReferenceTest.cxx` is 1506 lines and
  `RIBHandlerTest.cxx` 1238.

**The game engine and the core libraries:**

* **`engine` needs Vulkan to open a window.** `api/engine/Engine.h:15` includes
  `api/render/realtime/Window.h`, which includes the Vulkan device headers. An app with no window
  still needs the Vulkan SDK, and imagetool cannot use `run<T>`.
* **The ui shell is wired by hand in each app.** pong, tetris, voxel and the editor each build a
  `Screen`, declare a `GameMenu` and toggle the overlay.
* **Rebound keys are saved by pong's code.** `event::Bindings::overrides()` and `restore()` have
  no caller outside tests. pong stores one setting per key (`pong/src/PongEngine.cxx:213`).
* **`ecs::snapshot<T>()` has to be called by hand** at the top of each `simulate()`.
* **`type` holds behaviour as well as types:** particles, weather, cameras and animation.
* **`config` is four unrelated things.** It holds a document registry, a camera profile reader,
  the sprite sheet description and `windowSize`. `config/Type.h` is a closed enum of other
  libraries' document kinds.
* **`dag` is no longer a graph.** Since ADR-0104 it is an id counter and a transform with a
  change count, and the editor is its only consumer.
* **`brep` stores the editor's selection** in `Vertex`, `HalfEdge` and `Face`.
* **`asset::Type` is closed,** so the loader registry cannot be extended without editing `asset`.
  Its four image kinds have no effect, because the decoder is chosen by extension
  (`api/asset/media/loader/Image.cpp:31`).
* **`font` is still a freetype-gl port.** It takes `wchar_t`, so `ui::paint::TextRenderer`
  decodes UTF-8 and measures text itself (`TextRenderer.cpp:106-188`). `Canvas::text` takes a
  `font::TextBuffer`, so `render` depends on `font`.
* **Image writers bypass atomic writes.** `image::Writer::write` streams to the target path.
* **Five classes are named `Engine`.**

**The realtime renderer:**

* **A lit scene has no composition object.** An app wires three targets, `MeshRegistry`, `Lit`,
  `Grade` and three passes by hand (`api/render/tests/device/LitSceneTest.cpp:629-750`). The
  order rules exist only as prose and runtime throws.
* **Renderers are built three ways:** from a `DeviceContext`, from seven shared pointers, or from
  a `Spec`.
* **`realtime/` and `vulkan/` include each other.** An app has to name `vulkan::` for everyday
  types such as `vulkan::frame::RenderTarget`.
* **Streaming is implemented twice.** `Textures::poll` and `MeshRegistry::poll` run the same
  budgeted loop with different failure policies.
* **Release does not follow one rule.** `Buffer::grow` and `DepthBuffer::recreate` call
  `vkDeviceWaitIdle` (`vulkan/memory/Buffer.cxx:104`, `vulkan/frame/DepthBuffer.cxx:141`).
* **`render::realtime::Engine` is vestigial.** It has one subclass, and nothing calls its
  `assetManager()`.

**The ui:**

* **`Immediate` holds eight sub-systems** in 1276 + 752 lines. About 280 lines of its header are
  implementation structs.
* **The two ui models style the same things with different keys.** Each has its own `Dressing`,
  and default colours are split between `paint/Palette.h` and `Immediate.cpp:93-96`.
* **The active theme is held in three places.** `ui::Engine`, `ComponentRenderer` and
  `Immediate` each hold one.
* **Theme `fonts` and `align` are parsed and never applied.**
* **`Loader.cpp` holds two parsers with two error policies.**

**The editor:**

* **`Controller` is the editor.** It owns the scene, history, project, autosave, playback,
  render state, tools and views (`vertical3d/src/Controller.h:368-425`).
* **Posing writes into the model.** `Scene::pose` writes curve samples into the items
  (`src/scene/Scene.cxx:257-269`). `Batch` poses the live document through every frame.
* **`scene/` holds four jobs,** and the editor's directories depend on each other in cycles:
  scene and command, view and manipulator.
* **Commands take no arguments.** What a command acts on depends on the selection, the select
  tool's mask and the pane under the mouse.
* **Live render writes RIB text and parses it back** (`src/render/MoyaDelegate.cxx:72-96`).
* **Every source is listed twice,** in `vertical3d/CMakeLists.txt` and `tests/CMakeLists.txt`.
* **`SceneStore.cxx` is 1268 lines.** Adding an item kind touches three places in it.

**The games and tools:**

* **The four games follow the shell.** Each `main` is one `run<T>` call.
* **odyssey has drifted.** It has no `GameMenu` and no overlay, and Escape quits. It keeps
  empty stubs (`ui/Screen.h`, `actor/Actor.h`, `engine/Sprite.h`, `engine/SpriteSheet.h`). Its
  layout has no `src/`, and its target is `Odyssey`.
* **No demo shows lit rendering,** shadows, glTF, a placed voice or `IsometricController`.
* **voxel's camera duplicates `type::camera::Camera`** (`voxel/src/engine/Camera.cxx`).
* **imagetool's sheet merge is document logic** with no test (`imagetool/imagetool.cxx`).
* **v3dshell is an empty `main`,** unchanged since the first commit.

**The documents:**

* **Stale statements:**
  - `api/engine/Lifecycle.md:112` says the loop ticks after a quit handler. It breaks first.
  - `api/rendering/README.md:80` says every upload waits for the GPU. None does.
  - `api/rendering/ColourAndPost.md:56-58` says `Grade::replace` waits for an idle queue.
  - `offline/README.md:47,112-114` says traced hits are shaded one at a time, and links a dead
    TODO.md anchor.
  - `api/engine/Config.md:14-40` gives odyssey's document type as `map`. It is `world`.
  - `api/ui/ImmediateMode.md` shows `window()` and `table()` with signatures that do not compile.
  - `editor/README.md:55-66` lists sources by class, and is missing `src/render/` and ten more.
  - `../README.md` is missing `database`, and imagetool's pack and unpack.
  - `Games.md:44` says the odyssey suite covers sight. Sight moved to `api/grid`.
  - `api/README.md` describes audio as SDL3_mixer only.
  - ADR-0047 says only MSVC checks exhaustive switches. Clang builds with `-Wswitch` now.
* **Facts written in several places:** the library table (2), the vcpkg warning (4), the
  configure command (3), the starter build (2, already different), and `v3d.log` beside the
  executable (8).
* **Three pages cover too many subjects:** `api/Types.md` (474 lines), `api/Assets.md` (472) and
  `internals/UserInterface.md` (441).
* **`roadmap/README.md` links TODO.md sections that no longer exist.**

## Decisions

* **The games move to `demos/`, built by the root project.** Under the `examples/` definition
  they would leave ctest, failsfirst, `/W4 /WX`, `/analyze`, clang-tidy and the link rule. They
  would also need four vcpkg manifests. An in-tree directory under the `examples/` name would
  give that name two meanings. `examples/` stays for programs that consume the api from outside.
* **All four games stay,** tetris included.
* **imagetool stays a top-level tool.** It does real work, beside moya.
* **v3dshell is deleted.** The idea it stood for is kept, as a console inside the editor. It
  needs commands that take arguments and name what they act on, which step 13 builds.
* **Completed plans, roadmaps and audits are frozen records.** [sdlc.md](../sdlc.md) states the
  rule. Their dead links are not defects.

These need an ADR when their step starts:

* where the window lives, once `engine` no longer depends on `render` (step 5)
* what replaces `asset::Type` as a loader's key (step 9)
* what `config` becomes, and where each part goes (step 9)
* how a frame is evaluated without writing into the items (step 14)

## What blocks what

```
1 defects and stale docs ──────────────────────────────────────────── (none)
2 offline::Camera ── 3 RenderContext split ── 4 RenderMan as adapter
                                                    └── 16 live render through rib::Handler
5 engine without Vulkan ── 6 engine and shell joined
7 realtime vocabulary ── 8 lit scene composition ── 19 a lit demo
9 core library boundaries ── 10 font and image
11 ui styling ── 12 Immediate and Loader split
13 editor document layer ── 14 evaluated animation
                         └── 15 editor directories and brep selection
17 demos/ ── 18 odyssey on the shell ── 19 a lit demo
20 documents ─────────────────────────────────────────────────────── (after 1)
```

Steps on different lines do not block each other.

## Steps

### Step 1 — Defects and stale statements

**Closed.**

- Every RI token holds RI's string, and the five basis matrices hold RI's values. Every
  implemented entry point already ignored a null name except `RiCoordinateSystem` and
  `RiCoordSysTransform`, and those two now do too. One guard for every entry point comes with
  the adapter in step 4.
- The predefined spaces are kept apart from the ones a scene names. A predefined name from a
  scene is logged and refused. `"camera"` was exposed as well as `"raster"`, because every
  placement reads it.
- The traced rays of hits that did not trace are a TODO.md entry. The FMOD backend's missing CI
  coverage is a limitation, not work, so Testing.md states it instead.
- `v3dshell/` is deleted. TODO.md has an editor entry for a console over the operation layer.
- Every stale statement in the survey is corrected, ADR-0047 included. The ones found on the
  way are corrected too:
  - the renderer constructors in `Renderers.md`
  - `engine::Engine::alpha()`, named from inside the renderer
  - two `Controller.h` comments, and `CONTRIBUTING.md`'s plain `ctest`
  - the closed `asset::Type`, and theme fonts and `align`, which nothing draws with
- `roadmap/README.md` links only TODO.md sections that exist.
- Five new cases, each failing first.

### Step 2 — A shared offline camera

**Closed.**

- The camera is two classes in `api/render/offline`. `CameraOptions` holds RI's camera
  options and their defaulting rules. At WorldBegin it freezes into a `Camera`: a value holding
  the view and the matrices from camera space into screen, raster and NDC. A test can fill a
  `Camera` in directly, as the trace suites do.
- `Camera` replaces `RayHider::Camera`, `trace::Scene::Projection` and the scene's view. Every
  `raster * screen` product is gone, because `"raster"` is camera to raster everywhere now.
  `"NDC"` in the context was the identity, and is the camera's now.
- The traced scene holds the camera and the spaces the scene named, and answers any of them
  from world space or from camera space. `GridShader` and `HitShader` answer only current,
  shader and object space themselves. Asking from camera space keeps the grid's answers
  bit for bit, so the moya references are unchanged.
- A scene's named space reaches the traced scene when it is saved. `ptransform("lamp", P)`
  answers under both hiders.
- Five new cases. The three in `CameraTest` fail to build without the change, and the named
  space and NDC cases fail.

### Step 3 — RenderContext as a facade

**Open.** Needs step 2.

- `Transforms`, `Attributes`, `Lights`, `Shaders`, `TracedScene`, `Display` and `Progress` in
  libmoya, each with one job.
- `Attributes` is a value and a stack, so an attribute is saved and restored in one place.
- The cull, bound and bucket body of `addPolygon` moves into `reyes::Hider`. A split primitive
  re-enters the hider.
- `moya::FrameBuffer` becomes `reyes::Buckets`, owned by the reyes hider.
- `RenderContext` keeps one method per RI request and the world lifecycle.
- The editor's surface stays: `progress`, `stopped`, and the picture.
- `ReferenceTest` and `RIBHandlerTest` split along the new classes, with a shared fixture.

### Step 4 — RI's C interface as an adapter

**Open.** Needs step 3.

- `RenderMan.cxx` turns each call into a `rib::Handler` request, so each RI rule lives in
  `RIBHandler` alone.
- `RIBHandler` no longer needs a `Renderer`. The notice is printed once per process, not per
  render.

### Step 5 — The engine without Vulkan

**Open.** An ADR decides where the window lives.

- `engine` owns an SDL window, and `render` creates its surface from it.
- `engine` depends on SDL3, not on `render`.
- imagetool can use `run<T>`, `Settings` and `userPath`.

### Step 6 — The engine and the shell joined

**Open.** Needs step 5.

- A layer above `engine` and `ui::shell` builds the screen, the game menu and the overlay. Each
  app's hand wiring goes.
- The engine saves and restores `Bindings::overrides()` through `Settings`. pong's per-key
  settings go.
- The engine takes the ECS snapshot before each step, for the types an app registers.

### Step 7 — The realtime vocabulary

**Open.**

- `render::realtime::Engine` folds into its subclass, and both are named for what they are.
- `vulkan/` holds only the raw wrappers. Targets, renderers and `Textures` move to a level an
  app names without `vulkan::`.
- Every renderer takes the device context and a spec struct. `RenderTarget` takes a spec.
- One streaming component serves `Textures` and `MeshRegistry`, with one failure policy.
- Release follows one rule. `Buffer::grow` and `DepthBuffer::recreate` either retire through the
  ring, or the rule states the idle wait.
- The device tests gain a helper that records a frame and reads back a target.

### Step 8 — A lit scene

**Open.** Needs step 7.

- One object owns the shadow target, the cascades and the wiring of the three passes. The order
  rules are built into it.

### Step 9 — The core library boundaries

**Open.** An ADR decides `asset::Type`, and one decides `config`.

- `type` keeps geometry, transforms and value types. Models, skeletons and animation move to a
  library of their own. Effects move beside the systems that run them.
- `config` is dissolved into the libraries its parts belong to.
- `dag` folds into the editor's `Item`. `command/Placement` uses `type::Transform`.
- pong's components move from `ecs` into pong.
- A loader is keyed by an open identifier. Images are one kind.
- The secondary `Engine` classes are named for their roles.

### Step 10 — Font and image

**Open.** Needs step 9.

- `font` takes UTF-8 and measures and lays out text. `TextRenderer` stops decoding.
- `Canvas` takes quads or a vertex span, so `render` no longer depends on `font`.
- Image writers encode to bytes, and callers write through `asset::writeFile`.
- The asset image loader reads through `asset::readFile` and decodes from memory.

### Step 11 — One ui styling vocabulary

**Open.**

- The retained and immediate ui use one set of property names, one default palette and one
  theme reader. The values can still differ by class.
- The active theme is held in one place.
- Theme `fonts` and `align` either take effect or are removed from `Themes.md`.

### Step 12 — Immediate and Loader split

**Open.** Needs step 11.

- `Immediate` keeps its public surface in its header. Its internals move out, and its source
  splits into windows and scrolling, the text field, popups, and tabs and tables.
- `Loader.cpp` splits into a theme reader and a component reader, with one error policy.
- Their suites split to match.

### Step 13 — The editor's document layer

**Open.**

- A `Document` holds the scene, history, project, saved position, autosave and recovery.
- Each operation is a typed function on the document that applies and records itself. Menus and
  bindings call operations.
- Playback, the render window and input routing each get a small controller of their own.
- `Tool` becomes an active-tool router, so new tools have a slot.
- The editor's sources build into an internal library that the executable and the suite link.

### Step 14 — Evaluated animation

**Open.** Needs step 13. An ADR decides it.

- Posing a frame produces placements without changing the items.
- `Batch` and the render delegate render an evaluation, not a changed scene.

### Step 15 — The editor's directories

**Open.** Needs step 13.

- `scene/` holds the model. Storage and RIB export move to a directory of their own, and
  drawing moves to `view/`.
- Manipulators take a camera and a region, not a `ViewPort`. The cycles go.
- Component selection moves out of `api/brep` into the editor, keyed by item and index.
- `CreatePoly` and `Triangulation` move into `api/brep`. The RIB export triangulates n-gons.
- `Binder` and `Columns` move into `api/database`.
- `SceneStore` gets one codec per item kind.

### Step 16 — Live render through the handler

**Open.** Needs step 4.

- The RIB exporter emits to a `rib::Handler`. A text handler writes files, and moya's handler
  takes the calls directly.

### Step 17 — demos/

**Open.**

- pong, tetris, voxel and odyssey move to `demos/`, built by the root project.
- CI, the link rule, failsfirst, the cpplint exclusions and every path in the documents follow.
- `Games.md` becomes the demos' page, and says which api features each demo shows.

### Step 18 — odyssey on the shell

**Open.** Needs step 17.

- odyssey gets the game menu and the overlay, and its empty stubs go.
- Its layout and target name match the other demos.
- voxel's camera becomes a controller over `type::camera::Camera`.
- imagetool's sheet merge moves into the api and gets a suite.

### Step 19 — A lit demo

**Open.** Needs steps 8 and 17.

- A demo draws a lit scene with shadows from a glTF model, and plays a placed voice.

### Step 20 — The documents

**Open.**

- Each duplicated fact gets one home, and the other copies link to it.
- `Types.md`, `Assets.md` and `internals/UserInterface.md` split by subject.
- `editor/README.md` describes each directory instead of listing its classes.
- `contributing/` and `internals/` get an index page.
- The library table is checked against the manifest, or generated from it.
