# ADR-0052: Camera Basis — A Consumer Names Which Way `lookat()` Crosses, And Today's Hand Stays The Default

**Date**: 2026-09-12
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0012](0012-camera-builds-vulkan-clip-space.md) settled that `type::camera` builds Vulkan
clip space and left the axes alone: `eye`, `up`, `right` and `direction` are the right handed,
y up basis they always were. `Profile::lookat()` derives three of those four from the fourth,
and it crosses `right = up × direction`.

`glm::lookAt` crosses the other way, `right = direction × up`. Both are right handed bases and
both render. From one eye, one up and one centre they produce rights that are negatives of each
other and the same up, so a scene drawn through one is the horizontal mirror of the same scene
drawn through the other — and a mirror reverses the winding a front face presents.

An external consumer met this. retcon — an app on this api that is not in this tree
([ADR-0027](0027-the-api-is-consumed-as-source.md)) — has an isometric camera of its own that
duplicates `type::camera::Isometric` almost exactly, and could not delete it. Its meshes are
wound for `glm::lookAt`'s hand and both of its cull modes read the winding, so this camera does
not mirror its frame, it culls it to black. Two implementations of one isometric camera in two
repositories is a thing that drifts.

This is not a defect. `Camera` documents its convention, `Isometric::right()` derives from the
basis on purpose rather than naming a vector, and ADR-0012 chose it. It is a reuse problem, and
`type::camera` was the one part of the api an outside application could not adopt incrementally,
because adopting it was a decision about that application's whole renderer.

## Decision

`Profile` carries a `Hand`, and `lookat()` crosses the way it names. `Hand::UpCrossDirection`
is the default and is ADR-0012's basis unchanged; `Hand::DirectionCrossUp` is `glm::lookAt`'s.
Both crosses follow it — the up is recomputed as the component of the original up perpendicular
to the direction either way, so the two hands mirror horizontally and agree about which way is
up.

The hand is profile state and `Isometric::apply()` does not reset it, so an application sets it
once on the camera it owns and every placement written onto that camera honours it.

## Alternatives Considered

### Alternative 1: A hand the caller may name — **chosen**
- **Pros**: ADR-0012 is unamended for every camera in this tree, which names nothing and gets
  what it got before. The decision moves to the layer that knows which way its geometry is
  wound, which is the application. It is the same shape
  [ADR-0049](0049-a-consumer-chooses-the-swapchain-format.md) chose four days earlier for the
  swapchain format, for the same reason.
- **Cons**: two conventions exist in one camera class, so `right()` cannot be read without
  knowing which hand built it.
- **Why not**: n/a — chosen.

### Alternative 2: Document the constraint and decline
- **Pros**: one camera, one convention, matching the one renderer — which is what ADR-0012
  said and why. Nothing to build and nothing to get wrong.
- **Cons**: leaves a duplicate isometric camera in a consumer permanently, and leaves the one
  part of this api that cannot be adopted incrementally unadoptable. ADR-0049's third
  alternative already recorded the principle: the api being unusable by its second consumer is
  a fault in the api, not in the consumer.
- **Why not**: the cost is paid forever by every future consumer whose geometry is wound the
  common way, and it is paid to avoid a branch in one function.

### Alternative 3: Change the basis to `glm::lookAt`'s and correct this tree
- **Pros**: one convention again, and the one the wider ecosystem uses.
- **Cons**: it is a silent change to the meaning of every camera in the tree. Six of the
  editor's eight profiles are orthographic and its picking reads back through `project()` and
  `unproject()`; the nine apps are the proof obligation, and none of them is asking for this.
- **Why not**: it inverts who pays. The consumer that wants the other hand is the one that
  knows it wants it.

## Consequences

### Positive
- An application whose geometry was authored against `glm::lookAt` can adopt `type::camera`
  and `Isometric` rather than writing a second camera beside them.
- The hand travels with the profile through `clone()` and assignment, so a copied camera keeps
  building the basis it was built for.

### Negative
- `Profile::right()` is no longer answerable from the profile's inputs alone — a reader needs
  the hand as well. The default makes that invisible to every caller in this tree, which is
  also what makes it easy to forget.
- `Isometric::right()` and `forward()` follow the hand, so a pan or an orbit built on one and
  read through the other moves the scene the wrong way with nothing else looking wrong.

### Risks
- The hand is set before `lookat()` and does nothing after it, because the normals are state
  rather than a derivation. A caller that sets it on a profile already oriented gets the old
  basis until something calls `lookat()` again. Mitigated by the setter saying so and by
  `Isometric::apply()` calling `lookat()` every time.
