# ADR-0181: Build: Linux builds with Clang and GCC

**Status**: accepted
**Date**: 2026-11-05
**Tags**: build, releases
**Documented in**: [Build.md](../../contributing/Build.md#building-off-msvc)

## Context

The tree builds with MSVC on Windows, and the libraries that need neither Vulkan nor SDL build
with Clang on Linux. The whole tree has to build on Linux for the Flatpak. The Freedesktop SDK
the Flatpak builds on compiles with GCC, and offers Clang through an extension. Each compiler
warns about things the other does not.

## Decision

On Linux the whole tree builds with both Clang and GCC, each in a CI job of its own with warnings
as errors. The Flatpak builds with GCC, its SDK's own compiler.

## Alternatives

### Clang alone
- **For**: one Linux build to keep, as the portable job keeps now, and Clang's diagnostics are the
  more detailed.
- **Against**: the Flatpak would build with the SDK's LLVM extension rather than its own compiler,
  and nothing would catch code only GCC rejects.
- **Rejected because**: the compiler a release builds with should be one CI builds with.

## Consequences

- **Gains**: three compilers check every change, and the Flatpak's compiler is one of them.
  Code that only one compiler accepts is found in CI, not in a release.
- **Costs**: a second Linux job, with its own dependency build to cache. A warning one compiler
  raises and the others do not has to be fixed for all.
- **Revisit when**: the Flatpak builds with another compiler, or a release ships for another
  platform.
