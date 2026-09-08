# ADR-0043: Event Routing — An App Sees An Event Before The Bindings Do, And The Window Facts Are Not Its To Decline

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`engine::Engine::eventLoop()` polled SDL and offered every event to exactly two places: the
input engine, which turns it into a config-bound command through `mappings.json`, and a private
`handleEvent`, which answers quit, resize and focus. Both are inside the class, so a subclass
saw no raw `SDL_Event` at all.

That is a complete input model, and for an app whose input *is* named commands it is the better
one — a binding is a document rather than a switch statement. But it is the only one, and an app
embedding a ui toolkit it did not write cannot use it. Dear ImGui's SDL3 backend, RmlUi and
anything else of that shape want the events themselves; none has a polled mode, and polling
`SDL_GetKeyboardState` instead is not the same information, because a press and a release inside
one frame poll as nothing having happened.

So the loop could host only apps that had adopted its bindings wholesale — which is a hard
thing to adopt *while* the toolkit being replaced is still present and still needs events.

## Decision

`Engine::onEvent(const SDL_Event&)` is a virtual an app overrides to be offered every event
first, and returning true consumes it so the input engine never maps it. The engine's own
handling of quit, resize and focus runs whatever the app returns.

## Alternatives Considered

### Alternative 1: `eventLoop()` becomes virtual and an app writes its own
- **Pros**: no new concept; an app already subclasses `Engine`
- **Cons**: the app copies the accumulator drain, the nanosecond timing and the quit check —
  everything ADR-0032 settled — to change three lines in the middle
- **Why not**: the loop is the thing worth inheriting. A seam that is only reachable by
  replacing the loop is not a seam.

### Alternative 2: The bindings go first, and the app sees what they did not take
- **Pros**: an app cannot accidentally starve a binding that has always worked
- **Cons**: exactly inverts the layering. The app drew *over* the scene, so it is what the
  cursor is pointing at; a click on a button the app drew would fire the command bound to that
  click as well
- **Why not**: it is the bug this exists to prevent, and `ui::Cursor::press()` already applies
  the opposite rule one layer in (ADR-0038).

### Alternative 3: `handleEvent` runs first, before the app sees anything
- **Pros**: an app cannot swallow a close request even by returning true for everything
- **Cons**: forecloses a veto an app legitimately wants later — unsaved work asking "are you
  sure" on a close request
- **Why not**: the same safety comes free from running `handleEvent` *unconditionally* rather
  than *first*, which keeps the veto available as a later, deliberate change.

## Consequences

### Positive
- An app can host a ui toolkit it did not write without giving up the loop, which is what makes
  adopting the engine and replacing a ui two separate pieces of work rather than one.
- The default takes nothing, so the change is additive: the four apps in this tree do not
  override it and see the events they always saw.
- The order is testable. The inner loop body is now `Engine::route()`, which does not render and
  so can be driven from a test; `eventLoop()` still cannot be.

### Negative
- An app that returns true for everything silently disables its own bindings, and nothing
  reports that. It is the same failure `ui::Cursor` has and is diagnosed the same way.
- There are now two places input can be answered, and which one an app should use is a judgment
  rather than a rule. A command that could be a binding should be one.

### Risks
- An app doing real work in `onEvent` does it inside the poll loop, so a slow handler shows up
  as input latency rather than as a frame time. Mitigated by what the seam is for: a toolkit's
  `ProcessEvent` is a switch and a struct write.
