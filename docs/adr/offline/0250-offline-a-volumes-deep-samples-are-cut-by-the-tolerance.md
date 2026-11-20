# ADR-0250: Offline: a volume's deep samples are cut by the tolerance

**Status**: accepted
**Date**: 2026-11-19
**Tags**: offline, compositing, volumes
**Amends**: ADR-0233
**Documented in**: [offline/Compositing.md](../../offline/Compositing.md)

## Context

A deep file's volumetric sample spans a front and a back depth, and a compositor spreads its alpha
across that span as if the medium's density were even there. A camera sample that collides in a
medium records one depth, and each pixel's records are joined on the host by their IDs and a depth
tolerance. A grid's density varies along a ray, and a surface placed inside fog in compositing
needs samples on both sides of it. Arnold merges deep samples, volumes among them, by depth and
alpha tolerances.

## Decision

Each collision in a medium records its depth as a surface's hit does, and records of one volume
whose depths lie within the deep tolerance join into one sample. A sample joined from collisions
spans the least and the most depth it holds, so its front and back depths differ wherever the
medium's collisions spread.

## Alternatives

### One sample for each medium a ray crosses
- **For**: The fewest samples, and a span that is the medium's own.
- **Against**: A compositor spreads each sample's alpha evenly over its span, which a grid's
  density is not, and a surface inside the fog falls within one sample.
- **Rejected because**: Deep output is for placing things inside volumes, which one span per
  medium cannot do.

## Consequences

- **Gains**:
  - A volume's samples follow its density, as finely as the tolerance asks.
  - Volumes and surfaces join by one rule, so the merge has no case of its own for volumes.
- **Costs**:
  - A thick medium gives a pixel many samples, as many as its depth over the tolerance.
  - Each span is as wide as the collisions in it happened to spread, so it varies with the
    samples taken.
- **Revisit when**: Deep files of thick volumes grow too large to composite.
