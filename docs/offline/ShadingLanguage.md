# The shading language

How a shader written in the RenderMan Shading Language is compiled and run over a batch of
points, the built-in shaders and functions, and how normals are found.

## The shading language

Shading is a language, not a set of built-in models. A scene names a shader with `Surface`,
`LightSource` or `Imager`, and moya compiles that shader from SL source the first time it is
named. The language is in `api/render/offline/sl`.

### From source to a run

| Stage | Class |
|---|---|
| Lexing | `sl::Lexer` |
| Parsing into a syntax tree | `sl::Parser`, building `sl::syntax` nodes |
| Type checking | `sl::Types`, `sl::Compiler` |
| Uniform and varying inference | `sl::Inference` |
| Flattening into instructions | `sl::Emitter`, producing a `sl::runtime::Program` |
| Running over a batch | `sl::runtime::Machine` |
| Finding a shader by name | `sl::ShaderLibrary` |
| Binding a scene's parameters | `sl::Instance` |
| Writing the globals into a batch | `sl::Globals`, filled from an `sl::Point` per shading point |

- **The SL lexer matches the RIB lexer.** Both are a `peek` and `next` lexer over an
  `std::istream`, with an `error()` that ends the stream and a line and column on every token.
  The shared parts are in `api/render/offline`: `offline::Characters` reads the stream with its
  position and decodes string escapes, and each language's `Token` is an `offline::Lexeme` over
  its own kinds.
- **The keywords are a closed set**: the five shader types, the eight data types, the two
  storage classes, the control flow keywords and the three lighting constructs. Any other name
  is an identifier, so a shader may declare a variable called `output` or define its own
  `noise`.
- **`.` is a dot product and `^` is a cross product**, and both bind tighter than `*`. Check
  this before assuming an expression means what it looks like.
- **All five shader types parse, and four run.** A `volume` shader parses and returns false from
  `sl::syntax::Shader::supported()`. A scene that names one is told the shader is unsupported,
  not that it is malformed.
- **A displacement shader writes `P` and `N`.** It reads the surface's other position globals,
  `Ng`, `I`, `E`, `s`, `t`, `u`, `v`, `du` and `dv`, and nothing a surface writes, such as `Ci`.
  A scene names one with `Displacement`.
- **The C preprocessor is not run.** A `#` produces a diagnostic naming the missing
  preprocessor. There is no compiled-shader file: a shader is source, compiled on first use.
- **The compiler annotates the syntax tree in place.** Every expression gets a type and a
  storage class, and every variable gets a symbol index. `sl::Compiler::symbols()` is the list
  a machine allocates registers against. A local declared twice in nested scopes is two
  symbols.
- **`syntax::forEachChild` lists a node's children in source order.** A pass that treats most
  node kinds alike handles only the kinds it needs. A new kind of node needs one case there.
- **The varying inference runs to a fixed point**, because a loop can carry a varying value back
  to a name that was read before it was written. Anything assigned under a varying condition is
  varying. A value declared `uniform` that a varying value reaches is an error, not a silent
  widening. Inferring uniform where varying was correct gives a whole grid one point's answer.
  It looks like a shading bug and is a compiler bug. A `break` or `continue` under a varying
  condition makes the body of the loop it leaves varying. Inside an `illuminance` body, that
  loop is the `illuminance`, not a loop around it.
- **A run covers a batch, and a batch of one is not a special case.** The machine runs a flat
  program under a stack of execution masks. A condition all points agree on compiles to a jump.
  A condition they disagree on runs both branches, each with the points that took it, and the
  condition is read once. `break`, `continue` and `return` clear mask bits instead of jumping.
  An `illuminance` body is a loop over the lights, so a `break` or a `return` inside it ends it
  for those points and they run it for no later light. A grid's batch is its vertices, a
  traced hit's batch is the hit and its two neighbours, and an imager's batch is a row of
  pixels.
- **A function call is inlined.** A run has no call stack and no register file per call. A
  shader's own function is pasted in at each call, bracketed so that a `return` inside it ends
  the function for those points, not the shader. Recursion is rejected at the call graph, so
  inlining terminates.
- **An `output` formal is passed by reference.** What the function leaves in it is copied back
  to the caller's variable when the function returns. The argument has to be a variable the
  caller may assign, of the formal's own type, since a coercion on the way in has no way back.

### Shaders, lights and spaces

- **A shader is found by name.** A `.sl` file on `Option "searchpath" "shader"` wins over a
  built-in shader of the same name, so a scene can replace one. A file holding exactly one
  shader may have any file name: RI identifies a shader by the name in its source.
- **A search path is colon-separated, and `&` stands for the previous path.** A single letter
  before a colon is a Windows drive letter, not the end of a directory.
- **A shader that fails to parse or compile is logged by name and position**, and the failure
  is cached, so a broken shader on many primitives is reported once. A failed surface is
  replaced by `matte`. A failed light is left out. A shader named as the wrong type, such as a
  light given to `Surface`, is reported and replaced the same way.
- **A surface with no `Surface` request uses `constant`**, its own colour unlit.
- **`L` points from the shaded point toward the light**, in a light shader's `illuminate` and in
  a surface shader's `illuminance` body alike. A cosine falloff is `L . N`, and nothing in the
  library negates `L`. A `.sl` file written for the opposite reading renders its lights inside
  out.
- **A shader's `"shader"` space is where the scene instanced it.** `sl::Placed` is an instance
  and that transformation. Declared defaults are evaluated through the renderer's space table,
  not stored as numbers, so `point "shader" (0, 0, 1)` in the standard lights aims where the
  scene placed the light. A position a scene binds is in the same space.
- **A light shader runs in its own space.** While a light runs, `"shader"` is the inverse of
  the light's placement, not of the surface's. This places a `pointlight`'s default `from` where
  the scene put the light.
- **Both hiders bind the same globals.** A hider fills one `sl::Point` per shading point.
  `sl::Globals`, resolved once per program, writes the points into the batch and reads `Ci` and
  `Oi` back. `Globals::shine` runs a light over a batch. `du` and `dv` are the step to a
  neighbouring point: one over a grid's span, or a traced hit's stencil step.
- **A grid and a traced hit have different current spaces.** A grid is shaded in camera space
  and a traced hit in world space. The space table is therefore a callback each renderer
  implements (`sl::runtime::Renderer::space`), not a constant in the library.
- **`"object"` is the primitive's space**, as the transformation stood when the primitive was
  submitted. It differs from `"shader"` when a scene transforms between `Surface` and the
  primitive. A reyes primitive carries it in its `Shading`, and a traced primitive carries it
  as `trace::Primitive::object()`, so both hiders give the same answer. A moving primitive
  answers with the pose it is stored in.
- **`Renderer::space(name)` maps current space into the named space**, as RenderMan defines
  it. Every space follows that direction, `"shader"` included: it is the inverse of the
  shader's placement, since the placement maps the shader's own space into current space.
- **`ptransform("space", P)` maps `P` from current space into the named space**, as RenderMan's
  `transform("space", P)` does. `vtransform`, `ntransform` and `mtransform` take the same
  direction. The two-space forms map out of the first space and into the second.
- **A cast states its value in the named space.** `point "world" (0, 0, 0)` is the world
  origin expressed in current space, so a cast applies the inverse of `space(name)`. A vector
  cast drops the translation, and a normal cast takes the inverse transpose of that inverse.
- **NDC is raster space divided by the resolution.** x runs right and y runs down over
  [0, 1], from the upper left corner of the picture, as raster does. Its z is screen depth
  moved from [-1, 1] onto [0, 1], the value `depth()` returns.
- **A point into or out of a projected space is divided by w.** `"screen"`, `"raster"` and
  `"NDC"` are the projected spaces, as `Renderer::projected(name)` answers. A perspective
  projection writes the eye depth into w, so `ptransform`, `depth()` and a point cast divide by
  it, with either end of a two-space form projected. A point whose w is at or below 1e-6 is
  left undivided. `sl::project` is that divide, and moya's own projection calls it. A vector or
  a normal is not divided, because it has no position.
- **Both hiders answer the projected spaces from one camera.** `trace::Scene::projection()`
  holds the camera to screen, raster and NDC matrices, which moya sets when the world begins.
  A grid and a traced hit both read them, so a shader that reads a projected position gets the
  same value under either hider.

### Built-in shaders

The standard shaders are SL source compiled into the library, so `Surface "matte"` works with no
files on disk.

| Kind | Shaders |
|---|---|
| Surface | `constant`, `matte`, `metal`, `plastic`, `paintedplastic`, `shinymetal`, `glass` |
| Light | `ambientlight`, `distantlight`, `pointlight`, `spotlight`, `arealight` |
| Imager | `background` |

- **`paintedplastic`** multiplies `Cs` by the texture its `texturename` names. It tests whether
  it was given a texture by comparing strings, which compare by text.
- **`shinymetal` traces a reflection ray** where RI's version reads an environment map.
- **`glass` is this tree's own**; RI has no refracting shader. It sets `Oi` to one, because it
  shows what lies behind it by refraction. Its `Os` is what a shadow ray through it reads.
- **The directional lights** (`distantlight`, `pointlight`, `spotlight`, `arealight`) call
  `transmission()`, so they cast shadows. A `spotlight` tests its cone against `-L`, the
  direction its light travels.
- **`arealight` shines from its P**, evenly in every direction with the square of the distance,
  as `pointlight` shines from its position. It is the shader an `AreaLightSource` names.
- **An area light shines from points sampled on its geometry.** Its light shader runs once for
  each of its samples, with P the sample, and the illuminance loop sees each sample as a light
  of its own. Each sample carries `1 / nsamples` of the light, so the count changes the noise
  and not the brightness. `sl::shining()` and `sl::which()` count a light list this way for
  both hiders.
- **A shading point's samples are seeded by the light's handle, the point's position in world
  space and the sample's index** (`Area::point()`). A point gets the same samples on every run,
  in any order of buckets or pixels. A grid vertex is moved into world space first, so a vertex
  and a traced hit at the same position sample a light alike.

  Background: [ADR-0094](../adr/0094-offline-area-lights-sampled-per-shading-point.md)
- **`background`** adds its colour behind each pixel in proportion to `1 - alpha`, then sets
  `alpha` to one, so a pixel it painted is no longer empty. A scene can use it instead of a
  backdrop polygon.

### Built-in functions

- **Each built-in's `Signature` names its `Body`**, and the machine dispatches on the body, not
  the name. A function cannot be declared without saying what implements it.
  - `SOURCE` is written in SL and inlined.
  - `STUB` returns its default value and is reported once. `shadow` is the one stub.
  - Every other body is implemented in C++ in the machine.
- **`diffuse`, `specular` and `phong` are SL source** in `sl::Builtins`, written over
  `illuminance`, as the standard defines them. A shader that calls one has it copied into its
  own function list before anything else runs, so later stages see one kind of function. A
  shader's own definition of the name wins.
- **`Du`, `Dv`, `Deriv`, `area` and `calculatenormal` difference neighbouring shading points.**
  A derivative is the difference to the neighbouring point along u or v, divided by the step
  `du` or `dv` between them. The renderer says how its batch lies on the surface through
  `Renderer::shape()`. A grid's neighbour is the next vertex, or the previous one on the last row
  or column. A traced hit's neighbours are its stencil, described in
  [RayTracing.md](RayTracing.md). A batch of lone points has none, and every derivative there
  is zero.
  - `Du` and `Dv` take a float, a colour or a position. A position's derivative is a vector.
  - A uniform value's derivative is zero, because it is the same at every point.
  - `Deriv(num, den)` is RI's `Du(num) / Du(den) + Dv(num) / Dv(den)`. A term is dropped where
    `den` does not change in that direction, so `Deriv(f, u)` is `Du(f) / Du(u)`.
  - `area(P)` is the area of the patch one step along u and one along v span: the length of
    `(Du(P) * du) ^ (Dv(P) * dv)`.
  - `calculatenormal(P)` is `Du(P) ^ Dv(P)`. On a diced polygon it points the way `Ng` does,
    because dicing runs u along the first edge and v along the last.
  - A derivative is first order, so on a curved surface it is as good as the grid is fine.

  Background: [ADR-0093](../adr/0093-offline-derivatives-from-neighbouring-shading-points.md)

- **`ambient()` is C++.** An ambient light uses neither `illuminate` nor `solar`, so it has no
  direction, and an `illuminance` loop cannot reach it.
- **`solar` with an angle chooses `L` inside its cone, nearest the surface's illuminance axis**,
  as RenderMan does. An axis inside the light's cone is `L` itself, and an axis outside it gives
  the cone's edge nearest it, with the length of the light's axis. An illuminance loop hands its
  cone to each light it runs, as `runtime::Cone`, through `Renderer::light` and
  `Globals::shine`, and the light's machine reads it from `Machine::gather`. With no cone, as
  under `ambient()`, the light shines along its axis.
- **A machine's reports reach the log.** A report says what a run could not do, such as a space
  no renderer named or a stub that was called. A report made while the shader runs starts with
  the line and column of the instruction that made it, as `line 3, column 9: `. Each distinct
  report, position included, is logged once per machine as a warning, through the logger of
  the shader instance that wrote the machine's parameters. One fault at two places in the
  source is therefore reported twice. A shader whose parameter defaults fail to run is reported
  in the same way, once per machine, and the renderer treats it as a run that failed.
- **A run that fails is reported by the machine.** `Machine::run()` returns false and reports
  `error()`, such as a shader that runs without end. No renderer logs it again. A grid or a
  traced hit whose surface fails is drawn as if it had no shader. A light that fails lights
  nothing.
- **A failed imager stops at the row that failed.** The rows above it are shaded, and that row
  and the rows below it keep what the hider wrote. moya still writes the frame.
- **`==` and `!=` compare every component** of a colour, point, vector, normal or matrix. A
  float compared with one is promoted as an assignment promotes it, so a float against a matrix
  is the diagonal matrix.
- **A float bound onto a matrix parameter is the diagonal matrix**, as an assignment promotes
  it. A float bound onto a colour, point, vector or normal fills every component.
- **Matrices follow RenderMan's row vector convention.** A matrix literal is row major, and its
  last row is the translation. A point is a row vector on the left, so `A * B` applies `A` and
  then `B`. `A / B` is `A` times the inverse of `B`. `translate`, `rotate` and `scale` apply their
  own transform before the matrix they are given, as `ConcatTransform` does. `mtransform`
  applies its matrix and then the change of space. In the machine, glm's column vector
  matrices hold the same sixteen floats, so each product is written in the reverse order.
- **A built-in may return results through its arguments.** `Signature::outputs` names the first
  argument it writes, and every one from it on is written. `Signature::updates` names one
  argument that the built-in reads and then writes in place. `setxcomp`, `setycomp`, `setzcomp`
  and `setcomp` each update their first argument. The compiler requires a variable there that
  the shader may assign, and propagates the storage class of the call's inputs into it, as an
  assignment would. `fresnel` returns the unpolarised reflectance of a dielectric, with
  `refract`'s conventions, and writes the reflected and refracted directions.
- **A shader reads another's `output` parameter through message passing**, as RI defines it.
  `surface("name", v)`, `displacement("name", v)` and `lightsource("name", v)` copy that
  shader's parameter into `v` and return 1, or return 0 and leave `v` as it was. The parameter
  has to be declared `output` and be of `v`'s type.
  - A light reads `surface()`: the surface being shaded, as its values stand when the light
    runs.
  - A surface reads `displacement()`: the displacement shader that ran on the same grid. A
    traced hit has none to read, because a traced primitive was displaced when it was diced.
  - A surface reads `lightsource()` inside an `illuminance` body: the light the body is running
    for.
  - The renderer answers through `runtime::Renderer::message`, with `Machine::output()` reading
    the other machine. `"surface"` and `"displacement"` are keywords, and inside a shader's body
    the parser reads them as these calls.
- **The componentwise maths takes a float or a triple.** `abs`, `sign`, `floor`, `ceil`,
  `round`, `sqrt`, `exp`, one-argument `log`, `mod`, `pow`, `min`, `max`, `clamp` and `mix`
  apply the same function to each component. A float argument is read for every component.
  The result is the type the arguments promote to, as an arithmetic operator's is
  (`Signature::promotes`). `max(0, Ci)` is a colour, and `max(Cs, P)` is rejected as `Cs + P`
  is. The trigonometry and the angle conversions take floats only.
- **A space in front of a cast depends on the type.** On a point, vector or normal it is a
  coordinate space, which the renderer resolves when the shader runs. On a colour it is a
  colour space: `color "rgb" (...)` is the colour as given, and any other space compiles to
  `ctransform(space, "rgb", ...)`. A space on a matrix is rejected at compile time, because
  RenderMan's meaning for it is not implemented. A space on a float or a string is rejected.
- **`ctransform` implements one colour space, `"rgb"`.** Any other space it is given, whether to
  convert from or into, is reported once and the colour is returned unchanged.
- **A cast chooses between built-ins that differ only in their result type.** The compiler takes
  the first signature that accepts a call, unless the call is the operand of a cast and a later
  signature returns the cast's type. `color noise(P)` is three patterns, not one grey one, and
  `float texture(name)` is the first channel. Without a cast, `noise` returns a float and
  `texture` a colour.
- **`texture()` reads an image held for the frame.** `offline::Textures` reads each name once,
  looking on `Option "searchpath" "texture"` and then at the name as given. A name that fails
  to read is remembered as missing. The machine returns black for it and logs it once.
  `offline::Texture` samples bilinearly between texel centres and wraps periodically, RI's
  defaults, with `t` running down the image. A coordinate that is not finite reads black. A grey
  image fills all three channels and an alpha channel is dropped. Called with only a name,
  `texture()` reads at the shader's `s` and `t`.
- **`noise()` is Perlin's improved noise in SL's range**: `[0, 1]`, and `0.5` on every lattice
  point. Its permutation is shuffled by a `type::Random` with a fixed seed, so a pattern is the
  same on every machine. Its float, pair and point forms read a line, a plane and a volume of
  it. The lattice repeats every 256 cells, at any distance from the origin. A point that is
  not finite reads `0.5`.

Background: [ADR-0026](../adr/0026-offline-shaders-run-over-batches-of-points.md)

## Normals

Both hiders carry two normals, because SL's `faceforward` and `calculatenormal` are defined in
terms of the pair:

- **`Ng` is the geometric normal**: the plane the primitive lies in, one value across it, wound
  the way its vertices are. A polygon wound clockwise as the camera sees it has an `Ng` that
  faces the camera.
- **`N` is the shading normal.** A scene sets it per vertex with a varying `"N"`. Without one,
  `N` equals `Ng`, and the surface is faceted.

Rules:

- **Two-argument `faceforward(N, I)` turns `N` by `Ng`**, as RI defines it: the compiler supplies
  `Ng` as the third argument. A light shader has no `Ng`, so the two-argument form is refused
  there and the reference has to be given. A scene that gives `"N"` facing the camera has to
  wind its polygons so that `Ng` faces the camera too. Otherwise `faceforward` turns the normal
  away and the surface is shaded from behind.
- **A normal transforms by the inverse transpose**, never by the matrix that moves the points.
  The two agree under a rotation and a uniform scale, so the fault stays hidden until a scene
  scales one axis.
- **moya's `Vertex` holds both.** `RenderContext::addPolygon` fills them from
  `Polygon::geometricNormal()` before transforming anything. The diceable branch moves both into
  eye space. Dicing interpolates `N` and renormalises it. `Ng` is copied, since there is one.
- **A split carries `N` across.** A corner of a piece keeps the normal of the vertex it came
  from. Where a cut crosses an edge, the new vertex's normal is interpolated along the edge and
  renormalised, so a surface large enough to split is smooth rather than faceted per piece. A
  vertex with no normal of its own, or on an edge with an end that has none, takes the
  primitive's plane, which each piece is given through `reyes::Primitive::place()`. That plane is
  also every piece's `Ng`.
- **`trace::Triangle` has one constructor per case**: with and without per-corner normals.
  `shadingNormal(u, v)` interpolates over the barycentric coordinates `type::Ray::intersects`
  reports: `u` weights corner `b`, `v` weights corner `c`, and `a` takes the rest. That overload
  of `intersects` exists because Moller-Trumbore computes the coordinates on its way to the
  distance, and the four-argument form discards them.
