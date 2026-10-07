# Starter

The smallest complete Vertical3D application. It opens a window at the size its config sets,
clears it to a dark blue, and draws one orange rectangle inset 40 pixels from each edge. Use it
as the starting point for an app in a repository of your own.

## Files

| File | Holds |
|---|---|
| `CMakeLists.txt` | The root project. It selects three api libraries (`engine`, `log` and `render`), adds the repository as a subdirectory, and links `v3d::engine`, `v3d::log` and `v3d::render` |
| `vcpkg.json`, `vcpkg-configuration.json` | The package list and registry a consumer repository carries. They match the repository's own |
| `src/main.cxx` | `main`, which calls `v3d::engine::run<AppEngine>(argv[0], "starter")` |
| `src/AppEngine.h`, `src/AppEngine.cxx` | The app. `features()` asks for a window, keyboard input and config. `start()` creates the renderer, `render()` draws the rectangle, and `release()` shuts the renderer down before the window is destroyed |
| `data/config.json`, `data/window.json` | The config list and the window size, 1024 × 768. The build copies `data/` beside the executable |

## Building and running

Configure and build it as [../README.md](../README.md#building-an-example) describes, then run
`out/build/starter/v3dstarter.exe`. The app writes its log to `v3d.log` beside the executable.

[docs/api/UsingTheApi.md](../../docs/api/UsingTheApi.md) walks through building an app like this
one step by step.
