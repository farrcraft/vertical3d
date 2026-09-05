# ADR-0022: Offline Rendering Layout — Shared Code Lives In `api/render/offline`, And Each Renderer Is A Library With A Driver

**Date**: 2026-09-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`moya` and `talyn` are the two offline renderers, and they already duplicate: a `FrameBuffer`
of float planes with the same design and the same doc comment, and a `RenderContext` with the
same name. Phase 1 of [the offline rendering roadmap](../roadmap/OfflineRendering.md) needs a
framebuffer-to-`image::Image` conversion in both, which would be the third copy of something.

The layouts differ too. moya is `libmoya` + a driver + a suite of 39 ctest cases; talyn is one
executable, with no library, no `tests/` directory and no ctest entry — and it cannot get tests
without a library to link. The roadmap's verification argument turns on this: these are the only
renderers in this tree CI can run, so talyn's half of that is unreachable as the tree stands.

`api/render` builds one target, `v3dlib_render`, and every source in it is under `realtime/`.
Neither offline renderer may acquire a realtime dependency — that is what
[the modernization plan](../plans/completed/Modernization.md) settled and what keeps `api/type`
and `api/image` free of Vulkan.

## Decision

Code shared by the two offline renderers lives in `api/render/offline/`, built as its own target
alongside `v3dlib_render` rather than folded into it, and linking neither Vulkan nor SDL. `talyn`
splits into a library, a driver that links it, and a `tests/` directory, which is the layout
`moya` already has.

## Alternatives Considered

### Alternative 1: A second library in `api/render`, and talyn mirrors moya — **chosen**
- **Pros**: One home for rendering code, with the realtime/offline split visible in the path.
  The shared framebuffer, its image conversion and the RIB reader of
  [ADR-0023](0023-rib-is-the-offline-scene-description.md) are written once. talyn becomes
  testable, which is the whole of its half of the roadmap's verification argument.
- **Cons**: Two libraries in one `api/` subdirectory, where every other one builds exactly one.
  `v3dlib_render` keeps a name that now covers only the realtime half.
- **Why not**: n/a — chosen.

### Alternative 2: One `v3dlib_render` containing both halves
- **Pros**: The naming convention holds — one library per `api/` subdirectory, no new target.
- **Cons**: Every offline consumer would link Vulkan and SDL to get a float framebuffer, and the
  dependency that the whole arrangement exists to prevent would be one `target_link_libraries`
  line away from being real. CI runs the offline suites on a runner with no GPU.
- **Why not**: It destroys the property being protected.

### Alternative 3: `v3dlib_moya` is the shared library and talyn depends on it
- **Pros**: No new target, and moya's code is the more finished of the two.
- **Cons**: talyn would link the entire reyes pipeline and the RI entry points to get a
  framebuffer. It also fixes a dependency direction — the raytracer is part of the reyes
  renderer — that phase 6 of the roadmap has deliberately not decided.
- **Why not**: It answers phase 6 as a side effect of a file-organisation choice.

### Alternative 4: A new top-level `api/offline`, or talyn.cxx's proposed `libv3drender`
- **Pros**: Nothing in `api/render` changes, and the name is free of the realtime association.
- **Cons**: Two top-level homes for rendering code, and a reader looking for the offline
  renderer's shared code would look in `api/render` first. `libv3drender` is the older of the
  two names in the tree and predates `api/` entirely.
- **Why not**: The distinction is realtime versus offline, and a subdirectory of `api/render`
  says that where a sibling directory does not.

## Consequences

### Positive
- talyn gets a ctest entry, which is what the roadmap's phase 1 verification depends on: a
  rendered image compared against a committed reference, on every push, with no GPU.
- The framebuffer-to-`image::Image` conversion, the reference-image comparison and the RIB
  reader each land once rather than twice.
- The api boundary is enforced by a link line rather than by a rule: the offline library takes
  `v3dlib_type`, `v3dlib_image` and `v3dlib_log`, and anything realtime that leaks into those
  will fail to build for it.

### Negative
- `api/render` no longer builds one library, so `v3dlib_render` names the realtime half while
  its sibling names its half explicitly. The asymmetry is deliberate — renaming the realtime
  target touches ten `CMakeLists.txt` files and every app for no behavioural gain — but a
  reader has to learn it.
- talyn's split is churn against a renderer that has never produced a picture. It buys tests
  and nothing else until phase 1 lands.
- The two `RenderContext` classes are not obviously the same thing. A reyes context holds
  buckets and a raster space; a raytracer's holds a scene. Nothing here says they merge.

### Risks
- The offline library becomes the place code goes when it is unclear where it belongs. The rule
  is a second consumer, or a demonstrated one — not a plausible future one.
- A realtime dependency arrives transitively through a library that is fine today. The escape
  hatch is the offline suites in CI, which run on a machine with no GPU and no display: a
  dependency that matters fails there before it fails anywhere else.
