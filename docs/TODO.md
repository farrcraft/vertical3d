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

[] **Leak: moya's edited scenes leave blocks behind.** In the Debug build, `v3dtest_moya`
run on `moya_scene*` or `moya_sceneclient*` ends with the debug heap reporting 11 blocks, and on
`moya_session` or `moya_kept*` 4 blocks, of 16 and 48 bytes. Each run gives the same count. A
render of a RIB file, `moya_foundation_a_colour_chart_renders_its_reference_test`, reports none,
so it is something an edited `scene::Scene` allocates and never frees.

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
`api/ui/Immediate*.cpp` files, and the editor's controller and area layout in
`vertical3d/src/ControllerWindows.cxx` and `view/AreaLayout.cxx`. Some may be nested classes,
which the rule allows.

## Offline rendering

Open work on moya. [offline/](offline/README.md) describes it.

[] **moya's CPU traversal is several times slower than Embree's.** On a million
triangles `moya_bench` traces 11 to 12.5 million rays a second. Embree's paper gives 38 to 59
million on an older processor with half the cores. The build with spatial splits is five to
seven times slower. Most of a ray's cost does not grow with the scene: 32 triangles
trace at 8.5 million a second, and coherent rays come no faster than incoherent ones. Inside the
path tracer a traversal runs at the rate the benchmark measures, about 20 million a second. The
cost is therefore in the traversal itself, not in the launch or the copies of the rays and hits.
Profile a traversal to find where its cost goes, and then the build. ADR-0193's cost is that
this traversal reaches Embree's speed. In the path tracer over a lit room of a million
triangles, traversal takes 31 to 33 percent of the time, so a traversal twice as fast makes that
render about a fifth faster. [Traversal.md](offline/Traversal.md#speed) has the
figures, and `moya/bench/Baseline.md` the run to measure a change against.

[] **Texture lookups take three quarters of a textured render.** `moya_bench`'s textured room
has a coated OpenPBR surface whose colour and roughness are one image. It renders at under a
fifth of the lit room's samples a second, and 75 percent of its time is in texture lookups. The
material reads the image twice at each shading point, and a filtered lookup reads each of its
texels through the host's tile cache by a function call. Profile a lookup to find where its cost
goes: the call, the cache's search, or the texels a filter reads. `moya/bench/Baseline.md` holds
the run to measure a change against.

[] **The lit room's samples come a sixth slower than before materials.** `moya_bench`'s lit room
ran at 1.97 to 2.07 million samples a second before the materials plan, and runs at 1.64 to 1.71
after it, beyond a run's noise of a tenth. Its traversal runs at the rate it did, so the time is
in the integrator's other stages. Every surface there is diffuse, so the layered BSDF and the
specular samples the plan added are the place to look. Bisect the plan's commits with the
benchmark to find the one that slowed it.

[] **The volumes plan slowed scenes that hold no medium.** `paths-room.rib`, a diffuse room
under a dome, rendered at 512 by 384 in a Release build, takes about 10 percent longer after the
plan than at `da1fdf28`, before it. `moya_bench`'s lit room shows the same. A bisect puts 3
percent at step 10, 4 at step 11 and the rest after step 13, so no one change holds it. A light
sample copied the path's media into its shadow ray twice, which no longer happens, and that
recovered about a third. The rest, about 7 percent, is not in the size of what a sample or a
shading point builds. A BSDF of half as many closures, and a media stack of one entry, each
render that scene no faster. It is in the code the path's loop runs, so profile it. Build both
commits with symbols, record the scene with `xperf`'s CPU sampling from an elevated prompt, and
compare where each spends its time.

[] **A material's emission cannot vary over its surface.** An emitting material makes a light of
its geometry on the host, from its emission inputs as constants, so an emission input that
varies over the surface gives its node's default and is reported. Camera rays see the light's
radiance, not the program's. Emission that varies needs the light to take its radiance from the
program at each point it is seen, and its triangles' power from the program's mean over each.

[] **A mesh light's cut-away parts still light the scene.** A camera or indirect ray passes
through where an emitting material is cut out, but a light sample draws a point anywhere on the
light's triangles and gathers its whole radiance there. The light's power, and the light tree's
bounds, count the cut-away area too. A light sample needs to carry the opacity at the point it
draws, and a light's power to come from its opacity over each triangle, as emission that varies
needs from the program. [Integrator.md](offline/Integrator.md#surfaces-that-are-cut-out) states
the rule for every other ray.

[] **A split's step is sharp.** `splitlr` and `splittb` give one value below their center and
the other from it on. MaterialX's implementations smooth the step over a pixel, by the screen
derivatives of the coordinate the split reads. The coordinate is any value a node gives, so the
shading point's footprint in `st` measures it only when the split reads `st` itself. Smoothing
the step needs the program to carry each value's change across the footprint, or to run again
at the footprint's edges. [Patterns.md](offline/Patterns.md#ramps-and-splits) states the rule.

[] **An image read at a scaled coordinate filters over the wrong footprint.** A lookup filters
over the shading point's footprint in `st`, whatever coordinate its image node reads. A
tiledimage, or any node that scales its coordinate, reads more texels than that footprint covers,
and aliases by the scale. The fix is the one a split's step needs: the program carrying each
value's change across the footprint, or running again at its edges.
[Textures.md](offline/Textures.md#looking-up-a-texture) states the rule.

[] **A pass's records of what its samples cover grow with the picture.** A render of a
Cryptomatte layer or a deep file gives each camera sample of a pass a record of 32 bytes, in one
device buffer of every pixel's samples. A 3840 by 2160 picture at 16 samples a pass needs 4.2 GB
for it. The host merges the records after the pass, so a buffer of a band of rows, merged band by
band, would bound it. `integrate::internal::Records` owns the buffer.

[] **A compositing reference holds each channel to the beauty's error.** A pixel keeps the
standard error of its beauty alone, so `CompositingReferenceTest.cxx` compares every channel
within five of those errors. A channel of little light, such as a caustic, is held far more loosely
than its own noise would allow. A channel's own error, kept by the film for the channels a test
compares, would hold each to its own noise.

[] **A quadric has no `st`.** RI gives each quadric parametric coordinates, `s` around it and
`t` along it, which a pattern or a texture reads. moya holds a quadric's constant and
uniform primvars alone, so a node that reads `st` on a sphere reads nothing, and a striped or
textured sphere renders in its plain colour. A quadric hit needs to give its `st` from the
angle and height it was hit at, as RI defines them for each quadric.
[Surfaces.md](offline/Surfaces.md) states the rule as it stands.

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


