# ADR-0027: External Consumption — The api Is Taken As Source Through An `add_subdirectory`-able Root, Not As An Installed Package

**Date**: 2026-09-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

An application in another repository wants the `api/` libraries. Nothing in this tree is
consumable from outside it: there is no `install()` or `export()` anywhere, the only
`target_include_directories` is the one inside `v3d_add_shader` for generated shader headers, and
a consumer reaches a header by relative path — `pong/src/PongEngine.h` opens with
`#include "../../api/engine/Engine.h"`. The root `CMakeLists.txt` holds every `find_package`, the
`/std:c++latest` and `/permissive-` options, the four `v3d_add_*` helpers and an unconditional
`add_subdirectory` for all ten apps, so `api/` cannot be configured without the rest of the tree.

The dependency surface is pinned three ways — static boost, an MSVC-only build, and a vcpkg
baseline commit that lives in `vcpkg-configuration.json` rather than in `vcpkg.json` — and a
prebuilt binary makes matching all three exactly the consumer's problem. The api is also still
moving: [ADR-0022](0022-offline-rendering-shares-an-api-library.md) added a second library to
`api/render` this month.

## Decision

The api is consumed as source. The root becomes `add_subdirectory`-able — the apps and the ctest
registrations behind options, the dependencies and the helper functions in `cmake/` modules — and
each api library carries its own include root and a `v3d::` alias target, so another repository
takes this one as a submodule or through `FetchContent` and builds it with its own compiler and
its own flags. Nothing is installed, exported, packaged or turned into a port.

## Alternatives Considered

### Alternative 1: Source consumption through a nestable root — **chosen**
- **Pros**: There is no ABI surface, so the three pinned things stop being a matching problem: one
  compiler, one CRT, one boost, one set of flags, because the consumer builds them. An api change
  and the code that uses it compile together, so a break is a compile error in one command rather
  than a version-bump round trip against a renderer that changes every month. Most of the enabling
  work — per-target include directories, third-party linkage propagated by the libraries rather
  than named by each app, `/utf-8` in the interface of anything exposing spdlog — is what an
  installed package needs anyway.
- **Cons**: Every clean consumer build compiles the whole api. There is no version number; a
  submodule sha is the version.
- **Why not**: n/a — chosen.

### Alternative 2: An installed CMake package, found with `find_package(vertical3d)`
- **Pros**: The consumer builds its own code only. A real version, a real package config, and the
  api's surface stated as install rules rather than inferred from a directory layout.
- **Cons**: It needs everything Alternative 1 needs, plus install rules, a generated config with
  `find_dependency` for nine packages, and both configurations installed with generator
  expressions — because a debug consumer against a release install is a CRT mismatch, and the
  static boost makes it a link error at best. The consumer must still match the vcpkg baseline
  exactly, so the hardest constraint is not removed, only hidden until link time.
- **Why not**: It is strictly more work for a build whose hardest failure mode it does not fix. It
  becomes right when the api stops churning, and this decision is the first half of it.

### Alternative 3: A port in a private vcpkg registry
- **Pros**: The best endgame: the baseline and the triplet become shared by construction rather
  than by discipline, which is the one thing the other two options leave to the consumer to get
  right. The consumer's `vcpkg.json` names `vertical3d` and nothing else changes.
- **Cons**: It is Alternative 2 plus a portfile, a registry repository and a versions database.
  A port must install, so it cannot come first.
- **Why not**: Ordering. There is nothing to package until the api installs.

### Alternative 4: The application joins this monorepo
- **Pros**: Free. Every directory in the root already builds, which is the layout's whole premise,
  and none of the work above is needed.
- **Cons**: It is not an answer where the split is the point — a different release cadence, a
  different licence, or a collaborator who should not have the editor sources.
- **Why not**: It is the right answer when the second repository is only tidiness, and this
  decision does not displace it. It stops being available the moment the reason for the split is
  real, and nothing about the split is reversible later at a lower cost than now.

## Consequences

### Positive
- The consumer's configure is the second reader this CMake has ever had, which is the pressure
  that finds the root's globals. `v3d_add_shared_data` copies from `${CMAKE_SOURCE_DIR}/data`,
  which is the consumer's root in a nested build and not this repository's — a bug that cannot be
  observed while the only root is this one.
- The api boundary gets stated in the build rather than implied by the tree: an include root per
  library, and linkage propagated by the library that needs it. Both are corrections in their own
  right, and both are prerequisites of Alternatives 2 and 3.
- No release step stands between a change and the app that uses it.

### Negative
- The same header has two spellings: `../../api/image/Image.h` inside the tree and
  `<api/image/Image.h>` outside it. Rewriting every in-tree include to the second form is a
  whole-file diff across the tree for no build gain, so the asymmetry is kept and a reader has to
  learn it. **Reversed by [ADR-0048](0048-an-api-header-is-named-from-the-repository-root.md)**,
  which found a gain that is not a build gain: a `../` count is a fact about where both files
  sit, so the asymmetry prices every later directory move at this same diff.
- The root calls `find_package` for nine packages, and nested those resolve into the consumer's
  cache. A consumer inherits this project's dependency resolution whether or not it wants it: its
  own `find_package(Boost)` hits the `Boost_DIR` the tree left behind.
- The consumer must copy the vcpkg baseline verbatim, and nothing checks that it did. A different
  baseline is a different boost, and a boost library's file name carries its version.
- A clean build of the consumer builds the api, the engine and the shaders. There is no cached
  artefact to skip that.

### Risks
- The out-of-tree configure rots, because nothing in this repository exercises it — someone adds
  an `add_subdirectory` outside the option guard, or a library grows a header that only resolves
  through the root's global include path, and neither is visible here. The escape hatch is a CI
  step that configures a minimal consumer against the tree; without it this record describes a
  build that worked once.
- A binary drop turns out to be needed after all — a collaborator without the sources, or a
  licence split. The escape hatch is that this decision builds the first half of Alternative 2:
  adding `install()` rules and a package config is additive, and would supersede this record
  rather than unwind it.
