# ADR-0020: UI: themes are data, apps load the images

**Status**: accepted
**Date**: 2026-09-04
**Documented in**: [api/ui/Themes.md](../api/ui/Themes.md)

## Context

An app needs to change how its ui looks without editing or rebuilding `api/ui`. A look is
colours, metrics, fonts and images, and an image has to become a texture before it can be
drawn. `v3dlib_ui` depends on `v3dlib_render` for the canvas and on nothing else, so its tests
need no window, no device and no font. Text measuring and writing already reach the library as
callbacks the app supplies. Art is often packed into sprite sheets, so one image is not
necessarily one texture.

## Decision

A theme is JSON read by `ui::Engine::load`, and the renderer takes from it only the values it
names, keeping its defaults for the rest. The library never loads or uploads an image:
`Engine::resolveImages()` passes each source name to a callback the app supplies, and keeps the
`ui::Image` that comes back, which is a texture handle and the region of that texture the image
occupies. What a source name means is the app's business, so the library never reads a sprite
sheet.

## Alternatives

### `v3dlib_ui` links `v3dlib_asset` and loads its own images
- **For**: one call for an app instead of a callback.
- **Against**: turning an image into a texture needs the asset manager and the renderer, so the
  ui would take on the app's whole loading stack, and its tests would need a device.
- **Rejected because**: a callback keeps the library testable, and it is the same shape text
  already uses.

### Keep colours and metrics on the renderer, and theme nothing
- **For**: nothing to design; the values sit beside the code that draws with them and can be
  derived from the font size.
- **Against**: the style classes stay unused, and a second app wanting a different look has to
  edit the library.
- **Rejected because**: the look of an app belongs in its data.

### A theme replaces the whole style instead of overriding what it names
- **For**: what is drawn is exactly what the document says, with no hidden defaults.
- **Against**: every ui document would have to name every value before it drew correctly, and a
  theme could not carry a single colour.
- **Rejected because**: a theme that names nothing has to keep drawing what it drew.

### The library reads the sprite sheet itself
- **For**: an app answers a name with no lookup of its own.
- **Against**: `v3dlib_ui` would link the config library to do work the app is already set up
  to do.
- **Rejected because**: it adds a dependency to save one lookup in a callback.

### The app sets each component's texture region after the pass
- **For**: the callback stays a bare texture handle.
- **Against**: it is a second walk of the tree, and running the pass again loses the regions.
- **Rejected because**: the answer belongs in the one place images are resolved.

## Consequences

- **Gains**:
  - A ui is restyled from data without a rebuild, and a theme carrying one colour changes one
    thing.
  - `v3dlib_ui` gains no dependency, so themes, images and drawing are tested with no window
    and no device.
  - One texture can serve every icon on a screen.
- **Costs**:
  - The same value can be set in code on the renderer's base dressing and in the document, and
    the document wins where it speaks.
  - A style property is found by two strings, its name and its class. A typo in either is a
    property silently not found.
  - An app that never calls `resolveImages()` draws skinned buttons flat and icons not at all,
    with no error. The pass logs each source it could not resolve.
  - The theme schema is a compatibility surface for every app's ui document. Adding a property
    is safe; renaming one is not.
- **Revisit when**: an app needs the ui to load images itself, or the library gains a reason to
  depend on the asset stack anyway.
