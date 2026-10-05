# ADR-0027: Build: consume the api as source

**Status**: amended
**Date**: 2026-09-05
**Amended by**: [ADR-0048](0048-includes-name-headers-from-the-repository-root.md)
**Documented in**: [UsingTheApi.md](../api/UsingTheApi.md)

## Context

Applications in other repositories need the `api/` libraries. The dependencies are pinned three
ways: static boost, an MSVC-only build, and a vcpkg baseline commit in
`vcpkg-configuration.json`. A prebuilt binary would leave matching all three exactly to the
consumer. The api also still changes often.

## Decision

The api is consumed as source. Another repository takes this one as a submodule or through
`FetchContent`, adds it with `add_subdirectory`, and builds it with its own compiler and flags.
Each api library carries its own include root at the repository root and a `v3d::` alias target,
and nothing is installed, exported or packaged.

## Alternatives

### An installed CMake package, found with `find_package(vertical3d)`
- **For**: The consumer builds only its own code. It gets a real version number, and the api's
  surface is stated as install rules.
- **Against**: It needs everything source consumption needs, plus install rules, a generated
  config that finds every dependency, and both configurations installed. A debug consumer
  against a release install is a CRT mismatch. The consumer must still match the vcpkg baseline
  exactly, so the hardest constraint is only hidden until link time.
- **Rejected because**: It is more work and does not fix the hardest failure. It becomes right
  when the api stops changing, and this decision is the first half of it.

### A port in a private vcpkg registry
- **For**: The baseline and the triplet are shared by construction rather than by care. The
  consumer's `vcpkg.json` names `vertical3d` and nothing else.
- **Against**: It is the installed package plus a portfile, a registry repository and a versions
  database.
- **Rejected because**: A port must install, so it cannot come before an installed package.

### The application joins this monorepo
- **For**: No build work at all. Every directory in the root already builds.
- **Against**: It does not work where the split is the point: a different release cadence, a
  different licence, or a collaborator who should not have the editor sources.
- **Rejected because**: It remains the right answer when a second repository is only tidiness,
  but it cannot serve a real split.

## Consequences

- **Gains**:
  - There is no ABI to match. The consumer builds the api with one compiler, one CRT, one boost
    and one set of flags.
  - An api change and the code using it compile together, so a break is a compile error rather
    than a version bump.
  - The api boundary is stated in the build: an include root per library, and third-party
    linkage carried by the library that needs it. An installed package would need both.
- **Costs**:
  - Every clean consumer build compiles the whole api, the engine and the shaders.
  - There is no version number. A submodule commit is the version.
  - The consumer inherits this project's dependency resolution. Its own `find_package(Boost)`
    finds the `Boost_DIR` this tree left in the cache.
  - The consumer must copy the vcpkg baseline exactly, and nothing checks that it did. A
    different baseline means a different boost, and a boost library's file name includes its
    version.
  - Only the CI build of `examples/starter` exercises an out-of-tree configure. Anything it does
    not cover can break without notice here.
- **Revisit when**: a consumer needs a binary drop, such as a collaborator without the sources
  or a licence split. Adding `install()` rules and a package config builds on this decision and
  would supersede it.
