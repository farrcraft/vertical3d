# The game engine

This page is for someone writing an app against the api. It covers `v3d::engine::Engine`: how
an app starts and stops, the main loop, events and input, config documents, settings, logging,
audio and file writing. Drawing is in [Rendering.md](Rendering.md) and menus are in
[UserInterface.md](UserInterface.md). Terms are defined in the [glossary](README.md#glossary).

- [The app lifecycle](#the-app-lifecycle)
- [What the engine gives an app](#what-the-engine-gives-an-app)
- [The shell an app gets](#the-shell-an-app-gets)
- [The loop](#the-loop)
- [Events and input](#events-and-input)
- [Config documents](#config-documents)
- [Settings and the player's files](#settings-and-the-players-files)
- [Logging](#logging)
- [Audio](#audio)
- [Writing files](#writing-files)

There are two classes named `Engine`. This page is about `v3d::engine::Engine` in
[api/engine/Engine.h](../../api/engine/Engine.h), the game engine. The render engine,
`v3d::render::realtime::Engine` and its subclass `Engine3D`, is a separate object that an app
creates and holds. [Rendering.md](Rendering.md) covers it.

## The app lifecycle

An app is a subclass of `v3d::engine::Engine` and a `main` of one line.
[examples/starter/src/](../../examples/starter/src) is a complete working app.

```cpp
#include <api/engine/Application.h>
#include <api/engine/Engine.h>

class AppEngine : public v3d::engine::Engine {
 public:
    explicit AppEngine(const std::string& appPath) : Engine(appPath) {}
    bool tick(unsigned int delta) override;    // per-frame work, milliseconds
    bool simulate(float step) override;        // simulation, seconds
    bool render() override;

 protected:
    bool start() override;                     // build the renderer and the scene
    bool release() override;                   // release the renderer
};

int main(int argc, char* argv[]) {
    return v3d::engine::run<AppEngine>(argv[0], "myapp");
}
```

### The run function

`v3d::engine::run<T>(argv[0], name, args...)` in
[api/engine/Application.h](../../api/engine/Application.h) is the whole of `main`. It does this,
in order:

1. Works out the app path, the directory the executable is in, from `argv[0]`.
2. Opens the log at `v3d.log` in that directory.
3. Constructs `T` with the app path, followed by any extra `args` you passed. Use those for
   options parsed from the command line before the engine exists.
4. Calls `initialize()` and then `eventLoop()` inside a `try` block. An exception is written to
   the log as `"<name> failed: <message>"`. A windowed app has no console, so the log is the
   only place an error is readable.
5. Calls `shutdown()` outside the `try` block, so it runs whether the loop ended normally or by
   throwing.

It returns `EXIT_FAILURE` if startup, the loop or shutdown failed, and `EXIT_SUCCESS`
otherwise.

### Startup

`initialize()` is not virtual. It builds these, in this order, and then calls the app's
`start()`:

1. The logger.
2. The asset manager, rooted at `<app path>/data/`, with the picture and model loaders
   registered on it.
3. The event dispatcher (`entt::dispatcher`) and the event engine.
4. With `Feature::Config`: the config, read from `data/config.json`, and the bindings, if the
   config lists a binding document.
5. With `Feature::KeyboardInput` or `Feature::MouseInput`: the input devices.
6. With `Feature::Window`: SDL video and the window. The window takes its size from the window
   document if there is one, and its own default size otherwise.

`features()` says which of the four features the engine builds. The default is all four, so an
app overrides it only to ask for fewer:

```cpp
v3d::engine::Features features() const override {
    return v3d::engine::Feature::Window | v3d::engine::Feature::Config;
}
```

`start()` runs once everything above exists. It is where an app builds its renderer, its ui
and its scene. Returning false stops startup, and `run<T>` reports a failed run.

### Shutdown

`shutdown()` is private to the engine, and only `run<T>` calls it. It does this:

1. Calls the app's `release()`, once.
2. Destroys the window and shuts SDL down.

`release()` is where an app releases its renderer and anything else that presents to the
window. The renderer's context owns the Vulkan device, and the device keeps the window's
surface alive. `Window::destroy()` unloads the Vulkan library. A surface still held after
that is never destroyed, and the Vulkan instance reports it as leaked.

The engine's destructor destroys the window and SDL if `shutdown()` never ran, for example in a
test or after a failed start. It cannot call the app's `release()`, because the app's members
are already destroyed when a base class destructor runs.

### Rules

- **Startup work goes in `start()` and teardown goes in `release()`.** Do not override
  `initialize()` or call `shutdown()`; neither is possible.
- **Release everything that presents to the window in `release()`.** That includes a second
  renderer built after startup.
- **A quit command calls `quit()`.** `quit()` sets a flag, and the loop stops before the next
  frame. The loop still ticks and renders after an event handler returns, so the window must
  outlive the handler. An app cannot reach `shutdown()`, so it cannot tear the window down from
  a handler.
- The engine already answers the `ui::quit` command, a window close request and an SDL quit
  event by calling `quit()`. A menu's quit item and a quit key can both send `ui::quit`.

Background: [ADR-0080](../adr/0080-apps-the-engine-owns-startup-and-shutdown-order.md)

## What the engine gives an app

These are available from `start()` onwards. Each is null if the feature that builds it was not
asked for.

| Member | What it is |
|---|---|
| `logger()` | The process log. See [Logging](#logging). |
| `assets()` | The asset manager for `<app path>/data/`. See [Assets.md](Assets.md). |
| `config()` | The config documents. Needs `Feature::Config`. |
| `document(type)` | One config document as a `boost::json::object`, or null. Protected. |
| `dispatcher()` | The `entt::dispatcher` events are published on. |
| `events()` | The event engine, which maps source events to commands. |
| `window()` | The `realtime::Window`. Needs `Feature::Window`. |
| `keys()`, `mouse()` | Polled input state. Need the input features. |
| `held(command)` | Whether a key bound to a command is down. |
| `rebind(command, key)` | Points a command at a different key. Protected. |
| `alpha()` | How far between two simulation steps the frame is. |
| `statistics()` | What the loop measured about its pacing. |
| `measure(name)` | Times a scope into the statistics. Protected. |
| `registry_` | The `entt::registry` the app's entities live in. Protected. See [ECS.md](ECS.md). |

## The shell an app gets

The parts of an app that are the same in every game live in the api. An app keeps only what
makes it that game. Do not write your own version of these:

- **`main`** — `v3d::engine::run<T>`, described above.
- **The app path** — `v3d::engine::appPath(argv[0])`.
- **Text** — `v3d::ui::paint::TextRenderer` owns the font, the glyph atlas and text drawing.
- **The pause menu** — `v3d::ui::shell::GameMenu` is the menu the escape key opens over a game.
  It pauses the game through a callback and handles its own navigation commands.
- **Frame statistics** — `v3d::ui::shell::StatisticsOverlay` draws the loop's measurements. It
  starts hidden.
- **The screen** — `v3d::ui::shell::Screen` builds the text renderer, the component renderer
  and the statistics overlay over an `Engine3D`, and owns the canvas they draw into. Its
  `begin()` starts each frame and returns false while the window is minimised. In that case the
  app draws nothing that frame.

[UserInterface.md](UserInterface.md) describes how to use each of these.

Background: [ADR-0028](../adr/0028-apps-the-shared-app-shell-lives-in-the-api.md)

## The loop

`eventLoop()` repeats these steps until something calls `quit()`:

1. **Poll events.** Each SDL event is routed as described in
   [Event routing](#event-routing). If a handler called `quit()`, the loop stops here and
   nothing else runs for that frame.
2. **`tick(delta)`**, once, with the time since the last frame in whole milliseconds.
3. **`simulate(step)`**, zero or more times: once for each whole fixed step the elapsed time
   covers. `step` is always `1/60` of a second.
4. **`render()`**, once.
5. **Clear the input edges**, so the next frame starts with no presses or releases recorded.

If `tick`, `simulate` or `render` returns false, the loop ends and the run fails.

### tick and simulate

The two virtual functions take different units on purpose:

| | Called | Argument | Use it for |
|---|---|---|---|
| `tick(unsigned int delta)` | once per frame | milliseconds since the last frame | input state, ui animation, camera smoothing, per-frame budgets |
| `simulate(float step)` | once per fixed step | seconds, always `Accumulator::seconds` | anything that changes the game world |

**Simulation goes in `simulate()`, not in `tick()`.** Code in `simulate()` gives the same
result at any frame rate. Code in `tick()` does not. Nothing enforces this, and simulation left
in `tick()` compiles and runs, but its results depend on the frame rate. The differing argument
types mean that passing one function's time to the other does not compile.

Write simulation in seconds: a velocity is units per second, so `position += velocity * step`
needs no conversion.

voxel is a useful example because it overrides both. Its world steps in `simulate()`. Chunk
remeshing, which has a budget of chunks per frame, and the debug overlay's frame-time average
stay in `tick()`. Neither is simulation, and neither should run twice on a slow frame.

### The fixed step

`v3d::engine::Accumulator` in [api/engine/Accumulator.h](../../api/engine/Accumulator.h) does
the arithmetic:

- The loop measures each frame in nanoseconds with `SDL_GetTicksNS()`.
- **The step is 60 Hz and is a constant.** It is not read from config and cannot change at run
  time. `Accumulator::step` is 16,666,666 ns and `Accumulator::seconds` is `1.0f / 60.0f`.
- **A frame longer than 250 ms is clamped to 250 ms, and the excess time is dropped.** A window
  drag or a breakpoint therefore slows the world for a moment, rather than making the loop run
  hundreds of steps to catch up.
- Time that does not fill a whole step is carried to the next frame.

An app whose `simulate()` takes longer than the step it is given can never catch up. The clamp
turns that into permanent slow motion rather than a hang. The frame statistics show it.

### Alpha

`Engine::alpha()` is the fraction of a step that has elapsed but not yet been simulated, in
`[0, 1)`. A renderer that draws between the last two simulated states blends by it. Without
it, motion is drawn snapped to the last 60 Hz step, which judders on a faster display.
[ECS.md](ECS.md#drawing-between-steps) describes how to keep the previous state and draw with
`alpha()`.

### Frame statistics

`Engine::statistics()` returns an `engine::Statistics`
([api/engine/Statistics.h](../../api/engine/Statistics.h)):

- `last()` — the last frame's length in nanoseconds, before clamping.
- `mean()` — the mean frame length over the last 64 frames (`Statistics::window`).
- `steps()` — how many simulation steps the last frame ran.
- `frames()` — frames since the loop started.
- `rows()` — named timings. Each row has the last frame's time and the mean over the window.

**Steps per frame is the number to watch.** A healthy frame runs 0 or 1 steps, with an
occasional 2. A sustained 3 or more means the clamp is dropping time and something cannot keep
up.

To time part of a frame, hold the scope that `measure(name)` returns for as long as the work
runs. Two scopes with the same name in one frame add together.

```cpp
{
    auto timing = measure("remesh");
    remeshChunks();
}
```

`ui::shell::StatisticsOverlay` draws these numbers. See [UserInterface.md](UserInterface.md).

Background: [ADR-0032](../adr/0032-loop-fixed-step-simulation-variable-rate-rendering.md)

## Events and input

Input reaches an app in four ways. Use whichever fits:

- **Raw SDL events**, through `onEvent()`. Use this to host a ui toolkit you did not write.
- **Source events**, on the dispatcher's `sink<event::Source>`. Use this to capture a key.
- **Commands**, on the dispatcher's `sink<event::Event>`, made from source events by the
  binding document. Use this when the action should be named in config rather than in code.
- **Polling**, through `keys()`, `mouse()` and `held()`, from `tick()` or `simulate()`.

### Event routing

Each polled SDL event is offered to three places, in this order:

1. **The app's `onEvent(const SDL_Event&)`.** Returning true consumes the event, and the input
   devices never see it.
2. **The input devices**, which update the key and mouse state and publish source events. A
   key or mouse event they handle stops here.
3. **The engine's own handling** of quit, window close, resize and focus. This runs whatever
   `onEvent()` returned.

On a resize the engine updates the window's size and publishes
`event::kind::WindowResize`. On a focus change it publishes `event::kind::WindowFocus`. A key
released while the window is unfocused never arrives, so an app that tracks held keys itself
should drop them when focus is lost.

Rules for `onEvent()`:

- **Returning true hides the event from the bindings.** An app that returns true for every
  event disables its own binding document, and nothing reports it. Return true only for events
  the app's own ui actually used. A click on a button the app drew should not also fire the
  command bound to that click.
- **Keep it short.** It runs inside the poll loop, so a slow handler shows up as input latency.
- The default returns false. An app that does not host a ui toolkit does not override it.

Background: [ADR-0043](../adr/0043-input-apps-see-raw-events-before-bindings.md)

### Keys and commands

A key or mouse button going down or up is a **source event**, `v3d::event::Source`. A named
action that a binding makes from it is a **command**, `v3d::event::Event`. They are published on
two separate sinks of the dispatcher:

| Sink | Carries | Listen here to |
|---|---|---|
| `sink<event::Source>` | every key and button edge | capture a key, or react to a raw key |
| `sink<event::Event>` | commands only | act on named actions |

A device also publishes typed events on the dispatcher for every input:
`event::kind::KeyDown`, `KeyUp`, `MouseButton`, `MouseMotion`, `MouseWheel` and `TextInput`. An
app can listen to these directly and use no binding document at all. voxel and the editor do
this for mouse motion and resize.

`event::publish()` sends a source event in two stages:

1. Every listener on `sink<event::Source>` hears it.
2. Unless one of them called `consume()` on it, the event engine sends the commands it is bound
   to on `sink<event::Event>`.

So every listener hears the key before any listener hears its command, whatever order they
connected in. A key capture, such as a "press a key to rebind" menu item, listens on
`sink<event::Source>` and calls `consume()` on the key it takes. That key then makes no command.
pong's `handleSource` is an example.

**Send a source event only through `event::publish()`.** A source triggered directly on the
dispatcher reaches its listeners but is never mapped to a command.

A command's identity is `context::name`, for example `pong::leftPaddleUp`. Its `state()` is
`Pressed` or `Released`, copied from the source that triggered it, so one binding can serve both
edges. Its `data()` is the binding's `param`, if it has one.

Background: [ADR-0081](../adr/0081-input-key-events-and-commands-are-separate.md)

### Bindings

The binding document is the config document of type `binding`, conventionally
`data/mappings.json`. It is a list of mappings from a source to a destination command:

```json
{
  "mappings": [
    {
      "source": { "context": "keyboard", "name": "w", "state": "pressed" },
      "destination": { "context": "pong", "name": "leftPaddleUp", "param": 1 }
    }
  ]
}
```

- `source.context` is `keyboard` or `mouse`. `source.name` is a key name from the table in
  [api/input/Keyboard.cpp](../../api/input/Keyboard.cpp), such as `w`, `escape` or
  `arrow_up`, or a mouse button: `left`, `middle`, `right`, `x1` or `x2`.
- `source.state` is optional. `pressed` or `down` binds the press only, `released` or `up` the
  release only, and anything else, including no state, binds both.
- `destination.context` and `destination.name` name the command. `destination.param` is
  optional and becomes the command's data. It may be a whole number, a boolean or a string.
- A name no device can send is logged and bound anyway. It never fires.
- A malformed document is logged, and startup fails.

`Engine::rebind(command, key)` points a command at a different key while the app runs, for a
rebinding screen. The command keeps the context and the edge the document gave it. A rebinding
lasts only as long as the process. To keep it, store it in [Settings](#settings-and-the-players-files)
and apply it again at startup, as pong's `applyStoredBindings()` does.

### Polling the keyboard and mouse

`Engine::keys()` returns an `input::KeyState` and `Engine::mouse()` an `input::MouseState`.
Either is null if the app did not ask for that device's feature.

| Call | Answers |
|---|---|
| `held(name)` | Is it down now? |
| `pressed(name)` | Did it go down during this frame's events? |
| `released(name)` | Did it come up during this frame's events? |
| `mouse()->position()` | Where the cursor is, in window pixels. |
| `mouse()->wheel()` | How many notches the wheel turned this frame. Positive is away from the user. Several notches in one frame add up. |

Keys and buttons are named, using the same names as the binding document.

The loop clears the edges after `render()`, so `tick()`, `simulate()` and `render()` all see the
current frame's edges. A key pressed and released within one frame answers both `pressed()` and
`released()`, and is never `held()`. Polling SDL directly cannot tell you that.

`Engine::held("context::name")` is true while any key bound to that command is down. Use it for
movement or a camera pan read on the fixed step. It reads the keyboard state rather than
counting command edges, so it works for a binding that fires on press only, and it follows a
`rebind()`. Two limits:

- Only keys count. A mouse button bound to a command never makes it held.
- A command is its name and context, without its param. Directions that must be held
  independently need separate commands, not one command with different params.

### Mouse look

Call `window()->relativeMouse(true)`. The pointer is hidden and held inside the window, and
`event::kind::MouseMotion::motion()` reports how far the mouse moved, even at the edge of the
screen. The platform releases the mouse while the window is unfocused and takes it back when
focus returns. Turn the mode off only for the app's own reasons, such as a menu that needs a
pointer. voxel does this while its menu is open.

## Config documents

With `Feature::Config`, the engine reads `data/config.json`. It must use the indirect form: a
list of typed entries, each naming another file in `data/`.

```json
{
  "configs": [
    { "type": "binding", "file": "mappings.json" },
    { "type": "window",  "file": "window.json" },
    { "type": "sound",   "file": "sounds.json" },
    { "type": "map",     "file": "map.json" }
  ]
}
```

pong's [data/](../../pong/data/) directory is a complete example.

The api recognises seven types, listed in `config::Type` in
[api/config/Type.h](../../api/config/Type.h):

| Type | What reads it | Document shape |
|---|---|---|
| `window` | The engine, when it opens the window | `{"window": {"width": 1024, "height": 768}}`. Both must be whole numbers, or startup fails. |
| `binding` | The engine's bindings | See [Bindings](#bindings). |
| `ui` | `ui::Engine::load()` | Themes and containers. See [UserInterface.md](UserInterface.md). |
| `sound` | `audio::Engine::load()` | See [Audio](#audio). |
| `camera` | `config::CameraProfiles` | `{"cameras": [...]}`, named camera profiles. See below. |
| `layout` | The editor's viewport layout | The editor's own format. See [Editor.md](../Editor.md). |
| `sprite` | `config::SpriteSheets` | See [Sprite sheets](#sprite-sheets). |

**Any other type is an app's own document.** The config loads it like the others and files it
under the type its entry names. odyssey's board is one: its entry has type `map`, and it reads
the document with `config()->get("map")`. Inside an engine subclass, `document("map")` returns
the same document as a `boost::json::object`, or null if the config lists none.

How failures are reported:

- `config.json` missing, or an entry without a `type` or `file`, or a listed file that does not
  load, is logged and stops startup.
- The window document and the bindings check every key they read. A document they do not
  understand is a line in the log and a failed startup, not an exception.
- A config document names images and never loads them. The app resolves a theme's images and a
  sprite sheet's image through its own asset manager and renderer.

Background: [ADR-0020](../adr/0020-ui-themes-are-data-apps-load-the-images.md)

### Camera profiles

`config::CameraProfiles` ([api/config/CameraProfiles.h](../../api/config/CameraProfiles.h))
reads named `type::camera::Profile`s. Each entry needs a `name`. The other fields have defaults:
`orthographic` (true), `eye` (`[0, 0, -10]`), `lookat` (`[0, 0, 0]`), `up` (`[0, 1, 0]`),
`zoom` (10), `fov` (60, in degrees), `aspect` (1.33), `near` (0.1), `far` (100), and `adaptive`
(`none`, `projection`, `position` or `both`). A profile is described by where the camera is and
what it looks at, because `Profile::lookat()` is the one call that sets the basis and the
rotation together. The editor's `data/cameras.json` is the example in the tree. Cameras are
covered in [Types.md](Types.md#cameras).

### Sprite sheets

`config::SpriteSheets` ([api/config/SpriteSheets.h](../../api/config/SpriteSheets.h)) reads a
table of named rectangles in an image:

```json
{
  "sheets": [
    {
      "name": "units", "image": "units.png", "width": 256, "height": 256,
      "sprites": [
        { "name": "knight", "x": 0, "y": 0, "width": 32, "height": 32 }
      ]
    }
  ]
}
```

- Rectangles are in pixels, and each sheet states its own size. `SpriteSheet::uv()` divides one
  by the other to give the uv pair a canvas takes.
- A sheet with no name, image or size is rejected and the others are kept. So is a sprite whose
  rectangle runs off the sheet.
- `get()` returns an empty sheet or region for a name it does not hold, and `uv()` returns
  false. A missing sprite draws nothing and logs nothing, so check `has()` if it matters.
- **This is the one config document the api also writes.** A sprite sheet is usually made by a
  packing tool. Build sheets with `SpriteSheet(name, image, width, height)` and `place()`, add
  them with `SpriteSheets::add()`, and write `document()` with
  [`asset::writeDocument()`](#writing-files). `add()` replaces a sheet of the same name. To keep
  the sheets you did not repack, `load()` the existing document first. Sheets and sprites are
  written in the order they were added, so a repack produces a readable diff.

A ui image can name one region of a sheet. [UserInterface.md](UserInterface.md) covers that.

## Settings and the player's files

An app reads what it shipped with from one directory and writes what the player chose to
another:

| Function | Directory | Used for |
|---|---|---|
| `v3d::engine::appPath(argv[0])` | The directory the executable is in. The engine loads assets from its `data/` subdirectory. | Shipped assets. Rebuilt from source on every build, and not reliably writable. |
| `v3d::engine::userPath(org, app)` | A per-user directory from `SDL_GetPrefPath`. It is created if missing. | Settings, saved bindings, saved games. |

Both return paths ending in a separator. `userPath()` returns an empty string and logs if the
platform cannot say where user files go. The app should then run on its defaults.

**Choose `org` and `app` once and never change them.** They name the directory, so changing
either one after release loses every player's settings. The tree uses `Vertical3D` as the
organization and the app's own name as the app, as pong does.

### Settings

`engine::Settings` ([api/engine/Settings.h](../../api/engine/Settings.h)) is a document named
`settings.json` under the user path. It stores only what the player changed:

- Deleting the file resets everything.
- A setting nobody changed keeps following whatever the current build ships.
- An entry this build does not recognise is kept and written back unchanged.

Use it like this:

```cpp
settings_ = boost::make_shared<v3d::engine::Settings>("Vertical3D", "Pong", logger());
settings_->load();                                      // false if there is no file yet
double volume = settings_->number("volume", 0.8);       // the fallback if unset
settings_->set("volume", 0.5);
settings_->save();                                      // save on every change
```

- The typed reads are `text()`, `number()`, `integer()` and `flag()`. Each returns the fallback
  when the key is missing or holds a value of another kind.
- `set()` stores a value. `clear()` forgets one, so it follows the shipped default again.
- **Call `save()` when a setting changes, not when the app exits.** No exit path is guaranteed
  to run, and a crash should not lose a change.
- The document is versioned (`Settings::VERSION`). An older document is upgraded as it is read.
  A document with no version, or a version that is not a whole number, is refused and the app
  runs on defaults.
- A document from a newer build is read-only: `writable()` is false and `save()` does nothing.
  So a player who runs an older build keeps the settings the newer one wrote.
- Saving is atomic. See [Writing files](#writing-files).

The meaning of each key is the app's. `Settings` stores keys and values; applying one is the
app's job, for example `Engine::rebind()` for a key or `Window::request()` for a window size.
pong's `applyStoredBindings()` is the reference.

## Logging

There is one log per process, written by spdlog. `run<T>` opens it as `v3d.log` beside the
executable before anything logs. A test or a tool that never calls `log::Logger::open()` writes
`v3d.log` in its working directory.

A `v3d::log::Logger` is a handle on that one log. Passing a logger to a class says where it
logs; it does not give the class a separate log. Log through `get()`, with spdlog's `{}`
placeholders:

```cpp
logger()->get()->info("Loaded {} sprites from {}", count, name);
logger()->get()->error("Could not open {}", path);
```

Log failures where they happen. Api functions report most failures as a false return or a null
pointer plus a line in this log, so the log is the first place to look when something is
missing. The Vulkan validation layer, when installed, also writes to it.

## Audio

`api/audio` plays sound through SDL3_mixer. The engine does not create an audio engine or link
the library, so an app that plays sound does three things in `start()`:

```cpp
// link v3d::audio, then:
sound_ = boost::make_shared<v3d::audio::Engine>(logger(), dispatcher());
v3d::audio::registerLoaders(*assets(), logger());      // the .wav loader
sound_->initialize();                                  // open the audio device
if (const boost::json::object* sounds = document(v3d::config::Type::Sound)) {
    sound_->load(*sounds, *assets());
}
```

The sound document lists clips by id:

```json
{ "sounds": [ { "clip_id": "hit", "file": "hit.wav" } ] }
```

- **A clip's file resolves through the asset manager**, relative to `data/`, like every other
  asset. The manager must have had `audio::registerLoaders()` called on it. A clip whose source
  is not a file can be loaded with the overload of `load()` that takes a `Resolve` callback.
- To play a one-shot from anywhere, publish a sound event:
  `dispatcher()->trigger(v3d::event::kind::Sound("hit"))`. The audio engine listens for it.
- `playClip(id)` also plays a one-shot. `play(id, how)` returns a `Voice` for a sound you need
  to control. `audio::Play` sets the bus, the loop count (-1 loops until stopped), a fade-in and
  a gain.
- `stop(voice, fadeOutMs)`, `stopAll()`, `playing(voice)` and `gain(voice, level)` control
  playing sounds. A voice that has finished is refused, never confused with a newer sound.
- `busGain(bus, level)` sets the volume of a named bus such as `music` or `sfx`. It may be set
  before anything plays on that bus.
- **A device that does not open leaves the app silent, not broken.** `initialize()` returns
  false and logs, clips still load, and playing returns false.

The loader is registered for `.wav`.

Background: [ADR-0021](../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0079](../adr/0079-assets-loaders-are-registered.md)

## Writing files

`api/asset` has the functions for writing documents and other files. They are described in
[Assets.md](Assets.md#writing-documents). In short:

- **Every write is atomic.** The new contents go to a temporary file beside the target, which is
  then renamed over it. A failed write leaves the previous file intact.
- **A versioned JSON document is read forward one version at a time** with
  `asset::readForward()`. See [Assets.md](Assets.md#reading-old-documents-forward).
