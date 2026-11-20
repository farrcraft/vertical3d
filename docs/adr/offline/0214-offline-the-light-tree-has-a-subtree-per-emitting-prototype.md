# ADR-0214: Offline: the light tree has a subtree per emitting prototype

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline, lighting
**Documented in**: [offline/Lights.md](../../offline/Lights.md)

## Context

The integrator picks one light at each bounce through a tree over the lights, after Conty
Estevez and Kulla, with one random number and a bound on each node's contribution. A mesh light
emits from many triangles, and one prototype may be placed by many instances. The scene's
hierarchy already holds each prototype once, with a hierarchy over the instances above it. Light
linking gives each instance a set of lights that may light it. Distant and dome lights have no
position, so no box bounds them.

## Decision

The tree has two levels: a tree over the lights and the instances of mesh lights, and one
subtree over the triangles of each emitting prototype, shared by every instance of it. Each
unique set of linked lights has a tree of its own, sharing subtrees with the others. Distant and
dome lights are picked beside the tree, each with the same chance as the tree as a whole, and a
bounce samples one light.

## Alternatives

### One flat tree with a leaf for each emitting triangle of each instance
- **For**: The simplest tree, and each triangle is picked by its own bound, as Cycles' first
  light tree held them.
- **Against**: An instanced mesh light is copied into the tree once for each instance. Cycles
  measured its build fall from 65 seconds to under a second when it moved to subtrees.
- **Rejected because**: Its memory and build time grow with instances, which the hierarchy
  already avoids.

### A leaf for each light, with a mesh light's triangles picked by area
- **For**: The smallest tree, as pbrt-v4's light sampler holds its lights.
- **Against**: A large mesh light is sampled without regard to which of its triangles are near
  or face the shading point.
- **Rejected because**: It samples large and nearby mesh lights poorly.

### One tree, with each node's mask of the light sets it serves
- **For**: One tree for every set, and a walk that gives an unlinked child no weight.
- **Against**: A node that holds both linked and unlinked lights bounds them together, so the
  walk wastes samples on a branch whose light is mostly unlinked.
- **Rejected because**: Scenes have few unique sets, and a tree for each samples them as well
  as a scene without linking.

### Several lights for each bounce, by adaptive splitting
- **For**: A bounce near many lights of similar weight samples them all, as Conty Estevez and
  Kulla's splitting does.
- **Against**: The count of shadow rays varies by bounce, so a GPU sizes its shadow queues for
  the most a bounce can make.
- **Rejected because**: One light per bounce keeps one shadow ray per bounce on every device.

## Consequences

- **Gains**:
  - An instanced mesh light costs one subtree, however many instances place it.
  - Each triangle of a mesh light is picked by its own bound.
  - A linked light is sampled as well as an unlinked one.
  - A BSDF sample that meets a light finds its pick's density through the path from the root.
- **Costs**:
  - A walk into a subtree moves the shading point into the instance's space.
  - A scene with many unique light sets builds many top trees.
  - A dome light is picked as often as the tree, whatever their relative power.
  - A bounce near many equally bright lights is noisier than with splitting.
- **Revisit when**: scenes with many unique light sets make the top trees costly, or a measured
  scene shows the dome's fixed share wasting samples.
