# ADR-0056: Camera Precision — `lookat()` Keeps The Basis It Built, And Every Other Writer Of The Rotation Clears It

**Date**: 2026-09-12
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`Profile::lookat()` built the basis as a `mat4x4` whose columns are the three normals, cast it
to a quaternion and threw the matrix away. `Camera::createView()` was the only reader of that
quaternion, and it began by casting it back to a matrix — so a view was built out of a round
trip through a representation nothing in between needed.

The round trip costs precision. Measured against `glm::lookAt` across eight camera placements,
a view assembled this way differs from glm's by up to **1.9e-6** in an element, and up to twelve
of its sixteen elements differ.

That is not a correctness problem and no picture built from it is wrong. It is a *stability*
problem, and it belongs to a consumer rather than to this tree: retcon — an app on this api that
is not in this tree ([ADR-0027](0027-the-api-is-consumed-as-source.md)) — replaced its own
`glm::lookAt` camera with this one and its reference capture moved by 153 of 891600 pixels, none
of them geometrically wrong and two of them on a hard outline edge. A reference frame compared at
zero tolerance ([ADR-0054](0054-a-realtime-reference-is-a-picture-the-spec-determines.md)) is
re-baselined by a difference of one unit in the last place, so "close enough" is the thing that
costs rather than the thing that passes.

The entry was reported as U18 and asked one question before any code: **is bit equality with
`glm::lookAt` reachable at all?** It said in as many words that if it is not, the finding should
be written down instead of the change.

## Decision

**`lookat()` keeps the matrix it already built, and `createView()` prefers it over casting the
quaternion.** `Profile` carries `basis_` and `basisValid_`; the quaternion is still written,
because it is what `pan()`, `tilt()` and an arcball compose onto and what a consumer reads.

**Every other writer of the rotation clears the cache** — `Profile::rotation()`,
`Camera::pan()` and `Camera::tilt()` — and `createView()` falls back to `glm::mat4_cast()` when
it is clear. `Profile::clone()` copies both, since a clone of a profile built by `lookat()` would
otherwise build its view the other way.

**`lookat()` no longer normalizes the up vector.** `z` and `x` are unit and perpendicular, so
their cross product is already unit to within rounding, and `glm::lookAt` does not take that
step. Taking it was the second of the two differences and, once the round trip was gone, the
only one left.

## Alternatives Considered

### Alternative 1: keep the round trip, document that a view is rebuilt to within float rounding
- **Pros**: no cached state, no invalidation rule, nothing for a fourth writer to forget.
- **Cons**: leaves the consumer re-baselining a reference frame every time an unrelated change
  perturbs the last bits, which is the cost the entry was written about.
- **Why not**: the measurement said exactness was reachable, which is the condition U18 set for
  doing the work at all. Had it not been, this is what would have been recorded instead.

### Alternative 2: keep the matrix and leave the up vector normalized
- **Pros**: the smaller change, and the one U18 actually proposed.
- **Cons**: measured — it closes most of the gap and not the whole of it. Worst element error
  falls from 1.9e-6 to 1.2e-7, and the view is bit identical to glm's in one of eight placements
  rather than eight. A consumer would re-baseline once more and then keep re-baselining.
- **Why not**: a partial answer here has the cost of the full one and none of the benefit. The
  entry's own gate was exact equality, not a smaller number.

### Alternative 3: `createView()` reads the three normals instead of any rotation
- **Pros**: no cache and no invalidation.
- **Cons**: `pan()` and `tilt()` compose onto the rotation and leave the normals as `lookat()`
  left them, and `camera_perspective_view_test` sets the rotation directly and asserts
  `createView()` honours it. Both would break, and so would an arcball consumer.
- **Why not**: the normals and the rotation are independent state and `Profile` says so. This
  would quietly make one of them authoritative.

### Alternative 4: write the translation as `glm::lookAt` does, with three dot products
- **Pros**: it was the hypothesis U18 offered for where the residual lived.
- **Cons**: measured, and it changes nothing at all — the translation column is already
  identical, because `glm::translate`'s column sum and glm's dot products round the same way.
- **Why not**: it was not the cause. The cause was the round trip and the extra normalize.

## Consequences

### Positive
- A view built through `lookat()` is now element-for-element equal to `glm::lookAt`'s, with no
  tolerance, across every placement tested. A consumer holding reference frames against this
  camera stops re-baselining them.
- `createView()` does less work: a matrix it was handed rather than a cast of a cast.
- The finding cost nothing in this tree. The render suite's reference images build their
  matrices directly rather than through `type::camera::Camera` — `WorldDepthTest` says so — so
  nothing pinned at zero tolerance here depends on this arithmetic.

### Negative
- `Profile` now has state that can be stale, and three writers have to keep it honest. **A
  fourth writer of `rotation_` added later has to clear `basisValid_`, and nothing enforces
  that.** The header says so at the member.
- The up vector a consumer reads is no longer exactly normalized. It is unit to within float
  rounding, which is what `glm::lookAt`'s is.

### Risks
- The invalidation rule is the whole of what can go wrong, and what it produces is a view that
  is right to 2e-6 rather than a failure — which is invisible without a reference frame.
  `camera_a_set_rotation_outlives_a_cached_basis_test` covers the three writers that exist, by
  asserting that a directly set rotation and a pan after a `lookat()` both still build the view
  the quaternion describes.
- Equality is asserted with `==` rather than by comparing bits. The mirrored hand
  ([ADR-0052](0052-a-consumer-names-the-camera-hand.md)) turns some zeros negative, and
  `-0.0f == 0.0f` while their bits differ. That is the one place the gate is looser than "the
  same bits", and it is looser only about the sign of zero.
