# ADR-0105: Editor: animation is scalar curves on item properties

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: [editor/Animation.md](../editor/Animation.md)

## Context

The timeline keys and plays back a scene, and later a graph editor shapes the timing of what was
keyed. An animator keys one axis without the others, shapes each with tangents, and spins an
object more than once between two keys. Placement is not the only thing that moves: a lens
zooms, a light fades, and later materials and the parameters of a modelling operation's history
change over time. glTF keys whole values with quaternion rotations, USD samples any attribute
at time codes, and RIB is written one frame at a time.

## Decision

A key drives one number. Each curve belongs to an owner, named by document id, and to one of the
numeric properties the owner's kind declares, and its keys sit on frames, each with its own
interpolation and tangents. Rotation is keyed as three Euler angles, and the document holds the
frame rate and the range.

## Alternatives

### A track of whole values per property
- **For**: glTF animation imports and exports as it is, and a quaternion rotation never locks.
- **Against**: keying one axis keys all three, a quaternion turns at most half a turn between two
  keys, and a tangent on a quaternion track is nothing an animator can see or edit.
- **Rejected because**: the editor authors animation, and its keys should take the shape that
  animators edit.

### A key that snapshots the whole placement
- **For**: setting a key is a copy, and playback blends two copies.
- **Against**: every key keys everything, there is no timing per channel, and nothing but
  placement can move.
- **Rejected because**: it gives up every part of authoring that a timeline exists for.

### Only placement animates
- **For**: three properties of one kind of value, and nothing for each kind to declare.
- **Against**: the first lens zoom or light fade needs a second design.
- **Rejected because**: a curve on a numeric property costs the same whatever the property is.

### Keys in seconds
- **For**: glTF's unit, and a change of frame rate keeps every duration.
- **Against**: the transport steps by frames, and a change of rate moves every key off its frame.
- **Rejected because**: an animator places keys on frames.

## Consequences

- **Gains**:
  - Any axis is keyed alone, any key has its own tangents, and a rotation can turn any distance.
  - A lens, an emission and later anything with a number can be keyed by the same curve.
  - One row per curve, so a changed key rewrites that curve and nothing else.
- **Costs**:
  - An item's rotation is a quaternion, so a key turns it into Euler angles, choosing the angles
    nearest the curve's value at that frame. Euler angles can lock.
  - glTF export samples the rotation curves into quaternions, and import turns quaternion tracks
    into curves.
  - Every animated property needs a declared path that a curve can name.
- **Revisit when**: animation has to be authored on something that is not a number, or keys have
  to be shared between documents.
