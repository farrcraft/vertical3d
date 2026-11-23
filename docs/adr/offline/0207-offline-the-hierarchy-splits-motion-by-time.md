# ADR-0207: Offline: the hierarchy splits motion by time

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline
**Documented in**: [offline/Traversal.md](../../offline/Traversal.md)

## Context

Each scene object keeps its own sample times over the shutter, so objects in one hierarchy may
hold different counts of samples at different times. A box over the whole shutter is loose for
anything that moves far. A transformation interpolated by decomposition turns a box along a
curve, so boxes at the ends of a segment do not bound its middle. HIP RT takes uneven sample times
as they are, and OptiX takes even keys only.

## Decision

Each node holds its children's boxes at the start and end of the time range it covers. The
builder splits a node by time where that costs less than splitting it in space, as the
spatial-temporal hierarchy does. A rotating instance is bounded exactly, from the times at which
its corners' motion turns.

## Alternatives

### Boxes at two times, with a subtree per segment
- **For**: A simpler build with no time splits, as Embree's linear motion and OptiX's motion nodes
  hold motion.
- **Against**: An object with several segments needs a subtree for each, and objects with
  different times force the segments of all of them on each other.
- **Rejected because**: Time splits cost much less memory than a hierarchy per segment.

### One box over the whole shutter
- **For**: The simplest build and node, as the old hierarchy held motion.
- **Against**: A fast object's box covers its whole path, and every ray tests it.
- **Rejected because**: Production motion blur is often fast.

### Bounding a rotation by boxes at sampled times
- **For**: Simple, with a margin added.
- **Against**: A margin either misses the swept box or loosens it.
- **Rejected because**: pbrt and Embree both bound it exactly.

## Consequences

- **Gains**:
  - Each object's own times are kept, and fast motion stays tight.
  - A rotating instance is never missed between samples.
  - HIP RT takes the same times directly.
- **Costs**:
  - Nodes carry two boxes per child and a time range, twice the memory of a still node.
  - The builder weighs time splits as well as spatial ones, and is slower.
  - OptiX's back end resamples to even keys when it builds, as ADR-0201 foresaw.
- **Revisit when**: motion in moya's scenes is short enough that one box per segment is as fast.
