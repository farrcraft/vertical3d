# ADR-0245: Offline: grids are held as NanoVDB on every device

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes, build
**Amends**: ADR-0187
**Documented in**: [offline/Kernel.md](../../offline/Kernel.md)

## Context

Tracking reads a medium's grid at every collision, so the grid's layout sets the speed of every
volume render. Kernel code includes only its own headers, glm and a few standard headers, and the
same source is to run on HIP and CUDA as well as the CPU. OpenVDB reads the files, and its tree is
host code that no GPU compiler builds. NanoVDB is the read-only, linear form of a VDB tree that
pbrt-v4 and Cycles render from on the CPU and the GPU, with OpenVDB's grids converted to it on the
host. Each of its nodes stores the least and most of the values below it, and
null-scattering tracking's majorants need exactly those.

## Decision

Each grid is converted on the host from OpenVDB to NanoVDB by NanoVDB's own tools, held on the
device as its buffer, and read by kernel code through NanoVDB's headers with trilinear filtering.
NanoVDB's headers join what kernel code may include, beside glm. A grid's majorants are the most
its nodes store, walked by NanoVDB's hierarchical DDA.

## Alternatives

### moya's own sparse layout
- **For**: The kernel's include rule and ADR-0187 stay whole, and a layout of bricks under a
  coarse index is fitted to tracking alone.
- **Against**: moya writes and keeps a conversion, a lookup and a walk over majorants that
  NanoVDB already provides and has proven on the devices moya will add.
- **Rejected because**: NanoVDB is the layout the GPU renderers converged on, and owning another
  buys nothing a render shows.

### Dense bricks through the texture cache
- **For**: A grid larger than memory loads its tiles on demand, as images do.
- **Against**: Tracking reads far more voxels a ray than a surface reads texels, and a lookup
  through the cache calls the host for each texel on the CPU. A sparse grid pays for its empty
  tiles, and no GPU texture cache exists.
- **Rejected because**: It puts tracking behind moya's slowest path.

## Consequences

- **Gains**:
  - One layout for every device, already proven on the CPU, CUDA and HIP.
  - Majorants and their walk come with the layout.
  - A sparse grid takes about the memory its VDB tree does.
  - `.nvdb` files can be read as they are.
- **Costs**:
  - A third-party header enters kernel code, and with it NanoVDB's layout and versions.
  - vcpkg builds NanoVDB only with CUDA, so an overlay port builds its headers alone.
  - A grid larger than memory does not render.
  - A volume shader whose density is not a scale of one field needs a bound of its own, since a
    node's most is the field's.
- **Revisit when**: NanoVDB stops being maintained, a GPU back end cannot build its headers, or
  scenes need grids larger than memory.
