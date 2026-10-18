# ADR-0089: Paths: narrow strings are UTF-8, under a UTF-8 code page

**Status**: accepted
**Date**: 2026-10-17
**Documented in**: [docs/api/engine/Files.md](../api/engine/Files.md#paths)

## Context

Every path in the api is a narrow `std::string`, from `appPath()` through the logger, the asset
manager and each loader. Windows reads a narrow path in the process's ANSI code page, so a
character that page cannot hold is lost and the file cannot be found. SDL hands an app its
arguments and its directories in UTF-8, so a narrow string in the api is already UTF-8 in some
places and ANSI in others. The third-party libraries the loaders call, spdlog among them, open
files by narrow name with the C runtime.

## Decision

A narrow path is UTF-8 everywhere in the api. Every executable carries a manifest that sets the
process's code page to UTF-8, so Windows, the C runtime and Boost.Filesystem all read a narrow
path as UTF-8. `v3dlib_log` carries the manifest, so linking the logger embeds it.

## Alternatives

### Wide paths through every layer
- **For**: works under any code page, with no process-wide setting, and `boost::filesystem::path`
  says in the type that a string is a path
- **Against**: every function that takes a path changes signature, in each library and in every
  consumer. spdlog has to be rebuilt with wide file names. A third-party library that opens a file
  by narrow name still fails, unless each call is replaced with a read through a wide path
- **Rejected because**: it changes the whole api and still leaves the third-party opens broken

### Set a UTF-8 locale at startup
- **For**: one call in `run<T>`, and no build change
- **Against**: the locale reaches only the C runtime. Win32 calls, Boost.Filesystem's
  conversion and `argv` still use the ANSI code page, and a test or tool that does not go
  through `run<T>` never makes the call
- **Rejected because**: it fixes only part of the problem

## Consequences

- **Gains**: no signature changes, and the third-party libraries open UTF-8 paths with no change
  to them. A string from SDL and a string from the api are in the same encoding
- **Costs**: it needs Windows 10 version 1903 or later. An older Windows ignores the setting and
  keeps the old failure. An executable that does not link `v3dlib_log` has to add the manifest
  itself. Code loaded into a process the api does not build, such as a plugin, runs under that
  process's code page. A narrow string written to a console is shown in the console's own code
  page, which the manifest does not change
- **Revisit when**: the api has to run in a process whose manifest it does not control, or on a
  Windows older than 1903
