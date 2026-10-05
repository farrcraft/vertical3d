# The Offline Renderer

`moya`, a RenderMan renderer, is the *other* renderer, and shares nothing with the realtime
stack. It is a library, a driver and a suite: `moya/libmoya`, `moya/moya` and `moya/tests`.
What it is built on lives in `api/render/offline` (`v3dlib_render_offline`, namespace
`v3d::render::offline`), per [ADR-0022](adr/0022-offline-rendering-shares-an-api-library.md):
the RIB reader, the shading language, the film and the sampler, and the ray tracer.

That library **names neither Vulkan nor SDL**, and moya touches no window, device or swapchain,
so its suites render in CI where everything below the recorder in `api/render` cannot.

## Hiders

**A hider is how moya decides what the camera sees, and a scene picks one with `Hider`**, per
[ADR-0078](adr/0078-one-offline-renderer-with-two-hiders.md). Both read one graphics state and
write one framebuffer, so everything below about shading and sampling holds under either -
**except opacity: the reyes hider's samples are opaque.** A shader's `Oi` is bound and read the
same under both, but only the ray hider composites by it, so a translucent surface is translucent
under `"raytrace"` and opaque under `"hidden"`. Honouring it under reyes needs each sample to keep
every surface it meets and composite them in depth order, and [TODO.md](TODO.md) holds that.

- **A hider is a `moya::Hider`**: whether it sees the traced scene alone, and how it renders
  what the context gathered. `RenderContext` holds one and asks it both.
- **`"hidden"` is `moya::ReyesHider` and RI's default.** Primitives are split, diced into
  micropolygon grids, shaded a grid at a time and hidden into the samples of their buckets.
- **`"raytrace"` is `moya::RayHider`.** It casts a primary ray through every sample into the
  traced scene and buckets nothing. A ray is found by inverting the camera to raster
  transformation the reyes hider projects through - solving for x and y at a depth rather than
  inverting the whole matrix, whose depth terms RI's default clipping range ruins - so it takes
  any camera the reyes hider takes, an off-centre `ScreenWindow` included. Rays start on the
  near plane.
- **A hider name moya does not know is logged and leaves the hider as it was.**
- **The ray tracer is shared by both**, per
  [ADR-0077](adr/0077-one-ray-tracer-both-renderers-reach.md): `offline::trace::Scene` holds
  its primitives in world space as one list, and `offline::trace::Tracer` traces rays through it
  and shades what they meet. A primitive is anything with an `intersect` and a `describe` -
  triangles and spheres are two - so a new kind is a new class and the scene does not change.
  Each hit is shaded by a `trace::HitShader` made for it on the stack, which answers the
  shader's callbacks; a hit a shader traces into gets its own, so nothing is saved and restored
  around a traced ray.
  Every primitive a scene gives is added to it in world space as it arrives. The reyes hider
  reads it for `trace()` and `transmission()`, and the ray hider for everything.
- **The depth plane is raster space z under either hider**: the projection, then the raster
  matrix, which is what a reyes sample is hidden by. A ray's hit is carried back into camera
  space and projected the same way.
- **A traced primitive takes its primitive's colour rather than a varying `"Cs"`.** The reyes
  hider interpolates `"Cs"` across a grid, so a scene that colours its vertices differs between
  the two.

No plan is open against it. [The roadmap](roadmap/completed/OfflineRendering.md) is complete,
[OfflineRenderingPhase3](plans/completed/OfflineRenderingPhase3.md) is the account of how the
shading language got here, and [OfflineRenderingPhases4To6](plans/completed/OfflineRenderingPhases4To6.md)
of sampling, recursion and the shared ray tracer. What is left is in
[TODO.md](TODO.md#offline-rendering).

## Building

**`api/render/offline` is added by `api/CMakeLists.txt`** from the library manifest, as every api
library is, and so inherits nothing from the `api/render` directory above it. It sets `/utf-8` in
its own CMakeLists for that reason: spdlog's bundled fmt has a `static_assert` that fails without
it.

## Framebuffers and raster space

- **The plane count is not the channel count.** `offline::FrameBuffer` is a stack of float
  planes. `image(channels)` takes the leading planes as the picture and leaves the rest to the
  renderer. moya's are RGB, a depth and a coverage, named by `moya::FrameBuffer::Plane`, under
  either hider.
- **Coverage is what an imager reads as `alpha`**, and it is the difference between a pixel
  nothing was drawn into and a black one. It is the filtered fraction of a pixel's samples that
  hit, so it is one or nothing only at one sample per pixel under a one pixel box.
- **moya's raster space counts y downward from the upper left**, which is RI's convention and
  `image::Image`'s row order. The composition is `raster * screen`, since a matrix applies to
  what is on its right. Reversing either would write a correct render upside down, or in eye
  units.
- **A `RenderContext` writes a file only when `RiDisplay` named one with type `"file"`.** The RI
  token table in `RenderMan.cxx` is `RtToken`, that is, pointers, and most of it is still
  uninitialised. A null token reaches the context as an empty string rather than as an error.

## Sampling

A pixel is a filtered set of seeded samples, per
[ADR-0076](adr/0076-a-pixel-is-a-filtered-set-of-seeded-samples.md). `offline::Sampling` is what
`PixelSamples`, `PixelFilter`, `PixelVariance`, `Shutter` and `DepthOfField` asked for, starting at
the RI defaults; `offline::Sampler` gives a pixel's samples; `offline::Film` filters them into
pixels and resolves into a renderer's planes. Both hiders render through them.

- **The RI defaults are two by two samples under a gaussian two pixels wide**, so a scene that
  names nothing is antialiased and four times slower than one sample a pixel. A reference that
  pins hiding and shading names `PixelSamples 1 1` and `PixelFilter "box" 1 1`, in code and in
  its `.rib`, and `reference-sampled.png` pins the defaults.
- **An axis with one stratum is sampled at the pixel centre**, not jittered. That is what makes
  one sample under a one pixel box give back exactly the picture a renderer drew at its pixel
  centres, so a reference pinned that way survives a change to the sampler.
- **A miss carries a colour into the film.** Under the ray hider it is
  `trace::Scene::background()`, which is black unless something sets it, so a filtered colour
  is premultiplied in the ordinary case. Coverage counts only hits either way.
- **moya hides into a sample store the size of the frame**, `moya::Samples`, and filters it
  through the film once the last bucket is done. A sample is not finished until every grid that
  could reach it is hidden, and the sweep comes round again when a split lands behind it, so no
  bucket is finished before the sweep is. Where the buckets fall therefore never shows.
- **The reyes hider tests a sample against the micropolygon**, as two triangles, and interpolates
  its depth there, rather than filling the micropolygon's raster bound.
- **A lens blurs only through a perspective camera.** `Sampling::lensRadius()` is
  `focalLength / (2 * fstop)` in camera space units, and an orthographic camera has no lens to
  move, so it ignores `DepthOfField`. The lens is at the eye. The ray hider moves a sample's
  ray's origin across it and aims it at the point it would have reached on the plane of focus.
  The reyes hider moves a micropolygon instead, by
  the sample's lens point times `1 - z / focalDistance` in eye x and y, which leaves the depth
  and so the divide alone: a corner's raster position is linear in the lens point, so a
  micropolygon is projected three times and every sample's corners are a sum of those. Its
  bound is grown to the lens's four extremes so every sample it can reach is tested.
- **Only a transform moves.** `offline::MovingTransform` is the current transformation at a
  motion block's two ends: inside a block each transform request applies to its own copy of
  the transformation the block found, and outside one a request applies to both ends. Between
  them translation and scale are interpolated linearly and rotation by a quaternion. A primitive
  repeated inside a block would deform, which moya does not build: the reader hands the later
  poses to `Handler::deformation()`, which is null unless a renderer overrides it, and then lists
  them in `Reader::unsupported()`. What a renderer can build is the handler's to say, as an
  area light is - `Handler::areaLightSource` is an ordinary light unless it is overridden.
- **The traced scene stores a moving triangle where the open end put it**, and
  `trace::Scene::nearest()` takes the poses at a time: a ray is carried back into that pose and its hit is carried forward
  again. The `Tracer` works the poses out once for the sample's time, so its shadow and traced rays look at the same moment
  without inverting a motion per ray.
- **moya places a moving micropolygon per sample.** A primitive carries its moving object to eye
  transformation, and the hider moves the eye space corners from the open end to a sample's
  time. Its bound is where it is at eight slices of the shutter, grown by the furthest a corner
  moves in one, and a sample is rejected by its own slice's bound before anything is placed. A
  moving primitive is culled by its bound at both ends, and measured for splitting at the open
  end alone, since a split shrinks a primitive and never the distance it travels.
- **The ray hider adapts its sample count and the reyes hider does not.** A `PixelVariance`
  above zero has the ray hider take another seeded set wherever the variance of a pixel's mean
  is above it, up to four times the first set; `RenderContext::samplesTaken()` says how many a
  pixel took. A reyes hider samples a whole bucket at once, and adapting per pixel would split a
  grid's hiding in two, so it takes the count `PixelSamples` names, per
  [ADR-0076](adr/0076-a-pixel-is-a-filtered-set-of-seeded-samples.md).
- **A depth is not filtered.** It is the nearest hit among the samples inside the pixel, since a
  blend of two surfaces' depths is a depth neither is at.
- **The filters are RI's formulas**, cut off at the width a scene gives. Catmull-rom peaks at two
  and has a negative lobe, so a pixel's weights can sum to nearly nothing; the film answers black
  there rather than dividing by it.

## RIB

**The reader is a library of its own.** `offline::rib::Reader` dispatches onto
`offline::rib::Handler`, a C++ interface with typed parameter lists rather than the RI C ABI, per
[ADR-0025](adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md), and moya implements
it as `moya::RIBHandler`. RIB is also what the editor exports to, one way, per
[ADR-0023](adr/0023-rib-is-the-offline-scene-description.md).

- **Every method has an empty body rather than being pure virtual**, because the RI standard
  asks a renderer to accept a request it does not support. A misspelled override is therefore
  silent, so every override carries `override`.
- **A matrix crosses the RIB boundary by being read in order, not transposed.** RIB writes row
  major under a row vector convention and glm stores column major under a column vector one, so
  `glm::make_mat4` over the sixteen floats *is* the conversion. This applies to `Transform`,
  `ConcatTransform`, `RtMatrix` and the editor's export alike.
- **A `PixelFilter` the reader does not know is reported and never reaches the handler**, so a
  renderer keeps the filter it had. A `PixelSamples` rate reaches it as a count of at least one.
  RIB's `DepthOfField` with no arguments is a pinhole, sent as an infinite fstop.
- **A RIB `Polygon` carries no vertex count.** It is the length of `"P"`, which the reader
  divides out. A parameter list is therefore parsed before the count is known, and an
  unbracketed varying or vertex parameter ends the parse rather than being guessed at.
- **A parameter is typed by a declaration or it is dropped.** The reader declares RI's standard
  names, `Option "trace"`'s `maxdepth` and `Option "searchpath"`'s `shader` and `texture` among
  them, and nothing else. A shader's own parameter that is not one of those needs `Declare` or an inline
  type, `"uniform float size" [0.5]`, or the shader runs with its default.
- **`MakeTexture` is understood and moya makes nothing**, which is `Handler::makeTexture`'s
  default. There is no `txmake`: the image a scene
  names in a shader is the texture, read through `image::Factory`, so a scene that converts one
  first should name the image rather than what it converted it to.
- **A polygon's `"st"` is its texture coordinates**, two floats a vertex. A traced hit weights
  a triangle's three by its barycentrics, and a triangle given none has `s` and `t` equal to
  them. The reyes hider interpolates them across a grid as it does `"Cs"` and carries them
  through a split; a grid whose corners have none takes its own parameters.
- **The ray hider intersects a `Sphere` where it is defined**, cut to its slab of heights and
  its sweep, with RI's outward normal and its `u` and `v`. The reyes hider dices polygons only:
  a sphere is not drawn, and the first in a scene is logged.

## Cameras and transforms

- **moya's world-to-camera transform applies as it stands.** `prepareWorld` saves the current
  transformation as the camera coordinate system, and by the RI standard that transformation
  *is* the world to camera one. A transpose or an inverse is right only when it is a rotation.
- **Both hiders see through the same camera coordinate systems**, so a camera one accepts the
  other does: an off-centre `ScreenWindow`, a matrix that scales, and one that reverses
  handedness, which RI's camera basis for a general lookat produces.

## The shading language

Shading is a language rather than a set of built-in models, per
[ADR-0026](adr/0026-shading-is-a-language-over-a-batch.md), and it lives in
`api/render/offline` beside the RIB one: `sl::Lexer`, the `sl::syntax` nodes and `sl::Parser` read a shader,
`sl::Types` and `sl::Compiler` check it, `sl::Emitter` flattens it into a `sl::runtime::Program`,
and `sl::runtime::Machine` runs that over a batch. `sl::ShaderLibrary` maps a name to a program
and `sl::Instance` binds a scene's parameters onto one. What follows is where the seams are
rather than a tour.

- **The `SL*` files are to a `.sl` file what the `RIB*` ones are to a `.rib` file**, and are
  shaped the same way on purpose - a `peek`/`next` lexer over an `std::istream`, an `error()`
  that ends the stream, and a line and column on every token. What is the same is shared:
  `offline::Characters` reads the stream with its position and decodes a string's escapes, and
  each language's `Token` is an `offline::Lexeme` over its own kinds.
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
- **Which children a node has is written down once.** `syntax::forEachChild` hands a node's
  expressions and statements to a visitor in source order, so a walk that treats most kinds
  alike - gathering what a shader calls, joining what an expression reads - names only the
  kinds it cares about, and a new kind of node is one case there.
- **The varying inference is `sl::Inference`, over the tree the checker annotated, and runs to a
  fixed point**, because a loop carries a varying value back
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
  `continue` and `return` clear lanes rather than jumping. A grid's batch is its micropolygons,
  a traced hit's is one point, and an imager's is a row of pixels.
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
- **Both hiders bind the same globals.** A hider fills an `sl::Point` per shading point and
  `sl::Globals`, resolved once per program, writes it into the batch and reads `Ci` and `Oi`
  back, and `Globals::shine` runs a light over a batch, so there is one list of what a shader
  is given rather than one per hider. `du` and `dv` are zero for a traced hit, which has no
  neighbouring point to difference.
- **A grid and a traced hit have different current spaces.** `GridShader` works in camera space
  and a traced hit in world space, which is why the space table is a callback each
  implements rather than a constant the library holds. `"object"` is the one a grid declines to
  answer: it is the transform at the primitive rather than at the shader, and a primitive does
  not carry one.
- **A traced primitive carries the lights that were on when it was made**, as one set shared by
  every primitive made until they change. A primitive given none, which is how a scene built in
  code is lit, is shaded by the scene's own list.
- **`transmission()` is where a shadow lives**, and the three standard directional lights call
  it. Both hiders answer it from the shared ray tracer; a renderer that cannot lets all the
  light through, so that one call is the whole of the difference between a shadowed scene and an
  unshadowed one. A ray leaving a surface is offset along the geometric normal and toward the light: started on
  the surface it meets the surface it left, and every lit pixel comes out black in a pattern
  that reads as a normal fault rather than a numerical one.
- **`trace()` goes as deep as `Option "trace" "maxdepth"` says**, two by default, and past it
  answers the background. The `Tracer` keeps a machine per program per depth, because a surface
  tracing into another with the same shader is still part way through its run when the other
  starts.
- **A grid traces through the shared scene.** `GridShader` carries a shading point's ray from
  camera space into world space, started off the grid's plane. A grid is shaded once for all of
  its samples, so under the reyes hider its rays look at the scene at the shutter's open, where
  the ray hider's look at each sample's time.
- **A light shader runs in its own space.** While a light runs, `"shader"` is the light's
  placement rather than the surface's, which is what puts a `pointlight`'s default `from` where
  the scene placed it.
- **A traced ray composites what it passes through**, front to back by each surface's `Oi`, with
  the background behind what is left. A shader that never writes `Oi` is as opaque as its
  primitive's `Os`. A ray carrying on through a surface is not a traced ray and does not count
  against the depth. `transmission()` multiplies by every occluder's `Os`, read off the
  primitive rather than by running its shader, so a shadow ray is never a shading one.
- **A built-in is declared with what runs it.** Each `Signature` names its `Body`, and the
  machine dispatches on that rather than on the name, so a function cannot be declared without
  saying what it does. `SOURCE` is one written in the language and inlined; `STUB` is one the
  machine answers with its default and reports once.
- **A built-in may answer through its arguments.** `Signature::outputs` names the first one it
  writes. The compiler requires a variable there and spreads the storage of what the call read
  into it, as an assignment would. `fresnel` is the one that does: the unpolarised reflectance
  of a dielectric, with `refract`'s conventions, and the reflected and refracted directions.
- **`shinymetal` traces where RI's reads an environment map**, and **`glass`** is this tree's,
  since RI has no refracting shader. glass sets `Oi` to one, because it shows what is behind it
  by refraction, and its `Os` is what a shadow through it reads. **`paintedplastic`** is RI's,
  multiplying `Cs` by the texture its `texturename` names.
- **`texture()` is an image the renderer holds for the frame.** `offline::Textures` reads each
  name once, looking on `Option "searchpath" "texture"` and then at the name as it is, and
  remembers a name that does not read; the machine answers black for it and says so once.
  `offline::Texture` samples bilinearly between texel centres and wraps periodically, RI's
  defaults, with `t` running down the image. Called with only a name, it reads at the shader's
  `s` and `t`.
- **`noise()` is Perlin's improved noise in SL's range**, `[0, 1]` and `0.5` on the lattice, over a
  permutation shuffled by a `type::Random` of a fixed seed, so a pattern is the same on every
  machine. Its float, pair and point forms read a line, a plane and a volume of it.
- **A cast chooses between built-ins that differ only in their result.** The compiler takes the
  first signature that accepts a call unless the call is a cast's operand and a later one answers
  the cast's type: `color noise(P)` is three patterns rather than a grey one, and
  `float texture(name)` is the first channel. Without a cast, `noise` is a float and `texture` a
  colour.
- **Two strings compare by their text**, which is how `paintedplastic` asks whether it was given
  a texture at all. `shadow` and `calculatenormal` are the built-ins left declared and stubbed.

## Normals

Both hiders carry two, because SL's `faceforward` and `calculatenormal` are defined in terms
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
- `trace::Triangle` has a constructor per case, and `shadingNormal(u, v)` interpolates over the
  barycentric coordinates `type::Ray::intersects` reports - `u` weighs `b` and `v` weighs `c`, so
  `a` carries the rest. The overload that reports them exists because Moller-Trumbore solves for
  them on its way to the distance and the four argument form throws them away.

## Reyes

**A `ReyesPrimitive` carries the transform, colour and geometric normal it was submitted under.**
Splitting resubmits pieces through the first pass during the second one, when none of it is
current, and a split builds its pieces from intersection points that carry no colour and no
normal at all.
