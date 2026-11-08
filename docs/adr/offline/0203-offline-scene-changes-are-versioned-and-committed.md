# ADR-0203: Offline: scene changes are versioned and committed

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, editor
**Documented in**: [offline/Scene.md](../../offline/Scene.md)

## Context

Several structures are built from moya's scene: the BVH, the light tree, the compiled materials,
and a copy of each on every device. Each rebuilds only what a change touched, and they may rebuild
at different times. The editor sends edits while a render samples, and a drag can move many
objects at once. Cycles marks each changed socket and clears the marks in one sync of every
structure, and Hydra keeps dirty bits on each prim with a version for the scene beside them.
Riley takes edits only while no render runs, Arnold stops a render to take them, and Embree
applies a batch on commit.

## Decision

Every change a client makes is gathered, and a commit applies the batch as one new version of the
scene. Each object and each of its parameters is stamped with the version that last changed it,
and a consumer that keeps the version it last built from asks for what is newer. A render takes
a commit between passes, never during one.

## Alternatives

### Dirty flags on each parameter, cleared when the renderer syncs
- **For**: A bit per parameter, and Cycles has shown it works in a production renderer.
- **Against**: The flags hold one consumer's view. A structure that rebuilds later, on a thread
  of its own or on a second device, finds them cleared by another. The one sync has to rebuild
  everything in a fixed order.
- **Rejected because**: moya's structures rebuild independently, and each needs its own view of
  what changed.

### Each change applied as it arrives, under a lock
- **For**: An edit is visible at once, with no commit to call.
- **Against**: A pass would see half of a drag, and every reader of the scene takes the lock.
  A drag of many objects restarts sampling once per object.
- **Rejected because**: A pass must see one consistent scene, and an edit of many objects is one
  change.

## Consequences

- **Gains**:
  - Each consumer rebuilds from exactly what changed since it last built, whenever it builds.
  - A pass always sees a whole commit, and the editor's drag restarts sampling once.
  - Adding a consumer, such as a second device, changes no other consumer.
- **Costs**:
  - A version per parameter takes more memory than a bit.
  - A client must commit, or its changes are never rendered.
  - A deleted object's version is kept until every consumer has seen its deletion.
- **Revisit when**: the scene has one consumer that rebuilds everything in one place, or edits
  must reach a render within a pass.
