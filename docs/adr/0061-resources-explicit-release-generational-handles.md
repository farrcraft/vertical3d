# ADR-0061: Resources: explicit release, generational handles

**Status**: accepted
**Date**: 2026-10-03
**Amends**: [ADR-0010](0010-meshes-owned-by-the-app-that-built-them.md)
**Documented in**: [internals/realtime/Memory.md](../internals/realtime/Memory.md)

## Context

`pipeline::Resources` registers textures, materials and pipelines and never removes them, and
its slots are never reused. That works while everything loads at startup, and leaks as soon as
an app replaces a texture or loads and unloads content during play. Two questions follow: who
decides a resource is dead, and when its Vulkan objects may be destroyed, given that frames
recorded earlier may still be reading them. A handle is a value copied into every `DrawItem`,
and its slot is packed into the `SortKey`
([ADR-0004](0004-rendering-submit-draw-items-as-data.md)).

## Decision

A resource is released explicitly, by handle. A handle carries a slot and a 32-bit generation:
a released slot is reused with its generation incremented, and a handle whose generation is not
the slot's current one resolves to nothing. The Vulkan objects behind it are retired to the
in-flight ring ([ADR-0051](0051-frames-in-flight-ring-separate-from-presenting.md)) as a
callback, and destroyed once every frame that began before the release has finished.

## Alternatives

### A reference count held by whatever keeps a handle
- **For**: Nothing can be released while something names it, and nobody has to remember to
  release.
- **Against**: Every copy of a handle into a `DrawItem` becomes a counted copy, and a count on a
  slot packed into a sort key means nothing.
- **Rejected because**: It costs the draw path for a convenience only loading and unloading
  need.

### A scope: everything registered for a scene is released with it
- **For**: Matches what loading by region needs: unload the region and everything it loaded
  goes.
- **Against**: Built into the registry it is a second ownership model, which the editor and the
  UI would never use.
- **Rejected because**: A scope is a list of handles released in a loop, which can be built on
  explicit release in a few lines; the reverse is not true.

### Destroy on release, after `vkDeviceWaitIdle`
- **For**: Correct with no queue, and nothing outlives its handle.
- **Against**: Every release during play stalls the main thread until the device drains.
- **Rejected because**: The stall lands exactly where releases happen during play.

## Consequences

- **Gains**:
  - A texture can be replaced or unloaded without leaking, and a reused slot cannot be reached
    through an old handle.
  - The draw path is unchanged: the slot is still the sort order. Drawing with a released
    handle draws nothing rather than using freed memory.
  - Anything with Vulkan objects to free after the frames in flight retires through the same
    ring, which holds callbacks and needs no knowledge of what it destroys.
- **Costs**:
  - A caller has to remember to release, and a forgotten release leaks quietly.
  - A handle is eight bytes. The generation is a separate 32-bit field because a narrow one
    wraps after a few hundred reuses of a slot. A map keyed by handle keys by the whole handle,
    not the slot.
  - Objects outlive their handle by up to the frames in flight, so a test counting live
    allocations has to run the ring that far first.
  - Freeing too early shows only as a validation message on a device. A frame driven without
    `Ring::begin()` never collects.
- **Revisit when**: forgotten releases become a common defect, which would argue for scopes or
  ownership built on top of explicit release.
