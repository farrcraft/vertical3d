# ADR-0090: Input: commands are handled through one registry

**Status**: accepted
**Date**: 2026-10-17
**Documented in**: [docs/api/engine/Input.md](../api/engine/Input.md#commands)

## Context

A binding document names the commands its keys send, and an app handles each command by comparing
the event's name in its own listener. Nothing connects the two, so a binding to a command that no
code handles loads, fires and does nothing. A command can be handled in three places: an app's
listener, the shell's game menu, or a `held()` query in `simulate()`. The engine cannot see any of
them, so it cannot say which bindings are dead, and a test cannot either.

## Decision

`event::Commands` maps each command's `context::name` to its handler, and the engine owns one. An
app registers every command it handles, including the ones it reads with `held()`, in its
constructor, and the engine's single listener runs the handler an event names. After `start()`
the engine logs every binding whose command has no handler, and a test builds the app without a
window to check its bindings the same way.

## Alternatives

### A declared list of command names beside each listener
- **For**: no change to how an app handles a command, and a test reads the list
- **Against**: the list and the comparisons in the listener are written separately, and nothing
  stops them disagreeing
- **Rejected because**: the test would check the list, not the code that handles the commands

### Register commands in `start()`
- **For**: a handler can be registered beside the object it calls, after that object exists
- **Against**: `start()` needs a window and a device, so no test could see what an app registers
- **Rejected because**: the check would only run when someone starts the game

## Consequences

- **Gains**: a dead binding is a test failure and a line in the log. A key and a menu item that
  send the same command reach the same handler. The editor's command directory becomes the api's,
  and every app handles commands one way
- **Costs**: every app's input handling is rewritten. A handler registered in the constructor
  runs only after `start()`, so it may not use what `start()` builds until then. The shell's
  game menu is registered through a function the app calls rather than by the menu itself. An
  app suite compiles the app's engine class, and so links what the app links
- **Revisit when**: commands need to be registered or removed while the game runs, such as by a
  mod or a scripted level
