# ADR-0033: Build: select api libraries through a manifest

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [Build.md](../contributing/Build.md), [UsingTheApi.md](../api/UsingTheApi.md#selecting-libraries)

## Context

A consumer adds the api as source ([ADR-0027](0027-build-consume-the-api-as-source.md)). Without
a selection, it builds every library and must install every library's dependencies. An app that
only reads a PNG through `v3d::image` would still need the Vulkan SDK, SDL3 and the mixer. Inside
this repository every library is always built, so a library that links something it never
declared still works here, and only a narrow consumer finds out.

## Decision

A consumer names the api libraries it links in `V3D_LIBRARIES`, which defaults to `all`. A
manifest in `cmake/v3dApiLibraries.cmake` lists, for each library, the api libraries it requires
and the third-party packages it uses. The tree adds only the closure of the request and looks
only for its packages, and the configure fails if the manifest disagrees with the real link
lines.

## Alternatives

### Drop `REQUIRED` and skip what is not found
- **For**: A few lines of change. No manifest, no closure, and nothing to keep in step.
- **Against**: With a broken Vulkan install the consumer gets no `v3d::render` and therefore no
  `v3d::engine`. The error is an unknown target in the consumer's own CMakeLists, naming neither
  Vulkan nor the cause. A consumer that wants only `v3d::image` still builds whatever its machine
  can build.
- **Rejected because**: It answers "what can this machine build" when the question is "what does
  this application need", and every failure appears far from its cause.

### A `V3D_BUILD_<LIB>` option per library
- **For**: The usual CMake shape, and each option shows in `cmake-gui`.
- **Against**: One boolean per library, and the consumer has to work out the closure. Asking for
  `engine` without `render` is a configure error about an unknown target.
- **Rejected because**: It moves the closure to the consumer, who knows least about it.

### `find_package` inside each library's own CMakeLists, with no selection
- **For**: Each dependency is stated in one place, and there is no closure logic.
- **Against**: Every library is still added, so the consumer still needs the Vulkan SDK. An
  imported target created inside `api/<lib>/` is also not visible to an app directory at the
  root, so the apps would stop linking.
- **Rejected because**: It does not reduce what a consumer has to install.

### An installed package with a generated config
- **For**: The standard way to take part of a library, with one `find_dependency` per component.
- **Against**: It needs install rules and both configurations installed, and the consumer must
  still match the vcpkg baseline exactly.
- **Rejected because**: [ADR-0027](0027-build-consume-the-api-as-source.md) rejected it for the
  same reasons, and they still hold. The manifest is groundwork for it.

## Consequences

- **Gains**:
  - A consumer installs only the dependencies of what it links. A selection of `image` and `log`
    configures with no Vulkan, SDL3, Freetype or EnTT.
  - `v3d::render_offline` can be taken alone, with no graphics driver, SDK or window system.
  - The configure-time check turns the duplicated dependency list into a checked one.
  - `examples/starter` names a narrow selection, so CI catches a library that links something its
    manifest entry does not declare.
- **Costs**:
  - Every dependency edge is stated twice, in the library's `target_link_libraries` and in the
    manifest. A new package means editing two files.
  - `cgltf` adds an include directory, not a target, so the check cannot confirm its entry.
  - `engine` requires `render`, and so the Vulkan SDK, because `Engine.h` holds the concrete
    window. The loop's headless parts cannot be taken without it.
  - The manifest names packages, not versions or components, and the check does not see a
    library linked only through a generator expression.
- **Revisit when**: a headless consumer needs the engine loop without Vulkan, or the api is
  installed as a package and the manifest becomes its generated config.
