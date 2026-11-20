# ADR-0229: Offline: stages take their texture access as a type

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, textures, code
**Documented in**: [offline/Textures.md](../../offline/Textures.md)

## Context

A material's program reads textures, and the tiles a lookup needs may not be in memory. Each
device has to give the same picture whatever its cache holds. On the CPU the integrator runs in
a library per instruction set that links no host code, and a cache too small for one lookup's
tiles still has to render. A GPU cannot call the host from inside a kernel, so its lookups can
only record a miss for the host to load after the pass.

## Decision

Each stage that runs a material's program takes the device's texture access as a template
parameter, beside its traversal, and each device instantiates the stages with its own. The CPU's
reads each texel through a host function that loads the tile it lies in, so a lookup never
misses. A test's is an image held whole in memory, which every device's lookups are checked
against.

## Alternatives

### The traversal type answers texture lookups too
- **For**: No stage gains a parameter, and a device still hands the stages one type.
- **Against**: One type mixes ray queries with texel reads, and a test cannot pair the reference
  traversal with the CPU's textures, or the reverse.
- **Rejected because**: Traversal and textures are checked against different oracles.

### A tile table on every device, and missed samples run again
- **For**: One shape of lookup on every device, as GPU renderers with demand loading run.
- **Against**: A sample runs again until every tile it reads is resident at once, so the cache
  must hold any one sample's tiles, and the CPU pays for passes it does not need.
- **Rejected because**: A cache smaller than one sample's tiles could never finish a sample.

### A function declared in kernel code and defined per device build
- **For**: The shape Cycles' image lookups take, with no template reaching the stages.
- **Against**: One binary holds one definition, so a test could not compare two in one run.
- **Rejected because**: The tests and the CPU's instruction sets need several in one binary.

## Consequences

- **Gains**:
  - A lookup reads the same texels whatever the cache holds, on the CPU with any memory limit.
  - The in-memory oracle checks every device's lookups, as the reference traversal checks
    traversals.
  - A GPU's demand loading fits behind the same shape when its back end arrives.
- **Costs**:
  - Every stage that shades is instantiated once more for each pair of types, which grows
    compile time and code size.
  - The CPU calls into the host for each texel, which costs a call and a lock where a resident
    table would cost a load.
- **Revisit when**: a GPU back end needs the stages to be one function, or texel calls dominate
  a textured render's time on the CPU.
