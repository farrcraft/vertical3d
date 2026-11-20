# ADR-0212: Offline: the path state is declared once, laid out per device

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline, code
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

The integrator's stages pass a path's state from one to the next. On the CPU they run back to
back on one thread, so the state can live on that thread's stack. On a GPU each stage is a kernel
over a queue of paths, so the state lives in device memory between kernels. A GPU reads fastest
when neighbouring threads read neighbouring values of one field. The ZSobol sampler holds no
state, and a queued stage draws its random numbers without knowing what earlier stages drew.

## Decision

The path state is declared once in kernel code, and each device lays it out: a structure per path
on the CPU, and an array per field on a GPU. A shadow ray
is a state of its own, which the CPU traces when it is made and a GPU queues. Each bounce owns a
fixed block of the sampler's dimensions, and each draw a fixed place in its block.

## Alternatives

### One structure per path on every device
- **For**: The simplest code. A GPU keeps an array of structures and reaches them through queues
  of indices.
- **Against**: A stage that reads one field reads the whole structure on a GPU. The cost grows
  with every field that volumes, subsurface scattering and path guiding add.
- **Rejected because**: It costs every GPU stage memory bandwidth, and the state only grows.

### Layouts generated from a description by a build tool
- **For**: The layout of each device is plain generated code, as pbrt-v4's `soac` makes it.
- **Against**: A tool runs in the build, and the generated headers have to pass the kernel's
  rules. A reader of a stage sees accessors that are not in the tree.
- **Rejected because**: C++20 lays out both forms from one declaration without a tool.

### The next free dimension for each draw
- **For**: No block of dimensions is reserved, so no dimension is left unused.
- **Against**: A draw's dimension depends on every draw before it in the path. A queued stage
  would have to carry a counter, and a draw added to one stage would move every later one.
- **Rejected because**: A GPU stage could not draw its own numbers, and a change would move the
  draws of other stages.

### A shadow ray held in the path's state
- **For**: One state per path, and no second queue.
- **Against**: A GPU has to trace a path's shadow ray before the path moves on, and a stage that
  makes several shadow rays has nowhere to put them.
- **Rejected because**: Shadow rays are traced fastest in a queue of their own.

## Consequences

- **Gains**:
  - The CPU keeps the fused path with its state on the stack, and a GPU reads each field in one
    access.
  - The two layouts cannot drift apart, since both come from one list.
  - Any stage draws its numbers from the pixel, the sample, the bounce and its place, on any
    device.
- **Costs**:
  - Stage code reaches a field through an accessor rather than a member.
  - A bounce's block of dimensions is sized for the most draws any bounce makes, so most bounces
    leave some unused.
  - A GPU holds shadow states beside path states, and sizes both queues before a launch.
- **Revisit when**: a GPU back end shows that grouping some fields reads faster than one array
  for each, or a bounce's draws outgrow a fixed block.
