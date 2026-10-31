# Setting up a ui

What an app builds once, in its `start()`, before it can draw or drive a ui.

## Getting set up

### Screen: the renderers and the canvas

An app built on `WindowRenderer` uses `v3d::ui::shell::Screen` and builds none of the pieces itself.
An app on the game engine has its `shell::Shell` build the screen, through
`present(renderer, options)`. [engine/Shell.md](../engine/Shell.md) covers it. A `Screen`
builds:

- the `TextRenderer`, uploading its atlas through the `WindowRenderer`
- the `ComponentRenderer` over that text
- a `StatisticsOverlay`, unless `Options::statistics` is false
- an `Immediate` layer, if `Options::immediate` is true
- the canvas all of these draw into

```cpp
v3d::ui::shell::Screen::Options options;
options.size = fontSize;          // text size at a scale of one
options.immediate = true;         // also build an Immediate layer
shell_->present(&engine_, options);   // or build a Screen directly, without the game engine
screen_ = shell_->screen();
```

Each frame:

```cpp
if (!screen_->begin(shell_->ui().get())) {
    return;                       // minimised: the frame was presented empty
}
if (screen_->resized()) {
    resize(screen_->canvas().width(), screen_->canvas().height());   // the app's own things
}
// ... the app draws its own content into screen_->canvas() if it likes ...
shell_->draw();                         // the retained ui, then the statistics over it
engine_.quads()->submit(screen_->canvas(), pass.get());
```

- `begin(ui)` dresses the component renderer and the immediate layer from the ui's active theme
  when it has changed, then begins the frame through `WindowRenderer::beginFrame`, sizes the
  canvas to the frame, and clears it. It returns false when the window is minimised or there is
  no renderer. A null ui keeps the theme drawn with. The theme is applied before anything draws,
  so a theme change shows in full on the next frame.
- `draw(ui, statistics)` draws the ui, then the statistics, then the tooltip of what the cursor
  rests on over both. See [Tooltips](Components.md#tooltips).
- `Options::shortcut` is how a command's shortcut is shown, as `"Ctrl+Z"`, in a dropped panel
  and a tooltip. `shell::Shell::present()` fills it when it is empty, with the command's first
  keyboard binding printed by `event::printed()`. Left empty without a shell, nothing shows a
  shortcut.
- `begin(ui)` also moves the ui's clock on by the real time since the last frame began, and
  hands it to the component renderer and the immediate layer. Transitions are measured on it.
  It follows real time rather than the simulation, so a menu eases open over a paused game. An
  app that draws a `ComponentRenderer` or an `Immediate` without a screen gives each a
  `ui::Clock` through `clock()`, and one never given a clock draws every change at once.
  `Screen::clock()` reads the clock, for anything else an app animates over the ui. It starts
  at zero.
- `shortcut(command)` gives the shortcut a menu shows for a command, through
  `Options::shortcut`, for an app that lists commands somewhere other than a menu.
- `resized()` is true on the first frame and on any frame whose size differs from the last.
  The app resizes its own resources then.
- The passes, the draw order and where the canvas is submitted stay the app's.
- `Options::dress` is a function `void(paint::Dressing*, float size)` that fills in the
  dressing (colours and metrics, see [Themes and styles](Themes.md#themes-and-styles)) for a text
  size. When it is empty, the line height is set to 1.4 times the size.
- `scale(factor)` draws the ui larger or smaller, as a user's preference does. It multiplies the
  scale the screen follows, described below.

### Scaling to the monitor and the window

A screen draws every size at `factor()`, which is `scale()` times what `Options::scaling`
follows:

| `Options::scaling` | Follows | For |
|---|---|---|
| `Scaling::Display`, the default | the monitor's scale: 1 at 100%, 1.5 at 150% | a tool, whose text should be the same physical size on any monitor |
| `Scaling::Height` | the window's height divided by `Options::reference`, which defaults to 1080 | a game, whose menu should fill the same share of its window at any size |

Before the first frame there is no window height, and a `Height` screen follows nothing.

- **Everything is multiplied.** The text is drawn at `Options::size` times the factor. The
  component renderer's and the immediate layer's dressings start from the built-in sizes times
  the factor, before `dress` is called with the scaled text size. A theme's sizes are logical
  pixels, and are multiplied by the factor too.
- **Outlines stay sharp.** An outline's width is multiplied, rounded down to whole device pixels,
  and never less than one, so a one pixel border is one pixel at 150% and two at 200%.
- **The screen rebuilds itself.** A change of factor rebuilds everything that depends on the size,
  calls `dress` again and applies the theme again. A `Height` screen rebuilds when `begin()` finds
  the window a new height. A `Display` screen rebuilds when it is given a new monitor scale through
  `display(scale)`.
- **The shell passes the monitor's scale on.** `Shell::present()` gives the screen the window's
  scale, and gives it the new one each time the engine publishes `event::kind::DisplayScale`.
  An app that builds a `Screen` itself calls `display()` the same way.
- **The cursor is not scaled.** Mouse points, the canvas and every box are device pixels, so hit
  testing is the same at every scale.

Background: [ADR-0131](../../adr/0131-ui-tools-scale-with-the-monitor-games-with-the-window.md)

Do not keep a reference to `components()` across frames. A rescale replaces it, and an old
reference points at an object that is no longer drawn. `immediate()` and `text()` are the same
objects across a rescale, given the new size, so the immediate layer keeps where its windows
were dragged, what is folded and how far each list is scrolled.

An app that needs a piece `Screen` does not build writes it beside the screen. An app that
does not use `WindowRenderer` builds `TextRenderer` itself with its own `Upload`.

### Loading the document

An app on the game engine has its shell build the ui and load the `ui` config document into it,
in `Shell::start()`. Without the game engine, build and load it directly:

```cpp
vgui_ = boost::make_shared<v3d::ui::Interface>(events, dispatcher, logger);
if (!vgui_->load(document)) {
    return false;
}
vgui_->resolveImages(resolver);   // once the renderer exists; see Images
```

`Interface::load()` reads the themes and containers. It returns false and logs the reason when
the document is malformed. `container(name)` returns a loaded container, and
`Container::get(name)` finds a component anywhere inside it.

An app that builds part of its ui as it runs adds containers to a loaded ui:

- `Interface::add(document, before)` reads a document holding only `containers`, and puts them
  before the container named `before`, or after every container when `before` is empty or not
  held. Containers draw in the order they are held, so what is added before a container draws
  under it. They draw in the active theme. A document that does not read, or that names a
  container already held, adds nothing. Resolve the images of what was added with
  `resolveComponentImages()`.
- `Interface::remove(name)` takes a container out. A component in it that held the focus or the
  cursor's rest stops holding it.

### The keyboard adapter

`v3d::ui::shell::Keyboard` connects the ui to SDL keyboard events. Build one and pass it every
event from the app's `onEvent()`:

```cpp
uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui_, dispatcher(), window());

bool App::onEvent(const SDL_Event& event) {
    return uiKeys_->event(event);
}
```

`onEvent()` runs before the input bindings ([engine/](../engine/README.md) covers the event order).
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
  leaves. It follows `Interface::onFocus()`, so a box clicked and typed into in the same frame
  does not lose its first character.
- `immediate(layer)` offers an immediate layer the keys and characters the retained ui did
  not take, and gives it the SDL clipboard. Text input is then also on while the layer's
  `textField()` has the keyboard, which the adapter follows through `Immediate::onTyping()`.
  The adapter can be built with no `ui::Interface`, for an app that draws only through the
  immediate layer. [ImmediateMode.md](ImmediateMode.md#typing-into-a-field) covers the field.

Rules the adapter follows:

- Only a key going down is ever taken. A key release always passes through, so
  `input::KeyState` never holds a key down that the ui took.
- A key that `api/input` has no name for is never taken.
- The key name comes from `input::keyName()`, the same table the bindings use.
- `Interface::onFocus()` holds one listener, and the last caller wins. The adapter sets it. An
  app that wants its own listener sets it after building the adapter and clears it before
  the adapter is destroyed.
- Only a component whose type takes typed text (a `TextBox`) turns text input on, or an
  immediate layer's field that has the keyboard.
- The retained ui is offered a key first. An immediate layer sees only what it did not take.
- The layer must outlive the adapter, or be taken off it with `immediate(nullptr)` first,
  because the adapter holds a callback on the layer and clears it when it is destroyed.
- `Immediate::onTyping()` holds one listener, as `Interface::onFocus()` does, and the adapter sets
  it.

The mouse is not routed by the adapter. See [The mouse](Mouse.md#the-mouse).
