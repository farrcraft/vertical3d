# Ui documents and layout

A retained-mode ui is described by a JSON document. This page covers the document's format and
how each component's box is worked out from it.

## The ui document

A ui document is JSON with three top-level keys:

```json
{
  "theme": "default",
  "themes": [ { "name": "default", "styles": [ ... ] } ],
  "containers": [
    { "name": "hud", "visible": true, "components": [ ... ] }
  ]
}
```

- `themes` (required) is an array of themes. See [Themes and styles](Themes.md#themes-and-styles).
- `theme` (optional) names the active theme. A name that matches no theme fails the load.
- `containers` (required) is an array. Each container has a `name`, a `visible` flag and a
  `components` array. A container is shown and hidden as a whole.

Every component entry has a `type` and a `name`. These keys apply to every component:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | required | the component type, from the table in [Components](Components.md#components) |
| `name` | string | required | what the app looks it up by |
| `position` | `[x, y]` | `Auto` | offset from the anchored corner; each a length |
| `size` | `[width, height]` | `Auto` | each a length |
| `anchor` | string | `top-left` | `top-left`, `top-right`, `bottom-left`, `bottom-right` or `centre` (`center` also reads) |
| `style` | string | none | a style name within the component's style class |
| `visible` | bool | true | |
| `enabled` | bool | true | see [Enabled and disabled](Components.md#enabled-and-disabled) |
| `pickable` | bool | per type | whether a press can land on it |
| `focusable` | bool | per type | whether it can take the keyboard |
| `clip` | bool | false | whether its children are cut off at its box |
| `depth` | number | 0 | draw order among siblings; higher draws later |
| `children` | array | none | component entries held inside this one |

A length is a number of pixels (`120`), a percentage string (`"50%"`), or absent for `Auto`.
A menu and a menu bar ignore `position`, `size` and `anchor`, because the renderer places
them.

## The box model

Every component has a `Layout`:

```
Layout { Length x, y, width, height; Anchor anchor; }
Length { value, unit }   unit = Auto | Pixels | Percent
```

Each frame, the ui works out every component's box from its layout and its parent's box. A
root component's parent is the whole canvas.

- **Pixels** is the number given.
- **Percent** is a percentage of the parent's size in the same axis.
- **Auto** for a size is the component's natural size: the width of a label's text, the
  side of an icon, the room a button's label needs. A component with no natural size (a
  panel, a bar, a tab bar) takes all the room it is offered.
- **Auto** for a position is zero. The component sits exactly at its anchored corner.

`anchor` names the corner of the parent that `x` and `y` are measured from. Offsets always
point inwards. A component anchored `bottom-right` with an `x` of 8 sits 8 pixels in from the
right edge, whatever the parent's width. With `centre`, the component is centred and `x` and
`y` move it right and down.

A component that names no position and no size fills its parent.

`layout()` is the input. `position()` and `size()` are the output: the absolute box the
component was given on the last frame. The layout does not change when it is resolved, so a
percentage still means a percentage on the next frame.

Layout never reads a box from an earlier frame. A tree laid out twice lands in the same
place, the first frame matches the tenth, and a resize places every child against the new
size. One consequence: writing `position()` does not move a component. Change its `layout()`
instead.

### Flow boxes

`hbox` (`HorizontalBox`) and `vbox` (`VerticalBox`) place their children in a line, in the
order the children are listed.

- `spacing` is the gap between children.
- A hidden child leaves no gap.
- `depth` has no effect inside a flow box. The listed order is the drawn order.
- A child's position along the line is ignored. Its offset across the line (`x` in a `vbox`,
  `y` in an `hbox`) still applies. Its anchor is ignored.
- Percentages resolve against the flow box's size.

**`Auto` means something different inside a flow box.** Along the line, children share the
room, so each child is offered none of it. An `Auto` extent along the line is the child's
natural size, and a child with no natural size gets zero. Across the line, each child is
offered the full width (or height) of the box.

- In a `vbox`, an `Auto` panel is as wide as the box and zero pixels tall. Give it a height.
- A `list` or `scrollbar` in a `vbox` with no stated height has no height.

`stretch: true` forces every child to the box's full extent across the line, overriding the
child's own size.

**A flow box may wrap** with `wrap: true`.

- It starts a new line, spaced by the same gap, when the next child would run past its end.
- A child longer than the line gets a line of its own.
- A wrapping box does not stretch.
- Across its lines, it sizes itself to the lines its children fill. A grid of fixed-size
  cells needs no stated height.
- Along the line it takes the room it is offered. Inside an `hbox` that room is zero, so a
  wrapping box inside an `hbox` needs a width.

### What gets a box

A component that has not been drawn has no box, and a press cannot land on it. This applies
to its children as well. A hidden component, a tab page that is not selected, and the rows of
a list that are scrolled out of view are not laid out on that frame.

### Strips: menu bars and toolbars

Menu bars and toolbars are called strips. They are placed at the edges of the canvas rather
than by their layout, and they stack:

- Every menu bar takes the top of the canvas.
- Each `top` toolbar takes a band under the menu bars and the top toolbars listed before it.
- Each `left` toolbar runs down the side, below every menu bar and top toolbar, beside the
  left toolbars listed before it.

The order a document lists strips in decides the order within each edge, never whether a left
strip starts above a top one.

`ComponentRenderer::insets(engine)` returns the space the strips take: the left inset in x
and the top inset in y. An app draws its own content in the rest. It uses the same
calculation as the draw, so the two always agree.

Within a container, the draw order is:

1. every non-strip component, by `depth`, with the listed order kept between equal depths
2. toolbars, each at the corner the stacking gave it
3. menu bars, last, so an open menu's panel drops over the strips below

Containers are drawn in the order the document lists them.
