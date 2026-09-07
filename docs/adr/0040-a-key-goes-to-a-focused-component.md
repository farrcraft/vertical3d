# ADR-0040: UI Input — A Key Goes To A Focused Component, And A Character Is Not A Key

**Date**: 2026-09-07
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0038](0038-a-cursor-is-routed-by-the-library-that-drew-it.md) gave the ui a cursor: a point
is offered to what was drawn, the first thing that takes it stops the walk, and a press
dispatches the component's command. It left the keyboard alone, and that is the one thing
stopping a text box being written — `TODO.md` records it as "the one missing component that
needs something the library does not have: a key goes to the app's input engine and nothing
routes one to a focused component".

Two things are missing rather than one. **Where a key goes**: `api/input` dispatches a
`KeyDown` to whoever is listening, and every listener hears every key, so there is nowhere for
"this one is being typed into a box" to live. **What a key carries**: `input::Keyboard` maps an
`SDL_Keycode` to a name — "a", "space", "return" — which is the right thing for a binding and
the wrong thing for a text box. The name cannot tell "a" from "A", cannot carry the character a
dead key and the one after it compose into, and cannot carry what an input method decided.

## Decision

**The focus is the ui's, a press moves it, and `ui::Keys` routes a key to whatever holds it —
the keyboard's `ui::Cursor`.** `ui::Engine` holds which component is focused because both
routers reach it; `Component::focusable()` is opt-in the way `pickable()` is, and
`Component::focused()` is the flag the engine writes so that drawing a component reads the
component.

**A character is a second kind of input, not a key.** `input::Keyboard` turns
`SDL_EVENT_TEXT_INPUT` into an `event::TextInput` carrying utf-8, and `Keys::text()` is what
puts it in. `Keys::press()` handles the keys that name an operation — backspace, delete, the
caret moves, return, escape — and also consumes the keys that will arrive again as characters,
so that typing "w" into a box does not also walk the player forward.

## Alternatives Considered

### Alternative 1: A focus on the ui, a `Keys` router, and text separate from keys — **chosen**
- **Pros**: The shape the library already has for the cursor, so there is one story for input
  rather than two: a router is handed what the app saw and answers whether the ui took it, and
  the component carries out what the router names. Text arriving composed is the only way a
  text box works outside ASCII, and it is free — SDL has already done it.
- **Cons**: Two entry points to keep in step, and an app must call both. A key that composes
  text is seen twice — consumed by `press()` and acted on by `text()` — which reads oddly until
  the reason is known.
- **Why not**: n/a — chosen.

### Alternative 2: Route a key by hit testing, the way a point is routed
- **Pros**: One mechanism for both, and no focus state at all.
- **Cons**: There is nothing to hit test against. A keyboard has no position, and "the
  component under the cursor" means a box stops taking what is typed into it the moment the
  mouse is moved off it.
- **Why not**: It is not what a keyboard is.

### Alternative 3: Let the app own the focus and tell the ui which component is focused
- **Pros**: The library holds no state it has to keep right, and an app that has its own idea
  of focus — a modal, a console — stays in charge of it.
- **Cons**: Every app writes the same code, and the press that ought to move the focus is
  already in `ui::Cursor`, so the app would have to ask the cursor what it picked and hand the
  answer back. That is the shell work [ADR-0028](0028-an-apps-shell-belongs-to-the-api.md) says
  belongs to the api.
- **Why not**: The press already knows. Making the app carry the answer across is work for
  nothing.

### Alternative 4: Build the text from key names and the shift state
- **Pros**: No new event, no `SDL_StartTextInput`, and the key names are already there.
- **Cons**: Correct for unshifted ASCII on a US layout and wrong everywhere else. Every
  accented character, every non-Latin script and every input method is out of reach, and the
  shift table would live in the ui rather than in the platform that owns it.
- **Why not**: It is a keyboard layout implementation, and the platform has one.

### Alternative 5: A `TextBox` that reads the raw SDL events itself
- **Pros**: Nothing to route: the component asks for what it needs.
- **Cons**: Puts SDL in `api/ui`, which is a library that today draws onto a cpu-side canvas
  and needs no window, no device and no font to test — the property that makes every case in
  `api/ui/tests` runnable in CI.
- **Why not**: It would cost the library its testability for one component's convenience.

## Consequences

### Positive
- `TextBox` is a component like any other: a type, a loader branch, a draw path, a natural
  size and a style class. It owns its text and its caret the way a `SelectList` owns its rows,
  and a return sends its command so that what the text *means* stays the app's.
- The caret is a byte offset into utf-8 that only ever lands on a character boundary, so an
  accented character is inserted, stepped over and erased whole.
- An app with nothing focused is unchanged: `Keys` takes no key, so a game's movement bindings
  go on working until something is clicked into.
- `event::TextInput` is the ui's today and anybody's tomorrow — a console, a chat line and a
  rename field all want composed characters rather than key names.

### Negative
- `SDL_StartTextInput` is called for the life of the window rather than as a box takes the
  focus. On a desktop that costs nothing; on a platform with an on-screen keyboard it would
  raise one and never lower it.
- A key that composes text is consumed while a box has the focus even when the box then refuses
  the character, so an app cannot bind a letter to anything that should work while typing.
- Nothing walks the tree for the next focusable component, so there is no tab order: the focus
  moves by press and by press alone.
- `Cursor` and `Keys` are two objects an app has to hold and two calls it has to make, and
  nothing enforces that it makes both.

### Risks
- **A press moves the focus before the component acts on it**, so a component that wanted the
  focus left alone cannot say so. The escape hatch is `focusable(false)`, which is the default.
- A caret cannot be placed by clicking: a press focuses the box and leaves the caret where it
  was. Doing it needs the `Measure` callback in `ui::Cursor`, which today names no text at all,
  and that is a change to what `Cursor` is rather than an addition to it.
- There is no selection, so there is no cut, copy or paste over a range. `insert()` takes a run
  of characters, so a paste is expressible the moment something delivers one.
