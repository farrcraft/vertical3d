# ADR-0026: Offline Shading — Shading Is A Language, And A Shader Runs Over A Batch Of Shading Points

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Phase 2 of [the offline rendering roadmap](../roadmap/OfflineRendering.md) left both offline
renderers able to read a scene and unable to shade it. `Surface "plastic"` and
`LightSource "distantlight"` reach a handler with their parameters typed by the declaration table
([ADR-0025](0025-the-rib-reader-dispatches-a-cpp-request-interface.md)), and both renderers drop
them: what comes out is one flat colour per surface, with no light and no material behind it.

RIB names a shader by string. That is the whole of the difficulty. A `.rib` file written anywhere
else says `Surface "carpaint"` and expects the renderer to find `carpaint.sl` and run it, because
in RenderMan the shader is a program the scene's author wrote. A renderer that instead maps the
name onto a table of models it was built with answers a scene it has never seen with the nearest
of its built-ins, and the picture is not the one the file describes.

The two renderers shade at different granularities and always will. moya is a reyes pipeline: it
dices a primitive into a grid and wants every vertex of that grid shaded at once. talyn is a ray
tracer: it has one hit at a time. Whatever runs a shader has to serve both, and the shape of that
execution is the part of this decision that cannot be walked back — it is the inner loop of every
image either renderer will ever produce.

The cost is ordering, and it is real. Phases 4 and 5 sit behind this rather than beside it, and
this is a subsystem where the alternative was a weekend.

## Decision

Shading is a language: a subset of the RenderMan Shading Language, lexed, parsed, type-checked and
compiled by `api/render/offline` into a flat instruction program, and executed by a register
machine that **operates over a batch of shading points under an execution mask**. A varying
conditional runs both arms with the lanes that took each; moya's batch is a grid and talyn's is a
batch of one, and those are the same code path.

A shader is compiled from source when it is first named, and there is no compiled-shader file. The
standard shaders — `constant`, `matte`, `metal`, `plastic`, the four standard lights and a
`background` imager — are compiled into the library as source strings, so a renderer has them
whatever is on disk; `Option "searchpath" "shader"` finds everything else.

## Alternatives Considered

### Alternative 1: A shading language over a batch of shading points — **chosen**
- **Pros**: A scene written elsewhere renders as its author wrote it. The batch is what makes
  moya's grid and talyn's hit one implementation rather than two, and it is what lets a varying
  `if` cost a mask push rather than an interpreter call per vertex. `illuminance` — a surface
  shader running a light's shader over the same points — is expressible, and it is the mechanism
  the standard defines `diffuse` and `specular` in terms of. `trace()` becomes a callable thing,
  which is what makes phase 6 a question anyone can answer.
- **Cons**: A lexer, a parser, a semantic pass, a machine and a standard library, in front of the
  shading maths rather than instead of it. Phases 4 and 5 wait for it.
- **Why not**: n/a — chosen.

### Alternative 2: Fixed-function shading — a set of C++ models selected by name
- **Pros**: A weekend. `matte`, `plastic`, `metal` and `constant` are a hundred lines of maths that
  a language would have to call anyway, and phases 4 and 5 start immediately.
- **Cons**: The name in the RIB file stops meaning what RIB says it means. A scene naming a shader
  this tree has never heard of gets the nearest built-in and a warning, which is
  [phase 1's failure mode](../plans/completed/OfflineRenderingPhase1.md) — a scene that rendered
  wrong and a scene that was not understood look identical from outside. Every scene-author-facing
  extension afterwards is a C++ change and a rebuild.
- **Why not**: It is not less work, it is the same work with the front end removed: the models are
  written either way. What is given up for that saving is the property that made RIB worth reading
  in the first place ([ADR-0023](0023-rib-is-the-offline-scene-description.md)) — that a file
  describes the picture completely.

### Alternative 3: A shading language with a scalar execution model
- **Pros**: A far simpler machine. No mask stack, no lane bookkeeping, and `if` is a jump; a shader
  is a tree the machine walks, and talyn's one hit is the natural case.
- **Cons**: moya's inner loop becomes one interpreter entry per grid vertex, and the register file
  is allocated or reset at that same rate. A grid is where reyes does all of its work, so this is
  the whole cost of the renderer. Nothing fixes it afterwards short of rewriting the machine, and
  the semantics change with it — a language whose `if` is a jump has no answer for a condition that
  differs across a grid except to run the shader again per point.
- **Why not**: It is the one part of this decision with no migration path. Everything else here can
  be revised by writing more code; this would be revised by deleting the machine.

### Alternative 4: Embed an existing language — Lua, or GLSL through SPIR-V
- **Pros**: No lexer, no parser, no machine. A mature implementation, and in GLSL's case a vector
  type system that already fits shading.
- **Cons**: Neither reads a `.sl` file, so a RIB file's shader name still resolves to nothing and
  alternative 2's problem is untouched — the scene's author wrote SL. GLSL's execution model is a
  GPU's, and running it on the CPU means a SPIR-V interpreter, which is the machine this decision
  is about with a harder instruction set. Lua has no varying, no uniform and no mask, so the batch
  would be a loop in Lua over points, which is alternative 3 with a dependency. `illuminance` is
  expressible in neither without inventing it.
- **Why not**: The work being avoided is the front end, and the front end is the part that makes a
  foreign `.sl` file render. What would be acquired instead is a dependency and a second execution
  model to reconcile with the batch.

### Alternative 5: A `.slo`-style compiled shader file, as RenderMan has
- **Pros**: A shader compiles once rather than per render, and a compiler becomes a separate tool
  that a build can run ahead of time.
- **Cons**: A build artefact that can be stale, and a second thing on disk that has to be found,
  version-checked and regenerated. This tree has no shader toolchain to produce one and no build
  step that would; compiling a shader is milliseconds against a render measured in seconds.
- **Why not**: It buys nothing at this scale and adds a staleness failure that is invisible in the
  picture. Compiling on first use, and caching in the library for the run, is the same saving
  without the file.

## Consequences

### Positive
- A `.rib` file plus a `.sl` file renders in both renderers, which is what RIB promised.
- One implementation shades a reyes grid and a ray hit. talyn is not a special case in the machine;
  it is a batch of one, and a case asserts the two agree.
- `ambient`, `diffuse`, `specular` and `phong` are ordinary shader functions written over
  `illuminance` rather than privileged built-ins, so they are testable as source and a scene can
  replace them.
- `transmission` and `trace` are calls onto a renderer-implemented interface, which puts the one
  thing the two renderers genuinely disagree about — whether a shadow ray exists — behind a single
  method rather than inside the illuminance loop.
- The language's own correctness is unit-testable without either renderer: a source string, a
  batch, and an assertion on a register.

### Negative
- Phases 4 and 5 are behind this phase rather than beside it. That is the roadmap's own warning
  about a subsystem, and it is accepted here rather than discovered later.
- The tree acquires a compiler, which is a body of code with its own failure modes — a parse error
  in a shader is now a class of error a renderer has to report well.
- Two execution paths through the same machine exist in the sense that a uniform condition compiles
  to a jump and a varying one to a mask, so a bug in the varying inference is invisible until a
  grid disagrees with itself.

### Risks
- **The varying inference is the quiet fault.** Inferring uniform where varying was correct gives a
  whole grid one point's answer, which reads as a shading bug and is a compiler bug. Mitigation is
  that the inference is its own pass with its own cases, including an assignment inside a varying
  conditional, and that a grid-of-one case pins the two paths against each other.
- **An interpreted shader per grid vertex may be too slow for a real image.** Mitigation is the
  batch itself and a register file allocated per program rather than per run; if that is not
  enough, the instruction set is the thing to make wider, and no part of this record forecloses
  that.
- **A subset of SL is a moving line**, and the shader that is rejected is always the next one.
  Mitigation is that the parser accepts all five shader types and reports the two it does not
  execute by name, so a scene learns what is unsupported rather than what is unparsed.
