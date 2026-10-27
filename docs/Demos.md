# The demos

This document is for someone working on the five demos in `demos/` (pong, tetris, voxel,
odyssey and diorama) or learning the api from them. Each is a small game or scene, and each is an
app like any other in the tree: it builds, is tested, and passes the analysers with the rest.
Each section says how to play the demo, what its data files are, and which api features it is
the best example of.

| Demo | What it shows |
|---|---|
| [Pong](#pong) | The shell's menu and statistics overlay, commands read while their key is held, rebinding a key, a canvas coordinate space, drawing between steps, and sound |
| [Tetris](#tetris) | The smallest complete game: an app command on the menu, a text asset, and textures loaded by name |
| [Voxel](#voxel) | `tick()` beside `simulate()`, mouse look, an app's own pipeline and shaders, timing a frame, and the immediate-mode ui |
| [Odyssey](#odyssey) | `api/grid`, regions loaded on a worker thread, an app's own document kind, an `ecs::System`, and the shell over an app that pauses itself |
| [Diorama](#diorama) | A lit scene of glTF models with a shadow map, the isometric camera controller, and a voice placed in the world |

The editor in `vertical3d/` has its own document, [editor/](editor/README.md). The offline renderer
`moya` is covered in [offline/](offline/README.md), and the image tool in
[Imagetool.md](Imagetool.md).

## Running a demo

Build the demo's target, then run the executable from the build tree, for example
`out/build/x64-Debug/demos/pong/pong.exe`. [contributing/Build.md](contributing/Build.md) covers
the build.

Every demo follows the same pattern:

- `main` is one call to `v3d::engine::run<T>(argv[0], identity)`, and the demo is a subclass of
  `v3d::engine::Engine`. The identity names the directory a player's files are kept in.
- Data is read from the `data/` directory beside the executable. The build copies the demo's
  own `data/` there, and the shared fonts from the repository's root `data/`.
- `data/config.json` lists the demo's documents by kind. Each kind is one a library reads or
  one of the demo's own.
- The log is where [api/engine/README.md](api/engine/README.md#logging) says.
- Keys are bound to commands in `data/mappings.json`. A command is a context and a name,
  written `context::name`.

[api/engine/](api/engine/README.md) explains the app lifecycle, the loop, config and bindings.

| Demo | Target | Window | Document kinds | Tests |
|---|---|---|---|---|
| Pong | `pong` | 800 × 600 | `binding`, `ui`, `window`, `sound` | `pong` suite: ball, paddle, game state, scene, bindings |
| Tetris | `tetris` | 800 × 600 | `binding`, `ui`, `window` | `tetris` suite: board, tetrad, bindings |
| Voxel | `voxel` | 800 × 600 | `binding`, `ui`, `window` | `voxel` suite: chunks, culling, mesh cache, Morton codes, camera, player, bindings |
| Odyssey | `odyssey` | 1280 × 768 | `binding`, `ui`, `window`, and the demo's own `world` | `odyssey` suite: map, world, bindings |
| Diorama | `diorama` | 1280 × 720 | `binding`, `sound`, `ui`, `window` | `diorama` suite: layout, models, sun, bindings |

No suite needs a window or a GPU. The renderers are not covered.

## Pong

Two paddles and a ball, for one player against the computer or two players at one keyboard.
Sources are in `demos/pong/src/`.

**Controls:**

| Key | Command | Effect |
|---|---|---|
| W, S | `pong::leftPaddleUp`, `pong::leftPaddleDown` | Move player 1's paddle while held |
| Up, Down | `pong::rightPaddleUp`, `pong::rightPaddleDown` | Move player 2's paddle while held (co-op only) |
| Escape | `ui::showGameMenu` | Show or hide the menu, which pauses the game |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |
| F3 | `pong::toggleStatistics` | Show or hide the frame statistics |
| F1 | `ui::fullscreen` | Switch between full screen and a window |

**Game modes.** The game starts in co-op mode, with both paddles controlled by players. The
menu offers Singleplayer and Co-op. In Singleplayer the right paddle is steered by the computer.
Choosing a mode resets the match.

**Returns.** Where the ball strikes a paddle sets the angle it goes back at: flat off the centre,
up to fifty degrees off either end. A paddle that is moving also turns the return the way it is
going. A return always leaves at the speed the ball arrived at, so no number of returns speeds
the ball up.

**The Options menu** sets the number of points that wins a match ("Rounds", 5 by default) and
lets the player rebind each of the four paddle keys. A key item captures the next key pressed.
Escape is never captured; it leaves the menu.

**Data files** (`demos/pong/data/`):

| File | Holds |
|---|---|
| `config.json` | The list of documents below |
| `mappings.json` | Key bindings |
| `vgui.json` | The game menu |
| `window.json` | The window size |
| `sounds.json` | The `hit`, `score` and `victory` clips and their `.wav` files |

**What pong is the reference for:**

- **Movement from held keys.** `PongEngine::simulate` asks `held()` for each paddle command every
  step and passes the answer to `PongScene::steer`. A key held through the menu or a change of
  mode moves its paddle as soon as play resumes, and one let go while the menu was up stops it.
  Outside co-op the computer steers the right paddle, and its keys do nothing.

- **Rebinding a key.** `PongEngine::rebindPaddleKey` calls `Engine::rebind()` with the key a
  menu item captured. The engine saves the change to the player's settings, and restores it the
  next time the game starts. A key the player never changed keeps following `mappings.json`.
  The settings are kept under the identity `main` passes to `run<T>`, so changing it loses every
  player's settings. [api/engine/Files.md](api/engine/Files.md) says what is saved.
- **Capturing a key.** `PongEngine::handleSource` listens on `sink<event::Source>`. While a menu
  item is capturing, it takes the key and calls `consume()`, so the key's bindings do not fire.
- **A canvas coordinate space.** The court is 800 × 600 units. `PongRenderer` calls
  `Canvas::space()` with `Fit::Contain`, so the court keeps its proportions at any window size,
  with bars at the sides.
- **Drawing between steps.** The constructor registers `Position2D` and `Position1D` with
  `Engine::interpolate<T>()`, so the loop keeps each one's value from the step before. The
  renderer draws the ball and paddles through `ecs::interpolated` with `Engine::alpha()`. See
  [api/ECS.md](api/ECS.md).
- **Sound.** The scene triggers `event::kind::Sound("hit")` on the dispatcher, and
  `audio::Player` plays the clip named in `sounds.json`.
- **The shell.** pong holds a `shell::Shell` with the menu and a statistics command. The menu
  in `vgui.json` pauses the game while it is up. See [api/engine/Shell.md](api/engine/Shell.md).

## Tetris

Falling tetrads on a board. Sources are in `demos/tetris/src/`.

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
| F1 | `ui::fullscreen` | Switch between full screen and a window |

A rotation that does not fit tries the same rotation shifted 1 and then 2 columns either way
before giving up. Play commands are ignored while the game is paused or over.

**Data files** (`demos/tetris/data/`):

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

A first-person walk over procedurally generated block terrain. Sources are in `demos/voxel/src/`.

**Controls:**

| Input | Command | Effect |
|---|---|---|
| W, A, S, D | `voxel::moveForward`, `moveLeft`, `moveBackward`, `moveRight` | Walk while held |
| Space, Left Ctrl | `voxel::moveUp`, `voxel::moveDown` | Rise or sink while held |
| Mouse | (no binding) | Look around |
| F3 | `voxel::debug` | Show or hide the debug window |
| Escape | `ui::showGameMenu` | Show or hide the menu (Resume, Quit), which pauses the game and frees the pointer |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |

**Data files.** `demos/voxel/data/` holds `config.json`, `mappings.json`, `vgui.json` and
`window.json`. The terrain shaders are `demos/voxel/shaders/voxel.vert` and `voxel.frag`. The build
compiles them with `v3d_add_shader` and embeds them in the executable.

**Building.** Voxel is the only target that needs libnoise, a git submodule built separately.
Its test suite links libnoise too. [contributing/Dependencies.md](contributing/Dependencies.md)
says how to build it.

**What voxel is the reference for:**

- **`tick()` versus `simulate()`.** Voxel overrides both. The world, including player movement,
  advances in `simulate(float step)`, which runs at a fixed step in seconds. Chunk remeshing
  stays in `tick(unsigned int delta)`: it has a budget of chunks per frame and is not
  simulation. [api/engine/](api/engine/README.md) explains the loop.
- **Mouse look.** `Window::relativeMouse(true)` hides the pointer and holds it in the window.
  `Controller::handleMotion` turns `MouseMotion::motion()`, the distance moved, into heading and
  pitch, at a tenth of a degree a pixel. Moving right turns the view right and moving down
  turns it down. The pitch stops at straight up and straight down, and the heading turns about
  the world's up axis, so the view never rolls. When the menu opens, the game leaves relative
  mode so the pointer shows; it returns to relative mode when the menu closes.
- **A controller over the api's camera.** The player sees through a `type::camera::Camera`.
  voxel's `CameraController` holds the heading, the pitch and the velocity, and writes the eye
  and the orientation into the camera after every change. The camera's profile uses the hand
  `glm::lookAt` builds, which the terrain is wound for. The lens keeps a 90 degree horizontal
  field of view at any aspect.
- **Movement from held keys.** `Controller::simulate` asks `held()` for each movement command
  every step and passes the answer to `Player::move`, which starts or stops that direction. A
  held direction accelerates at 8 units a second squared to 12 units a second, and a released
  one slows to rest at the same rate. A key let go while the menu was up therefore brings the
  player to rest once the menu closes.
- **An app's own pipeline.** The terrain is drawn with voxel's own shaders in a pass that depth
  tests and sorts items front to back. The text and menu are drawn in a second pass of quads,
  with no depth test, over the terrain.
- **Measuring a frame.** `Engine::measure("chunks")` times a named span. The debug window lists
  the spans, the GPU time of each pass, the player's position and the chunk counts. It is drawn
  with the immediate-mode layer `ui::Immediate`, which receives the cursor only while the menu
  is up.

## Odyssey

A tile-based game: a player in a world of walls and crates, with line of sight. The world is made
of regions, each a board of its own, loaded as the player approaches them. The game itself is not
written yet. Sources are in `demos/odyssey/src/`.

**Controls:**

| Input | Command | Effect |
|---|---|---|
| W, A, S, D | `odyssey::moveForward`, `moveLeft`, `moveBackward`, `moveRight` | Step one tile; also stops a walk in progress |
| Left click | `odyssey::use` | Walk to the clicked tile along the shortest path, across every loaded region |
| Escape | `ui::showGameMenu` | Show or hide the menu (Resume, Quit), which pauses the game |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |
| F3 | `odyssey::toggleStatistics` | Show or hide the frame statistics |
| F1 | `ui::fullscreen` | Switch between full screen and a window |

A walk moves one tile every 0.15 seconds. While the menu is up, no command moves the player, and
a walk in progress stands still until the menu closes.

**The world.** `data/world.json` lists the regions and names the one the player starts in:

```json
{
  "regions": [
    { "file": "map.json", "corner": { "x": 0, "y": 0 }, "size": { "x": 20, "y": 12 } },
    { "file": "east.json", "corner": { "x": 20, "y": 0 }, "size": { "x": 20, "y": 12 } }
  ],
  "start": 0
}
```

- Each region names its map, its corner in world tiles, and its size, which its map must match.
  Regions may not overlap, and every number must be a whole number.
- Positions are world tiles, and a tile is one world unit. Each region's grid is placed by its
  corner, as [api/Grid.md](api/Grid.md#regions) describes.
- The start region loads with the game. Another region is loaded on a worker thread once the
  player comes within 4 tiles of it, and released once the player is more than 10 tiles from it.
  A region whose map cannot be read is asked for again only after the player has gone away.
- A step from one region's edge onto the next region's first tile crosses between them, once the
  next one is loaded. A clicked route and the player's sight cross every loaded region, so the
  player walks and sees through the doorway into the next one. A region that is not loaded is a
  wall to both, and what falls out of sight is drawn from memory.
- The screen shows 20 × 12 tiles and scrolls to keep the player near its middle, inside the
  world's edges.

**The map format.** Each region's map, such as `data/map.json`, holds a `tiles` array of strings,
one string per row and one character per tile. All rows must be the same length.

| Character | Tile | Walkable | Blocks sight |
|---|---|---|---|
| `.` | Floor | Yes | No |
| `#` | Wall | No | Yes |
| `o` | Crate | No | No (sight passes over it) |
| `@` | Floor, and the player's start | Yes | No |

Any other character is refused. With no `@`, the player starts on the first walkable tile. A
tile is 64 × 64 pixels, so one 20 × 12 region fills the 1280 × 768 window.

**Data files** (`demos/odyssey/data/`):

| File | Holds |
|---|---|
| `config.json` | Lists `mappings.json`, `vgui.json`, `window.json` and `world.json`, the last under the game's own type `world` |
| `mappings.json`, `vgui.json`, `window.json` | Bindings, menu and window size |
| `world.json` | The regions, and where the player starts |
| `map.json`, `east.json` | The two regions' boards, joined by a doorway on row 5 |
| `sample.png` | The player sprite |
| `player.json` | Not read |

**Sight.** Each region's `grid::Sight` reaches 7 tiles, `World::SIGHT_RANGE`, measured as a
square (Chebyshev distance), and remembers every tile it has seen. Each region keeps its own,
and keeps it while the region is released. The renderer does not draw tiles never seen and dims
remembered ones that are out of sight.

**What odyssey is the reference for:**

- **`api/grid`.** Each region's map is parsed with `grid::fromPicture` into a `grid::TileGrid`
  placed by its corner. A click is routed with `grid::findPath` over a `grid::GridSet` of the
  loaded regions, and `grid::Sight` remembers what the player has seen. The tile kinds and the
  file formats are odyssey's own; the grid stores only passability and cover. See
  [api/Grid.md](api/Grid.md).
- **Regions loaded while the game runs.** `tile::World` reads a region on an `asset::Worker` and
  hands it over in `tick()`. See [api/Assets.md](api/Assets.md#loading-on-a-worker-thread).
- **An app's own document kind.** `config.json` files the world under the kind `world`, and
  the game reads it with `manifest()->get(tile::World::DOCUMENT)`.
- **`ecs::System`.** `system::Movement` derives from `v3d::ecs::System` and advances each
  entity's `Path` component in `simulate()`. The player's tile is a `grid::TileCoord` component.
  See [api/ECS.md](api/ECS.md).
- **The shell over a game that pauses itself.** odyssey holds a `shell::Shell` with the menu
  and a statistics command, and its renderer draws the world into the shell's screen before the
  menu and the overlay. The menu's suspend callback sets the engine's own pause flag, which the
  commands and `simulate()` read. See [api/engine/Shell.md](api/engine/Shell.md).

## Diorama

A small village seen from above: two houses, a ring of trees and a well on a square of grass,
under a sun that goes around the sky once every two minutes. There is no game. Sources are in
`demos/diorama/src/`.

**Controls:**

| Input | Command | Effect |
|---|---|---|
| W, A, S, D | `camera::pan_up`, `pan_left`, `pan_down`, `pan_right` | Pan the view while held |
| Q, E | `camera::rotate_left`, `camera::rotate_right` | Turn the view a quarter turn |
| R, F | `camera::zoom_in`, `camera::zoom_out` | Zoom while held |
| Mouse wheel | (no binding) | Zoom a step a notch |
| Escape | `ui::showGameMenu` | Show or hide the menu (Resume, Quit), which pauses the sun and the pan and zoom |
| Up, Down, Return | `ui::menuPrevious`, `ui::menuNext`, `ui::selectMenu` | Navigate the menu |
| F3 | `diorama::toggleStatistics` | Show or hide the frame statistics |
| F1 | `ui::fullscreen` | Switch between full screen and a window |

Q and E still turn the view while the menu is up, because the camera controller answers every
rotate command that reaches the dispatcher.

**Data files** (`demos/diorama/data/`):

| File | Holds |
|---|---|
| `config.json`, `mappings.json`, `vgui.json`, `window.json` | Config, bindings, menu, window size |
| `sounds.json` | The well's clip, under the id `well` |
| `models/ground.glb`, `house.glb`, `tree.glb`, `well.glb` | The four models, each one mesh with a material per colour and its origin at the centre of its base |
| `well.wav` | Four seconds of running water, which loops without a seam |

The models and the sound are generated, and `demos/diorama/assets/` holds what generates them.
Run either from the root of the tree, and commit what it writes:

```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python demos/diorama/assets/make_models.py
node demos/diorama/assets/make_sounds.ts
```

**What the diorama is the reference for:**

- **A lit scene.** `Renderer` builds a `LitScene` at the first frame, once the window's size is
  known, and resizes it when the window changes. Each frame it draws the scene into the window's
  colour pass first, because the grade covers the whole pass, and the menu and the statistics
  after it. See [api/rendering/Lighting.md](api/rendering/Lighting.md#a-lit-scene).
- **glTF models as entities.** `Layout.cxx` lists what stands where. Each placement's model is
  loaded once with `MeshRegistry::load()`, and each placement is an entity with an
  `ecs::component::Transform` and a `component::Mesh`. The nine trees draw one model, so they
  are drawn together. See [api/rendering/TexturesAndMeshes.md](api/rendering/TexturesAndMeshes.md).
- **A shadow map that is fitted once.** Nothing moves, so `lighting::fitShadow()` fits the map
  around the casters when the village is stood up. The ground casts nothing and stays out of the
  fit. Each frame builds the light's matrix from where the sun is with `lighting::shadowLight()`,
  so the shadows turn with it.
- **The isometric camera controller.** `engine::IsometricController` drives a
  `type::camera::Isometric` from the camera commands and the wheel. The camera commands have no
  handler of their own, so each is registered with `addHeld()`, which the bindings check needs.
  See [api/engine/Input.md](api/engine/Input.md#driving-an-isometric-camera).
- **A voice placed in the world.** The well's clip loops at the water, through `audio::Play`'s
  `position`. `tick()` sets the listener from the camera's profile, so the well moves between the
  ears as the view turns. `release()` calls `audio::Player::shutdown()`. See
  [api/engine/Audio.md](api/engine/Audio.md).
