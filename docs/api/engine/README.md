# The game engine

These pages are for someone writing an app against the api. They cover
`v3d::engine::Engine` in [api/engine/Engine.h](../../../api/engine/Engine.h), the class every
app subclasses. Terms are defined in the [glossary](../README.md#glossary).

There are two classes named `Engine`. These pages are about the game engine. The render engine,
`v3d::render::realtime::Engine` and its subclass `Engine3D`, is a separate object that an app
creates and holds; [rendering/](../rendering/README.md) covers it. Menus and other ui are in
[ui/](../ui/README.md).

| Page | Read it to |
|---|---|
| This page | See what the engine hands an app, which parts of an app the api provides, and how to log |
| [Lifecycle.md](Lifecycle.md) | Write `main`, start up, shut down, and quit |
| [Loop.md](Loop.md) | Put work in `tick()` or `simulate()`, and read the loop's statistics |
| [Input.md](Input.md) | Receive events, bind keys to commands, and poll the keyboard and mouse |
| [Config.md](Config.md) | Read the config documents an app ships with |
| [Files.md](Files.md) | Save the player's settings, and write files safely |
| [Audio.md](Audio.md) | Load and play sounds |

## What the engine gives an app

These are available from `start()` onwards. Each is null if the feature that builds it was not
asked for.

| Member | What it is |
|---|---|
| `logger()` | The process log. See [Logging](#logging). |
| `assets()` | The asset manager for `<app path>/data/`. See [Assets.md](../Assets.md). |
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
| `registry_` | The `entt::registry` the app's entities live in. Protected. See [ECS.md](../ECS.md). |

## The shell an app gets

The parts of an app that are the same in every game live in the api. An app keeps only what
makes it that game. Do not write your own version of these:

- **`main`** — `v3d::engine::run<T>`, described in [Lifecycle.md](Lifecycle.md#the-run-function).
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

[ui/](../ui/README.md) describes how to use each of these.

Background: [ADR-0028](../../adr/0028-apps-the-shared-app-shell-lives-in-the-api.md)

## Logging

There is one log per process, written by spdlog. `run<T>` opens it as `v3d.log` beside the
executable before anything logs. A test or a tool that never calls `log::Logger::open()` writes
`v3d.log` in its working directory.

`Logger::open(path)` returns whether the log is written to `path`. A path that cannot be opened,
such as one in a directory the process cannot write to, does not throw: the log goes to stderr
and its first line says why. A handle taken before `open()` keeps writing where it was taken.

**A format string is checked when it compiles.** `logger_->get()` is spdlog's logger, which takes
fmt's checked format string, so a call whose arguments do not match its `{}` fields does not
build.

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
