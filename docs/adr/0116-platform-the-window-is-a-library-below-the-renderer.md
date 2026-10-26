# ADR-0116: Platform: the window is a library below the renderer

**Status**: accepted
**Date**: 2026-10-26
**Documented in**: [docs/api/engine/Window.md](../api/engine/Window.md)

## Context

The OS window, its events and its platform calls (fullscreen, focus, relative mouse, text input)
are needed by the game loop and the ui shell, and the renderer needs only a surface to present
to. The renderer's window holds the SDL window, a Vulkan instance and the surface together, so
the loop, the ui shell and any windowless tool that links the engine need the Vulkan SDK. An
instance made per window does not fit a program with more than one window, such as an editor
with a detached render view. A headless renderer needs an instance with no window at all.

## Decision

The window is an api library of its own, `window`, below `engine`, `ui` and `render`. It holds
the SDL window and its platform calls, and links no graphics API. The renderer makes each
window's surface itself, against one Vulkan instance that every window in the process shares.

## Alternatives

### The engine owns the window
- **For**: no new library, and the loop that pumps the window's events owns it.
- **Against**: the ui shell turns text input on and off on the window, so `ui` would have to
  depend on `engine`, and a window could exist only inside the game loop.
- **Rejected because**: `ui` and `engine` are siblings, and a layer above both joins them.

### The engine defines a window interface that a renderer implements
- **For**: the engine names no window library at all.
- **Against**: SDL is the one platform layer, so the interface would have one implementation, and
  every platform call would be declared twice.
- **Rejected because**: the abstraction buys nothing that a plain SDL window does not.

### The window stays in the renderer, and the loop is made generic over it
- **For**: no class moves.
- **Against**: the ui shell still needs the renderer for text input, and the platform calls stay
  in a library about drawing.
- **Rejected because**: it hides the dependency rather than removing it.

### A Vulkan instance for each window
- **For**: each window and its renderer are independent.
- **Against**: an instance is per process in Vulkan's design, a second window doubles it, and
  objects from two instances cannot be shared.
- **Rejected because**: one instance is what Vulkan expects, and the headless renderer already
  makes one with no window.

## Consequences

- **Gains**: `engine` and a windowless tool that links it build without Vulkan. The ui shell
  depends on the window alone. A second window, or a window outside the game loop, needs no new
  instance.
- **Costs**: one more api library. The surface is the renderer's, so the renderer has to be
  released before the window is destroyed, and nothing in the types enforces that order.
- **Revisit when**: a second graphics API or a second platform layer is added, which would make
  the window name what it can present with.
