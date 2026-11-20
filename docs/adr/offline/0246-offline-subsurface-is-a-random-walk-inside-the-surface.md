# ADR-0246: Offline: subsurface is a random walk inside the surface

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, volumes
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

OpenPBR defines subsurface as a dense medium below a dielectric surface: a colour that is its
multiple-scattering albedo, a radius per channel that is its mean free path, and an anisotropy.
A renderer either walks that medium or approximates its effect by a diffusion profile, and Cycles,
Arnold and RenderMan offer both. A diffusion profile picks an exit point near the entry, which
bleeds through thin features and cannot model an anisotropic phase. A walk follows the geometry,
and a path that enters an open mesh can wander off without meeting the surface again. moya already
tracks homogeneous media and nests them, so a walk needs no machinery of its own.

## Decision

A surface whose subsurface weight is above zero holds a homogeneous medium inside it, and a path
that enters through its dielectric walks that medium by ADR-0244's tracking until it leaves. The
colour becomes a single-scattering albedo by the mapping OpenPBR's companion paper gives, and the
radius times its scale becomes a mean free path at each wavelength. A walk counts its scattering
events against a limit of its own, draws no light sample, and is absorbed at the limit.

## Alternatives

### A diffusion profile
- **For**: It converges faster on smooth skin and tolerates open meshes, as Christensen and
  Burley's profile does in Cycles and RenderMan.
- **Against**: It bleeds through thin and concave features, ignores the anisotropy, and needs
  probe rays and a profile of its own on every device.
- **Rejected because**: It approximates the medium OpenPBR defines, where a walk renders it.

### Both, chosen per material
- **For**: A user chooses speed or accuracy, as Cycles, Arnold and RenderMan let them.
- **Against**: Two systems to keep in step with OpenPBR, whose parameters name no method.
- **Rejected because**: OpenPBR gives a material no way to choose.

## Consequences

- **Gains**:
  - Subsurface renders the medium OpenPBR defines, anisotropy and thin features included.
  - A skin is a chromatic medium, tracked at four wavelengths as any medium is.
  - The walk reuses tracking and nesting, so every device gains it with them.
- **Costs**:
  - A walk is noisier on smooth skin than a profile.
  - A walk in an open mesh ends at its limit, absorbed, and darkens where it should not.
  - Light reaches a walk only through its boundary, so a light inside a subsurface object does not
    light it from within.
- **Revisit when**: OpenPBR names a method, or skin close-ups need a profile to converge.
