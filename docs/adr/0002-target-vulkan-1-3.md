# ADR-0002: Vulkan Version — Target 1.3 Rather Than 1.0

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The instance created by `vulkan::Instance` declared `VK_API_VERSION_1_0`, which was the
default carried over from the first sketch rather than a considered choice. The frame loop
is about to be written, and the version determines whether it has to build `VkRenderPass`
and `VkFramebuffer` objects or can skip them. Deciding after the frame loop exists would
mean building those objects and then deleting them.

## Decision

The instance declares `VK_API_VERSION_1_3`, and physical devices reporting less than 1.3 are
not selected.

## Alternatives Considered

### Alternative 1: Target 1.3 — **chosen**
- **Pros**: Dynamic rendering removes `VkRenderPass` and `VkFramebuffer` entirely.
  `synchronization2` makes barriers substantially less painful. Descriptor indexing, core
  since 1.2, comes along for free.
- **Cons**: Requires a driver reporting 1.3, and the feature structs have to be enabled
  through a `pNext` chain rather than the plain `VkPhysicalDeviceFeatures` the code uses
  today.
- **Why not**: n/a — chosen.

### Alternative 2: Stay at 1.0 and enable extensions where needed
- **Pros**: Widest possible driver support. No version check needed in device selection.
- **Cons**: Dynamic rendering and synchronisation2 would each need their extension form,
  with availability branches and a fallback path for drivers that lack them — the same
  machinery as a version bump, plus the branches.
- **Why not**: More complexity for compatibility this project does not need.

### Alternative 3: Target 1.2
- **Pros**: Gets descriptor indexing, which is what bindless texturing would need. Slightly
  broader support than 1.3, and materially broader under MoltenVK.
- **Why not**: Misses dynamic rendering, which is the feature actually motivating the bump.
  MoltenVK's coverage is irrelevant to a Windows-only project.

## Consequences

### Positive
- Phase 2 never builds render pass or framebuffer objects at all.
- Barriers use the `synchronization2` forms, which are easier to get right.
- Bindless texturing becomes a question of complexity rather than capability.

### Negative
- Device creation must move from `pEnabledFeatures` to `VkPhysicalDeviceFeatures2` with a
  `pNext` chain, and must query feature support before enabling anything.
- Physical device selection gains a version check, and a machine whose driver reports less
  than 1.3 gets a hard failure rather than a degraded path.

### Risks
- Support tracks driver version more than silicon age, so a recent GPU on a stale OEM driver
  can still fail. The failure is explicit — device selection reports the version it needed —
  rather than a confusing crash later.
- The compatibility figures behind this decision were characterised, not measured.
  vulkan.gpuinfo.org is the source to check if the assumption is ever questioned.
