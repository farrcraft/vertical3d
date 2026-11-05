# ADR-0172: Build: each build directory is a CMake preset

**Status**: accepted
**Date**: 2026-11-04
**Tags**: build, tooling
**Documented in**: [Build.md](../../contributing/Build.md#configure-and-build)

## Context

The tree is configured in several ways. Visual Studio reads its own settings file, a shell
configures with a cmake command copied from the documents, and the analysis builds are set up by
hand with options nothing in the tree records. Each of these states the generator, the vcpkg
toolchain and the triplet again, and the copies drift. A build directory whose options are not
written down cannot be recreated exactly once it is deleted.

## Decision

A `CMakePresets.json` at the root names each Windows build directory and the options it is
configured with: the everyday Debug build and the three analysis builds. Visual Studio, VS Code
and a shell all configure from it, and the documented command is `cmake --preset <name>`. It
replaces `CMakeSettings.json`.

## Alternatives

### Keep CMakeSettings.json and the documented cmake command
- **For**: nothing changes, and Visual Studio already opens the folder.
- **Against**: only Visual Studio reads the file, so the shell and VS Code each have their own
  definition, and the analysis builds have none.
- **Rejected because**: every definition is a copy of the others, and the copies have already
  drifted.

### A configure script that takes a build name
- **For**: a short shell command, written in the same style as `scripts\build.cmd`.
- **Against**: no IDE reads it, so Visual Studio still needs its own file. It also rebuilds what
  presets already do.
- **Rejected because**: it adds another definition when the aim is to have only one.

### A task runner such as Make or just, wrapping cmake
- **For**: one entry point for configuring, building, testing and the gates.
- **Against**: neither tool comes with Windows or MSVC, and Make's recipes run in a POSIX shell.
  No IDE configures from it.
- **Rejected because**: it adds a dependency and still leaves the IDEs with their own
  definitions.

## Consequences

- **Gains**: every build directory can be recreated from the tree by its name. Visual Studio,
  VS Code and a shell configure it with the same options.
- **Costs**: Visual Studio ignores `CMakeSettings.json` once a presets file exists, so moving to
  presets is all or nothing. The presets are Windows only, so the portable Linux job keeps its
  own configure in its script. CI also keeps its own configure command, because its vcpkg root
  and its link pool differ from a developer's.
- **Revisit when**: CI or the portable job can configure from a preset without overriding most
  of what it sets.
