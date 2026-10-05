# ADR-0022: Offline: shared library with no Vulkan

**Status**: amended
**Date**: 2026-09-04
**Amended by**: [ADR-0078](0078-offline-moya-is-the-one-renderer-ray-tracing-is-a-hider.md)
**Documented in**: [OfflineRenderer.md](../OfflineRenderer.md)

## Context

The offline renderer needs code that is not specific to one renderer: a float framebuffer, its
conversion to `image::Image`, the RIB reader and the code that compares an image against a
reference. `api/render` builds one target, `v3dlib_render`, and all of it is realtime Vulkan
code. Offline rendering must not acquire a realtime dependency, because its test suites run in
CI on machines with no GPU and no display. `api/type` and `api/image` are kept free of Vulkan
for the same reason.

## Decision

Offline rendering code that is not specific to one renderer lives in `api/render/offline`. It
builds as its own target, `v3dlib_render_offline`, beside `v3dlib_render` rather than inside
it, and links neither Vulkan nor SDL.

## Alternatives

### One `v3dlib_render` containing both halves
- **For**: Every `api/` subdirectory keeps building exactly one library, and there is no new
  target.
- **Against**: Every offline consumer would link Vulkan and SDL to get a float framebuffer. The
  dependency the split exists to prevent would be one `target_link_libraries` line away.
- **Rejected because**: It removes the property being protected.

### The renderer's own library is the shared library
- **For**: No new target, and the renderer's code is the most finished code to start from.
- **Against**: Anything that wants a framebuffer or the RIB reader would link the whole reyes
  pipeline and the RI C entry points. The dependency would point from shared code up into one
  renderer.
- **Rejected because**: Shared code belongs below the renderers, not inside one of them.

### A new top-level `api/offline`
- **For**: Nothing in `api/render` changes, and the name carries no realtime association.
- **Against**: Rendering code would have two top-level homes, and a reader looking for offline
  rendering code looks in `api/render` first.
- **Rejected because**: The distinction is realtime against offline, and a subdirectory of
  `api/render` states that where a sibling directory does not.

## Consequences

- **Gains**:
  - The framebuffer, its image conversion, the reference comparison and the RIB reader are
    written once.
  - The boundary is enforced by the link line. The offline library takes `v3dlib_log`,
    `v3dlib_image`, `v3dlib_type` and glm, so a realtime dependency arriving through any of them
    fails its build.
- **Costs**:
  - `api/render` builds two libraries where every other `api/` subdirectory builds one.
    `v3dlib_render` names only the realtime half, and a reader has to learn that.
  - The library can become the place code goes when its home is unclear. The rule for adding to
    it is a demonstrated use outside the renderer's own state, not a plausible future one.
- **Revisit when**: a realtime dependency reaches the offline library transitively, or the
  realtime target is renamed for other reasons and the two names can be made symmetric.
