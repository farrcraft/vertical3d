# ADR-0071: Skinning: joint matrices in one storage buffer

**Status**: accepted
**Date**: 2026-10-03
**Amends**: [ADR-0064](0064-lighting-lit-passes-use-the-shared-recorder.md)
**Documented in**: [internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md)

## Context

A skinned vertex is moved by a weighted sum of up to four joint matrices, and each character
draws with its own set of them, called a palette, sampled once a frame
([ADR-0070](0070-animation-cpu-sampling-playback-on-the-fixed-step.md)). A palette changes per
character and per frame, which matches none of the update frequencies in
[ADR-0008](0008-shaders-descriptor-sets-by-update-frequency.md). Both the lit pass and the
shadow pass must read it, or a character casts the shadow of its rest pose, and both bind the
scene set at set 2 once per pass ([ADR-0064](0064-lighting-lit-passes-use-the-shared-recorder.md)).
A typical rig's palette is about 4 KiB, and an item's 128 bytes of push constants hold only two
matrices.

## Decision

Every palette drawn in a frame is written into one read-only storage buffer at set 2, beside the
scene uniform and the shadow map, before the frame is recorded. An item names where its palette
starts, as a first joint index in its push block, and a static item pushes zero.

## Alternatives

### A uniform buffer per character, at set 3
- **For**: Each palette is a block the shader indexes directly, with no offset to carry.
- **Against**: A descriptor set per object, bound per item, which ADR-0008 rejects. A uniform
  range is 64 KiB at most on much hardware, which holds about sixteen palettes.
- **Rejected because**: It brings back a per-object bind and runs out of room with a crowd.

### A dynamic uniform offset on set 2
- **For**: One buffer, and no change to the push block.
- **Against**: A dynamic offset is supplied when the set is bound, so set 2 would be rebound per
  item, against ADR-0064's one bind per pass. It has the same 64 KiB range limit.
- **Rejected because**: The offset belongs to the item, and the push block is where an item's
  own data goes.

## Consequences

- **Gains**:
  - A skinned shadow costs nothing beyond the skinned pipeline, because the shadow pass already
    binds set 2.
  - Any number of characters fits, up to the device's storage buffer range, and a static item
    costs four bytes of push constants.
- **Costs**:
  - The vertex stage reads a storage buffer, which every Vulkan 1.3 device supports but some
    read more slowly than a uniform buffer.
  - Every lit shader, including a replacement, declares a binding that only skinned shaders
    read.
  - Palettes are written before any pass is recorded, so a character is sampled whether or not
    it is drawn.
  - The posing code writes the buffer and the drawing code pushes the offset. An offset from
    another frame gives a wrong pose rather than an error, so the offset is passed in the value
    the posing walk returns.
- **Revisit when**: instancing is built, since per-instance data would use the same push block
  and set, or when storage reads in the vertex stage prove slow on a target device.
