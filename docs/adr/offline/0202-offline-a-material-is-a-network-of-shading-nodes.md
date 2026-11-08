# ADR-0202: Offline: a material is a network of shading nodes

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, interchange
**Documented in**: [plans/MoyaScene.md](../../plans/MoyaScene.md)

## Context

A material reaches the scene before it is compiled into the program that shades with it. RIB
gives a surface or a displacement shader as a name and parameters, MaterialX gives a document of
nodes, OSL gives shaders and their connections, and Hydra gives a network of nodes with named
terminals. Riley takes a material as a network of shading nodes, and Cycles and Arnold hold one
as a graph of shader nodes. None of them stores a MaterialX document as its material.

## Decision

A material holds a network of shading nodes. Each node names its shader and its parameter
values, a connection joins one node's output to another's input, and the network names a node's
output for each terminal, such as the surface and the displacement. The scene does not interpret
the shaders a network names. Compiling them is the material program's work.

## Alternatives

### A MaterialX document, held whole
- **For**: One form for every material, and MaterialX's own library checks and reads it.
- **Against**: RIB's shaders and OSL networks would each be translated into MaterialX before the
  scene holds them, and an OSL network does not map onto MaterialX's nodes. The scene would link
  MaterialX to hold any material at all.
- **Rejected because**: A network holds every client's form as it is, and MaterialX documents are
  networks already.

## Consequences

- **Gains**:
  - RIB, MaterialX, OSL and Hydra each give a material in their own shape, with no translation
    into another language.
  - A Hydra delegate copies a material network across with little conversion.
  - The scene needs no shading library, and a material whose shader nothing compiles is still
    held, so the program that compiles it can report it.
- **Costs**:
  - A network is checked only when it is compiled, so an error in one shows late.
  - The material program reads nodes from several languages, and keeps their names apart.
- **Revisit when**: every client gives materials as MaterialX, or a client needs the scene to
  check its networks when they are set.
