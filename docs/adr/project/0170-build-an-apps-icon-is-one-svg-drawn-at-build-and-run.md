# ADR-0170: Build: an app's icon is one SVG, drawn at build and at run

**Status**: accepted
**Date**: 2026-11-04
**Tags**: build
**Documented in**: [Build.md](../../contributing/Build.md#app-icons)

## Context

Every app needs an icon in several places. A window shows one in its title bar and the
taskbar, a Windows executable embeds one for Explorer and shortcuts, and a Flatpak installs
one with its desktop entry. Each place takes a different form: pixels for the window, an
`.ico` of several sizes for the executable, and a scalable SVG or a set of PNGs for Linux.
Copies kept by hand in each form drift apart as the art changes.

## Decision

Each app's icon is one SVG in the tree. The build draws it into an `.ico` and embeds that in
the executable. The engine draws the same SVG into the window's icon when it opens the window,
from the name the window config gives.

## Alternatives

### Commit an .ico and a PNG set beside the SVG
- **For**: no build step, and the files load with no rasteriser. Blender and Krita keep icons
  this way.
- **Against**: every change to the art means regenerating each form by hand, with a tool
  outside the tree, and a forgotten form ships the old icon.
- **Rejected because**: the tree already draws SVG, so the derived forms cost nothing to make
  and cannot go stale.

### Embed only the .ico, and have the window load it from the executable
- **For**: one runtime path, through SDL's hint for a Windows icon resource.
- **Against**: it works only on Windows. A Linux build would need a second path, and an app
  without the resource shows nothing.
- **Rejected because**: the window's icon belongs to the engine on every platform, and the
  executable's to the platform's packaging.

## Consequences

- **Gains**: one file to change per app. The Flatpak can install the same SVG as its scalable
  icon. A new app gets an icon with one CMake call and one line in its window config.
- **Costs**: each build that touches an icon runs a tool built in the tree, and each windowed
  app draws its icon at startup. An icon is limited to the SVG NanoSVG draws: no text, filters
  or embedded images.
- **Revisit when**: an icon needs art the SVG subset cannot draw, or a platform needs a form
  that cannot be drawn from the SVG.
