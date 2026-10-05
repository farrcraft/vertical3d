# ADR-0007: CI: render tests on software Vulkan

**Status**: amended
**Date**: 2026-08-30
**Amended by**: [ADR-0054](0054-testing-golden-images-hold-only-spec-exact-output.md)
**Documented in**: [Testing.md](../contributing/Testing.md)

## Context

The renderer is Vulkan only, with no software fallback, so a render test needs a Vulkan device.
The repository is public and CI runs on GitHub's free tier, whose standard runners have no GPU.
GPU runners are part of the paid tier. The project builds only under MSVC, so a CI job that
compiles it must run on Windows.

## Decision

Render tests run in CI on a `windows-latest` runner, against Mesa's lavapipe, a software Vulkan
implementation. The assertion every render test makes is that the validation layers report
nothing. A test does not depend on a reference picture produced by one implementation.

## Alternatives

### Linux runner with lavapipe
- **For**: The cheapest setup. Lavapipe is one package install on `ubuntu-latest`, with no
  third-party download to pin.
- **Against**: The project would have to build under gcc or clang. That means removing the
  MSVC-only compiler flags and auditing every Windows assumption in the tree.
- **Rejected because**: Portability is a large piece of work that nothing else needs, and CI
  alone does not justify it.

### Self-hosted runner with a real GPU
- **For**: Tests run on a real driver, which is the only place driver-specific bugs appear. No
  GitHub charge.
- **Against**: A machine must stay online and maintained. On a public repository, a pull request
  from a fork can run arbitrary code on a self-hosted runner, and GitHub warns against it.
- **Rejected because**: The security exposure is not acceptable for a public repository, and
  workflow configuration cannot fully close it.

### No render tests in CI
- **For**: No infrastructure, no flaky tests, and the fastest CI.
- **Against**: The Vulkan layer, the largest body of new code, gets no automated regression
  protection.
- **Rejected because**: The renderer is the code most in need of a regression check.

### Golden images as the general assertion
- **For**: Catches visual regressions that validation layers cannot see.
- **Against**: A reference is stable only on the implementation that produced it, so a picture
  made on lavapipe says nothing about a hardware driver. It also adds binary files and a
  tolerance to tune, a common source of flaky tests.
- **Rejected because**: Validation errors are the most common class of Vulkan defect, and
  checking for them needs no reference data. Which pictures can be compared exactly is
  [ADR-0054](0054-testing-golden-images-hold-only-spec-exact-output.md).

## Consequences

- **Gains**:
  - No new spending, and no dependency on the project becoming portable.
  - Lavapipe is a conformant Vulkan 1.3 implementation, so it meets
    [ADR-0002](0002-vulkan-require-version-1-3.md).
  - Synchronization and lifetime mistakes are caught in CI, where they are cheapest to diagnose.
  - The engine has an offscreen render path, because tests render without a window.
- **Costs**:
  - CI compiles the tree, so it is much slower than a lint-only workflow.
  - A green run is weaker evidence than it looks. Software rasterization exercises the API, not a
    driver, and driver-specific bugs still reach a real GPU first.
  - Render tests must stay small enough for a software rasterizer, so performance and load tests
    stay local.
  - Mesa's Windows build comes from a third-party release, not a package manager. The workflow
    pins an exact version, which has to be raised by hand.
- **Revisit when**: GitHub's free tier for public repositories changes, or lavapipe stops
  supporting what [ADR-0002](0002-vulkan-require-version-1-3.md) requires. SwiftShader is the
  other software implementation to try.
