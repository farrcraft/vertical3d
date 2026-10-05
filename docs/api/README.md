# The api libraries

This page is for someone writing an application against the libraries under
[api/](../../api/). It lists what each library is for, shows how they depend on each other,
and says where to read next. It ends with a glossary of the terms the other api documents use.

- [The libraries](#the-libraries)
- [How they depend on each other](#how-they-depend-on-each-other)
- [Where to read next](#where-to-read-next)
- [Glossary](#glossary)

## The libraries

Each directory under `api/` is one library. Its CMake target is `v3dlib_<name>`, and a project
outside the tree links it as `v3d::<name>`. Its namespace follows its path, so `api/type/camera`
is `v3d::type::camera`. Two libraries sit inside another library's directory: `asset_media` in
`api/asset/media` and `render_offline` in `api/render/offline`.

| Library | What it is for |
|---|---|
| [`asset`](../../api/asset/) | Finds files under a data directory and loads them through registered loaders. Holds the JSON and text loaders, atomic file writes and document migration. |
| [`asset_media`](../../api/asset/media/) | The loaders for pictures (png, jpeg, tga, bmp) and glTF models. |
| [`audio`](../../api/audio/) | Sound playback over SDL3_mixer, and the loader for wav files. |
| [`brep`](../../api/brep/) | Boundary representation meshes with half-edge topology, which the editor models with. |
| [`config`](../../api/config/) | Reads `data/config.json` and the documents it lists. Also reads camera profiles and sprite sheets. |
| [`dag`](../../api/dag/) | A node with a unique id, and the transform an editor mesh is placed by. |
| [`ecs`](../../api/ecs/) | Shared components and helpers over the EnTT entity registry. |
| [`engine`](../../api/engine/) | The game engine: the app lifecycle, the main loop, the window, config, input and settings. |
| [`event`](../../api/event/) | Event types, key bindings, and the mapping from a key to a command. |
| [`font`](../../api/font/) | Rasterizes a typeface with FreeType into a texture atlas and lays out text. |
| [`grid`](../../api/grid/) | Tile boards: pathfinding, line of sight, terrain from a picture, and overlay geometry. |
| [`image`](../../api/image/) | Image readers and writers for png, jpeg, tga and bmp, cropping, comparison and the texture atlas. |
| [`input`](../../api/input/) | Keyboard and mouse devices, and the state of what is held and what changed this frame. |
| [`log`](../../api/log/) | The process log, a thin wrapper over spdlog. |
| [`render`](../../api/render/) | The Vulkan realtime renderer: window, frames, passes, draw items, canvases, sprites, meshes and lighting. |
| [`render_offline`](../../api/render/offline/) | What the offline renderer moya is built on: RIB, the shading language, sampling, the film and the ray tracer. It uses no Vulkan and no SDL. |
| [`type`](../../api/type/) | Value types and maths: cameras, geometry queries, transforms, models, skeletons, animation, particle effects and random numbers. |
| [`ui`](../../api/ui/) | The user interface: components, layout, themes, an immediate-mode layer, and the shell every game shares. |

## How they depend on each other

Each library links the libraries and packages it uses, so an app names only the `v3d::`
targets it calls into. The list below is the dependency manifest in
[cmake/v3dApiLibraries.cmake](../../cmake/v3dApiLibraries.cmake). Every library also links
Boost.

| Library | Api libraries it links | Third-party packages |
|---|---|---|
| `log` | none | spdlog |
| `type` | none | glm |
| `grid` | none | glm |
| `image` | log | libpng, libjpeg, glm |
| `asset` | log | (Boost only) |
| `event` | log | glm, EnTT |
| `dag` | type | glm |
| `ecs` | type | glm, EnTT |
| `brep` | dag, type | glm |
| `font` | log, image | FreeType, glm |
| `config` | log, asset, type | glm |
| `input` | event, type | **SDL3**, glm, EnTT |
| `asset_media` | log, asset, image, type | cgltf |
| `audio` | log, event, asset | **SDL3_mixer**, EnTT |
| `render_offline` | log, image, type | glm |
| `render` | log, asset, asset_media, ecs, font, image, type | **Vulkan**, VulkanMemoryAllocator, **SDL3**, glm, EnTT |
| `ui` | log, render, asset, event, font, image, input, type | glm, EnTT |
| `engine` | log, asset, asset_media, config, event, input, render, type | **SDL3**, EnTT |

The same picture as layers, lowest first. A library uses only libraries in the rows above it.

```
log   type   grid                          no device, no window
image  asset  event  dag  ecs
font   config  brep  asset_media
render_offline                             offline renderer, no Vulkan or SDL
input                                      SDL3
audio                                      SDL3_mixer
render                                     Vulkan, SDL3
ui     engine                              Vulkan, SDL3 (through render)
```

Three consequences matter when choosing what to link:

- **Anything that links `render` needs the Vulkan SDK to configure**, because the renderer's
  shaders are compiled at build time. That includes `ui` and `engine`.
- **Only `audio` needs SDL3_mixer.** The engine does not link it, so an app that plays sound
  links `v3d::audio` itself and registers its loader. [engine/Audio.md](engine/Audio.md#audio) shows how.
- **`type`, `grid`, `ecs`, `image`, `asset` and `config` need no device and no window**, so a
  test of game rules built on them runs anywhere.

A project outside the tree chooses which libraries to build with the `V3D_LIBRARIES` CMake
variable. [UsingTheApi.md](UsingTheApi.md) covers that.

## Where to read next

| To do this | Read |
|---|---|
| Set up a project of your own against the api | [UsingTheApi.md](UsingTheApi.md) |
| Write the app: lifecycle, loop, input, config, settings, logging, audio | [engine/](engine/README.md) |
| Draw: frames, passes, canvases, sprites, meshes, lighting | [rendering/](rendering/README.md) |
| Build menus and HUDs | [ui/](ui/README.md) |
| Keep game state in entities | [ECS.md](ECS.md) |
| Use cameras, geometry queries, models, animation or particle effects | [Types.md](Types.md) |
| Build a tile board with paths and sight lines | [Grid.md](Grid.md) |
| Load images, fonts, models and JSON documents | [Assets.md](Assets.md) |
| Change the realtime renderer itself | [../internals/realtime/](../internals/realtime/README.md) |
| Change the ui library itself | [../internals/UserInterface.md](../internals/UserInterface.md) |
| Work on moya or `render_offline` | [../offline/](../offline/README.md) |
| See a complete small app | [examples/starter](../../examples/starter/) |

## Glossary

Terms are grouped by subject. Each entry links to the document that covers it.

### The app and the loop

- **Game engine** — `v3d::engine::Engine` in `api/engine`. It runs the main loop and owns the
  window, the config, the asset manager and the input devices. Each app subclasses it.
  See [engine/](engine/README.md).
- **Render engine** — `v3d::render::realtime::Engine`, a different class in `api/render`. Its
  subclass `Engine3D` is the one apps use. See [rendering/](rendering/README.md).
- **Engine3D** — `v3d::render::realtime::Engine3D`, the realtime renderer an app holds. It
  builds and presents frames into the window. See [rendering/](rendering/README.md).
- **Controller** — the class name tetris, voxel and the editor give their game engine subclass.
  It is a naming habit, not an api class.
- **App** — a subclass of the game engine plus a one-line `main` that calls `run<T>`. See
  [engine/Lifecycle.md](engine/Lifecycle.md#the-app-lifecycle).
- **Feature** — one part of the game engine an app opts into: `Window`, `Config`,
  `KeyboardInput` or `MouseInput`. An app lists them in `features()`, and all four is the
  default. See [engine/Lifecycle.md](engine/Lifecycle.md#the-app-lifecycle).
- **start / release** — the app's own startup and teardown hooks. The engine calls `start()`
  once every feature is up, and `release()` before it destroys the window. See
  [engine/Lifecycle.md](engine/Lifecycle.md#the-app-lifecycle).
- **quit** — `Engine::quit()`. It sets a flag that stops the loop after the current frame. A quit
  command calls it. See [engine/Lifecycle.md](engine/Lifecycle.md#the-app-lifecycle).
- **App path** — the directory the executable is in. The engine loads assets from its `data/`
  subdirectory. See [engine/Files.md](engine/Files.md#settings-and-the-players-files).
- **User path** — the per-user directory an app writes settings and saves to. See
  [engine/Files.md](engine/Files.md#settings-and-the-players-files).
- **tick** — `tick(unsigned int delta)`, called once per frame with the frame's length in
  milliseconds. It is for per-frame work that is not simulation. See [engine/Loop.md](engine/Loop.md#the-loop).
- **simulate** — `simulate(float step)`, called once for each fixed step the frame's time
  covers, with the step in seconds. Simulation goes here. See [engine/Loop.md](engine/Loop.md#the-loop).
- **Step** — one fixed slice of simulated time: 1/60 of a second. See
  [engine/Loop.md](engine/Loop.md#the-loop).
- **Alpha** — `Engine::alpha()`, the fraction of a step that has elapsed but not been simulated.
  A renderer uses it to draw between the last two simulated states. See
  [ECS.md](ECS.md#drawing-between-steps).
- **Interpolation** — drawing a value part of the way between its previous and current step, by
  alpha, so motion looks smooth above 60 Hz. See [ECS.md](ECS.md#drawing-between-steps).
- **Statistics** — what the loop measures about its own pacing: frame times and steps per frame.
  See [engine/Loop.md](engine/Loop.md#frame-statistics).

### Input and config

- **Source event** — `event::Source`, a key or mouse button going down or up as a device read
  it. See [engine/Input.md](engine/Input.md#keys-and-commands).
- **Command** — `event::Event`, a named action such as `pong::leftPaddleUp` that a binding
  makes from a source event. A command is identified as `context::name`. See
  [engine/Input.md](engine/Input.md#keys-and-commands).
- **Context** — the first half of an event's identity: a device name such as `keyboard` for a
  source, or an app-chosen group such as `pong` or `ui` for a command.
- **Binding** — one entry in the binding document that maps a source event to a command. See
  [engine/Input.md](engine/Input.md#bindings).
- **Edge** — a press or a release that happened during the current frame, as against a key that
  is held. See [engine/Input.md](engine/Input.md#polling-the-keyboard-and-mouse).
- **Held command** — a command whose bound key is down now, answered by `Engine::held()`. See
  [engine/Input.md](engine/Input.md#polling-the-keyboard-and-mouse).
- **Dispatcher** — the `entt::dispatcher` the engine creates. Devices, the event engine, the ui
  and audio publish and listen on it.
- **Config document** — a JSON file listed in `data/config.json` under a type such as `window`
  or `binding`. See [engine/Config.md](engine/Config.md#config-documents).
- **Settings** — `engine::Settings`, the player's changes stored under the user path. See
  [engine/Files.md](engine/Files.md#settings-and-the-players-files).

### Assets and files

- **Asset manager** — `asset::Manager`, which resolves a name under one directory and loads it
  through a registered loader. See [Assets.md](Assets.md#the-asset-manager).
- **Asset loader** — an `asset::Loader` registered on a manager for one asset type and a set of
  file extensions. See [Assets.md](Assets.md#loaders).
- **Asset kind** — the class a loader returns, such as `asset::kind::Json` or
  `asset::media::kind::Model`. See [Assets.md](Assets.md#loaders).
- **Atomic write** — replacing a file by writing a temporary beside it and renaming it over the
  target, so a failed write leaves the old file intact. See
  [Assets.md](Assets.md#writing-documents).
- **Migration** — one step that upgrades a JSON document by one version. See
  [Assets.md](Assets.md#reading-old-documents-forward).
- **Texture atlas** — `image::TextureAtlas`, one image that many smaller images are packed into.
  See [Assets.md](Assets.md#texture-atlas).
- **Gutter** — the one-texel border the atlas leaves around every region it packs. See
  [Assets.md](Assets.md#texture-atlas).
- **Sprite sheet** — an image plus a table of named pixel rectangles in it, read by
  `config::SpriteSheets`. See [engine/Config.md](engine/Config.md#sprite-sheets).
- **Manifest and closure** — the `V3D_LIBRARIES` CMake variable names the api libraries a
  project links (the manifest). CMake builds those plus everything they depend on (the
  closure), and looks only for the packages the closure needs. See
  [UsingTheApi.md](UsingTheApi.md).

### Entities

- **Registry** — the `entt::registry` that holds an app's entities and their components. The
  game engine holds it as `registry_`. See [ECS.md](ECS.md).
- **Component (ECS)** — a plain value attached to an entity, such as `ecs::component::Transform`.
  See [ECS.md](ECS.md).
- **System** — `ecs::System`, a class that advances some entities by one simulation step. See
  [ECS.md](ECS.md#systems).
- **Snapshot** — copying each entity's component into its `ecs::Previous<T>` at the start of a
  step, so it can be drawn between steps. See [ECS.md](ECS.md#drawing-between-steps).
- **Settle** — making an entity's previous value equal to its current one after a teleport. See
  [ECS.md](ECS.md#drawing-between-steps).

### Geometry, models and effects

- **Mesh** — three different things: a `brep::BRep` the editor models with, a `type::Model` a
  file loads into, and a device-side mesh in the renderer. See [Types.md](Types.md#models).
- **Model** — `type::Model`: one vertex array, one index list, materials and parts. See
  [Types.md](Types.md#models).
- **Part** — a range of a model's indices drawn with one material. See
  [Types.md](Types.md#models).
- **Skeleton, clip, pose, palette** — a model's joints; a named animation; every joint's local
  transform at one time; the matrices a skinned vertex is moved by. See
  [Types.md](Types.md#animation).
- **Emitter** — the description of what a particle effect spawns and how the particles move.
  See [Types.md](Types.md#particle-effects) and [ECS.md](ECS.md#emitter).
- **Hand (handedness)** — which way a camera crosses its basis vectors, and so which world
  direction is on the right of the screen. See [Types.md](Types.md#handedness).
- **Profile** — `type::camera::Profile`, a camera's settings: eye, orientation, clipping, field
  of view and projection kind. See [Types.md](Types.md#cameras).

### Drawing

- **Frame** — everything drawn for one presented image: a list of passes, built during the
  tick and recorded by `Engine3D::renderFrame()`. See [rendering/FramesAndTargets.md](rendering/FramesAndTargets.md).
- **Pass** — one drawing operation into one target, with its own camera, clear, depth test and
  list of draw items. See [rendering/FramesAndTargets.md](rendering/FramesAndTargets.md).
- **Draw item** — `DrawItem`, a description of one draw: pipeline and material handles, push
  constants and a sort key. The renderer records it later. See [rendering/](rendering/README.md).
- **Sort key** — the value a pass sorts its draw items by when sorting is on: layer, then
  pipeline, then material, then depth. See [rendering/FramesAndTargets.md](rendering/FramesAndTargets.md).
- **Canvas** — a buffer of 2D quads an app fills during the tick and submits to a pass.
  `realtime::Canvas` is in screen space; `WorldCanvas` and `LineCanvas` are in world space. See
  [rendering/Canvas.md](rendering/Canvas.md).
- **Render target** — an offscreen image a pass draws into instead of the window. Another pass
  can then read it as a texture. See [rendering/FramesAndTargets.md](rendering/FramesAndTargets.md).
- **Handle** — a small id that names a renderer resource such as a texture or pipeline. It
  carries a generation, so a released handle never resolves again. See
  [rendering/TexturesAndMeshes.md](rendering/TexturesAndMeshes.md).
- **Frames in flight** — frames the GPU may still be working on while the CPU builds the next.
  The **in-flight ring** (`vulkan::frame::Ring`) holds each one's buffers and fences, and frees
  released resources once no frame in flight uses them. See
  [internals/realtime/](../internals/realtime/README.md).
- **Presenter** — `vulkan::Presenter`, which acquires a swapchain image, submits the recorded
  frame and presents it. See [internals/realtime/](../internals/realtime/README.md).
- **Recorder** — the code that turns a pass's draw items into Vulkan commands, binding a
  pipeline, set or buffer only when it changes. See
  [internals/realtime/](../internals/realtime/README.md).
- **Lit pass** — a pass that draws meshes with light and shadow through
  `vulkan::renderer::Lit`. See [rendering/Lighting.md](rendering/Lighting.md).
- **Scene set** — descriptor set 2, which a lit pass binds once. It holds the light, the shadow
  map and the frame's joint matrices. See [rendering/Lighting.md](rendering/Lighting.md).

### User interface

- **Shell** — `ui::shell`, the pieces every game shares: `Screen`, `GameMenu`,
  `StatisticsOverlay`, the SDL keyboard adapter and a file chooser. See
  [engine/README.md](engine/README.md#the-shell-an-app-gets) and [ui/ShellComponents.md](ui/ShellComponents.md).
- **Retained UI** — a tree of components built once from a JSON document and kept in step with
  the game. See [ui/](ui/README.md).
- **Immediate UI** — `ui::Immediate`, a ui written as calls that run every frame, with no tree
  to keep in step. See [ui/ImmediateMode.md](ui/ImmediateMode.md).
- **Component (UI)** — one widget in the retained tree, such as a `Button` or a `Panel`. Not
  related to an ECS component. See [ui/Components.md](ui/Components.md).
- **Theme** — a JSON description of the colours, fonts, metrics and images the ui is drawn
  with. The app loads the images it names. See [ui/Themes.md](ui/Themes.md).

### Offline rendering

- **RIB** — the RenderMan Interface Bytestream, the scene file format moya reads. See
  [offline/](../offline/README.md).
- **Hider** — the part of moya that decides which surfaces the camera sees: the Reyes hider or
  the ray tracing hider. A scene picks one with `Hider`. See
  [offline/](../offline/README.md).
