# ADR-0025: RIB Dispatch — The Reader Hands A Renderer C++ Requests With Typed Parameter Lists

**Date**: 2026-09-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0023](0023-rib-is-the-offline-scene-description.md) settled that one RIB reader in
`api/render/offline` drives both offline renderers, dispatching requests onto an interface each
implements. It did not settle what that interface is made of, and phase 2 of
[the offline rendering roadmap](../roadmap/OfflineRendering.md) cannot write the reader without
it.

moya already declares a full RI interface in `RenderMan.h` — `RtToken`, `RtPointer`, `RtMatrix`
and varargs — and RIB is a serialisation of exactly those calls, so reusing it as the reader's
interface is the obvious first thought. Two facts work against it. **A `va_list` cannot be
constructed at runtime**, so a reader holding a parameter list it parsed out of a file cannot
call `RiPolygon` at all; only the `RiPolygonV(nverts, n, tokens[], parms[])` form, which the RI
standard provides for precisely this caller. And `RenderMan.h` is moya's header:
[ADR-0022](0022-offline-rendering-shares-an-api-library.md) put the shared library below both
renderers rather than beside one, so `api/render/offline` including it would invert that.

The interface also outlives phase 2. A shader's parameters, a light's parameters and a pixel
filter's parameters all arrive through it in phases 3 and 4, so its parameter representation is
the part that is expensive to change later.

## Decision

The reader dispatches onto an abstract C++ interface whose methods take `std::string`, `float`,
`glm::mat4x4` and a parsed `ParameterList`, and whose every method has an empty default body.
The RI C ABI stays moya's own, and neither `RtToken` nor a varargs form appears in
`api/render/offline`.

## Alternatives Considered

### Alternative 1: A C++ request interface with typed parameter lists — **chosen**
- **Pros**: The reader can call it, which the varargs form makes impossible. The shared library
  keeps its own vocabulary and stays below both renderers. A parameter list parsed once against
  the declaration table is handed over already typed, so neither renderer re-derives how many
  floats `"Cs"` is. talyn gets the RI request set without acquiring moya's C header. Adding a
  request in phase 3 is a method with a default body, which breaks neither renderer.
- **Cons**: A second surface beside moya's C entry points, which can drift from it. An empty
  default body makes a misspelled override silent.
- **Why not**: n/a — chosen.

### Alternative 2: The reader calls moya's RI C entry points
- **Pros**: One interface for the C API and the reader, and RIB is literally a serialisation of
  those calls. moya's declared-but-empty entry points stop being dead code.
- **Cons**: Impossible for the varargs forms, which are most of the geometry and every shader
  request — a `va_list` cannot be built at runtime. Only the `V` forms are reachable, and
  `api/render/offline` would have to include moya's header to name their argument types, which
  inverts ADR-0022. talyn would link the RI C ABI to be handed a triangle.
- **Why not**: The half of it that is possible costs the library's independence, and the half
  that matters is not possible.

### Alternative 3: An intermediate scene representation the reader fills and each renderer reads
- **Pros**: The reader has no interface to design, and a scene becomes a value that can be
  built without a file.
- **Cons**: RI is a state machine — the current transform, the current colour, the current
  surface — and a scene representation has to either freeze that state per primitive or
  reproduce the stack. moya wants a primitive the moment it is bounded, so buffering the whole
  scene first is work the reyes pipeline was written not to need.
- **Why not**: Alternative 3 of ADR-0023 rejected an intermediate representation for the same
  reason it is rejected here: nothing justifies its shape until both renderers shade.

### Alternative 4: A callback per request rather than an interface
- **Pros**: A renderer registers only what it handles, and there is no base class to inherit.
- **Cons**: Thirty `std::function` members constructed per read, and no way to see at a glance
  which requests a renderer answers. The seam `api/ui` uses callbacks for is two functions, not
  thirty.
- **Why not**: An interface with default bodies gives the same "handle only what you support"
  property with a compiler-checked list of what exists.

## Consequences

### Positive
- One reader serves both renderers, and a suite drives it with a handler that only counts, so
  the parser is tested without either renderer.
- The RI standard's rule that an unsupported request is accepted and ignored is the default,
  not something each renderer has to remember to write.
- Parameters are typed once, in the reader, against the declaration table — the place that has
  the declarations.

### Negative
- moya now has two ways in, its C entry points and its handler, and they can disagree about
  what a request does. Both funnel into `RenderContext`, which is where the behaviour actually
  lives, so a disagreement is a missing call rather than a divergent one.
- The RI `V` entry points stay empty. Implementing them on top of the handler would give the C
  API and the reader one path, and is the obvious follow-on, but nothing calls them today.

### Risks
- **A misspelled override is silent**, because the default body is empty rather than pure
  virtual. Mitigation is `override` on every method in both renderers, which the compiler then
  checks; the tree already builds at `/W4`.
- The interface grows a method per request and becomes the union of what two renderers want.
  Mitigation is that it mirrors the RI request set, which is a specification someone else
  argued over — a method that does not correspond to an RI request does not belong on it.
