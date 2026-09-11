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
would need next. No plan is open against them;
[plans/completed/OfflineRenderingPhase3.md](plans/completed/OfflineRenderingPhase3.md) closed on
2026-09-10 and is the account of how the shading language got here.

## Building

**`api/render/CMakeLists.txt` adds `offline` below its `set(CMAKE_CXX_FLAGS "/utf-8")` line**,
where `tests` already is. A subdirectory added above that line does not inherit the flag.

## Framebuffers and raster space

- **The plane count is not the channel count.** `offline::FrameBuffer` is a stack of float
  planes. `image(channels)` takes the leading planes as the picture and leaves the rest to the
  renderer. moya's are RGB, a depth and a coverage, named by `moya::FrameBuffer::Plane`;
  talyn's four are RGBA, where the alpha is that same coverage.
- **Coverage is what an imager reads as `alpha`**, and it is the difference between a pixel
  nothing was drawn into and a black one. One or nothing while there is one sample per pixel
  centre; a sampler that takes more would put a fraction there.
- **moya's raster space counts y downward from the upper left**, which is RI's convention and
  `image::Image`'s row order. The composition is `raster * screen`, since a matrix applies to
  what is on its right. Reversing either would write a correct render upside down, or in eye
  units.
- **A `RenderContext` writes a file only when `RiDisplay` named one with type `"file"`.** The RI
  token table in `RenderMan.cxx` is `RtToken`, that is, pointers, and most of it is still
  uninitialised. A null token reaches the context as an empty string rather than as an error.

## RIB

**One reader serves both renderers.** `offline::rib::Reader` dispatches onto
`offline::rib::Handler`, a C++ interface with typed parameter lists rather than the RI C ABI, per
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
- **talyn refuses a camera `type::camera::Profile` cannot hold**: an off centre `ScreenWindow`, a
  non-rigid matrix, or one that reverses handedness. RI's camera basis for a general lookat
  produces exactly those. `rib::Handler::error()` says which one it was, and the reader still
  succeeds, because the request was understood.

## The shading language

Shading is a language rather than a set of built-in models, per
[ADR-0026](adr/0026-shading-is-a-language-over-a-batch.md), and it lives in
`api/render/offline` beside the RIB one: `sl::Lexer`, `sl::Syntax` and `sl::Parser` read a shader,
`sl::Types` and `sl::Compiler` check it, `sl::Emitter` flattens it into a `sl::runtime::Program`,
and `sl::runtime::Machine` runs that over a batch. `sl::ShaderLibrary` maps a name to a program
and `sl::Instance` binds a scene's parameters onto one. What follows is where the seams are
rather than a tour.

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
  comes back from the parser answering false to `sl::Shader::supported()`, so a scene carrying
  one is told what is unsupported rather than what is malformed - the same distinction the RIB
  reader draws between a request that is recognised and one that is unparsed.
- **The C preprocessor is not run.** A `#` is a diagnostic naming the missing tool, not a
  comment, and there is no compiled-shader file: a shader is source, compiled when it is first
  named.
- **The compiler annotates the tree in place.** Every expression comes out with a type and a
  storage class and every variable with a symbol index, rather than a second structure keyed by
  node. `sl::Compiler::symbols()` is then the list a machine allocates registers against - a
  local declared twice in nested scopes is two of them.
- **The varying inference runs to a fixed point**, because a loop carries a varying value back
  to a name that was read before it was written. Inferring uniform where varying was right
  gives a whole grid one point's answer, which reads as a shading bug and is a compiler bug.
  Anything assigned under a varying condition is varying, and a value declared `uniform` that a
  varying one reaches is a fault rather than a quiet widening.
- **A signature is the declared interface, not a claim about the implementation.** `diffuse`,
  `specular` and `phong` are in `sl::Builtins` beside `pow` and `normalize`, and are shader
  source written over `illuminance` rather than C++ - which is what the standard says and what
  makes them testable. A shader that calls one has it adopted into its own function list before
  anything else runs, so nothing downstream of the compiler sees two kinds of function, and a
  shader's own definition of the name wins. `ambient()` is the exception and is C++: a light
  using neither `illuminate` nor `solar` has no direction to test against a cone, which is
  exactly what makes it ambient and what an illuminance loop cannot reach.
- **A run is a batch, and a batch of one is not a special case.** `sl::runtime::Machine` runs a
  flat program under a stack of execution masks: a condition every point agrees about is a
  jump, one they disagree about runs both arms with the lanes that took each, and `break`,
  `continue` and `return` clear lanes rather than jumping. moya's batch is a micropolygon grid,
  talyn's is one hit, and an imager's is a row of pixels.
- **A call is inlined.** A run has no register file per call and no call stack, so a shader's
  own function is pasted in where it was called, bracketed so that a `return` inside it means
  the lanes are done with the function rather than with the shader. Recursion is rejected at
  the call graph, which is what makes pasting terminate.
- **L points from the point being shaded toward the light**, in a light shader's `illuminate`
  and in a surface shader's `illuminance` body alike. RI's own text can be read both ways and
  the two ends have to agree, so it is fixed here: a cosine falloff is `L . N` and nothing in
  the library negates it. A `.sl` file written against the other reading renders its lights
  inside out.
- **A shader's own space is where the scene instanced it.** `sl::Placed` is an instance and
  that transform, and the declared defaults are *run* through the renderer's space table rather
  than remembered - which is what makes `point "shader" (0, 0, 1)` in the standard lights aim
  where the scene put them. A position a scene binds is stated in the same space.
- **The two renderers' current spaces differ.** moya works in camera space and talyn in world
  space, which is why the space table is a callback each implements rather than a constant the
  library holds. `"object"` is the one moya declines to answer: it is the transform at the
  primitive rather than at the shader, and a primitive does not carry one.
- **`transmission()` is where a shadow lives**, and the three standard directional lights call
  it. A renderer that cannot answer lets all the light through, so that one call is the whole
  of the difference between moya, which draws what it drew before, and talyn, which traces. A
  ray leaving a surface is offset along the geometric normal and toward the light: started on
  the surface it meets the surface it left, and every lit pixel comes out black in a pattern
  that reads as a normal fault rather than a numerical one.

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
