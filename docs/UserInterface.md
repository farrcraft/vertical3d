# The User Interface

What `api/ui` does, as of 2026-09-13. Open questions are at the end.

The decisions behind its shape are [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md),
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md),
[ADR-0034](adr/0034-a-component-has-children-and-a-box.md),
[ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md),
[ADR-0036](adr/0036-text-is-a-distinct-kind-of-quad.md),
[ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md),
[ADR-0038](adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md),
[ADR-0039](adr/0039-layout-never-reads-the-box-it-wrote.md),
[ADR-0040](adr/0040-a-key-goes-to-a-focused-component.md),
[ADR-0045](adr/0045-a-window-is-dragged-by-the-bar-that-folds-it.md),
[ADR-0046](adr/0046-a-table-given-a-height-scrolls-in-its-own-right.md),
[ADR-0057](adr/0057-a-selection-is-an-anchor-the-caret-moved-from.md) and
[ADR-0058](adr/0058-the-platform-half-of-a-ui-router-is-the-apis.md). Those say why; this
says what.

## Two ways to write a ui, and which to reach for

**A tree, when the ui is authored.** A menu bar, a settings screen, a HUD: built once by
`ui::Engine` from a JSON document, looked up by name, and kept in step with the game by
whatever answers its commands. `ui::ComponentRenderer` draws it.

**Calls, when the ui is a function of live state.** A debug readout, a tool panel: written as
a sequence between `Immediate::begin()` and `end()`, with nothing to keep in step because it
is recomputed every frame. `ui::Immediate` is both the layout and the draw.

A cursor position is the layer's only input, so **whether there is one to offer is the app's to
say**. A game that owns the mouse has none — mouselook warps the pointer back to the centre
every frame, so where it is says nothing — and a window it puts up is a readout rather than
something to fold, drag or scroll. `voxel` is that case and answers it the way a game does: the
menu going up is what hands the pointer back, so its F3 readout takes a real `Input` exactly
while the menu is up and a default one otherwise. The five fields come off
`input::MouseState` — the cursor, the primary button and its two edges, and the wheel.

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
Cursor        turns a point into a command, moves the focus, and places a caret
Keys          turns a key into an edit on whatever has the focus
  shell::Keyboard  the platform half of it: an SDL event in, and text input following the focus
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

`v3d::ui::paint::Measure` and `v3d::ui::paint::Write` in [`paint/Text.h`](../api/ui/paint/Text.h) are the seam. Both
renderers take the pair and name no font type, which is why drawing a ui costs no device and
why the whole library is testable without a window — per
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md).

`ui::TextRenderer` is what supplies a pair: one font, packed into one atlas of signed distance
field glyphs, with the drawn size closed over per
[ADR-0036](adr/0036-text-is-a-distinct-kind-of-quad.md). A ui at one size and a heading at
another are two callback pairs from one `TextRenderer`, and one atlas serves both.

It takes its atlas upload as a `TextRenderer::Upload` callback rather than a `vulkan::renderer::Quad`,
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
| `TextBox` | a plate, one line of text, a highlight behind the selected run, and a caret when it is focused | its text, its caret and its anchor |
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

Every one of them may also name `focus` and `focus-width`, which is the ring around the control
when it holds the keyboard. `button` is the one class a `Dressing` reads nothing else out of: a
button's fill is its nine images and its label is the base's, and the first style of the set
answers for the ring whatever state it was written for, because a ring says where the keyboard is
rather than what state the button is in.

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
`focusable()` with it, in its own constructor: a button, a check box, a radio button, a
scrollbar, a select list, a tab bar and a text box exist to be driven, and a panel or a label
laid over a scene does not. A press is remembered until it comes up, which is what drags a scrollbar's thumb
across frames.

A press also moves the focus — onto what it landed on when that component asked to be
focusable, and off whatever had it otherwise — which is what makes clicking into a box mean
"type here". In a text box it says where as well: the caret goes to the character under the
point and the anchor with it, so following the cursor selects the run between the two —
[ADR-0057](adr/0057-a-selection-is-an-anchor-the-caret-moved-from.md). That is what the
`paint::Measure` a cursor is given is for, and it should be the same one the renderer drawing
that ui was given; a cursor given none routes every press as before and leaves the caret where
it was. A button lights up under the cursor whether it sits on a strip or in the tree, and
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

`press()` takes two more arguments saying whether shift and control are held, because a key
name carries no modifier and this library cannot ask `api/input` for one without taking SDL with
it. Shift and tab is the focus moving backwards, shift and a caret key selects, and control
names the four chords a text box answers. A ui with nothing focused is left alone by tab as it
is by every other key.

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
there. A `Scrollbar` is moved rather than stepped: an arrow by a line — a bound list's row, or
`Scrollbar::lineStep` for a range of pixels that says nothing about what a line of it is —
`pageup` and `pagedown` by what the page shows, and `home` and `end` to the ends of the content.
A bar showing all of its content takes no key at all, because a control that swallows a key it
could not have acted on stops a game being played while it holds the focus. A component does not
own the state it shows, so activating a check box sends its command and marks nothing —
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md).

**A selection is an anchor the caret moved away from** —
[ADR-0057](adr/0057-a-selection-is-an-anchor-the-caret-moved-from.md). Nothing is selected
exactly when the two are in the same place, so every operation that moves the caret says one
thing: whether the anchor comes with it. Shift and a caret key selects the run it travelled, and
an arrow with nothing held lands on an end of the selection rather than a character past it.
Typing, a backspace, a delete and a paste all replace a selected run.

Cut, copy, paste and select all are `control` and `x`, `c`, `v`, `a`, and the clipboard behind
them is the app's: `Keys::Clipboard` is a pair of callbacks, for the reason text measuring is a
callback. A router given neither still edits — a cut with nowhere to hand the run does not
take it out, because a cut that loses the text is worse than one that did not happen, and a paste
with nothing to read puts nothing in. Every other chord goes on to the app, so a `ctrl-s` still
saves while somebody is typing.

**Something has to give out the first focus.** `Engine::focusFirst()` puts it on the first
focusable component, and is how a screen says it is keyboard driven — an app calls it as the
screen goes up. `focusNext()` will not do it, on purpose: tab must not take the focus onto the
first widget of a hud nobody is looking at, so a ui with nothing focused stays that way.

**A focused component is ringed**, traced around its box after it is drawn, in the `focus`
colour at `focus-width` thick of the style class the component is drawn in — so a theme can mark
a focused text box differently from a focused list, and one naming neither rings every control
out of the base. `ComponentRenderer`'s `ringed()` is what says which class rings which
component, and a component dressed by the base alone is ringed out of it. The draw walk traces
it rather than any one component, because where the keyboard is is the ui's business and one
ring drawn one way is the point of it.

The characters come from SDL's text input — shift already applied, a dead key and the one after
it already one character, an input method's several keys already however many characters it
decided on. `input::Keyboard` raises them as `event::TextInput` for anything that wants them as
an event; the ui gets them through the seam below, ahead of the bindings.

## The seam an app writes

`ui::Keys` names no platform type, which is what leaves it testable without a window and leaves
an app four things to do before it runs: decode the event, read the modifiers off it, find a
clipboard, and get the platform composing at all. That is the same work in every app, so it is
the api's — `ui::shell::Keyboard`, per
[ADR-0058](adr/0058-the-platform-half-of-a-ui-router-is-the-apis.md).

```cpp
uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui_, dispatcher_, window());

bool App::onEvent(const SDL_Event& event) {
    return uiKeys_->event(event);
}
```

It goes in `onEvent()` because [ADR-0043](adr/0043-an-app-sees-an-event-before-the-bindings-do.md)
puts the app ahead of the bindings: a key the ui took must not also fire the command bound to
it, and returning true is what stops it. **Only a key going down is ever taken.** A release
always goes through, so a key held when a box took the focus is still seen to come up and
`input::KeyState` is not left holding it down. A key `api/input` has no name for is not taken
either, since it is nothing the ui could have acted on.

The key name is `input::keyName()`'s — the same table the device binding that key reads — so a
binding written against backspace and what a text box answers cannot drift apart. Shift and
control come off the event rather than from the keyboard's held state, because what a key meant
is what was down as it arrived.

**Text input follows the focus.** The platform composes nothing until it is asked to, so the
seam turns `Window::textInput()` on while a text box holds the keyboard and off again after. It
follows `Engine::onFocus()` rather than checking per event, because the focus also moves under a
press the seam never sees — a box clicked into and typed into in one frame would otherwise lose
its first character. `onFocus()` holds one listener and the last caller wins.

The clipboard is SDL's, wired in by the seam; `shell::Keyboard::clipboard()` is public so a
`ui::Keys` built directly can have the same pair.

**The cursor stays the app's.** `ui::Cursor` takes points rather than events, and a press has to
interleave with whatever else an app does with one — the editor offers the ui a press and drives
a camera with the one the ui did not take — so a seam that consumed mouse events would decide
that for every app.

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
It comes from `input::MouseState::wheel()`, which accumulates the notches a frame saw and is
cleared with the button edges: a wheel sends one event per notch, so a flick that turned three
has to read as three rather than as the last of them.

`LineCanvas` cuts its stream the same way, on different terms: its rectangle is in the pixels
of the image drawn into and the modelview does not apply to it, because a line canvas is world
space and no transform there would carry a screen rectangle.

## Testing it

`api/ui/tests/` needs no window, no device and no font: `Canvas` is CPU side and the text
callbacks are the app's. A case builds components, draws them onto a canvas, and asserts on the
boxes the draw left or on the primitives it emitted. [Testing.md](Testing.md) has the rest.

## What is not built yet

- **A scrollbar bound to no list scrolls nothing.** It is the arithmetic: a bar told which
  `SelectList` it scrolls moves that list, and one given a range of its own leaves the app to
  read `offset()` and translate whatever it scrolls. Laying one out beside the thing it scrolls
  is the app's either way.
- **An `Immediate` widget is hovered a frame after it is drawn**, which is what lets a window
  drawn later take the cursor from one under it.
- **An `Immediate` widget takes the rest of its row unless told otherwise.**
  `nextItemWidth(float)` is what tells it, spent by the widget that follows and forgotten
  after it, which is what lets two scrubbers share a row. A separator always takes the row.

[TODO.md](TODO.md) carries these, and
[plans/UiConsolidation.md](plans/UiConsolidation.md) is what closed the ones that are gone.

## Still open

- **Nothing enforces which of the two ways to use.** The rule above is a rule of thumb in a
  document, and a reader who wants a HUD out of `Immediate` will get one that flickers under
  the cursor rather than an error.
- **Adding a component means editing eight places** — `component::Type`, `component::name()`,
  the loader's branch, the renderer's paint switch, its `ringed()`, the Arranger's `natural()`,
  the cursor's and the keys'. The compiler names all eight
  ([ADR-0047](adr/0047-a-component-type-is-checked-by-the-compiler.md)), so forgetting one is
  a build error rather than a component that silently is not there — but a registry is the only
  thing that would reduce the count.
