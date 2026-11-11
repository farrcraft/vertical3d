# TODO

This file lists open work that no open plan in [plans/](plans/) covers. It is for anyone looking
for something to pick up.

An entry is something **missing or unfinished** in the code. When an entry is done, delete it.
Do not tick it or mark it done. Everything here is open.

Two kinds of item do not belong here:

- **A finished feature that nothing calls yet.** It is not missing anything. Describe it in the
  document that owns the subject.
- **An option that was considered and rejected.** It is not unfinished. Record it in the ADR
  that made the decision.

Each entry states the gap in the code, not who is waiting for it. Every entry is ready to pick
up. None waits for a game, a scene or a second caller to need it first.

An open plan or roadmap may order entries from here. An entry it orders moves into it and leaves
this file, and the section the entry left names the plan or roadmap that now holds it.

## Tests

[Testing.md](contributing/Testing.md) describes what the suites cover.

[] **Two things need a window or a sound card to test:** `Feature::Window`, and whether
`audio::Player` is audible. SDL's dummy audio driver tests what the mixer is told, not what is
heard. A software Vulkan driver does not help with either.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. Blending, filtered sampling, multisampling
and text are therefore checked only by a silent validation log and by spot checks. Checking them
more strongly needs a second Vulkan implementation to compare against, and this tree has none.

## Known review findings

Defects a review found that the change under review did not introduce.
[Review.md](contributing/Review.md) says how this list is used: a reviewer reads it first and
does not report an entry again. Each entry names its class.

[] **Leak: a started FMOD event leaves 72 bytes behind.** In a build with `V3D_FMOD_ROOT`,
`fmod_events_test/the_fmod_backend_stops_an_event_it_started` run alone ends with the debug
heap reporting one 72 byte block. The other FMOD cases, each run alone, report none. The block
is allocated on the CRT heap, not FMOD's, so it is something `FmodEvents` or the case allocates
around `start()` and `stop()`.

[] **Validation: the editor and voxel leave a surface when they quit.** Closing either app, or
ending it with `--capture`, prints `VUID-vkDestroyInstance-instance-00629` with one
`VkSurfaceKHR` not destroyed. The layer writes it to standard output as the instance is
destroyed, after the log is closed, so `v3d.log` shows nothing. Pong, tetris, odyssey and
diorama destroy their surface.

[] **Semantics: a uniform parameter on a mesh is read as one value.** `rib::Declaration::elements`
gives one element to a uniform parameter whatever the primitive. RenderMan gives a uniform
parameter one value per face of a polygon mesh or subdivision mesh, so a uniform array passed to
`RiPointsPolygons` with several faces is read as its first value.

[] **Drift: a class's definitions spread over files named for something else.** Conventions.md
"Size" asks for a class to be split rather than spread. `Eraser` is defined in
`api/script/internal/EraserExpressions.cxx` and `EraserTypes.cxx`, `ui::Immediate` in the
`api/ui/Immediate*.cpp` files, `sl::runtime::Machine` in `sl/runtime/Library.cxx`, and the
editor's controller and area layout in `vertical3d/src/ControllerWindows.cxx` and
`view/AreaLayout.cxx`. Some may be nested classes, which the rule allows.

## Offline rendering

Open work on moya. [offline/](offline/README.md) describes it.

[] **RiObjectBegin names no object.** RI's C interface returns no handle from `RiObjectBegin`,
and `RiObjectEnd` and `RiObjectInstance` reach no handler, so a program that calls RI directly
cannot instance an object. RIB's `ObjectBegin` reaches the handler with a number or a string.
RI's handle is a pointer, so the C interface needs a table from the handles it gives out to the
handler's names.

[] **The new core's CPU traversal is several times slower than Embree's.** On a million
triangles `moya_bench` traces 11 to 12.5 million rays a second. Embree's paper gives 38 to 59
million on an older processor with half the cores. The build with spatial splits is five to
seven times slower. Most of a ray's cost does not grow with the scene: 32 triangles
trace at 8.5 million a second, and coherent rays come no faster than incoherent ones. Inside the
path tracer a traversal runs at the rate the benchmark measures, about 20 million a second. The
cost is therefore in the traversal itself, not in the launch or the copies of the rays and hits.
Profile a traversal to find where its cost goes, and then the build. ADR-0193's cost is that
this traversal reaches Embree's speed. In the path tracer over a lit room of a million
triangles, traversal takes 35 to 38 percent of the time, so a traversal twice as fast makes that
render about a fifth faster. [Traversal.md](offline/Traversal.md#speed) has the
figures, and `moya/bench/Baseline.md` the run to measure a change against.

## Editor

Open work on the editor app. [editor/](editor/README.md) describes the editor. Every one of the
menu's 98 commands has a handler. Each group below is a piece of work of its own, in any order.
[MoyaRebuild](roadmap/MoyaRebuild.md#9-interactive-rendering) holds taking an edit into a render
that is running. The [EditorScripting](plans/EditorScripting.md) plan holds the scripting work:
rotation order, shared meshes in a scene document, placing an item and setting a key's
interpolation by command, a headless mode, and batches in parallel.

[] **A clicked button keeps the focus ring.** A press on a retained button gives it the focus, so
a workspace tab or the search keeps the focus colour's outline after a click, until a press
lands elsewhere. A ring is wanted when the keyboard moved the focus, and not after a click, as
a browser's `:focus-visible` decides.

[] **A maximised orthographic view loses its x axis colour and its selection outline.** Front,
maximised with Ctrl+Space, draws the grid's x axis grey rather than red, and a selected cube's
face without its amber edges. The same view in the quad draws both. The cause is not known: the
grid's major lines at the larger size, or the depth bias between the shaded face and its edges,
are where to start.

[] **NURBS surfaces.** The editor models polygon meshes only. A NURBS surface kind of item
would be made by revolving, lofting or sweeping curves, and trimmed by curves on it. It needs an
ADR on how it relates to meshes, and moya's `NuPatch` and `TrimCurve` have empty bodies.

[] **A render shows nothing until it finishes.** moya resolves its samples into pixels at the end
of a frame, so the render view waits for the whole picture. Resolving each bucket as it
finishes would let `Delegate` hand over pictures as they improve.


