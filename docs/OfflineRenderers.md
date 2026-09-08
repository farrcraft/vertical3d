# The Offline Renderers

`talyn` (raytracer) and `moya` (reyes, behind the RenderMan interface) are the *other*
renderers, and share nothing with the realtime stack. Each is a library, a driver and a suite:
`talyn/libtalyn`, `talyn/talyn` and `talyn/tests`, and the same three for moya. See
[ADR-0022](adr/0022-offline-rendering-shares-an-api-library.md). What they share lives in
`api/render/offline` (`v3dlib_render_offline`, namespace `v3d::render::offline`).

That library **names neither Vulkan nor SDL**, and neither renderer touches a window, a device
or a swapchain, so their suites render in CI where everything below the recorder in
`api/render` cannot.

[roadmap/OfflineRendering.md](roadmap/OfflineRendering.md) says where they stand and what each
would need next. [plans/OfflineRenderingPhase3.md](plans/OfflineRenderingPhase3.md) is the open
plan, taking up light and surface as a shading language.

## Building

**`api/render/CMakeLists.txt` adds `offline` below its `set(CMAKE_CXX_FLAGS "/utf-8")` line**,
where `tests` already is. A subdirectory added above that line does not inherit the flag.

## Framebuffers and raster space

- **The plane count is not the channel count.** `offline::FrameBuffer` is a stack of float
  planes. `image(channels)` takes the leading planes as the picture and leaves the rest to the
  renderer. moya's are RGB plus a depth, named by `moya::FrameBuffer::Plane`; talyn's four are
  RGBA.
- **moya's raster space counts y downward from the upper left**, which is RI's convention and
  `image::Image`'s row order. The composition is `raster * screen`, since a matrix applies to
  what is on its right. Reversing either would write a correct render upside down, or in eye
  units.
- **A `RenderContext` writes a file only when `RiDisplay` named one with type `"file"`.** The RI
  token table in `RenderMan.cxx` is `RtToken`, that is, pointers, and most of it is still
  uninitialised. A null token reaches the context as an empty string rather than as an error.

## RIB

**One reader serves both renderers.** `offline::RIBReader` dispatches onto
`offline::RIBHandler`, a C++ interface with typed parameter lists rather than the RI C ABI, per
[ADR-0025](adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md). Each renderer
implements it as `<renderer>::RIBHandler`. RIB is also what the editor exports to, one way, per
[ADR-0023](adr/0023-rib-is-the-offline-scene-description.md).

- **Every method has an empty body rather than being pure virtual**, because the RI standard
  asks a renderer to accept a request it does not support. A misspelled override is therefore
  silent, so every override carries `override`.
- **A matrix crosses the RIB boundary by being read in order, not transposed.** RIB writes row
  major under a row vector convention and glm stores column major under a column vector one, so
  `glm::make_mat4` over the sixteen floats *is* the conversion. This applies to `Transform`,
  `ConcatTransform`, `RtMatrix` and the editor's export alike.
- **A RIB `Polygon` carries no vertex count.** It is the length of `"P"`, which the reader
  divides out. A parameter list is therefore parsed before the count is known, and an
  unbracketed varying or vertex parameter ends the parse rather than being guessed at.

## Cameras and transforms

- **moya's world-to-camera transform applies as it stands.** `prepareWorld` saves the current
  transformation as the camera coordinate system, and by the RI standard that transformation
  *is* the world to camera one. A transpose or an inverse is right only when it is a rotation.
- **talyn refuses a camera `type::CameraProfile` cannot hold**: an off centre `ScreenWindow`, a
  non-rigid matrix, or one that reverses handedness. RI's camera basis for a general lookat
  produces exactly those. `RIBHandler::error()` says which one it was, and the reader still
  succeeds, because the request was understood.

## The shading language

Shading is a language rather than a set of built-in models, per
[ADR-0026](adr/0026-shading-is-a-language-over-a-batch.md), and it lives in
`api/render/offline` beside the RIB one: `SLLexer`, `SLSyntax` and `SLParser` read a shader,
`SLTypes` and `SLCompiler` check it, and `SLBuiltins` says what the standard library provides.
The phase that builds the rest of it is open, so what follows is where the seams are rather
than a tour.

- **The `SL*` files are to a `.sl` file what the `RIB*` ones are to a `.rib` file**, and are
  shaped the same way on purpose - a `peek`/`next` lexer over an `std::istream`, an `error()`
  that ends the stream, and a line and column on every token.
- **A keyword is a closed set**: the five shader types, the eight data types, the two storage
  classes, the control flow and the three lighting constructs. Everything else that looks like
  a name is an identifier, so a shader may declare a variable called `output` or write its own
  `noise`.
- **`.` is a dot product and `^` is a cross product**, and both bind tighter than a multiply.
  Neither is what a reader coming from another language expects, which is the one part of the
  grammar worth checking before assuming a shader means what it looks like.
- **All five shader types parse and three of them run.** A `displacement` or a `volume` shader
  comes back from the parser answering false to `SLShader::supported()`, so a scene carrying
  one is told what is unsupported rather than what is malformed - the same distinction the RIB
  reader draws between a request that is recognised and one that is unparsed.
- **The C preprocessor is not run.** A `#` is a diagnostic naming the missing tool, not a
  comment, and there is no compiled-shader file: a shader is source, compiled when it is first
  named.
- **The compiler annotates the tree in place.** Every expression comes out with a type and a
  storage class and every variable with a symbol index, rather than a second structure keyed by
  node. `SLCompiler::symbols()` is then the list a machine allocates registers against - a
  local declared twice in nested scopes is two of them.
- **The varying inference runs to a fixed point**, because a loop carries a varying value back
  to a name that was read before it was written. Inferring uniform where varying was right
  gives a whole grid one point's answer, which reads as a shading bug and is a compiler bug.
  Anything assigned under a varying condition is varying, and a value declared `uniform` that a
  varying one reaches is a fault rather than a quiet widening.
- **A signature is the declared interface, not a claim about the implementation.** `ambient`,
  `diffuse` and `specular` are in `SLBuiltins` beside `pow` and `normalize`, and are shader
  source written over `illuminance` rather than C++ - which is what the standard says and what
  makes them testable. A caller cannot tell, and neither can the type checker.

## Normals

Both renderers carry two, because SL's `faceforward` and `calculatenormal` are defined in terms
of the pair: **`Ng` is the geometric normal** - the plane the primitive lies in, one value across
it, wound the way its vertices are - and **`N` is the shading normal**, which a scene sets per
vertex with a varying `"N"` and which is `Ng` when it does not.

- **A normal transforms by the inverse transpose**, never by the matrix that moves the points.
  The two agree under a rotation and a uniform scale, which is every fixture in the tree bar the
  two that scale one axis, so this is a fault that hides until a scene does.
- moya's `Vertex` holds both. `RenderContext::addPolygon` fills them from
  `Polygon::geometricNormal()` before it moves anything, and the diceable branch is where both go
  into eye space. Dicing interpolates `N` and renormalises; `Ng` is copied, since there is one.
- **A split does not carry a per-vertex normal**, for the reason it does not carry a colour: its
  pieces are built from intersection points, which have neither. A piece inherits the whole
  primitive's plane through `ReyesPrimitive::place()`, so a surface large enough to split is
  faceted per piece.
- talyn's `Triangle` has a constructor per case, and `shadingNormal(u, v)` interpolates over the
  barycentric coordinates `type::Ray::intersects` reports - `u` weighs `b` and `v` weighs `c`, so
  `a` carries the rest. The overload that reports them exists because Moller-Trumbore solves for
  them on its way to the distance and the four argument form throws them away.

## Reyes

**A `ReyesPrimitive` carries the transform, colour and geometric normal it was submitted under.**
Splitting resubmits pieces through the first pass during the second one, when none of it is
current, and a split builds its pieces from intersection points that carry no colour and no
normal at all.
