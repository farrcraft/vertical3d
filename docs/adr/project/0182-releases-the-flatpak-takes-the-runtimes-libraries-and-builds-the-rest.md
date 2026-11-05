# ADR-0182: Releases: the Flatpak takes the runtime's libraries and builds the rest

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases, build
**Documented in**: [Releasing.md](../../contributing/Releasing.md#what-a-release-is)

## Context

Flathub builds an app from source with no network, and every source the build uses has to be
listed in the manifest. The Freedesktop runtime the Flatpak runs on carries SDL3, ICU, FreeType,
HarfBuzz, libpng, libjpeg, SQLite, zlib and the Vulkan loader, at its own versions. The tree's
other dependencies are not in it. Every other build takes its dependencies from vcpkg, which
downloads its sources as it builds.

## Decision

The Flatpak uses the runtime's libraries where the runtime has them. The manifest builds each
other dependency as a module of its own, pinned to a source archive and its hash. CI builds the
Flatpak on every pull request, so the versions it links are tested.

## Alternatives

### vcpkg inside the Flatpak, from a generated list of its sources
- **For**: one dependency list for every platform, at the versions every other build tests,
  with the overlay ports' patches.
- **Against**: a generator has to list every archive vcpkg downloads, and be rerun whenever the
  baseline moves. Every library is bundled, including those the runtime has, so the app is
  bigger and misses the runtime's security fixes. No Flathub app builds this way.
- **Rejected because**: Flathub's large C++ apps that build from source, Krita and FreeCAD among
  them, build their dependencies as modules on the runtime, and a first of its kind risks review.

### The Linux release, repackaged
- **For**: Blender's Flathub package repackages Blender's own prebuilt release, so the Flatpak
  holds exactly what Blender ships elsewhere.
- **Against**: Flathub allows a prebuilt binary only from a well-known vendor, case by case, and
  this project is not one.
- **Rejected because**: Flathub would not accept it.

## Consequences

- **Gains**: the app is built as Flathub's other apps are, so review follows a known path. It is
  smaller, and it gets the runtime's security fixes without a release.
- **Costs**: a second list of dependencies, beside `vcpkg.json`, kept in step by hand. The
  Flatpak links other versions than the other builds, so CI has to build it on every pull
  request. The tree's CMake finds some packages by the names only vcpkg gives them, and needs
  fallbacks for the runtime's. The overlay ports' changes have to be carried into the modules.
- **Revisit when**: Flathub accepts an app built through vcpkg, or the runtime drops a library
  the tree relies on.
