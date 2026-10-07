# Setting up a ui

What an app builds once, in its `start()`, before it can draw or drive a ui.

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
  dressing (colours and metrics, see [Themes and styles](Themes.md#themes-and-styles)) for a text
  size. When it is empty, the line height is set to 1.4 times the size.
- `scale(factor)` redraws text larger or smaller. It rebuilds everything that depends on the
  font size, calls `dress` again, and applies the theme again.
- `theme(theme)` dresses the component renderer and the immediate layer from a theme, and
  keeps it for later rebuilds.

Do not keep a reference to `components()` across frames. `scale()` replaces it, and an old
reference points at an object that is no longer drawn. `immediate()` and `text()` are the same
objects across a rescale, given the new size, so the immediate layer keeps where its windows
were dragged, what is folded and how far each list is scrolled.

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

The mouse is not routed by the adapter. See [The mouse](Mouse.md#the-mouse).
