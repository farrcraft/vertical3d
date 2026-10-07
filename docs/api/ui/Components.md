# Components

The components a ui document can name, and two properties every component has: whether it is
enabled, and whether it clips what it draws.

## Components

The `type` string in the document, the class in `v3d::ui::component`, and the keys each type
reads besides the common ones:

| `type` | Class | Draws | Keys | Pickable / focusable |
|---|---|---|---|---|
| `panel` | `Panel` | a filled box with a border, optionally rounded | none | no / no |
| `label` | `Label` | its text, wrapped to the width it was given, or one line when the width is `Auto` | `label` | no / no |
| `icon` | `Icon` | an image at the component's size | `source` (required) | no / no |
| `bar` | `Bar` | a track and the filled fraction of it | `fraction` (0 to 1), `direction` (`vertical`, else horizontal) | no / no |
| `button` | `Button` | a label, an icon, or a nine-slice skin | `label`, `icon`, `toggle`, `command`, `context` | yes / yes |
| `checkbox` | `CheckBox` | a square mark and a label beside it | `label`, `checked`, `command`, `context` | yes / yes |
| `radio` | `RadioButton` | a round mark and a label beside it | as `checkbox`, plus `group` | yes / yes |
| `scrollbar` | `Scrollbar` | a track and a thumb | `direction` (`horizontal`, else vertical), `content` and `page` (together), `offset` | yes / yes |
| `list` | `SelectList` | a plate and as many rows as fit, the chosen row highlighted | `items` (array of strings), `selected`, `command`, `context` | yes / yes |
| `slider` | `Slider` | a track, the fill up to the value, and a thumb | `minimum` (0), `maximum` (1), `step` (0 for continuous), `value`, `command`, `context` | yes / yes |
| `tabs` | `TabBar` | a strip of tabs, and the selected page under it | `selected`; its `children` are `tab` entries | yes / yes |
| `tab` | `TabPage` | the components it holds | `label` | no / no |
| `textbox` | `TextBox` | a plate, one line of text, a highlight behind the selection, and a caret while focused | `text`, `placeholder`, `limit` (bytes), `command`, `context` | yes / yes |
| `hbox`, `vbox` | `HorizontalBox`, `VerticalBox` | nothing; they place their children | `spacing`, `stretch`, `wrap` | no / no |
| `toolbar` | `Toolbar` | a row or column of buttons at an edge | `edge` (`top` or `left`), `buttons` (array of button entries) | yes / no |
| `menubar` | `MenuBar` | a strip of menu labels that drop panels | `menus`: array of `{ "label", "items" }` | yes / no |
| `menu` | `Menu` | a game menu, one level at a time, centred | `items` | no / no |

Notes on individual components:

- **Natural sizes.** A label's is its text width and one line. A label given a width wraps
  to it and its natural height is the rows it takes. An icon's is a square the height of a
  strip (`bar-height`). A check box's is the mark plus its label. A list's width is its widest
  row. A text box takes the width it is offered and the height of one line plus padding. A
  slider takes the width offered and the height of its thumb (`mark-size`). A scrollbar is
  `scrollbar-width` thick and as long as the room it is in.
- **A component does not own the state it shows.** A click on a check box, radio button or
  toggle button sends its command and changes nothing. The app answers the command and sets
  `checked()`. This keeps the mark in line with the app's state, however the command was
  sent. A radio button does not clear the others in its `group`; the app does.
- **These components do own their state:** a `SelectList` owns which row is chosen, a
  `Slider` its value, a `TabBar` which page is up, and a `TextBox` its text, caret and
  selection. The app reads them when the command arrives, for example `selected()` or
  `selection()` on a list and `text()` on a text box.
- **A `Scrollbar`** is arithmetic only. `scrolls(list)` binds it to a `SelectList`, and it
  then moves that list. Unbound, it holds its own `range(content, page)` and `offset()`, and
  the app reads `offset()` and moves whatever it scrolls itself. Placing it beside what it
  scrolls is the app's job either way. Binding is done in code; the document cannot bind
  one.
- **A `Slider`** key that is absent, is not a number, or is beyond the range of a float takes
  its default. The default `value` is the minimum.
- **A `SelectList`** with no `items` is filled by the app with `items(rows)`. It draws only
  the rows its box shows and clips them to its plate.
- **A `TabBar`** draws and lays out only the selected page. Components on other pages have no
  box that frame, so they are not picked or focused.
- **A `Toolbar`** button entry reads every button key, plus `name`, `style`, `visible` and
  `enabled`. A `name` that is not a string is ignored, and the button has no name. A button
  with an `icon` is sized to the icon; otherwise to its label. A toolbar button falls back to
  its label when its icon is not resolved. A hidden button takes no room in its strip, is not
  drawn and takes no press, and the buttons after it close up.
- **A `Menu` or `MenuBar` item** has a `label`, a `type`, and optionally `command` and
  `context`. Item types are `action`, `submenu` (which holds its own `items`), `check`,
  `radio`, `input`, `numeric_input` and `key_input`. A check and a radio item are marked by
  the app, the same as a check box. Radio items are drawn with the same square mark as check
  items.
- **An open menu bar takes every press.** While a menu is open, a press anywhere on screen
  closes it and is consumed. A click meant for the scene under an open menu does nothing.

## Enabled and disabled

`Component::enabled(false)` marks a component that cannot be used right now. It is a property
of the component, not a hover or press state. It differs from `pickable()`: `pickable(false)`
means "scenery, never clickable", such as a label or a panel.

**Disabling a component disables everything it holds.** A box is how a screen greys out a
group of controls. A disabled toolbar disables its buttons, and a disabled menu disables its
items. A disabled submenu item disables the submenu it opens, and the keyboard does not open
it.

`enabled()` is the component's own flag. `ui::usable(component)` is true only when the
component and everything holding it are enabled. A component can report `enabled()` true while
`usable()` is false. Every part of the library checks `usable()`.

A disabled component, or one inside a disabled component:

| Area | Behaviour |
|---|---|
| mouse | is not offered the point, so the press falls through to what is under it |
| hover | its button state is left alone |
| tab order | is skipped |
| focus | cannot be given the focus |
| keys and characters | are not taken, so they reach the app's bindings |
| toolbar | its button neither lights up nor sends |
| menu bar and menu | its item neither lights up nor sends, and is written in `disabled-text` |
| drawing | is never lit, writes its text in `disabled-text`, tints an icon with it, and draws no focus ring |
| state | keeps whatever state it had |
| document | `"enabled": false` on any component; `"state": "disabled"` on a button style |

A component disabled while it has the focus keeps `focused()` until something moves the
focus. It draws no ring and takes no keys, but `Engine::focused()` still returns it and
`onFocus()` is not called. `tab` and `escape` still move the focus off it.

## Clipping

A component with `"clip": true` (or `clip(true)` in code) cuts off what it holds at its own
box. Clipping is off by default, because a menu drops a panel outside its strip and a badge
may sit half outside its plate. A `SelectList`, an `Immediate` window and an `Immediate`
table given a height clip themselves.

- A clip inside a clip can only shrink the visible area.
- A clip is a rectangle. A panel with rounded corners clips its children to its box, not to
  its curve. No theme in this tree rounds a clipped panel.
- Clipping affects drawing only. A child clipped out of sight can still take a press where
  its box is.
- Each clipped region is drawn as a separate batch, so clipping costs a draw call per
  region.

[rendering/](../rendering/README.md) covers how the canvas carries a clip.
