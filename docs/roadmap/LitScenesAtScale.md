# Lit Scenes at Scale

What the lit renderer needs for a scene with many objects, a colour grade authored outside the
tree, and sound placed in the scene. Today each object is its own draw, the grade's table is
indexed by linear colour, and a voice has no position.

The gaps are each described in [TODO.md](../TODO.md#lit-scenes) and
[TODO.md](../TODO.md#audio). This roadmap orders them. Only the instancing steps depend on each
other. The grade and the panned voice are here because they are due with the same kind of scene.

## What exists

- **[`Lit`](../../api/render/realtime/vulkan/renderer/Lit.h)** draws a mesh in a cel pass and an
  outline pass. Both bind the camera at set 0, the albedo at set 1 and the scene at set 2. The
  scene set carries every joint palette drawn in the frame in one storage buffer.
- **Each object's data is a push constant block**, `Lit::Object`. It holds the object's
  transform, its base colour, its outline width and where its joint palette starts.
- **`meshes()` and `casters()` submit one draw item per part per entity.** A dozen characters and
  a few dozen props make a few hundred draws in each of the lit pass and the shadow pass.
- **The recorder can already draw instances.** Nothing above it asks for more than one.
- **[`Grade`](../../api/render/realtime/Grade.h)** looks each pixel up in a 16 by 16 by 16 table
  stored as UNORM, indexed by the scene's linear colour. A strip authored for display grades
  wrongly, and 16 entries over linear light leave little resolution in the shadows.
- **[`audio::Engine`](../../api/audio/Engine.h)** plays a voice on a named bus with a fade and a
  gain. A voice has no pan and no position.

## What it needs

### 1. Per-object data in a storage buffer

- `Lit::Object` moves from the push constant block into a storage buffer, one per frame in
  flight, written once per frame.
- The buffer is bound in the scene set at set 2, beside the joint palettes. A shader reads its
  object by `gl_InstanceIndex`.
- Every lit shader and every replacement pipeline changes its layout. That constrains every
  caller that supplies its own pipeline, so it needs an ADR.
- This step changes no picture. It is tested by the existing lit cases passing unchanged.

### 2. Instanced draws

- Draw items are grouped by mesh entry and part. Each group is one instanced draw.
- Skinned objects are instanced the same way. Each instance reads its own palette offset from the
  storage buffer.
- Sorting by key still orders groups by pipeline and material.

### 3. Instanced shadow casters

`casters()` groups its draws the same way, so the shadow pass costs a draw per distinct part
rather than per object.

### 4. A grade authored for display

The lookup coordinate is encoded to sRGB before sampling. Alternatively, the table grows beyond 16
entries a side. Either way, a strip authored in an image editor grades the way it looks there.
This has no order against steps 1 to 3.

### 5. A panned voice

- `Play` gains a pan value, and the engine a `pan(voice, value)` call, using what SDL_mixer 3
  offers for a track.
- The caller converts a position to a pan, because the mapping from the world to the speakers
  depends on the game's camera.
- A listener with a position and distance attenuation follow when a game needs sound in a 3D
  scene. This has no order against steps 1 to 4.

## The ordering

```
1 Per-object storage buffer ──> 2 Instanced draws ──> 3 Instanced casters

4 Grade authored for display   (no order)
5 Panned voice                 (no order)
```

## What this needs decided

- **Where per-object data lives**: step 1, an ADR.

## Verification

- Steps 1 to 3 must leave every lit device case unchanged. A test counts the draws recorded for a
  scene with repeated meshes.
- Step 4 is tested by grading a known colour through a strip, as the identity and inverting grade
  tests do now.
- Step 5 cannot be tested without a sound device. The conversion and the call are tested; the sound
  is checked by ear.

## Triggers

Steps 1 to 3 are due when a scene's object count, or a profile of recording time, shows the cost.
Step 4 is due when a game uses a grade authored outside the tree. Step 5 is due when a game needs
positional sound.
