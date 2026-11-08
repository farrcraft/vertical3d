# ADR-0197: Offline: kernel code uses glm for vectors and matrices

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, code
**Documented in**: [offline/Kernel.md](../../offline/Kernel.md)

## Context

Kernel code needs vector and matrix types that compile under MSVC, Clang, GCC, HIP and nvcc. The
rest of the tree uses glm, and the scene interface takes glm from the editor. glm 1.0 marks each
of its functions as both host and device code under nvcc and under HIP. Cycles and pbrt-v4 write
types of their own, and OSL uses Imath's, which also compile for a device. A spectrum of four
wavelengths and the lanes of a SIMD traversal are not geometric vectors in any of these.

## Decision

Kernel code uses glm's vectors, matrices and quaternions, with glm's SIMD intrinsics left off.
Sampled spectra and SIMD lanes are types of moya's own. Data stored in device memory is laid out
by the device layer, not by glm's types.

## Alternatives

### Types of moya's own, as Cycles and pbrt-v4 have
- **For**: Layout, alignment and every function are moya's, sized for the kernel.
- **Against**: A second set of vector types in the tree, with conversions at the scene
  interface and in every test.
- **Rejected because**: glm already compiles for each device, so the second set buys nothing a
  storage layout does not.

### Imath, as OSL uses
- **For**: Compiles for a device, and comes with OpenEXR.
- **Against**: A second vector library beside glm, with conversions at every boundary.
- **Rejected because**: Its gain over glm is none, and its cost is the conversions.

## Consequences

- **Gains**:
  - The scene interface, the editor and kernel code share one set of types, with no conversion.
  - glm's functions are tested by a project much larger than moya.
- **Costs**:
  - glm's `vec3` is twelve bytes, which a GPU loads in three parts, so device buffers keep their
    own padded layout.
  - Rounding agrees across CPU compilers only if each compiles kernel code with the same
    contraction of multiplies and adds, which the kernel target has to pin.
  - A glm release that breaks a device compiler blocks that back end until it is fixed.
- **Revisit when**: glm drops device support, or a GPU back end measures a cost in its types.
