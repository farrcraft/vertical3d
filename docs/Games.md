# The games and tools

This document is for someone working on the four games (pong, tetris, voxel and odyssey) or
learning the api from them. It also covers the two small command-line programs, imagetool and
v3dshell. Each section says how to run the program, what its controls and data files are, and
which api features it is the best example of.

- [Running a game](#running-a-game)
- [Pong](#pong)
- [Tetris](#tetris)
- [Voxel](#voxel)
- [Odyssey](#odyssey)
- [imagetool](#imagetool)
- [v3dshell](#v3dshell)

The editor in `vertical3d/` has its own document, [Editor.md](Editor.md). The offline renderer
`moya` is covered in [OfflineRenderer.md](OfflineRenderer.md).

## Running a game

Build the game's target, then run the executable from the build tree, for example
`out/build/x64-Debug/pong/pong.exe`. [contributing/Build.md](contributing/Build.md) covers the
build.

Every game follows the same pattern:

- `main` is one call to `v3d::engine::run<T>(argv[0], name)`, and the game is a subclass of
  `v3d::engine::Engine`.
- Data is read from the `data/` directory beside the executable. The build copies the game's
  own `data/` there, and the shared fonts from the repository's root `data/`.
- `data/config.json` lists the game's config documents by type. Each `type` is one of the
  engine's config types or a name of the game's own.
- The log is `v3d.log` beside the executable.
- Keys are bound to commands in `data/mappings.json`. A command is a context and a name,
  written `context::name`.

[api/Engine.md](api/Engine.md) explains the app lifecycle, the loop, config and bindings.

| Game | Target | Window | Config types | Tests |
|---|---|---|---|---|
| Pong | `pong` | 800 × 600 | `binding`, `ui`, `window`, `sound` | `pong` suite: ball, paddle, game state, scene |
| Tetris | `tetris` | 800 × 600 | `binding`, `ui`, `window` | `tetris` suite: board, tetrad |
| Voxel | `voxel` | 800 × 600 | `binding`, `ui`, `window` | `voxel` suite: chunks, culling, mesh cache, Morton codes |
| Odyssey | `Odyssey` | 1280 × 768 | `binding`, `window`, and the game's own `map` | `odyssey` suite: map, sight |

No suite needs a window or a GPU. The renderers are not covered.

## Pong

Two paddles and a ball, for one player against the computer or two players at one keyboard.
Sources are in `pong/src/`.

**Controls:**

| Key | Command | Effect |
|---|---|---|
| W, S | `pong::leftPaddleUp`, `pong::leftPaddleDown` | Move player 1's paddle while held |
| Up, Down | `pong::rightPaddleUp`, `pong::rightPaddleDown` | Move player 2's paddle while held (co-op only) |
| Escape | `pong::showGameMenu` | Show or hide the menu, which pauses the game |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |
| F3 | `pong::toggleStatistics` | Show or hide the frame statistics |

F1 is bound to `pong::toggleFS`, which nothing handles.

**Game modes.** The game starts in co-op mode, with both paddles controlled by players. The
menu offers Singleplayer, Co-op and Multiplayer. In Singleplayer the right paddle is steered by
the computer. Multiplayer currently behaves the same as Singleplayer. Choosing a mode resets the
match.

**The Options menu** sets the number of points that wins a match ("Rounds", 5 by default) and
lets the player rebind each of the four paddle keys. A key item captures the next key pressed.
Escape is never captured; it leaves the menu.

**Data files** (`pong/data/`):

| File | Holds |
|---|---|
| `config.json` | The list of documents below |
| `mappings.json` | Key bindings |
| `vgui.json` | The game menu |
| `window.json` | The window size |
| `sounds.json` | The `hit`, `score` and `victory` clips and their `.wav` files |

**What pong is the reference for:**

- **Settings and rebinding.** `PongEngine::rebindPaddleKey` calls `Engine::rebind()` with the
  key a menu item captured. It then stores the key in an `engine::Settings` document and saves
  it at once. `applyStoredBindings()` reapplies stored keys at startup; a key the player never
  changed keeps following `mappings.json`. The settings live in `settings.json` under
  `engine::userPath("Vertical3D", "Pong")`. Do not change that pair: it is the directory, and a
  new pair loses every player's settings.
- **Capturing a key.** `PongEngine::handleSource` listens on `sink<event::Source>`. While a menu
  item is capturing, it takes the key and calls `consume()`, so the key's bindings do not fire.
- **A canvas coordinate space.** The court is 800 × 600 units. `PongRenderer` calls
  `Canvas::space()` with `Fit::Contain`, so the court keeps its proportions at any window size,
  with bars at the sides.
- **Interpolation.** `simulate()` snapshots `Position2D` and `Position1D` into
  `ecs::Previous<T>` before each step. The renderer draws the ball and paddles through
  `ecs::interpolated` with `Engine::alpha()`. See [api/ECS.md](api/ECS.md).
- **Sound.** The scene triggers `event::kind::Sound("hit")` on the dispatcher, and
  `audio::Engine` plays the clip named in `sounds.json`.
- **The game menu.** `ui::shell::GameMenu` drives the menu in `vgui.json`, and its callback
  pauses the game while the menu is up.

## Tetris

Falling tetrads on a board. Sources are in `tetris/src/`.

**Controls:**

| Key | Command | Effect |
|---|---|---|
| A, D | `tetris::movePieceLeft`, `tetris::movePieceRight` | Move the piece one column |
| W, S | `tetris::rotatePieceCW`, `tetris::rotatePieceCCW` | Rotate the piece |
| Space | `tetris::dropPiece` | Toggle fast falling for the current piece |
| Escape | `ui::showGameMenu` | Show or hide the menu (New Game, Quit), which pauses the game |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |
| F2 | `tetris::debugMode` | Show the current piece's position and size |
| F3 | `tetris::toggleStatistics` | Show or hide the frame statistics |

F1 is bound to `tetris::toggleFS`, which nothing handles.

A rotation that does not fit tries the same rotation shifted 1 and then 2 columns either way
before giving up. Play commands are ignored while the game is paused or over.

**Data files** (`tetris/data/`):

| File | Holds |
|---|---|
| `config.json`, `mappings.json`, `vgui.json`, `window.json` | Config, bindings, menu, window size |
| `pieces/shapes.txt` | The seven tetrads: four rows of `0` and `1` for each, followed by a colour name |
| `pieces/<colour>.tga` | The block texture for each colour |
| `pieces/pieces.psd` | The source artwork for the textures |

**What tetris is the reference for:** the smallest complete game. It shows a `GameMenu` with
an app command (`ui::newGame`), a text file loaded through the asset manager
(`v3d::asset::kind::Text`), and textures loaded by name. The falling piece advances in
`simulate()`; moves and rotations are applied when their commands arrive.

## Voxel

A first-person walk over procedurally generated block terrain. Sources are in `voxel/src/`.

**Controls:**

| Input | Command | Effect |
|---|---|---|
| W, A, S, D | `voxel::moveForward`, `moveLeft`, `moveBackward`, `moveRight` | Walk while held |
| Space, Left Ctrl | `voxel::moveUp`, `voxel::moveDown` | Rise or sink while held |
| Mouse | (no binding) | Look around |
| F3 | `voxel::debug` | Show or hide the debug window |
| Escape | `ui::showGameMenu` | Show or hide the menu (Resume, Quit), which pauses the game and frees the pointer |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |

**Data files.** `voxel/data/` holds `config.json`, `mappings.json`, `vgui.json` and
`window.json`. The terrain shaders are `voxel/shaders/voxel.vert` and `voxel.frag`. The build
compiles them with `v3d_add_shader` and embeds them in the executable.

**Building.** Voxel is the only target that needs libnoise, a git submodule built separately.
Its test suite links libnoise too. [contributing/Dependencies.md](contributing/Dependencies.md)
says how to build it.

**What voxel is the reference for:**

- **`tick()` versus `simulate()`.** Voxel overrides both. The world, including player movement,
  advances in `simulate(float step)`, which runs at a fixed step in seconds. Chunk remeshing
  stays in `tick(unsigned int delta)`: it has a budget of chunks per frame and is not
  simulation. [api/Engine.md](api/Engine.md) explains the loop.
- **Mouse look.** `Window::relativeMouse(true)` hides the pointer and holds it in the window.
  `Controller::handleMotion` turns `MouseMotion::motion()`, the distance moved, into heading and
  pitch. When the menu opens, the game leaves relative mode so the pointer shows; it returns to
  relative mode when the menu closes.
- **Movement from held keys.** The movement bindings name no state, so each key sends a command
  on press and on release. `Player::move` toggles that direction on each one.
- **An app's own pipeline.** The terrain is drawn with voxel's own shaders in a pass that depth
  tests and sorts items front to back. The text and menu are drawn in a second pass of quads,
  with no depth test, over the terrain.
- **Measuring a frame.** `Engine::measure("chunks")` times a named span. The debug window lists
  the spans, the GPU time of each pass, the player's position and the chunk counts. It is drawn
  with the immediate-mode layer `ui::Immediate`, which receives the cursor only while the menu
  is up.

## Odyssey

A tile-based game: a player on a board of walls and crates, with line of sight. The game itself
is not written yet. Sources are in `odyssey/`.

**Controls:**

| Input | Command | Effect |
|---|---|---|
| W, A, S, D | `odyssey::moveForward`, `moveLeft`, `moveBackward`, `moveRight` | Step one tile; also stops a walk in progress |
| Left click | `odyssey::use` | Walk to the clicked tile along the shortest path |
| Escape | `ui::quit` | Quit |

Space is bound to `odyssey::moveUp`, which nothing handles. A walk moves one tile every 0.15
seconds.

**The map format.** `data/map.json` holds a `tiles` array of strings, one string per row and one
character per tile. All rows must be the same length.

| Character | Tile | Walkable | Blocks sight |
|---|---|---|---|
| `.` | Floor | Yes | No |
| `#` | Wall | No | Yes |
| `o` | Crate | No | No (sight passes over it) |
| `@` | Floor, and the player's start | Yes | No |

Any other character is refused. With no `@`, the player starts on the first walkable tile. A
tile is 64 × 64 pixels, so the shipped 20 × 12 board fills the 1280 × 768 window.

**Data files** (`odyssey/data/`):

| File | Holds |
|---|---|
| `config.json` | Lists `mappings.json`, `window.json` and `map.json`, the last under the game's own type `map` |
| `mappings.json`, `window.json` | Bindings and window size |
| `map.json` | The board |
| `sample.png` | The player sprite |
| `player.json` | Not read |

**Sight.** `tile::Sight` reaches 7 tiles, measured as a square (Chebyshev distance), and
remembers every tile it has seen. The renderer does not draw tiles never seen and dims
remembered ones that are out of sight.

**What odyssey is the reference for:**

- **`api/grid`.** The map is parsed with `grid::fromPicture` into a `grid::TileGrid`. A click is
  routed with `grid::findPath`, and `tile::Sight` asks `grid::hasLineOfSight` what is visible.
  The tile kinds and the map format are odyssey's own; the grid stores only passability and
  cover. See [api/Grid.md](api/Grid.md).
- **An app's own config document.** `config.json` files the map under the type `map`, and the
  game reads it with `config()->get("map")`.
- **`ecs::System`.** `system::Movement` derives from `v3d::ecs::System` and advances each
  entity's `Path` component in `simulate()`. The player's tile is a `grid::TileCoord` component.
  See [api/ECS.md](api/ECS.md).

## imagetool

A command-line tool over `api/image`. It reads an image, can report its size, can cut out a
rectangle, and can write the result in another format.

```
imagetool --file in.png --info
imagetool --file sheet.png --crop 64,0,32,32 --outfile sprite.tga
imagetool --file photo.jpg --outfile photo.bmp
```

| Option | Effect |
|---|---|
| `--file <path>` | The image to read. Without it, the tool prints the help |
| `--info` | Print the width, height and bits per pixel of the image read |
| `--outfile <path>` | Write the image to this file |
| `--crop x,y,width,height` | Cut out this rectangle before writing. Requires `--outfile`, four non-negative whole numbers, and a rectangle inside the image |
| `--silent` | Do not print the progress lines. `--info` still prints |
| `--help` | Print the options |

The format is chosen by the file extension, for reading and writing alike: `png`, `jpg` or
`jpeg`, `tga` and `bmp`. Relative paths resolve against the working directory. The tool exits
with a failure status and a message on any error.

## v3dshell

An empty program: `main` returns success and does nothing. It is a placeholder for a future
command shell for Vertical3D.
