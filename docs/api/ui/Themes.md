# Themes, styles and images

A theme sets how a ui looks without changing its document. It can also name images, which the
app loads.

## Themes and styles

A theme is data in the ui document. It is a list of styles. Each style has a `class`, a
`name`, and up to four arrays of properties: `colors`, `numbers`, `fonts` and `images`.

```json
{ "class": "panel", "name": "plate",
  "colors":  [ { "name": "background", "value": [0.1, 0.1, 0.12, 0.9] } ],
  "numbers": [ { "name": "radius", "value": 6 } ] }
```

- A colour's `value` is four numbers, RGBA from 0 to 1.
- A number's `value` is one number.
- A font has `source`, and optionally `face`, `size`, `bold` and `italics`.
- An image has a `source`, which the app resolves (see [Images](#images)).
- Any property may carry an `align` hint.

A component's `style` key names a style within its class. A component that names no style is
dressed by the first style of its class in the theme. One style can therefore dress every
panel without each panel naming it.

Changing the active theme is `Engine::activeTheme(name)`. Pass the theme to the renderer with
`Screen::theme()` or `ComponentRenderer::theme()`.

### The `ui` class: the base for retained mode

A theme's `ui` style sets the defaults every other class is applied over. A property the
theme does not name keeps its built-in value, so a theme with no styles draws in the
defaults.

| Kind | Keys |
|---|---|
| colours | `panel`, `border`, `track`, `fill`, `thumb`, `mark`, `caret`, `placeholder`, `tab`, `text`, `active-text`, `disabled-text`, `highlight`, `hover`, `focus` |
| numbers | `line-height`, `padding`, `bar-height`, `icon-size`, `panel-padding`, `scrollbar-width`, `mark-size`, `border-width`, `focus-width`, `radius` |

The parts of the ui with no class of their own use the base: labels, icons, menu panels,
toolbar strips and button labels.

An app can also set these values in code through `ComponentRenderer::dressing()`, or through
`Screen::Options::dress`. Where the theme names a value, the theme wins.

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
| `tools` | `Immediate` | see [Immediate mode](ImmediateMode.md#immediate-mode) |

Every class above except `tools` may also name `focus` and `focus-width`, the colour and
thickness of the ring drawn around a component that has the keyboard. A class that names
neither uses the base values. A component with no class (a label, an icon, a box) is ringed
with the base values.

A `textbox` `highlight` is drawn behind the selected text, and the text is drawn over it in
its usual colour. An opaque highlight hides the selected text.

The base `ui` and the `tools` class are separate because a HUD and a tool panel use the same
keys at about twice the size of each other.

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
  in `config::SpriteSheets` and returns the sheet's handle and the region's corners.
- Run `resolveImages()` once the renderer exists. It resolves theme images and every icon and
  button icon in every container. It returns how many were resolved, and logs each source it
  could not resolve.
- An app that never runs it draws skinned buttons flat and icons not at all, with no error.
- `Icon::source(name)` and `Button::icon(name)` point a component at a new source. The old
  image is dropped at once, so the component shows nothing until it is resolved again. Call
  `resolveComponentImages(resolve, component)` to resolve just that component. A later full
  pass keeps the change.
