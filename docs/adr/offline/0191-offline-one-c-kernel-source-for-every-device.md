# ADR-0191: Offline: one C++ kernel source for every device

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, build
**Documented in**: [offline/Kernel.md](../../offline/Kernel.md)

## Context

moya renders on the CPU and has to reach comparable results on GPUs. A GPU's ray tracing
hardware is reached only through its vendor's interface: HIP RT on AMD, OptiX on NVIDIA, and
Vulkan or DirectX ray tracing on any. Two sources of the integrator and the materials drift
apart. RenderMan XPU and Cycles compile one C++ source for the CPU and through CUDA or HIP for
the GPU. Vulkan ray tracing needs a separate shading language, and its motion blur and curves are
NVIDIA extensions.

## Decision

The integrator, materials, lights, sampling and volumes are written once, in a restricted subset
of C++ that the CPU compilers, HIP and CUDA all accept. A device layer holds what differs:
memory, ray traversal and the texture cache. The device back ends are the CPU, HIP with HIP RT,
and CUDA with OptiX.

## Alternatives

### Vulkan ray tracing, with kernels in Slang
- **For**: Every GPU vendor, and the API the realtime renderer uses.
- **Against**: No production offline renderer is built on it. Motion blur and curves are NVIDIA
  extensions. The kernels would be in a second language, and Slang's CPU target is preliminary.
- **Rejected because**: It limits quality and performance to what real-time ray tracing offers.

### CUDA and OptiX only
- **For**: The most capable GPU ray tracing, and what most GPU path tracers use.
- **Against**: NVIDIA only.
- **Rejected because**: moya should run on any capable GPU, and HIP costs little beside CUDA.

### A CPU renderer with no GPU path
- **For**: One device, and no subset rules.
- **Against**: Production renderers are moving their final frames to the GPU.
- **Rejected because**: Performance is a goal, and a GPU path added later needs a rewrite.

## Consequences

- **Gains**:
  - One integrator and one material library on every device, so results stay close.
  - Each vendor's ray tracing hardware is used through its own interface.
- **Costs**:
  - Shared code avoids what GPUs cannot run: virtual calls, exceptions and arbitrary
    allocation.
  - Each GPU back end is an optional build with its vendor's SDK, and CI without a GPU tests only
    the CPU.
  - HIP and CUDA builds sit outside the MSVC toolchain the rest of the tree uses.
- **Revisit when**: a portable interface reaches feature parity with HIP RT and OptiX.
