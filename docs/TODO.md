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

Each entry states the gap in the code, not who is waiting for it. An entry may name a trigger:
the condition that makes the work due.

An open plan or roadmap may order entries from here. The entry stays here until the work lands,
and its section names the plan or roadmap that orders it.

## Offline rendering

Each of these waits until a scene needs it. The
[OfflineShading plan](plans/completed/OfflineShading.md) held both rather than build them early.

[] **The reyes hider adds every polygon to the traced scene.**

- moya fans each polygon into the traced scene as it arrives, before culling, and shades its
  lights once for it. This lets an off-screen caster cast a shadow.
- Every standard light with a direction calls `transmission()`, so only a scene with no
  shadowing light gains. Such a scene casts no ray, and the hierarchy is built on the first
  ray, so it pays only the memory the triangles hold.
- The triangles still have to be made as the scene is read, because a shader bound later may
  trace. The fix would drop them when rendering starts if nothing traced.
- Due when a large scene with no shadowing light shows that memory.

[] **`offline::trace::Tracer` runs a shader once per hit.**

- Each hit gets its own `HitShader` and its own run of the machine, so a ray-hidden image pays
  the machine's per-run cost once per pixel sample.
- The fix batches the hits of a scanline that share a shader into one run, as the reyes hider
  shades a grid. It is for speed alone, and gives no derivatives.
- Due when a profile of a Release render shows that preparing and running the machine for one
  hit outweighs intersection. A pixel's samples are seeded per pixel, so batching them in another
  order does not change the picture.

Background: [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md#step-17--held-area-lights-displacement-and-acceleration).

## moya's front end and reyes hider

Defects in `moya/libmoya` that its comments described wrongly. Each is due when a scene or a
caller reaches it.

[] **Five RI C entry points are unsafe or empty.**

- `RiProjection` reads the value after `"fov"` as a token and builds a string from it, then
  reads a `double` where RI passes a pointer to an `RtFloat`. The behaviour is undefined.
- `RiEnd` with no context open calls `pop_back` on an empty stack.
- `RiGetContext` returns a pointer into a `std::vector<RenderContext>`, so a later `RiBegin`
  that grows the vector leaves the handle dangling. `RiContext` ignores the handle.
- `RiDisplayV`, `RiPolygonV` and `RiProjectionV` are empty, and `RiDisplay` ignores its
  parameter list.
- `RtErrorHandler`, `RtProcSubdivFunc`, `RtProcFreeFunc` and `RtArchiveCallback` return
  `RtFloat` where RI has `RtVoid`, so `RiErrorIgnore` and its kin cannot be passed to
  `RiErrorHandler`.

[] **A bad name is accepted silently.**

- `RiProjection(RI_NULL)` gives an orthographic projection and an unknown name the identity.
  RI has the reverse, and nothing logs the unknown name.
- `setCoordinateSystem()` and `coordinateSystem()` with an unknown name insert an all-zero
  matrix and make it current, which flattens everything drawn after it.

[] **`RenderContext` assumes its caller's order.**

- `addPolygon()` under the reyes hider dereferences `frameBuffer_`, which exists only after
  `prepareWorld()`. A polygon sent before `RiWorldBegin` crashes.
- `render()` ignores the result of `image::Factory::write`, so an unknown extension writes no
  file and reports nothing. It also makes a new `Logger` instead of using `logger_`.
- `RiCropWindow` has no effect. `crop_` is stored by nothing and read by nothing.

[] **The reyes hider departs from RI in four places.**

- `GridShader` sets I to P for every camera. That is right for a perspective camera only; an
  orthographic camera's I is (0, 0, 1) in camera space. The ray hider is right.
- `GridShader::space("object")` fails, although the primitive's placement is that transform.
  `shade()` is given the `Shading`, not the primitive.
- ShadingRate is read as a side length in pixels: the first pass splits at
  `sqrt(gridSize) * shadingRate` pixels. RI defines it as an area.
- `Polygon::dice()` uses the first four vertices, so a polygon with more is diced wrongly.

## Tile grids

`api/grid` is a library of its own, and odyssey uses it. The grid owns a map's picture and
terrain legend; the rest of a map belongs to the game. [Grid.md](api/Grid.md) describes the
library.

[] **A route and sight stop at a region's edge.**

- A region is a grid placed by its corner, and [Grid.md](api/Grid.md#regions) states the rules.
  odyssey loads its regions as the player approaches and crosses between them by stepping.
- `findPath` and `hasLineOfSight` each take one grid, so odyssey's clicked routes and its sight
  stay within the region the player stands in. The next region is dark until the player crosses.
- Joining them needs a search over several grids, or a game that stitches one together.
- Due when a game needs a route, or sight, that crosses a seam.

[] **Remembered sight belongs in `api/grid`.**

- Fog of war that keeps seen ground revealed is in odyssey's
  [tile/Sight.h](../odyssey/tile/Sight.h), but nothing about it is specific to odyssey.
- Move it to `api/grid` when a second game needs it.
- Background: [LargeWorlds, step 8](plans/completed/LargeWorlds.md#step-8--remembered-sight).

## Voxel

[] **Mouselook turns far too fast and rolls the view.**

- Ten pixels of horizontal mouse motion turns the view most of the way round and tips it over.
- The code reads one pixel as one unit of `Player::look()`'s heading and pitch. That scale was
  set before relative mouse mode existed.
- Background: [ShellAndShipping, step 3](plans/completed/ShellAndShipping.md#step-3--a-relative-mouse).

## Sprite sheets

The api has every piece of a sprite sheet round trip: `image::TextureAtlas` places regions,
`config::SpriteSheets` reads and writes the document that names them, and `image::crop` cuts a
region back out of a sheet.

[] **Nothing packs a sheet, and nothing unpacks a whole sheet.**

- `imagetool --crop` cuts out one region, so a caller that already knows where the sprites are
  can split a sheet one rectangle at a time.
- Nothing reads a `sprites.json` and cuts out every region it names.
- Undecided: whether this belongs in `imagetool`, in a separate `spritetool`, or with whoever
  needs it first.

## Models

`api/asset` reads glTF 2.0 into a `v3d::type::Model`. That is the only geometry the api loads
from a file. No app in this tree loads one: voxel builds its terrain procedurally and the editor
models with `brep::BRep`. The gap below therefore comes from the library's own tests, not from
an app. [Assets.md](api/Assets.md) describes the loader.

[] **External buffers are resolved by cgltf, not by the asset manager.** A `.gltf` file with
external buffers has them resolved relative to the file, which is cgltf's own behaviour. It
matches the asset manager's path handling only because the manager passes cgltf a full path.

## Lit scenes

[] **There are no cascaded shadow maps.**

- `lighting::fitShadowToView` fits one map to the camera's view out to a distance. Over a long
  view, one map spreads its texels thinly, and near shadows lose detail.
- Cascades split the view into slices by distance, each with its own map, and the lit shader
  picks the slice a fragment falls in.
- `lighting::fitShadow` covers each caster's position and a margin, not its extent or scale. A
  caller with large casters passes a margin big enough for them.
- Due when a game's view is long enough for near shadows to look coarse.
- Background: [LitScene, step 8](plans/completed/LitScene.md#step-8--a-shadow-map).

## Loading

[] **A texture has no request of its own.**

- `MeshRegistry::request` returns a mesh handle at once and fills it later. A texture handle
  cannot do the same, because `pipeline::Resources` would have to swap the image behind a live
  handle and rewrite its descriptor set.
- A worker job that decodes the image, and a completion that calls `Textures::texture(image)`,
  covers the need meanwhile.
- Due when a game wants a texture handle before its pixels arrive.

[] **An upload still waits for its copy.**

- `memory::Uploader::oneShot` waits on its own fence, so the caller can free its staging buffer
  when it returns. Frames in flight keep running, and the main thread waits only for the copy.
- An upload that returned at once would retire its staging buffer through the frame ring.
- Due when a profile shows the fence wait itself.

[] **The upload budget's default is not measured.** `MeshRegistry::DEFAULT_UPLOAD_BUDGET` is
4 MiB, chosen without measuring a region's upload, because no game in the tree requests meshes.
Due when one does.

## Audio

`audio::Engine` plays a voice on a named bus, with a fade, a gain and a pan.

[] **A voice has no position.**

- A pan places a sound between the speakers. Nothing places it in a 3D scene, or makes it
  quieter with distance.
- SDL_mixer 3.2's `MIX_SetTrack3DPosition` places a track and attenuates it by distance, with
  the listener fixed at the origin. A caller would give each position relative to its listener.
- Due when a game needs sound in a 3D scene.
- Background: [LitScenesAtScale, step 10](plans/completed/LitScenesAtScale.md#step-10--close-phase-3).

## Tests

[Testing.md](contributing/Testing.md) describes what the suites cover.

[] **Nothing tests a requested model that decodes and does not upload.** `MeshRegistry::poll`
logs and abandons such a request, but no fixture decodes cleanly and then fails `upload()`'s
checks, so the branch never runs in a test.

[] **Nothing binds one value to a colour shader parameter.** `sl::Instance` copies a single
value to every channel, and no case in `SlShaderLibraryTest` binds fewer than three.

[] **Two things need a window or a sound device to test:** `Feature::Window` and
`audio::Engine::initialize()`. A software Vulkan driver does not help with either.
`ui::TextRenderer`'s atlas upload needs a device, which the `render_device` suite could give it.

[] **Some rendering cannot be checked against a reference picture.** A reference image may hold
only what the Vulkan specification determines exactly. Blending, filtered sampling, multisampling
and text are therefore checked only by a silent validation log and by spot checks. Checking them
more strongly needs a second Vulkan implementation to compare against, and this tree has none.

## Known review findings

Defects a review found that the change under review did not introduce.
[Review.md](contributing/Review.md) says how this list is used: a reviewer reads it first and
does not report an entry again. Each entry names its class.

[] **The PNG writer has no error recovery.** Cleanup, major. `writer::Png` installs no
`setjmp(png_jmpbuf(...))`, so any `png_error`, including its own "Image is too tall", aborts the
process and leaks `row_pointers`. The error pointer it gives `png_set_error_fn` is the `c_str()`
of a temporary `std::string`, so `pngtest_warning` reads a dangling pointer as the file name.

[] **The TGA writer writes a wrong footer and can overflow its size.** Semantics and boundary
input, major.

- `tga_footer` declares both offsets as `int64_t`, so the footer is 34 bytes. TGA 2.0 has a
  26-byte footer with 4-byte offsets.
- `width_ * height_ * bytespp` is computed in `unsigned int`, which overflows for an image the
  writer accepts, up to 65535 on a side.

[] **The base `image::Writer::write` returns true and writes nothing.** Contract, minor. A format
registered with the base class reports success silently.

[] **A bus volume is not a bus gain.** Contract, major. `audio::Engine::busGain` calls
`MIX_SetTagGain`, which overwrites the gain of each track tagged at that moment. It does nothing
when no track plays on the bus and stores nothing. A volume set before anything plays has no
effect, and a later `play()` resets the track to `Play::gain`.

[] **voxel's camera rotation is undefined.** Contract, major. `Camera::rotateFlight` and
`rotateFirstPerson` rotate a default-constructed `glm::mat4`, which is uninitialised because the
tree does not define `GLM_FORCE_CTOR_INIT`. They also pass degrees to `glm::rotate`, which takes
radians, so the 90 degree pitch clamp does not match the rotation.

[] **voxel's player speed is never read.** Contract, minor. `PLAYER_VELOCITY` is read only by
`Camera::updateVelocity()`, and that and `updatePosition()` have no caller. `Player::move()` sets
a velocity of 1, so the player walks at one unit a second.

[] **voxel's perspective may not honour its horizontal field of view.** Semantics, minor.
`Camera::perspective` multiplies both scales by the aspect ratio, so the horizontal field of view
equals `fovx` only at an aspect of 1. Check against the intended projection.

[] **A shading language `output` is never copied back.** Semantics, major.

- A shader parameter declared `output` sets `Symbol::output`, which nothing reads.
- A function formal declared `output` is accepted. `Emitter::emitInline` copies arguments into
  formals and never copies them back, so a function that writes one leaves the caller's variable
  unchanged. The shading language passes an output formal by reference.

[] **Two-argument `faceforward(N, I)` uses N as its own reference.** Semantics, minor. RenderMan
defines the default reference as Ng. The two-argument form is in `sl/runtime/Library.cxx`.

[] **A runtime shader report has no source position.** Contract, minor. The emitter writes a line
and column into every `sl::runtime::Instruction`, and no machine report reads them.

[] **The RIB reader reads a Sphere's parameters as one vertex.** Semantics, minor.
`Reader::primitiveRequest` passes `vertices = 1`. RI gives a quadric four varying values, so a
bracketed varying "Cs" warns about its length and a bare one reads one element. It does not
matter while spheres ignore their parameter list.

[] **`RiPolygon` reads only "P".** Contract, minor. The RIB path also reads "Cs", "N" and "st",
so the C entry point and the RIB reader build different polygons from the same request.

[] **A default `camera::Profile::Frame` disagrees with its hand.** Contract, minor. The Frame
defaults (direction -z, right +x, up +y) form the DirectionCrossUp basis, but a Frame-built
profile has `hand_` UpCrossDirection and an identity rotation. Its `direction()` is -z while
`createView()` looks along +z until `lookat()` or `turn()` runs. Check whether a caller relies on
the defaults.

[] **The glTF loader drops two failures without a word.** Boundary input, minor.

- `influenceOf()` maps a joint index outside the skin to joint 0 at full weight, and sets no
  `Dropped` flag.
- `decodeEmbedded()` returns null without logging when an image has neither a buffer view nor a
  uri. Every other failure in that function logs.

[] **A region load that throws is not logged.** Cleanup, minor. `odyssey/tile/World.cpp` marks
the region failed and drops the exception held in the `std::expected`.

[] **`CameraControlTool::deactivate` ignores which mode it was given.** Contract, minor. It always
resets to `CAMERA_MODE_NONE`, so releasing one camera modifier while another is held cancels the
held move.

[] **A menu `Item` of any type can show a check mark.** Contract, minor. `Item::checked()` returns
`checked_` whatever the type, unlike `Button::checked()`, which also requires a toggle.
`ComponentRenderer::panel()` draws a mark for any checked item.

[] **`Immediate::theme()` reads every `Dressing` metric but `indent`.** Contract, minor. A "tools"
style cannot set the bullet indent.

[] **pong's multiplayer mode is single player.** Contract, minor. `ui::setMultiplayerMode` calls
`mode(false)`, as single player does, so the computer still plays the right paddle.

[] **`BRep::addEdge(const glm::vec3&)` is declared and never defined.** Build, minor. A call
compiles and fails to link.

[] **Two tests do not check what they claim.** Weak test, minor.

- odyssey's `a_world_destroyed_before_its_loads_arrive` streams at (1, 1), five tiles from the
  east region, and `APPROACH` is 4. No load is in flight when the world is destroyed. Streaming
  at (2, 1) puts one in flight.
- moya's `framebuffer_negative_bound_test` has no assertion. It passes unless an out-of-range
  bucket index crashes, and never checks which bucket a primitive lands in.

## Editor

Open work for when the editor app itself moves forward. [editor/](editor/README.md) describes the
editor. [EditorModelling](roadmap/EditorModelling.md) orders these. The
[EditorModelling plan](plans/EditorModelling.md) takes up everything here but most of the
missing command handlers.

[] 55 of the menu's 77 commands have no handler, and only log themselves when chosen.

[] There are no modelling operations. In a component mode you can select a face, but moving it
moves the whole object.

[] The manipulators move only the first selected object.

[] The viewport panes cannot be resized by dragging.
