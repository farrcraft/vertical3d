# ADR-0088: Lit: per-object data in a storage buffer

**Status**: accepted
**Date**: 2026-10-17
**Amends**: [ADR-0008](0008-shaders-descriptor-sets-by-update-frequency.md), [ADR-0071](0071-skinning-joint-matrices-in-one-storage-buffer.md)
**Documented in**: [internals/realtime/Pipelines.md](../internals/realtime/Pipelines.md)

## Context

A lit draw pushes its object's transform, base colour, outline width and first joint as push
constants, so each object is its own draw. A scene of a few dozen props is a few hundred draws in
each of the lit pass and the shadow pass, where an instanced draw could cover every object that
shares a part. An instanced draw has one push block for all of its instances, so each
instance's data has to come from somewhere the instance index can reach. Every lit shader and
every replacement for one reads the same layout, so the place chosen binds them all.

## Decision

Each object's data is written into a read-only storage buffer in the scene set at set 2, one per
frame in flight, and a shader reads its own entry by `gl_InstanceIndex`. A draw names where its
objects start through its first instance, and pushes nothing. `Lit` owns the buffer in its frame
slot, and the walks that submit lit and shadow draws write into it as they submit.

## Alternatives

### A push constant holding an index into the buffer
- **For**: The buffer is the same, and a draw that is not instanced keeps a push block it can
  read without the instance index.
- **Against**: Every instance of a draw shares the push block, so the shader must still add the
  instance number to the index. The first instance already carries that base.
- **Rejected because**: It pushes a value the draw already supplies.

### Per-instance vertex attributes
- **For**: The usual form of instancing. It needs no descriptor, and older hardware reads
  attributes faster than storage buffers.
- **Against**: A matrix takes four attribute locations, and the skinned pipelines already use
  locations 3 and 4. Every replacement shader would change its vertex input as well as its sets.
- **Rejected because**: It spreads the object across the vertex input, where the palette beside
  it is already a storage buffer.

### A value the caller builds and passes, as palettes are
- **For**: The data flow is explicit, and a frame's objects are computed once for every pass that
  draws them.
- **Against**: Every caller builds a third value per frame and passes it to the scene and to each
  walk. An object index from another frame draws the wrong object rather than failing.
- **Rejected because**: `Lit` already keeps a slot per frame in flight, and the walks know every
  object they submit.

## Consequences

- **Gains**:
  - Entities that share a mesh part draw in one instanced draw, in the lit pass and the shadow
    pass alike.
  - An object's size is no longer bounded by the push constant budget.
  - Callers keep the same calls, except that `Lit` is passed by pointer rather than const
    reference.
- **Costs**:
  - Every lit shader, including a replacement, changes its layout and reads its object through
    the instance index.
  - The walks must be called after `scene()` for the same frame. A walk before it writes into the
    slot of a frame that may still be on the GPU.
  - The vertex and fragment stages both read a storage buffer per draw, which some devices read
    more slowly than push constants.
  - A fragment stage has no instance index, so its vertex stage passes the index down as a flat
    varying, and a replacement pair has to agree on its location.
  - The shadow walk writes its own copy of each caster's object, beside the lit walk's.
- **Revisit when**: a profile shows the storage reads cost more than the draws they save, or a
  pass other than lit needs per-object data.
