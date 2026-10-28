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
either one after release loses every player's settings. An app names them once, as the
`engine::Identity` it passes to `run<T>`. The tree uses `Vertical3D` as the organization and
the app's own name as the application.

### Settings

`engine::Settings` ([api/engine/Settings.h](../../../api/engine/Settings.h)) is a document named
`settings.json` under the user path. It stores only what the player changed:

- Deleting the file resets everything.
- A setting nobody changed keeps following whatever the current build ships.
- An entry this build does not recognise is kept and written back unchanged.

**The engine reads the document as it starts**, when the app has an identity, and an app reaches
it through `Engine::settings()`. It is null in an engine built without an identity, as in a
test. Use it like this:

```cpp
double volume = settings()->number("volume", 0.8);     // the fallback if unset
settings()->set("volume", 0.5);
settings()->save();                                     // save on every change
```

- The typed reads are `text()`, `number()`, `integer()` and `flag()`. Each returns the fallback
  when the key is missing or holds a value of another kind. `value()` returns the stored JSON
  as it is, for a setting such as a list, and its reader checks its shape.
- `set()` stores a value. `clear()` forgets one, so it follows the shipped default again.
- **The engine keeps the player's key bindings under `bindings`.** It applies them after the
  binding config loads, before `start()`, and saves after every change to them.
  [Input.md](Input.md#bindings) has the rules.
- **Call `save()` when a setting changes, not when the app exits.** No exit path is guaranteed
  to run, and a crash should not lose a change.
- The document is versioned (`Settings::VERSION`). An older document is upgraded as it is read.
  A document with no version, or a version that is not a whole number, is refused and the app
  runs on defaults.
- A document from a newer build is read-only: `writable()` is false and `save()` does nothing.
  A player who runs an older build therefore keeps the settings the newer one wrote.
- Saving is atomic. See [Writing files](#writing-files).

The meaning of every other key is the app's. `Settings` stores keys and values, and applying
one is the app's job, for example `Window::request()` for a window size.

## Paths

**A narrow path is UTF-8.** That holds for every `std::string` path in the api: `appPath()`,
`userPath()`, the log file, `asset::Manager`'s root and the names given to every loader. SDL
already hands an app its arguments and directories in UTF-8.

Windows reads a narrow path as UTF-8 only in a process whose manifest sets the UTF-8 code page.
`v3dlib_log` carries that manifest as an interface source, so every executable that links
`v3dlib_log` embeds it. An executable that does not link `v3dlib_log` and opens a file has to
add [api/log/Utf8.manifest](../../../api/log/Utf8.manifest) to its own sources.

- It needs Windows 10 version 1903 or later. An older Windows ignores the setting, and a path
  with characters outside the ANSI code page cannot be opened there.
- Convert a path to a narrow string with `string()` only inside a process with the manifest.
  Without it, `boost::filesystem::path::string()` converts through the ANSI code page and
  replaces each character that page cannot hold with `?`.
- A narrow string written to a console is shown in the console's own code page, which the
  manifest does not change. The log file is UTF-8.

Background: [ADR-0089](../../adr/0089-paths-narrow-strings-are-utf-8-under-a-utf-8-code-page.md)

## Writing files

`api/asset` has the functions for writing documents and other files. They are described in
[Assets.md](../Assets.md#writing-documents). In short:

- **Every write is atomic.** The new contents go to a temporary file beside the target, which is
  then renamed over it. A failed write leaves the previous file intact.
- **A versioned JSON document is read forward one version at a time** with
  `asset::readForward()`. See [Assets.md](../Assets.md#reading-old-documents-forward).
