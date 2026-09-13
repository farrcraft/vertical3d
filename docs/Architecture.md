# Architecture

A map of the tree, and the invariants that cross it. The deeper references sit beside this
one: [RenderingPipeline.md](RenderingPipeline.md) for the realtime renderer,
[OfflineRenderers.md](OfflineRenderers.md) for talyn and moya, [Editor.md](Editor.md) for
`vertical3d/`, and [adr/](adr/) for why any of it is shaped as it is.

## The libraries

Reusable C++ libraries live under [api/](../api/), one target per subdirectory named
`v3dlib_<name>`, with the namespace mirroring the path — `v3d::asset`,
`v3d::render::realtime::vulkan`. The [README](../README.md#layout) lists what each one is for.

The apps that consume them are at the top level: pong, tetris, voxel, odyssey, vertical3d,
talyn, moya, imagetool, v3dshell. Every one of those directories builds with the tree.

## Two different classes named Engine

`v3d::engine::Engine` (api/engine) is the *game* engine: main loop, window, asset manager,
config, input. Each app subclasses it as `Controller`.

`v3d::render::realtime::Engine` (api/render) is the *render* engine, subclassed as `Engine3D`.

Apps hold both.

## The shell around a game belongs to the api

Per [ADR-0028](adr/0028-an-apps-shell-belongs-to-the-api.md), an app carries only what makes
it that game. Five pieces live in the api:

- `v3d::engine::run<T>(argv[0], "<name>")` is an app's `main`. It derives the app path, runs
  initialize and eventLoop inside a try block that logs what a renderer threw, and shuts down
  outside it.
- `v3d::ui::paint::TextRenderer` owns the font, the atlas and the glyphs. It hands
  `ComponentRenderer` the `measure()` and `write()` callbacks that
  [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md) keeps it built from.
- `v3d::ui::shell::GameMenu` is the menu the escape key puts up. It holds the pause as a `Suspend`
  callback.
- `v3d::ui::shell::StatisticsOverlay` draws what the loop measured about its own pacing, hidden
  until something shows it. It copies the numbers into a `Sample` rather than reading an
  `engine::Statistics`, because `api/ui` sits below `api/engine` and cannot name that class.
- `Engine3D::beginFrame` is the minimized-window check a `draw()` opens with.

An app that reimplements one of these has diverged rather than customised.

**Feature flags decide what exists.** `Engine::initialize(int features)` takes a bitmask of
`v3d::engine::Feature` and constructs only what was asked for. `Feature::Config` loads
`data/config.json`, which must use the indirect form:
`{"configs": [{"type": "...", "file": "..."}]}`. Pong's `data/` is the reference. The types are
`window`, `binding`, `ui`, `sound`, `camera`, `layout` and `sprite`; the last is a table of
names over pixel rectangles in an image, read by `config::SpriteSheets`. **That one is also
the only config document the tree writes**: a sprite sheet is packed by a tool rather than
typed by a person, so `SpriteSheets::document()` emits what `load()` reads and a packer does
not carry a second implementation of the format.

**A config document names an image and never loads one**, per
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md): a theme's images and a
sprite sheet's are both resolved by the app through its own asset manager and renderer.

`Config::load` and `registerEventMappings` guard every lookup and log a `false`, but **a
window config is not guarded**: `initialize` reads `width` and `height` with `at()`, so a
`window.json` naming neither throws.

## Where a player's files go

`v3d::engine::appPath(argv[0])` is where an app reads what it shipped with;
`v3d::engine::userPath(org, app)` is where it writes what the player chose, and it creates
the directory. `engine::Settings` is the document in there: an overlay of what was changed,
so deleting it is a reset and a setting nobody touched keeps tracking the shipped value.
Written whole or not at all, per
[ADR-0041](adr/0041-a-document-is-written-whole-or-not-at-all.md).

**The org is `Vertical3D` and the app is its own name**, as pong uses them. Neither can change
once an app has shipped: they are the directory, and a new pair orphans every existing
player's settings.

The schema of the document is the app's. `Settings` knows a key, a value and a version;
applying a setting is `Engine::rebind()` or `Window::request()`, and pong's
`applyStoredBindings()` is the reference for what an app does with what it read.

## Rendering

Window → Engine3D → Context3D → Frame → Pass → DrawItem, one of each. An app fills a
`realtime::Canvas` during its tick, submits it, and calls `renderFrame()`. Drawing goes
through dynamic rendering: no `VkRenderPass`, no `VkFramebuffer`.

**There are two primitives**: the batched quad
([ADR-0005](adr/0005-one-batched-quad-primitive.md)) and the line
([ADR-0011](adr/0011-lines-are-the-second-primitive.md)). A rectangle, a sprite, a glyph and a
menu panel are all the quad, so text needs no separate path and `ui::ComponentRenderer` draws
onto the same canvas. Lines are in world space and read the pass camera at set 0.

[RenderingPipeline.md](RenderingPipeline.md) covers the whole chain, both primitives,
offscreen targets and the descriptor set layout. It is the reference for anything below this
line.

## ECS

entt. `v3d::engine::Engine` holds the `entt::registry` as a protected member, so an app's
`Controller` inherits it and passes `&registry_` into the render engine as a raw
`entt::registry*`. [ECSDesign.md](ECSDesign.md) says what exists; the open question is what a
renderable component looks like, and
[RenderingPipeline.md](RenderingPipeline.md#still-open-how-this-meets-the-ecs) states it.

## Tile grids

`api/grid` is a board plus the two things asked of one: a route across it and a sight line
over it. It names no device, so its suite runs anywhere.
[ADR-0029](adr/0029-tile-grids-are-an-api-library.md) settles its three rules — movement is
8-way at a flat cost per step, a diagonal may not pass between two blocked tiles, and sight is
symmetric because the endpoints are ordered before the line is traced.

- **The grid is asked what it cannot know.** `TileFilter` and `SightBlocker` are caller
  predicates. The grid never learns what an occupant is, which keeps it usable for map
  generation too.
- **One predicate serves the path search, the reachable set and the distance field**, so a tile
  a movement highlight offers is one the path search can reach for the cost shown.
- **`tileDistance()` is Chebyshev**, and it is the metric anything measured in tiles uses. A
  second metric invented elsewhere would disagree with what movement charges.
- **The overlay is geometry, not drawing.** `Overlay.h` hands its segments to a `LineSink`
  callback instead of writing into a `LineCanvas`. So `v3dlib_render` gains no dependency on
  `v3dlib_grid`, and every case is assertable with no device. There is no world space filled
  primitive in the tree, so only outlines come across; a filled tile has no home yet.

**`odyssey` is the consumer.** It reads a board from `data/map.json` — rows of characters, one
per tile — into a `TileGrid`, a click routes the player there with `findPath`, and
`tile::Sight` asks `hasLineOfSight` what the player can see from where it stands. The app's own
`tile::Kind` is what decides passability and cover together; the grid holds both and has an
opinion about neither, which is the split ADR-0029 is built on. How far the player can see and
what it remembers seeing are the app's for the same reason the map format is - the grid answers
about two tiles and knows nothing of a viewer - and the format is deliberately not in the api:
one consumer is not a library.

## Geometry

**Three things are called a mesh, and they are not interchangeable.**

| | |
|---|---|
| `brep::BRep` | half-edge topology, what the editor models with |
| `type::Model` | an interleaved vertex array with indices and a material — what a file on disk becomes |
| `render::realtime::vulkan::Mesh` | two device buffers |

A `Model`'s vertex layout is a contract between the loader and whatever pipeline an app
writes, not something the device enforces. `vulkan::Mesh` takes bytes and a count because the
stride belongs to the pipeline.

**A model names its texture rather than carrying it.** `type::Model::Material` holds a base
colour and the name the file gave its image, and the app resolves that name through the asset
manager. This is the same shape
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) settles for themes; see
[ADR-0030](adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md). A glTF whose
image is *embedded* — a `.glb`'s own buffer, or a data uri — has no name to give, so it
arrives decoded instead, on `asset::Model::baseColourImage()`. That is on the asset rather
than on the material because `api/type` is built against glm alone and a material holding an
image would take `api/image` into every consumer of a mesh.

**Meshes are owned by the app**, not by `Resources`
([ADR-0010](adr/0010-meshes-are-owned-by-the-app.md)).

## The loop has two virtuals, and they mean different things

Per [ADR-0032](adr/0032-the-loop-simulates-at-a-fixed-step.md), `eventLoop()` measures each
frame in nanoseconds, hands it to `tick(unsigned int delta)` once, then drains however many
whole 60 Hz steps that frame owes through `simulate(float step)`, then calls `render()`.

Before any of that it polls, and `Engine::route()` offers each event to three places in a
fixed order: `onEvent()` first, then the input engine's bindings, then the engine's own
`handleEvent`. That order is [ADR-0043](adr/0043-an-app-sees-an-event-before-the-bindings-do.md)
and is what lets an app host a ui toolkit it did not write.

**Three places, but the bindings are not the only way to hear about input.** A device
publishes its `event::kind::*` — `KeyDown`, `MouseButton`, `MouseMotion` — through the
dispatcher whichever mappers exist, and the mapper subscribes to a separate source event
alongside them. So an app subscribes to the abstracted event directly and adopts no
`mappings.json`, which is what voxel and the editor already do for motion and resize; a
binding document is a convenience, not the price of admission. What the bindings buy is a
command named in config rather than in a switch.

`Engine::keys()` and `Engine::mouse()` are the polled half of the same thing, and answer
`held()` for what is down now plus `pressed()` and `released()` for what changed edge during
this frame's events. The loop clears the edges after `render()`, so a key pressed and released
inside one frame answers both and is never seen held — the distinction polling SDL directly
cannot make. Either is null when the app did not ask for that device's `Feature`.

**Simulation goes in `simulate()`.** What runs there produces the same result whatever the
frame rate was; what runs in `tick()` does not. Per-frame work that is not simulation — input
state, UI animation, camera smoothing — is what `tick()` is still for.

`Engine::alpha()` is the fraction of a step held but not yet simulated, for a renderer that
interpolates between two simulation states. Nothing reads it yet. `Engine::statistics()` is
what the loop measured about its own pacing; steps-per-frame is the number worth watching.

Every app that simulates is on `simulate()`. **Voxel is the one that overrides both**, and is
worth reading as the example: its world steps in `simulate()`, while chunk remeshing — a budget
of so many chunks per frame — and the debug overlay's frame-time average stay in `tick()`,
because neither is simulation and neither wants to run twice on a slow frame.

## Invariants that bite

- **Simulation belongs in `simulate()`, not `tick()`.** Both are called from the loop and
  nothing enforces the split, so simulation left in `tick()` is frame-rate dependent and
  compiles. `tick` is milliseconds and `simulate` is seconds, which is the only thing that
  stops one being passed where the other belongs.
- **`onEvent()` returning true consumes the event, and the bindings never see it.** That is
  what it is for — a click that both presses a button the app drew and gives an order to the
  scene is the bug it prevents — and it is also how an app silently disables its own
  `mappings.json` by taking everything. Quit, resize and focus run whatever it returns.
- **A quit command calls `Engine::quit()`, never `shutdown()`.** `eventLoop` ticks and renders
  after a handler returns, so tearing the window down inside one leaves the next frame drawing
  into a destroyed window. `quit()` sets a flag the loop breaks on, and `main` calls
  `shutdown()` after `eventLoop()` returns.
- **An app's `shutdown()` must tear its renderer down before the base class runs.** The context
  owns the device that holds the window's surface alive, and `Window::destroy()` unloads the
  vulkan library. A surface released after that is never destroyed, and the instance reports it
  leaked.
- **`DrawItem::pushCapacity` is 128 bytes**, which is all vulkan guarantees, so an item can
  carry a transform alongside the floats a lit or graded material wants. The cost is paid per
  item per frame: an item is copied into a pass's queue by value, so the unused part of the
  block is memcpyd whether or not a pipeline declared it.
- **A frame may submit any number of canvases, and each takes a buffer of its own.** Appending
  into one buffer would not work, because `vulkan::Buffer::grow` replaces the allocation and
  invalidates the handle every draw item recorded before it is holding.
- **The swapchain is UNORM, not sRGB**, so colour is authored in display space
  ([ADR-0009](adr/0009-colour-authored-in-display-space.md)). A lit 3D scene will revisit this.
- **`v3d::type::camera::Camera` builds Vulkan clip space**, and `project()` and `unproject()` are
  inverses ([ADR-0012](adr/0012-camera-builds-vulkan-clip-space.md)). **Its basis is
  `right = up x direction`**, the opposite hand to `glm::lookAt`'s. Screen right is
  `camera::Profile::right()`. A camera behaviour that names a world axis copied from a `lookAt`
  moves the scene the wrong way with nothing else looking wrong, so `type::camera::Isometric`
  carries a hand and crosses by it, and asserts the direction through `project()`. A consumer
  whose geometry is wound for `glm::lookAt` names the other hand
  ([ADR-0052](adr/0052-a-consumer-names-the-camera-hand.md)); that basis is a mirror rather than
  a second rotation, so the profile's quaternion is the proper one either way and `createView()`
  negates view x. Nothing in this tree names it, so `right()` here always means the first one.
- **`image::Image` row 0 is the top of the picture.** Every consumer downstream reads them that
  way: the canvas, the texture factory, the atlas packer. The jpeg reader also asks the decoder
  for RGB whatever the file holds, because it builds a 24 bit `Image` and copies three bytes a
  pixel.
