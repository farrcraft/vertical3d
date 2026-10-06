# Game menu, statistics overlay and file chooser

Three ready-made pieces of ui in `ui::shell` that games and the editor share.

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
  field. A new listing has no row chosen, so a `pick()` with no row named does nothing until one
  is chosen in it.
- `accept()` refuses an empty name or one containing a separator. In `Open` mode it refuses a
  file that does not exist.
- In `Save` mode a bare name is given the extension. Saving over an existing file asks first:
  `accept()` returns false and `confirming()` is true, and accepting the same name again
  replaces the file.
