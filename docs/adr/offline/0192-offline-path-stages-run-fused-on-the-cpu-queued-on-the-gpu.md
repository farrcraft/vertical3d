# ADR-0192: Offline: path stages run fused on the CPU, queued on the GPU

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

A path tracer can follow one path per thread from camera to end, which is called a megakernel.
It can instead keep each path's state in memory and run a small kernel per stage over a queue of
paths, which is called wavefront execution. On a GPU, one large kernel diverges and reserves
registers for its worst branch. On a CPU, a wavefront writes and reads every path's state between
stages. Cycles runs the same stage code as a megakernel on the CPU and as a wavefront on the GPU.

## Decision

The integrator is a set of stage functions over an explicit path state: generate, intersect,
shade, sample lights, scatter and accumulate. The CPU runs the stages back to back for one path
at a time on each thread. A GPU back end runs each stage as its own kernel over queues of paths,
sorted by material.

## Alternatives

### Wavefront execution on every device
- **For**: The CPU and the GPU run the same schedule.
- **Against**: On a CPU it is slower than a megakernel, as pbrt-v4 reports for its own.
- **Rejected because**: It costs the CPU speed and gains nothing the shared stage code does not.

### A megakernel on every device
- **For**: The simplest code, with no path state in memory.
- **Against**: Divergence and register pressure on a GPU grow with the number of materials.
- **Rejected because**: GPU performance falls as scenes grow.

## Consequences

- **Gains**:
  - The CPU runs at full speed, and a GPU back end changes the schedule rather than the code.
  - Path state is explicit, so it can be laid out as one array per field on a GPU.
- **Costs**:
  - Stage functions take all their state as arguments, which is more verbose than one loop.
  - The CPU and GPU schedules order work differently, so their results agree statistically,
    not exactly.
- **Revisit when**: CPUs or GPUs change enough that one schedule suits both.
