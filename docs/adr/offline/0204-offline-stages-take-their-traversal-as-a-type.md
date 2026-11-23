# ADR-0204: Offline: stages take their traversal as a type

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline, code
**Documented in**: [offline/Traversal.md](../../offline/Traversal.md)

## Context

The integrator's stages are kernel code, and ray traversal belongs to each device: SIMD host code
on the CPU, HIP RT or OptiX on a GPU. On the CPU the stages run fused, so the stage that needs a
hit calls traversal directly. moya's tests compare every traversal with one that tests every
primitive, and the CPU builds a traversal for each instruction set it runs. HIP RT, OptiX and
Embree each hand back a distance, a primitive, a position on it and a chain of instances, and
none hands back a hit point.

## Decision

Each stage that traces a ray takes the traversal as a template parameter, and each device
instantiates the stages with its own. A hit holds its distance, its primitive, its position on
the primitive, its time and the chain of instances that placed it, to a depth fixed when moya is
compiled. The stage that shades a hit computes its point, normals and differentials from it.

## Alternatives

### One function declared in kernel code and defined per device build
- **For**: The shape Cycles uses. No template reaches the stages, and each back end compiles one
  definition.
- **Against**: One binary holds one traversal. The slow reference and each instruction set need a
  runtime branch inside the function, and kernel code chooses its back end by the preprocessor.
- **Rejected because**: The tests and the CPU's instruction sets both need several traversals in
  one binary.

### Traversal as a stage of its own over a queue of rays on every device
- **For**: Kernel code never calls traversal, and every device runs one shape of integrator, as
  pbrt-v4's wavefront integrator does.
- **Against**: The CPU writes and reads every path's state around each traversal. pbrt's own
  account finds this path slower on the CPU, and keeps it mainly for debugging.
- **Rejected because**: It gives up the fused CPU path that ADR-0192 chose.

### A hit that holds its point, normals and differentials
- **For**: The shade stage reads them directly, as pbrt-v4's surface interaction gives them.
- **Against**: A GPU's hit program computes them for every hit, including hits that a closer one
  replaces, and grows large.
- **Rejected because**: HIP RT and OptiX return the small record, and the shade stage computes
  the rest once.

## Consequences

- **Gains**:
  - The reference traversal, every CPU instruction set and each GPU's traversal sit behind one
    shape, and a test runs any of them over the same stages.
  - Kernel code names no device and needs no preprocessor choice of back end.
  - A hit maps directly onto what HIP RT, OptiX and Embree return.
- **Costs**:
  - The stages are instantiated once for each traversal, which grows compile time and code size.
  - The shade stage recomputes a hit's point and normals from its primitive.
  - Instances nest no deeper than the compiled limit.
- **Revisit when**: a GPU interface needs the stages to be a single function it compiles, or
  instances nest deeper than any fixed limit allows.
