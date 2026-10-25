# ADR-0107: Build: the api libraries build with Clang off Windows

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: [contributing/Build.md](../contributing/Build.md)

## Context

The tree is developed and verified with MSVC on Windows, and its CMake hands MSVC's spellings of
its flags to whatever compiler configured it, including through the interface of every api
library. A game that consumes the api as source therefore cannot be built by any other compiler,
for no reason in the code. A game that wants to ship on macOS needs the api to compile under
Clang, and Linux is where that can be checked cheaply in CI. Most of the api has no platform in
it: the grid, the types, the event system, the ECS helpers, the asset reader and the log. A
compiler-neutral build that nothing exercises decays as soon as the next MSVC-only flag or
extension lands.

## Decision

Every compiler flag in the build is guarded by compiler, beside its Clang and GCC equivalent, so
the build has no MSVC-only spelling. CI builds and tests the api libraries that need neither
Vulkan nor SDL with Clang on Linux, on every pull request. MSVC stays the compiler the tree is
developed and fully verified with, and the Windows gates are unchanged.

## Alternatives

### Stay MSVC-only, and say so
- **For**: no new CI job, no second dependency cache, and no Linux-only breakage for a Windows
  developer to fix blind.
- **Against**: every consumer that wants another platform has to patch the api's CMake, which is
  the local fork that authoring engine changes upstream exists to prevent.
- **Rejected because**: the flags block other compilers for no reason in the code, and the fix
  loses nothing on MSVC.

### Guard the flags, with no build to check them
- **For**: unblocks a consumer at once, at no CI cost.
- **Against**: nothing notices the next unguarded flag, or a header that only MSVC accepts, until
  a consumer's build breaks.
- **Rejected because**: an unenforced rule in a tree that is otherwise gated is a rule that has
  already been broken by the time anyone looks.

### Check it with clang-cl on Windows
- **For**: reuses the Windows runner, its Vulkan SDK and its dependency cache.
- **Against**: clang-cl accepts MSVC's flag spellings and uses MSVC's standard library, so it
  passes against today's build.
- **Rejected because**: it cannot fail on the defect it would exist to catch.

### Check it with Clang's GNU driver on Windows
- **For**: rejects MSVC's spellings, and still reuses the Windows runner and cache.
- **Against**: it still compiles against MSVC's standard library and the Windows headers, so a
  dependency on either goes unnoticed.
- **Rejected because**: it proves the flags and not the code, and a consumer off Windows needs
  both.

### Check it on a macOS runner
- **For**: the platform a consumer actually wants, with Apple's Clang and libc++.
- **Against**: a macOS runner costs several times a Linux one and builds the dependencies again
  from cold, and Metal leaves the renderer out either way.
- **Rejected because**: Clang on Linux catches the compiler and platform faults at a fraction of
  the cost, and a consumer that ships on macOS runs its own macOS build.

## Consequences

- **Gains**: a consumer can build the platform-free api libraries with another compiler, and CI
  keeps it that way. A flag added to the build states what it means on every compiler.
- **Costs**: a second CI job, with a dependency cache of its own that takes most of an hour to
  fill cold. A change to a covered library can pass every local gate and fail on Linux, and there
  is no Linux machine here to reproduce it on. The covered libraries cannot use an MSVC extension
  or a Windows header.
- **Revisit when**: a consumer needs the renderer, the window or the audio off Windows, which
  means Vulkan and SDL on Linux in CI. Or when the Linux job's cost outweighs a consumer that has
  stopped needing it.
