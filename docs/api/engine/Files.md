# Settings and files

Where an app writes what the player chose, and how any document is written so that a crash
cannot leave half a file.

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

`engine::Settings` ([api/engine/Settings.h](../../../api/engine/Settings.h)) is a document named
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

## Writing files

`api/asset` has the functions for writing documents and other files. They are described in
[Assets.md](../Assets.md#writing-documents). In short:

- **Every write is atomic.** The new contents go to a temporary file beside the target, which is
  then renamed over it. A failed write leaves the previous file intact.
- **A versioned JSON document is read forward one version at a time** with
  `asset::readForward()`. See [Assets.md](../Assets.md#reading-old-documents-forward).
