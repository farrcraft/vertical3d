# ADR-0176: Releases: the Windows release links its libraries as DLLs

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases, build
**Documented in**: [Build.md](../../contributing/Build.md#the-install-tree)

## Context

A release installs eight apps side by side, and they share most of their libraries: SDL, ICU,
FreeType, HarfBuzz, thorvg and Boost among them. Every build and every test run links them as
DLLs, through vcpkg's `x64-windows` triplet. vcpkg's `usd` port builds OpenUSD only as DLLs,
whatever the triplet, and OpenUSD finds its plugins beside them. The Vulkan loader comes with the
GPU driver.

## Decision

A Windows release builds with the `x64-windows` triplet, as every other build does. The install
holds one copy of each DLL the apps load, beside them, and leaves out the Vulkan loader.

## Alternatives

### Static libraries with the shared runtime, `x64-windows-static-md`
- **For**: each app carries its own libraries, so most DLLs are gone, and an app cannot load a
  wrong copy of one.
- **Against**: OpenUSD's 18 DLLs and its plugins still have to be gathered, so the install step
  is needed anyway. Each app carries its own copy of every library it uses. In a trial build
  the apps and libraries came to 104 MB, against 47 MB shared, and each demo to 12 MB rather
  than 1 MB. A release would link a second set of packages, built and cached apart from the set
  every test runs on.
- **Rejected because**: it keeps the DLLs it was meant to remove, doubles the install, and ships
  libraries no test has run against.

## Consequences

- **Gains**: the release links exactly what CI built and tested. One copy of each library serves
  every app, and OpenUSD's DLLs and plugins sit beside the others as Blender lays out its own.
- **Costs**: the install has to find and copy 41 DLLs, and leave out the system's and the GPU
  driver's. An app run with a DLL missing fails to start rather than to link.
- **Revisit when**: vcpkg's `usd` port builds OpenUSD as a static library, or the apps ship
  separately rather than as one product.
