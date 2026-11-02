# The user interface

These pages are for someone building a ui for an app with `api/ui` (`v3dlib_ui`). How the
library works inside is in [internals/ui/](../../internals/ui/README.md).

| Page | Read it to |
|---|---|
| This page | Choose between retained and immediate mode, and learn how a control sends a command |
| [Setup.md](Setup.md) | Build the renderers, load a ui document, and connect the keyboard |
| [DocumentsAndLayout.md](DocumentsAndLayout.md) | Write a ui document, and place components with the box model |
| [Components.md](Components.md) | Look up what each component does, disable one, or clip one |
| [Themes.md](Themes.md) | Style the ui from a theme, and supply the images a theme names |
| [Mouse.md](Mouse.md) | Know where a click goes, and when the app gets it instead |
| [Keyboard.md](Keyboard.md) | Handle focus, tab order, keys, typed text and text editing |
| [ImmediateMode.md](ImmediateMode.md) | Build a panel from live state every frame |
| [ShellComponents.md](ShellComponents.md) | Use the game menu, the statistics overlay, the file chooser and the prompt |

## Two ways to write a ui

`api/ui` offers two models. Both draw onto the same `realtime::canvas::Canvas`, use the same box
drawing and the same text callbacks, so they look like one ui on screen.

**Retained mode** is a tree of components. `v3d::ui::Interface` builds the tree once from a JSON
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
  behind the cursor (see [Immediate mode](ImmediateMode.md#immediate-mode)).

The two read different style classes from a theme: `ui` for retained mode and `tools` for
immediate mode. See [Themes and styles](Themes.md#themes-and-styles).

### Everything is drawn as quads on the app's canvas

The ui owns no device, pass or draw call. Panels, highlights and text are all quads appended
to a canvas the app is already filling. The app submits that canvas itself, through
`WindowRenderer::quads()`, so a ui adds no pass and no draw of its own. [rendering/](../rendering/README.md)
covers the canvas and the quad pipeline.

```
app's Canvas  <--  ComponentRenderer::draw(canvas, engine)   the retained tree
              <--  Immediate::begin(canvas, input) ... end()  immediate calls
              <--  TextRenderer::draw(canvas, ...)            a line of text anywhere
```

### Text is supplied by the app

Both models measure, wrap and draw their text through a `paint::TextRenderer`, which the app's
screen owns and passes to them. Each piece of text is laid out as a `font::text::Paragraph`.

- **`paint::Type`** ([`paint/Type.h`](../../../api/ui/paint/Type.h)) is a renderer at one size
  and in one weight. `width(text)` measures a string, `rows(text, width)` breaks it as a
  paragraph lays it out, and `draw(canvas, text, pen, colour)` draws it with its pen on the
  baseline. `in(weight)` and `scaled(factor)` give the same renderer in another weight or at
  another size. A type with no renderer measures nothing and draws nothing.
- **`ComponentRenderer(body)`** takes the `Type` the retained ui's text is set in.
  `emphasis(sets)` takes a `paint::Emphasis`: the `Setting` a centred menu's title is set in,
  with how large its band is against the body text, and the `Setting` of its chosen item. A
  setting with no type is set as the body text is.
- **`Immediate(text, size)`** takes the renderer and the size it draws at, and `text(text,
  size)` rescales it. `textWrapped()` breaks its rows through `TextRenderer::rows()`.

A text renderer needs no device, so `api/ui` stays testable without a window or a GPU. A test
reads back what was drawn through `TextRenderer::trace()`, which is told each line drawn with its
text, pen, colour, weight and size.

`input::Cursor` takes the same `Type`, to place a text box's caret where it is clicked.

`v3d::ui::paint::TextRenderer` holds one typeface in two weights. Every size and weight from one
`TextRenderer` shares one glyph store, and one store of colour glyphs.

The faces are in the shared data directory:

| Face | File | Used for |
|---|---|---|
| Regular | `fonts/Inter-Regular.ttf` | all text, unless a role asks for semibold |
| SemiBold | `fonts/Inter-SemiBold.ttf` | a role whose weight is semibold, such as a window's title |
| Fallback | `fonts/NotoSans-Regular.ttf` | a character the two Inter faces lack, in either weight |
| Arabic | `fonts/NotoSansArabic.ttf` | Arabic script, which Noto Sans lacks |
| Hebrew | `fonts/NotoSansHebrew.ttf` | Hebrew |
| Devanagari | `fonts/NotoSansDevanagari.ttf` | Devanagari |
| Thai | `fonts/NotoSansThai.ttf` | Thai |
| CJK | `fonts/NotoSansCJKsc-Regular.otf` | Chinese, Japanese kana and Korean Hangul, in every region's forms |
| Emoji | `fonts/NotoColorEmoji.ttf` | an emoji, in colour, from Noto Color Emoji's COLR version 1 build |

The faces are tried in the table's order. The four script faces are variable, so their weight
follows the text's. `TextRenderer::Faces` names other files, its `scripts` in the order they are
tried, and `scriptFonts()` gives the default list. A semibold face that cannot be loaded sets
semibold text in the regular face, and a fallback, script or emoji face that cannot be loaded
leaves the missing glyph in place. Each is logged. Each face's licence is beside it in `fonts/`.

A Han character is drawn in the forms of the text's language: the style's `language`, or the
renderer's shaping cache's locale when the style names none. With neither, it takes the CJK
face's Simplified Chinese forms.

Background: [ADR-0129](../../adr/0129-ui-inter-in-two-weights-noto-sans-as-fallback.md),
[ADR-0151](../../adr/0151-ui-noto-faces-for-each-script-in-the-tree.md)

Text is UTF-8, and `v3d::font` shapes it and lays it out as a `font::text::Paragraph`. Any character
the faces have is drawn, with no set of glyphs named up front: a glyph is rasterized the first
time it is drawn, at the size it is drawn at, into a `font::glyph::Store`. Its pen is kept to a
quarter of a pixel and each line's baseline is on a whole pixel. It is hinted vertically, so
text on the pixel grid is drawn at its exact size and sharp. A change of scale draws at the new
size. A `Screen` calls `beginFrame()` on its text renderer each frame, so a glyph unused since
the frame before may make room for another.

A glyph its face draws in colour, such as an emoji, is painted at its size into the store of
colour glyphs, as [Fonts.md](../Fonts.md#colour-glyphs) describes. It is painted in its face's
first palette, any layer in the text's colour taking the text's red, green and blue. It is
drawn as a picture in its own colours, faded by the text's alpha, and not corrected as text is.
How a face draws each glyph is read once and kept.

`width()` counts the space at the end of a string, so a caret placed after it lands past it.
`draw()` starts a new line below at a newline.

`draw(canvas, paragraph, pen)` draws a `font::text::Paragraph`, each glyph in its style's colour,
placed as a string is. Build the paragraph with `shaping()`, the renderer's cache, and a style from
`style(size, weight)`, which names the renderer's faces. A style's effects are drawn beneath the
text from each glyph's distance field, kept in the same glyph store, as
[Fonts.md](../Fonts.md#effect-fields) describes. Each glyph carries its style's contrast, so it
looks one weight in any target, as [Canvas.md](../rendering/Canvas.md#text-weight) describes.

A variable face's glyphs are rasterized, and their effects' fields made, at the coordinates
each run was shaped at, as [Fonts.md](../Fonts.md#variable-fonts) describes. A style's weight
and size set its weight and optical size axes, and its `variations` set any axis over them.

No `api/ui` header names a Vulkan type. The text renderer's glyph pages reach the device through
callbacks, as below. An app that draws the canvas through its own renderer supplies its own.

Background: [ADR-0145](../../adr/0145-text-coverage-on-the-pixel-grid-curves-when-scaled.md)

### Curve text

`paint::CurveText` draws a `font::text::Paragraph` from its glyphs' curves, sharp at any size and
under any transform: scaled, turned or animated. **`draw(canvas, paragraph, transform)`** places the
paragraph, in pixels with y downwards, through an affine `glm::mat3` into the canvas.

- Each glyph is a quad over its outline, a pixel of the canvas wider on every side so its edge
  is not cut. Its corners carry their place in the em square, and the quad shader finds each
  pixel's coverage from the curves that cross it.
- Each glyph is drawn in its style's colour, and the ellipsis of a cut line in the style before
  the cut.
- A glyph its face draws in colour is drawn from its layers' curves, through a program the
  curve store keeps, in the face's first palette. A layer in the text's colour takes the
  style's colour, and the style's alpha fades the glyph. A glyph whose colour is a picture,
  from a bitmap table, is painted at its size onto a page of four channels. It is drawn as a
  picture through the same transform, so it softens when it is scaled up.
- A style's effects are drawn beneath the text from each glyph's distance field, every glyph's
  effects before any glyph. The fields are kept in a `font::glyph::Store` of the curve text's own,
  whose pages reach the device through the `GlyphPages` given with the `CurvePages`, as are the
  pictures of colour glyphs. Call
  `beginFrame()` once a frame, so the store can make room for new fields.
- The curves come from a `font::glyph::CurveStore`, read the first time a glyph is drawn at any
  size. A variable face's glyph is read at its run's coordinates, and each point on its axes is kept
  apart.
  `paint::CurvePages` keeps the store's two arrays on the device as float textures, through three
  callbacks as `GlyphPages` does. With a context's `Textures`, create is
  `textures->texture({ .data, .width, .height })`, update is `textures->update()` of texels, and
  release is `textures->release()`.
- `CurvePages::sync()` makes an array's texture again, whole, when the array has outgrown it, and
  otherwise copies the rows the store changed. `draw()` syncs before it hands the quads to the
  canvas.

**Text in the world.** `draw(world, paragraph, transform)` stands the paragraph in a
`canvas::World`, and `draw(order, key, paragraph, transform)` adds it to a `canvas::DepthOrder`
with every glyph at one key. Each effect goes in at the same key, in a layer that is its place
among its glyph's effects, so the order draws it beneath the glyphs. The transform is a
`glm::mat4` from the paragraph's space, in pixels with y downwards on the plane z = 0, into the
world. A glyph there is wider than its outline by
`WORLD_MARGIN`, a sixteenth of an em on every side, because a screen pixel is not known there.
That is enough for text drawn larger than sixteen pixels to the em. To face the camera, build the
transform from the camera's right and the world's up.

Background: [ADR-0145](../../adr/0145-text-coverage-on-the-pixel-grid-curves-when-scaled.md)

### Glyph pages

`paint::GlyphPages` keeps the device's copy of a `font::glyph::Store`'s pages, one texture for
each, in the store's channels: one byte a pixel, or four for colour glyphs. A `TextRenderer`
copies the pages it is given, one for each of its stores. `sync(store)` is called once a frame,
after the frame's glyphs have been asked of the store and before the frame is submitted:

- A page the store added becomes a texture, made whole from its pixels.
- A plot the store changed is copied into its page's texture. Earlier frames may still draw the
  page, and draw what it held before.
- A page the store let go has its texture released.

`texture(page)` gives a page's texture, and an unset handle for a page not yet synced. Like
`Upload`, the device is reached through three callbacks, so the ui names no renderer. With a
context's `Textures`:

```cpp
GlyphPages pages(
    [textures](const unsigned char* pixels, uint32_t side, uint32_t channels) {
        return textures->texture(pixels, side, side, channels);
    },
    [textures](const TextureHandle& page, const glm::uvec2& at, const unsigned char* pixels, const glm::uvec2& size,
        uint32_t channels) {
        textures->update(page, at, { .data = pixels, .width = size.x, .height = size.y, .channels = channels });
    },
    [textures](const TextureHandle& page) { textures->release(page); });
```

## Commands

A control sends a command when it is activated. In the document, a command is the pair
`command` and `context`. Both must be present, or the component carries no command. The
`context` is resolved through the event router, and the command is sent on the ui's
dispatcher as a `v3d::event::Event`. An app answers it like any other command.

A command may also name `data`: a whole number, a flag or a string, which the event carries as
its data. It tells one sender from others that send the same command, such as which of several
headers a button is on. Data of any other kind is logged and ignored, and the command is kept.
An input menu item's value replaces its data.

What activates each control:

| Component | Mouse | Keyboard |
|---|---|---|
| `Button`, `CheckBox`, `RadioButton` | a press on it | `return` or `space` while focused |
| `SelectList` | a press on a row selects it, then sends | an arrow, `home` or `end` that changes the row selects it, then sends |
| `Slider` | a press or drag that changes the value | a key that changes the value |
| `TextBox` | never; a click places the caret | `return` only |
| `TabBar` | a press on a tab changes the page; nothing is sent | arrows change the page; nothing is sent |
| `Scrollbar` | a press jumps the thumb there and a drag follows; nothing is sent | see [The keyboard](Keyboard.md#the-keyboard); nothing is sent |
| toolbar button, menu item | a press on it | through the game menu's own commands |

The mouse and the keyboard both ask `ui::input::command()` which event a component carries,
so a control the mouse activates is always one the keyboard activates too. A command is sent
as the press lands, not on release.

## Testing a ui

`api/ui/tests/` needs no window and no GPU. `Canvas` is CPU-side, the text callbacks can be
stand-ins, and the shared font is copied beside the suite. A test builds components, draws them
onto a canvas, and checks the boxes the draw left or the primitives it added. Layout alone can
be checked with no canvas at all; see
[internals/ui/](../../internals/ui/README.md). `TextRenderer` is tested there
with a stand-in upload, which leaves only the upload itself uncovered.
[Testing.md](../../contributing/Testing.md) has the rest.
