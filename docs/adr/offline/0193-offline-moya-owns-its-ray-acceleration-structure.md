# ADR-0193: Offline: moya owns its ray acceleration structure

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline
**Documented in**: [offline/Traversal.md](../../offline/Traversal.md)

## Context

A production scene needs instancing, motion blur over several segments, curves, points and
subdivision surfaces in its ray traversal. Embree provides all of them on the CPU. On a GPU, ray
tracing hardware is reached only through HIP RT or OptiX, and HIP RT has no cubic curves. A curve
that the CPU and the GPU intersect by different code renders differently on each, as Cycles
found with Embree.

## Decision

moya builds and traverses its own bounding volume hierarchy on the CPU. It is wide, traversed
with SIMD, built by binned surface area with spatial splits, holds motion over several segments,
and nests instances. Every primitive but the triangle is intersected by moya's own code on every
device, and a GPU's hardware traverses boxes and triangles.

## Alternatives

### Embree on the CPU
- **For**: Highly tuned, Apache licensed, with every primitive and motion kind built in.
- **Against**: Curves and points intersect by Embree's code on the CPU and moya's on the GPU.
  Its geometry types, precision and releases are Intel's.
- **Rejected because**: Agreement between devices and control of robustness need one
  intersection routine per primitive.

### Software traversal on the GPU too
- **For**: One traversal everywhere.
- **Against**: It leaves the GPU's ray tracing hardware unused.
- **Rejected because**: It gives up most of the GPU's ray tracing speed.

## Consequences

- **Gains**:
  - Curves, points and other primitives intersect the same way on every device.
  - Watertight triangle tests, self-intersection offsets and motion are moya's own, and tested.
  - The hierarchy is built from moya's own scene, with no copy into another library's types.
- **Costs**:
  - The CPU traversal has to reach Embree's speed to make up for not using it.
  - Triangles still intersect by different code on a GPU's hardware, so results there agree
    within a tolerance.
  - ADR-0092's hierarchy is replaced, and that record is amended.
- **Revisit when**: GPU ray tracing interfaces accept a renderer's own intersection for every
  primitive at hardware speed.
