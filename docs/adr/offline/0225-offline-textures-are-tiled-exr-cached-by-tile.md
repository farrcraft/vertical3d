# ADR-0225: Offline: textures are tiled EXR, cached by tile

**Status**: accepted
**Date**: 2026-11-11
**Tags**: offline, textures, files
**Documented in**: [plans/MoyaMaterials.md](../../plans/MoyaMaterials.md)

## Context

A production scene's textures do not fit in a GPU's memory, nor always in the host's. Each device
has to give the same picture on every run, so a lookup cannot read a coarser level because the
tile it needs is absent. Filtering is correct only in a linear space, and textures arrive in
sRGB, in other spaces, or as data that is no colour at all. The CPU, HIP and CUDA devices run one
kernel source. A texture is named by a path that may be relative.

## Decision

A texture is a tiled, mipmapped OpenEXR file whose colour is converted into ACEScg when it is
made, and any other image is made into one in a cache directory. Tiles are cached within a memory
limit, and a lookup whose tile is absent runs again once the tile is loaded, after the pass on a
GPU. A path is found relative to the file that names it, then along RI's texture search path.

## Alternatives

### Every image loaded whole and mipmapped in memory
- **For**: No miss and nothing made ahead of time, the same on every device.
- **Against**: Memory grows with every texture in the scene.
- **Rejected because**: A production scene's textures do not fit. Cycles moved to a tile cache
  for that reason.

### OpenImageIO's cache and texture system
- **For**: Mature, reads every common format, and filters as Arnold and RenderMan have used it.
- **Against**: It runs on the CPU alone, and its algorithms are not moya's.
- **Rejected because**: The GPU devices would still need a cache of moya's own, and two caches
  would filter differently.

### A coarser level read when a tile is absent
- **For**: A lookup never waits, as RenderMan XPU trades resolution for memory.
- **Against**: The picture depends on what the cache held when the lookup ran.
- **Rejected because**: Every device gives one picture on every run.

### Colour converted at lookup
- **For**: One made file serves every colour space a texture is read in.
- **Against**: Its levels are filtered in the source's encoding, not in linear light.
- **Rejected because**: A texture's mip levels have to average light, not encoded values.

## Consequences

- **Gains**:
  - Memory stays within its limit whatever the scene's textures.
  - The same picture on every device, whatever the cache holds.
  - Mip levels are filtered in ACEScg, and a data texture is never converted.
- **Costs**:
  - Every texture is a made file on disk beside its source, and the first render of a new image
    waits for it to be made.
  - A GPU pass with misses runs part of its work twice.
  - An image read in two colour spaces is made twice.
- **Revisit when**: GPU drivers page sparse textures from the host fast enough that a device can
  wait on a miss inside a pass.
