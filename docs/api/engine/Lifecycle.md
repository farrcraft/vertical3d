# The app lifecycle

How an app is started, how it starts up and shuts down, and how it quits.

## The app lifecycle

An app is a subclass of `v3d::engine::Engine` and a `main` of one line.
[examples/starter/src/](../../../examples/starter/src) is a complete working app.

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
[api/engine/Application.h](../../../api/engine/Application.h) is the whole of `main`. It does this,
in order:

1. Works out the app path, the directory the executable is in, from `argv[0]`.
2. Opens the log at `v3d.log` in that directory. If that file cannot be opened, as in a
   directory the app cannot write to, the log goes to stderr instead and the app still runs.
3. Constructs `T` with the app path, followed by any extra `args` you passed. Use those for
   options parsed from the command line before the engine exists.
4. Calls `initialize()` and then `eventLoop()` inside a `try` block. An exception is written to
   the log as `"<name> failed: <message>"`. A windowed app has no console, so the log is the
   only place an error is readable.
5. Calls `shutdown()` after the `try` block, so it runs whether the loop ended normally or by
   throwing. A throw from `release()`, such as a lost device, still has the window destroyed
   and SDL shut down after it, and is then caught and logged the same way.

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

Background: [ADR-0080](../../adr/0080-apps-the-engine-owns-startup-and-shutdown-order.md)
