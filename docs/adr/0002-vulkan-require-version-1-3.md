# ADR-0002: Vulkan: require version 1.3

**Status**: accepted
**Date**: 2026-08-30
**Documented in**: [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

The Vulkan version an instance declares decides which core features the renderer can rely on.
Under 1.0, every pass needs `VkRenderPass` and `VkFramebuffer` objects. Under 1.3, dynamic
rendering removes both, and `synchronization2` simplifies barriers. The frame loop is built
on whichever answer is chosen, so changing it later means rewriting the loop.

## Decision

The instance declares Vulkan 1.3, and a physical device that reports less than 1.3 is not
selected.

## Alternatives

### Stay at 1.0 and enable extensions where needed
- **For**: the widest driver support, and no version check in device selection.
- **Against**: dynamic rendering and `synchronization2` each need their extension form, with
  availability checks and a fallback path for drivers that lack them. That is the same work as
  a version requirement, plus the branches.
- **Rejected because**: it adds complexity for compatibility this project does not need.

### Require 1.2
- **For**: brings descriptor indexing, which bindless texturing needs. Slightly broader driver
  support than 1.3, and much broader under MoltenVK on macOS.
- **Against**: dynamic rendering is not core in 1.2.
- **Rejected because**: dynamic rendering is the feature that motivates the requirement.
  MoltenVK coverage does not matter to a Windows-only project.

## Consequences

- **Gains**:
  - The renderer never builds render pass or framebuffer objects.
  - Barriers use the `synchronization2` forms, which are easier to get right.
  - Descriptor indexing is core, so bindless texturing is a question of complexity rather than
    capability.
- **Costs**:
  - Device features are enabled through a `VkPhysicalDeviceFeatures2` `pNext` chain, after
    querying support, rather than through the plain features struct.
  - A machine whose driver reports less than 1.3 fails at device selection, with a message
    naming the version needed, rather than running in a reduced mode.
  - Support follows the driver more than the hardware, so a recent GPU on an old OEM driver can
    still fail.
- **Revisit when**: a target machine that has to be supported cannot get a 1.3 driver. The
  driver coverage behind this choice was estimated, not measured; vulkan.gpuinfo.org is where
  to check it.
