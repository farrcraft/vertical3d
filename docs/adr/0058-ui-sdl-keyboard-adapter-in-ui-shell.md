# ADR-0058: UI: SDL keyboard adapter in ui/shell

**Status**: accepted
**Date**: 2026-09-13
**Amends**: [ADR-0040](0040-ui-keyboard-focus-and-text-input.md)
**Documented in**: [The User Interface](../api/UserInterface.md), [User Interface Internals](../internals/UserInterface.md)

## Context

`ui::input::Keys` takes key names and UTF-8 text, and names no platform type, so the library is
testable without a window. Before it runs, an app has to decode the `SDL_Event`, read the shift
and control state, supply a clipboard, and turn the platform's text input on. Text input should
be on only while something that takes text has the focus, because on some platforms it raises
an on-screen keyboard. The focus also moves under a mouse press, which the keyboard path never
sees. A binding and a text box need to agree on key names.

## Decision

`v3d::ui::shell::Keyboard` is the platform half of the keyboard: it holds a `ui::input::Keys`,
turns each `SDL_Event` into the calls `Keys` takes, and supplies the SDL clipboard. It turns
text input on and off as the focus reaches and leaves a component that takes text, by listening
to `ui::Engine::onFocus()` rather than checking per event. Key names come from
`input::keyName()`, so bindings and the ui read one table.

## Alternatives

### Each app writes the decode itself
- **For**: `api/ui` names no `SDL_Event`.
- **Against**: every app writes the same switch, the same modifier masks and the same clipboard
  pair, and each copy can drift from the binding key names.
- **Rejected because**: shared app wiring belongs in the api, per
  [ADR-0028](0028-apps-the-shared-app-shell-lives-in-the-api.md), and SDL3 already reaches every
  `v3dlib_ui` consumer through `v3dlib_render`.

### The adapter routes the mouse too, so an app holds one object
- **For**: one object and one call for all ui input.
- **Against**: apps interleave a press with their own handling. The editor offers a press to the
  ui and drives a camera with the press the ui did not take. Consuming mouse events inside the
  adapter would fix that order for every app.
- **Rejected because**: `ui::input::Cursor` takes points rather than events so that each app
  keeps control of that order.

### The text box turns text input on and off, or it is checked per event
- **For**: no callback on `ui::Engine`, and the component that needs text asks for it.
- **Against**: a component owns no window. Checking per event is late: a box clicked and typed
  into in the same frame loses its first character, because the adapter never saw the press
  that focused it.
- **Rejected because**: the engine owns the focus, so the engine is what can report that it
  moved.

### `Keys` listens for key events on the dispatcher
- **For**: no SDL near `api/ui`, and the events already carry the api's key names.
- **Against**: the adapter still needs the window to switch text input with the focus, so it
  already sits where the SDL event is. When this was decided, a listener could not take a key
  before the bindings did. [ADR-0081](0081-input-key-events-and-commands-are-separate.md) since
  made that possible.
- **Rejected because**: reading the SDL event in the adapter that already holds the window
  needs no extra wiring. Nothing has needed the dispatcher route since ADR-0081 made it viable.

## Consequences

- **Gains**:
  - An app's keyboard wiring for the ui is one call in its event handler, the same in every app.
  - One key name table serves bindings and the ui.
  - Text input follows the focus, so an on-screen keyboard rises and falls with a text box.
  - `Engine::onFocus()` is available to anything else that needs to know where the keyboard is.
  - A key release is never consumed, so `input::KeyState` never sees a key stuck down.
- **Costs**:
  - `v3dlib_ui` links `v3dlib_input`, for the key name table.
  - `onFocus()` holds one listener, and the last caller replaces the others.
  - The mouse is still the app's, so an app holds a `Cursor` and a `Keyboard`.
  - An app that overrides its event handler must remember to pass events to the adapter, or the
    ui cannot be typed into, with no error.
  - Only components whose type is marked as taking text receive characters.
- **Revisit when**: a second listener needs `onFocus()`, or an app needs the keyboard path
  without SDL.
