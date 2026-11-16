# ADR-0241: Offline: RI's Sides decides which faces a material emits from

**Status**: accepted
**Date**: 2026-11-15
**Tags**: offline, lighting
**Documented in**: [offline/Lights.md](../../offline/Lights.md), [offline/Rib.md](../../offline/Rib.md), [offline/Scene.md](../../offline/Scene.md)

## Context

A material that emits makes a light of the triangles it is given, and a triangle emits only from
the side its winding faces. RI's `Sides` attribute is 2 unless a scene says otherwise, and RI's
`constant` surface shades both faces of a two-sided surface. A RIB scene therefore expects a
surface that emits to be seen from either side, and one whose winding faces away renders black.
UsdGeom's `doubleSided`, which is false unless set, names the same property of a gprim, and a
UsdLux mesh light emits from the side its winding faces.

## Decision

An instance carries `doubleSided`, false unless a client sets it, and the scene client sets it
from RI's `Sides`. A material that emits from geometry a two-sided instance places emits from both
faces, and from the side its winding faces otherwise. A mesh light emits from the side its winding
faces, whatever the instance holds.

## Alternatives

### Every emitting material emits from both faces
- **For**: no attribute to carry, and a RIB scene renders as RI draws it with no request read.
- **Against**: a scene cannot ask for a one-sided emitter, which a lamp's shade or a screen needs,
  and `Sides 1` would mean nothing.
- **Rejected because**: RI and UsdGeom both name the property, so the scene can hold it.

### Keep one side and rewind the scenes that face away
- **For**: no change to the core.
- **Against**: any RIB scene with a back-facing `constant` surface renders black, unlike RI.
- **Rejected because**: the scene client's purpose is to render what RI describes.

### Sides also governs mesh lights
- **For**: one rule for everything that emits from triangles.
- **Against**: a UsdLux mesh light emits from its front, and every RIB scene that names one would
  change its light.
- **Rejected because**: a mesh light is a light, whose side its schema states.

## Consequences

- **Gains**: RI's surfaces that emit render from either side, and a scene can still ask for one
  side with `Sides 1`.
- **Costs**: a two-sided emitter is bounded by every direction in the light tree, so the tree
  picks it less well, and its power counts both faces. `Sides 1` does not hide a back face from
  the camera, which RI's renderers do.
- **Revisit when**: back faces are culled, or a client needs a two-sided mesh light.
