# ADR-0235: Offline: a bounce samples one closure alone

**Status**: accepted
**Date**: 2026-11-14
**Tags**: offline, compositing, lighting
**Documented in**: [offline/Materials.md](../../offline/Materials.md)

## Context

A light path expression splits a path's light by the kind of each bounce, so a bounce has to
belong to one lobe. A BSDF of several closures draws a direction by picking one of them. Weighing
that direction by every closure's value over the mixture's density lowers the beauty's noise, but
the weight then belongs to every lobe at once. A compositor rebuilds the beauty from channels that
split it, so those channels have to add up to it pixel by pixel.

## Decision

A bounce is weighed by the closure it picked alone: that closure's value over its chance of being
picked times its own density. The bounce's event is that closure's lobe. A light sample is split
by each closure's value towards the light, and each part is weighed against that closure's own
density.

## Alternatives

### The mixture's weight, labelled by the closure picked
- **For**: The beauty keeps the lowest noise, and nothing about sampling changes.
- **Against**: A lobe's channel holds part of the other lobes' light, by an amount that depends
  on how the lobes overlap.
- **Rejected because**: A channel that converges to the wrong light misleads a compositor.

### The mixture's weight for the beauty, and a ratio for the channels
- **For**: The beauty is unchanged, and each lobe's channel converges to its own light.
- **Against**: The path carries one more float, and the split channels add up to the beauty only
  on average.
- **Rejected because**: A rebuilt beauty that differs from the rendered one by noise breaks the
  compositor's sum.

### The first bounce's share of each lobe, as Cycles carries for its passes
- **For**: The beauty is unchanged, and splits at the first bounce are right and add up.
- **Against**: Several floats of path state, and a split at any later bounce stays wrong.
- **Rejected because**: It serves one kind of expression, not the grammar.

## Consequences

- **Gains**:
  - Each lobe's channel converges to that lobe's own light, for every expression.
  - Channels that split the beauty add up to it pixel by pixel.
  - No path state for it.
- **Costs**:
  - A surface whose lobes overlap, such as a rough coat over a diffuse base, renders with more
    noise for the same samples.
  - Every picture of the path tracer changes once, within its errors.
- **Revisit when**: A way to keep the mixture's noise and still split light exactly by lobe is
  found, such as carrying a weight for each lobe.
