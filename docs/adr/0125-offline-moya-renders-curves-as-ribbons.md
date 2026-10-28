# ADR-0125: Offline: moya renders curves as ribbons

**Status**: accepted
**Date**: 2026-10-27
**Documented in**: [offline/README.md](../offline/README.md)

## Context

The editor exports its curves as RI `Curves` and renders them through moya, in Render View and
in batches, and the same RIB may go to another RenderMan renderer. moya draws polygons and
spheres, and `Curves` has an empty body, so an exported curve renders as nothing. Hair, fur,
wires and grass are thousands of thin curves whose width varies along their length. RI defines a
curve as a flat ribbon of a given width, facing the camera unless normals are given. Every
production renderer has a native curve primitive: RenderMan, Arnold, Cycles and Karma.

## Decision

moya renders RI `Curves`, linear and cubic under any basis, periodic or not, as ribbons of the
given width under both hiders. The reyes hider splits and dices a curve as it does any other
primitive, and the ray hider intersects curves in its bounding volume hierarchy.

## Alternatives

### The export turns curves into polygons
- **For**: moya needs nothing new, and the polygons render under both hiders today.
- **Against**: the RIB holds polygons instead of curves, so another renderer gets no curves. A
  ribbon cannot face the camera, and hair multiplies the primitive count.
- **Rejected because**: the RIB has to describe the scene, not one renderer's limits.

### Curves never render
- **For**: nothing to build, and most curves are construction guides.
- **Against**: wires, hair and grass can never be rendered, and a RIB from another tool with
  curves renders incomplete.
- **Rejected because**: whether a curve renders belongs to the curve, not to the renderer.

### Round tubes rather than ribbons
- **For**: a tube looks right from every side, as Arnold's thick mode and RenderMan's round
  curves do.
- **Against**: RI defines ribbons, and a tube costs more to intersect and to dice, which hair
  pays for thousands of times.
- **Rejected because**: the standard primitive comes first, and a round shape can be an
  attribute on it later.

## Consequences

- **Gains**:
  - Curves render in Render View, in batches and in any RIB moya reads.
  - Hair and fur become possible.
  - The editor's export stays faithful for other renderers.
- **Costs**:
  - A new primitive in two hiders, each with its own splitting or intersection.
  - A ribbon facing the camera looks flat close up and in reflections.
  - RI has no weighted curves, so a NURBS curve with weights renders as an approximation.
- **Revisit when**: close-up thick curves are wanted, which needs round curves, or moya takes
  NURBS curves, which needs a `NuCurves` request.
