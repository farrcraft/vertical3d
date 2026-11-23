# ADR-0247: Offline: hair scatters by Chiang's model

**Status**: accepted
**Date**: 2026-11-16
**Tags**: offline, interchange
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

A hair fibre is thinner than a pixel, and light reflects off its cuticle, passes through it, and
reflects inside it before leaving, each with its own tint and spread. moya's curves are round or
flat, and a hit on one gives its offset across the fibre and its place along it. MaterialX defines
Chiang and others' model as `chiang_hair_bsdf`, with nodes that give its roughness and its
absorption from a colour or from melanin. Cycles adds Huang and others' near-field model, which no
MaterialX node names, and d'Eon and others' model is the more exact form that Chiang's fits. A
material reaches moya's BSDF library only as a MaterialX graph.

## Decision

A hair scatters by Chiang and others' far-field model, as a closure of the BSDF library, named by
a MaterialX `surface` node whose `bsdf` is `chiang_hair_bsdf`. The fibre's direction is the curve's
tangent, and its offset across, from -1 to 1, is the hit's place across the width, on round and
flat curves alike. On a surface that is not a curve, the model is reported once and shades with
the surface's tangent and an offset of zero.

## Alternatives

### Huang and others' near-field model
- **For**: Exact up close and on elliptical fibres, as Cycles offers it beside Chiang's.
- **Against**: No MaterialX node names it, so no client can ask for it, and it needs the fibre's
  true cross-section where moya's curves are round or flat.
- **Rejected because**: A model no client can name is a model nothing renders.

### d'Eon and others' energy-conserving model
- **For**: The most exact far-field model, of which Chiang's is a fit.
- **Against**: Costlier to sample, and nothing names it.
- **Rejected because**: Chiang's fit keeps its look and samples exactly.

## Consequences

- **Gains**:
  - Any client that writes MaterialX names a hair, with its tints, roughness and melanin.
  - pbrt-v4 and Cycles give values to check the model against.
  - The model reads what a curve's hit already gives, once the hit carries a tangent.
- **Costs**:
  - The model is far-field, so a fibre that fills many pixels is less exact than it would be
    under a near-field model.
  - The compiler gains MaterialX's `surface` node over a BSDF beside `open_pbr_surface`.
  - Curves need a tangent along the fibre, which their hits do not give today.
- **Revisit when**: MaterialX names a near-field or elliptical hair model, or close-ups of hair
  show the far-field model's error.
