# ADR-0071: Skinning — A Frame's Joint Palettes Are One Storage Buffer In The Scene Set, And An Item Names Its First Joint

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

Amends [ADR-0064](0064-a-pass-carries-a-scene-set-and-a-depth-bias.md).

## Context

A skinned vertex is moved by a weighted sum of up to four joint matrices, and each character
draws with its own palette of them, sampled once a frame
([ADR-0070](0070-animation-is-sampled-from-playback-on-the-step.md)). The palette changes per
character and per frame, which [ADR-0008](0008-binding-by-update-frequency.md)'s frequencies
place nowhere: it is not the camera, not the scene and not the material. Both lit passes have to
read it, or a skinned character casts the shadow of its bind pose. Both already bind one scene
set at set 2 for the pass (ADR-0064). A Mixamo rig is about 65 joints, which is 4 KiB of
matrices, and retcon draws about twelve characters. The item's push block is 84 of the 128 bytes
`DrawItem` carries. [SkeletalAnimation](../plans/completed/SkeletalAnimation.md) is the plan that needs
this settled.

## Decision

**Every palette drawn in a frame is written into one read-only storage buffer at set 2,
binding 2,** beside the scene uniform and the shadow map. It is written while the frame is
built and before the ring begins it, as the scene uniform already is. **An item names where
its palette starts, as a first joint in its push block**, which grows to 88 bytes. A static
item pushes zero, and its pipeline reads nothing there. The buffer is one per frame in flight,
grows by doubling, and retires an outgrown buffer through the ring
([ADR-0061](0061-a-resource-is-released-explicitly.md)).

## Alternatives Considered

### Alternative 1: A uniform buffer per character, at set 3
- **Pros**: Each palette is a block the shader indexes directly, with no offset to carry.
- **Cons**: A set per object, which is what ADR-0008 rejected, bound per item. A uniform range is
  64 KiB at most on much hardware, which is sixteen Mixamo palettes in a frame.
- **Why not**: It reintroduces a per-object bind, and runs out of room at a horde.

### Alternative 2: A dynamic uniform offset on set 2
- **Pros**: One buffer, as chosen, and no change to the push block.
- **Cons**: A dynamic offset is supplied when the set is bound, so set 2 is rebound per item,
  which breaks ADR-0064's bind once a pass. It has the same 64 KiB range limit.
- **Why not**: The offset belongs to the item, and the push block is where an item's own data
  already goes.

### Alternative 3: The palette in the push block
- **Pros**: Nothing to bind and no buffer to manage.
- **Cons**: 128 bytes hold two matrices.
- **Why not**: It does not fit.

### Alternative 4: One storage buffer at set 2, offset in the push block — **chosen**
- **Pros**: Set 2 stays bound once a pass, and both lit passes read the same matrices with no
  further binding. Any number of characters fits, up to the device's storage range. A static
  item costs four bytes of push.
- **Cons**: A storage buffer read in the vertex stage, which every Vulkan 1.3 device offers but
  which is slower than a uniform read on some. Every lit shader declares a binding that only the
  skinned ones read.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- A skinned shadow costs nothing beyond the skinned pipeline, because the shadow pass already
  binds the set.
- The push block and set 2 are where instancing would put an instance's data, if instancing
  is ever built.

### Negative
- The scene set's layout grows a binding, and the push block four bytes. A replacement shader
  declares both, per [ADR-0067](0067-lit-shaders-are-embedded-and-replaceable.md), and retcon's
  handoff says so.
- A frame's palettes are written in one place before any pass is recorded, so a character is
  sampled whether or not it ends up drawn.

### Risks
- The first joint and the palette are written by different code: the walk that poses writes the
  buffer, and the walks that draw push the offset. An offset taken from another frame's walk is
  a wrong pose rather than an error. The mitigation is that the offset travels in the value the
  posing walk returns, which the drawing walks take as an argument. A device case pins the rest
  pose, where any wrong offset shows.
