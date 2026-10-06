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
- **All five shader types parse, and three run.** A `displacement` or `volume` shader parses and
  returns false from `sl::syntax::Shader::supported()`. A scene that names one is told the
  shader is unsupported, not that it is malformed.
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
  It looks like a shading bug and is a compiler bug.
- **A run covers a batch, and a batch of one is not a special case.** The machine runs a flat
  program under a stack of execution masks. A condition all points agree on compiles to a jump.
  A condition they disagree on runs both branches, each with the points that took it, and the
  condition is read once. `break`, `continue` and `return` clear mask bits instead of jumping.
  An `illuminance` body is a loop over the lights, so a `break` or a `return` inside it ends it
  for those points and they run it for no later light. A grid's batch is its vertices, a
  traced hit's batch is one point, and an imager's batch is a row of pixels.
- **A function call is inlined.** A run has no call stack and no register file per call. A
  shader's own function is pasted in at each call, bracketed so that a `return` inside it ends
  the function for those points, not the shader. Recursion is rejected at the call graph, so
  inlining terminates.

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
- **A light shader runs in its own space.** While a light runs, `"shader"` is the light's
  placement, not the surface's. This places a `pointlight`'s default `from` where the scene put
  the light.
- **Both hiders bind the same globals.** A hider fills one `sl::Point` per shading point.
  `sl::Globals`, resolved once per program, writes the points into the batch and reads `Ci` and
  `Oi` back. `Globals::shine` runs a light over a batch. `du` and `dv` are zero for a traced
  hit, because a single point has no neighbour to difference.
- **A grid and a traced hit have different current spaces.** A grid is shaded in camera space
  and a traced hit in world space. The space table is therefore a callback each renderer
  implements (`sl::runtime::Renderer::space`), not a constant in the library. A grid has no
  answer for `"object"`, because that is the transformation at the primitive, and a primitive
  does not carry it.

### Built-in shaders

The standard shaders are SL source compiled into the library, so `Surface "matte"` works with no
files on disk.

| Kind | Shaders |
|---|---|
| Surface | `constant`, `matte`, `metal`, `plastic`, `paintedplastic`, `shinymetal`, `glass` |
| Light | `ambientlight`, `distantlight`, `pointlight`, `spotlight` |
| Imager | `background` |

- **`paintedplastic`** multiplies `Cs` by the texture its `texturename` names. It tests whether
  it was given a texture by comparing strings, which compare by text.
- **`shinymetal` traces a reflection ray** where RI's version reads an environment map.
- **`glass` is this tree's own**; RI has no refracting shader. It sets `Oi` to one, because it
  shows what lies behind it by refraction. Its `Os` is what a shadow ray through it reads.
- **The three directional lights** (`distantlight`, `pointlight`, `spotlight`) call
  `transmission()`, so they cast shadows. A `spotlight` tests its cone against `-L`, the
  direction its light travels.
- **`background`** adds its colour behind each pixel in proportion to `1 - alpha`, then sets
  `alpha` to one, so a pixel it painted is no longer empty. A scene can use it instead of a
  backdrop polygon.

### Built-in functions

- **Each built-in's `Signature` names its `Body`**, and the machine dispatches on the body, not
  the name. A function cannot be declared without saying what implements it.
  - `SOURCE` is written in SL and inlined.
  - `STUB` returns its default value and is reported once. `shadow` and `calculatenormal` are
    the two stubs.
  - Every other body is implemented in C++ in the machine.
- **`diffuse`, `specular` and `phong` are SL source** in `sl::Builtins`, written over
  `illuminance`, as the standard defines them. A shader that calls one has it copied into its
  own function list before anything else runs, so later stages see one kind of function. A
  shader's own definition of the name wins.
- **`ambient()` is C++.** An ambient light uses neither `illuminate` nor `solar`, so it has no
  direction, and an `illuminance` loop cannot reach it.
- **`solar` is lit along its axis.** An angle other than 0 would let `L` be any direction inside a
  cone, chosen against the surface's own `illuminance` cone, which a light shader is not given.
  The machine reports such an angle once and uses the axis.
- **A machine's reports reach the log.** A report says what a run could not do, such as a space
  no renderer named or an angle `solar` cannot honour. Each distinct report is logged once per
  machine as a warning, through the logger of the shader instance that wrote the machine's
  parameters. A shader whose parameter defaults fail to run is logged as an error, and the
  renderer treats it as a run that failed.
- **`==` and `!=` compare every component** of a colour, point, vector, normal or matrix. A
  float compared with one is promoted as an assignment promotes it, so a float against a matrix
  is the diagonal matrix.
- **A built-in may return results through its arguments.** `Signature::outputs` names the first
  argument it writes, and every one from it on is written. `Signature::updates` names one it
  reads and writes in place, which is what `setxcomp`, `setycomp`, `setzcomp` and `setcomp` do
  to their first. The compiler requires a variable there that the shader may assign, and
  propagates the storage class of the call's inputs into it, as an assignment would. `fresnel` is
  the one built-in with outputs: it returns the unpolarised reflectance of a dielectric, with
  `refract`'s conventions, and writes the reflected and refracted directions.
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
  image fills all three channels and an alpha channel is dropped. Called with only a name, `texture()` reads at the shader's `s` and `t`.
- **`noise()` is Perlin's improved noise in SL's range**: `[0, 1]`, and `0.5` on every lattice
  point. Its permutation is shuffled by a `type::Random` with a fixed seed, so a pattern is the
  same on every machine. Its float, pair and point forms read a line, a plane and a volume of
  it.

Background: [ADR-0026](../adr/0026-offline-shaders-run-over-batches-of-points.md)

## Normals

Both hiders carry two normals, because SL's `faceforward` and `calculatenormal` are defined in
terms of the pair:

- **`Ng` is the geometric normal**: the plane the primitive lies in, one value across it, wound
  the way its vertices are.
- **`N` is the shading normal.** A scene sets it per vertex with a varying `"N"`. Without one,
  `N` equals `Ng`, and the surface is faceted.

Rules:

- **A normal transforms by the inverse transpose**, never by the matrix that moves the points.
  The two agree under a rotation and a uniform scale, so the fault stays hidden until a scene
  scales one axis.
- **moya's `Vertex` holds both.** `RenderContext::addPolygon` fills them from
  `Polygon::geometricNormal()` before transforming anything. The diceable branch moves both into
  eye space. Dicing interpolates `N` and renormalises it. `Ng` is copied, since there is one.
- **A split does not carry per-vertex normals or colours.** Its pieces are built from
  intersection points, which have neither. Each piece takes the whole primitive's plane through
  `ReyesPrimitive::place()`, so a surface large enough to split is faceted per piece.
- **`trace::Triangle` has one constructor per case**: with and without per-corner normals.
  `shadingNormal(u, v)` interpolates over the barycentric coordinates `type::Ray::intersects`
  reports: `u` weights corner `b`, `v` weights corner `c`, and `a` takes the rest. That overload
  of `intersects` exists because Moller-Trumbore computes the coordinates on its way to the
  distance, and the four-argument form discards them.
