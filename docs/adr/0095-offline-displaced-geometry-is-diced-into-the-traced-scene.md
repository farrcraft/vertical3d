# ADR-0095: Offline: displaced geometry is diced into the traced scene

**Status**: proposed
**Date**: 2026-10-18
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

A displacement shader moves a surface's points, and the reyes hider runs it on each grid it
dices. The ray tracer intersects triangles and spheres exactly, and does not tessellate. Both
hiders trace shadows and reflections through one traced scene, which holds every primitive as
the scene declared it. A displaced surface's shadow then follows its undisplaced shape under
either hider, and a point displaced behind that shape is shadowed by it.

## Decision

A primitive with a displacement shader is diced into grids by the reyes hider's dicing, and the
displacement shader runs on each grid. The grids' triangles go into the traced scene in place of
the primitive, so the ray hider draws them and both hiders' shadows meet the displaced surface.

## Alternatives

### The ray hider refuses displacement
- **For**: nothing new is built. Bump mapping through `calculatenormal` already works under the
  ray hider, and a report says the displacement was not run.
- **Against**: a displaced surface renders flat under the ray hider and displaced under the reyes
  hider, and its shadow is wrong under both.
- **Rejected because**: a scene would look different under the two hiders, and its shadows would
  be wrong under both.

### The ray hider bumps instead of displacing
- **For**: it runs the displacement shader for its normal only, at each hit, with the stencil the
  derivatives already use. It needs no new geometry.
- **Against**: the silhouette and the shadow keep the undisplaced shape, so a ripple shades as a
  ripple but has a straight edge and a flat shadow.
- **Rejected because**: the shadow is still wrong, and the two hiders still draw different
  silhouettes.

## Consequences

- **Gains**: the same displaced geometry is in both hiders' shadows and reflections, and the ray
  hider draws its silhouette. The dicing and the displacement already exist, and the bounding
  volume hierarchy keeps the extra triangles cheap to trace.
- **Costs**: a displaced primitive is held as two triangles per micropolygon, which costs memory
  in proportion to its size on screen. The triangles are flat between grid points, so a fine
  displacement needs a fine grid. The displacement runs once for the traced scene, so a shader
  that varies with time or with the ray does not move with it.
- **Revisit when**: displaced geometry's memory shows in a scene, or a displacement must vary per
  ray, which only tessellating on demand inside the tracer would give.
