# ADR-0051: Frame Pacing — The In-Flight Ring Is Its Own Class, And Presenting Is What Needs A Chain

**Date**: 2026-09-11
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`vulkan::frame::Presenter` owns two things that have nothing to do with each other: the ring of
frames recorded ahead of the device — a command pool, a command buffer and a fence per frame,
`framesInFlight()`, `frame()`, `waitFrame()` — and the acquire/present loop, which needs a
swapchain, an image-available semaphore per frame and a render-finished semaphore per image.
Every renderer in the tree takes a `Presenter` and uses only the first half: `Quad`, `Line` and
`World` size their geometry rings by `framesInFlight()`, index them by `frame()` and call
`waitFrame()` before rewriting a slot. `FrameUniforms` is constructed from `framesInFlight()`
alone.

The consequence surfaced while building the render tests [ADR-0007](0007-ci-rendering-tests.md)
decided. A device can now be selected without a surface, and a pass can draw into a target it
names ([ADR-0031](0031-a-pass-draws-into-a-target-it-names.md)) — but a context with no window
still cannot construct a single renderer, because a renderer asks for a presenter and a presenter
asks for a chain. Nothing in the ring needs one.

## Decision

The in-flight ring moves into `vulkan::frame::Ring`, which needs a device and nothing else.
`Presenter` holds a `Ring` and adds the swapchain, the semaphores and the acquire/present loop.
Renderers and `FrameUniforms` take a `Ring`.

## Alternatives Considered

### Alternative 1: Split the ring out of the presenter — **chosen**
- **Pros**: Names the two jobs, which are already two jobs. The ring becomes constructible
  without a surface, so a headless context can build every renderer. It is the smallest change
  that makes the dependency honest: a renderer that never presents stops asking for a presenter.
- **Cons**: Touches the path every frame in the tree goes through. The per-frame fence is waited
  on by the ring and signalled by a submit that stays in `Presenter`, so ownership of that fence
  now spans the boundary and the ring has to expose it.
- **Why not**: n/a — chosen.

### Alternative 2: Make the presenter's swapchain optional, as ADR-0007's device work did
- **Pros**: Smallest possible change, and consistent with the precedent set days earlier, where
  a null surface gives a device that only draws.
- **Cons**: Leaves a class named `Presenter` that in one of its two modes cannot present, and
  leaves the two responsibilities tangled in one file with a null check separating them. A
  device that only draws is still a device; a presenter that never presents is misnamed.
- **Why not**: The name would have to lie for the shape to stay. The device precedent works
  because presenting is a capability a device may lack, not the thing a device is for.

### Alternative 3: An abstract interface the renderers take, implemented by the presenter
- **Pros**: `Presenter` is untouched, so the present path carries no risk at all. A headless
  implementation sits beside it.
- **Cons**: Adds virtual dispatch to `frame()`, which is called per draw batch per frame, and
  creates a second implementation of the ring that has to stay in step with the first. Two
  implementations of frame pacing is the thing most likely to drift silently.
- **Why not**: It buys safety on the present path by paying for it with duplication on the
  pacing path, and the duplication is permanent while the risk is one-off.

### Alternative 4: Leave it, and let render tests cover only what needs no renderer
- **Pros**: No change at all. A suite can already draw a clearing pass into a target, capture
  it and assert the validation layer is silent — which covers device selection, the recorder,
  the layout transitions and the barriers, and is the highest-yield class of defect per
  ADR-0007.
- **Cons**: `Quad`, `Line` and `World` are the largest untested thing in `api/render/realtime`
  and would stay untested, which is most of what the testing workstream is for.
- **Why not**: Deferred rather than rejected as a sequencing option, and then not taken: the
  coverage it gives up is the coverage worth having.

## Consequences

### Positive
- A headless context can build every renderer, which is what
  [RenderTestsInCI](../plans/RenderTestsInCI.md) needs before a suite can draw anything real.
- The dependency a renderer declares becomes the dependency it has. `Quad` asking for a
  `Presenter` was always a claim that it presents, which it does not.
- `FrameUniforms` needs a frame count rather than a presenter, which was already true and was
  not sayable.

### Negative
- The per-frame fence is created and waited on by `Ring` and signalled by `Presenter`'s submit.
  That is one responsibility split across two classes, and it is the part of this that a reader
  will have to be told rather than infer.
- Two classes exist where one did, and the smaller of them has no independent use in this tree
  today beyond the tests that motivated it.

### Risks
- The ring is on the hot path, and a mistake in the fence or the command-buffer rotation is a
  synchronization bug rather than a compile error. The mitigation is that
  `VK_LAYER_VALIDATE_SYNC=1` sees exactly this class of fault, and every app in the tree
  exercises the ring on every frame, so a regression is immediate rather than latent.
- The escape hatch is that `Presenter`'s public surface does not change. If the split proves
  wrong, `Ring` folds back into it without touching a caller of `acquire()` or `present()`.
