# Themes, styles and images

A theme sets how a ui looks without changing its document. It can also name images, which the
app loads.

## Themes and styles

A theme is data in the ui document. It is a list of styles. Each style has a `class`, a
`name`, and up to three arrays of properties: `colors`, `numbers` and `images`.

```json
{ "class": "panel", "name": "plate",
  "colors":  [ { "name": "background", "value": [0.1, 0.1, 0.12, 0.9] } ],
  "numbers": [ { "name": "radius", "value": 6 } ] }
```

- A colour's `value` is four numbers, RGBA from 0 to 1.
- A number's `value` is one number. A size is in logical pixels, which the screen multiplies by its
  scale (see [Scaling](Setup.md#scaling-to-the-monitor-and-the-window)). An outline's width is
  rounded down to whole device pixels and is never less than one.
- An image has a `source`, which the app resolves (see [Images](#images)).
- Any other member of a style is not read. Every component's text is drawn in the text
  renderer's typeface, so a theme does not name fonts. A theme chooses a weight for a role, such
  as `title-weight`.

A component's `style` key names a style within its class. A component that names no style is
dressed by the first style of its class in the theme. One style can therefore dress every
panel without each panel naming it.

**The active theme is the `Interface`'s.** The document names it with `"theme"`, and
`Interface::activeTheme(name)` changes it. `Screen::begin(ui)` draws each frame in the ui's
active theme, so an app on a `Screen` does nothing more. An app that drives a
`ComponentRenderer` or an `Immediate` directly hands each the theme with its `theme()`.

**A theme starts from the defaults, not from the theme before it.** A value the new theme does
not name goes back to what it was before any theme, and passing no theme puts every value back.
`Immediate::theme()` follows the same rule. A default the app writes into `dressing()` survives
a switch, unless a theme also names it, in which case the switch puts back the value from
before that theme.

### The base classes: `ui` and `tools`

Both ways of writing a ui are drawn with one `paint::Dressing`, and a theme sets it through one
set of keys. A theme's `ui` style sets the retained components' base, which every other class
is applied over. Its `tools` style sets the immediate layer's. The two are separate classes
because a hud is read at a glance and a tool panel is read closely, so their sizes differ by
about a factor of two. A property the theme does not name keeps its built-in value, so a theme
with no styles draws in the defaults.

Each key means the same thing in both. A key a ui model has nothing to draw with is read and
has no effect there.

### Colours from a base, an accent and a contrast

A `ui` or `tools` style can set every colour at once from three keys:

| Key | Kind | What it sets |
|---|---|---|
| `base` | colour | the surface a panel is drawn on. A base lighter than mid grey makes a light theme, with text darker than its surfaces |
| `accent` | colour | the solid accent: marks, the caret, the focus ring and the filled part of a bar. The highlight is a darker step of it |
| `contrast` | number | how far apart the steps are. 1 is the built-in spread, above 1 spreads them, and 0 gives every surface the base's lightness |

A style that names any of the three has every colour role derived from them, and one it does
not name takes its built-in value. The built-in base is `#1d1f23`, the accent `#4d8ce6`, and
the contrast 1, and the defaults every app starts from are derived from them.

- **Two scales.** `paint::Palette` works out a twelve-step neutral scale of the base's hue and a
  twelve-step accent scale of the accent's hue. Each step's lightness is a fixed distance from
  the base's or the accent's, measured in the OKLCH colour space. The contrast multiplies every
  distance.
- **One step per role.** Each colour role takes one step of one scale. The `ui` and `tools`
  classes differ only in `track`, which sits at the level of a widget in `tools`.
- **Shadows and backdrops are black.** `shadow` is black at 45% in a dark theme and 18% in a
  light one, where a shadow shows more. `backdrop` is black at 60% in either, because what it
  dims is the scene behind the ui and not the theme's own surfaces.
- **Legible by construction.** `highlight` starts one step below the accent and is darkened
  until `active-text` reads on it at 4.5:1. An accent of any lightness therefore gives a
  highlight that carries text.
- **Named roles win.** A role the style names directly is read after the derivation and
  replaces the derived colour. Metrics are never derived.
- **Derived colours replace the app's.** A colour an app wrote into `dressing()` is replaced by a
  derived one, as a colour the theme named would replace it. A switch to another theme puts it
  back.

```json
{ "class": "ui", "name": "default",
  "colors":  [ { "name": "accent", "value": [0.95, 0.55, 0.20, 1.0] } ] }
```

The built-in roles meet these contrasts, and the ui suite checks them:

| Role | Against | At least |
|---|---|---|
| `text`, `secondary-text` | `panel`, `title-bar`, `widget`, `hover`, `bar` | 4.5:1 |
| `active-text` | `highlight`, `hover`, `pressed` | 4.5:1 |
| `disabled-text`, `placeholder` | `panel`, `widget` | 3:1 |
| `border`, `focus`, `mark`, `caret` | `panel` | 3:1 |

Background: [ADR-0128](../../adr/0128-ui-theme-colours-derive-from-base-accent-and-contrast.md)

| Key | What it colours or sizes | Drawn by |
|---|---|---|
| `gap` | what shows between two areas that tile a window | the app, through `dressing()` |
| `bar` | a strip along an edge: a menu bar, a status bar, a strip of tabs | the app, through `dressing()` |
| `panel` | the background of a panel, a window, a menu, a list or a text box | both |
| `border` | an outline | both |
| `title-bar` | the band across the top of a window, and a table's header | immediate |
| `track` | the unfilled part of a bar or a scrollbar, and the box a mark sits in | both |
| `fill` | the filled part of a bar | both |
| `thumb` | the part of a scrollbar or a slider that is taken hold of | both |
| `mark` | the mark in a checked box or a chosen radio button, and the line under the chosen tab of a strip | both |
| `caret` | a text field's caret | both |
| `placeholder` | what an empty text field shows in place of text | both |
| `tab` | the recess a set of tabs sits in, which a tab that is not chosen shows | both |
| `chosen` | the face of the chosen one of a set of choices, such as the chosen tab of a set | both |
| `widget` | the face of a button, a field or a check box at rest | immediate |
| `rule` | a separator | immediate |
| `hairline` | a faint line that divides without drawing the eye, such as the edge of an area | the app, through `dressing()` |
| `text` | ordinary text | both |
| `active-text` | the text of the chosen or hovered item | both |
| `secondary-text` | text that says less than the text beside it, such as a shortcut | both |
| `disabled-text` | the text of a control that cannot be used | both |
| `highlight` | what is drawn behind the chosen item, and behind selected text | both |
| `selected` | what is drawn behind an item that is chosen, under ordinary text | the app, through `dressing()` |
| `hover` | what is drawn behind the item the cursor is on | both |
| `pressed` | the face of a flat button while a press on it is held | retained |
| `focus` | the ring around the component that has the keyboard, or the outline of the field that has it | both |
| `shadow` | the shadow a menu casts, at its darkest | retained |
| `backdrop` | what is laid over everything behind a container that dims it | retained |
| `plate-opacity` | how opaque the plate is behind text drawn over the scene, from 0 to 1, multiplying the panel's alpha. Defaults to 0.6 | retained |
| `panel-opacity` | how opaque `panel` is, from 0 to 1, multiplying its alpha whether it was derived or named. Defaults to 1 | both |
| `line-height` | the height of one line of text | both |
| `padding` | the gap between text and the panel or widget around it | both |
| `spacing` | the gap between one widget and the next | immediate |
| `bar-height` | how tall a menu bar, a toolbar, a title bar, a tab or a button is | both |
| `icon-size` | the side of a button's icon | both |
| `panel-padding` | the gap above and below the items of a dropped panel | retained |
| `scrollbar-width` | how thick a scrollbar is | both |
| `mark-size` | the side of the box, or the width of the disc, a mark sits in | retained |
| `border-width` | how thick an outline is | both |
| `focus-width` | how thick the focus ring is | retained |
| `radius` | how far corners are rounded, 0 for square. A rounded corner fades to nothing across a pixel centred on its curve, so a small radius still reads as round | both |
| `indent` | how far a bullet pushes its text in | immediate |
| `shadow-size` | how far past a menu's edge its shadow fades out. Defaults to 12 | retained |
| `shadow-offset` | how far down a menu's shadow is moved. Defaults to 4 | retained |
| `open-distance` | how far above its place a menu starts as it opens, sliding down into it. Defaults to 4 | retained |
| `hover-duration` | how long a change of fill or text colour eases over, in seconds. Defaults to 0.1, and 0 changes at once | both |
| `open-duration` | how long a menu takes to open, in seconds. Defaults to 0.12, and 0 opens at once | retained |
| `submenu-delay` | how long the cursor rests on a dropped panel's submenu item before it opens, in seconds. Defaults to 0.2 | retained |
| `tooltip-delay` | how long the cursor rests on a component before its tooltip shows, in seconds. Defaults to 0.5 | both |
| `tooltip-quick-delay` | the same, when a tooltip was showing a moment before. Defaults to 0.1 | both |
| `title-weight` | how heavy a window's title and a table's header are set, as CSS numbers weights: below 500 is regular, 500 and above semibold. Defaults to 600 | immediate |

The parts of the retained ui with no class of their own use the base: labels, icons, menu
panels, toolbar strips and button labels.

A centred menu and each panel a menu bar drops cast a shadow: their box moved down by
`shadow-offset`, filled with `shadow`, with an edge that fades to nothing over `shadow-size`.
A `shadow` with no alpha draws none.

A change of a fill or a text colour eases over `hover-duration`, whatever caused it: the cursor
moving onto a button, a press, a menu item chosen from the keyboard. A fill that appears fades
in its own colour, and one that goes fades out in its own colour. A centred menu and a dropped
panel open over `open-duration`, fading in as they slide down `open-distance` into place, and
close at once. Only colour, opacity and that slide move: a component is picked where it lands
from the first frame. In the immediate layer a widget's fill eases, and its text changes at once.

An app can also set these values in code, through `ComponentRenderer::dressing()`,
`Immediate::dressing()` or `Screen::Options::dress`. Where the theme names a value, the theme
wins.

### A theme beneath another

`Interface::beneath(document)` puts the themes a document names beneath every theme the ui has
loaded, so the ui's own document names only what it changes. The shell puts the game theme
beneath every game's document this way.

- **A style fills in what it does not name** from the style beneath of the same class and name,
  and of the same `state` for a button. Its own values win.
- **A style beneath with no match is added** after the theme's own styles, so the theme's own
  first style of a class still dresses a component that names none. Each theme takes its own
  copy, so filling one theme's copy in later changes no other theme.
- **A game overrides the game theme by naming its style.** The game theme's base style is
  `{ "class": "ui", "name": "default" }`, so a game's `ui` style named `default` is filled in
  from it. A `ui` style under another name comes first and is read alone, and the game theme's
  base style is not read at all.
- **A ui with no theme takes the document's themes** as its own, and the first is active.
- It runs after `load()`, which replaces every theme.

### Per-component classes

| Class | Read by | Keys |
|---|---|---|
| `panel` | `Panel` | `background`, `border`, `border-width`, `radius` |
| `bar` | `Bar` | `track`, `fill`, `border`, `border-width`, `radius` |
| `scrollbar` | `Scrollbar` | `track`, `thumb`, `border`, `border-width`, `radius` |
| `slider` | `Slider` | `track`, `fill`, `thumb`, `border`, `border-width`, `radius`, `mark-size` |
| `checkbox` | `CheckBox` | `background`, `mark`, `border`, `text`, `border-width`, `mark-size` |
| `radio` | `RadioButton` | as `checkbox` |
| `list` | `SelectList` | `background`, `border`, `highlight`, `text`, `active-text`, `border-width`, `radius`, `line-height` |
| `tabs` | `TabBar`, `TabPage` | `background`, `tab`, `chosen`, `mark`, `text`, `active-text`, `border`, `bar-height`, `radius` |
| `textbox` | `TextBox` | `background`, `border`, `text`, `caret`, `placeholder`, `highlight`, `border-width`, `radius`, `line-height` |
| `hud` | `ComponentRenderer::hud()`, the statistics overlay | `background`, `border`, `text`, `plate-opacity`, `border-width`, `padding`, `radius`, `line-height` |
| `button` | `Button` | nine images and `corner`; see below |
| `tools` | `Immediate` | the base keys above |

Every class above except `tools` may also name `focus` and `focus-width`, the colour and
thickness of the ring drawn around a component that has the keyboard. A class that names
neither uses the base values. A component with no class (a label, an icon, a box) is ringed
with the base values.

A `textbox` `highlight` is drawn behind the selected text, and the text is drawn over it in
its usual colour. An opaque highlight hides the selected text.

### Text over the scene

`ComponentRenderer::hud(canvas, lines, place)` draws lines of text over the scene, such as a
score or a label, on one plate. A game draws its HUD this way rather than in colours of its own.
The lines are set in tabular figures, so a score that changes keeps its width. The statistics
overlay is too.

- **The `hud` class dresses it.** The plate is the panel's colour, or the style's `background`,
  with its alpha multiplied by `plate-opacity`. The lines are in `text`, inside `padding`, a
  `line-height` apart. `place.style` names the style, and empty takes the theme's first.
- **It is placed by an anchor.** `place.at` is a point in the canvas's pixels, and
  `place.anchor` is the share of the plate put there: 0, 0 for its top left, 0.5, 0.5 for its
  centre. A point in a canvas's own space goes through `Canvas::toPixels()` first.
- **It returns the plate's box**, so the next block can be placed under it.
- **It writes into the screen's canvas**, because the text writer the renderer was built with
  writes there.
- **The statistics overlay is dressed the same way**, by the `hud` style named `statistics`, or
  the theme's first `hud` style when there is none of that name.

The game theme carries a `hud` style named `default`, 60% opaque with 12 pixels of padding and
6 pixel corners, and one named `statistics`, 75% opaque.

### Button styles

A `button` style draws a nine-slice skin: up to nine images named `top-left`, `top-right`,
`bottom-left`, `bottom-right`, `top`, `bottom`, `left`, `right` and `center`. Corners are
drawn at the size of the number `corner`, edges are stretched along the sides, and the centre
fills the rest. Every image is optional. A style that resolves no image draws the button flat.

A button style also names which look it dresses with `"state"`:

| `state` | Used when |
|---|---|
| `normal` (default) | the button is idle |
| `hover` | the cursor is over it |
| `press` | it is held down |
| `disabled` (or `inactive`) | it is not usable |

Several button styles can share one name, one per state. A disabled button's label is
written in `disabled-text` from the `ui` style. The focus ring on a button comes from the
first button style with that name, whatever its state.

## Images

The ui document names images but never loads them. The app turns each source name into an
image.

```cpp
std::size_t resolved = vgui_->resolveImages([this](const std::string& source) {
    return lookUpOrUpload(source);   // returns a v3d::ui::Image
});
```

- The callback returns a `v3d::ui::Image`: a texture handle and the two texture coordinates
  that bound the image inside it. One sprite sheet can serve every icon on a screen. A bare
  `TextureHandle` converts to the whole texture.
- What a source name means is the app's choice. An app with a sprite sheet looks the name up
  in `asset::media::SpriteSheets` and returns the sheet's handle and the region's corners.
- Run `resolveImages()` once the renderer exists. It resolves theme images and every icon and
  button icon in every container. It returns how many were resolved, and logs each source it
  could not resolve.
- An app that never runs it draws skinned buttons flat and icons not at all, with no error.
- `Icon::source(name)` and `Button::icon(name)` point a component at a new source. The old
  image is dropped at once, so the component shows nothing until it is resolved again. Call
  `resolveComponentImages(resolve, component)` to resolve just that component. A later full
  pass keeps the change.
