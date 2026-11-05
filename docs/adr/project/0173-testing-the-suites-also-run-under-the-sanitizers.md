# ADR-0173: Testing: the suites also run under the sanitizers

**Status**: accepted
**Date**: 2026-11-04
**Tags**: testing, ci, build
**Documented in**: [Testing.md](../../contributing/Testing.md#sanitizers)

## Context

Every check in the tree reads the code: the compiler's warnings, `/analyze` and clang-tidy.
None of them watches the code run. A read past the end of a buffer, a use after free or a
signed overflow can pass every test and every analyser. It shows up later as a crash or a
wrong pixel, far from its cause. The Vulkan validation layer covers misuse of the Vulkan API,
not of memory.

## Decision

A build option instruments the tree with AddressSanitizer, and with the undefined behaviour
sanitizer where the compiler has one, and a report fails the program that made it. The Linux
CI job builds its libraries with both and runs every suite under them. On Windows, an `asan`
preset builds the whole tree with MSVC's AddressSanitizer, for a local run.

## Alternatives

### A sanitized Windows build in CI as well
- **For**: covers the code only Windows builds, the renderer, the window and the apps, on
  every pull request.
- **Against**: a second full build and test run of the Windows job, which is already the
  longest, and MSVC has no undefined behaviour sanitizer to add.
- **Rejected because**: the Linux job covers both sanitizers on the libraries without a
  device, where the parsing and the data structures are, for a few minutes of build time.

### Valgrind or Dr. Memory over an unchanged build
- **For**: no second build configuration, and it sees the third party code as well.
- **Against**: runs tens of times slower, Valgrind does not run on Windows, and neither
  reports undefined behaviour.
- **Rejected because**: the sanitizers find the same faults at a fraction of the run time.

### A separate Linux job for the sanitized build
- **For**: the plain Clang build stays exactly what a consumer builds.
- **Against**: the same libraries are built and tested twice.
- **Rejected because**: the sanitizers change no rule of what compiles, so the plain build
  would check nothing the sanitized one does not.

## Consequences

- **Gains**: every suite on Linux is also a memory and undefined behaviour check, and a new
  test gains both without asking. Windows-only code can be checked the same way locally.
- **Costs**: the Linux tests run a few times slower. The vcpkg packages are not instrumented,
  so a fault inside one goes unreported. The Windows-only code is checked only when someone
  runs the `asan` preset, and its build directory needs a package install of its own.
- **Revisit when**: the Windows job gets faster or a fault turns up that only the Windows-only
  code could have shown, either of which argues for a sanitized Windows build in CI.
