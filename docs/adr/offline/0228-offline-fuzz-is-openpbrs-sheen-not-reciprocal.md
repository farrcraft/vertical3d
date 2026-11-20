# ADR-0228: Offline: fuzz is OpenPBR's sheen, not reciprocal

**Status**: accepted
**Date**: 2026-11-12
**Tags**: offline, shaders, lighting
**Amends**: ADR-0222
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

OpenPBR Surface's fuzz is a layer of fibres over the coat, and OpenPBR names Zeltner, Burley and
Chiang's sheen for it, as MaterialX's `sheen_bsdf` does for OpenPBR. That lobe is a linearly
transformed cosine fitted to the outgoing direction alone, so swapping its two directions changes
its value. ADR-0222 checks every lobe of the closure list for reciprocity on its own. A
reciprocal sheen exists, Estevez and Kulla's, which Arnold's Standard Surface uses.

## Decision

moya's fuzz closure is Zeltner's sheen, with the fits MaterialX gives, sampled exactly from its
transformed cosine. It is the one lobe of the library that is not reciprocal, and its own checks
are chi-squared and its closed-form albedo in place of reciprocity.

## Alternatives

### Estevez and Kulla's sheen
- **For**: Reciprocal, so every lobe keeps ADR-0222's check, and well tested in production.
- **Against**: Its albedo has no closed form, so the layering needs another table, and OpenPBR's
  fuzz looks different under it from what OpenPBR's authors and MaterialX render.
- **Rejected because**: An OpenPBR material is meant to render as its authors and MaterialX's
  reference render it.

## Consequences

- **Gains**:
  - OpenPBR's fuzz renders as OpenPBR and MaterialX specify it.
  - Its albedo is exact, so a sample's weight is the albedo the layering scales by.
  - It samples exactly, with no table.
- **Costs**:
  - A bidirectional integrator would see a fuzzed surface differently from each end.
  - Its fits hold only from a roughness of 0.01, and their albedo is held to [0, 1].
- **Revisit when**: OpenPBR names a reciprocal fuzz, or moya gains a bidirectional integrator.
