# Immediate mode

`v3d::ui::Immediate` builds a panel from function calls every frame. When to choose it over a
document is in [README.md](README.md#two-ways-to-write-a-ui).

## Immediate mode

```cpp
v3d::ui::Immediate* layer = screen_->immediate();
layer->begin(&screen_->canvas(), input);
if (layer->window("Debug", glm::vec2(20, 20), glm::vec2(260, 180), 0.85f)) {
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
| `window(title, position, size, alpha)` / `endWindow()` | a window with a title bar | whether to draw its contents |
| `text`, `textDisabled`, `textWrapped`, `bulletText` | a line of text | |
| `button(label)`, `smallButton(label)` | a button, bar-height or line-height | true on the frame it was clicked |
| `selectable(label, selected)` | a full-width row, highlighted when selected | true when clicked |
| `dragInt(label, &value, low, high)` | an integer scrubbed by dragging across it | true when the value changed |
| `progressBar(fraction, overlay)` | a filled track with text over it | |
| `separator()` | a rule across the width | |
| `tabBar(id)` / `tab(label)` / `endTabBar()` | a strip of tabs | `tab()` is true for the selected tab |
| `table(id, columns, height)` / `column` / `headerRow` / `nextRow` / `nextColumn` / `endTable` | a table | |

Layout helpers:

- `sameLine()` puts the next widget beside the last one.
- `nextItemWidth(width)` sets the width of the next widget only. Without it, a widget takes
  the rest of the row. A separator always takes the whole row.
- `beginDisabled()` / `endDisabled()` draw everything between them dimmed and unresponsive.
  The scopes nest.
- A click on a tab takes effect at `endTabBar()`, so the old tab answers for the whole frame
  on which the click lands.

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

The layer keeps a little state per id: a window's fold, drag offset and scroll, and a tab
strip's selection. `Immediate::retention` (60 frames) is how long that state is kept after a
widget stops being drawn. A panel behind a toggle comes back scrolled, folded and placed as
it was. `retained()` returns how many ids hold state, for a caller that builds ids from
changing text.

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

A **table given a height** (the third argument to `table()`) scrolls its own rows:

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

`Immediate::theme(theme)` reads the theme's `tools` style. `Screen::theme()` calls it for you.

| Kind | Keys |
|---|---|
| colours | `panel`, `border`, `title-bar`, `text`, `active-text`, `dim-text`, `widget`, `highlight`, `hover`, `fill`, `rule` |
| numbers | `line-height`, `padding`, `spacing`, `bar-height`, `border-width`, `radius`, `scrollbar-width` |
