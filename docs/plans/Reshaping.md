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
- Five new cases. The NDC case fails without the change. The other four fail to build, the
  named space case because its file also reads the new `camera()`.

### Step 3 — RenderContext as a facade

**Closed.** `RenderContext.cxx` went from 1242 lines to 636, and its header from 631 to 509, most
of which is the documentation of one method per RI request.

- `Transforms`, `Attributes`, `Lights`, `Shaders`, `TracedScene`, `Display` and `Progress` are
  in libmoya, each with one job. `docs/offline/README.md` lists them.
- `AttributeBegin` saves the whole `Attributes` value with the transformation, and
  `AttributeEnd` restores it.
- `reyes::Hider` does both reyes passes. A split piece goes back to the hider's `add()`, and no
  longer passes the context's check for a flat motion, which read the transformation current at
  render time rather than the piece's.
- The bucket grid is `reyes::Buckets`, owned by the reyes hider. The output planes are the
  context's `picture()`, indexed by `moya::Planes`. The editor reads `picture()`.
- **Different from the plan:** the bucket size, grid size, their pins and the motion cache stay
  options of the context. `RiHider` makes a new hider, which would otherwise lose them. The
  hider reads them from the context.
- **Different from the plan:** clearing the pixels outside the crop window is `Display`'s, not
  `offline::FrameBuffer`'s, since only `Display` reads the crop window.
- `ReferenceTest` is `Reference.h` and three files by subject. `RIBHandlerTest` is `Handling.h`
  and five files by subject. No case changed.

### Step 4 — RI's C interface as an adapter

**Closed.**

- Each entry point in `RenderMan.cxx` converts its arguments and calls the active
  `RIBHandler`. `RiOption`, `RiSphere` and `RiPointsPolygons` now work from C, through the
  handler's code for a file.
- One guard serves every entry point: RI_NULL becomes the empty name. `RIBHandler` ignores an
  empty shader, light or space name, from a file as well as from C.
- `RIBHandler` owns its context and the declarations the C interface types parameters by.
  `RenderContext` no longer holds declarations. `Renderer` is the C interface's stack of
  handlers, and `RiGetContext`'s handle is the handler. The notice is printed by the first
  handler a process makes.
- **Different from the plan:** `CoordinateSystem` and `CoordSysTransform` are new handler
  requests, and the reader reads them, because the rule for a predefined name has to live in
  the handler. A file can now name a space.
- **Different from the plan:** a deforming primitive in a motion block is held back by
  `RIBHandler` as well as by the reader, because the C interface has no reader in front of it.
- **Different from the plan:** `RiSphere` passes no parameter to the handler. RI ends its named
  arguments on a float, and a `va_list` cannot be started from one.
- Seven new cases. The C cases for the sphere, the mesh and the motion block fail without the
  change. The other four fail to build, because the handler's constructor and the handler
  interface both changed. The displacement case sets its search path through `RiOption` now.

### Step 5 — The engine without Vulkan

**Closed.** [ADR-0116](../adr/0116-platform-the-window-is-a-library-below-the-renderer.md)
decides it.

- **Different from the plan:** the window is a library of its own, `window`, below `engine`,
  `ui` and `render`, rather than a part of `engine`. `ui::shell::Keyboard` turns text input on
  and off on the window, and under the plan `ui` would have had to depend on `engine`.
- `engine` links `window` and SDL3, not `render`. A tool can run the engine with no window and
  no Vulkan SDK. imagetool does not use the engine today, and nothing here makes it.
- `Context3D` makes the surface for the window, against one Vulkan instance for the process
  that `sharedInstance()` returns. Each window made its own instance before.
- No new case. The window suite still presents to a real window through the shared instance
  with the validation layer silent, and pong and the editor start with no validation message.

### Step 6 — The engine and the shell joined

**Closed.** [ADR-0117](../adr/0117-apps-the-game-shell-is-an-object-an-app-holds.md) decides
where the shell lives.

- `api/shell` holds `shell::Shell`, an object an app holds above `engine` and `ui`. Its
  constructor declares the menu's commands and the statistics command, `start()` builds the ui
  and the menu, `present()` builds the screen over the app's renderer, and `release()` drops
  them before the renderer goes. pong, tetris, voxel and the editor use it, and their own
  wiring of the ui, the menu, the screen and the overlay is gone. Four new cases.
- **Different from the plan:** odyssey does not use it yet. It has no game menu or overlay to
  move, and step 18 gives it them on the shell.
- `run<T>` takes an `engine::Identity`, the organization and application a player's files are
  kept under, and hands it to the engine before it starts. The engine reads the player's
  `Settings` as it starts, restores the saved binding overrides after the binding config loads,
  and saves them after every change. `Bindings::changed()` reports a change, so one made
  through `bindings()` is saved too. pong's per-key settings are gone, so a player's earlier
  pong rebindings are not carried over. Three new cases.
- An app registers each type it draws between steps with `Engine::interpolate<T>()`. The
  loop calls `step()`, which snapshots each registered type and then calls `simulate()`.
  `engine` links `ecs`. pong registers its two position types. One new case.

### Step 7 — The realtime vocabulary

**Closed.**

- `render::realtime::Engine` is folded into its subclass. The renderer an app holds is
  `WindowRenderer`, and the context it owns is `WindowContext`. Neither takes the asset manager,
  which nothing read, so pong and voxel no longer link `v3dlib_asset`.
- `vulkan/` holds only the wrappers, and includes nothing above it. The renderers are in
  `renderer/`, and `RenderTarget`, `Recorder`, `Mesh`, `Resources` and `Material` are at the
  root. An app names `vulkan::` only to build a pipeline or a buffer of its own.
- **Different from the plan:** the swapchain takes the present modes to try, best first, and
  reports the one it got. `Present.h` maps a request to that list and back, because the
  swapchain included it.
- Every renderer, `Grade` and `RenderTarget` takes `(const DeviceContext&, Spec)` and copies
  out what it uses. None holds the context, which owns the `Quad`, `Line` and `World` it builds.
  **Different from the plan:** `RenderTarget` takes the context as well as a spec, so all of
  them are built one way.
- `UploadQueue` holds what the worker decoded and uploads it to a byte budget, for `Textures`
  and `MeshRegistry`. A failed request is logged and frees its path. A texture's handle draws
  white until it is released, and a mesh's resolves to nothing. A texture used to keep its
  path, so a file that had failed once read white for good.
- Every release goes through the ring but the swapchain's, which waits for the device because a
  present has no fence. `Capture` replaces a readback it outgrows and retires the old one. The
  context replaces its depth buffer on a new extent and retires the old one. `Buffer::grow` and
  `DepthBuffer::recreate` are gone.
- `Headless::draw()` and `picture()` record a frame whose passes name their own targets and
  read one target back. Twenty-two cases use them. A case that draws into the frame's own image,
  records several frames, or reads one layer still spells the steps out.
- Seven new cases. The upload queue's four fail to build without it, and the texture case for a
  freed path fails. The two release cases fail to build, because `Ring::retiring()` is new.

### Step 8 — A lit scene

**Closed.**

- `LitScene` owns the scene target, the shadow map with a layer per cascade, the lit renderer
  and the grade. `draw()` adds a shadow pass per cascade, the lit pass, and the grade into the
  app's pass. The order rules are kept inside it. The scene is written first, each shadow pass
  names its layer, cascade and bias, and each reader names what it reads.
- A frame's shadows are nothing, one light matrix with every caster drawn, or cascades culled
  per cascade. The passes are named after the scene, so two scenes share a frame.
- A scene is drawn once a frame, and a second draw before the frame is submitted throws. Two
  scenes in one frame grade into passes of their own, since a grade covers its whole pass.
- The full lit scene case draws through `LitScene`. Six new cases. Five fail to build without
  it, and the once-a-frame case fails without its check. The cascade case checks that the
  cascades darken the ground.
- **Found on the way:** a mesh retired into the ring held the ring through its uploader, so a
  mesh released in the last frames before shutdown kept the ring and the device alive. The
  context drains the ring as it is destroyed. One new case, which fails without the change.

### Step 9 — The core library boundaries

**Closed.** [ADR-0118](../adr/0118-assets-a-loader-is-keyed-by-the-kind-it-produces.md) decides
the loader's key, and [ADR-0119](../adr/0119-assets-an-apps-document-list-is-an-asset-manifest.md)
decides `config`.

- A loader is registered for the asset class it produces. A load picks the loader by extension
  and checks the class, and a file whose extension no loader reads is loaded by the loader for
  the class asked for. `asset::Type` is gone, and every image format is `kind::Image`. Three new
  cases.
- `asset::Manifest` holds the documents `config.json` lists, under open kinds. Each library that
  reads a kind declares its name, as `ui::Interface::DOCUMENT`. One new case.
- `windowSize` and `CameraProfiles` are in `engine`, and `SpriteSheets` is in `asset_media`.
  **Different from the plan:** `CameraProfiles` is not beside `type::camera::Profile`, because
  `type` would then link `log`, `asset` and Boost.JSON.
- **Different from the plan:** two libraries leave `type`, not one. `animation` holds
  skeletons, poses, clips, curves and tracks. `model` holds `model::Model` over it, so the
  editor's keys and a sprite clip do not bring a model with them. The particle effects are
  `ecs::effect`, beside the `Emitter` component that steps them.
- `dag` is gone. `Item` holds its id and a `type::Transform` placement, and a command records an
  item's whole placement as one. Five new cases.
- pong's `Position1D`, `Position2D` and `Color3` are pong's. The ecs and engine suites draw
  positions of their own between steps.
- `ui::Interface`, `audio::Player`, `event::Router` and `input::Devices` replace the four
  secondary `Engine` classes.

### Step 10 — Font and image

**Closed.**

- `font` decodes UTF-8, and a glyph's charcode is a `char32_t`. `loadGlyphs()` and the layout
  take UTF-8, and `TextureFont::width()` measures it. The layout draws only glyphs already
  packed, so it never adds to an atlas on the device. The white square is
  `TextureFont::SOLID`. `TextRenderer` no longer decodes, filters or measures.
- `Canvas::text()` takes laid out vertices and indices, so `render` does not link `font`.
- A `Writer` encodes to bytes. `Factory::encode()` and `Factory::kind()` replace
  `Factory::write()`, and moya, `Capture`, imagetool and the suites write through
  `asset::writeFile`. `TextureAtlas::write()` is gone, since `image` writes no file.
- The asset image loader reads through `asset::readFile` and decodes from memory.
- Five new cases. They cover the UTF-8 decoder's `next()`, the layout of an unpacked glyph, a
  font's width against its layout, and the canvas refusing an index past its run. The fifth
  encodes every format to bytes its reader decodes.

### Step 11 — One ui styling vocabulary

**Closed.**

- Both ui models draw with one `paint::Dressing`, read from a theme by one set of keys through
  `style::readBase()`. The `ui` and `tools` classes still set different values. Its default
  constructor and `Dressing::tools()` build both sets of defaults from one palette, and
  `Palette.h` is gone. `Themes.md` has the one key table, with which model draws each key.
- The immediate layer's keys follow the retained ones. `dim-text` is `disabled-text`, and
  `placeholder` for an empty field. Its caret, check mark, focused field, scrollbar thumb,
  tracks and tabs read `caret`, `mark`, `focus`, `thumb`, `track` and `tab`, so their default
  colours changed. A combo's arrow is drawn in the text colour.
- The active theme is the `Interface`'s. `Screen::begin(ui)` hands it to the component renderer
  and the immediate layer when it changes, before either draws. `Screen::theme()` is gone, and
  the immediate layer keeps no theme. pong, tetris and voxel did not pass their theme on before.
- Theme fonts and `align` are removed. A style's `fonts` array is not read, and
  `style::property::Font` and `Property::Alignment` are gone.
- Three new cases, which fail to build without the change. The theme case checks that a
  style's fonts are not read.

### Step 12 — Immediate and Loader split

**Closed.**

- `Immediate.h` declares only what an app calls. The state, the per-frame structs and the
  steps every widget shares are `immediate::Context`, which an `Immediate` holds through a
  unique pointer, so it moves and is not copied. The source is `Immediate.cpp` and five files
  in `immediate/`: the context, windows and scrolling, the text field, the combo's list, and
  tabs and tables.
- `Loader` is a facade over `loader::ThemeReader` and `loader::ComponentReader`. Both read
  through `loader/Members.h`, which states the one rule. A bad required member, or a bad entry
  of an array, refuses the document. A bad optional member warns and keeps its default.
- **Different from the plan:** the rule is the one `DocumentsAndLayout.md` already stated, and
  the code is brought to it. A malformed colour no longer reads as white, and an unknown menu
  item type is refused. A wrong-typed `theme`, `styles`, property array, `children`, flag,
  position or size is now ignored, as is an unknown `edge`, `state`, `direction` or `anchor`.
  Some of those refused the document before, and some were ignored silently.
- The Immediate suites are six files over `tests/Layer.h`, and `ThemeTest` is three over
  `tests/Documents.h`. Every moved case keeps its Boost.Test suite, so none is new, and none
  changed, except that one renamed local and one document that the rule now loads.
- Five new cases, one for each kind of member whose handling changed. Each fails without the
  change.

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

**Closed.**

- `RIBExportVisitor` sends its requests to a `rib::Handler`. `rib::Writer` in
  `api/render/offline` writes them as RIB text, and Batch and the export write files through
  it. A number is written in the fewest digits that read back as the same float, where the old
  export wrote six significant digits.
- **Different from the plan:** Render View records the shot in a `rib::Recorder` on the
  editor's thread, and replays it into moya's handler on the render's thread. Calling moya's
  handler directly would build its world, shaders included, on the editor's thread. The
  recorder also holds the meshes until every light is out, which the text buffer did before.
- `Declaration` gains a text form and equality, and `filterName()` names a `Filter`, for the
  writer.
- Eight new cases, all in a new suite for the writer and the recorder, which fail to build
  without the change. The writer's fixed point case writes every request, reads it back, and
  writes it again.

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
