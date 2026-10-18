# ADR-0092: Offline: a bounding volume hierarchy for traced rays

**Status**: proposed
**Date**: 2026-10-18
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

Both hiders trace shadow and reflection rays through one `trace::Scene`, and it tests every
primitive for every ray. Area lights multiply the number of shadow rays per shading point. A
scene mixes scales: a ground quad hundreds of units wide sits beside triangles a fraction of a
unit across. A moving primitive is stored at its reference pose, and a ray is tested against it
at the ray's time. The structure has to hold still and moving primitives, and leave both
hiders unchanged.

## Decision

`trace::Scene` holds its primitives in a bounding volume hierarchy, built with a binned surface
area heuristic the first time a ray is traced after a primitive was added. A moving primitive
is bounded by the union of its bounds across its poses, and sits in the hierarchy like any
other.

## Alternatives

### A uniform grid
- **For**: simple to build and to step through, with no tree to balance.
- **Against**: a ground plane hundreds of units wide and small triangles beside it put most
  primitives in a few cells, or need cells too small for the plane. Its resolution is one more
  limit to tune per scene. A fast-moving primitive lands in many cells.
- **Rejected because**: the scenes this renderer reads mix scales, and a grid does not adapt
  to that.

### A kd-tree
- **For**: traversal is often faster than a hierarchy's, because its cells do not overlap.
- **Against**: a primitive can sit in several leaves, so a ray can test it more than once. A
  build with a good split heuristic costs more, and a swept bound of a moving primitive spreads
  it over many leaves.
- **Rejected because**: its traversal gain does not pay for a costlier build and for the
  moving primitives.

### A hierarchy of still primitives, with moving ones in a list
- **For**: every bound in the hierarchy is a plain one, and a moving primitive needs no swept
  bound.
- **Against**: every ray tests every moving primitive, so a motion-blurred scene stays as slow
  as it is now.
- **Rejected because**: a swept bound costs little, and keeps a motion-blurred scene fast.

## Consequences

- **Gains**: a ray tests the primitives near it, not the whole scene. Neither hider changes,
  and a new kind of primitive needs only a bound.
- **Costs**: the hierarchy is built once per render and holds memory beside the primitives. A
  swept bound is loose for a primitive that moves far, so a ray tests it where it is not. A
  primitive added after the first trace rebuilds the whole hierarchy.
- **Revisit when**: a profile shows traversal, rather than shading, dominating a render, or
  primitives are added between traces often enough that a full rebuild shows.
