# ADR-0053: Allocation — A Consumer Chooses Whether Memory Is Suballocated, And One Allocation Per Resource Stays The Default

**Date**: 2026-09-12
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Every buffer and image in the renderer used to call `vkAllocateMemory` for itself — five sites
and eight frees, sharing only a helper that picked a memory type. Its docblock stated the
assumption it rested on: *"there are two kinds of allocation, and each is made once at load
time"*.

That is true of the nine apps here. Their vertex buffers, textures and depth images are built
when a scene loads and freed at shutdown, so one device allocation per resource costs nothing
and is the simplest thing that works.

It is not true of an application with per-frame resources. `maxMemoryAllocationCount` is a real
device limit rather than a theoretical one — 4096 on much hardware — and an application with a
uniform buffer and an overlay vertex buffer per frame in flight reaches it by allocating rather
than by running out of memory. An external consumer ([ADR-0027](0027-the-api-is-consumed-as-source.md))
has exactly that shape and carries its own allocator beside this one, which is a duplicate of
the part of a device tier least worth duplicating.

## Decision

`memory::Allocator` is the one place an allocation is made, and it does it one of two ways.
`Kind::Direct` is one device allocation per resource and is the default, unchanged from what
every site did before. `Kind::Suballocated` hands out regions of larger blocks through the
Vulkan Memory Allocator, and a consumer names it when it constructs its `Device`.

A resource holds an `Allocation` rather than a `VkDeviceMemory`, because a region is identified
by more than the block it sits in: a suballocated allocation starts part way into its block and
several share one, so mapping and freeing go through the allocator that made it rather than
through the device.

## Alternatives Considered

### Alternative 1: Both kinds behind one seam, direct as the default — **chosen**
- **Pros**: nothing in this tree changes shape, and the apps that were right to allocate
  directly still do. The decision moves to the layer that knows its own allocation pattern.
  Third time the tree has answered a consumer this way — [ADR-0049](0049-a-consumer-chooses-the-swapchain-format.md)
  for the swapchain format and [ADR-0052](0052-a-consumer-names-the-camera-hand.md) for the
  camera basis.
- **Cons**: two paths to keep correct, and the tree's own apps exercise only one of them.
- **Why not**: n/a — chosen. The second path is not left to rot: a `render_device` case draws
  a frame through it and compares the picture, which is what the suite that draws is for.

### Alternative 2: Suballocate always, and delete the direct path
- **Pros**: one path, no branch, and the allocator a large application would want anyway.
- **Cons**: every buffer, image and texture in nine apps changes how it is allocated, to fix a
  problem none of them has. It also makes the library a hard dependency of the renderer rather
  than of the consumers that want it.
- **Why not**: the blast radius is the whole renderer and the beneficiary is not in this tree.

### Alternative 3: Leave the five sites alone and let a consumer bring its own allocator
- **Pros**: nothing to build, which is the status quo.
- **Cons**: it is the status quo that produced a duplicated GPU tier in a consumer. There was
  no seam to bring an allocator to — the sites called `vkAllocateMemory` inline, so replacing
  the allocator meant replacing the resources.
- **Why not**: the api being unusable by its second consumer is a fault in the api, which
  ADR-0049 already recorded.

## Consequences

### Positive
- An application with per-frame resources can use `memory::Buffer`, `TextureFactory`,
  `RenderTarget` and `DepthBuffer` rather than reimplementing them around its own allocator.
- There is now one place an allocation is made, so a future change to how memory is found is
  one file rather than five.

### Negative
- `Allocation` is four fields where a `VkDeviceMemory` used to be, and it crosses into
  `pipeline::Texture`, so the allocator is named by a header that most of the renderer reaches.
  `Allocator.h` declares the two library handles rather than including `vk_mem_alloc.h` to keep
  that header out of every translation unit.
- The library is a dependency of `v3dlib_render` whether or not a consumer asks for
  suballocation, and a consumer's own root manifest has to declare it too — vcpkg manifest mode
  reads only the top-level project's.

### Risks
- An allocator holding blocks of its own must be destroyed before the device it allocated from,
  which a member destroyed after `~Device`'s body is not. This was found by the case that runs
  the suballocated path, and is why `~Device` releases it explicitly rather than leaving it to
  member destruction order.
