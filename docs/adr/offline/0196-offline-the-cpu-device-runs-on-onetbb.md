# ADR-0196: Offline: the CPU device runs on oneTBB

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, build
**Documented in**: [offline/Devices.md](../../offline/Devices.md)

## Context

The CPU device spreads tiles of an image over every core. Building a BVH needs nested parallel
work, and an interactive render has to stop when the scene changes. moya also runs inside other
processes, the editor and Hydra hosts among them, and OpenUSD in those hosts already schedules
its work on oneTBB, as OIDN does. Cycles and Embree schedule on oneTBB. pbrt-v4 and Mitsuba 3
each have a thread pool of their own.

## Decision

The CPU device schedules its work through oneTBB: tiles, the BVH build, and every other parallel
loop in the core. A render is stopped through TBB's cancellation. oneTBB moves from the
manifest's `usd` feature into the feature that holds moya's packages.

## Alternatives

### A thread pool of moya's own over `std::thread`
- **For**: No package, and every detail of the scheduler is moya's.
- **Against**: Work stealing, nested parallelism and cancellation have to be written. Inside a
  host that runs oneTBB, two schedulers each start a thread per core.
- **Rejected because**: It competes for cores with the host and with OIDN.

### The standard library's parallel algorithms
- **For**: No package, and part of C++17.
- **Against**: No control over grain size, no cancellation, and no nesting worth relying on.
  libstdc++ runs them on oneTBB anyway.
- **Rejected because**: A render cannot be stopped through them.

### OpenMP
- **For**: Loops are parallel by one pragma.
- **Against**: MSVC supports only OpenMP 2.0, without tasks. Clang needs its runtime shipped.
- **Rejected because**: Nested tasks are not portable across the tree's compilers.

## Consequences

- **Gains**:
  - One scheduler in a process that hosts moya, so moya's threads and the host's share the cores,
    and a host's thread limit applies to moya.
  - Work stealing for uneven tiles, and nested parallelism for the BVH build.
- **Costs**:
  - Every build of moya builds oneTBB, the Linux clang job included.
  - Scheduling is TBB's, so tile order is not fixed, and a picture must not depend on it.
  - Kernel code cannot call TBB, so the device calls kernel code and never the reverse.
- **Revisit when**: the C++ standard gains a scheduler with nesting and cancellation that the
  tree's compilers all ship.
