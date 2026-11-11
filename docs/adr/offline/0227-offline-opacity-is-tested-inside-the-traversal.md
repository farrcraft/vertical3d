# ADR-0227: Offline: opacity is tested inside the traversal

**Status**: accepted
**Date**: 2026-11-11
**Tags**: offline, shaders
**Documented in**: [plans/MoyaMaterials.md](../../plans/MoyaMaterials.md)

## Context

OpenPBR's `geometry_opacity` cuts a surface out, and foliage stacks many cut-out cards along one
ray. Camera, indirect and shadow rays all have to pass through what is cut out. A limit on the
surfaces a ray may pass darkens dense foliage, and a shadow drawn at random through partial
opacity is noisy. Each device has its own traversal: moya's on the CPU, OptiX's with its any-hit
programs, and HIP RT's with its filters. Each device has to give the same picture on every run.

## Decision

The traversal tests a hit on geometry whose material has an opacity, by running the material's
program, and continues past a hit that does not count. A shadow ray multiplies its transmittance
by one minus the opacity and stops at zero. A camera or indirect ray keeps a hit with a
probability equal to its opacity, drawn from a hash of the sample's draw and the primitive, with
no limit on the surfaces it passes.

## Alternatives

### The integrator passes through
- **For**: The traversal is unchanged, and a transparent hit is just another hit.
- **Against**: Each layer is another traversal from the world's root, and another step of the
  path's depth, so dense foliage needs a limit that darkens it.
- **Rejected because**: Foliage is the common case, and it pays most.

### A transparent closure for camera rays, and a test in traversal for shadows
- **For**: Shadows attenuate in one traversal, and camera rays need no test in traversal.
- **Against**: Camera and indirect rays still pay a traversal and a bounce for each layer, and a
  limit on transparent bounces darkens dense foliage.
- **Rejected because**: The test that shadows need serves every other ray too.

### A shadow ray drawn through partial opacity
- **For**: One rule for every kind of ray.
- **Against**: A shadow through a half-transparent surface is noisy where attenuation is smooth.
- **Rejected because**: Attenuation is exact for a shadow and costs no more.

## Consequences

- **Gains**:
  - A ray passes any number of cut-out layers in one traversal, with no limit to darken them.
  - Shadows through partial opacity are smooth.
  - Opaque geometry costs nothing, since only geometry with an opacity is tested.
- **Costs**:
  - Every device's traversal reaches the node program, so a traversal is no longer geometry
    alone.
  - Each device has to test a primitive once per ray, which OptiX needs a flag to promise.
  - A test inside traversal runs the program at hits that a nearer hit would have hidden.
- **Revisit when**: Hardware opacity micromaps cover the cut-outs that textures make, so a test in
  traversal is needed only where they do not.
