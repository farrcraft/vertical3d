# Editor Finish — The Editor Brought to Its Designed Look and Its Missing Editors

Drafted 2026-10-31 against `fb84216f`. This plan's steps fall in `api/ui` and `vertical3d`. When
it closes, every part of the editor's window matches the design its tiled areas were built
from. The views draw from the theme. The headers read as menus. The outliner, properties
editor, status bar and notices show what the design shows. A timeline, a graph editor, a
console, a render view and render settings are editors an area can hold. The shipped
workspaces include Animation, Shading and Rendering.

The editor is described in [editor/](../editor/README.md), and `api/ui` in
[api/ui/](../api/ui/README.md) and [internals/ui/](../internals/ui/README.md).

A step with a testable CPU half brings test cases with it. A step that changes how something
looks is checked by running the editor and reading a screenshot, as
[Testing.md](../contributing/Testing.md) describes.

## What the survey found

The editor was built at `fb84216f`, run at its default window size, and photographed at
startup, with a cube selected, with the command search open, in each workspace and with an
area maximised. Each picture was set beside the design's mockup of the same state.

**What matches the design.** The areas sit 3 pixels apart with rounded corners and hairlines,
and the active area has the accent outline. Each area has a header and a maximise button. The
top bar's ten menus match the design one for one. The tool shelf holds the transform tools and
the modelling operations, with the chosen tool in the accent. Each view has its axis marker,
and a perspective view its navigation gizmo. The outliner stands over the properties editor
down the right. The status bar, the notices and the command search are all present.

**What differs from the design:**

* **The text is larger.** The design sets body text at 12 pixels on rows of 22 to 26. The
  editor sets it at 15 (`vertical3d/src/Renderer.h:256`), so every row, header and bar is
  taller and the window holds less.
* **The views draw outside the theme.** The grid's lines are grey with a blue tint and its
  lines through the origin are near white (`vertical3d/src/view/ConstructionPlane.h:83-85`). The
  design draws the origin lines in the axis colours. A selected object is drawn light blue
  (`vertical3d/src/view/WireframeVisitor.h:107`), where the design draws selected geometry in
  amber.
* **A viewport's header does not read as menus.** It is an opaque strip of plain words:
  "Viewport", the camera, the shading and the mask. The design draws it at 80% opacity over
  the scene, with the view's name semibold in a pill and a drop mark on each menu. It adds an
  Overlays menu, and an icon for the editor's kind in place of its name. No menu bar label in
  `api/ui` can show a drop mark.
* **The gizmo has no disc at rest.** A disc lies behind it only while the cursor is over it
  (`vertical3d/src/view/ViewOverlay.h:74`). The design keeps a faint one behind it always.
* **The outliner is a flat list.** Its rows have no icon for their kind, there is no Scene row
  at the root, and the chosen row is a solid accent bar where the design tints it. A tree row
  in the immediate layer has no icon (`api/ui/Immediate.h:249-253`), and the layer draws no
  image at all.
* **The properties editor is one field to a row.** Translation, rotation and scale stack their
  X, Y and Z down the panel (`vertical3d/src/control/PropertiesEditor.cxx:202`). The design sets
  the three side by side with their letters in the axis colours and their units after the
  value, under tabs for Item, Mesh, Shading and Keys. The scene has no unit of length.
* **The status bar is words.** It names each mouse button ("Left Select") and counts the
  selection ("1 item") (`vertical3d/src/control/StatusBar.h:31-43`). The design draws a mouse
  with the button lit, a chip for a held modifier, the selected item's name, its vertices and
  faces, and a frame rate coloured by how healthy it is.
* **A notice is text alone.** The design gives it a mark for done or refused and the shortcut
  that undoes it.
* **The top bar has no mark of the app**, and its workspaces are separate buttons where the
  design groups them.
* **A divider shows no hint.** The design shows how a drag, a double click, Escape and Ctrl
  act on it, on a tooltip beside the divider under the cursor.

**What is missing:**

* **Three of the design's editors and two of its panels.** An area can hold a viewport, an
  outliner or a properties editor (`vertical3d/src/control/AreaChrome.cxx:232-238`). The design
  adds a timeline, a graph editor, a console, a render view and render settings. The render
  view and the render settings exist as floating containers
  (`vertical3d/data/vgui.json:353`, `:389`), outside the areas.
* **A timeline.** The scene keeps a timeline (`vertical3d/src/scene/Timeline.h:17`) and each
  item's curves, and the Animate menu plays, steps and keys them. Nothing shows the frame range,
  the keys or the playhead. Nothing sets keys as an edit is made.
* **Three workspaces.** Only Modelling and Perspective ship (`vertical3d/data/layout.json`). The
  design's Animation, Shading and Rendering do not exist. No shipped area holds two editors, so
  a tab strip is never seen until one is made by hand.
* **The editor-type menu's shortcuts.** The design gives each kind of editor a key that shows it
  in the area under the cursor.

## The design

The colours, type, spacing and motion are the ones the tiled areas were built to. They are in
the theme's palette already. These are the parts this plan adds:

**Views:**

| Part | Target |
|---|---|
| Grid lines | The theme's hairline over the area colour, every tenth line a step brighter |
| Lines through the origin | The two axes in the view's plane, in their axis colours at 75% |
| A selected object or component | `#f2a33a` |
| Gizmo | A disc of white at 5% behind it always, brighter under the cursor |

**A viewport's header**, 28 pixels, at 80% of the area header colour over the scene:

- An icon button for the editor's kind, with a drop mark, which opens the editor-type menu.
- The view's name, semibold, on a pill in the control colour while the view is active.
- The shading, with a drop mark.
- An Overlays menu holding what `G`, `M` and `H` toggle.
- The maximise button at the right.

**The outliner:** a Scene row at the root. Each row has a 14 pixel icon in its kind's colour:
a camera, a light in yellow, a mesh in orange, a curve in green and a group in grey. The chosen
row is the accent at 24%.

**The properties editor:** tabs for Item, Mesh, Shading and Keys, in that order. Each of
location, rotation and scale is one row of three fields, each led by its letter in its axis
colour. Lengths carry their unit and angles a degree sign.

**The status bar**, 26 pixels at 11 pixel text: for each button, a 11 by 15 pixel mouse with
that button lit, a chip for a modifier the action needs, and the action. Then the item's name,
its counts, the frame of the range, and the frame rate in green, amber or red.

**A notice:** a check for done or a cross for refused, the message, and the undo shortcut in
secondary text.

**The timeline**, 132 pixels: first frame, previous key, play, next key and last frame; the
frame; the range; the rate; Auto key. Below them, a row for each selected item with its keys
as diamonds, and the playhead across all of them in the accent.

**The graph editor:** each curve of the selected items in its axis colour over a grid of
frames and values, with the chosen key and its handles in the selection's amber.

**The console:** what was run and what it said, above a line that takes a command.

**The workspaces:** Modelling and Perspective as they ship. Animation adds an area along the
foot holding the timeline, the graph editor and the console as tabs. Shading puts a shaded
view beside the properties. Rendering puts the render view beside the render settings.

## Decisions

| Decision | Step |
|---|---|
| Editor: the ui's text size. Decided in ADR-0138: 12 pixels, with the scale preference for larger. | 1 |
| Editor: the scene's unit of length. Decided in ADR-0140: each project names its own, metres by default. | 6 |
| Editor: what the console runs. Open: the editor's commands by name, or a scripting language. | 13 |
| Editor: where a render is shown. Open: an editor in an area, a floating window, or both. | 14 |
| Editor: how icons are made. Decided in ADR-0139: SVG files, drawn at the ui's scale. | 4 |

Each decision is researched against how established tools answer it before it is drafted as an
ADR.

## What blocks what

- **Step 1 blocks every step that draws**, since it sets the size everything else is laid out
  at.
- **Step 4 blocks steps 3, 5 and 7.** The header's kind icon, the outliner's rows and the status
  bar's mice are images in the immediate layer or the icon sheet.
- **Step 6 needs the unit decided** before lengths can carry one.
- **Steps 11 to 14 block step 15.** A workspace can only hold editors that exist.
- **Step 11 blocks step 12.** The graph editor shares the timeline's frame scale and playhead.

## Steps

### Step 1 — The ui's text size

**Closed.** ADR-0138 decided it, after weighing 15 pixels and 13.

- The editor's text is 12 pixels. Rows are 24 pixels, headers and bars 28, the gap around text
  12, and a dropped panel's padding 4. Icons are 18 pixels on 30 pixel buttons.
- The glyphs were magnified in a screenshot and read sharply at 12 pixels, so the margin 13
  pixels would have given was not needed.
- The design's 11 pixel secondary text needs a size of its own, which the status bar in step 7
  is the first to use.

### Step 2 — Views drawn from the theme

**Closed.**

- The renderer gives each view's grid its colours every frame. An ordinary line is the theme's
  hairline laid over the view's background, and every tenth line is twice as strong. A line is
  drawn opaque, so the blend is made before the line is.
- A line through the origin takes the colour of the world axis it runs along, at 75%.
- A selected object and a selected component share one amber, `#f2a33a`. The theme's
  `selected` role is the accent at 24%, a tint behind a chosen row, so it is not the colour
  geometry is selected in.
- The gizmo keeps a disc of white at 5% behind it at rest, and 10% under the cursor.
- The grid suite covers the axis colours by direction, and the overlay suite the disc at rest
  and under the cursor. The editor was photographed with a cube selected.

### Step 3 — A viewport's header

**Closed.**

- `api/ui`: a menu bar has an `opacity` that its strip is drawn at. Each of its menus may show an
  `icon` in place of its label and a `drop` mark after it. `emphasis` sets a title in the chosen
  item's weight on a pill of the widget colour. A title's icon and mark take the label's colour.
- A header is drawn at 80% over the view. Its editor menu shows the kind's icon, every menu has
  a drop mark, and the active view's camera name is set apart. `AreaChrome::sync()` takes the
  active area, so the emphasis moves without a rebuild.
- An Overlays menu, shown as its icon, toggles the grid, the meshes, the cameras, the lights and
  the handles, each marked while on.
- Shift+F5, Shift+F7 and Shift+F9 show a viewport, the properties editor and the outliner in the
  area under the cursor. The header's Editor menu shows each beside its kind, as every menu
  shows the first key bound to an item's command.
- The menu bar and document suites cover the opacity, the icon, the drop mark and the
  emphasis. The chrome suite covers the header's dressing, the Overlays menu's marks and the
  emphasis following the active area. The editor was photographed with the headers dressed.

### Step 4 — Images in the immediate layer and the icon sheet

**Closed.** ADR-0139 decided how the icons are made, after weighing SVG drawn to a sheet by a
script, an icon font, and the hand-drawn sheet.

- Each icon is an SVG file in white. The renderer draws every icon into one image at the ui's
  icon size in device pixels, draws them again when the scale changes, and points the ui's
  images at the new texture. The hand-drawn sheet is gone, and its ten icons are redrawn from
  the design's paths.
- `v3dlib_image` gains `reader::Svg`, which draws a document at its own size or scaled to fit a
  size and centred, through NanoSVG. Both manifests name the port.
- The sheet gains icons for each kind of item and of editor, a mouse with each button lit, a
  check, a cross, the app's mark, a drop mark and the Overlays mark.
- `api/ui`: a button's icon is tinted the colour its label would be, and a menu item's the
  colour of its label. The immediate layer draws an image at a size, centred down a line, and
  text in a colour of its own, and a tree row may lead with a tinted icon.
- The image suite covers drawing at both sizes and what cannot be drawn. The ui suite covers
  the tint, the image, the coloured text and the row's icon. The editor suite covers the
  sheet's grid, drawing into cells, and every shipped icon drawing. The editor was
  photographed with the shelf drawn from SVG.
- The decision was not foreseen when the plan was drafted. It is recorded in the decisions
  above.

### Step 5 — The outliner

**Closed.**

- A Scene row at the root holds every item, open until it is folded. A click on it selects
  nothing.
- Each row leads with its kind's icon in its kind's colour. `Outliner::mark()` names both, and
  the outliner is given the renderer's image lookup to resolve them.
- `api/ui`: a chosen tree row is drawn on the theme's `selected` role, the accent at 24%, instead
  of the solid highlight.
- The outliner suite covers the Scene row's fold and its click, and each kind's icon and
  colour. The ui suite covers the chosen row's tint. The editor was photographed with a cube
  selected.

### Step 6 — The properties editor

**Closed.** ADR-0140 decided the unit, after weighing a fixed metre and no unit.

- A project names its unit of length: millimetres, centimetres, metres, kilometres, inches or
  feet, and a new one metres. It is stored in the meta table as `length_unit`. Changing it moves
  nothing, and is one change in the history.
- `api/interchange` gains `rescale()`, which multiplies every length of an imported scene by a
  factor. An import is scaled from metres into the document's unit.
- With nothing selected, the properties editor shows the scene's unit as a list.
- One item shows tabs for Item, Mesh for a mesh, and Keys. Shading was left out, because an item
  has no shading of its own to show yet. Mesh counts vertices and faces, and Keys lists each
  curve with its count of keys.
- Translation, rotation and scale are each one row of three fields, led by their letters in the
  axis colours. A distance carries the unit, an angle a degree sign, and a light's cone `rad`.
- `api/ui`: a number field takes a `NumberLook` for its label's colour and its unit, shows its
  value without the zeros it does not need, and never starts the value before the label ends.
  `rowWidth()` gives a row's width for dividing it. `Screen::Options::charset` names the glyphs
  beyond ascii, and the editor adds a degree sign, a middle dot, a multiplication sign, an en
  dash and an ellipsis.
- The interchange suite covers the rescale. The editor suites cover the unit's symbols, the
  store's round trip and refusal, the command, the import's scale, the row of three, the units,
  and the tabs. The ui suites cover the number's look and form, and the charset. The editor was
  photographed with a cube selected.

### Step 7 — The status bar

**Closed.**

- Each button's action is a mouse with that button lit and the action. While the left button
  only selects, Shift, Alt and Ctrl follow it on chips, with what each does. The bar's text is
  11 pixels.
- One selected item is named, with a mesh's vertices and faces. Several are counted by kind.
- The frame reads "Frame 24 of 120".
- The frame rate is green at nine tenths of the monitor's rate or more, amber below that, and
  red below half. `api/window` gains `Window::refreshRate()`.
- The status bar suite covers the modifiers, the icons, the selection, the frame and the
  health. The editor was photographed with a cube selected.

### Step 8 — Notices

**Open.**

- A notice shows a check when the operation was done and a cross when it was refused.
- A notice from an undoable operation shows the undo shortcut in secondary text.

### Step 9 — The top bar and the dividers

**Open.**

- The top bar leads with the app's mark, in the accent.
- The workspaces are one group of tabs, the chosen one on the control colour and semibold.
- A divider under the cursor shows a tooltip after the usual delay: drag to resize, double
  click to even, Escape to cancel, and Ctrl to snap to tenths.

### Step 10 — Auto key

**Open.**

- A setting on the timeline, saved with the project. While it is on, an edit to a number a
  curve can drive sets a key at the current frame, in the same undo step as the edit.
- The Animate menu shows it as a toggle.

### Step 11 — The timeline

**Open.**

- A timeline is a kind of editor an area can hold.
- Its controls play, stop, step and jump through the keys as the Animate menu does. Its fields
  edit the frame, the range and the rate.
- A row per selected item shows its keys as diamonds, the chosen keys in the selection's amber.
  A click moves the playhead there. A drag along the frames scrubs.
- A key can be chosen, moved along the frames and deleted, with undo.

### Step 12 — The graph editor

**Open.** Needs step 11.

- A graph editor is a kind of editor an area can hold. It shares the timeline's frames and
  playhead.
- It draws each curve of the selected items, in its axis colour where it has one.
- A key and its handles can be chosen and dragged, with undo. The view pans and zooms as an
  orthographic viewport does.

### Step 13 — The console

**Open.** Decide what the console runs.

- A console is a kind of editor an area can hold.
- It lists each command run and each notice posted, in the order they happened.
- Its line takes a command as decided, runs it, and lists what it said. The arrows step
  through what was typed before.

### Step 14 — The render view and the render settings

**Open.** Decide where a render is shown.

- The render view and the render settings are kinds of editor an area can hold, as decided.
  Each shows what its floating container shows now.
- Render opens the render view in an area that holds one, as decided.

### Step 15 — The workspaces

**Open.** Needs steps 11 to 14.

- Animation, Shading and Rendering ship beside Modelling and Perspective.
- Animation's foot holds the timeline, the graph editor and the console as tabs, so a tab strip
  shows from the start.
