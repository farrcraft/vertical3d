# ADR-0061: Resources: explicit release, generational handles

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`pipeline::Resources` adds and never removes, and its `Registry` promises that a slot is never
reused. That held while every consumer drew one board loaded at startup. It stops holding as
soon as anything replaces a texture: cozy's debug hot reload leaks the texture it replaces,
cozy's M6 regions load and unload sheets during play, and
[milestone 4](../roadmap/completed/m4-LitScene.md)'s texture classes need a registry that release works
in. Two questions have to be settled together: who decides that a resource is dead, and when its
Vulkan objects may be destroyed, given that a frame recorded before the release may still be
reading them. [Large worlds](../plans/completed/LargeWorlds.md#step-1--a-registry-slot-is-reused-and-a-stale-handle-is-refused)
is the plan that needs it.

## Decision

**A resource is released explicitly, by handle.** A handle carries a slot and a 32-bit
generation. A released slot is reused by the next `add` with its generation incremented, and
`resolve` refuses a handle whose generation is not the slot's current one, so a handle used
after its release resolves to nothing. **The objects behind it are retired to the in-flight ring
([ADR-0051](0051-frames-in-flight-ring-separate-from-presenting.md)) as a callback, and destroyed once
every frame that began before the release has finished.** This amends
[ADR-0010](0010-meshes-owned-by-the-app-that-built-them.md)'s premise that `Resources` never frees. It does
not supersede it, because meshes stay the app's.

## Alternatives Considered

### Alternative 1: A reference count held by whatever keeps a handle
- **Pros**: Nothing can be released while something still names it, and nobody has to remember
  to release.
- **Cons**: A handle is a value copied into every `DrawItem` and packed into a `SortKey`
  ([ADR-0004](0004-rendering-submit-draw-items-as-data.md)). Counting it makes every one of those copies a
  counted one, and a count held in a sort key means nothing.
- **Why not**: It costs the draw path for a convenience only load and unload need.

### Alternative 2: A scope, everything registered for a scene released with it
- **Pros**: Matches what cozy's region and retcon's scene both want: unload the region, and
  everything it loaded goes.
- **Cons**: A scope is a list of handles the caller keeps and releases in a loop. Built into the
  registry it is a second ownership model, which the editor and the ui would never use.
- **Why not**: It can be built on top of explicit release in a few lines, and not the other way
  around.

### Alternative 3: Explicit release, a generation, and destruction deferred by the ring — **chosen**
- **Pros**: The draw path is unchanged: `id()` is still the slot and the sort order. The mistake
  it allows, drawing with a released handle, gives a handle that resolves to nothing rather
  than a use after free. The ring already knows when a frame has finished, and the queue holds
  callbacks, so it does not need to know what it destroys.
- **Cons**: A caller has to remember to release, and a release that is forgotten leaks quietly,
  as it does today.
- **Why chosen**: It is the cheapest of the three to get wrong.

### Alternative 4: Destroy on release, behind a `vkDeviceWaitIdle`
- **Pros**: Correct with no queue at all, and nothing outlives its handle.
- **Cons**: Unloading a region stalls the main thread until the device drains, which is the
  hitch cozy's M6 starts with.
- **Why not**: It is correct and visibly slow, in the one place a release happens during play.

## Consequences

### Positive
- A texture can be replaced or unloaded without leaking, and a slot reused after a release can
  never be reached through the old handle.
- Anything with Vulkan objects to free after the frames in flight, milestone 4's images and
  samplers or a mesh once uploads stop idling the queue, retires into the same ring without a
  change to it.

### Negative
- **A handle is eight bytes rather than four.** `SortKey` packs only the slot, so it does not
  grow. The generation is a second 32-bit field rather than bits taken from the slot, because a
  narrow generation wraps after a few hundred reuses of one slot, and a long session streaming
  regions reaches that.
- Two handles with the same slot but different generations are different handles, so a map
  keyed by handle has to key by the whole handle rather than by `id()`.
- Objects outlive their handle by up to the frames in flight. A device suite that counts live
  allocations has to run the ring that far before counting.

### Risks
- **Something that is freed too early gives no error except a validation message, and only on a
  device.** The device suite's release case, which runs with the validation layer on, is the
  guard. Under lavapipe in CI it is the same check on a second driver.
- A frame driven without `Ring::begin()` never collects. Both drivers in the tree, `Presenter`
  and the device tests, begin through it. If another driver appears, it has to as well.
