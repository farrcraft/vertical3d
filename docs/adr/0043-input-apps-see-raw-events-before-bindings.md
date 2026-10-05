# ADR-0043: Input: apps see raw events before bindings

**Status**: amended
**Date**: 2026-09-07
**Amended by**: [ADR-0081](0081-input-key-events-and-commands-are-separate.md)
**Documented in**: [api/Engine.md](../api/Engine.md)

## Context

The engine's loop offers each SDL event to the input engine, which maps it to a command through
the app's bindings, and then handles quit, resize and focus itself. An app sees no raw event. A ui
toolkit the app did not write, such as Dear ImGui or RmlUi, needs the raw events and has no polled
mode. Polling the keyboard state is not a substitute, because a press and a release inside one
frame poll as no change.

## Decision

An app overrides `Engine::onEvent(const SDL_Event&)` to be offered every event first, and
returning true consumes the event so the input engine never maps it. The engine handles quit,
resize and focus whatever `onEvent` returns, and skips them only for an event the input engine
itself consumed.

## Alternatives

### Make `eventLoop()` virtual and let an app write its own
- **For**: no new concept, since an app already subclasses `Engine`.
- **Against**: the app copies the fixed-step drain
  ([ADR-0032](0032-loop-fixed-step-simulation-variable-rate-rendering.md)), the timing and the
  quit check to change three lines in the middle.
- **Rejected because**: the loop is the part worth inheriting.

### Bindings first, and the app sees what they did not take
- **For**: an app cannot accidentally starve a binding that has always worked.
- **Against**: the app draws over the scene, so it is what the cursor points at. A click on a
  button the app drew would also fire the command bound to that click.
- **Rejected because**: it inverts the layering. `ui::Cursor::press()` applies the same
  front-to-back rule inside the ui ([ADR-0038](0038-ui-the-ui-hit-tests-the-mouse-before-the-app.md)).

### Window handling first, before the app sees anything
- **For**: an app cannot swallow a close request, even by returning true for everything.
- **Against**: it rules out a veto an app may want later, such as asking about unsaved work on a
  close request.
- **Rejected because**: handling window events regardless of what the app returns gives the same
  safety and keeps the veto possible.

## Consequences

- **Gains**:
  - An app can host a ui toolkit it did not write and still use the engine's loop, so adopting
    the engine and replacing a ui are separate pieces of work.
  - The default consumes nothing, so an app that does not override it sees no change.
  - The order is tested through `Engine::route()`, which handles one event without rendering.
- **Costs**:
  - An app that returns true for everything disables its own bindings, and nothing reports it.
  - Input can be answered in two places, and which to use is a judgement. A command that can be
    a binding should be one.
  - Work in `onEvent` runs inside the poll loop, so a slow handler shows as input latency rather
    than frame time.
- **Revisit when**: an app needs to veto a close request, or a toolkit needs events the engine
  handles first.
