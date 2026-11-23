# ADR-0185: Offline: a surface sets its trace bias

**Status**: superseded
**Date**: 2026-11-06
**Tags**: offline
**Superseded by**: [ADR-0186](0186-offline-moya-renders-by-path-tracing-alone.md)

## Context

A shadow or traced ray starts a small distance off the surface it leaves, along the geometric
normal, so that it does not meet that surface again. The reyes hider shades its own grids, but
traces through a separate copy of each displaced surface, diced to a fixed size. Where the
surface curves, the grids and the copy lie apart by more than a fixed offset of 0.0001, and a ray
from a grid meets the copy and shadows its own surface in stripes. How far apart they lie depends
on the surface's size and curvature, which the renderer does not compute.

## Decision

`Attribute "trace" "bias"` sets, per primitive, how far a ray leaving it starts, in world units,
under both hiders. The default stays 0.0001, and a scene with displaced surfaces sets a larger
bias. The name and meaning are those RenderMan renderers use, so a RIB written for one carries
over.

## Alternatives

### An automatic bias measured while dicing
- **For**: nothing for a scene to set. moya would measure how far each displaced primitive's
  traced copy strays from its grids, and offset that primitive's rays by as much.
- **Against**: the reyes grids are diced bucket by bucket after the traced copy is built, so the
  gap is not known when the copy is made. It is moya's own scheme, with no counterpart a
  RenderMan user would know.
- **Rejected because**: it needs the two dicings to be compared, which they never are, and the
  attribute the established renderers use already fixes the defect.

### A ray ignores its own primitive's traced copy
- **For**: no stripes, and no value to choose.
- **Against**: a displaced surface could no longer shadow itself, so one ripple could not cast a
  shadow onto the next.
- **Rejected because**: it trades a visible defect for a wrong picture.

## Consequences

- **Gains**: a displaced scene renders without stripes under the reyes hider, and a RenderMan
  RIB's trace bias means the same in moya.
- **Costs**: a scene has to know to set it, and a default render of a strongly displaced
  surface still stripes. Too large a bias lets light through where two surfaces touch.
- **Revisit when**: the reyes hider traces against the grids it shades rather than a separate
  copy, which would remove the gap.
