# ADR-0152: Editor: scripts call named commands with typed arguments

**Status**: accepted
**Date**: 2026-11-02
**Documented in**: docs/editor/Scripting.md

## Context

A console, a script run against a project without a window, and later plug-ins all drive the
editor from text. The menus and keys already reach the document's operations through named
commands, whose handlers read what to act on from the selection, the frame and the active view.
Text has to name what to act on itself, and what it ran should be shown in a form that can be run
again. Maya, Modo, Blender and Houdini each keep a layer of named commands with arguments under
their scripting language, and log every action in that form.

## Decision

Every command a script can run is named and declares typed arguments. A menu or key sends it
with no arguments and its handler fills them from the context, while a script or a console line
passes them. The console lists each command run, from any source, in the form a script would
write it.

## Alternatives

### The scripting language bound straight to the document's operations
- **For**: no command layer to declare, and a script reaches every operation the moment it exists.
- **Against**: the menus and the scripts reach the document by separate paths that can drift
  apart, and a menu action has no textual form to log or replay.
- **Rejected because**: one definition should serve a menu, a key, the console and a script.

### Named commands without arguments, run as a list
- **For**: the command registry runs them as it is, and a script is a list of names.
- **Against**: a command acts only on what the ui has chosen, and a script cannot branch, loop or
  name an item.
- **Rejected because**: a headless script has no ui to choose for it.

## Consequences

- **Gains**: what a user does in the editor can be read back as a script and run again. A script,
  a menu and a key reach an operation through the same command and the same refusal.
- **Costs**: each scriptable command declares its arguments, with a text form for items, parts
  and frames. A command a menu sends now has two sources of its arguments, and both are tested.
- **Revisit when**: plug-ins need to define commands of their own that the menus and keys bind to.
