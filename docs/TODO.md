# TODO

Loose ends and open work: what is missing or unfinished and is not covered by an open plan in
[plans/](plans/). An entry is deleted when it is done rather than marked, so everything here is
live.

**Missing or unfinished is the whole test, and it is narrower than it reads.** A feature that is
complete and that nothing here happens to call is not missing anything, and a choice that was
weighed and rejected is not unfinished: the first belongs in the document that owns the subject
and the second in the ADR that settled it. Neither is work, and a list carrying them is one
where nothing on it is actually due.

An entry states the gap in the code, not who is waiting on it. The api is consumed as source
([ADR-0027](adr/0027-the-api-is-consumed-as-source.md)) and most of what consumes it is not in
this tree - cozy and retcon are both apps on it in another repository - so what the apps here
happen to use is evidence about this tree and nothing else. Where an entry names a consumer it
is because the usage explains the gap, and "nothing in this tree" is the strongest claim any of
them can make.

## The clang-tidy backlog

[.clang-tidy](../.clang-tidy) enables bugprone, performance, misc and readability and subtracts
20 checks by name. The tree is clean at the 186 that are left. Seven of the subtractions are
settled rather than pending and are not listed here - the file says why. The rest are this
table: what the tree reports at that check, counted once per distinct site over a full
`-DV3D_CLANG_TIDY=ON` build. Removing a line means fixing what it reports, never widening the
exclusion. `voxel/src/noise` is not counted - it is vendored verbatim and is skipped by
clang-tidy, `/analyze` and cpplint alike.

| Check | Sites | Note |
|---|---|---|
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

## RiRotate's sign

Carried out of [OfflineRenderingPhase3](plans/completed/OfflineRenderingPhase3.md), which named
it as an open question and could not settle it.

[] RI states its rotations in a left handed system and both offline renderers hand the angle
   straight to `glm::rotate`, which is counter-clockwise by the right hand rule. Nothing in the
   tree can tell the difference: the two renderers agree with each other whichever of them is
   right, so a reference picture agreeing with itself says nothing. A light placed by a
   rotation was expected to make it visible and did not. What would settle it is a scene whose
   correct picture is known from outside this tree

## Tile grids

`api/grid` is a library of its own - [ADR-0029](adr/0029-tile-grids-are-an-api-library.md) - and
`odyssey` is what consumes it here.

[] nothing in the api reads or writes a map. The one format that exists is odyssey's own, in `odyssey/tile/Map.cpp`, so anything wanting to load a grid - a map editor, a generator, a second game - writes its own loader or lifts that file. A format in the api earns a record when a second consumer reads one
[] `TileFilter` is a `std::function` called for every neighbour of every visited tile, which is the first thing to templatise if a board is ever large enough to notice

## Sprite sheets

`image::TextureAtlas` places regions, `config::SpriteSheets` reads and writes the document that
names them, and `image::crop` cuts one back out of a sheet. Every half of a packer's round trip
is in the tree.

[] nothing in the tree packs a sheet, and unpacking one is a rectangle at a time. `imagetool
--crop` cuts one region, so a sheet can be exploded by a caller that already knows where its
sprites are; nothing reads a `sprites.json` and cuts out everything it names. Whether that
belongs to `imagetool`, to a `spritetool` beside it, or to whoever needs it is undecided

## Models

`api/asset` reads glTF 2.0 into a `v3d::type::Model`, which is the only geometry the api loads
from a file. Nothing in this tree loads one - `voxel` builds its terrain procedurally and the
editor models with `brep::BRep` - so the gaps below are what the library's own tests reach
rather than what an app here has hit.

[] a `type::Model` has no path onto the device. `vulkan::Mesh` takes bytes, a stride-free count and indices, so the step is an app's four lines; a helper on the render side would need a vertex layout the api does not own
[] only the first material in a file is kept, because a merge is one draw. A file whose parts need different surfaces has to become several models, and nothing splits one yet
[] `.gltf` with external buffers resolves them relative to the file, which is cgltf's own behaviour rather than the asset manager's path handling. The two agree today because the manager hands over a full path

## Offscreen rendering

A pass draws into a target it names -
[ADR-0031](adr/0031-a-pass-draws-into-a-target-it-names.md). Nothing in this tree draws into
one, so each gap below is one a consumer meets before this tree does.

[] a target is single-buffered, so a pass wanting the previous frame's contents needs two and has to swap them itself. A double-buffered target would be the natural next shape
[] nothing catches a pipeline built against one colour format drawing into a target of another. It is a wrong picture rather than a validation error, because dynamic rendering takes the format from the pipeline
[] `Frame::passBefore` exists because `Engine3D` creates the colour pass in its constructor. A frame that let a pass say where it belongs, or an engine that created its pass lazily, would not need it

## The game loop

The loop simulates at a fixed step and renders at a variable one -
[ADR-0032](adr/0032-the-loop-simulates-at-a-fixed-step.md).

[] the api interpolates nothing. `Engine::alpha()` is the fraction a renderer would blend the last two simulation states by, and nothing in this tree reads it, so a world drawn here is snapped to the last completed step and its motion is quantised to 60 Hz however fast the display is

## Ongoing workstreams

**Tests.** Every library needing neither a window nor a GPU is covered. The GPU half —
everything below the recorder in `api/render` — now has a suite that draws:
`v3dtest_render_device` runs against lavapipe on the runner, which
[RenderTestsInCI](plans/completed/RenderTestsInCI.md) built and closed, and four of its cases
are pinned to committed pictures by
[RealtimeGoldenImage](plans/completed/RealtimeGoldenImage.md). The pipeline cache is what is
left there: nothing that draws asserts it, and what would is a count of what was compiled
rather than a picture.

What the plan left is what needs a window or a sound device rather than a device to draw
with: `Feature::Window`, `ui::TextRenderer` and `audio::Engine::initialize()`. They are named
beside `api/render` in [Testing.md](Testing.md) and were waiting on the same
[ADR-0007](adr/0007-ci-rendering-tests.md), but a software Vulkan implementation answers none of
them, so they outlive it.

What a picture cannot cover outlives that plan too, by
[ADR-0054](adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md): a reference holds
only what the specification determines, so blending, filtered sampling, multisampling and text
are asserted by validation silence and spot checks and by nothing stronger. Widening that needs
a second implementation to compare against rather than a second rule, and there is none in this
tree.

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
