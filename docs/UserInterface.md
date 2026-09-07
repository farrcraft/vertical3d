# The User Interface

What `api/ui` does, as of 2026-09-07. Open questions are at the end.

The decisions behind its shape are [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md),
[ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md),
[ADR-0034](adr/0034-a-component-has-children-and-a-box.md),
[ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md),
[ADR-0036](adr/0036-text-is-a-distinct-kind-of-quad.md),
[ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md) and
[ADR-0038](adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md). Those say why; this
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
  — the width of a label's text, the side of an icon, the room a button's label needs — and
  for a position it is wherever it was last placed.

`anchor` says which corner of the parent `x` and `y` are measured from, and they always grow
inwards, so a bottom-right anchor with an `x` of 8 sits eight pixels in from the right edge
whatever the parent's width is.

`layout()` is the input. `position()` and `size()` are the **output** — the absolute box the
component was last drawn in. The two are separate so a percentage survives being resolved.

**A component is not clickable until it has been drawn**, and that failure inherits: every
child of a component that was skipped is unplaced too.

`HorizontalBox` and `VerticalBox` arrange what they hold in a line, with a gap and an optional
stretch across the line. A hidden child leaves no gap behind it. A z index changes nothing
inside a flow box, because the order it holds them in is what it is for.

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
| `Label` | one line of text | its text |
| `Icon` | a texture at the component's size | its source and handle |
| `Bar` | a track and the fraction of it that is filled | its fraction |
| `Button` | a label, or an icon, or a nine-slice skin | nothing — a toggle's mark is set by whatever answers its command |
| `CheckBox`, `RadioButton` | a mark and a label beside it | nothing, for the same reason |
| `Scrollbar` | a track and a thumb | its range and offset |
| `SelectList` | a plate and as many rows as it shows | its rows and which is chosen |
| `TabBar`, `TabPage` | a strip of tabs and the one page chosen | which page is up |
| `HorizontalBox`, `VerticalBox` | nothing — they place what they hold | spacing and stretch |
| `Menu`, `MenuItem`, `MenuBar` | a panel of items, or a strip that drops one | which item is active, and any capture |

A component that does not own the state it shows is deliberate: a click sends a command and
marks nothing, and whatever answers the command sets `checked()`, so the mark cannot disagree
with what the app believes. A `SelectList` and a `TabBar` are the exceptions, because which
row is chosen is a place in their own contents rather than a fact about the app.

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
| `panel`, `bar`, `scrollbar`, `checkbox`, `radio`, `list`, `tabs` | the component of that kind |
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
clickable — which is why `pickable()` is false by default. A press is remembered until it comes
up, which is what drags a scrollbar's thumb across frames.

Everything is tested against the boxes the last draw left, so an app that routes input before
it draws sees a dead ui for one frame.

## Clipping

A component that asks to `clip()` cuts what it holds off at its own box, which the batch
carries as a scissor rectangle — [ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md).
It is asked for rather than default, because a menu drops a panel out of the strip it came
from. A `SelectList` and an `Immediate` window clip themselves.

A clip is axis aligned and square, so a panel with rounded corners clips to the box and not to
the curve.

## Testing it

`api/ui/tests/` needs no window, no device and no font: `Canvas` is CPU side and the text
callbacks are the app's. A case builds components, draws them onto a canvas, and asserts on the
boxes the draw left or on the primitives it emitted. [Testing.md](Testing.md) has the rest.

## What is not built yet

- **There is no text box**, and it is the one missing component that needs something the
  library does not have: a key goes to the app's input engine and nothing routes one to a
  focused component.
- **A hover is the strips'.** `Toolbar` writes a hover state onto its buttons and no other
  component has one, so a button in a tree does not light up under the cursor.
- **A scrollbar scrolls nothing.** It is the arithmetic, and putting one beside a `SelectList`
  is still the app's.
- **An `Immediate` widget is hovered a frame after it is drawn**, which is what lets a window
  drawn later take the cursor from one under it.
- **A percentage of a parent that has not been drawn is a percentage of zero**, so the frame
  after a resize places a child against the previous size.

[TODO.md](TODO.md) carries these, and
[plans/UiConsolidation.md](plans/UiConsolidation.md) is what closed the ones that are gone.

## Still open

- **Nothing enforces which of the two ways to use.** The rule above is a rule of thumb in a
  document, and a reader who wants a HUD out of `Immediate` will get one that flickers under
  the cursor rather than an error.
- **Adding a component means editing five places** — the enum, the loader's branch, the draw
  walk's switch, `natural()`, and the cursor's — and the compiler checks none of them against
  the others.
