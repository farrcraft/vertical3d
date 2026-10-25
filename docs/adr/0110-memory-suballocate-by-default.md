# ADR-0110: Memory: suballocate by default

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0053
**Documented in**: [internals/realtime/Memory.md](../internals/realtime/Memory.md)

## Context

A world that streams makes buffers and images during play, a region's worth at a time, and the
upload budget has to keep each poll inside a small share of a frame. Once an upload no longer
waits for its copy, what a poll costs is mostly making memory for what it uploads. With one
device allocation per resource, a mesh costs about as much as a megabyte texture, and a region
of a few dozen of each takes several frames' worth of time. A device also limits how many
allocations are live, and a streamed world reaches that limit after a few dozen regions.

## Decision

The allocator places every buffer and image in a region of a larger block, through the Vulkan
Memory Allocator, unless a consumer asks for one device allocation per resource. Direct
allocation stays as the other kind, and the device, `Context3D` and `Engine3D` still take the
choice.

## Alternatives

### Keep direct allocation as the default, and set the budget for it
- **For**: no app's memory changes, and the default keeps what each existing app was built
  against.
- **Against**: on the development machine a poll fits a frame's share only at a quarter of a
  megabyte, about one texture or two meshes, so a region takes more than a second to stream,
  and the allocation limit stays.
- **Rejected because**: every vendor guide and engine suballocates for this reason, and measured
  here it makes a poll several times cheaper.

### Suballocate only, and remove direct allocation
- **For**: one path, with no choice to make or test.
- **Against**: a case that wants each resource's memory to be its own, such as a check that a
  resource is bound at offset zero, loses the way to ask for it.
- **Rejected because**: the direct path costs little to keep, and a device test still draws
  through it.

## Consequences

- **Gains**: a poll uploads several times as much in its share of a frame, and the number of
  live allocations stays far under the device's limit.
- **Costs**: every app moves to memory it did not ask for. The allocator holds blocks larger
  than what is live in them, and a poll that fills the last block pays for a new one. The
  direct path is now the one only tests run.
- **Revisit when**: a dedicated allocation per resource is wanted for a kind of resource, such as
  a large render target, which VMA can make within the suballocated kind.
