# ADR-0086: Loading: one worker thread, polled from the main thread

**Status**: accepted
**Date**: 2026-10-16
**Documented in**: [docs/api/Assets.md](../api/Assets.md)

## Context

Every load runs on the main thread, so a load is a frame hitch the size of the load. Decoding
an image or parsing a model is most of that time, and needs no device. Uploading to the device
and changing an ECS registry must stay on the main thread, because the frame loop records
against both. No code in `api/` starts a thread, so every class there assumes one caller at a
time. One `asset::Manager` loads correctly from several threads at once, which a test in the
asset media suite asserts.

## Decision

A load is split into a job that decodes on one worker thread and a completion that runs on the
main thread. The worker holds each finished result until its owner polls it, once a frame from
`tick()`, and the poll runs each completion there. A job may only decode and parse through an
`asset::Manager`; it never calls Vulkan, touches an ECS registry, or reaches a realtime registry.

## Alternatives

### A pool of worker threads
- **For**: decodes several files at once on a machine with spare cores.
- **Against**: the order results arrive in varies from run to run, and a pool needs a size,
  shutdown rules and a queue shared between threads.
- **Rejected because**: one thread already takes the hitch off the frame. A pool adds
  nondeterminism before any load is shown to need it.

### Completions run on the worker thread
- **For**: no poll, and a result is used the moment it exists.
- **Against**: every completion would need a lock around whatever it touches, and nearly
  everything a completion does is an upload or a registry change.
- **Rejected because**: it spreads thread safety through code that assumes one caller.

### `std::async` and futures the caller waits on
- **For**: no new class, and the standard library owns the thread.
- **Against**: a future's destructor can block, the thread count is the implementation's choice,
  and a caller has to check every future each frame.
- **Rejected because**: the destructor blocking turns a released handle into a hitch.

## Consequences

- **Gains**: a load costs the frame only its upload. Code that runs on the main thread stays
  single-threaded, and the order of completions follows the order of requests.
- **Costs**: a result is at least one frame late, and a request that is released before it
  finishes still spends the worker's time decoding. MSVC has no thread sanitiser, so the worker's
  tests have to fix the order of events themselves rather than rely on a tool to find a race.
- **Revisit when**: a profile shows the worker itself is the bottleneck, or a job needs the
  device, such as compressing a texture on the GPU.
