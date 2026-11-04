# Immediate mode

`v3d::ui::Immediate` builds a panel from function calls every frame. When to choose it over a
document is in [README.md](README.md#two-ways-to-write-a-ui).

## Immediate mode

```cpp
v3d::ui::Immediate* layer = screen_->immediate();
layer->begin(&screen_->canvas(), input);
if (layer->window("Debug", { .position = glm::vec2(20, 20), .size = glm::vec2(260, 180) }, 0.85F)) {
    layer->text("chunks " + std::to_string(count));
    if (layer->button("Reset")) {
        reset();
    }
}
layer->endWindow();               // always, even when window() returned false
layer->end();
```

### Input

`Immediate::Input` is what the cursor did since the last frame. The app fills it each frame:

| Field | Meaning |
|---|---|
| `cursor` | the cursor position |
| `down` | the primary button is held |
| `pressed`, `released` | the button went down or came up this frame |
| `wheel` | wheel notches turned since the last frame |

These come from `input::MouseState`: `position()`, `held()`, `pressed()`, `released()` and
`wheel()`. `wheel()` adds up every notch a frame saw, so a fast flick counts every notch. A
default-constructed `Input` is no cursor at all.

### Widgets

| Call | Draws | Returns |
|---|---|---|
| `window(title, area, alpha)` / `endWindow()` | a window with a title bar | whether to draw its contents |
| `region(name, area, alpha)` / `endRegion()` | a panel with no title bar at a fixed place | |
| `text`, `textDisabled`, `textWrapped`, `bulletText` | a line of text | |
| `text(line, colour)` | a line of text in a colour of its own | |
| `image(picture, size, tint)` | an image the size given, tinted, centred down a line when it is shorter than one | |
| `button(label)`, `smallButton(label)` | a button, bar-height or line-height | true on the frame it was clicked |
| `iconButton(id, { .icon, .tooltip, .keys, .on })` | a square button a bar high, its icon in the middle. One that is `on` is filled as a checked toggle | true on the frame it was clicked |
| `selectable(label, selected)` | a full-width row, highlighted when selected | true when clicked |
| `treeRow(label, { .depth, .open, .selected, .icon, .iconTint })` | a full-width row of a tree, indented by its depth, with a fold mark when `open` is given, and its icon, tinted, before the label. A selected row is drawn on the `selected` tint | true when clicked anywhere but the mark |
| `heading(label, &open)` | a semibold line that heads a group of rows. Given `open`, it draws a fold mark before its label, and a click anywhere on it flips `*open` | whether to draw the group |
| `dragInt(label, &value, low, high)` | an integer scrubbed by dragging across it | true when the value changed |
| `dragFloat(label, &value, speed)` | a number scrubbed by dragging across it, or typed into after a click | true when the value changed |
| `sliderFloat(label, &value, low, high)` | a number set by pressing or dragging along a track | true when the value changed |
| `checkbox(label, &value)` | a box ticked or not, with its label beside it | true on the frame it was clicked |
| `combo(label, &selected, items)` | the label at the left, and the chosen item at the right before a drop mark. A click opens the list | true on the frame the choice changed |
| `textField(label, &value, limit)` | one line of text the user edits | true when the value changed |
| `textField(label, &value, { .icon, .labelled, .limit })` | the same, led by an icon in the `placeholder` colour, by its label in `secondary-text`, or by both | true when the value changed |
| `progressBar(fraction, overlay)` | a filled track with text over it | |
| `separator()` | a rule across the width | |
| `tabBar(id)` / `tab(label)` / `endTabBar()` | a recess across the row in the `tab` colour, shared evenly by its tabs. The chosen tab is raised to the `chosen` colour and set semibold | `tab()` is true for the selected tab |
| `table(id, { .columns, .height })` / `column` / `headerRow` / `nextRow` / `nextColumn` / `endTable` | a table | |
| `surface(id, height)` | nothing: a box the width of the row for the caller to draw into | a `Surface`: its box, the cursor, and its press |

Layout helpers:

- `sameLine()` puts the next widget beside the last one.
- `nextItemWidth(width)` sets the width of the next widget only. Without it, a widget takes
  the rest of the row. A separator always takes the whole row.
- `measure(line)` is how wide a line of text is drawn, for laying out widgets beside it.
- `rowWidth()` is the width a row has from its margin to its right edge. A caller dividing a row
  between widgets takes the spacing between them out of it first.
- `roomBelow()` is how tall the next row can be and still end inside its window or region, so a
  widget given that height fills the rest without making it scroll. It is 0 outside a window.
- `beginDisabled()` / `endDisabled()` draw everything between them dimmed and unresponsive.
  The scopes nest.
- A click on a tab takes effect at `endTabBar()`, so the old tab answers for the whole frame
  on which the click lands.
- A combo's list is drawn at `end()`, over everything, and is not clipped to its window. A
  click on an item takes effect on the next frame's `combo()` call. A press outside the list
  closes it, and so does a frame that does not draw the combo.

A **region** is a window with no title bar, which neither moves nor folds. An app draws one as a
panel that fills part of its own layout, such as an editor's area. Its name is its id and is
not drawn. Its contents are clipped and scrolled as a window's are, and it takes the cursor from
whatever was drawn before it. An `alpha` of 0 draws no background, for a region over something
the app has already filled.

A **tree row** is indented by one line for each level of `depth`. A row given `open` has
something beneath it, and draws a mark in the first line of its indent, pointing right while
folded and down while open. A click on the mark flips `*open` and is not a click on the row. The
caller draws the rows beneath an open row and skips those beneath a folded one.

A **number** from `dragFloat()` draws its label at the left in `secondary-text` and its value
at the right, to three places without the zeros it does not need: `1.5`, `0` and `0.123`. A
value never starts before its label has ended. A `NumberLook` gives the label a colour of its
own, a unit written straight after the value, and an edge, a rule 2 pixels wide at a scale of 1
down the field's left side. A unit set apart by a space starts with one:
`{ .labelColour = red, .unit = " m", .edge = red }`. The edge follows the field's rounded
corners.

- A press that travels more than 3 pixels across it drags the value, by `speed` for each pixel
  after those 3.
- A press that does not travel types into it, with the whole value chosen, so typing replaces
  it. The keys and the keyboard work as a text field's do.
- What was typed is taken when the field gives up the keyboard, unless escape gave it up or the
  text is not a finite number. Then the value stays as it was.

An **icon button's tooltip** shows once the cursor has rested on it for the theme's
`tooltip-delay`. It is drawn at `end()`, over everything else, in a plate just under the
button, or above it where it would leave the canvas. Its `keys` follow the text in
`secondary-text`.

- After a tooltip has shown, the next shows after `tooltip-quick-delay`, so moving along a row
  of buttons shows each one's almost at once.
- A press puts it away until the cursor moves.
- A layer given no clock shows a tooltip at once.

A **surface** is for a widget the layer does not have, such as a track of keys along a
timeline. The caller draws into it and acts on what its `Surface` reports.

- `hovered` is true while the cursor is on it and on nothing drawn over it. `pressed` is true
  on the frame a press goes down on it.
- `held` stays true from that press until it comes up, wherever the cursor goes, so a drag that
  leaves the surface keeps acting on it. `released` is true on the frame the held press comes
  up. `pressAt` is where the press went down.
- `wheel` is the notches the wheel turned while the cursor was on it, away from the reader
  first, and 0 while the cursor is elsewhere.
- `fill(min, max, colour)`, `line(from, to, width, colour)`, `diamond(centre, radius, colour)`
  and `textAt(line, at, colour)` draw at the points given, clipped as the widgets around them
  are, and take no place in the row. A diamond is a square stood on its corner.

`itemFinished()` is true on the frame an edit of the item just drawn ends: a number's drag is
let go, or a number or a text field gives up the keyboard. A caller that applies each change as
it comes, as a drag reports them, records the whole edit as one change on that frame.

Three more calls serve a field that takes lines, as a console does:

- `itemEntered()` is true on the frame a return made the field just drawn give up the
  keyboard. `itemFinished()` is true then too. A tab, an escape or a press elsewhere finishes an
  edit without entering it.
- `itemTyping()` says the field just drawn holds the keyboard, once the frame's input has
  reached it.
- `focusNext()` gives the keyboard to the next text field drawn in the same frame, with the
  caret at the end of its text. A frame that draws no field lets the request go. A field that
  is disabled does not take it.

### Ids, hover and capture

A widget's **id** is its label hashed with the id stack. Two widgets with the same label in
the same scope share an id, and so share hover and press state. Separate them with
`pushId()` and `popId()`, for example around each row of a table that has a "Kill" button in
every row.

**Hover runs one frame behind.** Which widget the cursor is on is settled at `end()` and used
on the next frame. This lets a window drawn later take the cursor from one drawn under it. A
widget that has just appeared or moved is hovered a frame late.

`Immediate::capturing()` is true when the cursor is over something the layer drew, or is
dragging something it drew. Ask it before acting on a click of the app's own. It answers from
the previous frame.

`Immediate::itemHovered()` is true when the cursor is on the item just drawn and nothing drawn
over it took the cursor. It answers for text and progress bars as well as widgets, so it is
what a tooltip is drawn from. It runs a frame behind, as every hover does. A disabled widget
answers as text does, so a tooltip can say why it is disabled.

**Something another renderer draws is not this layer's, unless it is blocked.** A modal
backdrop drawn over the whole viewport by another renderer is neither a window nor a widget, so
a click on it reaches the scene. `Immediate::block(min, max)` claims a rectangle for the layer
without drawing in it:

```cpp
ui.block(glm::vec2(0.0F), glm::vec2(width, height));   // the backdrop's rectangle
```

The rectangle counts toward `capturing()` as a window does. It takes the cursor from whatever the
layer drew before it, a widget drawn after it inside it takes the cursor back, and the answer is
a frame behind.

The layer keeps a little state per id: a window's fold, drag offset and scroll, and a tab
strip's selection. `Immediate::retention` (60 frames) is how long that state is kept after a
widget stops being drawn. A panel behind a toggle comes back scrolled, folded and placed as
it was. `retained()` returns how many ids hold state, for a caller that builds ids from
changing text.

### Typing into a field

`textField(label, &value, limit)` edits a string the caller owns. The label is the field's id,
and is drawn dimmed inside it while it is empty and does not have the keyboard. A `FieldLook`
can lead the field with an icon, drawn in the `placeholder` colour, as a filter leads with a
magnifier. With `labelled` it draws the label at the left in `secondary-text` at all times, as a
number does, in place of the placeholder. The text and the caret start past both.

- A press on the field gives it the keyboard and puts the caret where the press landed.
- A return, a tab, an escape or a press anywhere else gives the keyboard up. What was typed is
  kept.
- A field that is not drawn for a frame gives the keyboard up at `end()`. What was typed since
  it was last drawn is lost.
- The caller's string is the value. A change the caller makes while the field has the keyboard
  replaces what is being edited.
- `limit` caps the string's length in bytes. Zero is no limit. Typing past it is refused.
- The editing keys and chords are a `TextBox`'s. [Keyboard.md](Keyboard.md#editing-text) lists
  them.

Keys reach the field between frames, as the app's event loop sees them:

| Call | Does | Returns |
|---|---|---|
| `press(key, shifted, controlled)` | edits the line of the field that has the keyboard | whether the field took the key |
| `typed(utf8)` | inserts composed characters at the caret | whether the field took them |
| `composing(utf8)` | shows an input method's composition at the caret, until it is typed in | whether the field took it |
| `typing()` | | whether a field has the keyboard |
| `onTyping(callback)` | calls back with `typing()` each time it changes | |
| `clipboard(board)` | sets the clipboard cut, copy and paste use | |

The edit lands on a line the layer keeps, and reaches the caller's string when the field is
next drawn. A key the field took must not also fire a binding. A field takes every key that
composes text, so typing an `e` into it does not also send whatever `e` is bound to. A key that
composes nothing, such as a function key, and a control chord other than the four editing
chords, are left for the app.

`shell::Keyboard` does this routing for an app, along with the platform's text input and
clipboard. A game that draws only through `Immediate` builds one with no `ui::Interface`, offers it
the layer, and passes it every event from `onEvent()`:

```cpp
keys_ = boost::make_shared<v3d::ui::shell::Keyboard>(nullptr, dispatcher(), window());
keys_->immediate(screen_->immediate());

bool App::onEvent(const SDL_Event& event) {
    return keys_->event(event);
}
```

[Setup.md](Setup.md#the-keyboard-adapter) has the adapter's rules.

### Titles and headers

A window's title and a table's header are set in the dressing's `titleWeight`, semibold by
default, through the layer's text renderer. A `titleWeight` below 500 sets them in the regular
face.

### Moving and folding a window

The title bar both folds a window and moves it.

- A press on the bar that stays within 3 pixels folds or unfolds the window on release.
- A press that travels further drags the window instead, and does not fold it. The first 3
  pixels of travel are not applied, so the window lags the cursor by that much.
- The `position` passed to `window()` is an anchor. A drag is kept as an offset from it, so a
  window the caller moves every frame still follows and keeps the user's offset.
- The window is clamped so its title bar stays on the canvas, and its full width stays
  inside the canvas horizontally. A window cannot be dragged out of reach.
- A window can end up somewhere other than where the caller put it. A caller drawing
  something beside the window cannot rely on its position.
- To start a window fresh, give it a new title (a new id).

### Scrolling

A window clips its body to its edges and scrolls when its content is taller than it.

- The content height is measured as it is drawn. A window decides whether it needs a
  scrollbar from the previous frame's content, so its bar appears one frame after the content
  overflows.
- The wheel scrolls three rows per notch.

A **table given a height** (the `height` of its `TableShape`) scrolls its own rows:

- It clips to that height and draws its own scrollbar down its right side.
- `headerRow()` is drawn above the scrolled region, so the column names stay put while the
  rows move under them.
- The width of the scrollbar is always reserved, so its bar appears on the same frame the
  rows overflow and the columns do not shift when a row is added. A table given a height is
  therefore narrower than the same table without one.
- A height smaller than the header leaves no room for rows, and draws an empty table.

A table given no height is as tall as its rows and scrolls with whatever holds it.

The wheel turns the innermost scrolling region under the cursor. A table inside a window takes
the wheel from the window.

A row scrolled out of view can still respond to hover if the cursor is where its box went.

### The `tools` theme class

`Immediate::theme(theme)` reads the theme's `tools` style. `Screen::begin(ui)` calls it with the
ui's active theme. The `tools` style takes the same keys as the retained ui's `ui` style, and
[Themes and styles](Themes.md#the-base-classes-ui-and-tools) lists them. The immediate layer
starts from `paint::Dressing::tools()`: the retained defaults at a tool panel's sizes, with the
track, the tabs and the hover colour lifted to suit widgets that are drawn raised.
