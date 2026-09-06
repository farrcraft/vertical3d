# Offline Rendering, Phase 2 — A Scene Worth Rendering

Drafted 2026-09-05, **closed 2026-09-05**. Took up phase 2 of
[the offline rendering roadmap](../../roadmap/OfflineRendering.md), which stays the account of
where both renderers stand and what the later phases are; this plan does not repeat it.

Phase 1 ended with both renderers computing a pixel from geometry, and with the geometry
hard-coded in C++. Neither can be handed a scene: talyn has a reader that recognises twenty-odd
requests and acts on one of them, moya has the RI entry points that reader should be calling,
and the two halves of one design sit in different applications. So the deliverable here is the
route from a file to a picture — a real tokenizer, a declaration table, one reader in
`api/render/offline` dispatching onto an interface each renderer implements, and the requests
each renderer needs to render what a hand-written scene says.

The phase closes when a person can write a `.rib` file and both renderers draw it, and when the
editor can write one.

**All eight steps landed.** Both renderers read RIB and draw it, the editor exports it, and each
renderer's phase 1 reference image is now reached by two routes. Four things turned up that this
plan did not anticipate, and are recorded in the step notes below: the world to camera transform
needed neither a transpose nor an inverse, `prepareWorld` was saving an identity as the camera,
a primitive had to carry the state it was submitted under across a split, and RI's camera basis
is one `CameraProfile` cannot hold.

## Decisions

Recorded in [docs/adr/](../../adr/), not here. The ones that shape this plan:

| ADR | Decision |
|---|---|
| [0022](../../adr/0022-offline-rendering-shares-an-api-library.md) | Shared offline code is `api/render/offline`; each renderer is a library with a driver |
| [0023](../../adr/0023-rib-is-the-offline-scene-description.md) | RIB is what both renderers read; the editor exports to it, one way |
| [0024](../../adr/0024-api-type-serves-both-renderers.md) | `api/type` serves both, and a convention is a parameter rather than a fork |
| [0018](../../adr/0018-a-project-is-json-and-stores-topology-verbatim.md) | The project file is the editor's own, and stores topology verbatim |
| [0013](../../adr/0013-mesh-is-a-dag-node.md) | A mesh is a dag node with a transform; the scene holds no cameras |
| [0025](../../adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md) | The reader hands a renderer C++ requests with typed parameter lists |

## What blocks what

Steps 2 to 4 are the reader, and nothing downstream of them can start: there is no way to get a
number out of a RIB file today without the tokenizer, no way to know how many numbers a
parameter takes without the declaration table, and nowhere for a request to land without the
interface. Step 1 is the record for the shape of that interface and is cheap.

Steps 5, 6 and 7 are the three consumers and are independent of each other. Step 8 closes the
phase and is what the first four are for.

```
1 ADR-0025 ─────────────────────────┐
                                    v
2 tokenizer ──> 3 declarations ──> 4 interface + reader ─┬─> 5 moya reads ──┬─> 8 reference scenes
                                                         ├─> 6 talyn reads ─┘
                                                         └─> 7 editor exports
```

## Steps

### Step 1 — ADR-0025, what the reader hands a renderer

[ADR-0023](../../adr/0023-rib-is-the-offline-scene-description.md) settled that one reader
dispatches onto an interface both renderers implement. It did not settle what that interface is
made of, and there are three candidates: moya's RI C ABI (`RtToken`, `RtPointer`, varargs), a
C++ request interface taking `std::string`, `float`, `glm::mat4x4` and a parsed parameter list,
or an intermediate scene representation that alternative 3 of ADR-0023 already rejected.

The record has to exist before step 4 writes the interface, because it constrains phases 3 to 5:
a shader's parameters, a light's parameters and a filter's parameters all arrive through it.

Two things it has to weigh, both of which point the same way. **A `va_list` cannot be built at
runtime**, so a reader holding a parsed parameter list cannot call `RiPolygon` at all — only the
`RiPolygonV` form, which is what the RI standard provides for exactly this caller. And
`api/render/offline` cannot include `RenderMan.h`: that header is moya's C ABI, and
[ADR-0022](../../adr/0022-offline-rendering-shares-an-api-library.md) put the shared library below
both renderers rather than beside one.

**Done when**: `docs/adr/0025-*.md` exists, is in the index, and step 4 cites it rather than
re-deriving it.

**Landed** as [ADR-0025](../../adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md).

### Step 2 — the tokenizer

`api/render/offline`, a lexer over a RIB stream. The current reader is `file_ >> token`, which
splits on whitespace, so `Polygon "P"` arrives with the quotes attached and `[-100. 0. -100.`
arrives as three tokens one of which begins with a bracket. Nothing above it can be written
until this is real.

- Five token kinds: an identifier, a quoted string, a number, `[`, `]`. Nothing else is legal
  in an ASCII RIB stream.
- Comments run from `#` to the end of the line. `##` is a structure comment carrying metadata —
  `##CameraOrientation`, `##Shaders` — and is a comment to a renderer like any other.
- Whitespace and newlines are not significant. An array spans lines in real RIB, which is why
  the token stream and not the line is the unit.
- Strings are double-quoted and carry the C escapes plus `\ddd` octal.
- Numbers take an optional sign, a leading or trailing `.` — `.5` and `5.` both appear — and an
  exponent. A number token holds a `float` and the reader narrows on use: `Format 640 480 1`
  and `Clipping 10 1000.0` are the same token kind, and `boost::lexical_cast<unsigned int>`
  over the second one throws.
- Every token carries a line and a column. A parse error that does not say where is most of the
  cost of a parse error.
- **An identifier ends the arguments of the request before it.** Only strings, numbers and
  arrays can be an argument, so the next identifier is the next request whether or not either
  one is recognised. That is what makes recovery from an unknown request possible without
  knowing its arity.
- **Binary and gzipped RIB are rejected by their first bytes** — `0x80` for an encoded request,
  `0x1f 0x8b` for gzip — with a message naming which, per ADR-0023's risk. Both parse into
  nonsense otherwise.

The ninety-line commented-out RIB at the head of talyn's `RIBReader.cxx`, which step 6 deletes,
is the standard's own example file and is the best fixture in the tree. It moves into
`api/render/offline/tests/data/` — a comment that is not explaining code, made into a test.

**Done when**: `v3dtest_render_offline` covers a quoted string with spaces and escapes, an array
spanning lines, `.5` and `5.` and `-1e3`, a comment to end of line, a `##` line, a binary header
and a gzip header rejected by name, and a token's reported line.

**Landed** as `RIBLexer`, and the example file is `api/render/offline/tests/data/example.rib`.
The lexer takes an `std::istream` rather than a path, so a case is a string literal and only the
two fixture cases touch the disk.

### Step 3 — declarations and typed parameter lists

Without this the reader is stuck at the first parameter that is not `P`, because RIB does not
say how many values a parameter takes — the declaration does.

- A declaration is a class and a type with an optional array count: `"uniform point"`,
  `"varying float"`, `"float[3]"`. An omitted class is uniform.
- The type gives the float count, RI table 5.1: float and integer 1, point, vector, normal and
  color 3, hpoint 4, matrix 16, string none.
- The class gives the element count: constant and uniform 1, varying and vertex one per vertex.
  So `"P"` on a four-vertex polygon is twelve floats and `"Cs"` is three or twelve depending on
  which of the two it was declared as.
- The standard variables are pre-declared — `P`, `Pz`, `Pw`, `N`, `Cs`, `Os`, `s`, `t`, `st` —
  along with the shader parameter names the RI token table lists. **They are declared here as
  the table's own strings.** `RenderMan.h` names most of them as uninitialised `RtToken`s and is
  moya's header; the shared library does not include it.
- `Declare "d" "uniform point"` adds one. The inline form — a parameter name that is itself a
  declaration, `"uniform float squish" 5` — is read where it is used and does not enter the
  table.
- A `ParameterList` maps a name to its values with the declaration that typed them, and reads
  back as `points()`, `floats()`, `strings()`, `matrix()`. A name that is not there reads as
  empty rather than throwing: a renderer must accept a request carrying a parameter it does not
  support.
- **A bracketed array is self-delimiting and a bare value is not.** `"roughness" .3` is legal,
  which is the whole reason the table exists. So an undeclared name followed by `[` is
  recoverable — report the name once, skip to the matching `]` — and an undeclared name followed
  by a bare value ends the parse with a message naming it, because there is no way to know how
  much of what follows belonged to it.

**Done when**: the suite covers each type's float count, a class deciding the element count,
`Declare` then use, the inline form, and an undeclared name in both the recoverable and the
fatal position.

**Landed** as `RIBDeclarations` and `ParameterList`, with one addition the standard's own
example forced: **an array whose length disagrees with its declaration is a warning, not an
error.** That file declares `"d"` a `uniform point` and then passes it `[1]`, one float where
three were promised. The array bounds itself either way, so the values are taken as they came
and the mismatch is reported once per name.

`bucketsize` and `gridsize` are pre-declared alongside the standard variables. They are
implementation specific rather than standard, and the example file's opening `Option "limits"`
is what wanted them.

### Step 4 — the request interface, and the reader that drives it

`offline::RIBHandler` and `offline::RIBReader`, per step 1's record.

- Every method on the handler has an **empty body rather than being pure virtual**. ADR-0023
  already says an unimplemented request is a no-op, which is what the RI standard asks of a
  renderer that does not support a feature; it also means a request added in phase 3 does not
  break both renderers on the day it lands. The cost is that a misspelled override is silent, so
  every override in both renderers carries `override`.
- The requests phase 2 handles, and no more: `version`, `Declare`, `Option`, `Format`,
  `FrameAspectRatio`, `ScreenWindow`, `CropWindow`, `Projection`, `Clipping`, `Display`,
  `FrameBegin`/`FrameEnd`, `WorldBegin`/`WorldEnd`, `AttributeBegin`/`AttributeEnd`,
  `TransformBegin`/`TransformEnd`, `Identity`, `Transform`, `ConcatTransform`, `Translate`,
  `Rotate`, `Scale`, `Color`, `Opacity`, `ShadingRate`, `Attribute`, `Surface`, `LightSource`,
  `Polygon`, `PointsPolygons`, `Sphere`.
- `Surface`, `LightSource` and `Sphere` are in that list and both renderers drop them. That is
  the difference between a request that is recognised and one that is unparsed: the reader
  consumes their arguments correctly, so the request after them is still read.
- **`Transform [16 floats]` is a change of convention, not a copy.** RIB writes the matrix in
  row-major order under RI's row-vector convention; glm stores column-major under a
  column-vector one. Reading the sixteen floats straight into a `glm::mat4` *is* the conversion,
  and an added `glm::transpose` undoes it. One line, and the line most likely to be corrected
  into being wrong — the suite pins it by transforming a point through a matrix that translates.
- `read(path, handler)` takes the handler as a parameter rather than holding one, so one reader
  serves both renderers and a suite can drive it with a handler that only counts.
- **An unrecognised request is reported, once per name.** Phase 1's lesson is that a scene which
  rendered nothing and a scene which was not understood look identical from outside.

**Done when**: a counting handler over the standard's example file from step 2 sees every
request in it with the right arguments, a `Polygon` arrives carrying four points, an unknown
request is reported and skipped without disturbing the one after it, and the matrix convention
has a case.

**Landed.** The interface is `offline::RIBHandler` and each renderer's implementation is
`<renderer>::RIBHandler` in its own namespace, which settles the first open question below.

One thing the plan had wrong: **RIB carries no vertex count for a `Polygon`.** The C API's
`RiPolygon(nverts, ...)` takes one and the RIB request does not - the count is the length of the
position array, so the reader divides `"P"` by three and hands the handler the result. That in
turn means a parameter list is read before the vertex count is known, so an unbracketed varying
or vertex parameter is unreadable and ends the parse rather than being guessed at.

### Step 5 — moya reads RIB

`moya::RIBHandler` in `libmoya`, driving a `RenderContext`. Then the requests a hand-written
scene forces, every one of which phase 1 named as expected rather than as a defect:

- **`RiProjection("perspective")` builds the identity matrix.** A RIB scene says
  `Projection "perspective"`, so nothing renders until this is real. It has to produce the depth
  range the orthographic branch produces and `Frustum::extract` reads, or the first pass culls
  the scene away — step 8's reference image is what proves it did.
- **The orthographic screen window is hard-coded to [-1, 1]** rather than read from `screen_`
  and `frameAspect_`, so a 4:3 image stretches a square window across it. RI's default is
  [-frameaspect, frameaspect] by [-1, 1] when the frame aspect is at least one and the
  reciprocal otherwise, with `RiFormat` setting the frame aspect to `xres * pixelaspect / yres`
  unless `RiFrameAspectRatio` overrode it.
- **`RiTransform` has no body, and `RiConcatTransform` is not declared at all** — the one RI
  request missing from a header that otherwise declares the interface in full. Both are how a
  RIB scene places its camera and its objects.
- **The world-to-camera transform is a transpose.** `glm::transpose(coordinateSystems_["camera"])`
  is an inverse only for a pure rotation, and the first scene that places its camera with a
  matrix that translates is the first one it is wrong for. `glm::inverse`.
- **`RiAttributeBegin` and `RiAttributeEnd` are empty.** A scene with two objects nests them and
  expects the current colour, surface and transform to push and pop together. `pushTransform`
  and `popTransform` exist; the graphics state around them does not.
- **`RiColor` is commented out** and `Bucket::shade` writes one `SURFACE_COLOR` over everything.
  Carrying the current colour onto the polygon and shading from it is not phase 3 — there is no
  light and no material in it — and without it every scene is one flat grey, which is a picture
  that cannot tell two polygons apart.
- The driver reads a file. `moya --file scene.rib` replaces the quad built in `main`, which is
  what this phase exists to end.

**Done when**: `moya --file scene.rib --output out.png` renders a perspective scene of two
differently coloured polygons placed by a transform, and a suite case asserts that a RIB scene
reaches `addPolygon` with the points the file names.

**Landed**, and four things beyond the six above turned up on the way to the first picture:

- **The world to camera transform needed neither a transpose nor an inverse.** This plan called
  for `glm::inverse`, and that is as wrong as the transpose it replaces. `prepareWorld` saves
  the current transformation as the camera coordinate system, and by the RI standard that
  transformation *is* the world to camera one - so it applies as it stands. A transpose and an
  inverse are both right when it is a rotation and neither is when it also translates, which is
  why nothing caught it: the only scene in the tree had an identity there.
- **`prepareWorld` was saving an identity as the camera transform.** It called `projection("")`
  first, which ends by resetting the current transformation, and only then saved that reset as
  the camera. So no scene could place a camera at all. It now projects only if the scene named
  no projection, which is also what stops a scene's own `Projection` being composed twice.
- **A perspective projection needs the divide, and nothing divided.** The first pass and the
  hider both took `xyz` from a projected point and dropped `w`, which is correct only while `w`
  is one. `project()` is that divide, and the plan's "make perspective real" was half the work.
- **A primitive has to carry the state it was submitted under.** Splitting resubmits pieces
  through the first pass during the *second* pass, when the current transformation and colour
  are no longer the ones the primitive was added with - so a scene of two placed objects
  measured a split piece of the first against the state of the last. Worse for colour: a split
  builds its pieces out of intersection points, so their vertices carry none at all and every
  split polygon came out white. `ReyesPrimitive::place()` holds both and a piece inherits them.

`Bucket`'s `shade()` and its one `SURFACE_COLOR` are gone: dicing interpolates the colour onto
the grid, which is what a `Cs` given per vertex needs anyway, and the constant it wrote over the
top of that was the whole of shading. `RiConcatTransform` was added to `RenderMan.h`, and the
empty `RiColor`, `RiOpacity`, `RiTransform`, `RiAttributeBegin`, `RiAttributeEnd`,
`RiScreenWindow`, `RiFrameAspectRatio` and `RiShadingRate` were given bodies so the C API and
the handler do not drift.

Two suite cases changed because the screen window did. `render_context_split_terminates_test`
asserted 4^5 pieces from a 288 pixel bound; the correct window makes that bound 216 pixels and
the answer 4^4. And **the moya reference image was regenerated**: a 64 by 48 frame had been
rendering through a square screen window, which stretched it. The scene is unchanged and the new
picture is the same quad, correctly proportioned.

### Step 6 — talyn reads RIB

`talyn::RIBHandler` in `libtalyn`, filling a `Scene`. `talyn::RIBReader` is deleted: its one
implemented request is `Format`, and its commented example file left in step 2.

- **The transform stack is the handler's.** RIB's current transformation inside the world block
  is object-to-world, and `talyn::Scene` holds triangles in world space, so a polygon's points
  go through it on the way in.
- A `Polygon` fans over its vertices. RI says a polygon is planar and convex, so a fan is the
  whole of it, and `PointsPolygons` is the same fan per face.
- **The camera is the interesting part.** RIB gives a projection type, an fov, the clipping
  distances and a world-to-camera matrix; `type::CameraProfile` wants an eye and a basis. That
  matrix is rigid for any camera worth pointing, so the eye is its inverse's translation and the
  basis is that inverse's columns. **A non-rigid camera transform and an off-centre
  `ScreenWindow` are what `CameraProfile` cannot express** — reject either with a message rather
  than rendering a picture that is quietly wrong. If one turns out to be needed, that is where
  [ADR-0024](../../adr/0024-api-type-serves-both-renderers.md)'s parameter arrives; this phase does
  not expect to need it.
- `Color` sets the colour of the triangles that follow, which is the flat colour `Triangle`
  already carries.
- The driver changes only in what it constructs — it already dispatches on the extension and
  already writes what the framebuffer holds.

**Done when**: `talyn --file scene.rib --outfile out.png` writes a picture with geometry in it,
which is the asymmetry phase 1 named and left open, and a suite case asserts that a RIB fixture
builds the scene a code case builds — the camera's eye and basis, the triangle count, and one
triangle's points and colour.

**Landed**, with the camera limit sharper than this plan expected. The decomposition is what was
written — the eye is where the matrix puts the world origin back and the orientation is its
rotation — but **the matrix has to have a positive determinant, and RI's camera basis usually
does not.** The standard's own example places its camera at (10, 10, 10) looking at the origin
with a matrix whose determinant is -1: RI's camera space is left handed with respect to the
world, and `createView()` builds `R' * T(-eye)` out of a rotation, which can never equal a
matrix that reverses handedness. So a general RI lookat camera is refused with a message rather
than rendered mirrored, alongside the off centre screen window and the non-rigid matrix this
plan already named. An axis aligned placement — which is what every fixture here uses — is fine.

That is the one limit in the phase a later step will want to reopen, and it is a question about
`api/type` rather than about talyn: it is exactly the shape
[ADR-0024](../../adr/0024-api-type-serves-both-renderers.md) anticipated, a convention only one
renderer needs. Nothing here needed it, so nothing was changed — and for every other reason step
6 predicted, there is no `api/type` change: the camera measures y downward from the top of the
viewport and `image::Image` row 0 is the top, so the pixel loop still maps straight across.

The RI default screen window is worked out in the handler rather than shared with moya's, which
holds its own because a reyes render context *is* the RI graphics state and a raytracer's scene
is not. Six lines of a specification's default, written twice, against a refactor larger than
the duplication.

### Step 7 — the editor exports RIB

`editor::RIBExportVisitor` in `vertical3d/src/scene/`, a `SceneVisitor` beside `WireframeVisitor`.

- One mesh is `AttributeBegin`, a `ConcatTransform` from its `dag::Transform::matrix()`, one
  `Polygon "P" [...]` per face, `AttributeEnd`. `faceLoop()` and `loopSegment()` already walk a
  face.
- The frame around it is `##RenderMan RIB-Structure 1.1`, `version 3.03`, `Format`,
  `Projection`, `Clipping`, the camera `Transform`, `WorldBegin` and `WorldEnd`. The scene holds
  no camera — the views own theirs per ADR-0013 — so the active view's is what the export means
  by the camera.
- **Topology and a placement per mesh, and nothing else**, per ADR-0023. The editor's `Scene`
  has no lights and no materials, so what comes out renders grey until the editor's scene model
  grows what phase 3 needs.
- `project::export::rib`, registered in `Controller` beside `project::save`, with an Export
  submenu in `vgui.json` beside Import. One document at a fixed `export.rib` beside the
  executable — there is no file chooser in the tree.
- **moya dices a polygon's first four vertices and drops the rest**, so an exported n-gon
  renders as a quad. The exporter writes the face loop as it stands; the fixture is a mesh of
  quads and triangles until dicing grows.
- The test exports a known mesh and **reads the file back through step 4's reader** into a
  counting handler. Asserting the text asserts the formatting; asserting what the reader makes
  of it asserts the file. `v3dtest_vertical3d` links `v3dlib_render_offline` for it — a link
  line, not a dependency on a renderer.

**Done when**: the export command writes a file the reader parses into the mesh count, face
count and transform the scene held, and one of the two renderers draws it.

**Landed** as `project::export::rib`, with an Export submenu beside Import. The round trip case
also writes what it exported to `data_out/export.rib` beside the executable: a round trip cannot
assert that a renderer draws the file, and handing it to one is the only thing the export is
for. talyn draws that cube — in white, because the editor's scene has no materials, which is
what [ADR-0023](../../adr/0023-rib-is-the-offline-scene-description.md) said a scene out of the
editor would look like.

The camera is written as an explicit `ScreenWindow` rather than left to the frame aspect. The
editor's camera states its aperture as a pixel aspect and an ortho zoom, and there is no reason
to make a reader rederive it from a format.

### Step 8 — the reference scenes are RIB

`talyn/tests/data/reference-triangle.rib` and `moya/tests/data/reference-polygon.rib`, each
describing the scene the phase 1 case builds in code.

- Each is rendered through the reader and compared against **the same committed PNG** the
  code-built case compares against. Two routes to one picture: if the file path and the code
  path disagree, the test says which, and nothing is regenerated.
- If the RIB camera cannot be made to land on the phase 1 one, the reference is regenerated and
  the commit says which request changed the picture and why. Regenerating a reference is
  expected through phases 3 to 5; regenerating one silently is the failure mode.
- The drivers are the other half, on the same two files:
  `talyn --file ... --outfile ...` and `moya --file ... --output ...`. That is what a person runs
  before reading a ctest failure.

**Done when**: `ctest -R talyn` and `ctest -R moya` each render a scene from a committed `.rib`
and match the reference the code-built case matches, on a machine with no GPU.

**Landed, and neither reference was regenerated for it.** Both `.rib` scenes match their phase 1
picture exactly, so the file path and the code path are pinned to one image apiece.

talyn's needed one thing the plan did not foresee: **a scene has no background colour to set**,
because `RiImager` is not implemented and the roadmap puts it in phase 3. The file says with
geometry what the code built scene says with `Scene::background()` — a backdrop polygon far
enough back and large enough to fill the frame, which every ray that misses the triangle hits
instead. The same pixels, by a route RIB can express today.

## Verification

The four gates in [sdlc.md](../../sdlc.md) §4: `ninja -C out/build/x64-Debug`, cpplint per
`CLAUDE.md`, and `ctest --test-dir out/build/x64-Debug --output-on-failure`.

Render verification is ctest, as it was in phase 1. Neither renderer touches a window, a device
or a swapchain, so "run it and read the validation log" — the method for every other rendering
change in this tree — is not the method here.

The new suite surface is `v3dtest_render_offline` for steps 2 to 4, the existing moya, talyn and
vertical3d suites for steps 5 to 7, and both reference cases for step 8.

## What this phase does not do

- **No shading.** `Surface`, `LightSource` and `Illuminate` reach both handlers and are dropped;
  the RI standard shaders are phase 3, and so is the fixed-function-or-language question the
  roadmap still has open. `Color` is graphics state, not a material.
- **No sampling.** One sample per pixel centre. The five `Ri*Filter` functions keep returning
  `0.0` until phase 4.
- **No `V` entry points.** `RiPolygonV` and its siblings are the RI standard's own answer to a
  caller holding a runtime parameter list, and implementing them on step 4's handler would give
  moya's C API and the reader one path. Nothing calls them, and ADR-0022's rule for writing
  shared code is a second consumer rather than a plausible one.
- **No binary or gzipped RIB.** Rejected by their first bytes, per ADR-0023.
- **No `Sphere`.** It reaches both handlers and both drop it, and `RiSphere` stays empty. A
  quadric needs a `ReyesPrimitive` that dices and a ray intersection, which is a primitive
  rather than a parser.
- **No RIB written by a renderer.** `RiBegin("poly.rib")` naming a file to write is the RIB
  generator half of the standard, and neither of these is one.
- **No import.** ADR-0023's export is one way, and the four `import::*` commands in the editor's
  menu stay unhandled.
- **No `api/type` change expected** — see step 6 for the two things that would force one.

## Open questions

Small enough to settle in the code with a comment rather than in a record, but named so they are
settled deliberately rather than by whoever types first. All three are settled:

- **What the handler is called.** `RIBHandler`, and each renderer's implementation is
  `<renderer>::RIBHandler` in its own namespace. Naming it for the request set would have been
  the better name for a second format that does not exist and is not on the roadmap.
- **Who owns the declaration table.** **Both.** The reader owns it and parses with it, and
  `Declare` still reaches the handler — a renderer that wants to know what a scene declared can,
  without every renderer having to build a table to be handed typed values.
- **Where the editor writes.** `export.rib` beside the executable, matching what `project.json`
  already does, until there is a file chooser.

One question this plan did not open and the work did: **is `RiRotate` the same rotation glm
builds?** Both renderers hand the angle to `glm::rotate`, which is counter-clockwise by the
right hand rule; RI states its rotations in a left handed system, so the two may disagree in
sign. Nothing in the tree can tell — both renderers use glm and agree with each other, and there
is no reference renderer to disagree with. It is named here because a scene from elsewhere is
what would surface it, and phase 2 is what makes such a scene readable.
