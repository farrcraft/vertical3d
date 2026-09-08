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
