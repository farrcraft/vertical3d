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

An open plan or roadmap may order entries from here. The entry stays here until the work lands,
and its section names the plan or roadmap that orders it.

## Review gates

[] **No linter or formatter reads the tree's TypeScript.** The type check catches wrong types and
unused code, and the gates' tests catch wrong behaviour. Style, such as quotes and the form of
a header, is kept by hand. Choosing a linter or a formatter is a decision of its own, and
whether `typescript-eslint` supports TypeScript 7 bears on it.

- Background: [ScriptTooling](plans/completed/ScriptTooling.md#outcome).

[] **Nothing holds a TypeScript function to 100 lines.** clang-tidy checks the rule for C++, and
no tool in the tree measures a TypeScript function.

- Background: [ScriptTooling](plans/completed/ScriptTooling.md#outcome).

## Lit scenes

[] **`lighting::fitShadow` covers each caster's position and a margin, not its extent or
scale.** A caller with large casters passes a margin big enough for them. A registered mesh
carries a sphere around its vertices (`MeshRegistry::Entry::bound`), which the fit could
scale by each caster's transform instead.

- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

## Offline rendering

[] **A shader's traced rays include the hits that did not trace.** `trace()` casts one ray for
every hit of a run, whatever branch each hit took, because a shader callback is given no mask of
the hits that reached it. The rays of the hits that did not call `trace()` are cast and thrown
away. The callback needs the run's active hits, so it casts rays for those alone.

[] **Displaced slopes stripe under the reyes hider.** A displaced surface rendered by reyes shows
black and white bands on its slopes. `moya/tests/data/reference-displaced.rib` shows them at
256×192. The ray-trace hider renders the same scene cleanly.

[] **A shader's `printf()` output is never shown.** The runtime collects what `printf()` writes,
and nothing prints it or sends it to the log.

[] **moya exits 0 when it cannot write the image.** An output file with an extension no writer
takes logs an error, and the process still exits with 0, so a script that runs moya sees success.

[] **moya's message for an out-of-range `--grid` or `--bucket` is wrong.** It says the default
is used. A scene's `Option "limits"` can still set the size, so the message names one case of
two.

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

## Editor

Open work on the editor app. [editor/](editor/README.md) describes the editor. Every one of the
menu's 97 commands has a handler. Each group below is a piece of work of its own, in any order.

[] **A clicked button keeps the focus ring.** A press on a retained button gives it the focus, so
a workspace tab or the search keeps the focus colour's outline after a click, until a press
lands elsewhere. A ring is wanted when the keyboard moved the focus, and not after a click, as
a browser's `:focus-visible` decides.

[] **A maximised orthographic view loses its x axis colour and its selection outline.** Front,
maximised with Ctrl+Space, draws the grid's x axis grey rather than red, and a selected cube's
face without its amber edges. The same view in the quad draws both. The cause is not known: the
grid's major lines at the larger size, or the depth bias between the shaded face and its edges,
are where to start.

[] **A script cannot place an item.** A script can create primitives, select, key and undo, but
no command sets an item's translation, rotation or scale to a value. A script that builds a scene
leaves every item where its create command put it.

[] **Shared meshes in a scene document.** Two mesh items may share one `BRep`, but the scene store
writes each item's geometry in full, so they read back as two copies. Keeping them shared needs
the geometry as a record of its own, which the mesh items refer to by id, and a new mesh version.

[] **NURBS surfaces.** The editor models polygon meshes only. A NURBS surface kind of item
would be made by revolving, lofting or sweeping curves, and trimmed by curves on it. It needs an
ADR on how it relates to meshes, and moya's `NuPatch` and `TrimCurve` have empty bodies.

[] **A key's interpolation cannot be chosen.** The graph editor draws step, linear and Bezier
keys, and turns a key's slopes, but no command sets a key's interpolation. A key made in the
editor is always Bezier, and only an imported key can be step or linear.

[] **Rotation order.** Every item turns about x, then y, then z, so an item that locks in that
order cannot be keyed out of it. Each item needs an order of its own, saved and offered to
whatever keys it.

[] **A render shows nothing until it finishes.** moya resolves its samples into pixels at the end
of a frame, so the render view waits for the whole picture. Resolving each bucket as it
finishes would let `Delegate` hand over pictures as they improve.

[] **A live render starts again from nothing.** An edit restarts the whole render, because moya
reads a scene once. Taking edits into a running render needs moya to keep its scene and update
it, which the editor's interface already sends.

[] **A script cannot run without a window.** `--script` runs a TypeScript file once the editor
has started, in its window. A headless mode would open the project, run the script against it
and quit, with no window or device, so a script could batch edits or exports on a machine with
no GPU.

[] **A batch renders one frame at a time.** Its frames are independent files, and could run as
several moya.exe processes at once.


