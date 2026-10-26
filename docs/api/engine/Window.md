# The window

`v3d::window::Window`, in [api/window/Window.h](../../../api/window/Window.h), is the OS window
and the platform calls that act on it. It is a library of its own, `window`, which links SDL3
and no graphics API. `engine`, `render` and `ui` each use it.

## Who makes it

**The engine opens the window when an app asks for `Feature::Window`.** The size comes from the
`window` config document, and `window()` returns it from `start()` until `release()` has run.
An app with no `Feature::Window` has no window, and builds and runs without the Vulkan SDK.

A program that does not run the engine can make a window itself: start SDL's video subsystem,
construct a `Window` with a logger, and call `create(size)`. `create()` returns false, and logs
why, when the window cannot be made. The window is made for presenting with Vulkan, and SDL
loads the Vulkan loader when it creates one, so a machine with no Vulkan driver fails here.

## What it does

| Call | What it does |
|---|---|
| `width()`, `height()` | The size the window was last given. The engine records each resize SDL reports |
| `request(size)` | Asks for a size. The recorded size changes only when SDL reports the resize |
| `caption(text)` | Sets the title |
| `focused()` | Whether the window has keyboard focus |
| `fullscreen(on)` | Fills the display the window is on, or goes back to a window |
| `textInput(on)` | Starts or stops the platform composing text. `ui::shell::Keyboard` calls it as focus moves |
| `relativeMouse(on)` | Hides the cursor and holds it in the window, for mouselook |
| `cursor(on)`, `warpCursor(x, y)` | Shows or hides the cursor, and moves it |
| `sdl()` | The `SDL_Window`, for a renderer to present to |

## Presenting to it

**A renderer makes the surface it presents to from `sdl()`.** `Engine3D::initialize(window)`
does this. Every surface is made against one Vulkan instance for the process, which
`render::realtime::sharedInstance()` returns. It is made on first use and destroyed when the
last thing holding it lets go, so two windows share it, and a renderer that is released and
made again gets a new one.

**A renderer is released before its window is destroyed.** The renderer's device holds the
surface, and a surface has to go before the window it presents to. The engine destroys the
window after the app's `release()`, so an app releases its renderer there.
[Lifecycle.md](Lifecycle.md) has the order.

Background: [ADR-0116](../../adr/0116-platform-the-window-is-a-library-below-the-renderer.md)
