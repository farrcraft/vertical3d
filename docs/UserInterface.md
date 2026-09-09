# The User Interface

What `api/ui` does, as of 2026-09-08. Open questions are at the end.

The decisions behind its shape are [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md),
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md),
[ADR-0034](adr/0034-a-component-has-children-and-a-box.md),
[ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md),
[ADR-0036](adr/0036-text-is-a-distinct-kind-of-quad.md),
[ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md),
[ADR-0038](adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md),
[ADR-0039](adr/0039-layout-never-reads-the-box-it-wrote.md),
[ADR-0040](adr/0040-a-key-goes-to-a-focused-component.md),
[ADR-0045](adr/0045-a-window-is-dragged-by-the-bar-that-folds-it.md) and
[ADR-0046](adr/0046-a-table-given-a-height-scrolls-in-its-own-right.md). Those say why; this
says what.

## Two ways to write a ui, and which to reach for

**A tree, when the ui is authored.** A menu bar, a settings screen, a HUD: built once by
`ui::Engine` from a JSON document, looked up by name, and kept in step with the game by
whatever answers its commands. `ui::ComponentRenderer` draws it.

**Calls, when the ui is a function of live state.** A debug readout, a tool panel: written as
a sequence between `Immediate::begin()` and `end()`, with nothing to keep in step because it
is recomputed every frame. `ui::Immediate` is both the layout and the draw.

The rule is what owns the truth. A retained tree that shows a number has to be told when the
number changes, and the characteristic defect is a readout two frames stale; an immediate
panel cannot be stale and cannot be looked up by name. `voxel`'s F3 readout is the tree's
worst case and the layer's best one, which is why it is the layer's first consumer.

Both draw onto the same `realtime::Canvas`, share `Painter`'s box drawing, and take the same
text callbacks, so the two look like one ui. They read different style classes — see below.

## The classes

```
Engine        the loaded ui: containers, themes, which theme is active
  Loader      builds one out of a JSON document, and is then done with
Container     what a document named, and what a point is picked out of
Component     a box, children, and what a draw leaves on it

ComponentRenderer   paints a component, and owns the two below
  Arranger          resolves every box and calls back to paint each one
  style::Resolver   turns a theme into the Dressing a component is drawn with

Immediate     the other way to write a ui - layout and paint in one pass
Cursor        turns a point into a command, and moves the focus
Keys          turns a key into an edit on whatever has the focus
TextRenderer  one font, one atlas, and the Measure/Write pair both renderers take
Painter.h     fillBox, strokeBox and plateBox, which both ways draw out of
```

Two of those splits are worth knowing about. **The walk is the Arranger's and the painting
is the renderer's**, joined by a `Paint` callback: one walk still decides both what is drawn
and what is clicked, per ADR-0019, but it will run with no canvas and nothing to paint, so
layout can be asked for on its own. And **reading a config is the Loader's**, so `Engine.h`
names `Container` and `style::Theme` rather than including every component header and
`boost::json`.

## Everything is a quad on somebody else's canvas

Nothing here owns a device, a pass or a draw. A panel, a highlight and a line of text are all
the batched quad of [ADR-0005](adr/0005-one-batched-quad-primitive.md), appended to whatever
canvas the app is already filling, so a ui costs the frame no pass and no draw of its own.

```
app's Canvas  <--  ComponentRenderer::draw(canvas, engine)   the tree
              <--  Immediate::begin(canvas, input) ... end()  the calls
              <--  TextRenderer::draw(canvas, ...)            a line of text anywhere
```

An app submits that canvas through `Engine3D::quads()` when it is done with it.

## Text is the caller's

`v3d::ui::Measure` and `v3d::ui::Write` in [`Text.h`](../api/ui/Text.h) are the seam. Both
renderers take the pair and name no font type, which is why drawing a ui costs no device and
why the whole library is testable without a window — per
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md).

`ui::TextRenderer` is what supplies a pair: one font, packed into one atlas of signed distance
field glyphs, with the drawn size closed over per
[ADR-0036](adr/0036-text-is-a-distinct-kind-of-quad.md). A ui at one size and a heading at
another are two callback pairs from one `TextRenderer`, and one atlas serves both.

It takes its atlas upload as a `TextRenderer::Upload` callback rather than a `QuadRenderer`,
so the one thing in the class that needs a device is the one thing handed in and an app
drawing this canvas with a renderer of its own can use the class rather than copy it. `api/ui`
names no vulkan type anywhere as a result, which is what ADR-0019's seam was always claiming.
An app on `Engine3D` passes `quads->texture(image)`.

Both take a `std::string_view`. A component already holds its text, so measuring one must not
cost an allocation per label per frame.

## The box model

A `Component` has children and a `Layout`, and the draw walk resolves the layout against the
box around it — [ADR-0034](adr/0034-a-component-has-children-and-a-box.md).

```
Layout { Length x, y, width, height; Anchor anchor; }
Length { value, unit }   unit = Auto | Pixels | Percent
```

- **Pixels** is itself.
- **Percent** is of the parent's extent in the same axis.
- **Auto** hands the number back to the component: for a size that is what it makes of itself
  — the width of a label's text, the side of an icon, the room a button's label needs — and a
  component that makes nothing of itself takes the room it is offered. For a position it is no
  offset at all, so the component sits at the corner it is anchored to.

Nothing in layout reads a box a previous walk wrote —
[ADR-0039](adr/0039-layout-never-reads-the-box-it-wrote.md). A tree laid out twice lands in the
same place, the first frame is the same as the tenth, and a resize places every child against
the new size rather than the old one.

`anchor` says which corner of the parent `x` and `y` are measured from, and they always grow
inwards, so a bottom-right anchor with an `x` of 8 sits eight pixels in from the right edge
whatever the parent's width is.

`layout()` is the input. `position()` and `size()` are the **output** — the absolute box the
component was last drawn in. The two are separate so a percentage survives being resolved.

**A component is not clickable until it has been drawn**, and that failure inherits: every
child of a component that was skipped is unplaced too.

`HorizontalBox` and `VerticalBox` arrange what they hold in a line, with a gap and an optional
stretch across the line. A hidden child leaves no gap behind it. A z index changes nothing
inside a flow box, because the order it holds them in is what it is for. Along the line the
children share the room, so an Auto extent there is what the child makes of itself and a panel
that makes nothing of itself asks for nothing; across the line each is offered the whole of it.

## What a container draws, and in what order

```
non-strip components   in depth order, add order between equal depths
toolbars               each at the corner stack() gave it
menu bars              last, because an open menu drops a panel over the strips below it
```

The strips stack: a menu bar takes the top of the canvas, a top toolbar takes a band under
whatever is already there, and a left toolbar runs down the side of what is left.
`ComponentRenderer::insets()` is what an app asks for the area that leaves it — and it asks
the same `stack()` the draw does, so the two cannot disagree.

## The components

Every type in `component::Type` has a loader and a draw path; there are no empty declarations.

| | draws | owns |
|---|---|---|
| `Panel` | a filled box with a border | nothing |
| `Label` | its text, wrapped to the width it was given, or one line when that width is `Auto` | its text |
| `Icon` | a texture at the component's size | its source and handle |
| `Bar` | a track and the fraction of it that is filled | its fraction |
| `Button` | a label, or an icon, or a nine-slice skin | nothing — a toggle's mark is set by whatever answers its command |
| `CheckBox`, `RadioButton` | a mark and a label beside it | nothing, for the same reason |
| `Scrollbar` | a track and a thumb | its range and offset, or nothing at all when it was told which `SelectList` it scrolls |
| `SelectList` | a plate and as many rows as it shows | its rows and which is chosen |
| `TabBar`, `TabPage` | a strip of tabs and the one page chosen | which page is up |
| `TextBox` | a plate, one line of text, and a caret when it is focused | its text and its caret |
| `HorizontalBox`, `VerticalBox` | nothing — they place what they hold | spacing and stretch |
| `Menu`, `MenuItem`, `MenuBar` | a panel of items, or a strip that drops one | which item is active, and any capture |

A component that does not own the state it shows is deliberate: a click sends a command and
marks nothing, and whatever answers the command sets `checked()`, so the mark cannot disagree
with what the app believes. A `SelectList`, a `TabBar` and a `TextBox` are the exceptions,
because which row is chosen, which page is up and what has been half typed are places in their
own contents rather than facts about the app.

## Themes

A theme is data, and the app resolves the images it names —
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md). `ui::Engine::load()`
reads themes and containers out of one JSON document; `resolveImages()` is a second pass an app
runs once it has a renderer to upload through.

A `Theme` holds `Style`s; a `Style` is a bag of `Property`s of four kinds — colour, number,
font, image — each read from its own array and filed under a class. A component names a style;
one that names none is dressed by whichever style of its class the theme holds first, so a
theme can dress every panel without every panel naming one.

`style::Resolver` turns a theme into a `Dressing` — the plain struct of colours and metrics a
component is drawn with, with no strings in it. It works one out per (class, style name) and
keeps it until the theme or the base changes, because a component asks every frame and a theme
changes almost never.

The style classes:

| Class | Read by |
|---|---|
| `ui` | the retained components — the defaults every other class is applied over |
| `tools` | `ui::Immediate` |
| `panel`, `bar`, `scrollbar`, `checkbox`, `radio`, `list`, `tabs`, `textbox` | the component of that kind |
| `button` | `ComponentRenderer::skin()`, chosen by button state as well as by name |

`ui` and `tools` are separate on purpose: they want the same key names at about twice the
size, because a HUD is read at a glance and a tool panel is read closely.

## The cursor

`ui::Cursor` turns a point into a command —
[ADR-0038](adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md). It offers the point in
the reverse of the order the ui was drawn — menu bars, then toolbars, then the component tree
— and the first thing that takes it stops the walk.

A press on a `pickable()` component sends that component's bound event and is consumed. A
press on anything else is not consumed, so a HUD of labels over a scene leaves the scene
clickable — which is why `Component` leaves `pickable()` false. A control sets it, and
`focusable()` with it, in its own constructor: a button, a check box, a radio button, a select
list, a tab bar and a text box exist to be driven, and a panel or a label laid over a scene
does not. A press is remembered until it comes up, which is what drags a scrollbar's thumb
across frames.

A press also moves the focus — onto what it landed on when that component asked to be
focusable, and off whatever had it otherwise — which is what makes clicking into a box mean
"type here". A button lights up under the cursor whether it sits on a strip or in the tree, and
only ever the one a press would land on, so a hud of unpickable labels does not flicker as the
cursor crosses it.

Everything is tested against the boxes the last draw left, so an app that routes input before
it draws sees a dead ui for one frame.

An `Immediate` window is moved by its title bar, which is also what folds it —
[ADR-0045](adr/0045-a-window-is-dragged-by-the-bar-that-folds-it.md). A press that stays put
folds the window as it always did; one that travels past a few pixels drags it instead and does
not fold it. The position `window()` is given stays the anchor: the drag is kept as a
displacement from it, so a window the caller repositions every frame follows and keeps the nudge
it was given. The bar is held on the canvas, because the bar is the only thing that drags one
back.

`Immediate::capturing()` is the immediate layer's half of the same rule: whether the cursor is
over something that layer drew, or is dragging something it drew, so an app can ask whether a
click has already been spent before acting on one of its own. It answers from the previous
frame for the same reason a widget's hover does.

## The keyboard

`ui::Keys` is the cursor's counterpart —
[ADR-0040](adr/0040-a-key-goes-to-a-focused-component.md). The focus lives on `ui::Engine`, one
component at a time, and a key goes there or nowhere: a ui with nothing focused takes no key,
so a game's movement bindings go on working until something is clicked into.

Two calls, because **a character is not a key**.

```
Keys::press("backspace")   what api/input named - an operation, or a key to swallow
Keys::text("e")            what the platform composed - utf-8, straight in at the caret
```

**Tab is the second way the focus moves.** `Engine::focusNext()` walks to the next focusable
component in the order the tree is drawn in - containers as the config listed them, components
by depth with add order between equal depths, a flow box's children in the order it holds them -
and wraps at each end, skipping a hidden subtree whole. A ui author wanting a different tab
order reorders the document; there is no `tabIndex`.

`press()` takes a second argument saying whether shift is held, because a key name carries no
modifier and this library cannot ask `api/input` for one without taking SDL with it. It matters
for tab alone. A ui with nothing focused is left alone by tab as it is by every other key.

A key names an operation: backspace, delete, the caret moves, a return that sends the box's
command, an escape that leaves it. A key that will arrive again as a character is taken as well
and does nothing, so typing "w" into a box does not also walk the player forward — **in a text
box only**. The same letter reaching a focused button goes on to the app's bindings, because a
button is not something a player types into.

**Every control is driven, not only a text box.** Return and space activate whatever holds the
focus, sending the command a click sends — both routers ask `ui::command()` which event a
component carries, so a component a press activates and a key does not cannot happen. The
arrows step through a `SelectList`'s rows and a `TabBar`'s pages, with `home` and `end` at the
ends; neither wraps, because running off the last row is how a keyboard reaches it and stays
there. A component does not own the state it shows, so activating a check box sends its command
and marks nothing — [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md).

**Something has to give out the first focus.** `Engine::focusFirst()` puts it on the first
focusable component, and is how a screen says it is keyboard driven — an app calls it as the
screen goes up. `focusNext()` will not do it, on purpose: tab must not take the focus onto the
first widget of a hud nobody is looking at, so a ui with nothing focused stays that way.

**A focused component is ringed**, traced around its box after it is drawn, in the base
dressing's `focus` colour at `focus-width` thick. The draw walk does it rather than any one
component, because where the keyboard is is the ui's business and one ring for every control is
the point of it.

The characters come from `event::TextInput`, which `input::Keyboard` raises from SDL's text
input — shift already applied, a dead key and the one after it already one character, an input
method's several keys already however many characters it decided on.
`realtime::Window` starts text input with the window, because SDL sends none until it is asked
to.

## Clipping

A component that asks to `clip()` cuts what it holds off at its own box, which the batch
carries as a scissor rectangle — [ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md).
It is asked for rather than default, because a menu drops a panel out of the strip it came
from. A `SelectList` and an `Immediate` window clip themselves.

A clip is axis aligned and square, so a panel with rounded corners clips to the box and not to
the curve.

The immediate layer has two things that clip and scroll, and they nest. An `Immediate` window
cuts its body and scrolls it, deciding from last frame's content whether it needs a bar — so
the bar arrives the frame after the one that overflowed. A table **given a height** does the
same for its own rows, per
[ADR-0046](adr/0046-a-table-given-a-height-scrolls-in-its-own-right.md): it clips to that
height, draws a bar down its own right and keeps `headerRow()`'s band above the region rather
than in it, so the column names stay put while the rows pass under them. Its gutter is reserved
whether or not there is anything to scroll, which is what lets its bar appear the same frame
the content overflows and stops the columns re-flowing when a row arrives. A table given no
height is as tall as its rows and scrolls with whatever holds it.

The wheel turns the innermost region under the cursor, so a table takes it from the window it
is drawn in — the same rule that lets a window drawn later take the cursor from one under it.

`LineCanvas` cuts its stream the same way, on different terms: its rectangle is in the pixels
of the image drawn into and the modelview does not apply to it, because a line canvas is world
space and no transform there would carry a screen rectangle.

## Testing it

`api/ui/tests/` needs no window, no device and no font: `Canvas` is CPU side and the text
callbacks are the app's. A case builds components, draws them onto a canvas, and asserts on the
boxes the draw left or on the primitives it emitted. [Testing.md](Testing.md) has the rest.

## What is not built yet

- **A scrollbar scrolls nothing.** It is the arithmetic, and putting one beside a `SelectList`
  is still the app's.
- **An `Immediate` widget is hovered a frame after it is drawn**, which is what lets a window
  drawn later take the cursor from one under it.
- **An `Immediate` widget takes the rest of its row unless told otherwise.**
  `nextItemWidth(float)` is what tells it, spent by the widget that follows and forgotten
  after it, which is what lets two scrubbers share a row. A separator always takes the row.
- **A scrollbar takes no key.** Every other control is driven from the keyboard; a scrollbar is
  dragged, and paging the thing it scrolls is still the app's.
- **A caret cannot be placed by clicking.** A press focuses a text box and leaves the caret
  where it was, because `ui::Cursor` names no text and would need the `Measure` callback to
  find the character under a point.
- **There is no selection in a text box**, so no cut, copy or paste over a range. `insert()`
  takes a run of characters, so a paste is expressible the moment something delivers one.

[TODO.md](TODO.md) carries these, and
[plans/UiConsolidation.md](plans/UiConsolidation.md) is what closed the ones that are gone.

## Still open

- **Nothing enforces which of the two ways to use.** The rule above is a rule of thumb in a
  document, and a reader who wants a HUD out of `Immediate` will get one that flickers under
  the cursor rather than an error.
- **Adding a component means editing five places** — `component::Type`, the loader's branch,
  the renderer's paint switch, the Arranger's `natural()`, and the cursor's — and the compiler
  checks none of them against the others. The split between layout and paint moved two of
  those into their own files; it did not reduce the count.
