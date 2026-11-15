# ADR-0236: Offline: RI's built-in surfaces are MaterialX definitions

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, shaders, interchange
**Amends**: ADR-0186
**Documented in**: [offline/Rib.md](../../offline/Rib.md), [offline/Materials.md](../../offline/Materials.md)

## Context

RIB names a surface by a shader's name and its parameters, and RI defines seven built-in
surfaces that most RIB scenes use. moya's path tracer shades only MaterialX graphs compiled into
a node program, and it cannot run a shader written in the RenderMan Shading Language. A scene
holds the request as written, and turning a material into a program belongs to the material
compiler, not to a scene's client. RI lets a shader file of the same name on the shader search
path take the place of a built-in. moya already turns Standard Surface, UsdPreviewSurface and
glTF PBR into OpenPBR through MaterialX translation graphs.

## Decision

Each of RI's built-in surfaces is a MaterialX node definition of RI's parameters and defaults,
with a graph that turns it into OpenPBR Surface, which moya holds beside its other translations.
A surface of language `ri` compiles to the definition its name finds: a document of that name on
the shader search path first, then moya's own. A name that finds none is reported once and shades
as the default surface, which is RI's matte.

## Alternatives

### The scene client translates each built-in in code
- **For**: No files to install, and each rule is a function with its own test.
- **Against**: The RIB reader interprets shading, the editor and a Hydra delegate cannot reach the
  rules, and no user can replace a built-in.
- **Rejected because**: It puts material knowledge in one client of the scene, against the way
  every other foreign surface reaches OpenPBR.

### No translation
- **For**: One kind of material from RIB, as RenderMan's own did once it dropped its shading
  language.
- **Against**: Every RIB scene written for RI, moya's own installed scene among them, renders in
  one default material until it is rewritten.
- **Rejected because**: The built-ins have closure-based counterparts, as OSL's own shaders show,
  and dropping them loses scenes for no gain in the core.

## Consequences

- **Gains**:
  - RIB written for RI renders with materials close to what it asked for.
  - A user replaces or adds a surface by writing a MaterialX document, as a `.sl` file once did.
  - The definitions are data that a Hydra delegate and the editor can list as nodes.
- **Costs**:
  - RI's lobes add without conserving energy, so each graph is an approximation. A roughness
    follows a published conversion that no source calls exact.
  - `shinymetal`'s mirror term and a coloured `Os` have no exact counterpart in OpenPBR.
  - moya holds a definition and a graph for each built-in, which its tests keep near RI's.
- **Revisit when**: OSL patterns arrive, and a built-in written in OSL would be closer to RI's.
