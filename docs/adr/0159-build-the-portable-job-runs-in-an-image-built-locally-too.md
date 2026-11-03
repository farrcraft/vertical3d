# ADR-0159: Build: the portable job runs in an image built locally too

**Status**: accepted
**Date**: 2026-11-03
**Amends**: [ADR-0107](0107-build-the-api-libraries-build-with-clang-off-windows.md)
**Documented in**: [contributing/Build.md](../contributing/Build.md#building-off-msvc)

## Context

The Clang build on Linux fails on code that passes every gate on Windows, and the tree is
developed on Windows. A failure there is found only after a push, and a cold run takes most of
an hour. Reproducing it needs the same distribution, compiler, standard library and configure
as CI. Steps written once in a workflow and again in a document drift apart: the document
named an older Ubuntu than the job and missed a library from its list.

## Decision

A Dockerfile in the tree defines the image the portable job builds in, and one script run inside
it holds the configure, the build and the tests. CI builds that image and runs that script, and
a local command does the same with the tree mounted read-only.

## Alternatives

### Install the tools on the runner, and document how to reproduce it
- **For**: no image to build on each run, and the runner's preinstalled tools.
- **Against**: the workflow and the document each state the packages, the compiler and the
  configure, and nothing checks that they agree.
- **Rejected because**: they had already disagreed.

### A container for local runs, with the workflow keeping its own steps
- **For**: CI is unchanged, and a local run is still possible.
- **Against**: two definitions of the same build, one of which only CI exercises.
- **Rejected because**: a local run that differs from CI can pass where CI fails.

### A prebuilt image in a registry, used as the job's container
- **For**: the job skips the image build.
- **Against**: a second workflow to publish the image, a tag to keep in step with the
  Dockerfile, and a registry to trust.
- **Rejected because**: the image builds in about a minute, which does not pay for the publishing.

### CMake presets for the configure
- **For**: CMake's own way of naming a configure, readable by any IDE.
- **Against**: Visual Studio uses a CMakePresets.json in place of CMakeSettings.json once one
  exists, and presets do not install a compiler.
- **Rejected because**: it changes the Windows workflow and still leaves the toolchain unstated.

## Consequences

- **Gains**: the job's environment is a file in the tree, and a local run builds in the same
  image with the same script. A Linux failure can be reproduced before a push.
- **Costs**: each CI run builds the image. The image is plain Ubuntu, not the runner's image, so
  it holds only what the Dockerfile installs. A local run needs Docker Desktop, and its first
  run builds every dependency from source.
- **Revisit when**: the job needs a device or a window, which a container on a hosted runner
  cannot give it, or the image build grows long enough that publishing it pays.
