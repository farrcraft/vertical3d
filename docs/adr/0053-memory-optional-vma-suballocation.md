# ADR-0053: Memory: optional VMA suballocation

**Status**: accepted
**Date**: 2026-09-12
**Documented in**: [internals/realtime/Memory.md](../internals/realtime/Memory.md)

## Context

One device allocation per buffer or image is the simplest scheme, and it suits apps that build
their resources when a scene loads and free them at shutdown. A device limits the number of
live allocations (`maxMemoryAllocationCount`, often 4096), not only their total size. An app
with per-frame resources, such as a uniform buffer and a streamed vertex buffer per frame in
flight, reaches that limit by count long before it runs out of memory. If every resource calls
`vkAllocateMemory` itself, an app cannot change how memory is allocated without replacing the
resources.

## Decision

`memory::Allocator` is the one place an allocation is made, in one of two kinds chosen when the
`Device` is constructed: direct, one device allocation per resource and the default, or
suballocated, regions of larger blocks handed out by the Vulkan Memory Allocator (VMA). A
resource holds an `Allocation` rather than a `VkDeviceMemory`, because a suballocated region
starts partway into a shared block and is mapped and freed through the allocator that made it.

## Alternatives

### Suballocate always and remove the direct path
- **For**: One path with no branch, and the allocator a large app wants anyway.
- **Against**: Every buffer, image and texture in every app changes how it is allocated, to fix
  a problem none of them has.
- **Rejected because**: It changes the whole renderer for a benefit only some apps need, and
  keeping direct allocation as the default leaves every existing app unchanged.

## Consequences

- **Gains**:
  - An app with per-frame resources can use `memory::Buffer`, `TextureFactory`,
    `RenderTarget` and `DepthBuffer` rather than reimplementing them around its own allocator.
  - Allocation has one home, so a change to how memory is found is made in one file.
- **Costs**:
  - Two paths to keep correct, and the apps in this tree use only the direct one. A device test
    draws through the suballocated path to cover it.
  - `Allocation` reaches `pipeline::Texture` and so most of the renderer. `Allocator.h`
    declares the two VMA handles itself rather than including `vk_mem_alloc.h` everywhere.
  - VMA is a dependency of `v3dlib_render` whether or not an app suballocates, and an app's own
    vcpkg manifest has to declare it, because manifest mode reads only the top-level project's.
- **Revisit when**: the apps in this tree create resources per frame, at which point
  suballocation could become the default or the only path.
