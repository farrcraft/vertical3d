# ADR-0160: Build: the portable job uses the oldest supported Clang

**Status**: accepted
**Date**: 2026-11-03
**Tags**: build
**Amends**: [ADR-0107](0107-build-the-api-libraries-build-with-clang-off-windows.md)
**Documented in**: [contributing/Build.md](../../contributing/Build.md#building-off-msvc)

## Context

The portable job exists so that a consumer can build the api with a compiler other than MSVC,
and the consumer it names ships on macOS with Apple's Clang. Apple's Clang trails the LLVM
releases. Each Clang release accepts more of the newer standards, so a newer compiler in CI
lets through code that an older one rejects. The Linux distribution offers several Clang
versions at once, and its default moves with each release.

## Decision

The job builds with the oldest Clang the tree supports, named explicitly, which is Clang 20.
The floor rises only when the tree needs something Clang 20 lacks.

## Alternatives

### The newest Clang the distribution offers
- **For**: the newest warnings under `-Werror`, and the longest time before the version is
  dropped from the archive.
- **Against**: it accepts the most, so it catches the least of what a consumer's older compiler
  rejects.
- **Rejected because**: the job's purpose is the consumer's compiler, and clang-tidy already
  brings new diagnostics on Windows.

### The distribution's default Clang
- **For**: what a Linux developer gets from a plain install, with nothing to choose.
- **Against**: the version changes with the image, unannounced, and moves away from Apple's.
- **Rejected because**: a compiler that changes without a change to the tree makes a failure
  look like a regression in the code.

## Consequences

- **Gains**: code that needs a newer compiler than the floor fails in CI, not in a consumer's
  build. The version in CI changes only by a change to the tree.
- **Costs**: the job misses the newest Clang's diagnostics. Raising the floor is a decision to
  take each time, and the distribution will drop Clang 20 in a later release.
- **Revisit when**: Ubuntu stops packaging Clang 20, Apple's Clang moves past it, or the tree
  needs a feature that Clang 20 lacks.
