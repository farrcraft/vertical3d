# ADR-0223: Offline: a material is an interpreted node program

**Status**: accepted
**Date**: 2026-11-11
**Tags**: offline, shaders
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

A MaterialX material is a graph of pattern nodes that feeds a surface shader, and most of its
patterns vary over the surface. The CPU, HIP and CUDA devices run one kernel source compiled
ahead of time, and each device has to give the same picture. The editor's interactive render
changes materials while it renders. Many MaterialX nodes, `open_pbr_surface` among them, are
defined as graphs of other nodes.

## Decision

A material is compiled on the host into a program of instructions over a store of registers,
which one interpreter in the kernel source runs on every device. Compiling flattens every node
that has a graph implementation into standard nodes, and stops at surface shader and BSDF nodes,
which become instructions that build the closure list. An instruction reads the shading point,
a primvar by the slot the compiler gave it, or a texture by its handle in the texture cache.

## Alternatives

### Code generated for each material and compiled for each device
- **For**: Each node is inlined, so a material shades fastest. Karma XPU and MDL through OptiX
  compile materials this way.
- **Against**: Every device needs a compiler while rendering, and a cache of what it compiled
  that holds only for one GPU and driver. A new scene or a new material pauses for the compile.
  The materials stop being written once in the kernel source.
- **Rejected because**: It costs the one kernel source and an interactive material change, for
  a speed the interpreter's share of a render does not justify.

### A material evaluated on the host into constants and textures
- **For**: The cheapest on the device, which reads only constants and texture lookups.
- **Against**: Every pattern that varies with the shading point's position is lost, or has to
  be baked into a texture.
- **Rejected because**: Most of what a material does varies over the surface.

## Consequences

- **Gains**:
  - One program format and one interpreter serve every device, with nothing compiled while
    rendering.
  - A changed material is a new program, rebuilt as cheaply as the hierarchy.
  - Each node is tested once, against MaterialX's definition, for every device.
- **Costs**:
  - Each instruction costs a branch, and on a GPU paths running different materials diverge.
  - A node MaterialX defines in code alone has to be written as an instruction, and one that is
    missing renders as its default.
  - The register store has a size, and a graph that needs more than it holds is refused.
- **Revisit when**: Profiling shows the interpreter dominates the time a material takes, and a
  compiled route for MaterialX becomes worth a compiler on each device.
