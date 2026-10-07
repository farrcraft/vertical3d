# ADR-0040: UI: keyboard focus and text input

**Status**: amended
**Date**: 2026-09-07
**Amended by**: [ADR-0058](0058-ui-sdl-keyboard-adapter-in-ui-shell.md)
**Documented in**: [api/ui/Keyboard.md](../api/ui/Keyboard.md), [internals/UserInterface.md](../internals/UserInterface.md)

## Context

`api/input` sends each key to every listener, so there is no place to say that a key belongs to
the control being typed into. A key reaches the app as a name such as "a", "space" or "return",
which suits a binding but not a text box. A name cannot tell "a" from "A", and cannot carry a
character composed from a dead key or chosen by an input method. `api/ui` has to stay testable
without a window, so it cannot read platform events itself.

## Decision

The ui holds a keyboard focus: `ui::Engine` records which component has it, a press moves it,
and `v3d::ui::input::Keys` routes each key to the focused component. Typed text arrives
separately, as a UTF-8 string from the platform's text input event (`event::TextInput` in
`api/input`), and `Keys::text()` delivers it. While a text box has the focus, the keys that
compose text are consumed, so typing does not also trigger the app's bindings.

## Alternatives

### Route a key by hit-testing, the way the cursor is routed
- **For**: one mechanism for both devices, and no focus state.
- **Against**: a keyboard has no position. Routing to the component under the cursor would stop
  a box taking typed text as soon as the mouse moved off it.
- **Rejected because**: keyboard input needs a target that persists.

### The app owns the focus and tells the ui which component has it
- **For**: the library holds no state, and an app with its own idea of focus stays in charge.
- **Against**: every app writes the same code. The press that should move the focus is already
  handled by `ui::input::Cursor`, so the app would ask the cursor what it picked and hand that
  back.
- **Rejected because**: the library already sees the press; passing the answer through the app
  adds work for nothing.

### Build text from key names and the shift state
- **For**: no new event, and the key names already exist.
- **Against**: correct only for unshifted ASCII on a US layout. Accented characters, non-Latin
  scripts and input methods are out of reach, and the ui would hold a keyboard layout table.
- **Rejected because**: the platform already implements keyboard layouts.

### A text box reads raw SDL events itself
- **For**: nothing to route.
- **Against**: it puts SDL in `api/ui`, which today draws onto a CPU-side canvas and is tested
  with no window, device or font.
- **Rejected because**: it gives up the library's testability for one component.

## Consequences

- **Gains**:
  - A text box works outside ASCII, because composition is the platform's job.
  - The keyboard follows the same pattern as the cursor: a router is given what the app saw and
    answers whether the ui took it.
  - An app with nothing focused is unaffected, so game bindings work until something takes the
    focus.
  - A screen of controls can be driven without a mouse.
- **Costs**:
  - Keys and text are two entry points, and something must feed both.
  - A key that composes text is consumed while a text box has the focus, even when the box
    refuses the character, so an app cannot bind a letter to anything that should work while
    typing.
  - A press moves the focus before the component acts, so a component cannot refuse the focus
    except by not being focusable.
- **Revisit when**: a component other than a text box needs typed text, or an app needs its own
  focus model.
