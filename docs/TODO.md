# TODO

Loose ends and open work. The modernization plan closed on 2026-09-04 with all six phases
done. It is kept at [plans/completed/Modernization.md](plans/completed/Modernization.md) for
the reasoning behind each phase, and the items it closed around are collected here rather than
left in a finished plan.

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables bugprone, performance, misc and readability and subtracts
23 checks by name. The tree is clean at the 183 that are left. Seven of the subtractions are
settled rather than pending and are not listed here - the file says why. The rest are this
table: what the tree reports at that check, counted once per distinct site over a full
`-DV3D_CLANG_TIDY=ON` build. Removing a line means fixing what it reports, never widening the
exclusion. `voxel/src/noise` is not counted - it is vendored verbatim and is skipped by
clang-tidy, `/analyze` and cpplint alike.

| Check | Sites | Note |
|---|---|---|
| `readability-convert-member-functions-to-static` | 26 |  |
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

`api/grid` arrived on 2026-09-06 as [ADR-0029](adr/0029-tile-grids-are-an-api-library.md).
`odyssey` is its consumer as of the same day.

[] `LineOfSight` still has no consumer. Odyssey's map carries cover - a crate is `Cover::Half` and a wall is `Cover::Full` - and nothing asks what can be seen from where, because odyssey has nothing to see yet
[] odyssey's map format is its own and lives in `odyssey/tile/Map.cpp`. It earns a record and a home in the api the moment something other than that app reads or writes one, which a map editor or a generator would be
[] there is no world space filled primitive, so `Overlay.h` outlines a tile and cannot fill one. A filled highlight wants a third primitive beside the quad ([ADR-0005](adr/0005-one-batched-quad-primitive.md)) and the line ([ADR-0011](adr/0011-lines-are-the-second-primitive.md)), which is a decision rather than an addition
[] `TileFilter` is a `std::function` called for every neighbour of every visited tile, which is the first thing to templatise if a board is ever large enough to notice

## Models

`api/asset` reads glTF 2.0 into a `v3d::type::Model` as of 2026-09-06, which is the first geometry
anything in the tree loads from a file. No app uses it yet - `voxel` builds its terrain
procedurally and the editor models with `brep::BRep`.

[] `image::Reader` reads a file and nothing else, so a texture embedded in a `.glb` cannot be decoded and the loader reports it instead. A memory source is `png_set_read_fn` and `jpeg_mem_src`, plus the setjmp the png reader does not have today, which is why it is its own change rather than an overload
[] a `type::Model` has no path onto the device. `vulkan::Mesh` takes bytes, a stride-free count and indices, so the step is an app's four lines; a helper on the render side would need a vertex layout the api does not own
[] only the first material in a file is kept, because a merge is one draw. A file whose parts need different surfaces has to become several models, and nothing splits one yet
[] `.gltf` with external buffers resolves them relative to the file, which is cgltf's own behaviour rather than the asset manager's path handling. The two agree today because the manager hands over a full path

## Offscreen rendering

`Pass` gained a target on 2026-09-06 - [ADR-0031](adr/0031-a-pass-draws-into-a-target-it-names.md). No app
in the tree draws into one yet. It was built for the features that need it rather than for a picture
that exists today.

[] a depth target is allocated but never sampled. `RenderTarget` can carry a depth image and a pass writes it, but the image has no sampled usage and no view a descriptor set can bind, so a shadow map is written and cannot be read
[] a target is single-buffered, so a pass wanting the previous frame's contents needs two and has to swap them itself. A double-buffered target would be the natural next shape
[] nothing catches a pipeline built against one colour format drawing into a target of another. It is a wrong picture rather than a validation error, because dynamic rendering takes the format from the pipeline
[] `Frame::passBefore` exists because `Engine3D` creates the colour pass in its constructor. A frame that let a pass say where it belongs, or an engine that created its pass lazily, would not need it

## User interface

A component holds other components and asks for a box as of 2026-09-06 -
[ADR-0034](adr/0034-a-component-has-children-and-a-box.md) - and `ui::Immediate` is a second
way to write one, driven by calls rather than by a tree -
[ADR-0035](adr/0035-an-immediate-mode-layer-over-the-same-canvas.md). Nothing in the tree
uses either yet: the editor's menu bar and toolbars are strips the renderer places itself,
and the apps put up a menu and an overlay. Both were built for the huds, plates and debug
panels that need them.

[] nothing clips a child to its parent, and a box that overflows draws outside the one holding it. A scrollbar needs clipping before it needs anything else, which is why `Scrollbar.h` is still an empty declaration
[] `TabBar`, `TabPage`, `TextBox`, `CheckBox`, `RadioButton`, `Dialog`, `SelectList`, `Spinner`, `ToolTip`, `PopupMenu` and `RadialMenu` are still empty declarations with no loader and no draw path
[] `Frame` is an empty declaration that `Panel` now covers the drawing half of. Either it becomes the thing with a title bar that a dialog sits in, or it goes
[] nothing in the tree drives `ui::Immediate`, so the layer is covered by its cases and by nothing that draws. The editor's four viewports, voxel's chunk counts and odyssey's turn state are each a debug window waiting to be asked for
[] `Immediate` keeps no scroll, so a panel longer than its window runs off the bottom of it. This is the clipping item above, met from the other side
[] a widget in `Immediate` is hovered a frame after it is drawn, so the first frame of a window that appears under the cursor answers nothing
[] a percentage of a parent that has not been drawn is a percentage of zero, so the frame after a resize places a child against the previous size

## The game loop

The loop simulates at a fixed step as of 2026-09-06 —
[ADR-0032](adr/0032-the-loop-simulates-at-a-fixed-step.md), and
[plans/completed/GameLoopFoundations.md](plans/completed/GameLoopFoundations.md).

[] nothing reads `Engine::alpha()`. A renderer that interpolated between the last two simulation states would use it; until one does, the world is drawn snapped to the last completed step and motion is quantised to 60 Hz however fast the display is
[] only pong draws the frame statistics. `ui::StatisticsOverlay` is the api's, and tetris, voxel and vertical3d each already hold the `TextRenderer` it needs

## Ongoing workstreams

**Tests.** Every library needing neither a window nor a GPU is covered as of 2026-09-04. What
is left needs one: everything below the recorder in `api/render`, `Feature::Window`, and
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

[] 55 of the menu's 75 commands have no handler and log themselves
[] there is no modelling operation, so a component mode selects a face and then moves the whole object
[] one thing is selected at a time - no rubber band and no shift-click
[] there is no file chooser, no "save as" and no dirty flag
[] the viewport panes are not draggable
[] input capture for input-type menu items is unbuilt, so the five in `pong/data/vgui.json` are unreachable
