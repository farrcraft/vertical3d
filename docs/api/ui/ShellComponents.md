# Game menu, statistics overlay, file chooser and prompt

Four ready-made pieces of ui in `ui::shell` that games and the editor share.

## GameMenu, StatisticsOverlay, FileChooser and Prompt

These classes in `v3d::ui::shell` are the parts of a game's shell that every game would
otherwise write the same way. An app that writes its own version of one has diverged from
the api rather than customised it.

### GameMenu

`GameMenu` is the pause menu a game puts up over itself. An app on the game engine has its
`shell::Shell` build it with `Options::menu`, as [engine/Shell.md](../engine/Shell.md) shows. The
shell calls `declare()` in its constructor and builds the menu in `start()`:

```cpp
// in a constructor
v3d::ui::shell::GameMenu::declare(commands, menu_);

// once the ui is loaded
menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(ui, [this](bool suspended) {
    scene_->pause(suspended);
});
```

- It shows and hides a container, `game-menu` by default, and drives the `Menu` inside it,
  `main-menu` by default. Both names can be passed to the constructor.
- `GameMenu::declare(commands, menu)` registers its commands in the `ui` context:
  `showGameMenu` toggles it, and `menuPrevious`, `menuNext` and `selectMenu` navigate it
  while it is up. An app calls it from its constructor, before the menu exists, and passes the
  member it will hold the menu in. Each command does nothing while that member is empty. An app
  binds these in its mappings, for example `escape` to `showGameMenu`. These command names
  cannot be changed.
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
- `device(name)` sets a top line naming the GPU. `Screen` sets it from
  `Device::description()` when it builds the overlay.
- `lines(sample)` returns the frame's lines without drawing them, and `content(sample)` returns
  every line `draw()` would draw.

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
- `accept()` refuses a name that is empty, `.` or `..`, or that contains a separator or a
  colon. A colon is refused on every platform: on Windows it makes a drive relative path or an
  alternate data stream. A name must also be its own file name, so no other form leaves the
  directory. In `Open` mode `accept()` refuses a file that does not exist.
- In `Save` mode a bare name is given the extension. Saving over an existing file asks first:
  `accept()` returns false and `confirming()` is true. Listing another directory clears the
  question. Accepting the same name again in the same directory replaces the file.

### Prompt

`Prompt` asks one question with up to three answers, such as "Save changes?" with Save,
Discard and Cancel. Like the chooser, it adds no component type and drives components the
document names (defaults in `Prompt::Names`):

| Name | Component | Holds |
|---|---|---|
| `prompt` | a container | shown and hidden |
| `question` | a `label` | the question |
| `answer0`, `answer1`, `answer2` | `button`s | one answer each, in order |

```cpp
prompt_->open("Save changes?", {
    { "Save", [this]() { save(); } },
    { "Discard", [this]() { discard(); } },
    { "Cancel", {} }
}, {});
```

- The app routes each button's command to `answer(place)`, and anything that dismisses the
  prompt to `cancel()`.
- Each button is labelled with its answer. A button with no answer is hidden. Answers past
  `Prompt::most` are not offered.
- The prompt closes before a handler runs, so a handler can open another prompt.
- `cancel()` runs the handler given to `open()` for a dismissal. Opening a prompt while one is
  up cancels the first.
- An answer or a cancel on a prompt that is not up does nothing.
