# ADR-0222: Offline: a BSDF is a list of weighted closures

**Status**: amended
**Date**: 2026-11-11
**Tags**: offline, shaders, lighting
**Amended by**: ADR-0228
**Documented in**: [plans/MoyaMaterials.md](../../plans/MoyaMaterials.md)

## Context

OpenPBR Surface describes a surface as a stack of layers: a base, a dielectric specular layer
with transmission below it, a coat, fuzz and a thin film. MaterialX describes the same stack as a
graph of BSDF nodes joined by `layer`, `mix` and `add`. OSL's shaders return a tree of weighted
closures, and moya's library will hold hair, volume and stylised models beside OpenPBR. Kernel
code allocates nothing, so a shading point holds a bounded number of lobes on every device. A
single-scattering microfacet lobe loses energy as it grows rough.

## Decision

A shading point's BSDF is a bounded list of weighted closures, each closure one lobe of the
library. A material builds the list: `open_pbr_surface` and MaterialX's `layer` node weight each
closure by what the layers above it let through, and a sample picks a closure by its estimated
albedo over the path's wavelengths. A microfacet lobe's lost energy is restored by Kulla and
Conty's reciprocal compensation lobe, and an emitting material makes its prototype a light of
the light tree.

## Alternatives

### One fixed model of OpenPBR's parameters
- **For**: The fastest kernel, a shading point of fixed size, and layering written once by hand.
- **Against**: Every other MaterialX BSDF graph, every OSL closure and every new kind of surface
  has to be mapped onto OpenPBR's parameters or added as another fixed model.
- **Rejected because**: Materials would not render as authored, and OSL's closures would have
  nowhere to go.

### The fixed model for OpenPBR, and closures for every other graph
- **For**: The fixed model's speed for the common material, with closures for the rest.
- **Against**: OpenPBR's layering lives in two implementations that have to agree, and each
  device carries both.
- **Rejected because**: The closure list already carries OpenPBR in full, as Cycles' layered
  Principled BSDF shows.

### Turquin's scale for energy compensation
- **For**: Simpler than a compensation lobe: the single-scattering lobe is scaled by its albedo.
- **Against**: The scaled lobe is no longer reciprocal.
- **Rejected because**: Every lobe is checked for reciprocity on its own, and a compensation lobe
  keeps that check meaningful.

### Emission found by BSDF samples alone
- **For**: Nothing is added to the light tree, and a textured emission needs no estimate of its
  power.
- **Against**: A small or bright emitter is reached only by chance, so it renders with noise.
- **Rejected because**: The light tree already holds a subtree per emitting prototype.

## Consequences

- **Gains**:
  - Any MaterialX BSDF graph and any OSL closure tree renders as authored.
  - A new scattering model is a new closure, sampled and tested like every other.
  - Each lobe is reciprocal and passes a white furnace on its own.
  - An emitting material is sampled with MIS, as a mesh light is.
- **Costs**:
  - A shading point has a longest list, and a material that needs more closures loses its
    smallest ones.
  - Each shading point computes the layers' albedos to weight the closures, which a fixed model
    would fold into one evaluation.
  - Albedo-scaled layering is not reciprocal, so the layered whole is not, though each lobe is.
  - Kulla and Conty's lobe needs tables of albedo for every microfacet lobe, built with moya.
  - A textured emission needs an estimate of each triangle's power for the light tree.
- **Revisit when**: A layering model that is both reciprocal and cheap enough for every device,
  such as stochastic position-free layering, becomes practical.
