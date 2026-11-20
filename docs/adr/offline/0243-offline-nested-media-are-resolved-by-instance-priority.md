# ADR-0243: Offline: nested media are resolved by instance priority

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes, interchange
**Documented in**: [offline/Volumes.md](../../offline/Volumes.md)

## Context

A path that enters a closed surface enters the medium its material gives, and a dielectric bends
light by the ratio of the indices on its two sides. Closed surfaces nest and overlap: a liquid in
a glass shares its outer face with the glass's inner one, and modellers push the liquid into the
wall so that no gap shows. Neither RIB nor USD says which medium lies on each side of a surface.
RenderMan sets an intersection priority on each shape, Arnold and Octane set one on the material,
and Cycles keeps a stack of media without one. Kernel code allocates nothing, so whatever a path
remembers of the media it is inside has a fixed size.

## Decision

A path carries a stack of the closed surfaces it is inside, of a fixed depth, and each surface's
instance has a priority. A hit on a surface of lower priority than the path's medium is passed as
no bounce, and a dielectric bends light by the indices of the winning media on its two sides.
Equal priorities let the medium entered last win, and a volume that is not bounded by a surface
stays out of the stack and adds its density to whatever medium the path is in.

## Alternatives

### A stack without priorities
- **For**: The simplest rule, and what Cycles ships: a path pushes a medium as it enters a surface
  and pops it as it leaves.
- **Against**: Where a liquid meets glass, the path bends by the liquid against air rather than
  against glass, unless the modeller leaves a gap. Overlapping surfaces give a medium that depends
  on the order the faces are met.
- **Rejected because**: A glass of liquid is the common case of nesting, and it comes out wrong.

### Media named on each surface
- **For**: Exact, with nothing inferred, as pbrt-v4's `MediumInterface` names the medium on each
  side.
- **Against**: No client can fill it, since RIB and USD do not say what each surface separates.
  It works against ADR-0242, which gives a medium to the material inside a closed surface.
- **Rejected because**: Every client would have to work out what the renderer can resolve.

### Priority on the material
- **For**: Arnold and Octane set it there, beside the index it governs.
- **Against**: One glass material cannot sit at two priorities, and RIB's attribute state and
  RenderMan's own attribute place it on the shape.
- **Rejected because**: Priority is a property of how objects overlap, which the instance holds.

## Consequences

- **Gains**:
  - A glass of liquid bends light correctly at every interface, with overlapping geometry
    resolved by the priority.
  - A scene that sets no priority behaves as a plain stack, so nothing has to be set to start.
  - RenderMan's intersection priority maps onto the instance directly.
- **Costs**:
  - The traversal's filter gains a rule that passes surfaces of lower priority, and a shadow ray
    carries a copy of the stack.
  - A path nested deeper than the stack holds loses its medium of lowest priority.
  - A camera inside a medium finds its media by tracing out at the start of each path.
  - Arnold's priority on a material maps to each instance that names the material.
- **Revisit when**: USD adopts a schema of priority or of media on surfaces, or a scene needs
  nesting deeper than the stack holds.
