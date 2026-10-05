# The User Interface

This document is for someone building a ui for an app with `api/ui` (`v3dlib_ui`). It covers
the two ways to write a ui, the document format, the components, themes, input, and the shell
classes a game uses. How the library works inside is in
[internals/UserInterface.md](../internals/UserInterface.md).

- [Two ways to write a ui](#two-ways-to-write-a-ui)
- [Getting set up](#getting-set-up)
- [The ui document](#the-ui-document)
- [The box model](#the-box-model)
- [Components](#components)
- [Themes and styles](#themes-and-styles)
- [Images](#images)
- [Commands](#commands)
- [The mouse](#the-mouse)
- [The keyboard](#the-keyboard)
- [Editing text in a TextBox](#editing-text-in-a-textbox)
- [Enabled and disabled](#enabled-and-disabled)
- [Clipping](#clipping)
- [Immediate mode](#immediate-mode)
- [GameMenu, StatisticsOverlay and FileChooser](#gamemenu-statisticsoverlay-and-filechooser)
- [Testing a ui](#testing-a-ui)

## Two ways to write a ui

`api/ui` offers two models. Both draw onto the same `realtime::Canvas`, use the same box
drawing and the same text callbacks, so they look like one ui on screen.

**Retained mode** is a tree of components. `v3d::ui::Engine` builds the tree once from a JSON
document. The app looks components up by name and changes them when the game's state changes.
`v3d::ui::paint::ComponentRenderer` draws the tree. Use it for an authored ui: a menu bar, a
settings screen, a HUD, a game's pause menu.

**Immediate mode** is a sequence of calls. `v3d::ui::Immediate` is written between `begin()`
and `end()` every frame, and each call both lays out and draws one widget. Nothing has to be
kept in step with the game, because the panel is rebuilt from live state each frame. Use it
for a ui that shows live values: a debug readout, a tool panel.

How to choose:

- A retained component shows whatever it was last told. If the app forgets to update it, it
  shows a stale value.
- An immediate panel cannot be stale, but it cannot be looked up by name or styled from a
  document.
- Nothing enforces the choice. A HUD built with `Immediate` works, but its hover runs a frame
  behind the cursor (see [Immediate mode](#immediate-mode)).

The two read different style classes from a theme: `ui` for retained mode and `tools` for
immediate mode. See [Themes and styles](#themes-and-styles).

### Everything is drawn as quads on the app's canvas

The ui owns no device, pass or draw call. Panels, highlights and text are all quads appended
to a canvas the app is already filling. The app submits that canvas itself, through
`Engine3D::quads()`, so a ui adds no pass and no draw of its own. [Rendering.md](Rendering.md)
covers the canvas and the quad pipeline.

```
app's Canvas  <--  ComponentRenderer::draw(canvas, engine)   the retained tree
              <--  Immediate::begin(canvas, input) ... end()  immediate calls
              <--  TextRenderer::draw(canvas, ...)            a line of text anywhere
```

### Text is supplied by the app

Both models take text through two callbacks declared in [`paint/Text.h`](../../api/ui/paint/Text.h):

| Callback | Signature | Does |
|---|---|---|
| `paint::Measure` | `float(std::string_view)` | returns how wide a string is when drawn, in pixels |
| `paint::Write` | `void(std::string_view, const glm::vec2& pen, const glm::vec4& colour)` | draws a string with its pen on the baseline |

Neither renderer names a font type. This keeps `api/ui` testable without a window or a GPU.
The callbacks take a `std::string_view`, so measuring a label does not allocate. The view
must stay valid for the length of the call.

`v3d::ui::paint::TextRenderer` supplies the pair. It holds one font, packed into one atlas of
signed distance field glyphs. `measure(size)` and `write(canvas, size)` return callbacks for
one drawn size. Two sizes from one `TextRenderer` share the same atlas.

`TextRenderer` uploads its atlas through an `Upload` callback rather than a Vulkan type. No
`api/ui` header names a Vulkan type. An app that draws the canvas through its own renderer
can supply its own `Upload`.

## Getting set up

### Screen: the renderers and the canvas

An app built on `Engine3D` uses `v3d::ui::shell::Screen` and builds none of the pieces itself.
A `Screen` builds:

- the `TextRenderer`, uploading its atlas through the `Engine3D`
- the `ComponentRenderer` over that text
- a `StatisticsOverlay`, unless `Options::statistics` is false
- an `Immediate` layer, if `Options::immediate` is true
- the canvas all of these draw into

```cpp
v3d::ui::shell::Screen::Options options;
options.size = fontSize;          // text size at a scale of one
options.immediate = true;         // also build an Immediate layer
screen_ = boost::make_shared<v3d::ui::shell::Screen>(&engine_, assetManager, logger, options);
```

Each frame:

```cpp
if (!screen_->begin()) {
    return;                       // minimised: the frame was presented empty
}
if (screen_->resized()) {
    resize(screen_->canvas().width(), screen_->canvas().height());   // the app's own things
}
// ... the app draws its own content into screen_->canvas() if it likes ...
screen_->draw(ui_.get(), statistics);   // the retained ui, then the statistics over it
engine_.quads()->submit(screen_->canvas(), pass.get());
```

- `begin()` begins the frame through `Engine3D::beginFrame`, sizes the canvas to the frame,
  and clears it. It returns false when the window is minimised or there is no renderer.
- `resized()` is true on the first frame and on any frame whose size differs from the last.
  The app resizes its own resources then.
- The passes, the draw order and where the canvas is submitted stay the app's.
- `Options::dress` is a function `void(paint::Dressing*, float size)` that fills in the
  dressing (colours and metrics, see [Themes and styles](#themes-and-styles)) for a text
  size. When it is empty, the line height is set to 1.4 times the size.
- `scale(factor)` redraws text larger or smaller. It rebuilds everything that depends on the
  font size, calls `dress` again, and applies the theme again.
- `theme(theme)` dresses the component renderer and the immediate layer from a theme, and
  keeps it for later rebuilds.

Do not keep a reference to `components()`, `immediate()` or `text()` across frames.
`scale()` replaces them, and an old reference points at an object that is no longer drawn.

An app that needs a piece `Screen` does not build writes it beside the screen. An app that
does not use `Engine3D` builds `TextRenderer` itself with its own `Upload`.

### Loading the document

```cpp
vgui_ = boost::make_shared<v3d::ui::Engine>(events(), dispatcher(), logger());
const boost::json::object* ui = document(v3d::config::Type::Ui);
if (ui && !vgui_->load(*ui)) {
    return false;
}
vgui_->resolveImages(resolver);   // once the renderer exists; see Images
```

`Engine::load()` reads the themes and containers. It returns false and logs the reason when
the document is malformed. `container(name)` returns a loaded container, and
`Container::get(name)` finds a component anywhere inside it.

### The keyboard adapter

`v3d::ui::shell::Keyboard` connects the ui to SDL keyboard events. Build one and pass it every
event from the app's `onEvent()`:

```cpp
uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui_, dispatcher_, window());

bool App::onEvent(const SDL_Event& event) {
    return uiKeys_->event(event);
}
```

`onEvent()` runs before the input bindings ([Engine.md](Engine.md) covers the event order).
Returning true stops a key the ui used from also firing the command bound to it. If an app
overrides `onEvent()` for something else and forgets to pass the event on, the ui cannot be
typed into and nothing reports why.

The adapter does four things:

- It turns a key-down event into `Keys::press()`, and a text event into `Keys::text()`.
- It reads shift and control off the event itself, so the modifiers are the ones held when
  the key arrived.
- It supplies the SDL clipboard. `Keyboard::clipboard()` is public, so an app that builds an
  `input::Keys` directly can pass it the same pair.
- It turns platform text input on while a `TextBox` has the focus and off when the focus
  leaves. It follows `Engine::onFocus()`, so a box clicked and typed into in the same frame
  does not lose its first character.

Rules the adapter follows:

- Only a key going down is ever taken. A key release always passes through, so
  `input::KeyState` never holds a key down that the ui took.
- A key that `api/input` has no name for is never taken.
- The key name comes from `input::keyName()`, the same table the bindings use.
- `Engine::onFocus()` holds one listener, and the last caller wins. The adapter sets it. An
  app that wants its own listener sets it after building the adapter and clears it before
  the adapter is destroyed.
- Only a component whose type takes typed text (a `TextBox`) turns text input on.

The mouse is not routed by the adapter. See [The mouse](#the-mouse).

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

- `themes` (required) is an array of themes. See [Themes and styles](#themes-and-styles).
- `theme` (optional) names the active theme. A name that matches no theme fails the load.
- `containers` (required) is an array. Each container has a `name`, a `visible` flag and a
  `components` array. A container is shown and hidden as a whole.

Every component entry has a `type` and a `name`. These keys apply to every component:

| Key | Type | Default | Meaning |
|---|---|---|---|
| `type` | string | required | the component type, from the table in [Components](#components) |
| `name` | string | required | what the app looks it up by |
| `position` | `[x, y]` | `Auto` | offset from the anchored corner; each a length |
| `size` | `[width, height]` | `Auto` | each a length |
| `anchor` | string | `top-left` | `top-left`, `top-right`, `bottom-left`, `bottom-right` or `centre` (`center` also reads) |
| `style` | string | none | a style name within the component's style class |
| `visible` | bool | true | |
| `enabled` | bool | true | see [Enabled and disabled](#enabled-and-disabled) |
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

- A menu bar takes the top of the canvas.
- A `top` toolbar takes a band under whatever is already there.
- A `left` toolbar runs down the side of what is left.

`ComponentRenderer::insets(engine)` returns the space the strips take: the left inset in x
and the top inset in y. An app draws its own content in the rest. It uses the same
calculation as the draw, so the two always agree.

Within a container, the draw order is:

1. every non-strip component, by `depth`, with the listed order kept between equal depths
2. toolbars, each at the corner the stacking gave it
3. menu bars, last, so an open menu's panel drops over the strips below

Containers are drawn in the order the document lists them.

## Components

The `type` string in the document, the class in `v3d::ui::component`, and the keys each type
reads besides the common ones:

| `type` | Class | Draws | Keys | Pickable / focusable |
|---|---|---|---|---|
| `panel` | `Panel` | a filled box with a border, optionally rounded | none | no / no |
| `label` | `Label` | its text, wrapped to the width it was given, or one line when the width is `Auto` | `label` | no / no |
| `icon` | `Icon` | an image at the component's size | `source` (required) | no / no |
| `bar` | `Bar` | a track and the filled fraction of it | `fraction` (0 to 1), `direction` (`vertical`, else horizontal) | no / no |
| `button` | `Button` | a label, an icon, or a nine-slice skin | `label`, `icon`, `toggle`, `command`, `context` | yes / yes |
| `checkbox` | `CheckBox` | a square mark and a label beside it | `label`, `checked`, `command`, `context` | yes / yes |
| `radio` | `RadioButton` | a round mark and a label beside it | as `checkbox`, plus `group` | yes / yes |
| `scrollbar` | `Scrollbar` | a track and a thumb | `direction` (`horizontal`, else vertical), `content` and `page` (together), `offset` | yes / yes |
| `list` | `SelectList` | a plate and as many rows as fit, the chosen row highlighted | `items` (array of strings), `selected`, `command`, `context` | yes / yes |
| `slider` | `Slider` | a track, the fill up to the value, and a thumb | `minimum` (0), `maximum` (1), `step` (0 for continuous), `value`, `command`, `context` | yes / yes |
| `tabs` | `TabBar` | a strip of tabs, and the selected page under it | `selected`; its `children` are `tab` entries | yes / yes |
| `tab` | `TabPage` | the components it holds | `label` | no / no |
| `textbox` | `TextBox` | a plate, one line of text, a highlight behind the selection, and a caret while focused | `text`, `placeholder`, `limit` (bytes), `command`, `context` | yes / yes |
| `hbox`, `vbox` | `HorizontalBox`, `VerticalBox` | nothing; they place their children | `spacing`, `stretch`, `wrap` | no / no |
| `toolbar` | `Toolbar` | a row or column of buttons at an edge | `edge` (`top` or `left`), `buttons` (array of button entries) | yes / no |
| `menubar` | `MenuBar` | a strip of menu labels that drop panels | `menus`: array of `{ "label", "items" }` | yes / no |
| `menu` | `Menu` | a game menu, one level at a time, centred | `items` | no / no |

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
- **A `SelectList`** with no `items` is filled by the app with `items(rows)`. It draws only
  the rows its box shows and clips them to its plate.
- **A `TabBar`** draws and lays out only the selected page. Components on other pages have no
  box that frame, so they are not picked or focused.
- **A `Toolbar`** button entry reads every button key, plus `name`, `style`, `visible` and
  `enabled`. A button with an `icon` is sized to the icon; otherwise to its label. A toolbar
  button falls back to its label when its icon is not resolved.
- **A `Menu` or `MenuBar` item** has a `label`, a `type`, and optionally `command` and
  `context`. Item types are `action`, `submenu` (which holds its own `items`), `check`,
  `radio`, `input`, `numeric_input` and `key_input`. A check and a radio item are marked by
  the app, the same as a check box. Radio items are drawn with the same square mark as check
  items.
- **An open menu bar takes every press.** While a menu is open, a press anywhere on screen
  closes it and is consumed. A click meant for the scene under an open menu does nothing.

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
| `tools` | `Immediate` | see [Immediate mode](#immediate-mode) |

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

## Commands

A control sends a command when it is activated. In the document, a command is the pair
`command` and `context`. Both must be present, or the component carries no command. The
`context` is resolved through the event engine, and the command is sent on the ui's
dispatcher as a `v3d::event::Event`. An app answers it like any other command.

What activates each control:

| Component | Mouse | Keyboard |
|---|---|---|
| `Button`, `CheckBox`, `RadioButton` | a press on it | `return` or `space` while focused |
| `SelectList` | a press on a row selects it, then sends | an arrow, `home` or `end` that changes the row selects it, then sends |
| `Slider` | a press or drag that changes the value | a key that changes the value |
| `TextBox` | never; a click places the caret | `return` only |
| `TabBar` | a press on a tab changes the page; nothing is sent | arrows change the page; nothing is sent |
| `Scrollbar` | a press jumps the thumb there and a drag follows; nothing is sent | see [The keyboard](#the-keyboard); nothing is sent |
| toolbar button, menu item | a press on it | through the game menu's own commands |

The mouse and the keyboard both ask `ui::input::command()` which event a component carries,
so a control the mouse activates is always one the keyboard activates too. A command is sent
as the press lands, not on release.

## The mouse

`v3d::ui::input::Cursor` routes the mouse over a `ui::Engine`. The app passes it points, and
each call returns whether the ui used the point. The app acts on a point only when the ui
did not.

```cpp
uiCursor_ = boost::make_shared<v3d::ui::input::Cursor>(vgui_, dispatcher(), text->measure());

bool taken = uiCursor_->press(cursor);   // also motion(point) and release(point)
if (!taken) {
    // the press is the app's: pick in the scene, drive a camera ...
}
```

- `motion(point)` moves the hover highlight, and follows a held press.
- `press(point)` offers the point to the ui and sends a command if it lands on a control.
- `release(point)` ends a held press. It sends nothing.

The `Measure` argument is optional. It places the caret when a `TextBox` is clicked. Pass the
same `Measure` the renderer drawing the ui was given, so the two agree on where each
character is. A cursor given none still routes every press, but a click in a text box leaves
the caret where it was.

`Cursor` is not wrapped by the shell, because an app often interleaves the ui's press with
its own use of the mouse. The editor offers the ui a press first and drives its camera with
any press the ui did not take.

How a point is routed:

- Each visible container is offered the point, in the order the document lists them. The
  first container that takes it ends the search.
- Within a container, the menu bars are offered the point first, then the toolbars, then the
  component tree. This is the reverse of the draw order.
- In the tree, the topmost component under the point is found. A child is offered the point
  before its parent.
- A component that is not `pickable()` is passed over, and the search continues beneath it.
  A HUD of labels over a scene therefore leaves the scene clickable.
- A press that lands on nothing pickable is not consumed. It belongs to the app.
- A strip takes a press anywhere on it, including the gaps between its buttons. A strip
  that is hidden, disabled or marked `"pickable": false` is skipped, and the press falls to
  the tree under it.
- A press that lands is held until release. While it is held, every `motion()` goes to the
  held component. This is how a scrollbar thumb, a slider thumb and a text selection are
  dragged.

Hover:

- A button lights up under the cursor, on a strip or in the tree. Only the component a press
  would land on is lit, so unpickable labels never flicker as the cursor crosses them.
- Hover is a button state only. No other component has a hover look.

Everything is tested against the boxes left by the last draw. **Draw before routing input,**
or an app sees a dead ui for one frame. Nothing can be picked before the first frame is
drawn.

An `Immediate` layer has its own equivalent: `Immediate::capturing()`. See
[Immediate mode](#immediate-mode).

Whether to offer the ui a cursor at all is the app's choice. A game that holds the mouse in
relative mode for mouselook has no meaningful cursor. `voxel` passes its immediate debug
panel a real `Input` only while its menu is up, and an empty `Input` otherwise.

## The keyboard

`v3d::ui::input::Keys` routes keys to whichever component has the focus. `shell::Keyboard`
builds one and calls it, so most apps never call it directly.

### Focus

The **focus** is the one component that receives keys. `ui::Engine` holds it.

- A ui with nothing focused takes no keys. A game's movement keys keep working until
  something is clicked into or `focusFirst()` is called.
- A press moves the focus. It lands on the component pressed if that component is
  `focusable()`. Otherwise the focus is cleared. Clicking into a text box therefore means
  "type here", and clicking elsewhere stops typing.
- `escape` clears the focus.
- `Engine::focused()` returns the focused component. `Engine::onFocus(callback)` reports
  every change, in the same frame.
- A focused component is drawn with a ring around its box, in the `focus` colour at
  `focus-width` thickness of its style class.

**Starting a keyboard-driven screen.** Call `Engine::focusFirst()` as the screen goes up. It
puts the focus on the first focusable component. Nothing else gives out a first focus except
a press: `tab` does nothing while nothing is focused, so a HUD nobody is looking at never
takes the keyboard. No app in this tree calls `focusFirst()`; it is covered by
`api/ui/tests` only.

### Tab order

`tab` moves the focus to the next focusable component, and `shift`+`tab` to the previous
one. The order is the draw order:

- containers in the order the document lists them
- components by `depth`, keeping listed order between equal depths
- a flow box's children in the order it holds them

The order wraps at each end. Hidden and disabled components are skipped, with everything they
hold. If the focused component has left the order (hidden, disabled or removed), `tab`
restarts at the first component. To change the tab order, reorder the document. There is no
tab index.

`tab` is taken whenever something is focused, even if the focus cannot move.

Every control is focusable by default. To leave one out of the tab order, set
`"focusable": false`.

### Keys and characters

The ui receives two kinds of keyboard input:

| Call | Receives | Example |
|---|---|---|
| `Keys::press(key, shifted, controlled)` | a key name from `api/input`, and whether shift and control are held | `"backspace"`, `"arrow_left"`, `"return"`, `"tab"` |
| `Keys::text(utf8)` | characters the platform composed, as UTF-8 | `"e"`, `"É"` |

Characters arrive with shift already applied. A dead key and the key after it arrive as one
character. An input method's keys arrive as whatever characters it produced. `input::Keyboard`
also raises these characters as `event::TextInput` events for anything else that wants them.

The modifiers are passed in because a key name carries none, and `api/ui` cannot read SDL's
keyboard state.

### What each control does with a key

| Focused | Keys it takes |
|---|---|
| `Button`, `CheckBox`, `RadioButton` | `return` and `space` send its command |
| `SelectList` | `arrow_down`, `arrow_up`, `home`, `end` move the chosen row and send the command |
| `TabBar` | `arrow_right`, `arrow_left`, `home`, `end` change the page |
| `Scrollbar` | arrows along its direction move it by a line; `pageup` and `pagedown` by a page; `home` and `end` to the ends |
| `Slider` | `arrow_right`, `arrow_left` by one step; `pageup`, `pagedown` by a tenth of the range; `home`, `end` to the ends; sends its command when the value changes |
| `TextBox` | editing keys, and every key that types a character; see [Editing text](#editing-text-in-a-textbox) |

Rules:

- **Only a `TextBox` takes letters.** A letter reaching a focused button goes on to the
  app's bindings. A text box takes every key that will also arrive as a character, so typing
  "w" into a box does not also walk the player forward.
- A list and a tab bar do not wrap at the ends. Pressing an arrow on a list with nothing
  chosen chooses the first row.
- A scrollbar's line is the row height of its bound list, or `Scrollbar::lineStep` pixels
  for a range of its own. A scrollbar with nothing to scroll takes no keys.
- A slider with no `step` moves by a hundredth of its range per arrow.
- A key that moves nothing (an arrow at the end of a slider or scrollbar) is not taken, and
  goes on to the app.
- With control held, only a `TextBox` takes a key, and only the four editing chords. Every
  other chord goes on to the app, so `ctrl`+`s` still saves while somebody is typing.

### Game menus use commands, not focus

A game's pause menu (`Menu` and `GameMenu`) does not use focus or `Keys`. It is driven by the
`menuNext`, `menuPrevious`, `selectMenu` and `showGameMenu` commands that an app's bindings
send. A game can therefore bind its menu to a gamepad or any keys, and rebind them from inside
the menu. Menu items are not focusable. The two models meet only where a menu item captures a
key, which the app routes through `GameMenu::capture()`.

## Editing text in a TextBox

A `TextBox` holds one line of UTF-8 text, a **caret** (where typing goes) and an **anchor**.
Both are byte offsets. The selection is the run of text between the anchor and the caret.
Nothing is selected when the two are equal.

Every operation that moves the caret either brings the anchor with it, which leaves nothing
selected, or leaves the anchor behind, which selects the run the caret crossed.

| Input | Effect |
|---|---|
| a character | replaces the selection, or is inserted at the caret |
| `backspace`, `delete` | remove the selection, or one character before or after the caret |
| `arrow_left`, `arrow_right`, `home`, `end` | move the caret; with nothing held, an arrow over a selection lands on its end |
| `shift` + any of those | move the caret and leave the anchor, extending the selection |
| `return` | sends the box's command; the app reads `text()` |
| `control` + `a` | selects all |
| `control` + `c` | copies the selection |
| `control` + `x` | copies the selection and removes it |
| `control` + `v` | replaces the selection with the clipboard's text |
| a press | puts the caret and the anchor at the character under the point |
| a drag | moves the caret with the cursor, selecting from where the press landed |

- `limit` caps the text's length in bytes. A character or paste that would pass it is
  refused, but the key is still taken.
- The line slides left when the caret would pass the far edge, so a full box can still be
  typed into.
- `placeholder` is shown in the `placeholder` colour while the box is empty.
- The clipboard is a pair of callbacks, `Keys::Clipboard { read, write }`. `shell::Keyboard`
  supplies SDL's. Without a `write`, a cut does not remove the text. Without a `read`, a
  paste inserts nothing.
- A copy or a cut with nothing selected does nothing, so the clipboard keeps what it had.
- Placing the caret by clicking needs the `Measure` passed to `Cursor`.

## Enabled and disabled

`Component::enabled(false)` marks a component that cannot be used right now. It is a property
of the component, not a hover or press state. It differs from `pickable()`: `pickable(false)`
means "scenery, never clickable", such as a label or a panel.

**Disabling a component disables everything it holds.** A box is how a screen greys out a
group of controls. A disabled toolbar disables its buttons, and a disabled menu disables its
items.

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
focus. It draws no ring and takes no keys, but `Engine::focused()` still returns it and
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

[Rendering.md](Rendering.md) covers how the canvas carries a clip.

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

## GameMenu, StatisticsOverlay and FileChooser

These classes in `v3d::ui::shell` are the parts of a game's shell that every game would
otherwise write the same way. An app that writes its own version of one has diverged from
the api rather than customised it.

### GameMenu

`GameMenu` is the pause menu a game puts up over itself.

```cpp
menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
    scene_->pause(suspended);
});
```

- It shows and hides a container, `game-menu` by default, and drives the `Menu` inside it,
  `main-menu` by default. Both names can be passed to the constructor.
- It answers its own commands in the `ui` context: `showGameMenu` toggles it, and
  `menuPrevious`, `menuNext` and `selectMenu` navigate it while it is up. An app binds these
  in its mappings, for example `escape` to `showGameMenu`. These command names cannot be
  changed.
- `toggle()` opens the menu, or steps back out of a submenu. Closing the top level resumes
  the game.
- The `Suspend` callback runs with `true` as the menu opens and `false` as it closes. Do
  only small things in it, such as setting a flag. Tearing something down inside it can
  break the toggle that called it.
- `capturing()` is true while an input item is waiting for a value. The app sends the next
  key to `capture(value)` instead of acting on it. This is how a rebinding screen works.
- If the document names no such container, every call does nothing and the game runs without
  a menu.

### StatisticsOverlay

`StatisticsOverlay` draws the loop's frame timings in the corner of the frame. `Screen`
builds one unless told not to.

- It starts hidden. `toggle()` and `visible(bool)` show it.
- It draws from a `Sample` the app hands it each frame: the mean and last frame time in
  nanoseconds, the simulation steps the last frame ran, and a list of named timing spans.
  The app copies these out of `engine::Statistics`.
- `size` defaults to 16 pixels, smaller than the ui's own text.
- `lines(sample)` returns the text without drawing it.

### FileChooser

`FileChooser` lets a player choose a file to open or a name to save under, drawn in the
game's own ui. It works over a fullscreen game and follows the theme.

It adds no component type. It drives components the document names (defaults in
`FileChooser::Names`):

| Name | Component | Holds |
|---|---|---|
| `chooser` | a container | shown and hidden |
| `files` | a `list` | the directory listing |
| `name` | a `textbox` | the name typed or picked |
| `folder` | a `label` | where the list is, or the question being asked |

```cpp
chooser_->open(FileChooser::Mode::Save, directory, ".json", [this](const boost::filesystem::path& chosen) {
    save(chosen);
});
```

- The app routes the list's command to `pick()`, and its buttons' commands to `accept()` and
  `close()`.
- The listing is `..` (except at the root), then the directories, then the files that match
  the extension, each sorted by name.
- `pick()` on a directory steps into it, on `..` steps up, and on a file puts its name in the
  field.
- `accept()` refuses an empty name or one containing a separator. In `Open` mode it refuses a
  file that does not exist.
- In `Save` mode a bare name is given the extension. Saving over an existing file asks first:
  `accept()` returns false and `confirming()` is true, and accepting the same name again
  replaces the file.

## Testing a ui

`api/ui/tests/` needs no window, no GPU and no font. `Canvas` is CPU-side, and the text
callbacks can be stand-ins. A test builds components, draws them onto a canvas, and checks
the boxes the draw left or the primitives it added. Layout alone can be checked with no
canvas at all; see [internals/UserInterface.md](../internals/UserInterface.md).
`TextRenderer` needs a device to upload its atlas, so it is not covered there.
[Testing.md](../contributing/Testing.md) has the rest.
