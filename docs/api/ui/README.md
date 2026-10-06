# The user interface

These pages are for someone building a ui for an app with `api/ui` (`v3dlib_ui`). How the
library works inside is in [internals/UserInterface.md](../../internals/UserInterface.md).

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
| [ShellComponents.md](ShellComponents.md) | Use the game menu, the statistics overlay and the file chooser |

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
  behind the cursor (see [Immediate mode](ImmediateMode.md#immediate-mode)).

The two read different style classes from a theme: `ui` for retained mode and `tools` for
immediate mode. See [Themes and styles](Themes.md#themes-and-styles).

### Everything is drawn as quads on the app's canvas

The ui owns no device, pass or draw call. Panels, highlights and text are all quads appended
to a canvas the app is already filling. The app submits that canvas itself, through
`Engine3D::quads()`, so a ui adds no pass and no draw of its own. [rendering/](../rendering/README.md)
covers the canvas and the quad pipeline.

```
app's Canvas  <--  ComponentRenderer::draw(canvas, engine)   the retained tree
              <--  Immediate::begin(canvas, input) ... end()  immediate calls
              <--  TextRenderer::draw(canvas, ...)            a line of text anywhere
```

### Text is supplied by the app

Both models take text through two callbacks declared in [`paint/Text.h`](../../../api/ui/paint/Text.h):

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

The glyphs a `TextRenderer` can draw are the charcodes given to its constructor, which default
to printable ascii. They are packed and uploaded once, and the atlas is not added to afterwards.
Text is UTF-8 and is decoded to code points. A code point outside the packed set measures as
zero width and draws nothing. An app that shows other characters passes them as `charcodes`, and
may need a larger atlas to hold them.

`TextRenderer` uploads its atlas through an `Upload` callback rather than a Vulkan type. No
`api/ui` header names a Vulkan type. An app that draws the canvas through its own renderer
can supply its own `Upload`.

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
| `Scrollbar` | a press jumps the thumb there and a drag follows; nothing is sent | see [The keyboard](Keyboard.md#the-keyboard); nothing is sent |
| toolbar button, menu item | a press on it | through the game menu's own commands |

The mouse and the keyboard both ask `ui::input::command()` which event a component carries,
so a control the mouse activates is always one the keyboard activates too. A command is sent
as the press lands, not on release.

## Testing a ui

`api/ui/tests/` needs no window and no GPU. `Canvas` is CPU-side, the text callbacks can be
stand-ins, and the shared font is copied beside the suite. A test builds components, draws them onto a canvas, and checks
the boxes the draw left or the primitives it added. Layout alone can be checked with no
canvas at all; see [internals/UserInterface.md](../../internals/UserInterface.md).
`TextRenderer` is tested there with a stand-in upload, which leaves only the upload itself
uncovered.
[Testing.md](../../contributing/Testing.md) has the rest.
