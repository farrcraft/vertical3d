# UI Polish — A Designed Look for `api/ui`, a Tiled Editor and Finished Game Menus

Drafted 2026-10-30 against `b1087f68`. This plan's steps fall in `api/ui`, `api/shell`,
`api/window`, `api/event`, `vertical3d` and the five demos. When it closes, `api/ui` draws from a
theme derived from a few colours, in Inter at two weights, scaled to the display, with
shadows, transitions and tooltips. The editor's window is a tiling of areas that hold editors as
tabs, with clear borders, resizable and maximisable views, a status bar and a command search.
Every game opens on a titled menu over a dimmed backdrop, in its own accent colour.

`api/ui` is described in [api/ui/](../api/ui/README.md) and [internals/ui/](../internals/ui/README.md),
and the editor in [editor/](../editor/README.md).

A step with a testable CPU half brings test cases with it. A step that changes how something
looks is checked by running the app and reading a screenshot, as
[Testing.md](../contributing/Testing.md) describes.

## What the survey found

The tree was read at `b1087f68`, and each app was run and photographed at its default window
size.

**The look:**

* **No game names a style.** All five `vgui.json` files declare a theme with no styles, so every
  game draws `paint::Dressing`'s defaults (`api/ui/paint/Dressing.cpp:13-108`). The defaults
  were chosen for the editor. The retained radius is 0, and menus ignore `radius` altogether
  (`api/ui/paint/ComponentRenderer.cpp:819`, `:938`).
* **Text sizes run from 18 to 48 pixels.** voxel draws at 18, tetris at 22, pong at 28 and the
  editor at 15 times its scale. odyssey and diorama set no size, so their menus draw at the
  atlas's base size of 48 (`api/ui/paint/TextRenderer.cpp:37`).
* **There is one face.** `TextRenderer` packs `NotoSans-Regular.ttf` alone (`TextRenderer.cpp:32`),
  and a theme has no font or weight property.
* **States are partial.** A flat button has no pressed colour, and only skinned buttons take a
  per-state style. Nothing animates, nothing casts a shadow, and nothing dims what is behind a
  menu. `Immediate::itemHovered()` exists for a tooltip, and no tooltip is drawn.
* **Menus have no shortcuts and no icons.** A menu item has a label and a type, and nothing else
  (`api/ui/component/menu/Item.h`).
* **Several things draw outside the theme.** `StatisticsOverlay` hard-codes its colours
  (`api/ui/shell/StatisticsOverlay.cpp:40-41`). tetris draws its side panel and well border with
  its own colours (`demos/tetris/src/Renderer.cxx:199-294`), and pong draws its scores the same
  way (`demos/pong/src/PongRenderer.cxx:30-34`, `:135`).
* **Nothing reads the display's scale.** No code in `api/window`, `api/engine` or `api/ui` asks
  SDL for a display scale or pixel density.

**The editor:**

* **The views run together.** The three orthographic views share one background and one grid,
  and no line or gap separates them. Nothing names a view or marks the active one.
* **Dividers can be dragged but are not drawn.** `ViewLayout::grab` takes a press within 3 pixels
  of a divider (`vertical3d/src/view/ViewLayout.h:113-122`), and no cursor or highlight shows
  where one is.
* **The layout holds viewports only.** `data/layout.json` is a tree of splits with a camera at
  each leaf, and shares last only while the editor runs.
* **The chrome is a menu bar and two toolbars.** The select mask is a text toolbar along the top,
  and the tools are four 16-pixel icons down the left. There is no status bar.
* **The bindings cannot express a chord**, so undo is Z and redo is Y
  ([editor/README.md](../editor/README.md#running-it)).

**The games:**

* **The game menu is a bare list.** It is centred, with no title, and the game is not dimmed
  behind it.
* **pong's settings show no values.** The Options menu lists Rounds and four key bindings with
  nothing beside them.
* **`demos/pong/data/fonts.json` is not read by anything**, and names a font the tree does not
  hold.

## The design

How established tools and games solve each part was researched before this plan was drafted:
Blender, Unreal, Godot, Houdini, Maya, Figma, Linear and JetBrains for the editor, and the Xbox
accessibility guidelines for the games. The numbers below are the targets each step works to.

**Colour.** One neutral scale and one accent scale, from the darkest step up:

| Role | Value |
|---|---|
| Gap between areas | `#111214` |
| Bars (top bar, status bar, tab strip) | `#17181b` |
| Area | `#1d1f23` |
| Area header | `#25282d`, at 80% opacity over a viewport |
| Field | `#2a2d32` |
| Control | `#2e3137` |
| Hover | `#383b42` |
| Hairline | white at 7% |
| Text, secondary text, disabled text | `#e4e6ea`, `#a3a7b1`, `#858b94` |
| Accent | `#4d8ce6` |
| Selected geometry in a view | `#f2a33a` |
| Axes x, y, z | `#d9595f`, `#72b052`, `#5590e8` |

Text is at least 4.5:1 against its surface, and the edge of a control at least 3:1.

**Type and spacing.** Inter at 12 pixels for body text and 11 for secondary text, with SemiBold
for titles and the chosen item. Spacing is on a 4-pixel grid. Rows are 22 to 26 pixels, control
corners 4 pixels and menu corners 6. Toolbar icons are 18 pixels on 30-pixel buttons.

**Motion.** A tooltip shows after 500 ms, or after 100 ms when the cursor moves on from another.
A hover change takes 100 ms, a menu opens in 120 ms, and a submenu opens after 200 ms of hover.
A notice stays for 2.5 s. Resizing and maximising are not animated.

**The editor's frame:**

- Areas are separated by a 3-pixel gap, with 4-pixel corners and a hairline inside each. The
  active area's hairline is the accent.
- A divider can be taken 4 pixels either side of its gap. Under the cursor it turns the accent
  and the cursor becomes a resize cursor. Escape or a right click cancels a drag, Ctrl snaps to
  tenths, and a double click divides evenly. An area keeps at least its header's height plus 40
  pixels.
- An area's header is 28 pixels: the editor-type menu, the editor's own controls, and a
  maximise button. A viewport's header is drawn over the scene, and names the view and its
  shading as menus.
- A tab strip appears above an area only when it holds two or more editors. The chosen tab has
  the area's colour and a 2-pixel accent underline, and a `+` adds an editor.
- Ctrl+Space maximises the area under the cursor and restores it.
- A viewport draws an axis marker at its lower left, and a perspective view a navigation gizmo
  at its upper right.
- The top bar holds the menus, the workspaces and a command search on F3. The status bar shows
  what each mouse button does under the current tool, the selection, the frame and the frame
  rate. A notice such as "Extruded 1 face" shows at the foot of the active area.

**The games' shell:**

- Sizes are set for a 1080-pixel-high window and scaled by the window's height. Body text is at
  least 26 pixels, and a title is 1.75 times body text.
- A pause menu dims the game to 60% black, and its panel is 90% opaque. It opens in 150 to 200
  ms and closes at once.
- Everything stays inside a safe area 5% in from each side and 4% from the top and bottom.
- Menus wrap from last to first. The focused item has an accent fill, a brighter label and a
  2-pixel ring.
- A settings row puts its label on the left and its value on the right.

## Decisions

| Decision | Step |
|---|---|
| UI: how a theme sets its colours. Decided in ADR-0128: derived from a base colour, an accent and a contrast, with any role overridable. | 1 |
| UI: which face the ui is set in. Decided in ADR-0129: Inter Regular and SemiBold, with Noto Sans for the characters Inter lacks. | 2 |
| Shell: how the games' menus are designed. Decided in ADR-0130: one designed shell, with a title and an accent colour per game. | 14 |
| Editor: how the window is divided. Decided in ADR-0127: tiled areas that hold editors as tabs. | 7 |
| UI: where the display's scale comes from, and what it multiplies. Decided in ADR-0131: a tool follows the monitor and a game its window's height, a user preference multiplies either, and the text and every size are redrawn at the result. | 3 |
| UI: where a transition's state lives, given that a retained component is redrawn from its layout every frame. To be decided at step 5. | 5 |
| Input: how a binding names a chord such as Ctrl+Z, and how a chord and a held modifier share a key. To be decided at step 10. | 10 |
| Editor: where an area layout is saved: with the project, in the user's settings, or as named workspaces in both. To be decided at step 7. | 7 |

## What blocks what

- **Step 1 blocks every step that draws.** The roles and states it adds are what the others
  colour with.
- **Steps 2 and 3 are independent**, and are best done early so that later steps are sized in
  their final face and scale.
- **Step 4 blocks step 6's menu shadows and step 14's backdrop.**
- **Step 10 blocks step 7's maximise key, step 6's shortcut column and step 11's shortcut
  hints**, since each needs a chord to show.
- **Step 7 blocks steps 8, 12 and 13.** A viewport header, the outliner and the properties editor
  are drawn inside areas.
- **Step 9 needs step 7** for where a notice is shown, and is otherwise independent.
- **Step 14 blocks steps 15, 16 and 17.**

## Steps

### Step 1 — Roles derived from three colours

**Closed.** ADR-0128 decided it.

- `paint::Palette` derives a twelve-step neutral scale and a twelve-step accent scale in OKLCH,
  and gives each `Dressing` colour role one step. Each neutral step has a lightness offset and a
  share of the base's chroma, fitted so that the built-in base reproduces the design's scale.
- `Dressing` gains `gap`, `bar`, `hairline`, `secondaryText`, `selected` and `pressed`, and its
  defaults are the built-in palette's. Disabled text is the design's tertiary text, so the two are
  one role, and no disabled fill was needed.
- Secondary text is a step lighter than the design drew it, at `#a3a7b1`, because the drawn one
  read at 4.3:1 on a hover.
- A `ui` or `tools` style that names `base`, `accent` or `contrast` derives every colour, and the
  roles it names directly win. The editor's theme, which only restated the old defaults, is
  empty, so the editor draws the derived colours.
- The highlight darkens until active text reads on it at 4.5:1, so any accent gives a usable
  highlight.
- Buttons in the tree and on toolbars draw `pressed` while a press is held, and go back to
  hovered or normal on release. A menu item has no held state, because it acts as the press
  lands. Menus and dropped panels draw with `radius`, and with their border traced around them.
- `PaletteTest` pins the scale and checks each role's contrast for both ui models. The resolver,
  toolbar, cursor and renderer suites cover derivation, overrides, undoing a theme, the pressed
  state and rounded menus.
- [Themes.md](../api/ui/Themes.md#colours-from-a-base-an-accent-and-a-contrast) states the
  derivation, the override rule and the contrasts.

### Step 2 — Inter in two weights

**Closed.** ADR-0129 decided it.

- `data/fonts/` holds Inter 4.1 Regular and SemiBold from the Inter project's release, with
  `Inter-OFL.txt`. Noto Sans's licence is renamed `NotoSans-OFL.txt` to sit beside it.
- `TextureFont::fallback()` gives a face another face for the code points it lacks. Packing
  sends each one there, `packed()` looks there, and a fallback shared by two faces packs a code
  point once. Every glyph lookup goes through `packed()`, so the text buffer needed no change.
- `TextRenderer` packs the regular and semibold faces into one atlas, with Noto Sans as the
  fallback both share. Its default atlas is 1024 on a side, for the second face. A missing
  semibold face sets semibold text regular, and a missing fallback leaves the missing glyph.
- `paint::Measure` and `paint::Write` keep their signatures. `TextRenderer::measure()` and
  `write()` take a `paint::Weight` instead, so a renderer is handed one pair per weight.
- `Dressing::titleWeight`, read from `title-weight`, sets a window's title and a table's header
  semibold, through the writer `Immediate::titles()` is given. `Screen` gives it the semibold
  face. A table's header is drawn in secondary text rather than disabled text. No retained
  component has a title, so the chosen item's weight waits on a component that draws one, in
  step 14's menu title.
- The font suite covers the fallback and a shared fallback packing once. The ui suite covers both
  weights, the fallback through `TextRenderer`, a missing face, the weight numbers and the
  title's writer.

### Step 3 — The display's scale

**Closed.** ADR-0131 decided it, after weighing the monitor's scale for every app and a transform
on the whole canvas.

- `Window::displayScale()` reads SDL's display scale, and the engine publishes
  `event::kind::DisplayScale` when it changes. `Shell::present()` gives the screen the scale, and
  passes each new one on.
- `Screen::Options::scaling` is `Display` for a tool or `Height` for a game, against
  `Options::reference`. `factor()` is `scale()` times what the rule follows, and the screen
  rebuilds when it changes. Every app keeps `Display` until its game pass, in step 17.
- The dressings start from `Dressing::scaled()`, and the resolver and the immediate layer multiply
  a theme's sizes by the factor. An outline is rounded down to whole device pixels and is never
  less than one.
- The editor's `uiScale` multiplies the monitor's scale through its text size.
- `v3dlib_shell` names `event`, `window` and EnTT, which it now uses directly.
- The palette, resolver and screen suites cover scaled dressings, a theme's sizes at a scale,
  both rules and the immediate layer across a rescale. The window's scale and the event need a
  window, so they are checked by running the editor.

### Step 4 — Soft shadows and a backdrop

Open.

- The canvas gains a rounded box whose edge fades out over a given width. It draws a menu's,
  a tooltip's and a notice's shadow.
- `Screen` can dim the whole canvas behind a container, for a pause menu or a modal prompt.

### Step 5 — Transitions

Open. Needs its decision.

- A hover change, a menu opening and a notice fading run over the durations in the design. The
  dressing holds the durations, and a duration of 0 turns a transition off.
- Layout stays a function of the current frame's input. A transition changes only colour,
  opacity and a small offset.

### Step 6 — Tooltips and menus

Open. The shortcut column needs step 10.

- A component may carry a tooltip. The design's delays apply, and a tooltip shows a control's
  name and its shortcut.
- A menu item may carry an icon and a shortcut. The shortcut is filled in from the bindings,
  not written in the ui document, and is drawn right-aligned in secondary text.
- A menu may hold separators. A submenu opens after its delay, or at once on a click or the
  right arrow key.

### Step 7 — Areas

Open. ADR-0127 decided the areas, and the layout's home has to be decided.

- `ViewLayout` becomes a tree of splits whose leaves are areas. An area holds one or more
  editors, and a viewport is one kind of editor.
- Areas are drawn with the design's gaps, corners, hairlines and active outline.
- Dividers take the design's reach, highlight, cursor, cancel, snap, reset and least size. The
  window gains the resize cursors.
- An area can be split, joined, swapped and maximised, from its editor-type menu and by keys.
  A tab dropped on an area's edge splits it.
- The layout is saved and restored where the decision says. `data/layout.json` gives the layout
  a new user starts with.

### Step 8 — Viewport headers and overlays

Open. Needs step 7.

- A viewport's header names its view as a menu of the fixed cameras, and its shading as a menu
  of the display modes. The View menu's camera and render-mode items stay.
- Each view draws an axis marker, and a perspective view a navigation gizmo that turns the camera
  to look down an axis when clicked.

### Step 9 — The status bar and notices

Open. Needs step 7 for where a notice is shown.

- A status bar along the foot of the window shows what each mouse button does under the current
  tool and modifiers, the selection, the frame and the frame rate.
- A completed command can post a notice, which shows at the foot of the active area. Undo and
  redo post the name of the change.

### Step 10 — Key chords

Open. Needs its decision.

- A binding can name a key with modifiers, such as Ctrl+Z. A modifier held alone still drives
  the camera tools.
- The editor binds undo, redo, save and maximise to their usual chords.
- [editor/README.md](../editor/README.md#running-it) loses its note that bindings cannot express
  a chord.

### Step 11 — Command search

Open. Shortcut hints need step 10.

- F3 opens a search over every registered command. It matches as the user types, and shows each
  command's menu path and shortcut. Enter runs the chosen command.

### Step 12 — The editor's chrome

Open. Needs step 7.

- The menus are regrouped as File, Edit, Create, Select, Modify, View, Animate, Render, Window and
  Help.
- The tool shelf has 18-pixel icons with tooltips, and covers the modelling operations as well as
  the transform tools. The icons come from one sheet.
- The select mask moves into the viewport header.
- The top bar shows the workspaces, the project's name with a mark for unsaved changes, and the
  command search.

### Step 13 — The outliner and the properties editor

Open. Needs step 7.

- An outliner lists the scene's items as a tree, with a filter. A click selects, and the
  hierarchy matches `Scene`'s links.
- A properties editor shows the selected item's placement and settings, and edits them through
  commands, so each edit undoes.

### Step 14 — The games' shell

Open. ADR-0130 decided it.

- `api/ui` ships a game theme, and `Shell` loads it beneath a game's own document. A game's
  document names its title and accent.
- The game menu is drawn with a title, over the dimmed backdrop, inside the safe area, sized
  from the window's height. The game's simulation stays paused while the menu is open.
- Menus wrap, and the focused item shows the design's three cues.

### Step 15 — Settings that show their values

Open. Needs step 14.

- A numeric item shows its value, a key item the key bound to it, and a slider its value.
- pong's Options menu shows its rounds and its four bindings.

### Step 16 — HUD text and statistics

Open. Needs step 14.

- The game theme has a HUD style for a score or a label drawn over the game, on a plate whose
  opacity the theme sets.
- `StatisticsOverlay` draws from the theme.
- tetris's side panel and pong's scores draw through the theme, and their hard-coded colours go.
- `demos/pong/data/fonts.json` is deleted.

### Step 17 — Each game's pass

Open. Needs steps 14 to 16.

- Each game names its title and accent, and draws its menu at the shell's sizes, which ends
  odyssey's and diorama's 48-pixel menus.
- Each game is run, its menu, settings and HUD are photographed, and the validation log is read.
