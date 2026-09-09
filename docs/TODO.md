# TODO

Loose ends and open work: what is missing or unfinished and is not covered by an open plan in
[plans/](plans/). An entry is deleted when it is done rather than marked, so everything here is
live.

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables bugprone, performance, misc and readability and subtracts
22 checks by name. The tree is clean at the 184 that are left. Seven of the subtractions are
settled rather than pending and are not listed here - the file says why. The rest are this
table: what the tree reports at that check, counted once per distinct site over a full
`-DV3D_CLANG_TIDY=ON` build. Removing a line means fixing what it reports, never widening the
exclusion. `voxel/src/noise` is not counted - it is vendored verbatim and is skipped by
clang-tidy, `/analyze` and cpplint alike.

| Check | Sites | Note |
|---|---|---|
| `performance-unnecessary-value-param` | 31 | the fix is a const reference, not the by-value-and-move the check suggests |
| `bugprone-derived-method-shadowing-base-method` | 7 | `size()` on a strip and on a component mean different things, and a toolbar holds buttons where a component holds components |
| `readability-implicit-bool-conversion` | 69 |  |
| `bugprone-narrowing-conversions` | 111 |  |
| `readability-braces-around-statements` | 111 |  |
| `readability-math-missing-parentheses` | 131 |  |
| `bugprone-easily-swappable-parameters` | 233 |  |
| `performance-enum-size` | 303 |  |
| `misc-use-internal-linkage` | 526 |  |
| `misc-const-correctness` | 939 |  |
| `misc-non-private-member-variables-in-classes` | 1303 |  |
| `readability-magic-numbers` | 1883 |  |
| `readability-identifier-length` | 2483 |  |
| `misc-include-cleaner` | 3346 |  |
| `readability-uppercase-literal-suffix` | 4156 |  |

## Tile grids

`api/grid` is a library of its own - [ADR-0029](adr/0029-tile-grids-are-an-api-library.md) - and
`odyssey` is its only consumer.

[] `LineOfSight` still has no consumer. Odyssey's map carries cover - a crate is `Cover::Half` and a wall is `Cover::Full` - and nothing asks what can be seen from where, because odyssey has nothing to see yet
[] odyssey's map format is its own and lives in `odyssey/tile/Map.cpp`. It earns a record and a home in the api the moment something other than that app reads or writes one, which a map editor or a generator would be
[] `TileFilter` is a `std::function` called for every neighbour of every visited tile, which is the first thing to templatise if a board is ever large enough to notice

## Models

`api/asset` reads glTF 2.0 into a `v3d::type::Model`, which is the only geometry anything in the
tree loads from a file. No app uses it: `voxel` builds its terrain procedurally and the editor
models with `brep::BRep`.

[] a `type::Model` has no path onto the device. `vulkan::Mesh` takes bytes, a stride-free count and indices, so the step is an app's four lines; a helper on the render side would need a vertex layout the api does not own
[] only the first material in a file is kept, because a merge is one draw. A file whose parts need different surfaces has to become several models, and nothing splits one yet
[] `.gltf` with external buffers resolves them relative to the file, which is cgltf's own behaviour rather than the asset manager's path handling. The two agree today because the manager hands over a full path

## Offscreen rendering

A pass draws into a target it names -
[ADR-0031](adr/0031-a-pass-draws-into-a-target-it-names.md). No app in the tree draws into one:
it is there for the features that need it rather than for a picture that exists today.

[] a target is single-buffered, so a pass wanting the previous frame's contents needs two and has to swap them itself. A double-buffered target would be the natural next shape
[] nothing catches a pipeline built against one colour format drawing into a target of another. It is a wrong picture rather than a validation error, because dynamic rendering takes the format from the pipeline
[] `Frame::passBefore` exists because `Engine3D` creates the colour pass in its constructor. A frame that let a pass say where it belongs, or an engine that created its pass lazily, would not need it

## User interface

`api/ui` is two ways to write a ui - a tree of components
([ADR-0034](adr/0034-a-component-has-children-and-a-box.md)) and a layer of calls
([ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md)) - and
[UserInterface.md](UserInterface.md) is what owns it. Nothing in the tree uses the component
tree: the editor's menu bar and toolbars are strips the renderer places itself, and the apps put
up a menu and an overlay.

One of the entries below was weighed and declined rather than left undone: a widget being
hovered a frame late is the mechanism that lets a window take the cursor from one under it.

[] voxel's F3 readout is the only thing driving `ui::Immediate`. The editor's four viewports and odyssey's turn state are each a debug window waiting to be asked for, and a game that owns the mouse has no cursor to give the layer, so voxel's window cannot be folded or scrolled
[] a widget in `Immediate` is hovered a frame after it is drawn, so the first frame of a window that appears under the cursor answers nothing
[] adding a component still means editing seven places - `component::Type`, `component::name`, `ui::Loader`'s branch, `ComponentRenderer::paint`, `Arranger::natural`, `ui::Cursor`'s switch and `ui::Keys`'s - plus `style::Resolver`'s class when it is dressed by one of its own. The compiler now names all seven ([ADR-0047](adr/0047-a-component-type-is-checked-by-the-compiler.md)), so an omission is a build error rather than a component that silently is not there, but the count is unchanged. A registry is the only thing that would reduce it, and it was weighed and left: `paint()` and `natural()` read the renderer's and the arranger's own state, so a table of free functions would make two private members public. `style::Resolver::Class` is a second enum and is not checked against `Type`
[] a clip is a scissor, so it is axis aligned and square: a panel with rounded corners clips to the box and not to the curve
[] a caret cannot be placed by clicking: a press focuses a text box and leaves the caret where it was. `ui::Cursor` names no text, so finding the character under a point would mean giving it the `Measure` callback - a change to what a cursor is rather than an addition to it
[] there is no selection in a text box, so no cut, copy or paste over a range. `TextBox::insert()` takes a run of characters, so a paste is expressible the moment something delivers one
[] nothing in the tree constructs a `ui::Keys`, so the keyboard router has no consumer: the editor builds a `ui::Cursor` and no app builds either a `Keys` or calls `Engine::focusFirst()`. Every case for it is in `api/ui/tests`, which is a library proven and an app seam unbuilt
[] a scrollbar takes no key, so it is the one control that still needs a mouse. Every other one is driven from the keyboard per [ADR-0040](adr/0040-a-key-goes-to-a-focused-component.md), and paging the thing a bar scrolls is still the app's rather than the bar's
[] a focus ring is drawn from the base dressing, so a theme cannot ring a button differently from a list. `focus` and `focus-width` are chrome properties with no per class override, which is what one ring for every control buys
[] `SDL_StartTextInput` is on for the life of the window rather than for as long as something is focused, which is free on a desktop and would raise an on screen keyboard and never lower it anywhere else
[] nothing draws into a `LineCanvas` clip ([ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md)). The viewport panes that would want one are the editor's

## The game loop

The loop simulates at a fixed step and renders at a variable one -
[ADR-0032](adr/0032-the-loop-simulates-at-a-fixed-step.md).

[] nothing reads `Engine::alpha()`. A renderer that interpolated between the last two simulation states would use it; until one does, the world is drawn snapped to the last completed step and motion is quantised to 60 Hz however fast the display is
[] only pong draws the frame statistics. `ui::StatisticsOverlay` is the api's, and tetris, voxel and vertical3d each already hold the `TextRenderer` it needs

## Ongoing workstreams

**Tests.** Every library needing neither a window nor a GPU is covered. What is left needs one:
everything below the recorder in `api/render`, `Feature::Window`, and
`audio::Engine::initialize()` — all of it waiting on
[ADR-0007](adr/0007-ci-rendering-tests.md).

**Documentation.** Reference material lives in this directory, one document per subject and
[README.md](README.md) as the index; `CLAUDE.md` routes into them rather than holding a copy.
Two gaps are left. The rationale for the Vulkan move and for the SDL3 upgrade is recorded
nowhere — [ADR-0001](adr/0001-vulkan-replaces-opengl.md) records the decision, not the
reasoning behind it. And [ECSDesign.md](ECSDesign.md) is the one reference document that is
still a set of notes rather than a description of the tree; what a renderable component looks
like is the live question in it, and
[RenderingPipeline.md](RenderingPipeline.md#still-open-how-this-meets-the-ecs) states it.

## Editor

Open work, for when the app is what moves forward rather than the platform.

[] 55 of the menu's 76 commands have no handler and log themselves
[] there is no modelling operation, so a component mode selects a face and then moves the whole object
[] one thing is selected at a time - no rubber band and no shift-click
[] there is no file chooser, no "save as" and no dirty flag
[] the viewport panes are not draggable
