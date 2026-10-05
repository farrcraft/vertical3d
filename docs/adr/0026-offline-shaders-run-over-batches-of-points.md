# ADR-0026: Offline: shaders run over batches of points

**Status**: accepted
**Date**: 2026-09-08
**Documented in**: [OfflineRenderer.md](../OfflineRenderer.md)

## Context

RIB names a shader by string, as in `Surface "carpaint"`. In RenderMan a shader is a program
the scene's author wrote in the RenderMan Shading Language (SL), and the renderer is expected to
find and run it. A renderer that maps the name onto a fixed table of built-in models renders an
unknown name as its nearest built-in, which is not the picture the file describes. Shading
happens at two granularities: the reyes hider shades every vertex of a diced grid at once, and a
traced ray has one hit. The shape of shader execution is the inner loop of every image, and it
is the part of this choice that cannot be changed later without rewriting the machine.

## Decision

Shading is a language. `api/render/offline` compiles a subset of SL into a flat instruction
program, and a machine runs it over a batch of shading points under an execution mask, so a
grid and a single traced hit take the same code path. Shaders are compiled from source when
first named, and the standard shaders are compiled into the library as source.

## Alternatives

### Fixed-function shading: C++ models selected by name
- **For**: Quick to write. `matte`, `plastic`, `metal` and `constant` are a small amount of
  maths, and work on texturing and sampling could start at once.
- **Against**: The name in a RIB file stops meaning what RIB says it means. A scene naming an
  unknown shader gets the nearest built-in and a warning, so a scene rendered wrongly and a
  scene not understood look the same. Every new shader is a C++ change and a rebuild.
- **Rejected because**: The shading models are written either way. Removing the language saves
  the front end and gives up the reason RIB was chosen
  ([ADR-0023](0023-offline-rib-is-the-scene-format.md)): that a file describes the picture
  completely.

### A shading language with a scalar execution model
- **For**: A much simpler machine, with no mask stack and no per-point bookkeeping. `if` is a
  jump, and one traced hit is the natural case.
- **Against**: The reyes hider would enter the interpreter once per grid vertex, which is where
  it does all its work. The semantics change too: a scalar `if` has no answer for a condition
  that differs across a grid except to run the shader once per point.
- **Rejected because**: It has no migration path. Everything else here can be revised by adding
  code; this would be revised by replacing the machine.

### Embed an existing language: Lua, or GLSL through SPIR-V
- **For**: No lexer, parser or machine to write, and a mature implementation. GLSL already has a
  vector type system suited to shading.
- **Against**: Neither reads a `.sl` file, so a RIB file's shader name still resolves to
  nothing. Running GLSL on the CPU needs a SPIR-V interpreter, which is a harder machine. Lua has
  no uniform or varying values and no mask, so the batch becomes a Lua loop over points. Neither
  can express `illuminance`, the construct that runs light shaders from a surface shader.
- **Rejected because**: The work avoided is the front end, and the front end is what makes a
  foreign `.sl` file render. The cost is a dependency and a second execution model.

## Consequences

- **Gains**:
  - A `.rib` file and the `.sl` files it names render as their author wrote them.
  - One machine shades a reyes grid and a traced hit. A batch of one is not a special case.
  - `diffuse`, `specular` and `phong` are SL source written over `illuminance`, as the standard
    defines them, so a scene can replace them.
  - `trace()` and `transmission()` are calls onto an interface the renderer implements.
  - The language is unit-testable without the renderer: a source string, a batch and a check on
    a register.
- **Costs**:
  - The tree carries a compiler: a lexer, a parser, a type checker, a varying inference pass and
    a machine. A parse error in a shader is an error class the renderer has to report well.
  - A uniform condition compiles to a jump and a varying one to a mask. Inferring uniform where
    varying was correct gives a whole grid one point's answer, which looks like a shading bug.
  - An interpreted shader per grid may be too slow for large images.
  - A subset of SL always rejects some shader. The parser accepts all five shader types and
    reports by name the ones it does not run.
- **Revisit when**: shading time dominates a real render, which would make a wider instruction
  set or a compiled form worth building.
