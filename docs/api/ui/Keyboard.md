# The keyboard

Keyboard focus, tab order, keys versus typed characters, and text editing in a `TextBox`.

## The keyboard

`v3d::ui::input::Keys` routes keys to whichever component has the focus. `shell::Keyboard`
builds one and calls it, so most apps never call it directly.

### Focus

The **focus** is the one component that receives keys. `ui::Engine` holds it.

- A ui with nothing focused takes no keys. A game's movement keys keep working until
  something is clicked into or `focusFirst()` is called.
- A press moves the focus. It lands on the component pressed if that component is
  `focusable()`. Otherwise the focus is cleared. Clicking into a text box therefore means
  "type here", and clicking elsewhere stops typing.
- `escape` clears the focus.
- A component hidden while it has the focus, or whose container or tab page is hidden, gives
  the focus up on the next key or character, which then goes on to the app's bindings. That
  includes `tab` and `escape`. A dialog
  that closes over a focused text box therefore does not keep taking the game's keys.
  `Engine::reachable(component)` answers whether a component is still in the tab order.
- A component disabled while it has the focus keeps it, takes no keys, and lets them through.
- `Engine::focused()` returns the focused component. `Engine::onFocus(callback)` reports
  every change, in the same frame.
- A focused component is drawn with a ring around its box, in the `focus` colour at
  `focus-width` thickness of its style class.

**Starting a keyboard-driven screen.** Call `Engine::focusFirst()` as the screen goes up. It
puts the focus on the first focusable component. Nothing else gives out a first focus except
a press: `tab` does nothing while nothing is focused, so a HUD nobody is looking at never
takes the keyboard. No app in this tree calls `focusFirst()`; it is covered by
`api/ui/tests` only.

### Tab order

`tab` moves the focus to the next focusable component, and `shift`+`tab` to the previous
one. The order is the draw order:

- containers in the order the document lists them
- components by `depth`, keeping listed order between equal depths
- a flow box's children in the order it holds them

The order wraps at each end. Hidden and disabled components are skipped, with everything they
hold. If the focused component was disabled, `tab` restarts at the first component. One that was
hidden or removed gives the focus up instead, as above. To change the tab order, reorder the document. There is no
tab index.

`tab` is taken whenever something is focused, even if the focus cannot move.

Every control is focusable by default. To leave one out of the tab order, set
`"focusable": false`.

### Keys and characters

The ui receives two kinds of keyboard input:

| Call | Receives | Example |
|---|---|---|
| `Keys::press(key, shifted, controlled)` | a key name from `api/input`, and whether shift and control are held | `"backspace"`, `"arrow_left"`, `"return"`, `"tab"` |
| `Keys::text(utf8)` | characters the platform composed, as UTF-8 | `"e"`, `"É"` |

Characters arrive with shift already applied. A dead key and the key after it arrive as one
character. An input method's keys arrive as whatever characters it produced. `input::Keyboard`
also raises these characters as `event::TextInput` events for anything else that wants them.

The modifiers are passed in because a key name carries none, and `api/ui` cannot read SDL's
keyboard state.

### What each control does with a key

| Focused | Keys it takes |
|---|---|
| `Button`, `CheckBox`, `RadioButton` | `return` and `space` send its command |
| `SelectList` | `arrow_down`, `arrow_up`, `home`, `end` move the chosen row and send the command |
| `TabBar` | `arrow_right`, `arrow_left`, `home`, `end` change the page |
| `Scrollbar` | arrows along its direction move it by a line; `pageup` and `pagedown` by a page; `home` and `end` to the ends |
| `Slider` | `arrow_right`, `arrow_left` by one step; `pageup`, `pagedown` by a tenth of the range; `home`, `end` to the ends; sends its command when the value changes |
| `TextBox` | editing keys, and every key that types a character; see [Editing text](#editing-text-in-a-textbox) |

Rules:

- **Only a `TextBox` takes letters.** A letter reaching a focused button goes on to the
  app's bindings. A text box takes every key that will also arrive as a character, so typing
  "w" into a box does not also walk the player forward.
- A list and a tab bar do not wrap at the ends. Pressing an arrow on a list with nothing
  chosen chooses the first row.
- A scrollbar's line is the row height of its bound list, or `Scrollbar::lineStep` pixels
  for a range of its own. A scrollbar with nothing to scroll takes no keys.
- A slider with no `step` moves by a hundredth of its range per arrow.
- A key that moves nothing (an arrow at the end of a slider or scrollbar) is not taken, and
  goes on to the app.
- With control held, only a `TextBox` takes a key, and only the four editing chords. Every
  other chord goes on to the app, so `ctrl`+`s` still saves while somebody is typing.

### Game menus use commands, not focus

A game's pause menu (`Menu` and `GameMenu`) does not use focus or `Keys`. It is driven by the
`menuNext`, `menuPrevious`, `selectMenu` and `showGameMenu` commands that an app's bindings
send. A game can therefore bind its menu to a gamepad or any keys, and rebind them from inside
the menu. Menu items are not focusable. The two models meet only where a menu item captures a
key, which the app routes through `GameMenu::capture()`.

## Editing text in a TextBox

A `TextBox` holds one line of UTF-8 text, a **caret** (where typing goes) and an **anchor**.
Both are byte offsets. The selection is the run of text between the anchor and the caret.
Nothing is selected when the two are equal.

Every operation that moves the caret either brings the anchor with it, which leaves nothing
selected, or leaves the anchor behind, which selects the run the caret crossed.

| Input | Effect |
|---|---|
| a character | replaces the selection, or is inserted at the caret |
| `backspace`, `delete` | remove the selection, or one character before or after the caret |
| `arrow_left`, `arrow_right`, `home`, `end` | move the caret; with nothing held, an arrow over a selection lands on its end |
| `shift` + any of those | move the caret and leave the anchor, extending the selection |
| `return` | sends the box's command; the app reads `text()` |
| `control` + `a` | selects all |
| `control` + `c` | copies the selection |
| `control` + `x` | copies the selection and removes it |
| `control` + `v` | replaces the selection with the clipboard's text |
| a press | puts the caret and the anchor at the character under the point |
| a drag | moves the caret with the cursor, selecting from where the press landed |

- `limit` caps the text's length in bytes. A character or paste that would pass it is
  refused, but the key is still taken.
- The line slides left when the caret would pass the far edge, so a full box can still be
  typed into.
- `placeholder` is shown in the `placeholder` colour while the box is empty.
- The clipboard is a pair of callbacks, `Keys::Clipboard { read, write }`. `shell::Keyboard`
  supplies SDL's. Without a `write`, a cut does not remove the text. Without a `read`, a
  paste inserts nothing.
- A copy or a cut with nothing selected does nothing, so the clipboard keeps what it had.
- Placing the caret by clicking needs the `Measure` passed to `Cursor`.
