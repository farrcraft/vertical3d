# The shell

`v3d::shell::Shell`, in [api/shell/Shell.h](../../../api/shell/Shell.h), builds what every app
builds around its own code. That is the ui and its document, the screen its text and ui are
drawn on, the pause menu, and the statistics overlay with the command that shows it. It is a
library of its own, `shell`, above `engine` and `ui`, which do not depend on each other.

## Using it

**An app holds a `Shell` as a member and makes three calls on it**, at three points in its life:

```cpp
// in the app's constructor: declare the commands, before the engine reads the bindings
AppEngine::AppEngine(const std::string& path) :
    v3d::engine::Engine(path),
    shell_(this, { .menu = true, .statistics = "app::toggleStatistics" }) {
}

// in start(): the ui, its document and the menu, then the renderer, which builds the screen
bool AppEngine::start() {
    const v3d::ui::shell::GameMenu::Suspend pause = [this](bool suspended) { scene_->pause(suspended); };
    if (!shell_.start(pause)) {
        return false;
    }
    renderer_ = boost::make_shared<AppRenderer>(window(), logger(), assets(), &shell_);
    return true;
}

// in the renderer's constructor, once its WindowRenderer is initialized
shell_->present(&engine_, options);

// in release(): the screen draws through the renderer, so it goes first
bool AppEngine::release() {
    shell_.release();
    renderer_->shutdown();
    return true;
}
```

- **The constructor declares the commands.** `Options::menu` declares the game menu's commands,
  and `Options::statistics` names the command that shows and hides the statistics overlay. A
  part not asked for declares nothing.
- **`start(suspend)` builds the ui** and loads the `ui` config document into it, with the
  themes of `Options::theme` beneath its own. With `Options::menu` it builds a `GameMenu` on the
  document's `game-menu` container, which calls `suspend` as it opens and closes. An app with no
  `ui` document has a ui with nothing in it but that theme. `start()` returns false when either
  document does not load.
- **`present(renderer, options)` builds the `Screen`** over the app's `WindowRenderer`. The options
  are the screen's own: the text size, the dressing, and whether it builds an overlay or an
  immediate layer. [ui/Setup.md](../ui/Setup.md#screen-the-renderers-and-the-canvas) covers
  them.
- **`release()` drops the screen, the menu and the ui.** It runs before the renderer is
  released, because the screen holds textures the renderer's device made.
- A shell holds the address of its own members in the commands it declares, so it cannot be
  copied or moved. An app holds it as a member, not through a pointer it replaces.

## The games' look

Every game is drawn in one designed shell, and names only its title and its accent.

- **The game theme** is `ui/game.json`, in the root `data/` directory that every game copies
  beside its own data. `Options::theme` names it by default, and the shell puts it beneath the
  game's own theme, as [ui/Themes.md](../ui/Themes.md#a-theme-beneath-another) describes. It
  sets the panel 90% opaque, rounder corners, a 2 pixel focus ring, a larger shadow and a
  menu that opens over 0.18 s. An app that keeps a theme of its own, as the editor does, sets
  `Options::theme` empty.
- **A game names its title and accent in its ui document**: the title as the game menu's
  `title`, and the accent in a `ui` style named `default`:

  ```json
  { "class": "ui", "name": "default",
    "colors": [ { "name": "accent", "value": [0.95, 0.55, 0.20, 1.0] } ] }
  ```

- **`Shell::game()` is the screen a game is drawn on**: body text 26 pixels high in a window
  1080 pixels high, scaled with the window's height. A game passes it to `present()`.
- **The game is paused while the menu is up**, through the `suspend` callback each game gives
  `start()`.

Background: [ADR-0130](../../adr/0130-shell-games-share-one-designed-shell-with-their-own-accent.md)

## Drawing

The app's renderer opens each frame through `screen()->begin(ui().get())`, draws its own
content, and then calls `draw()`, which draws the ui and the statistics over it into the
screen's canvas. `begin()` draws the frame in the ui's active theme. Where the canvas is
submitted, and in which pass, stays the renderer's.

`sample()` is what the overlay shows: the loop's frame times and steps, and a span for every
scope the app timed with `Engine::measure()`. `draw()` uses it. An app that adds spans of its
own, such as the device's pass times, adds them to `sample()` and passes the result to
`draw(sample)`.

| Accessor | Gives |
|---|---|
| `ui()` | The ui, from `start()` until `release()` |
| `menu()` | The game menu, or null without `Options::menu`. An app asks it whether the game is paused, and feeds a rebinding screen through `capture()` |
| `screen()` | The screen, from `present()` until `release()` |

## Leaving a part out

The games take the menu and a statistics command. voxel takes the menu and shows its statistics
in its own debug readout instead. The editor takes no game menu, because its menu is its menu
bar, and binds the statistics to `view::show::statistics`. An app that wants none of it does
not hold a shell.

Background: [ADR-0117](../../adr/0117-apps-the-game-shell-is-an-object-an-app-holds.md)
