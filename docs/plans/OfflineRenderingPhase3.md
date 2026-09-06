# Offline Rendering, Phase 3 — Light And Surface

Drafted 2026-09-05, **open**. Takes up phase 3 of
[the offline rendering roadmap](../roadmap/OfflineRendering.md), which stays the account of where
both renderers stand and what the later phases are; this plan does not repeat it.

Phase 2 ended with both renderers reading a scene from a RIB file and drawing it. A scene can
already *say* `Surface "plastic"` and `LightSource "distantlight"` — the reader consumes both
correctly and hands them to a handler with their parameters typed — and both renderers drop them
on the floor. What comes out is one flat colour per surface with no light and no material behind
it.

The roadmap left one question open under this phase: **fixed-function shading or a shading
language.** It is answered here as a language. That is the larger of the two answers by a wide
margin, and the ordering cost is stated rather than hidden: phases 4 and 5 sit behind this plan
rather than beside it, and this plan is a subsystem where a fixed-function phase 3 would have been
a weekend. What it buys is that a scene written elsewhere renders as its author wrote it rather
than as the nearest of four built-ins, and that `trace()` exists as a callable thing — which is
what makes phase 6 answerable at all.

The phase closes when a person can write a `.sl` file, name it from a `.rib` file, and have both
renderers run it.

## Decisions

Recorded in [docs/adr/](../adr/), not here. The ones that shape this plan:

| ADR | Decision |
|---|---|
| 0026 | Shading is a language, and it runs over a batch of shading points — **step 1 writes it** |
| [0022](../adr/0022-offline-rendering-shares-an-api-library.md) | Shared offline code is `api/render/offline`; each renderer is a library with a driver |
| [0023](../adr/0023-rib-is-the-offline-scene-description.md) | RIB is what both renderers read; the editor exports to it, one way |
| [0024](../adr/0024-api-type-serves-both-renderers.md) | `api/type` serves both, and a convention is a parameter rather than a fork |
| [0025](../adr/0025-the-rib-reader-dispatches-a-cpp-request-interface.md) | The reader hands a renderer C++ requests with typed parameter lists |

## What blocks what

The steps fall into three groups, and the middle one is the whole cost of the answer.

**Step 2 is the geometry half and blocks everything that shades**, because a shader is a function
of a normal and neither renderer has one. It depends on nothing in the language and can be built
and tested first, on its own, against a picture that shows the normals.

**Steps 3 to 7 are the language** and are strictly sequential — there is no way to type-check an
AST that does not parse, no way to run a program that has not been compiled, and no way to write a
built-in without the value model it operates on. This is the part that will take the time.

**Steps 8 to 12 are the two renderers** and are where the language first runs over real geometry.
Step 8 is the binding both renderers need; 9 and 10 are independent of each other; 11 and 12 close
the phase.

```
 2 normals ──────────────────────────────────────────┐
                                                     │
 1 ADR-0026                                          │
     │                                               v
     └──> 3 lexer ─> 4 parser ─> 5 semantics ─> 6 vm ─> 7 library ─> 8 binding ─┬─> 9 moya  ──┬─> 11 imager ─> 12 references
                                                                                └─> 10 talyn ─┘
```

## Steps

### Step 1 — ADR-0026, shading is a language

The roadmap has carried this question open since it was written, and it reaches past this phase:
it decides whether talyn is reached from a shader's `trace()`, which is the remaining half of
[phase 6](../roadmap/OfflineRendering.md). The record has to exist before step 3, because every
step after it is an implementation of one of the alternatives.

Four things it has to weigh, and the record is the place they are argued rather than here:

- **A language is a fixed-function shader set plus a compiler.** `diffuse()` and `specular()` are
  the same maths either way; the language calls them rather than being them. So the alternatives
  are not two different bodies of work, they are one body of work with an optional front end, and
  the question is only whether the front end is worth it.
- **What the front end buys is a scene this tree did not write.** RIB from elsewhere names shaders
  this tree has never heard of. Fixed function answers such a scene with the nearest of four
  built-ins and a warning; a language answers it with what the author wrote.
- **What it costs is the ordering.** Phases 4 and 5 are behind this rather than beside it. That is
  the roadmap's own warning about a subsystem, and accepting it is part of the decision rather than
  a consequence discovered later.
- **The execution model is the part that is hard to reverse.** A shader runs over a *batch* of
  shading points, not one — moya's batch is a micropolygon grid and talyn's is a single hit, and a
  batch of one is what makes those the same code path. Conditionals over a varying value run both
  arms under a mask. Choosing a scalar model instead would make moya's inner loop a per-vertex
  interpreter call, and no later change fixes that without rewriting the machine.

The record also settles two smaller things that are awkward to leave to whoever types first, and
are cheap to state alongside: **the standard shaders are compiled into the library** as source
strings, so a renderer always has `matte` whatever is on disk, with RI's `Option "searchpath"
"shader"` finding anything else; and **a shader is compiled from source when it is first named,**
with no separate compiled-shader file — RenderMan's `.slo` is a build artefact of a toolchain this
tree does not have, and one artefact is one thing to be stale.

**Done when**: `docs/adr/0026-*.md` exists, is in the index, and steps 3 to 7 cite it rather than
re-deriving it.

### Step 2 — a surface normal, geometric with `"N"` honoured

Independent of the language and blocking all of it. Neither renderer has a normal today: moya's
`Vertex` declares `normal_` and nothing writes it, and talyn's `Triangle` has no notion of one.

- **The default is the geometric normal** — the plane of the polygon, from its first three
  non-collinear vertices. A scene that supplies `"N"` overrides it per vertex, which the
  declaration table already types as a varying normal, and that is what gives a smooth surface.
  Both go through: `Ng` is always the geometric one and `N` is the shading one, because SL's
  `faceforward` and `calculatenormal` are defined in terms of both.
- **A normal transforms by the inverse transpose**, not by the matrix that moves the points. It is
  one line in each renderer and it is invisible under uniform scale, which is what every fixture
  in the tree currently uses — so the case that pins it has to scale one axis.
- moya: dicing interpolates the normal onto the grid alongside the position and the colour.
  **A split does not carry a per-vertex normal**, for the reason a split does not carry a colour —
  its pieces are built from intersection points that have none — so a piece inherits the
  primitive's geometric normal the way it inherits its colour, through `ReyesPrimitive::place()`.
  A smooth surface that is large enough to split is therefore faceted per piece; name it in a
  comment, and leave it for the phase that gives `Plane::clip` an interpolating edge split.
- talyn: `Triangle` gains a normal, or three of them. The hit needs barycentric coordinates to
  interpolate, and Möller-Trumbore computes them on the way to the distance —
  [`type::Ray::intersects`](../../api/type/Ray.h) throws them away. **An overload that also reports
  `u` and `v`** is the change, additive rather than a signature change, and by
  [ADR-0024](../adr/0024-api-type-serves-both-renderers.md) it belongs in `api/type` where the
  editor's picker can have it too.

**Done when**: `v3dtest_type` covers the barycentric overload against a hand-worked triangle, both
renderers' suites cover a face normal and a `"N"` override, a non-uniform scale has a case in each,
and a fixture that shades the normal as a colour renders the picture a normal map should be.

### Step 3 — the SL lexer

`api/render/offline`, following the `RIB*` precedent that is already there: `SLLexer` beside
`RIBLexer`, taking an `std::istream` so a case is a string literal.

- Token kinds: identifier, keyword, number, string, operator, punctuation. The keyword set is the
  shader types, the data types, the storage classes, the control flow, and the three lighting
  constructs — everything else is an identifier and is resolved by the symbol table in step 5.
- Comments are `/* */` and `//`. A block comment does not nest.
- Numbers carry the same shapes RIB's do — a leading or trailing `.`, an exponent — and are a
  `float`. SL has no integer type.
- Strings are double-quoted with the C escapes. A string in SL is a value with almost no
  operations on it: it names a coordinate space, a texture or a message, which is why it may be
  uniform only, and step 5 is where that is enforced.
- Every token carries a line and a column, for the same reason the RIB lexer's do.
- **The C preprocessor is not run.** Real `.sl` files go through `cpp` for `#include` and
  `#define`, and a shader that needs one is rejected by name rather than mis-parsed — a `#` at the
  head of a line is a diagnostic, not a comment.

**Done when**: `v3dtest_render_offline` covers each token kind, both comment forms, a block comment
containing a `//`, an unterminated string and an unterminated block comment as errors that say
where, and a `#` line rejected by name.

### Step 4 — the grammar, and the syntax tree

`SLParser` producing `SLSyntax` nodes. Recursive descent, because the grammar is small and the
error messages are the reason anyone will read this code.

- **Five shader types are parsed: `surface`, `light`, `displacement`, `volume`, `imager`.** Three
  of them are executed this phase — surface, light and imager — and the other two are parsed so a
  scene carrying one gets a message about what is not supported rather than a message about
  syntax. That is the same distinction phase 2 drew between a request that is recognised and one
  that is unparsed.
- A parameter list gives each parameter a type, an optional storage class, an optional `output`,
  and a **required default** — SL has no uninitialised parameter, and the default is what a scene
  that does not mention it gets.
- Types: `float`, `point`, `vector`, `normal`, `color`, `matrix`, `string`, `void`. The three
  point-like types are all three floats and differ only in how a transform treats them, which is
  step 5's problem and not the parser's.
- Statements: a declaration, an assignment with the compound forms, `if`/`else`, `for`, `while`,
  `break`, `continue`, `return`, a block, an expression statement, and the three lighting
  constructs.
- Expressions, in precedence order: the ternary, the logical and comparison operators, `+` and `-`,
  `*` and `/`, **`.` for dot product and `^` for cross product** — the two operators most likely to
  be read as something else — unary `-` and `!`, a typecast with an optional space name
  (`point "world" (0, 0, 0)`), a triple, a call, an array index, and a parenthesised expression.
- **Functions may be defined inside a shader.** Recursion is rejected at the call graph rather than
  at the parser: the machine in step 6 has a register file per shader run and no call stack, so a
  recursive shader has no meaning to give.
- Every diagnostic names a line, a column and what was expected. A parse that fails yields no
  program, and step 8 decides what a renderer does about that.

**Done when**: each of the four standard surface shaders and each of the four standard light
shaders parses, `a . b` and `a ^ b` have cases distinguishing them from a member access and an
exponent, precedence has a case per level, a `displacement` shader parses and is marked
unsupported rather than failing, and a syntax error reports a position.

### Step 5 — symbols, types, and the varying inference

`SLCompiler`, the pass between the tree and the program. Three jobs, and the third is the one that
decides whether the machine is fast or is an interpreter call per vertex.

- **Symbols and scopes.** A shader's parameters, its globals, its locals, and its functions. The
  globals are what the shader type decides: a surface shader gets `P`, `N`, `Ng`, `I`, `E`, `Cs`,
  `Os`, `s`, `t`, `u`, `v`, `du`, `dv`, `Ci` and `Oi`; a light shader gets `P`, `Ps`, `L`, `Cl`;
  an imager shader gets `P`, `Ci`, `Oi` and `alpha`. A global written by a shader that may not
  write it is an error, which is most of what stops a light shader being nonsense.
- **Types, with RI's coercions.** A float promotes to a point, a vector, a normal or a colour by
  replication. The three point-like types convert to each other freely in arithmetic and are
  *distinct to a transform*: `ptransform` translates, `vtransform` does not, and `ntransform` uses
  the inverse transpose. Getting that wrong is the same class of fault step 2's inverse transpose
  is, and it is worth a case each.
- **The varying inference.** A value is uniform unless something varying reaches it. Shader globals
  that differ per shading point are varying and parameters are uniform unless declared otherwise;
  anything assigned from a varying becomes varying, and **anything assigned inside control flow
  whose condition is varying becomes varying too**, because different points take different arms.
  A uniform value is stored once and a varying one per point, so this pass is what decides the
  memory the machine touches. It is also the pass that can be wrong quietly: inferring uniform
  where varying was correct gives the whole grid one point's answer, which looks like a shading
  bug and is a compiler bug.
- **A string is uniform, and so is a coordinate space name.** There is no per-point transform to
  look up, and allowing one would make every transform a runtime string lookup.

**Done when**: the standard shaders all compile, each coercion has a case, the three transform
functions have a case each against a matrix that both rotates and translates, a varying condition
makes an assignment inside it varying, a light shader writing `Ci` is an error, and an undeclared
identifier is reported with a position.

### Step 6 — the value model and the virtual machine

`SLValue`, `SLProgram` and `SLMachine`. The heart of ADR-0026's execution model.

- **A value is a type, a storage class and a buffer** — one element wide when uniform, one element
  per shading point when varying. A program is a flat list of instructions over register indices
  rather than a tree the machine walks, because the mask handling below wants a place to put a
  jump and because a tree walk over a batch allocates at every node.
- **The execution mask is a stack.** An `if` on a uniform condition is a jump. An `if` on a varying
  one runs both arms, each under the lanes that took it, and an arm whose mask is empty is skipped
  — which is the optimisation that makes the common case free. `while` and `for` iterate while any
  lane is live. `break`, `continue` and `return` clear lanes rather than jumping out.
- **Talyn's batch is one point.** No special case, no second path: the same program, the same
  instructions, a mask one bit wide. That is the whole reason the model is a batch, and a case
  asserts that a shader run over a grid of one gives what it gives over that point inside a grid
  of many.
- The machine holds no renderer state. What it needs from a renderer — the matrix for a named
  coordinate space, the active lights, whether light reaches a point, a ray traced — arrives
  through an interface the renderer implements, which is what step 7 fills in and steps 9 and 10
  implement.
- **A shader run allocates once.** The register file is sized by the program and the batch, and a
  renderer shading a thousand grids reuses it. moya's inner loop is the only place in either
  renderer where this matters, and it is where a first version will be slow if the file is per run.

**Done when**: a hand-built program computes an arithmetic expression over a batch and over a
batch of one to the same answer, a varying `if` gives each lane its own arm, a `break` in a varying
`while` leaves the other lanes running, an empty mask skips an arm, and a uniform condition
compiles to a jump rather than a mask.

### Step 7 — the standard library

`SLBuiltins`, and the renderer interface the interesting half of it calls through.

The plain built-ins, which are arithmetic over the value model and are cheap once step 6 is real:

- Maths: `abs`, `sign`, `floor`, `ceil`, `round`, `mod`, `min`, `max`, `clamp`, `mix`, `step`,
  `smoothstep`, `sqrt`, `pow`, `exp`, `log`, and the trigonometry with `radians` and `degrees`.
- Geometry: `length`, `distance`, `normalize`, `faceforward`, `reflect`, `refract`, `xcomp` and
  its siblings with their setters, `ptransform`, `vtransform`, `ntransform`, `mtransform`,
  `depth`, `calculatenormal`.
- Colour and matrix: `comp`, `setcomp`, `ctransform`, `determinant`, `translate`, `rotate`,
  `scale`.
- `printf`, which is how anyone debugs a shader and costs an afternoon.

The four that are not arithmetic, and are the reason the interface exists:

- **`illuminance`, `illuminate` and `solar` are the message passing**, and are the part of SL that
  is not a language feature anywhere else. `illuminance(P) { ... }` in a surface shader runs its
  body once per active light, with `L` and `Cl` set by that light's own shader — so running it
  means running another program, with its own register file, over the same batch. `illuminate`
  and `solar` in a light shader are the other end: the first for a light with a position, the
  second for one at infinity. `ambient()`, `diffuse()`, `specular()` and `phong()` are then
  ordinary functions written in terms of `illuminance` rather than built-ins with privileged
  access, which is both what the standard says and what makes them testable.
- **`transmission(Psrc, Pdst)` is where a shadow lives.** It asks the renderer how much light gets
  from one point to another; talyn answers by tracing and moya answers 1 until it has a shadow map.
  Putting it here rather than inside the illuminance loop keeps the loop's semantics the
  standard's, and puts the one thing the two renderers genuinely disagree about behind one call.
  It is a ray tracing extension rather than RI 3.03, and the record says so.
- **`trace(P, R)` is the phase 6 hook.** talyn implements it, moya answers with the background and
  reports once. Its existence is what makes phase 6 a question anyone can answer; **this phase does
  not answer it**, and nothing here decides whether moya's raytracing is talyn.
- **`texture`, `shadow` and `noise` are declared and stubbed.** Texture is phase 5 and rides on
  `api/image`; a shadow map needs a render to a depth file, which is a pass this tree does not
  have; noise is a body of code that earns its own step somewhere. Each returns its default and
  reports once per shader, which is phase 1's lesson: a scene that rendered nothing and a scene
  that was not understood look identical from outside.

**Done when**: each plain built-in has a case, `faceforward` and `refract` have one against
hand-worked values, a two-light scene runs `illuminance` twice with the right `L` and `Cl` each
time, `diffuse` over one distant light gives the cosine, and each stub reports exactly once.

### Step 8 — a shader instance, and how a scene names one

The binding, needed identically by both renderers, and the point at which RIB reaches the language.

- **`Shader` is a compiled program plus bound parameter values.** `Surface "plastic" "Ks" [0.8]`
  binds `Ks` from the `ParameterList` and leaves everything else at its declared default. A name
  the shader does not declare is reported and dropped — a renderer must accept a request carrying a
  parameter it does not support — and a type that cannot be coerced is reported rather than
  reinterpreted.
- **`ShaderLibrary` maps a name to a program**, compiling on first use and caching. The standard
  shaders — `constant`, `matte`, `metal`, `plastic`, `ambientlight`, `distantlight`, `pointlight`,
  `spotlight`, and a `background` imager — are compiled into the library as source strings per step
  1's record, so `Surface "matte"` works against no files at all, which is what makes the suite
  hermetic. `Option "searchpath" "shader"` adds directories for everything else.
- **A shader that fails to compile is reported and substituted**, not fatal. RI asks a renderer to
  carry on; the substitute is the default surface and the report names the file and the position.
  A scene whose shader failed and a scene that named no shader must not look the same, so the
  report is per name and is loud.
- **The handler interface grows what phase 2 left out.** `illuminate(handle, on)`, `imager(name,
  parameters)`, and a `lightSource` that carries the handle — the reader currently recognises the
  sequence number and throws it away, which is correct only while nothing can turn a light off.
  RIB 3.03 writes the handle as a number and later RIB writes a string; both are read, and the
  handle is a string to the handler either way.
- **`RenderMan.h` gets the matching bodies.** `RiSurface`, `RiLightSource`, `RiIlluminate` and
  `RiImager` are declared and empty or returning 0. Phase 2's precedent is that the C API and the
  handler are given bodies together so the two paths into a context do not drift.

**Done when**: a `ParameterList` binds onto a shader's declared parameters with defaults for the
rest, an unknown parameter and an uncoercible one are each reported, a compile failure substitutes
and reports, `Illuminate` reaches a handler with the handle its `LightSource` was given, and the
example file still reads with the new requests in it.

### Step 9 — moya shades a grid

Where the machine meets the renderer it was designed for.

- **The graphics state gains a surface shader and a light list**, pushed and popped by
  `attributeBegin` and `attributeEnd` alongside the transform, the colour and the shading rate that
  are there already. `Illuminate` turns a light on or off in that state.
- **A primitive carries both across a split**, on `ReyesPrimitive::place()` beside the placement and
  the colour, for the reason phase 2 found: splitting resubmits pieces during the second pass, when
  neither is current any more.
- **The shading points are the grid's vertices.** `P` is the vertex in the shader's current space,
  `N` and `Ng` come from step 2, `Cs` and `Os` from the graphics state or the primitive's own, `I`
  is `P - E`, and `s`, `t`, `u`, `v` are the grid parameters dicing already computes. `du` and `dv`
  are the grid spacing, which is what `calculatenormal` and any future derivative need.
- **The light shaders run over the same batch.** Each active light's program fills `L` and `Cl` for
  every point on the grid, and the surface's `illuminance` reads them.
- **`Ci` and `Oi` land back on the vertices**, and `hide` samples `Ci` where it samples a colour
  today. `Bucket::render`'s comment that dicing's interpolated colour is the whole of shading stops
  being true and comes out.
- **moya's current space is camera space.** That is what its first pass already works in, and it is
  what the space table hands the machine for `"current"`. It differs from talyn's, which is the
  reason the table is a renderer callback rather than a constant.

**Done when**: a suite case runs `matte` over a grid under one distant light and asserts the cosine
falloff at each vertex, `constant` reproduces the flat colour the current code writes, a scene with
two lights sums them, and `Illuminate` turning one off changes the picture.

### Step 10 — talyn shades a hit, and casts a shadow ray

- **`Scene` gains a light list and a triangle gains a shader.** A triangle's flat colour becomes
  its `Cs`, which is what `Color` already sets and what `matte` multiplies.
- **The batch is one point.** The nearest hit builds it: `P` is the hit, `N` and `Ng` from step 2's
  barycentric interpolation, `I` is the ray direction, `s` and `t` are the barycentric coordinates
  until there is a real parameterisation. Then the surface shader runs, and `Ci` is the pixel.
- **`transmission` traces.** A ray from the hit toward the light, and anything hit before the light
  blocks it — opaque only, because `Os` on an occluder is a shading question and this is a
  visibility one. **The self-intersection epsilon is the trap**: a shadow ray that starts exactly on
  the surface hits it, and every lit pixel comes out black in a pattern that looks like a normal
  fault. Offset along the geometric normal, not along the ray.
- **talyn's current space is world space**, because that is where its scene is. Named in a comment
  beside moya's camera space, since the pair is the thing that will confuse a reader.
- **One shader run per pixel is slow and that is accepted here.** Batching the hits of a scanline
  that share a shader is the obvious next thing and is not this phase; the 64 by 48 references do
  not need it and a real image will.

**Done when**: `talyn --file scene.rib --outfile out.png` renders a lit scene, a suite case asserts
the shaded value at a known hit against hand-worked maths, an occluder darkens the point behind it,
and a case pins the epsilon by putting a light directly above a large polygon.

### Step 11 — the imager, and talyn's background

The roadmap put `RiImager` in this phase for one reason: it is how a RIB file says what a ray that
hits nothing is worth, and phase 2's talyn reference works around its absence with a backdrop
polygon.

- **An imager shader runs over the finished framebuffer**, one batch per row or per tile, reading
  and writing `Ci`, `Oi` and `alpha`. `background` is the standard one and is the whole of what is
  needed here: composite the image over a constant colour where alpha says nothing was drawn.
- Both renderers run it, in the same place — after the last bucket for moya, after the last ray for
  talyn — which is the third consumer of the machine and the one that proves the batch model was
  not built for grids alone.
- **The talyn reference scene's backdrop polygon comes out** and is replaced by
  `Imager "background"`. Whether the picture is bit-identical is what the test says: if it is not,
  the reference is regenerated and the commit says which request changed it and why.

**Done when**: an imager runs over a framebuffer in both renderers, `Imager "background"` gives
the pixels the backdrop polygon gave, and a scene naming no imager is unchanged.

### Step 12 — the reference scenes grow light and surface

Phase 1 established that a picture is what tells a regression from the status quo, and phase 2
established that each picture is reached by two routes — a `.rib` file and a code-built scene. This
step keeps both properties across a change that alters what every renderer writes.

- **The existing two references stay exactly as they are**, by naming `Surface "constant"` in each
  `.rib` and building a constant shader in each code case. That is the strongest test in the phase:
  the picture the tree drew before there was a language is the picture the language draws, to the
  same tolerance, or something in steps 3 to 10 is wrong.
- **Two new references, one per renderer**, and each is a scene worth the name: a matte surface and
  a plastic one, a distant light and a point light, and for talyn a polygon whose shadow falls
  across another. Each with its `.rib` beside its `.png`, each rendered by both routes.
- **A shader is not tested by a picture.** The language's own cases are in
  `v3dtest_render_offline` and are where a wrong `smoothstep` is found; the images are there to
  catch what unit cases cannot, which is the wiring between the machine and a renderer.
- The drivers are the other half, on the same files — `moya --file ... --output ...` and
  `talyn --file ... --outfile ...`. That is what a person runs before reading a ctest failure.

**Done when**: `ctest -R moya` and `ctest -R talyn` each match four references — two unchanged from
phase 2 and two new — on a machine with no GPU.

## Verification

The four gates in [sdlc.md](../sdlc.md) §4: `ninja -C out/build/x64-Debug`, cpplint per
[Linting.md](../Linting.md), and `ctest --test-dir out/build/x64-Debug --output-on-failure`.

Render verification is ctest, as it was in phases 1 and 2. Neither renderer touches a window, a
device or a swapchain, so "run it and read the validation log" — the method for every other
rendering change in this tree — is not the method here.

The new suite surface is `v3dtest_render_offline` for steps 3 to 8 and `v3dtest_type` for step 2's
barycentric overload, the moya and talyn suites for steps 9 to 11, and four reference cases for
step 12. **The language's cases are the bulk of it**, and they are cheap: a shader source string, a
batch, and an assertion on a register. A phase this size that leans on four pictures to know
whether it works is a phase that will not be finished.

## What this phase does not do

- **No displacement, volume, interior or exterior shaders.** Parsed, and reported as unsupported.
  Displacement in particular reaches back into dicing and is a geometry change wearing a shading
  change's clothes.
- **No area lights.** `RiAreaLightSource` stays as it is; an area light is a sampling problem and
  sampling is phase 4.
- **No texture, no shadow maps, no noise.** Each is declared, stubbed and reported. Texture is
  phase 5 per the roadmap; a shadow map needs a depth render this tree has no pass for.
- **No `.slo`, and no C preprocessor.** A shader is source, compiled when named.
- **No sampling change.** One sample per pixel centre, and the five `Ri*Filter` functions keep
  returning `0.0`. Phase 4.
- **No reflection or refraction.** `trace()` exists and talyn implements it; no standard shader
  this phase ships calls it. Phase 5.
- **No `Sides`, `Orientation` or `ReverseOrientation`.** `faceforward` in a shader is what handles
  a normal facing away, and the requests that let a scene state it are a graphics state change with
  no shading of its own.
- **Phase 6 is not answered.** The hook exists; whether moya's raytracing *is* talyn is still a
  decision, and one this phase deliberately leaves for the phase that owns it.
- **No SIMD instructions.** The batch is the execution model, not the instruction set; whether the
  machine's inner loops ever become intrinsics is a performance question for a scene large enough
  to ask it.
- **No talyn batching beyond one hit.** Named in step 10, and the first thing to do if a real image
  is unusably slow.

## Open questions

Small enough to settle in the code with a comment rather than in a record, but named so they are
settled deliberately rather than by whoever types first.

- **What a shader's `"shader"` space is.** RI says it is the transform in force when the shader was
  instanced, which means the graphics state has to save one per `Surface` request. moya can; talyn
  attaches shaders to triangles already in world space and has nowhere to put it. Probably a
  per-shader-instance matrix on both, settled when step 8 writes the instance.
- **Whether `Cs` on a primitive beats `Cs` in the graphics state.** RI says the primitive's own
  varying `"Cs"` wins, which is what moya's dicing already does; talyn has no per-vertex colour at
  all and will take the graphics state's. State the asymmetry in a comment or close it.
- **How loud a stub is.** Once per shader, once per program, or once per name. Too quiet and a
  scene renders wrong silently, which is the failure mode phase 1 named; too loud and a
  640 by 480 render prints a million lines. Once per program name is the intended answer.
- **`RiRotate`'s sign**, carried forward from phase 2 and now with something at stake. RI states
  its rotations in a left handed system and both renderers hand the angle to `glm::rotate`, which
  is counter-clockwise by the right hand rule. Nothing in the tree can tell the difference, because
  both renderers agree with each other — but a light placed by a rotation is the first thing whose
  wrongness is visible rather than merely mirrored.
