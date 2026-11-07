# ADR-0194: Offline: stateless samples, one picture per device

**Status**: accepted
**Date**: 2026-11-06
**Tags**: offline, testing
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

A path tracer draws many random dimensions per sample, and converges faster on low-discrepancy
sequences than on jittered strata. Threads and GPU queues finish paths in any order. CPU and GPU
ray tracing hardware compute hit points with different rounding, and no production renderer
guarantees the same pixels on both. moya's tests compare renders with reference images.

## Decision

A sample's value depends only on its pixel, its index, its dimension and a seed that includes the
frame number, through an Owen-scrambled Sobol sequence in Morton order (pbrt-v4's ZSobol).
Accumulation is in a fixed order, so each device and build gives the same picture on every run.
A GPU's picture is compared with the CPU's within a statistical tolerance, not exactly.

## Alternatives

### Independent random numbers per sample
- **For**: Simple, and stateless in the same way.
- **Against**: Slower convergence and clumped noise.
- **Rejected because**: It wastes samples.

### A sampler that keeps state per pixel
- **For**: Some sequences adapt better as samples are added.
- **Against**: The result depends on the order in which work runs.
- **Rejected because**: It breaks the same picture on every run.

### Exact agreement between CPU and GPU
- **For**: One reference image for every device.
- **Against**: Hardware ray tracing rounds its own way, and fused operations differ by compiler.
- **Rejected because**: No device interface gives the guarantee.

## Consequences

- **Gains**:
  - The same picture on every run of one device and build, under any thread count.
  - Blue-noise error, which reads as less noisy and denoises better.
- **Costs**:
  - A GPU back end needs a statistical comparison in its tests, not a pixel comparison.
  - Screen-space sequences suit bidirectional methods poorly.
  - ADR-0076's jittered strata are replaced, and that record is amended.
- **Revisit when**: a bidirectional method joins the core.
