# ADR-0200: Offline: scene objects are nodes of declared parameters

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, interchange
**Documented in**: [offline/Scene.md](../../offline/Scene.md)

## Context

moya's retained scene is filled by RIB, which names parameters in typed lists, by the editor,
and later by a Hydra delegate, which names attributes of prims. Lights, materials and edits made
while rendering all add parameters over the rebuild, and a new parameter must not break a client.
Change tracking needs a unit smaller than an object, so that an edit rebuilds only what it
touched. Arnold, Cycles and MoonRay each declare every object kind's parameters once and set them
by name. Riley gives each kind its own calls with an untyped parameter list.

## Decision

Each kind of scene object declares its parameters once, each with a name, a type and a default.
A client sets and reads parameters by name, a value of the wrong type is refused, and change
tracking and the scene's description work from the declarations. An object is reached by a
handle of an index and a generation, so the handle of a deleted object matches nothing.

## Alternatives

### A call per object kind with an untyped parameter list, as Riley has
- **For**: Each kind's calls name what it needs, and a list takes new parameters without changing
  a signature. RIB's parameter lists map onto it directly.
- **Against**: A list is checked only by each kind's own code, so every kind writes its own
  checks and its own change tracking. A change is known by which argument was passed, not by which
  parameter moved.
- **Rejected because**: Declarations give checking, defaults, change tracking and the description
  once for every kind.

### A C++ struct per object kind, set whole
- **For**: Types are checked by the compiler, and reading a value costs nothing.
- **Against**: Every new parameter changes a struct that each client compiles against. A client
  that maps names, as RIB and Hydra do, needs code for every parameter. A change is known only for
  the whole object.
- **Rejected because**: The schema grows through the rebuild, and its clients all map names.

### Shared pointers as handles
- **For**: Cycles and Arnold hand out pointers, and reading through one is direct.
- **Against**: A client can keep a deleted object alive, or use one after the scene has let it
  go. A pointer means nothing on a device or in a description.
- **Rejected because**: A generation catches a stale handle, and an index is the same in every
  copy of the scene.

## Consequences

- **Gains**:
  - RIB and Hydra set parameters by name with no code per parameter.
  - A new parameter is a new declaration, and no client breaks.
  - Change tracking records each parameter, so a consumer rebuilds from exactly what moved.
- **Costs**:
  - A parameter's type is checked when it is set, not when the client is compiled.
  - Reading a parameter by name costs a lookup, so a consumer reads values once when it builds
    and not per ray.
  - The declarations are a schema moya keeps stable for its clients.
- **Revisit when**: a consumer needs to read parameters in a loop that a lookup slows, or the
  kinds stop growing.
