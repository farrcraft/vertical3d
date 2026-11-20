# ADR-0211: Offline: a round curve is swept by a sphere

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline
**Amends**: ADR-0208
**Documented in**: [offline/Traversal.md](../../offline/Traversal.md)

## Context

A round curve's surface has to be defined before every device can intersect it alike. Its
radius may change along it, and a curve has ends and joints. A flat curve faces each ray, and a
curve's look is set by what each ray meets. Where the radius holds still, every definition gives
the same tube.

## Decision

A round curve is the surface a sphere of the curve's radius sweeps as it moves along the curve.
Its ends are the spheres there. A flat curve covers, as each ray sees it, exactly what the round
one does.

## Alternatives

### A circle swept perpendicular to the curve
- **For**: OptiX's and Embree's round curves are defined so, and a shading frame follows the
  tangent directly.
- **Against**: Its ends are open, a kink between segments leaves a gap, and where the radius
  changes, its silhouette is not the flat curve's.
- **Rejected because**: A closed surface that a flat curve matches needs no caps, joints or
  special cases of its own.

## Consequences

- **Gains**:
  - Ends and joints are closed with no data beyond the segments.
  - A linear segment is two spheres and a cone, in closed form.
  - Switching a curve between round and flat changes its shading and depth, not its coverage.
- **Costs**:
  - Where the radius changes fast, the surface differs a little from OptiX's and Embree's, so a
    back end that uses their built-in curves would not match moya's own.
  - Each segment tests the sphere at its start, which a circle sweep would not.
- **Revisit when**: a GPU back end needs its vendor's built-in curves for speed.
