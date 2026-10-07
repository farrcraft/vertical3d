# ADR-0025: Offline: RIB reader calls a typed handler interface

**Status**: accepted
**Date**: 2026-09-05
**Documented in**: [offline/Rib.md](../offline/Rib.md)

## Context

The RIB reader in `api/render/offline` ([ADR-0023](0023-offline-rib-is-the-scene-format.md))
needs an interface to call for each request it parses. moya already declares the full RI C API
in `RenderMan.h`, and RIB is a serialisation of exactly those calls. Most RI functions take
varargs, and a `va_list` cannot be built at run time, so a reader holding a parsed parameter
list can only call the `V` forms such as `RiPolygonV`. `RenderMan.h` is moya's header, and the
shared library sits below moya ([ADR-0022](0022-offline-shared-library-with-no-vulkan.md)). The
parameter representation is the part that is expensive to change later, because shader, light
and filter parameters all arrive through it.

## Decision

The reader calls `offline::rib::Handler`, an abstract C++ interface with one method per RI
request. Its methods take standard C++ and glm types and a parsed, typed `ParameterList`, and
every method has an empty default body. The RI C API stays moya's own, and no `RtToken` or
varargs form appears in `api/render/offline`.

## Alternatives

### The reader calls moya's RI C entry points
- **For**: One interface serves the C API and the reader, and RIB is literally a serialisation
  of those calls. moya's empty entry points would gain a caller.
- **Against**: The varargs forms, which are most of the geometry and every shader request,
  cannot be called from parsed data. The `V` forms can, but naming their argument types means
  `api/render/offline` includes moya's header, which inverts ADR-0022.
- **Rejected because**: The possible half costs the library its independence, and the half that
  matters is not possible.

### An intermediate scene representation the reader fills
- **For**: There is no interface to design, and a scene becomes a value that code can build
  without a file.
- **Against**: RI is a state machine with a current transform, colour and surface. A scene value
  has to freeze that state per primitive or reproduce the stacks. The reyes hider buckets a
  primitive as it arrives, so buffering the whole scene first is work it does not need.
- **Rejected because**: Nothing justifies the representation's shape, for the same reason
  ADR-0023 rejected one.

### A callback per request
- **For**: A renderer registers only what it handles, and there is no base class.
- **Against**: Some thirty `std::function` members per reader, and no single place listing which
  requests a renderer answers.
- **Rejected because**: An interface with empty default bodies gives the same "handle only what
  you support" property, with a list of requests the compiler checks.

## Consequences

- **Gains**:
  - The reader is tested alone, with a handler that only records what it was called with.
  - The RI rule that an unsupported request is accepted and ignored is the default, not
    something each handler must write.
  - Parameters are typed once, in the reader, against the declaration table.
- **Costs**:
  - moya has two ways into its render context, the C entry points and its handler, and they can
    disagree. Both call into `moya::RenderContext`, so a disagreement is a missing call rather
    than divergent behaviour.
  - The RI `V` entry points stay empty.
  - A misspelled override compiles silently, because the default body is empty rather than pure
    virtual. Every override therefore carries `override`.
  - The interface grows a method per request. A method that does not match an RI request does
    not belong on it.
- **Revisit when**: something needs to call moya through the RI C API from parsed data, which
  would make implementing the `V` forms over the handler worthwhile.
