# How input is routed

This document is for someone changing `api/ui` itself. It covers how the cursor, the keys and
focus reach a component, and the shell's keyboard. [README.md](README.md) covers the classes
and the frame, and this document assumes it.

## Cursor

`input::Cursor` holds the `Interface`, the dispatcher, an optional `Measure`, and two weak
pointers: the component a press is held on, and the component hovered. They are weak so that
a container unloaded mid-press is not kept alive.

`press(point)`:

1. Clears the focus.
2. Offers the point to each visible container in document order, stopping at the first that
   takes it. Within a container:
   - each menu bar's `press()`, then each toolbar's `press()`
   - then `Container::pick()`
3. On a picked component: holds it, gives it the focus through `Interface::focus()` (which
   ignores a component that is not focusable), and calls `act()`.

`act()` is a switch on the component type:

- `SelectList`: selects the row under the point, then dispatches.
- `TabBar`: selects the tab under the point; sends nothing.
- `Scrollbar`: jumps the thumb to the point; sends nothing.
- `Slider`: jumps the thumb to the point; dispatches if the value changed.
- `TextBox`: places the caret and anchor at the point through `TextBox::at()` and the
  `Measure`; sends nothing.
- every other type: dispatches.

`dispatch()` asks `input::command()` for the event and sends it through `input::send()`.

`motion(point)`: if a press is held, calls `follow()` and returns true. `follow()` is a switch
naming the components a press drags: a scrollbar, a slider (dispatching when the value
changes), and a text box (extending the selection). Otherwise every strip is told about the
motion so that a button the cursor left stops showing hover, and then the tree is picked.
`hover()` lights the newly hovered button and restores the old one. A disabled button's state
is never changed.

`release(point)` calls `follow()` one last time and drops the held component. It sends
nothing.

## Keys

`input::Keys::press(key, shifted, controlled)`:

1. Returns false when nothing is focused.
2. Clears the focus and returns false when the focused component is `usable()` but not
   `Interface::reachable()`: it was hidden or removed while it held the focus, so the key goes on
   to the app.
3. `tab` calls `Interface::focusNext(!shifted)` and returns true.
4. `escape` clears the focus and returns true.
5. Returns false when the focused component is not `usable()`. Tab and escape are handled
   first, so the focus can always leave a disabled component.
6. Calls `act()`, a switch on the focused component's type:
   - `TextBox` goes to `edit()`.
   - `SelectList` to `choose()`, `TabBar` to `turn()`, `Scrollbar` to `nudge()`, `Slider`
     to `slide()`.
   - Every other type falls out of the switch. Then a control chord returns false, a key
     other than `return` or `space` returns false, and `return` or `space` sends
     `input::command()` and returns true.

`choose()` and `turn()` share `step()`, which moves an index without wrapping. Only `edit()`
takes a key that composes text. It recognises such a key by its name being one character
long, because `api/input` names a key one character wide when it types a character and with
a word when it does not.

`Keys::text(utf8)` inserts into the focused component only when `traits(type).text` is set and
the component is usable. A usable component that is not reachable gives up the focus, as in
`press()`. It returns true even when the insert was refused.

A `TextBox` sends its command from `edit()` on `return`, reading `box->event()` directly.
`input::command()` returns no event for a `TextBox`, so a click into a box never submits it.
Every other key reaching a `TextBox` goes to `input::edit()` on the box's `LineEdit`, which is
the same function `Immediate::press()` calls. The editing rules therefore have one
implementation for both ui models.

## Focus

`Interface::focus(component)` writes `Component::focused()` on the old and new components, then
calls the `onFocus()` listener if the focus changed. The flag lives on the component so that
painting a component reads only the component. The engine holds the focused component as a
weak pointer.

`Interface::tabOrder()` builds the list that `focusFirst()` and `focusNext()` move through, by
walking every visible container with `forEachDrawn()` and collecting usable, focusable
components. Strips' buttons and menu items are not children, so they are not in it.

## Shell keyboard

`shell::Keyboard` turns `SDL_EVENT_KEY_DOWN` into `Keys::press()` with the name from
`input::keyName()` and the modifiers from the event, and `SDL_EVENT_TEXT_INPUT` into
`Keys::text()`. It registers `follow()` with `Interface::onFocus()`, and `follow()` turns
`Window::textInput()` on exactly while the focused component's traits say it takes text, or
the immediate layer it was offered has a field with the keyboard. It listens to the layer
through `Immediate::onTyping()`. It gives both listeners back in its destructor. A second
text-taking component type needs only its `traits().text` flag set.

The retained ui is offered each key first, through `Keys`, and the immediate layer gets only
what `Keys` returned false for. With no `ui::Interface`, `Keys` takes nothing, so every key goes to
the layer.
