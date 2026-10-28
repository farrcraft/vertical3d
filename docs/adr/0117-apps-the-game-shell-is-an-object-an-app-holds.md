# ADR-0117: Apps: the game shell is an object an app holds

**Status**: accepted
**Date**: 2026-10-26
**Documented in**: [docs/api/engine/Shell.md](../api/engine/Shell.md)

## Context

Every game builds the same things around its own code. They are the ui engine and its document,
the screen its text and ui are drawn on, the pause menu, and the statistics overlay with the
command that shows it. The editor builds most of them too, with a different menu. `engine` and
`ui` are siblings, so neither can hold the code that joins them. An app that wants a different
shell, or none, has to be able to leave a part out.

## Decision

A library above `engine`, `ui` and `render`, `shell`, holds a `Shell` object that an app
constructs, starts and releases. Each part it builds is an option, so a game takes the menu and
the editor does not. An app keeps subclassing `engine::Engine`, and the shell takes the engine
it serves.

## Alternatives

### A game base class that derives from the engine
- **For**: an app writes one subclass and gets everything, with no object to hold.
- **Against**: an app that wants a different shell has to work around the start() and release()
  of a base that already built one, and a part cannot be left out without a flag for it.
- **Rejected because**: a shell made of parts the app holds is what an app leaving one out needs.

### The shell in `ui::shell`, with `ui` depending on `engine`
- **For**: no new library, and the shell's parts already live there.
- **Against**: `ui` would need the game loop to build a menu, and a tool using the ui without
  the engine would link it anyway.
- **Rejected because**: it adds a dependency between siblings instead of joining them above.

### The shell in `engine`, with `engine` depending on `ui`
- **For**: an app gets the shell from the engine it already subclasses.
- **Against**: `engine` would depend on `ui` and through it on `render`, which brings back the
  Vulkan dependency `engine` was freed of.
- **Rejected because**: the engine has to build without a renderer.

## Consequences

- **Gains**: a game's start() holds only what makes it that game. The editor uses the same
  shell with its own options. A part is left out by not asking for it.
- **Costs**: one more library. The shell's commands are declared in the app's constructor and its
  parts built in start(), so an app makes two calls at two points. The shell builds the screen
  over the app's renderer, so the shell is released before that renderer is.
- **Revisit when**: an app needs a part the shell builds in a way an option cannot express, which
  would make the parts worth exposing one by one.
