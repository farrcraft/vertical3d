# ADR-0179: Releases: an NSIS installer built by CPack

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases, build
**Documented in**: [Releasing.md](../../contributing/Releasing.md#what-a-release-is)

## Context

A Windows user expects an installer that adds the apps to the Start menu and removes them again.
The install rules already say what a release holds and where each file goes. An installer tool
that keeps its own list of files has to be kept in step with them by hand. The release is not
signed until a signing service accepts the project.

## Decision

The Windows installer is built with NSIS, through CPack's NSIS generator, from the install rules.
It adds a Start menu entry for each app with a window, and an uninstaller. It registers no file
types with the apps.

## Alternatives

### Inno Setup
- **For**: Godot's Windows installer uses it, and it installs without administrator rights.
- **Against**: it reads a script of its own that lists what to install, which the install rules
  already state. CPack has no generator for it.
- **Rejected because**: a second list of the release's files drifts from the first.

### WiX, building an MSI
- **For**: an MSI suits managed deployment, and CPack has a WiX generator.
- **Against**: since version 6, use of WiX's releases that brings in revenue needs its Open
  Source Maintenance Fee. Its XML is the hardest of the tools to write and review.
- **Rejected because**: no user of the apps has asked for managed deployment, and the fee
  binds anyone who builds the installer.

### MSIX
- **For**: Windows' current package format, with clean installs and updates.
- **Against**: an MSIX installs only when signed by a certificate Windows is set to trust, and the
  apps run in a container that redirects their file writes.
- **Rejected because**: it could not ship before signing is settled, and the container changes
  where the apps write.

### Registering the editor's projects and RIB files
- **For**: double-clicking a file would open it in its app.
- **Against**: a project is a directory whose manifest is a `.json` file, which cannot be
  claimed for the editor. A double-clicked `.rib` would start moya with no output named.
- **Rejected because**: neither gives the user a useful result yet.

## Consequences

- **Gains**: the installer holds exactly what the install rules install, with no second list. The
  installer builds on the CI runner with CPack, and NSIS is under the zlib licence.
- **Costs**: NSIS has to be installed where the installer is built. The installer is an
  executable, not an MSI, so a managed deployment has to script it. Opening a file from Explorer
  does not start its app.
- **Revisit when**: users need managed deployment, or a file type gets an action worth
  registering.
