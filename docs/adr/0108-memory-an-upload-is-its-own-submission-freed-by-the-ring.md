# ADR-0108: Memory: an upload is its own submission, freed by the ring

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: docs/internals/realtime/Memory.md, api/render/realtime/vulkan/memory/Uploader.h

## Context

A buffer or image in device-local memory is filled by a copy from a staging buffer, and the
staging buffer must outlive the copy. Today the uploader waits on a fence for each copy, so a
streamed region stalls the main thread once per mesh and texture. Uploads come from inside a
frame, from an app's start-up, from tests that never present, and in time from worker threads.
The device may have a queue family for transfers only, but the software driver that CI runs has
one family, and a resource moved between families needs a release and an acquire barrier.

## Decision

An upload records its copy into a command buffer of its own, submits it on the graphics queue
and returns without waiting. The staging buffer and the command buffer are retired through the
frame ring, so they are freed once the frame being recorded has finished. A barrier at the end of
the copy makes it visible to every later submission on the queue.

## Alternatives

### Record the copies into the frame
- **For**: one submission a frame, and the copies are ordered by the frame's own command stream,
  as WebGPU's pending writes are.
- **Against**: an upload made outside a frame, at start-up or in a test, needs a second path that
  flushes at once. Recording is tied to the thread recording the frame.
- **Rejected because**: it shuts out uploads from a worker thread and a later move to a transfer
  queue, and still needs the path this decision takes for uploads outside a frame.

### A dedicated transfer queue
- **For**: the copy engine runs the copies while the graphics queue draws, which is how engines
  that stream heavily, such as The Forge and Godot's transfer workers, do it.
- **Against**: each resource needs a release on the transfer queue and an acquire before its first
  use on the graphics queue, and the frame waits on a semaphore. Devices without the family fall
  back to the graphics queue, and CI has only the fallback, so the handoff is never tested there.
- **Rejected because**: at the default upload budget of a few megabytes a poll, the copy takes a
  fraction of a millisecond, so there is little to overlap, and the path it adds is the one CI
  cannot run.

## Consequences

- **Gains**: no upload waits on the device, in a frame or outside one, and one path serves every
  caller. A poll's uploads can share one submission.
- **Costs**: a submission per upload or per batch, and command buffers and staging memory held for
  up to the frames in flight. An upload depends on the ring, so an uploader no longer stands alone.
  A copy still runs on the graphics queue, between frames' work.
- **Revisit when**: a measured poll's copies take a noticeable share of a frame on the graphics
  queue, or worker-thread uploads arrive and need a queue that does not contend with drawing.
