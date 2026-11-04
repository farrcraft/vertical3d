# Editor Design Sync — The Editor and Its Mockups Brought Into Agreement

Drafted 2026-11-03 against `c3bc40a9`. The editor was built to a set of mockups, the combined
model of tiled areas with tabs where an area holds several editors. Since then the code and the
mockups have moved apart. Some differences are gaps in the code. Others are choices the code
made on purpose that the mockups never caught up with. This plan settles each difference one way
or the other. When it closes, each state of the editor matches its mockup, and each mockup
shows what the editor does.

The mockups are the "Editor Pane Models" design canvas. Its combined-model boards and its shared
parts are the target. Its areas-only and docks boards are options that were not chosen, and are
not changed.

The editor is described in [editor/](../editor/README.md), and `api/ui` in
[api/ui/](../api/ui/README.md) and [internals/ui/](../internals/ui/README.md). A step with a
testable CPU half brings test cases with it. A step that changes how something looks is checked
by running the editor and reading a screenshot, as [Testing.md](../contributing/Testing.md)
describes.

## What the survey found

The editor was built at `c3bc40a9` and photographed in ten states. They are startup, a cube
created, the command search, a divider under the cursor, Front maximised, each of the five
workspaces, and Modelling again after a switch. Each picture was set beside the mockup of the
same state, and colours were read from the pixels. The pictures and a note on each are on the
canvas.

**What matches:**

- the text size, the editor's mark, and the viewport header's menus and Overlays button
- the axis colours and the amber selection
- the outliner's icons and Scene row, and the units and axis letters in Properties
- the notice's mark and undo shortcut, and the status bar's mouse buttons and chips
- the timeline, graph editor and console as tabs, and the divider's hint

## Decisions

- **Where the code made a choice on purpose, the mockup changes.** The editor finish work
  recorded these choices and the reasons for them:
  - The current view's name sits on a pill in the control colour.
  - Properties has no Shading tab, because an item has no shading of its own.
  - Rendering puts the render view beside the render settings as two areas.
- **A chosen tab is marked as a place, not as a setting that is on.** Decided 2026-11-03. The
  accent fill marks a checked toggle and nothing else. A pill of choices, the workspaces or
  the Properties groups, marks the chosen one in the control colour and semibold. A tab strip
  marks its current tab with a 2 pixel accent underline on the panel colour. Blender's theme
  keeps the same split: a chosen tab is a lighter neutral, and only a checked toggle or radio
  button is blue. This replaces the editor finish rule that a chosen workspace keeps the
  accent fill. The rule binds every app that draws tabs, so step 1 records it in an ADR.
- **An item is shown in the views, and shown in renders, by two flags of its own.** Decided
  2026-11-03. Both are on by default, and a child is hidden wherever its parent is. Blender,
  Houdini, USD and glTF all keep what the views show apart from what is rendered, and the files
  the editor reads carry both. Step 5 adds the flags and records them in an ADR.
- **The timeline's transport controls are icons, each with a tooltip.** Decided 2026-11-03.
  Blender, Maya and Houdini all draw them as icons. Their controls also grow past five, which in
  words would crowd the frame and range fields out of a narrow area.
- **The Animation workspace shows a working view beside the view through the scene's camera.**
  Decided 2026-11-03. Animation is judged through the camera that renders it, and a quad above
  the timeline leaves each view too small to judge motion in. Maya, Houdini and Blender each let
  a viewport look through a scene camera, and step 7 adds it.
- **A tabbed area's tab strip holds what acts on the area, and the row under it what acts on the
  editor.** Decided 2026-11-03. The strip holds the area menu, the tabs, a + that adds an
  editor, and maximise. The row under it shows only for an editor with controls of its own. An
  area with one editor keeps its header as it is. Houdini's panes split their controls the same
  way. The mockup's single row would lose Split, Join, Swap and Close Tab, and has no place for
  a tabbed viewport's menus.
- **Where no choice was recorded, the code changes to match the mockup.**
- **Facts the mockup got wrong change in the mockup.** The editor is Y-up, so Front shows X and
  Y, Top shows X and Z, and Left shows Z and Y. The mockups were drawn Z-up. The status bar's
  actions are the editor's bindings, Dolly, Truck and Orbit. There are five workspaces, and the
  F3 command search has no mockup.

## What blocks what

- Step 1 comes before steps 2, 3, 4, 6 and 8. Steps 2, 3 and 4 draw chosen tabs through its
  new role. Steps 4, 6 and 8 add or label combos: the Parent row, the menu of rates and the
  hider.
- Step 5 comes before step 4's eye and check, which edit its flags. The rest of step 4 does not
  wait for it.
- Steps 2 to 8 are otherwise independent of each other.
- Step 9 can run beside any of them, since each difference it redraws is settled.
- Step 10 comes last, because it checks every other step.

## Steps

### Step 1 — A chosen tab has a look of its own, and a combo shows its label

A chosen tab is drawn as a pressed widget. `Immediate::tab()` fills it with the pressed face,
and the editor builds an area's tabs as toggle buttons. Both fill with the accent's highlight,
as a checked toggle does.

- The theme gains the roles a chosen tab is drawn with: the control colour for a chosen pill,
  and an accent underline on the panel colour for a chosen tab in a strip.
  `Immediate::tab()` and `TabBar` draw through them, and the editor's area tabs become tabs
  rather than toggles.
- An ADR records the rule in the decision above, with the toggle-alike rule it replaces as the
  rejected option. [Themes.md](../api/ui/Themes.md) states the roles.

`Immediate::combo()` uses its label only as an id. It draws the chosen item, and a filled square
where the mark that opens the list belongs. A combo then reads as a ticked checkbox: the Scene's
unit shows "Metres" and a square, and the render settings' hider shows "Reyes" and a square.

- The combo draws its label, as `checkbox()` and the sliders do, and a chevron for its mark.
- The immediate suite covers the label's placement and the mark, and each tab's chosen look.
- [ImmediateMode.md](../api/ui/ImmediateMode.md) states what a combo draws.

### Step 2 — The top bar and the tool shelf

- The chosen workspace is in the control colour and semibold, on a filled pill, in place of
  the accent fill in an outlined box.
- Search is a button with a magnifier, "Search commands" and an F3 chip. Today it is the word
  "Search".
- An unsaved project shows an amber dot after its name. Today it shows an asterisk. The window's
  caption keeps the asterisk, because a caption cannot be coloured.
- The tool shelf has a rule between the transform tools and the modelling operations.

### Step 3 — Area headers and tab strips

- A panel header names its editor after the editor-type button: Outliner, Properties.
- A maximised area shows the restore icon in place of the maximise icon, and the hint
  "Ctrl+Space restores the quad" in its header.
- The current tab in a tab strip has the accent underline on the panel colour, in place of
  the accent fill.
- An area with two or more editors draws one strip: the area menu, named for the current
  tab's editor, then the tabs, a + and maximise. The area menu holds Editor, which changes the
  current tab's kind, then Split, Join, Swap and Close Tab. The + opens a list of the kinds of
  editor and adds a tab of the one chosen.
- Under the strip, a row shows only for an editor with controls of its own. Today that is a
  viewport, with its camera, shading, mask and Overlays menus. A timeline, graph editor or
  console under a strip has no header row.
- An area with one editor keeps its header as it is.
- The area suite covers which header buttons each kind of area has, tabbed and untabbed.
- [editor/](../editor/README.md) states what an area's strip and header hold.

### Step 4 — The outliner and Properties

- The outliner's filter field leads with a magnifier.
- The current Properties tab is in the control colour and semibold, on a filled pill, in place
  of the accent fill.
- Each Properties field has a 2 pixel left edge in its axis colour.
- Properties groups its rows under Transform and Item headings. The Item group has a Name row,
  where the editor now labels the field with the item's kind, and a Parent row, a combo of the
  items that can be its parent. The scene already holds the hierarchy.
- Each outliner row ends in a visibility eye, and the Item group has a "Shown in renders" check.
  Both edit step 5's flags.

### Step 5 — An item can be hidden, and left out of renders

No item carries visibility. The importers drop the visibility a USD or glTF file holds, and RIB
export writes every item.

- An item carries two flags, shown in the views and shown in renders, both on by default. A
  child is hidden in the views, or left out of renders, wherever its parent is.
- An item hidden in the views is not drawn there and cannot be picked. The outliner still lists
  it, dimmed, with its eye closed.
- RIB export leaves out an item that is not shown in renders.
- The USD importer reads `visibility` and `purpose`. An invisible prim is hidden in both. A
  guide or proxy prim is shown in the views and left out of renders. The glTF importer reads
  `KHR_node_visibility`.
- Commands hide the selection, show every hidden item, and isolate the selection, each with a
  key and undo. H shows handles today, so the step chooses the keys.
- The scene store writes both flags, and its version moves.
- An ADR records the two flags, their inheritance and what export does with each. The options
  it weighs include RenderMan's split of render visibility into camera, indirect and
  transmission. [Scene.md](../editor/Scene.md) states the rules.
- The item, store, importer, export, picking and command suites cover the flags.

### Step 6 — The timeline's controls

- The frame, the range and the rate sit in one compact row, as the mockup sets them: a frame
  field, a pair of fields for the range, and a menu of rates. Today they are four wide fields.
- `api/ui`: `Immediate` gains an icon button, drawn from an SVG icon, with a tooltip.
  [ImmediateMode.md](../api/ui/ImmediateMode.md) states it, and the immediate suite covers it.
- The transport draws five icons through it: first frame, previous key, play, next key and last
  frame. The key buttons carry a key diamond. Play turns to Stop while the scene plays. Each
  tooltip names the action and its key.

### Step 7 — A view through a scene camera, and the Animation workspace

A viewport shows only the fixed views, Front to Perspective. Nothing shows what a scene camera
frames until it renders.

- A viewport header's camera menu lists the scene's cameras after the fixed views. Choosing one
  makes the view look through it, framed to the render's aspect, with the area outside the
  frame dimmed. The View menu does the same for the active view.
- Navigating a view that looks through a camera turns it back into a free perspective view at
  the same place. A lock in the camera menu makes navigation move the camera instead, as an
  edit with undo. An ADR records the choice between the two.
- A layout's viewport may name `scene-camera`: the selected camera, else the first in the
  scene, else a free perspective view until a camera exists.
- Animation's views are a free perspective view and a `scene-camera` view, side by side above
  the timeline area.
- The viewport, camera, command and area layout suites cover the menu, the framing, the drop
  back to a free view, the lock and the layout.
- [Scene.md](../editor/Scene.md) states how a view looks through a camera.

### Step 8 — The render settings

- The render settings' fields carry labels through step 1's combo and the existing fields.
- The mockup lists a frame range, which the render settings do not hold. The scene's range is
  set in the timeline, and a batch renders every frame of it. Step 9 takes the range out of the
  mockup's render settings.

### Step 9 — The mockups

Each difference settled as "the mockup changes" is redrawn on the canvas:

- Axes Y-up in every view, the axis marker and the gizmo.
- Five workspaces.
- The current view's name pill in the control colour.
- Properties without a Shading tab.
- Rendering as two areas, and its render settings without a frame range.
- The status bar's actions as the editor's bindings.
- The divider's hint in the editor's words.
- A board for the F3 command search.
- Animation with its two views.
- A tabbed area's strip with the area menu, the tabs, the + and maximise, and a tabbed
  viewport with its menus on the row below.

The canvas's summary note is rewritten to list what this plan left different, which should be
nothing.

### Step 10 — The closing survey

The editor is built and photographed in the same ten states as the first survey, and in each
state a step above changed. Each picture replaces the one on the canvas, beside its mockup. Any
difference still found is fixed in this plan or recorded in [TODO.md](../TODO.md) before the
plan closes.
