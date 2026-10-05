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
([ADR-0027](adr/0027-build-consume-the-api-as-source.md)) and most of what consumes it is not in
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

[] RI states its rotations in a left handed system and moya hands the angle straight to
   `glm::rotate`, which is counter-clockwise by the right hand rule. Nothing in the tree can
   tell the difference: both hiders read the same transformation, so they agree with each other
   whichever reading is right, so a reference picture agreeing with itself says nothing. A light placed by a
   rotation was expected to make it visible and did not. What would settle it is a scene whose
   correct picture is known from outside this tree

## Offline rendering

Held by [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration),
each until a scene asks for it.

[] area lights render as point lights. Sampling a light's area is the film's lens disc in another place, but a RenderMan area light runs its light shader at points on a primitive, and `AreaLightSource` binds a shader to geometry neither hider keeps as a light. It is due with a scene that wants soft shadows, which both hiders can now cast through the shared tracer

[] displacement shaders are refused and `calculatenormal` is a stub, so neither displacement nor bump is possible. Displacement reaches back into moya's dicing, moves a grid after it is shaded and needs a bound grown by `displacementbound`; under the ray hider it needs a tessellation a ray tracer does not otherwise do. Bump needs derivatives across a batch, and a traced hit is a batch of one. It is due with a scene that needs surface detail a texture cannot give

[] the reyes hider ignores `Oi`: a sample keeps the nearest surface and forces its opacity to one, so a translucent shader is opaque under `"hidden"` and translucent under `"raytrace"`. Honouring it needs a sample to keep every surface it meets and composite them front to back once the bucket is done - the ray hider's `see()` loop, over a sample's list rather than a ray's hits. It is due with a scene that wants glass or smoke drawn by the reyes hider

[] `offline::trace::Scene::nearest` tests every primitive. Since moya traces too, every shadow ray from every grid point pays for the whole scene. An acceleration structure goes inside `offline::trace` and neither renderer changes; it is due with a scene that takes a second to render, which the suites' times would show

## Tile grids

`api/grid` is a library of its own - [ADR-0029](adr/0029-grid-8-way-movement-symmetric-line-of-sight.md) - and
`odyssey` is what consumes it here. A map's picture and terrain legend are the grid's, and the
rest of a map is the game's - [ADR-0062](adr/0062-grid-parse-terrain-not-map-files.md).

[] a `TileGrid` is one rectangle centred on the world origin, so a world made of regions, each offset in the world and each loaded and released with its sheets, has no way to place a grid. Whether a region is a grid with an origin, a grid of grids or a game's list of grids is for the first consumer with regions to say; a world origin on `TileGrid` is the likeliest answer, and it changes every world and tile conversion - [LargeWorlds](plans/completed/LargeWorlds.md#step-7--regions) has the reasoning

[] remembered sight is odyssey's `tile/Sight.h`, though nothing about fog of war is odyssey's own. It moves to `api/grid` when a second consumer wants ground it has seen to stay revealed - [LargeWorlds](plans/completed/LargeWorlds.md#step-8--remembered-sight)

[] `TileFilter` is a `std::function` called for every neighbour of every visited tile, which is the first thing to templatise if a board is ever large enough to notice. A region-sized board is the likeliest first, so it waits behind regions - [LargeWorlds](plans/completed/LargeWorlds.md#step-9--the-movement-filter-as-a-template)

## Voxel

[] a remeshed chunk destroys its old mesh with the last reference to it, which is safe only because `memory::Uploader` idles the queue after every copy, as `ChunkMeshPool.h` says. Once uploads stop idling the queue - [milestone 7](roadmap/completed/m7-ShellAndShipping.md#asynchronous-loading) - the old mesh has to be retired through `frame::Ring::retire` ([ADR-0061](adr/0061-resources-explicit-release-generational-handles.md)) instead

[] mouselook turns far too fast and rolls the view: ten pixels of horizontal motion turns it most of the way round and tips it over. It reads one pixel as one unit of `Player::look()`'s heading and pitch, and that scale predates relative mouse mode - [ShellAndShipping](plans/completed/ShellAndShipping.md#step-3--a-relative-mouse)

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
editor models with `brep::BRep` - so the gap below is what the library's own tests reach
rather than what an app here has hit. `realtime::MeshRegistry` takes one onto the device
([LitScene](plans/completed/LitScene.md#step-6--a-model-onto-the-device)). A file is one model in
parts, a part per material ([ADR-0069](adr/0069-models-material-parts-over-one-vertex-buffer.md)).

[] `.gltf` with external buffers resolves them relative to the file, which is cgltf's own behaviour rather than the asset manager's path handling. The two agree today because the manager hands over a full path

## Lit scenes

[] a shadow map is fitted to its casters once, by `shadow::fit`, and covers nothing that walks out of that sphere. A world larger than one look-dev scene needs the fit to follow the camera, and past that cascades, which the roadmap left for after the move - [LitScene](plans/completed/LitScene.md#step-8--a-shadow-map)

[] the identity grade and the inverting grade were exact on the Radeon and assert a step at most, because lavapipe had not been seen yet. If CI shows lavapipe exact too, the tolerance in `PostTest.cpp` comes down to zero; if it does not, the step is the reason, and that goes beside the case - [LitScene](plans/completed/LitScene.md#step-10--the-chain-after-the-scene)

[] nothing draws many of one mesh in one draw. `meshes()` and `casters()` submit an item per part per entity, which at retcon's twelve characters and a few dozen props is a few hundred draws. Instancing would move `Lit::Object` from the push block into a per-frame storage buffer read by `gl_InstanceIndex`, beside the palette at set 2, and group the walk by entry and part; the recorder already draws instances. It is due when a count asks for it - retcon's horde density, a township's population, or a profile showing recording time - [SkeletalAnimation](plans/completed/SkeletalAnimation.md#step-8--instancing-held)

## Loading

[] every load is on the main thread, and is a hitch the size of the load. The first piece is decoding on a worker through an `asset::Manager` of its own, since the manager's loaders are shared and stateful, with the main thread polling once a frame and uploading what is ready through `TextureFactory::create(image)` and `MeshRegistry::add(name, model, albedos)`. Before any thread, a texture a glTF names has to be decoded on the load side: `MeshRegistry::acquire` decodes it inside the upload today. It would be the first thread in `api/`, which is a record of its own. It is due with cozy's M6 region streaming, or any load a player can see as a hitch - [ShellAndShipping](plans/completed/ShellAndShipping.md#step-12--asynchronous-loading-held)

## Frames

[] a minimised window spins. `Engine3D::beginFrame` presents an empty frame when the window has no area, `Swapchain::create` is asked for a chain again on every one and logs `Window has no area` at info each time, and nothing in the loop waits. Two seconds minimised wrote twelve thousand lines to pong's log. The loop could wait on an event while the window is minimised, or the swapchain could log the change of state rather than every frame of it - [ShellAndShipping](plans/completed/ShellAndShipping.md#step-6--one-screen-and-four-apps-on-it)

## Audio

`audio::Engine` plays a voice on a named bus with a fade and a gain, and nothing places it.

[] a voice has no pan. A footstep on the left of a fixed camera's screen wants one, and a position becomes a pan in the caller, since a flat world under a fixed camera makes that mapping the game's: `Play` would gain a pan and the engine `pan(voice, value)`, through whatever SDL_mixer 3 offers a track. It is due when cozy asks, since retcon pans through FMOD - [Effects](plans/completed/Effects.md#step-10--a-panned-voice-held)

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
[ADR-0007](adr/0007-ci-render-tests-on-software-vulkan.md), but a software Vulkan implementation answers none of
them, so they outlive it.

What a picture cannot cover outlives that plan too, by
[ADR-0054](adr/0054-testing-golden-images-hold-only-spec-exact-output.md): a reference holds
only what the specification determines, so blending, filtered sampling, multisampling and text
are asserted by validation silence and spot checks and by nothing stronger. Widening that needs
a second implementation to compare against rather than a second rule, and there is none in this
tree.

**Documentation.** Reference material lives in this directory, one document per subject and
[README.md](README.md) as the index; `CLAUDE.md` routes into them rather than holding a copy.
One gap is left. The rationale for the Vulkan move and for the SDL3 upgrade is recorded
nowhere — [ADR-0001](adr/0001-rendering-replace-opengl-with-vulkan.md) records the decision, not the
reasoning behind it.

## Editor

Open work, for when the app is what moves forward rather than the platform.

[] 55 of the menu's 76 commands have no handler and log themselves
[] there is no modelling operation, so a component mode selects a face and then moves the whole object
[] one thing is selected at a time - no rubber band and no shift-click
[] there is no dirty flag, so nothing warns before a load or a quit loses unsaved work
[] the viewport panes are not draggable
