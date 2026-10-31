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
| UI: where a transition's state lives. Decided in ADR-0132: on what it animates, a retained component or an immediate widget's per-id state, on a clock of real time. | 5 |
| Input: how a binding names a chord, and how a chord and a held modifier share a key. Decided in ADR-0133: modifier words before the key, and the most specific binding wins it. | 10 |
| Editor: where an area layout is saved. Decided in ADR-0134: named workspaces in the user's settings, which also record each project's last arrangement by its path. | 7 |

## What blocks what

- **Step 1 blocks every step that draws.** The roles and states it adds are what the others
  colour with.
- **Steps 2 and 3 are independent**, and are best done early so that later steps are sized in
  their final face and scale.
- **Step 4 blocks the shadows of step 6's tooltips and step 9's notices, and step 14's
  backdrop.**
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

**Closed.**

- The canvas gains a rectangle with a colour per corner, and a ring that blends from its inner
  radius to its outer one, or from its centre to its rim when it has no hole.
- `paint::shadowBox` fills a rounded box and fades its edge to nothing over a width. The fade is
  three bands on half a turn of a cosine, so it has no hard inner edge, and its corners turn about the box's
  own corner centres, so no two pieces overlap.
- `Dressing` gains `shadow`, `backdrop`, `shadowSize` and `shadowOffset`. Both colours are palette
  roles. A shadow is black at 45% in a dark theme and 18% in a light one, and a backdrop black at
  60% in either.
- A centred menu and every panel a menu bar drops cast a shadow now, since both exist. Tooltips
  and notices take theirs when steps 6 and 9 draw them.
- A container's `backdrop` flag, read from its document, lays the backdrop over the whole canvas
  before the container is drawn. It is on `Container` rather than `Screen`, because the
  component renderer draws containers in order and the game menu and the prompt are each one.
  No app sets it until step 14.
- The canvas suite covers both blends. The painter suite pins the fade's curve, and checks that
  the pieces cover the reach once, square and rounded. The renderer suite covers a menu's shadow
  and a backdrop, and the document suite the flag. pong's menu was photographed: under the
  panel the shadow darkens the court's divider from 89 to 49, and it fades back over 12 pixels.

### Step 5 — Transitions

**Closed.** ADR-0132 decided it, after weighing a store in the renderer and a record kept by the
cursor router.

- `ui::Transition` eases a colour toward whatever is asked for, starting whenever that changes and
  from the colour shown at that moment. A fill that appears or goes fades in its own colour.
  `ui::Opening` measures how far something has opened since the frame it was first drawn.
- A retained component carries a `Motion`, and an immediate widget's `Retained` entry a fill
  transition. Buttons, menu items, menu bar titles and the immediate layer's widget fills ease
  over `hover-duration`. A centred menu and a dropped panel fade in and slide down
  `open-distance` over `open-duration`, and close at once.
- The canvas gains `fade()` and `unfade()`, which multiply the alpha of everything drawn under
  them, text included.
- `Screen::begin()` advances a clock by the real time between frames and hands it to both ui
  models, so no app changed. Without a clock, every change shows at once.
- Layout never reads a transition, so a sliding panel is picked where it lands.
- Notices fade when step 9 draws them, and the game menu opens over 150 to 200 ms when step 14's
  game theme sets `open-duration`.
- The transition suite covers easing, retargeting partway, easing in from nothing, easing away
  and opening across frames. The canvas suite covers fading, and the renderer, immediate and
  resolver suites cover a button's hover, a menu opening, drawing without a clock, the
  immediate layer's fill and the theme's durations.

### Step 6 — Tooltips and menus

**Closed.**

- Any component may carry a `tooltip`. `shell::Tooltip` decides when it is due: after 500 ms of
  rest, or 100 ms when another was showing a moment before. The screen draws it last, under its
  component, with the shortcut of the command the component sends. The cursor writes what it
  rests on as `Interface::hovered()`, a toolbar's button included, and a press clears it.
- A dropped panel shows each item's shortcut right-aligned in secondary text, and widens for it.
  The screen's `Options::shortcut` looks it up from the bindings as the panel is drawn, and the
  shell fills it with a command's first keyboard binding, printed by `event::printed()`.
  `Engine::bindings()` is public so the shell can read them.
- A menu item may name an `icon`, drawn in the mark column and resolved with the ui's images. A
  `separator` item is a rule half a row tall that needs no label, and that neither the keys
  nor the cursor choose.
- A submenu in a dropped panel opens after 200 ms of rest on its item, and at once on a press.
  The renderer settles a waiting submenu with the ui's clock.
- The editor's tools carry tooltips, and its Project menu separators.
- The right arrow was to open a submenu at once, but a menu bar's panels take no keys at all.
  Walking them from the keyboard moves to step 12.
- The chord, menu bar, renderer, tooltip, cursor, document and shell suites cover the printed
  chord, the delay, a press opening at once, separators to the cursor and the keys, the
  shortcut column, the tooltip's plate and timing, what the cursor rests on, the document's
  keys and the shell's lookup.

### Step 7 — Areas

**Closed.** ADR-0127 decided the areas, and ADR-0134 where a layout is saved, after weighing
saving it with the project, in the user's settings alone, and in each project's directory.

- `AreaLayout` replaces `ViewLayout`. It is a tree of splits whose leaves are areas, and an area
  holds one or more editors, of which a viewport is one kind. `save()` writes what `load()`
  reads. `AreaViews` gives each viewport editor a `ViewPort` for as long as the editor lasts.
- Areas sit 3 pixels apart. The renderer draws the gaps, rounded corners, hairlines and the
  active area's accent line under the ui, and lights the divider under the cursor or held.
- A divider is taken 4 pixels either side of its gap and keeps each side to its least size.
  Escape and a right click cancel a drag, Ctrl snaps it to tenths and a double click evens it.
  The window shows resize cursors over it. `MouseButton` carries the platform's click count and
  `MouseMotion` the modifiers held. The metrics scale with the ui.
- `AreaChrome` gives each area a header and a maximise button. The header is a menu bar that is
  not docked, with one menu named for the editor's kind. It splits, joins and swaps the area,
  adds and closes tabs, and maximises it. An area with two or more editors has a strip of tabs.
  Ctrl+Space maximises the area under the cursor.
- A tab dragged onto an area's edge makes a new area there, and one dragged into an area joins
  its tabs. The frame shows where it would land.
- `api/ui` gained a menu bar laid out by its layout, data on a command, containers added and
  removed as an app runs, and an open menu that takes a press before any container.
- `Workspaces` reads the shipped set from `data/layout.json`, Modelling and Perspective, and
  keeps the user's own set and each project's arrangements in the engine's settings. Records of
  projects that no longer exist are pruned at startup. Ctrl+Page Down and Ctrl+Page Up step
  through the workspaces, and the Workspace menu saves, resets, exports and imports them.
- The suites cover the layout, the views, the frame, the pointer's dividers and tab drags, the
  chrome, the workspaces, the mouse's clicks and modifiers, and the ui's additions. Each area's
  header was photographed. The header's menus and the keys could not be driven in the running
  editor, because SDL does not take posted mouse messages, so they rest on their tests.

### Step 8 — Viewport headers and overlays

**Closed.**

- A viewport's header has a menu named for its camera, holding the fixed views and
  Orthographic, and one named for its shading, holding the shading, each side's fill and the
  cull. Each marks its own view. Their commands carry the area's id, and the same commands from
  a key or the View menu act on the active area, so the View menu's items stay as they were.
- `AreaChrome::marks()` lists what a view holds for both the View menu and the headers, and
  `AreaChrome::fixedViews()` replaces `Controller::fixedViews()`. A header is built again when
  its camera or its shading's name changes.
- Showing a fixed view writes the camera into its editor's state through `AreaLayout::set()`,
  so its tab and a saved layout name it.
- `ViewOverlay` draws each view's axis marker at its lower left, and a perspective view's
  navigation gizmo under its header at its upper right. A press on a ball turns the camera to
  look down that axis, through the pointer's new `controls()` hook, which takes a press before
  the dividers and the tools.
- The overlay suite covers the axes from the front and the top, the negative ends, both
  positions, finding a ball, turning the camera and drawing. The chrome suite covers the view
  menus, their marks and the rebuild, the pointer suite a control taking a press, the layout
  suite setting an editor's state, and the bindings suite every command a header sends. The
  editor was photographed. A click on the gizmo or a header's menus could not be driven in the
  running editor, so those rest on their tests.

### Step 9 — The status bar and notices

**Closed.**

- The editor draws a status bar along the foot of the window, beside the left toolbar, in the
  theme's `bar` with a `hairline` above. `StatusBar` says what the left and middle buttons do
  under the camera move, the select tool's adding and the transform mode, and counts the
  selection under the mask, an edge once. The frame and the frame rate sit beside the count.
  The areas are laid out above the bar.
- `Notices` shows an operation's message, done or refused, at the foot of the active area for
  2.5 s, fading in and out over 0.15 s, on a plate with a menu's shadow. `Controller::report()`
  posts every outcome, so undo and redo post the name of the change, and `refuse()` posts what
  a handler turns down itself.
- `Screen::clock()` gives an app the ui's clock, which the notices are timed on.
- The status bar and notice suites cover the buttons under each tool, the counts under each
  mask, an edge's halves, the frame rate, a notice's fades and its replacement. The screen suite
  covers the clock starting at zero. The editor was photographed with a notice showing.

### Step 10 — Key chords

**Closed.** ADR-0133 decided it, after weighing an exact match and letting every match fire.

- A source name may put `ctrl`, `shift` and `alt` before its key, as in `ctrl+shift+z`, each
  matching either side of the keyboard. `event::chord()` splits a name into its key and its
  modifiers.
- A source carries the modifiers held when it was sent, which both devices read from SDL. The
  mapper fires, of the bindings on a key whose modifiers are held, only those naming the most.
  A plain binding still fires under a modifier no chord claims, so a held modifier still drives
  the camera tools.
- The router sends a key's repeats and release to what its press matched, so letting go of Ctrl
  first still releases the chord. `Engine::held()` holds a chord while its key and its
  modifiers are down.
- A chord keeps its whole name in the bindings, so rebinding, saved overrides and `bound()` are
  unchanged, and the owner's check is put to its key.
- The editor binds undo to Ctrl+Z, redo to Ctrl+Y and Ctrl+Shift+Z, open to Ctrl+O and save to
  Ctrl+S. Maximise has no command until step 7, which binds it to Ctrl+Space.
- The chord, mapper, router, bindings and engine suites cover parsing, the most specific match,
  a plain binding under an unclaimed modifier, a chord's release after its modifier, a chord's
  name in the bindings and a held chord.

### Step 11 — Command search

**Closed.**

- F3 opens a panel of a field and a list. `CommandSearch` lists every command on the editor's
  menu bar and an area's header, under its menu path, with the shortcut its menu item shows.
  A held command and a dialog's buttons are on no menu, so the search covers what can be run
  rather than every registered command. The statistics overlay moved from F3 to F12.
- Every word of a query has to be in a command's path. A command whose own label begins with
  the first word comes first, then one where every word begins a word of the path. Typing
  matches again, the arrows move the choice and wrap, Enter or a click runs the command and
  Escape closes the panel. The editor takes the arrows and Escape before the ui while the panel
  is up, so Escape does not quit and closing the window still does.
- `Screen::shortcut()` gives an app the shortcut a menu shows.
- The search suite covers the catalogue, the ranking, a row's shortcut, typing and moving the
  choice. The bindings suite covers the panel's commands. The editor was run: F3 and "cu" listed
  Cube, the curves and the culls, the down arrow moved to Select Mask > Curve, and Enter set
  that mask.

### Step 12 — The editor's chrome

**Closed.**

- The menus are File, Edit, Create, Select, Modify, View, Animate, Render, Window and Help. The
  commands are unchanged. The camera moves are View > Navigate, the workspaces are on Window,
  and Help holds the command search.
- The tool shelf holds the four transform tools and the five modelling operations as icons,
  each with a tooltip. The icons are cells of one sheet, `data/icons/icons.png`, which
  `IconSheet` reads from `data/icons.json`, and the renderer resolves an `icon:` source to its
  cell. The separate PNGs are gone. A column toolbar's icon buttons are square, and a tooltip
  on a left toolbar hangs beside its button. The editor's icons are 18 pixels beside its 15
  pixel text, on 39 pixel buttons, since the shelf is as wide as an icon and the padding.
- An open menu bar takes the arrow keys, return, space and escape before the focus. The right
  arrow opens a submenu at once, or moves to the next menu.
- Each viewport header has a mask menu, and the select-mask toolbar is gone.
- `TopBar` puts a tab for each workspace, the project's name with ` *` while unsaved, and a
  Search button beside the menus. A closed menu bar takes a press only on a label, so the bar's
  buttons get theirs. A press outside every area now reaches no tool, so a click on the status
  bar no longer clears the selection.
- The menu bar, keyboard, toolbar and renderer suites cover the keys, the closed bar's press,
  square icons and a tooltip beside its box. The editor's suites cover the mask menu, the top
  bar, the icon sheet, a press outside the areas and every header command. The editor was
  photographed with the new menus, the top bar and the shelf. The shelf's tooltips and the menu
  keys could not be driven in the running editor, so they rest on their tests.

### Step 13 — The outliner and the properties editor

**Closed.** ADR-0135 decided that item names need not be unique, and ADR-0136 that panels are
drawn in immediate mode, after weighing retained documents rebuilt on a signature.

- Items have names, which the scene document saves at schema version 4 and an import takes from
  its nodes. A new item is named for its kind.
- The immediate layer gained a region with no title bar, a tree row that folds, a number that is
  dragged or typed into, and `itemFinished()`.
- An editor other than a viewport is a panel, drawn from the scene every frame under its area's
  header. A press on one reaches no tool. The header's area menu has an Editor submenu that
  changes the kind of editor shown, and Modelling has an outliner over a properties editor down
  its right.
- The outliner lists the items as the hierarchy's tree, folds rows, filters by words with each
  match's ancestors, and selects the item clicked, toggling while Shift is held.
- The properties editor shows the one selected item's name, its numbers a curve can drive, and
  a camera's projection, a light's type or a curve's render style. An edit is written as it
  happens and recorded as one `command::Edit` when it ends.
- The suites cover names in documents and imports, the layer's additions, and the outliner's
  rows and clicks. They cover the pointer's panels, replacing an editor, the Editor menu, the
  edit command, and the properties editor's drags, rename and refused lens. The editor was
  photographed with both panels. A click in a running panel could not be driven, so clicks rest
  on their tests.

### Step 14 — The games' shell

**Closed.** ADR-0130 decided it.

- The game theme is `data/ui/game.json`, in the shared data every game copies. `Shell` puts it
  beneath the ui document's themes through `Interface::beneath()`, where a style fills in what
  it does not name from the style of the same class and name. `Shell::Options::theme` names it,
  and the editor sets it empty.
- The theme gains `panel-opacity`, which multiplies the panel's alpha whether it was derived or
  named, so a game's own base still gives a 90% panel. The game theme also sets 8 pixel
  corners, a 2 pixel focus ring, a 24 pixel shadow and a 0.18 s opening.
- A menu may name a `title`, and a submenu is titled by its item's label. A centred menu writes
  it above its items, semibold at 1.75 times the body size, through `ComponentRenderer::emphasis()`,
  which `Screen` fills. The chosen item is written semibold, ringed in `focus` and filled with the
  highlight, and the menu is as wide as its widest label set semibold.
- A centred menu is placed in the safe area, `Arranger::safe()`. `GameMenu` dims the game with
  the backdrop as it opens.
- `Shell::game()` is a game's screen: 26 pixel body text at 1080 pixels, following the window's
  height. Each game adopts it in step 17. Every game already paused its simulation through the
  suspend callback, and menus already wrapped, so neither changed.
- The theme, document, arranger, renderer, game menu and shell suites cover the merge, the
  opacity, the title, the chosen setting, the ring, the safe area, the backdrop and the shell's
  theme. pong was photographed with its menu up, and with a title added to its built copy of the
  document, over a clean validation log.

### Step 15 — Settings that show their values

**Closed.**

- A centred menu's row puts its label on the left and its value right-aligned, in secondary
  text unless the row is chosen, and the menu makes room for the widest value.
- A numeric or text input shows the value it holds. A key input that `binds` a command shows
  the key bound to it through the screen's shortcut lookup, so the row stays true after a
  rebinding. It shows an ellipsis while it waits on a key.
- A `slider` item holds a whole number in a range. `ui::menuLess` and `ui::menuMore`, which
  `GameMenu` declares, step it and send its command with the new value. It shows its value
  between the arrows that step it.
- `GameMenu::item()` finds an item by its command, so a game gives a setting the value it holds.
- pong's Rounds is a slider from 1 to 21, set from the game's state at start, and stepped with
  left and right. Its numeric input sent the key's name as a string, which the handler
  ignored, so the rounds could not be changed before. Its four key items bind the paddle
  commands, and the labels lose their trailing colons.
- The game menu suite covers stepping, the ends of the range, an item that is not a slider and
  finding an item. The document suite covers reading a range and a binding, and the renderer
  suite a row's value, colour, column and capture. pong was run: the Options menu showed 5 and
  the four keys, the right arrow stepped the rounds to 7, and a key item showed the ellipsis
  while capturing.

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
