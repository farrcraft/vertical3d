# ADR-0219: Editor: a placement is set in the parent's space or the scene's

**Status**: accepted
**Date**: 2026-11-10
**Tags**: editor
**Documented in**: [docs/editor/Scripting.md](../../editor/Scripting.md)

## Context

A script that builds a scene sets each item's translation, rotation and scale to values. An item
holds its placement in its parent's space, and the properties editor and the keys show it there.
A script often has the point in the scene where an item is to stand instead, such as a point
another item reaches. A parent whose scale is not the same on every axis turns a rotation
under it into a shear, which no placement of the child can undo. Blender sets an object's
location in its parent's space and its world matrix in the scene's, and Maya's `xform` takes
either by a flag.

## Decision

The placement command takes its values in the parent's space by default, or in the scene's when
asked. In the scene's space it sets the placement in the parent's space that puts the item where
the values say. It refuses a rotation or a scale in the scene's space under a parent whose scale
differs between axes, and sets nothing.

## Alternatives

### The parent's space only
- **For**: the values are the numbers the properties editor shows and the keys hold, with no
  conversion.
- **Against**: a script that places an item in the scene has to compose its parents' placements
  itself.
- **Rejected because**: placing in the scene is common in a script that builds a scene, and every
  major tool offers it.

### The scene's space only
- **For**: a script never needs to know an item's parent.
- **Against**: a value set is not the value shown or keyed, and under a parent that scales
  unevenly some values cannot be set at all.
- **Rejected because**: keyed values are in the parent's space, so a command that keys what it
  sets has to speak that space.

### An approximation under an uneven parent
- **For**: the command never refuses.
- **Against**: the item does not end where the script said, with no sign of it.
- **Rejected because**: a script that is told no can choose what to do, and one given a near
  answer cannot tell.

## Consequences

- **Gains**: a script places an item in whichever space it has values for, and what it sets in the
  parent's space is what the editor shows and keys.
- **Costs**: an argument every call may carry, and a refusal a script has to handle under a parent
  that scales unevenly.
- **Revisit when**: the editor gains shear or pivots in an item's placement, which would let more
  scene placements be held exactly.
