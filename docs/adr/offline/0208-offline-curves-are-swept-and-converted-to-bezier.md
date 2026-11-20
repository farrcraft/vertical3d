# ADR-0208: Offline: curves are swept and converted to Bézier

**Status**: amended
**Date**: 2026-11-08
**Tags**: offline
**Amended by**: ADR-0211
**Documented in**: [offline/Traversal.md](../../offline/Traversal.md)

## Context

moya intersects curves by its own code on every device, so the CPU and a GPU agree. A curve is
thin, so its intersector sets how it looks close up and how robust it is where a ray grazes it.
RI's curves come in five bases, linear or cubic, with a round or a flat profile. Every cubic basis
is a cubic polynomial, and converts exactly to Bézier.

## Decision

A round curve is a circle swept along it, intersected by subdividing each segment to a fixed
depth and refining the hit by Newton's method, as Embree does. A flat curve is a ribbon that
faces the ray. Every cubic curve is converted to Bézier when the scene is built, and a linear
curve stays linear.

## Alternatives

### Reshetov's phantom intersector for round curves
- **For**: Stackless, exact to the swept surface, and fast.
- **Against**: It misbehaves at high curvature unless the curve is split further. NVIDIA chose
  against it for OptiX for this reason.
- **Rejected because**: Close-ups of curled hair are where a curve has to hold.

### Recursive subdivision until a piece is near straight
- **For**: Simple and robust, as pbrt does it.
- **Against**: Its depth varies with curvature and width, and a GPU needs a stack to hold it.
- **Rejected because**: A fixed depth with Newton's method holds as well on a fixed stack, and
  Cycles has run it on GPUs.

### Each basis intersected as it is given
- **For**: No conversion, and the scene's control points are the intersector's, as Embree and
  OptiX take several bases.
- **Against**: An evaluator and a bound for each basis, in every device's code.
- **Rejected because**: The conversion is exact, and one basis is one intersector to test.

## Consequences

- **Gains**:
  - Curves look alike on the CPU and every GPU, and hold close up.
  - One cubic intersector and one linear one.
  - Flat curves face the camera for camera rays, as RI's do.
- **Costs**:
  - A round curve costs more than a ribbon.
  - A flat curve faces each ray, so a shadow ray and a camera ray see different ribbons.
  - Conversion holds the curves twice while the scene is built.
- **Revisit when**: a GPU intersects every cubic basis in hardware with results moya's own code
  can match.
