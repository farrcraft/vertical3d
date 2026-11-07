# ADR-0187: Offline: moya owns its algorithms, libraries own formats

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, build, interchange
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

moya is meant to stand on its own, render to a high standard, and plug into other pipelines.
Most of what a renderer needs exists as libraries: ray traversal, texture caching, sampling,
subdivision, colour management, denoising and file formats. A library that does a renderer's
core work fixes how that work behaves, and its release schedule and its owner's priorities with
it. A pipeline expects moya to read and write the same files as every other tool, exactly as the
reference library does.

## Decision

moya implements its own rendering algorithms: acceleration structures, sampling, materials,
lights, volume tracking, the texture cache and its filtering, and tessellation. It uses the
reference library for each file format and standard it reads or writes: OpenEXR, OpenUSD,
MaterialX, OpenColorIO and OpenVDB. OpenSubdiv evaluates subdivision surfaces and OIDN denoises,
as the two exceptions.

## Alternatives

### Build on the established libraries throughout
- **For**: Embree, OpenImageIO's texture system and OSL are fast and widely used.
- **Against**: moya's ray traversal, texture filtering and shading would behave as those
  libraries do. Curves would intersect differently on the CPU and the GPU, as they do in Cycles.
- **Rejected because**: Quality and consistency across devices depend on owning the algorithms.

### Implement everything, the formats included
- **For**: No dependency at all.
- **Against**: A format moya implements itself can disagree with the reference library, and a
  pipeline sees that as a moya bug.
- **Rejected because**: A format exists so that tools agree, and the reference library defines
  agreement.

## Consequences

- **Gains**:
  - The parts of moya that set quality and speed are moya's own, and can be tested and changed.
  - Files moya reads and writes match what other tools read and write.
- **Costs**:
  - moya writes and maintains a wide BVH, a texture cache and a tessellator that libraries
    already provide.
  - OpenSubdiv's limit surface and OIDN's network are not moya's own.
- **Revisit when**: a reference library stops being maintained, or subdivision or denoising
  need behaviour the library cannot give.
