# ADR-0163: Apps: a picture is taken by the app from its command line

**Status**: accepted
**Date**: 2026-11-03
**Documented in**: docs/api/engine/Lifecycle.md

## Context

The documents show pictures of the editor and the demos, and a picture has to be made again
whenever an app changes. A picture taken from the screen records whatever window is in front, and
needs someone to bring the app to the state shown. The renderer can already write a presented
frame to a file, but only code inside an app can ask it to. The engine owns the loop and the quit,
and has no renderer. The editor quits only after asking about unsaved work, and it runs scripts
only one console line at a time.

## Decision

Every app built on the engine takes `--capture <file>` and `--capture-after <frames>` on its
command line. At that frame the engine asks whatever registered a capture to write the frame,
and quits once it is presented, without asking. The editor also takes `--script <file>`, a
TypeScript file of editor commands that runs once it has started, so a picture can show a scene
the script built.

## Alternatives

### An environment variable read by the renderer
- **For**: no code in any app, and every window renderer gains it at once.
- **Against**: configuration hidden inside a library, which shows only the state an app starts
  in, and leaves closing the app to whatever started it.
- **Rejected because**: it cannot put the editor in a known state, which most of its pictures
  need.

### A key bound in each app, pressed by a script
- **For**: a user can take a picture too, with no command line.
- **Against**: a script has to send key presses on timers to reach a state and take the picture,
  which breaks when the app is slower or faster than the timers.
- **Rejected because**: a picture has to come out the same on every run.

### A capture of the screen, taken from outside the app
- **For**: needs nothing from the app.
- **Against**: it records the window in front, which may not be the app, and it reaches a state
  only through input sent from outside.
- **Rejected because**: of both, which the plan for the documents rules out.

## Consequences

- **Gains**: a picture of any app is one command, and comes out the same on each run. The editor
  can run a script file, which users can use beyond pictures.
- **Costs**: every app's command line holds options most users never give. A capture is a frame
  count, not a time, so a scene that moves shows the state its frame rate reached. The quit after
  a capture skips the editor's question about unsaved work.
- **Revisit when**: the renderer can draw an app's frame with no window, so that a picture needs
  no window open on the desktop.
