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
- A number's `value` is one number.
- An image has a `source`, which the app resolves (see [Images](#images)).
- Any other member of a style is not read. Every component's text is drawn in the text
  renderer's font, so a theme does not name fonts.

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

| Key | What it colours or sizes | Drawn by |
|---|---|---|
| `panel` | the background of a panel, a window, a menu, a list or a text box | both |
| `border` | an outline | both |
| `title-bar` | the band across the top of a window, and a table's header | immediate |
| `track` | the unfilled part of a bar or a scrollbar, and the box a mark sits in | both |
| `fill` | the filled part of a bar | both |
| `thumb` | the part of a scrollbar or a slider that is taken hold of | both |
| `mark` | the mark in a checked box or a chosen radio button | both |
| `caret` | a text field's caret | both |
| `placeholder` | what an empty text field shows in place of text | both |
| `tab` | a tab that is not the chosen one | both |
| `widget` | the face of a button, a field or a check box at rest | immediate |
| `rule` | a separator, and the line under a tab strip | immediate |
| `text` | ordinary text | both |
| `active-text` | the text of the chosen or hovered item | both |
| `disabled-text` | the text of a control that cannot be used, and secondary text | both |
| `highlight` | what is drawn behind the chosen item, and behind selected text | both |
| `hover` | what is drawn behind the item the cursor is on | both |
| `focus` | the ring around the component that has the keyboard, or the outline of the field that has it | both |
| `line-height` | the height of one line of text | both |
| `padding` | the gap between text and the panel or widget around it | both |
| `spacing` | the gap between one widget and the next | immediate |
| `bar-height` | how tall a menu bar, a toolbar, a title bar, a tab or a button is | both |
| `icon-size` | the side of a button's icon | retained |
| `panel-padding` | the gap above and below the items of a dropped panel | retained |
| `scrollbar-width` | how thick a scrollbar is | both |
| `mark-size` | the side of the box, or the width of the disc, a mark sits in | retained |
| `border-width` | how thick an outline is | both |
| `focus-width` | how thick the focus ring is | retained |
| `radius` | how far corners are rounded, 0 for square | both |
| `indent` | how far a bullet pushes its text in | immediate |

The parts of the retained ui with no class of their own use the base: labels, icons, menu
panels, toolbar strips and button labels.

An app can also set these values in code, through `ComponentRenderer::dressing()`,
`Immediate::dressing()` or `Screen::Options::dress`. Where the theme names a value, the theme
wins.

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
| `tabs` | `TabBar`, `TabPage` | `background`, `tab`, `highlight`, `text`, `active-text`, `border`, `bar-height`, `radius` |
| `textbox` | `TextBox` | `background`, `border`, `text`, `caret`, `placeholder`, `highlight`, `border-width`, `radius`, `line-height` |
| `button` | `Button` | nine images and `corner`; see below |
| `tools` | `Immediate` | the base keys above |

Every class above except `tools` may also name `focus` and `focus-width`, the colour and
thickness of the ring drawn around a component that has the keyboard. A class that names
neither uses the base values. A component with no class (a label, an icon, a box) is ringed
with the base values.

A `textbox` `highlight` is drawn behind the selected text, and the text is drawn over it in
its usual colour. An opaque highlight hides the selected text.

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
