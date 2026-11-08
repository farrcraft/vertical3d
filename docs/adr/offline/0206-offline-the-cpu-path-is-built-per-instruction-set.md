# ADR-0206: Offline: the CPU path is built per instruction set

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline, build
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

The CPU traversal is held to Embree's speed, which needs AVX2 and AVX-512 where a machine has
them. A binary built for AVX2 does not start on a CPU without it. MSVC has no attribute that
compiles one function for several instruction sets, so a source is compiled for one with a flag
of its own. AVX-512 no longer lowers the clock on current Intel and AMD CPUs.

## Decision

The CPU's fused path, its stages and its traversal, is compiled once for each of SSE4.2, AVX2 and
AVX-512, and the CPU device chooses the newest the processor reports when it is made. An option
when the device is made names a lower one, so every variant can be tested on one machine. SSE4.2
is the oldest instruction set moya runs on.

## Alternatives

### One instruction set chosen when moya is built
- **For**: One compilation, and no choice at run time.
- **Against**: A release either leaves AVX2 unused or does not start on an older CPU. A test runs
  one variant per build.
- **Rejected because**: Embree, Cycles and OIDN all choose at start-up for this reason.

### SSE4.2 and AVX2 only
- **For**: One variant fewer, as Cycles ships.
- **Against**: AVX-512 machines leave half their lanes unused, and no longer pay a clock penalty
  for them.
- **Rejected because**: The variant costs compile time alone.

### A traversal without SIMD beside the others
- **For**: A plain reference for each SIMD traversal.
- **Against**: A fourth traversal to keep, when the traversal that tests every primitive is
  already the reference.
- **Rejected because**: The brute-force traversal checks every variant.

## Consequences

- **Gains**:
  - Each machine runs the widest code it supports, and an older one still runs moya.
  - CI tests every variant on one runner through the option.
- **Costs**:
  - The fused path is compiled three times, which triples its build time and its share of the
    binary.
  - The variants round alike only where their code does, so each is checked against the
    reference.
  - A CPU older than SSE4.2 does not run moya.
- **Revisit when**: a newer instruction set doubles the lanes again, or the oldest CPU worth
  supporting has AVX2.
