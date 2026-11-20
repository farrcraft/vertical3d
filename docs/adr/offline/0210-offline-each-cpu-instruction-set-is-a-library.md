# ADR-0210: Offline: each CPU instruction set is a library

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline, build
**Amends**: ADR-0206
**Documented in**: [offline/Devices.md](../../offline/Devices.md)

## Context

The CPU's fused path is compiled once for each of SSE4.2, AVX2 and AVX-512. An inline function
that is not a template on anything instruction-specific, such as glm's vector arithmetic or
`intersectTriangle`, has one symbol whichever source compiles it. When two sources compiled for
different instruction sets each emit a copy, the linker keeps one, and the SSE4.2 path can then
run an AVX2 instruction on a processor without it. Cycles avoids this by making every kernel
function static and using its own mathematics, and moya's kernel uses glm.

## Decision

Each instruction set's fused path is a shared library of its own, compiled whole for that
instruction set, which exports one function that gives a table of its entry points. The CPU
device loads the newest library the processor runs, from the directory of the running
executable, when it first needs one. Nothing compiled for one instruction set links into another.

## Alternatives

### Sources compiled for each instruction set in one binary, each in an inline namespace
- **For**: One binary, and kernel code's symbols differ by instruction set.
- **Against**: glm's functions still share symbols, and a debug build inlines none of them. Nothing
  in the build finds a copy the linker chose from the wrong source.
- **Rejected because**: It leaves a crash that appears only on older processors.

### The traversal alone for each instruction set, through intrinsics
- **For**: One binary. MSVC accepts intrinsics without `/arch`, and Clang and GCC take a target
  attribute on the traversal's functions.
- **Against**: Shading and sampling stay at SSE4.2, and MSVC mixes SSE and AVX encodings in one
  function.
- **Rejected because**: ADR-0206 chose the whole fused path for each instruction set.

## Consequences

- **Gains**:
  - No instruction a processor lacks can be reached, by construction.
  - Each library is the whole fused path at its instruction set, and a test loads each one.
- **Costs**:
  - Three more files to build, install and ship beside the apps.
  - A table of entry points between libmoya and each library, whose parameters both compile
    from the same headers and so must hold no instruction-specific type.
  - An executable that cannot find a library cannot render through the CPU device's traversal.
- **Revisit when**: MSVC compiles one function for several instruction sets, or glm can be
  compiled with internal linkage.
