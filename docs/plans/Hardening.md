# Hardening — RI Conformance, the Shading Language, Image Writers and voxel's Camera

Drafted 2026-10-24 against `ab4bdea1`. This plan fixes the defects in [TODO.md](../TODO.md).
It takes the sections "moya's front end and reyes hider" and "Known review findings", voxel's
mouselook, the two test gaps that need only a case, and the external buffers of a glTF file. When it
closes, those sections are empty, and the known-findings list a reviewer reads first is empty
too.

Each entry this plan takes stays in TODO.md until the step that fixes it lands, and is then
deleted there. Each step fixes its defects with a test that fails first, as
[Review.md](../contributing/Review.md) requires, or records why no such test can be written.

## What the survey found

The tree was read at `ab4bdea1`. Line numbers are from that commit. Every entry still holds.
These are the places where the fix is larger than the entry says, or where two entries share a
fix.

**moya's C interface:**

* **`RiProjection` reads its value pointer as a token, then a `double`**
  (`moya/libmoya/RenderMan.cxx:308-329`). `RiImager` and `RiAttribute` already build their
  variadic form on `collect()` (`:45`) and forward to the `V` form. `RiProjection`,
  `RiDisplay` and `RiPolygon` can do the same, which also gives `RiProjectionV`, `RiDisplayV` and
  `RiPolygonV` (`:331`, `:427`, `:757`) a body.
* **`RiPolygon` reading only "P" is the same fix.** Routed through `RiPolygonV`, it reaches the
  `build()` the RIB path uses (`moya/libmoya/RIBHandler.cxx:325-337`). That function is in an
  anonymous namespace and has to be shared.
* **`RIBHandler::display` also drops its parameters** (`RIBHandler.cxx:248-249`).
* **The test helper reaches the context through `RiGetContext()`**
  (`moya/tests/RenderManTest.cxx:23-25`). Making the handle stable changes that helper.
* **`coordinateSystems_[name]` is used in seven more places** inside `addPolygon` and
  `GridShader` (`RenderContext.cxx:930,932,995,1004`, `reyes/GridShader.cxx:70,151,239`). A
  lookup that refuses an unknown name has to cover them.
* **CropWindow is parsed by the RIB reader** but moya's `RIBHandler` does not override
  `Handler::cropWindow` (`api/render/offline/rib/Handler.h:56`). Honouring it means both hiders
  render only the cropped buckets.

**The reyes hider:**

* **19 of the 22 test scenes use an orthographic projection.** Correcting I for an orthographic
  camera changes the specular in those reference pictures.
* **`GridShader::space("object")` needs the primitive's placement**, which means a signature
  change through `Bucket` and `Hider`. The I fix changes the same function.
* **ShadingRate as an area agrees with today's reading at the default of 1**, and no test scene
  sets it, so no reference picture moves.
* **`Polygon::dice` drops every vertex after the fourth** (`moya/libmoya/Polygon.cxx:402-414`).
  `split()` does not reduce the vertex count, so a polygon with more than four vertices has to be
  broken up before dicing.
* **`FrameBuffer` has no accessor for one bucket's primitives** (`moya/libmoya/FrameBuffer.h`),
  so the weak bucket test needs one.

**The shading language:**

* **Two-argument `faceforward` is used by every standard shader** (`sl/ShaderLibrary.cxx:55-90`)
  and by `moya/tests/data/bumpy.sl` and `checked.sl`. The existing `sllibrary_faceforward_test`
  (`SlLibraryTest.cxx:196-212`) asserts the current behaviour. Ng exists only in surface and
  displacement shaders (`Compiler.cxx:78`).
* **An `output` formal needs an lvalue argument**, which the compiler does not check. What an
  `output` shader parameter means between shaders is message passing, which RI defines through
  `surface()`, `displacement()` and `lightsource()` lookups.
* **Runtime reports are de-duplicated by their text** (`sl/runtime/Machine.cxx:143-150`). With a
  position in the text, one fault at two sites reports twice.

**Image writers:**

* **The TGA size multiplies two `uint16_t`, promoted to `int`** (`api/image/writer/Tga.cxx:96`).
  65535² is a signed overflow, and the widened result is 17 GB.
* **The PNG writer allocates its rows before its height check** (`writer/Png.cxx:131-135`).

**voxel:**

* **Mouselook spins because the rotation matrix is uninitialised.** A debug build fills it with
  `0xCC`. Mouselook, the rotation finding and the look sensitivity are one fix in
  `voxel/src/engine/Camera.cxx:232-288` and `voxel/src/game/Player.cxx:82-89`.
* **The same functions have two more defects.** The rotation accumulates, so a turn with both a
  heading and a pitch pitches z twice. The axes are multiplied as row vectors, which rotates by
  the transpose.
* **`Camera::perspective` gives `aspect·f` and `aspect²·f`** where `f` and `aspect·f` are right
  (`Camera.cxx:179-183`).
* **`PlayerTest` asserts the player stops at once on release** (`voxel/tests/PlayerTest.cxx:19-32`).
  Driving speed through `updateVelocity()` decelerates.
* **The voxel suite has no camera test.**

**The rest:**

* **pong already has two players.** Co-op is two people, W/S against the arrows
  (`pong/src/PongScene.cxx:73-93`). Multiplayer is a third menu item with no meaning of its own.
* **A default `camera::Profile::Frame` has one caller,** the test that passes every vector
  (`api/type/tests/ProfileTest.cxx:126`). Nothing in production relies on the defaults.
* **No glTF fixture has an external buffer.** Named textures resolve through the asset manager
  (`api/render/realtime/DecodedModel.cpp:44-46`), and buffers through cgltf.
* **The `MeshRegistry::poll` branch is reachable with a stub loader** registered through
  `asset::Manager::registerLoader` (`api/asset/Manager.h:82`).

## Decisions

The choices below are settled inside their steps. None of them is an ADR: each is a bug fix, or
the behaviour of one class.

| Choice | Step | Proposal |
|---|---|---|
| What a polygon sent before `WorldBegin` does | 1 | Refused and logged, before it is traced |
| How a polygon of more than four vertices is diced | 2 | Split into quads and a triangle before dicing |
| What two-argument `faceforward` uses outside a surface | 3 | The compiler passes Ng where it exists, and refuses the two-argument form elsewhere |
| The largest image the TGA writer accepts | 4 | Refuse any image larger than a `uint32_t` byte count |
| voxel's look sensitivity | 5 | A constant in degrees per pixel, tuned by hand |
| Whether voxel's player stops at once | 5 | Decelerates through `updateVelocity()`, and `PlayerTest` states that |
| What pong's Multiplayer means | 6 | The menu item goes, since Co-op is two players |

## What blocks what

- **Step 1 before step 2.** Both change `RenderContext`, and step 1's lookup of a coordinate
  system is what step 2's object space reads.
- **Step 3's `faceforward` and step 2's I both move moya reference pictures.** Each step
  regenerates the pictures it moves, and says what changed in them.
- **Steps 4, 5 and 6 are independent** of each other and of steps 1 to 3.
- **[EditorScene](EditorScene.md) step 5 needs the `CameraControlTool` fix in step 6.**

## Steps

### Step 1 — RI's C interface and the render context

**Closed.** Each fix has a test that fails without it.

- `RiProjection`, `RiDisplay` and `RiPolygon` collect their lists and forward to their `V`
  forms. `Polygon::from` builds a polygon for both the C interface and the RIB path. Both
  therefore read P, Cs, N and st.
- `RiProjection` read its value pointer as a token. A case now passes a 45 degree fov and
  compares the screen matrix with one the context built directly.
- `RiDisplayV` and `RIBHandler::display` log each display parameter as not used. No parameter
  has a meaning in moya.
- `Renderer` holds each context through a `boost::shared_ptr`, so a handle from `RiGetContext`
  survives a later `RiBegin`. `RiContext` makes a context active by moving it to the top of
  the stack. `RiEnd` with an empty stack does nothing.
- The four callback types return `RtVoid`, and a case passes each standard error handler to
  `RiErrorHandler`. Before the change the case did not build.
- An empty projection name is RI_NULL and applies no projection. An unknown name is logged and
  the projection in force stays. The default projection is named `"orthographic"`.
- `coordinateSystem()` is the one read of a coordinate system. An unknown name is logged and
  gives the identity, and `setCoordinateSystem()` with one keeps the current transformation.
- A polygon before `WorldBegin` is logged once and dropped. A failed write is logged through
  the context's logger.
- `CropWindow` is stored from both paths. The ray hider casts rays only inside it, and
  `render()` clears every pixel outside it after the hider and the imager. The reyes hider
  renders every bucket, because a primitive is filed in the bucket of its upper left corner and
  may reach into the window from a bucket outside it.

The survey also found that `RiPointsPolygons` and `RiSphere` are empty in the C interface,
though the RIB path draws both. That is a missing feature rather than a defect in this step,
and [offline/README.md](../offline/README.md) lists it.

### Step 2 — The reyes hider follows RI

**Closed.** Each fix has a test that fails without it.

- `GridShader` gives I as (0, 0, 1) under an orthographic camera and as the point under a
  perspective one. E stays the origin under both. A traced hit uses the origin too, so the
  plan's "E to match" needed no change.
- A primitive's object space travels in its `Shading` rather than through a new argument to
  `shade()` and `displace()`. `RenderContext::shading()` is called when the primitive is
  submitted, so the transformation in force then is the primitive's.
- The ray hider answered no `"object"` space either, so `trace::Primitive` now carries one too.
  Both hiders give the same answer, and a case renders under each.
  `trace_a_hit_refuses_object_space_test` asserted the old refusal, and now checks the answer.
- ShadingRate is read as an area: a grid side is `sqrt(gridsize * ShadingRate)` pixels.
- A polygon with more than four vertices is broken into a fan of quadrilaterals, with a
  triangle last when the count is odd, before it is measured. A hexagon diced whole drew only
  its upper half before.
- `FrameBuffer::primitiveCount(column, row)` counts one bucket, and
  `framebuffer_negative_bound_test` checks where each primitive lands.
- Three reference pictures changed, all from the I fix alone: with only I put back, the old
  pictures matched. `reference-shadow.png` and `raytrace-shaded.png` render the same scene
  under the two hiders. Their mean difference fell from 3.12 to 2.12 levels.

### Step 3 — The shading language

**Partly closed.** Every item but message passing has landed, each with a test that fails
without it.

- An `output` formal is copied back to its argument after the inlined body. The compiler
  refuses an argument that is not a variable, one the caller may not assign, and one of another
  type, since a coercion has no way back out.
- The compiler gives two-argument `faceforward` Ng as its third argument, and refuses the form
  in a light shader. The two-argument signature is gone from the built-in table.
- The test scenes in `reference-shaded.rib`, `reference-sampled.rib`, `ReferenceTest` and
  `RIBHandlerTest`'s quad gave "N" facing the camera on polygons wound so that Ng faced away.
  RI's `faceforward` shaded those from behind, black. Each is now wound clockwise as the camera
  sees it, with its per-vertex lists in the same order. With N and Ng agreeing, the pictures
  rendered with Ng and with N as the reference are byte for byte the same. The rewinding alone
  moved two reference pictures by up to 8 levels at a highlight, because it swaps the grid's u
  and v. They are regenerated.
- A report made during a run starts with its instruction's line and column, and two places
  that make the same report are two reports. Seven existing cases now check how a report ends,
  and two that counted reports count one for each call site.
- `slshaderlibrary_binds_a_colour_from_one_value_test` covers a single value bound onto a
  colour. It passes on the old code, which already replicated the value.
- The RIB reader reads a Sphere's parameters as four varying values.

Message passing follows in a commit of its own.

The items as planned:

- An `output` formal is copied back to its argument when the function returns. An argument that
  is not a variable is refused at compile time.
- An `output` shader parameter is readable from another shader through RI's message passing
  functions, for the shaders moya runs together.
- Two-argument `faceforward` uses Ng as its reference.
- A runtime report names the line and column of the instruction that made it, and is
  de-duplicated by message and position.
- A single value bound to a colour parameter is covered by a case in `SlShaderLibraryTest`.
- The RIB reader reads a Sphere's parameters as four varying values.
- Tests in `SlCompilerTest.cxx`, `SlMachineTest.cxx`, `SlLibraryTest.cxx`,
  `SlShaderLibraryTest.cxx` and `RibReaderTest.cxx`. The pictures that move are regenerated.

### Step 4 — Image writers

**Open.**

- The PNG writer recovers from a libpng error through `setjmp`, frees what it allocated, and
  returns false. The error pointer it gives libpng lives as long as the write.
- The TGA footer is 26 bytes with 4-byte offsets. The size is computed wide, and an image too
  large to address is refused.
- The base `Writer::write` is pure virtual.
- Tests in `api/image/tests/ImageWriterTest.cxx`, including a PNG too tall to write that returns
  false rather than aborting.

### Step 5 — voxel's camera

**Open.**

- `rotateFlight` and `rotateFirstPerson` start from the identity, take radians, and rotate by a
  fresh matrix for each axis. Axes are multiplied as column vectors.
- The pitch clamp is in the same unit as the rotation.
- `Player::look` takes a sensitivity in degrees per pixel.
- `perspective` honours its horizontal field of view at every aspect.
- The player's speed comes from `PLAYER_VELOCITY` through `updateVelocity()` and
  `updatePosition()`.
- Tests in a new `voxel/tests/CameraTest.cxx`: a heading turn keeps the up axis, a pitch stops
  at its clamp, the projection's horizontal field of view matches at two aspects. `PlayerTest`
  states how the player stops.

### Step 6 — Small contracts

**Open.** Each fix with a test that fails first.

- `camera::Profile::Frame`'s defaults agree with its hand (`ProfileTest.cxx`).
- The glTF loader marks a joint outside its skin as dropped, and logs an image with neither a
  buffer view nor a uri (`GltfTest.cpp`).
- The glTF loader reads external buffers through the asset manager, as textures are, with a
  `.gltf` and `.bin` fixture (`GltfTest.cpp`).
- odyssey logs a region load that throws (`WorldTest.cxx`).
- odyssey's `a_world_destroyed_before_its_loads_arrive` streams at (2, 1) and requires a load in
  flight.
- `CameraControlTool::deactivate` resets only the mode it was given (a new
  `vertical3d/tests/tool/CameraControlToolTest.cxx`).
- A menu `Item` shows a check mark only when it is a toggle (`MenuBarTest.cpp`).
- `Immediate::theme()` reads `indent` (`ImmediateTest.cpp`).
- pong's Multiplayer item is removed, with its command and its line in
  [Games.md](../Games.md).
- `MeshRegistry::poll` abandons a model that decodes and fails `upload()`'s checks, reached with
  a stub loader (`MeshRegistryTest.cpp`).

## Outcome

Written when the plan closes.
