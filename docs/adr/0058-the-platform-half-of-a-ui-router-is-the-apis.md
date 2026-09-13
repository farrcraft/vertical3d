# ADR-0058: UI Input — The Platform Half Of The Keyboard Router Is The api's, And Text Input Follows The Focus

**Date**: 2026-09-13
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0040](0040-a-key-goes-to-a-focused-component.md) gave `api/ui` a keyboard router that
names no platform type: `Keys::press()` takes a key name and `Keys::text()` takes utf-8, which
is what keeps the library testable without a window. It left an app four things to do before
any of that runs — decode the `SDL_Event`, read shift and control off it, find a clipboard, and
get the platform composing characters at all — and recorded the cost as two of its own negative
consequences: that `Cursor` and `Keys` are two objects an app has to hold with nothing enforcing
both, and that `SDL_StartTextInput` was called for the life of the window because nothing was
following the focus.

Nothing in this tree had written that wiring. `ui::Keys` was constructed only in
`api/ui/tests`, `event::TextInput` had no listener outside `api/input`, and the editor built a
`ui::Cursor` with no keys, no measure and no clipboard. So the routing was proven and the seam
was not, which is the arrangement where the library looks finished and the first app to want a
text box discovers otherwise.

## Decision

`ui::shell::Keyboard` is the platform half: it holds a `ui::Keys`, turns an `SDL_Event` into the
two calls that router takes, supplies the SDL clipboard, and starts and stops text input as the
focus reaches a text box and leaves it — following `ui::Engine::onFocus()` rather than polling,
because the focus also moves under a press it never sees. The key name comes from
`input::keyName()`, so one table serves both the binding and the box.

## Alternatives Considered

### Alternative 1: Each app writes the decode, as ADR-0040 left it
- **Pros**: `api/ui` names no `SDL_Event` at all, and the seam is whatever each app needs
- **Cons**: it is the same switch, the same two modifier masks and the same clipboard pair in
  every app, and each copy is a place the key names can drift from `mappings.json`
- **Why not**: [ADR-0028](0028-an-apps-shell-belongs-to-the-api.md) already decided this class
  of question, and SDL3 is public on every `v3dlib_ui` consumer through `v3dlib_render`, so
  writing it once costs no dependency that was not already there.

### Alternative 2: The seam routes the mouse too, so an app holds one object rather than two
- **Pros**: closes ADR-0040's "two objects and two calls" outright
- **Cons**: a press has to interleave with whatever else an app does with one — the editor
  offers the ui a press and drives a camera with the one the ui did not take — so consuming
  mouse events in `onEvent` would decide that ordering for every app
- **Why not**: `ui::Cursor` takes points rather than events for that reason, and the object
  count is a smaller cost than taking the ordering away.

### Alternative 3: Text input is turned on and off by the text box, or checked per event
- **Pros**: no callback on `ui::Engine`, and the component that needs composing is what asks
- **Cons**: a component owns no window, and checking per event is a frame late — a box clicked
  into and typed into in the same frame loses its first character, because the press that
  focused it is not an event this seam sees
- **Why not**: the focus is the engine's, so the engine is what can say it moved, and saying so
  is cheaper than every other way of finding out.

### Alternative 4: `Keys` listens for `event::KeyDown` on the dispatcher instead
- **Pros**: no SDL anywhere near `api/ui`, and the events already carry the api's key names
- **Cons**: `input::Keyboard` triggers `KeyDown` and the mapper's source event in one call, so a
  listener cannot take a key before the bindings turn it into a command
- **Why not**: a key that both edits a box and fires a command is the bug
  [ADR-0043](0043-an-app-sees-an-event-before-the-bindings-do.md) put the app ahead of the
  bindings to prevent.

## Consequences

### Positive
- An app's `onEvent()` is one line, and it is the same line in every app.
- One key name table. A binding written against "backspace" and a text box answering it cannot
  disagree, because `input::keyName()` is what both read.
- `SDL_StartTextInput` is no longer on for the life of the window, which was ADR-0040's
  consequence and is now paid: on a platform with an on screen keyboard it rises with a text
  box and lowers with it.
- `Engine::onFocus()` is useful to more than this — an app wanting to show what the keyboard is
  on, or to suspend something while a field is being typed into, has the same hook.
- A release is never consumed, so a key held as a box took the focus is still seen to come up
  and `input::KeyState` cannot be left holding it down.

### Negative
- `v3dlib_ui` now links `v3dlib_input`, for one function. The alternative was a second copy of
  the key table, which is the thing worth avoiding, but it is a dependency the library did not
  have and the manifest in `cmake/v3dApiLibraries.cmake` records it.
- A `ui::Keys` built directly still needs its clipboard and its text input found by hand.
  `shell::Keyboard::clipboard()` is public for that reason, but nothing makes the direct route
  hard to take.
- `onFocus()` holds one listener and the last caller wins, so an app that wants the hook and the
  seam has to order the two. A list of listeners would not have this problem and is not worth
  the allocation until something needs two.
- The mouse is still the app's, so ADR-0040's "two objects, two calls" stands for the cursor.

### Risks
- **An app that overrides `onEvent` for something else has to remember to offer the seam the
  event**, and forgetting it is a ui that cannot be typed into with nothing reporting why. It is
  the failure ADR-0043 already named for `onEvent` generally, diagnosed the same way.
- **Text input follows the focus, so a component that is typed into but is not a `TextBox` gets
  no characters.** `shell::Keyboard::follow()` is the one place that decides, and a second such
  component is a line there rather than a new mechanism.
