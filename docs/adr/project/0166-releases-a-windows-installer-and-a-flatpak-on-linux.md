# ADR-0166: Releases: a Windows installer and a Flatpak on Linux

**Status**: accepted
**Date**: 2026-11-03
**Tags**: releases
**Documented in**: [Releases.md](../../roadmap/Releases.md), until a release process document states the rule

## Context

The apps build on Windows with MSVC. The libraries that need neither Vulkan nor SDL also build
with Clang on Linux. Each package format adds a build to keep working, and each platform adds a
port. Rendering is Vulkan 1.3. On macOS that means MoltenVK, which offers no geometry or
tessellation shaders. An app there also needs a paid Developer ID and notarization to run
without a warning.

## Decision

Releases ship for Windows and for Linux, Windows first. Windows gets an installer, and Linux gets
a Flatpak published on Flathub. macOS is not a release platform.

## Alternatives

### deb and rpm packages
- **For**: they install with the distribution's own tools, and CPack builds both.
- **Against**: each distribution family expects its own conventions, and a package built on one
  release of a distribution is not tested on the others. The projects compared leave them to
  the distributions and ship one format for all.
- **Rejected because**: a Flatpak runs the same on every distribution, with the Vulkan driver of
  the host.

### AppImage or a tarball beside the Flatpak
- **For**: no sandbox, and no runtime to install first.
- **Against**: neither updates itself, and each is one more build to test.
- **Rejected because**: one Linux format is enough to start.

### A portable zip beside the Windows installer
- **For**: runs without installing, which most of the projects compared offer.
- **Against**: the apps would have to work both from a read-only install and from any folder, and
  it is one more artifact to test.
- **Rejected because**: the installer covers what users need, and a zip can be added from the
  same install tree.

### macOS through MoltenVK
- **For**: a third platform, with the Vulkan 1.3 API.
- **Against**: the renderer's use of the missing shader stages is unchecked, and a release needs
  an Apple Silicon runner, a paid Developer ID and notarization.
- **Rejected because**: two platforms come first.

## Consequences

- **Gains**: two release platforms, each with one format to build and test. Flathub handles
  updates, and the installer gives Windows users Start menu entries and an uninstaller.
- **Costs**: the apps must be ported to Linux. Flathub builds from the sources the manifest lists,
  with no network during the build, so vcpkg's downloads have to be declared in the manifest.
  The Windows installer needs a signature, or users meet a SmartScreen warning.
- **Revisit when**: users ask for a format or a platform this leaves out.
