# ADR-0226: Offline: RIB names MaterialX nodes in RenderMan's syntax

**Status**: accepted
**Date**: 2026-11-11
**Tags**: offline, shaders, interchange
**Documented in**: [plans/MoyaMaterials.md](../../plans/MoyaMaterials.md)

## Context

moya's test scenes, its reference renders and `moya --paths` read RIB, and RI's `Surface` names
a shader of RI's own, not a MaterialX material. A material in the scene is a network of shading
nodes. RenderMan's RIB writes such a network with `Pattern` and `Bxdf` requests, each a node of
a type and a handle, and connects an input to another node's output by a `reference` parameter.
Pipelines keep materials in MaterialX documents, and no renderer names one from RIB. A test
scene reads best as one file.

## Decision

`Pattern` and `Bxdf` requests make MaterialX nodes of the types they name, in the attribute
state's material, connected by RenderMan's `reference` parameters, and the last `Bxdf` is the
surface terminal. A `Bxdf` of type `materialx` names a document and a material in it, and its
other parameters override the material's inputs by name. RenderMan's own node types keep this
syntax, and can be mapped onto MaterialX nodes later.

## Alternatives

### RenderMan's own nodes, mapped onto OpenPBR and MaterialX
- **For**: RenderMan's RIB reads as written, as its lights already do.
- **Against**: The mapping from `PxrSurface` and its patterns is large and approximate, and a
  MaterialX material still has no route from RIB.
- **Rejected because**: MaterialX is moya's material, and RenderMan's nodes can still be mapped
  later through the same requests.

### A request of its own that names a MaterialX document
- **For**: Any document renders as authored, with overrides as the request's parameters.
- **Against**: Every test scene needs a document beside it, and the request is a second syntax
  for a network that RenderMan's already writes.
- **Rejected because**: A `Bxdf` of type `materialx` names a document within RenderMan's syntax,
  and inline nodes keep a test in one file.

## Consequences

- **Gains**:
  - A test scene writes its materials inline, in one RIB file.
  - A MaterialX document renders as authored, with inputs overridden from RIB.
  - Each request makes one node of the scene's network, with no translation between them.
- **Costs**:
  - RenderMan parses such a file but does not render its materials.
  - A RIB author writes MaterialX node and input names, which no other RIB tool reads.
  - The last `Bxdf` being the surface is a rule of moya's, where RenderMan takes the one in force.
- **Revisit when**: RenderMan or another renderer gives RIB a way to name MaterialX materials.
