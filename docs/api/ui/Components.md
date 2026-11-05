# Components

The components a ui document can name, and two properties every component has: whether it is
enabled, and whether it clips what it draws.

## Components

The `type` string in the document, the class in `v3d::ui::component`, and the keys each type
reads besides the common ones:

| `type` | Class | Draws | Keys | Pickable / focusable |
|---|---|---|---|---|
| `panel` | `Panel` | a filled box with a border, optionally rounded, or a recess in the `tab` colour with no border | `recess` (false) | no / no |
| `label` | `Label` | its text, wrapped to the width it was given, or one line when the width is `Auto` | `label` | no / no |
| `icon` | `Icon` | an image at the component's size, or nothing until it has one | `source`; with none, the app gives it an image with `Icon::image()` | no / no |
| `bar` | `Bar` | a track and the filled fraction of it | `fraction` (0 to 1), `direction` (`vertical`, else horizontal) | no / no |
| `button` | `Button` | a label, an icon, or a nine-slice skin | `label`, `icon`, `toggle`, `tab` (`strip` or `choice`), `command`, `context` | yes / yes |
| `checkbox` | `CheckBox` | a square mark and a label beside it | `label`, `checked`, `command`, `context` | yes / yes |
| `radio` | `RadioButton` | a round mark and a label beside it | as `checkbox`, plus `group` | yes / yes |
| `scrollbar` | `Scrollbar` | a track and a thumb | `direction` (`horizontal`, else vertical), `content` and `page` (together), `offset` | yes / yes |
| `list` | `SelectList` | a plate and as many rows as fit, the chosen row highlighted | `items` (array of strings), `selected`, `command`, `context` | yes / yes |
| `slider` | `Slider` | a track, the fill up to the value, and a thumb | `minimum` (0), `maximum` (1), `step` (0 for continuous), `value`, `command`, `context` | yes / yes |
| `tabs` | `TabBar` | a strip of tabs, and the selected page under it | `selected`; its `children` are `tab` entries | yes / yes |
| `tab` | `TabPage` | the components it holds | `label` | no / no |
| `textbox` | `TextBox` | a plate, one line of text, a highlight behind the selection, and a caret while focused | `text`, `placeholder`, `limit` (bytes), `command`, `context` | yes / yes |
| `hbox`, `vbox` | `HorizontalBox`, `VerticalBox` | nothing; they place their children | `spacing`, `stretch`, `wrap` | no / no |
| `toolbar` | `Toolbar` | a row or column of buttons at an edge, with a faint rule between groups | `edge` (`top` or `left`), `buttons` (array of button entries, and `{ "type": "separator" }` for a rule before the next button) | yes / no |
| `menubar` | `menu::Bar` | a strip of menu labels that drop panels | `menus`: array of `{ "label", "items" }`, each optionally with `icon`, `drop` (false), `emphasis` (false), `accent` (false) and `named` (false, a title that shows its label after its icon too); `docked` (true); `opacity` (1, and a bar at 0 draws no rule either); `note` (text after the titles, in the secondary text colour) | yes / no |
| `menu` | `menu::Menu` | a game menu, one level at a time, centred in the safe area under its title | `items`, `title` | no / no |

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
- **A chosen tab is marked as the place shown, not filled as a setting that is on.** A
  `TabBar`'s strip is a recess in the `tab` colour. Its chosen tab is in the page's `panel`
  colour with a line in the `mark` colour under it, and its label is semibold. A toggle
  button given a `tab` look is drawn the same way while it is checked: `strip` as a
  `TabBar`'s tab, `choice` raised to the `chosen` colour, as one of a set of choices. The app
  draws the recess behind a row of such buttons, such as a `panel` with `recess` set. A tab
  under the cursor is raised halfway. Its label is measured semibold, so choosing it moves
  nothing. A plain toggle is filled with the `highlight` while it is checked.
  Background: [ADR-0167](../../adr/ui/0167-ui-a-chosen-tab-is-marked-as-a-place-not-a-toggle.md).
- **A `Toolbar`** button entry reads every button key, plus `name`, `style`, `visible` and
  `enabled`. A `name` that is not a string is ignored, and the button has no name. A button
  with an `icon` is sized to the icon; otherwise to its label. In a column, a button with an
  icon is square, as tall as the strip is wide, and one without is a line tall. A toolbar
  button falls back to its label when its icon is not resolved. A button's icon is tinted the
  colour its label would be drawn in, and a menu item's icon the colour of its label, so an icon
  drawn in white takes the theme's text colours. An `icon` component is not tinted, because it
  shows a picture. A hidden button takes no room
  in its strip, is not drawn and takes no press, and the buttons after it close up.
- **A `menu::Menu` or `menu::Bar` item** has a `label`, a `type`, and optionally `command`,
  `context` and `icon`. Item types are `action`, `submenu` (which holds its own `items`),
  `check`, `radio`, `input`, `numeric_input`, `key_input`, `slider` and `separator`. A check and
  a radio item are marked by the app, the same as a check box. Radio items are drawn with the
  same square mark as check items.
- **A `menu::Bar`'s title** is its menu's label, or the menu's `icon` once that is resolved,
  tinted the colour the label would be, or the theme's accent with `accent`, as an app's mark
  is. A title with `drop` is followed by a chevron pointing down, in the `secondaryText`
  colour until the title is under the cursor. A label stands three quarters of the padding
  from its chevron, and an icon a quarter. A title with `emphasis` is set on a pill of the
  `widget` colour, in the bar's own weight, so neither its text nor anything after it moves as
  it is set apart and back. An app sets it as the editor sets its active view's name. A bar's
  `opacity` multiplies the strip's alpha, so a bar can lie over a view and let it show
  through. The titles are drawn whole.
- **A centred menu** is drawn one level at a time in the middle of the canvas's safe area. The
  safe area is 5% of the width in from each side and 4% of the height in from the top and the
  bottom. A level too large for the safe area keeps its top left corner inside it.
  - Its `title` is written above its items, semibold and 1.75 times the size of their text when
    the menu is drawn on a `Screen`. A submenu is titled by the label of the item that opens it,
    unless it names a `title` of its own.
  - The chosen item shows three cues: the `highlight` behind it, its label in `active-text` and
    semibold, and a ring in `focus` at `focus-width` around it. The menu is as wide as its widest
    label set semibold, so it does not change width as the choice moves.
  - The down and up arrows wrap from the last item to the first and back.
  - Each row is a settings row: its label on the left and its value right-aligned, in
    `secondary-text` unless the row is chosen. The menu makes room for the widest value any row
    can show, the ellipsis of a capture and both ends of a slider's range among them, so it
    keeps its width as a value changes. A row shows an ellipsis while a capture waits on it,
    the key bound to the command it `binds`, a slider's value between the arrows that step it,
    or the value it holds. A row with none shows its label alone.
- **A slider item** holds a whole number between its `minimum` and `maximum`, which default to
  0 and 10, and moves by its `step`, 1 by default. Its `value` starts it, clamped to the range,
  and without one it starts at the minimum. `Menu::step()`, which `menuLess` and `menuMore` send,
  moves it and sends its command carrying the new value. A step past an end sends nothing.
- **An item's `binds`** names a command whole, as a binding document writes it, such as
  `pong::leftPaddleUp`. The context is the part before the first `::`, so a command may hold
  `::` itself. The row shows the key that command is bound to, through the screen's
  shortcut lookup, so it stays true after a rebinding.
- **A separator** is a rule between two groups of items, half a row tall. It needs no label and
  sends nothing. The keys step over it and the cursor passes over it, so nothing chooses it.
- **A menu item's icon** is drawn in the column a check mark takes, when the item is not
  checked. It is resolved with the rest of the ui's images.
- **A dropped panel shows each item's shortcut**, the key its command is bound to, right-aligned
  in `secondary-text`, and in `active-text` on the chosen row, where secondary text would not
  read against the highlight. The panel widens to fit the widest. Nothing writes the shortcut in the
  document: the screen asks the bindings as the panel is drawn, so a rebinding shows at once.
  A command is looked up by its name, so two items sending one command with different values
  show the same shortcut. A centred game menu shows none.
- **A submenu in a dropped panel opens after the cursor rests on its item** for the theme's
  `submenu-delay`, and at once on a press. Moving onto an item with no submenu closes any
  deeper panel at once. A menu bar drawn without a clock opens a submenu as soon as the cursor
  reaches it.
- **An open menu bar takes every press.** While a menu is open, a press anywhere on screen
  closes it and is consumed. A click meant for the scene under an open menu does nothing.
- **An open menu bar takes the keys that walk it**, before anything that has the focus. The
  down and up arrows choose the next and the previous item of the deepest panel, stepping over
  separators and wrapping. The right arrow opens the chosen item's submenu at once, or the next
  menu when it has none. The left arrow closes the deepest submenu, or opens the previous menu
  from the first panel. A menu opened from the keys chooses its first item. Return and space act
  on the chosen item as a press would, and escape closes the deepest submenu, or the bar.
- **A closed menu bar takes a press only on a label.** A press on the rest of its band is not
  taken, and neither is the cursor resting there. A container listed after the bar's can put
  buttons beside the labels, which take their presses, their hover and their tooltips.

## Tooltips

Any component may carry a `tooltip`. Once the cursor has rested on it for the theme's
`tooltip-delay`, the screen draws the text in a small plate just under the component. The
shortcut of the command the component sends follows it, in `secondary-text`.

- A tooltip shows after `tooltip-quick-delay` instead when another was showing a moment
  before, so moving along a row of buttons shows each one's almost at once. A cursor that has
  been off every tooltip for longer than `tooltip-delay` waits the full delay again.
- A press puts the tooltip away until the cursor moves again.
- A tooltip is drawn last, over every container and the statistics. Where it would leave the
  canvas below its component, it is drawn above it.
- A button on a toolbar down the left has its tooltip beside it, to its right and level with
  it, where one underneath would cover the next button. Where it would leave the canvas on the
  right, it is drawn to the left.
- A toolbar's buttons and the components of the tree show tooltips. A menu item shows its
  shortcut beside its label instead.
- `shell::Screen` draws them, from what `input::Cursor` writes as `Interface::hovered()`. An app
  that draws a `ComponentRenderer` itself calls `tooltip()` with the text to show.

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
focus. It draws no ring and takes no keys, but `Interface::focused()` still returns it and
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
