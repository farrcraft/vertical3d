# ADR-0190: Offline: materials are a BSDF library and MaterialX graphs

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, shaders, interchange
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

A path tracer's integrator evaluates and samples a surface's scattering, so a material has to
describe scattering, not compute a colour. The same material has to give the same result on the
CPU and on every GPU moya supports. Pipelines describe materials in MaterialX and OpenPBR Surface,
and many studios write patterns in OSL. OSL runs on CPUs and through OptiX only. Cycles keeps a
node machine of its own beside OSL for that reason.

## Decision

moya has a library of scattering models of its own, written once for every device: OpenPBR
Surface for surfaces, with models for hair, volumes and stylised looks beside it. A material is a
MaterialX graph of patterns feeding those models, compiled into a small node program that every
device runs. OSL is a second way to write patterns, on the devices OSL supports.

## Alternatives

### OSL as the shading language
- **For**: The industry's shading language, with closures and the richest patterns.
- **Against**: It runs only on CPUs and NVIDIA GPUs. It brings LLVM, and the closures still need
  an implementation in moya on each device.
- **Rejected because**: A material would not run on every device moya supports.

### A shading language of moya's own, compiled for each device
- **For**: Full control, and programmable on every device.
- **Against**: A compiler to maintain, with no other tool writing it. MaterialX content still
  needs a translator.
- **Rejected because**: MaterialX and OSL already cover authoring, and a third language adds
  nothing a pipeline can use.

### NVIDIA's MDL
- **For**: Generates evaluation and sampling code for CPUs and GPUs.
- **Against**: One vendor owns it, and it is a second material language beside MaterialX.
- **Rejected because**: MaterialX is the standard pipelines exchange.

## Consequences

- **Gains**:
  - Every material runs on every device, with the integrator in charge of sampling.
  - MaterialX and OpenPBR materials from USD render as authored.
  - Scattering models are moya's own, under ADR-0187.
- **Costs**:
  - Patterns beyond MaterialX's nodes need OSL, and run only where OSL runs.
  - Each scattering model is written to run on every device.
- **Revisit when**: OSL gains a back end for every GPU moya supports.
