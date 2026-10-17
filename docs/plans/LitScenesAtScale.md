# Lit Scenes at Scale — Instanced Draws, a Display Grade and a Panned Voice

Drafted 2026-10-16 from [the Lit Scenes at Scale roadmap](../roadmap/LitScenesAtScale.md). The
goal is a lit scene that costs a draw per distinct part rather than per object. It also covers
a colour grade that matches the strip an image editor shows, and a voice that can sound from one
side. It
covers all five of the roadmap's steps. Cascaded shadow maps stay in
[TODO.md](../TODO.md#lit-scenes), behind their own trigger.

Each step starts with a test that fails first, or records why none can be written. A step that
deletes or changes a [TODO.md](../TODO.md) entry does so when it lands.

## What the code shows

Reading the code for this plan corrected three things the roadmap assumed.

- **No app or example draws with `Lit`.** The only callers of `Lit`, `entity::meshes()` and
  `entity::casters()` are `Meshes.cpp` and the device suites `LitSceneTest.cpp` and
  `SkinTest.cpp`. A layout change migrates no app. It still changes every replacement shader
  passed through `Lit::Shaders`, and anything built against `Lit::sceneLayout()`.
- **No lit case can have a reference picture.** A committed reference may hold only output the
  Vulkan specification fixes exactly ([Testing.md](../contributing/Testing.md#realtime-references)),
  and a lit, shadowed, perspective scene is outside that. The lit cases check chosen pixels, item
  counts and a silent validation log. "The existing lit cases pass unchanged" therefore does not
  show that no picture changed. Two things stand in for a reference: a baseline of every picture
  the device suite writes, compared on the same GPU, and cases that compare two pictures drawn
  in one run.
- **The grade's output decodes as well as its input.** `grade.frag` reads the scene from an sRGB
  target, which decodes, and writes to one that encodes. A table authored for display holds
  display values, so the lookup has to encode its coordinate *and* decode its result.

Facts the plan relies on:

- **`Lit::Object` is 88 bytes**, pushed to the vertex and fragment stages: the model matrix, the
  base colour, the outline width and the first joint. `lit.glsl` declares it, and `skin.glsl`
  reads `object.firstJoint`.
- **Set 2 has three bindings**: the scene uniform at 0, the shadow map at 1, the joint palette
  at 2. `Lit` keeps one slot per frame in flight. Its palette grows by doubling, retires the old
  buffer through the ring, and rewrites the binding.
- **`lit.scene()` is called before `meshes()` and `casters()`.** It waits on the frame and
  returns the set, so a frame's buffers are free to write by the time items are submitted.
- **The recorder already passes `instances` and `firstInstance`** to `vkCmdDrawIndexed` and
  `vkCmdDraw`. Every producer sets one instance. In Vulkan, `gl_InstanceIndex` includes
  `firstInstance`, so an item can name where its objects start without a push constant.
- **The sort key has no mesh field.** It packs layer, pipeline, material and depth. `meshes()`
  sets no depth.
- **SDL3_mixer 3.2.0, the version installed, has `MIX_SetTrackStereo`**, which takes a left and
  right gain for a track, and `MIX_SetTrack3DPosition`, with a listener fixed at the origin.
- **pong is the only app with sound**, and it plays through `event::kind::Sound`, which carries a
  clip id and nothing else.

## Decisions

| ADR | Decision | Step |
|---|---|---|
| **New** | Lit: where per-object data lives. Amends ADR-0008 and ADR-0071, both of which name this as their revisit trigger | 2 |

The pan law is not an ADR. It is the behaviour of one call, stated in
[Audio.md](../api/engine/Audio.md) when step 8 lands.

## What blocks what

```
Phase 1 — instanced lit draws
1 Crowd cases and a baseline ──> 2 ADR ──> 3 Objects in a storage buffer ──> 4 Instanced draws ──> 5 Instanced casters

Phase 2 — the grade        (no order against phase 1)
7 Grade authored for display

Phase 3 — the panned voice (no order against phases 1 and 2)
8 Pan on a voice ──> 9 pong pans its sounds

6 and 10 close their phases. 11 closes the plan.
```

- Step 1 must land before step 3. Without the baseline, step 3 has nothing that would show it
  changed a picture.
- Step 7 should land after [ToolingDebt](ToolingDebt.md) step 2, which settles the grade tests'
  tolerance. If it lands first, step 7 re-reads that tolerance against its own results.

## Steps

### Phase 1 — Instanced lit draws

#### Step 1 — Crowd cases and a baseline

**Closed.** `LitCrowdTest.cpp` adds two cases for entities that share a mesh. In the first, six
entities share a model of two parts, outlined and casting shadows. Each part's top shows its own
colour where its entity stands. In the second, two strips share a skinned mesh at different points
in the bend. Together they draw exactly what each draws alone. Both cover code already in the
base. Each was checked by breaking `Meshes.cpp`: one model matrix for every item fails both,
and one first joint for every item fails the second. `cube()` and `twoCubes()` moved to
`Shapes.h` so both lit files share them.

The baseline is the 53 pictures the device suite writes to `data_out/`, kept in
`out/build/lit-baseline/`, which is not committed. Two runs on the local GPU, a Radeon, wrote
them byte for byte the same. If the baseline is lost, check out this step's commit and run the
suite to make it again.

#### Step 2 — The ADR

**Open.** The ADR records where per-object data lives and who writes it.

- **Recommended:** a read-only storage buffer at set 2, binding 3, one per frame in flight. A
  shader reads its object as `objects[gl_InstanceIndex]`, and an item names its first object
  through `firstInstance`. The push block is gone.
- **Rival: a push constant holding an index**, as ADR-0008 proposed. An instanced draw needs a
  per-instance index anyway, so for a group of instances the push constant carries nothing
  `firstInstance` does not.
- **Rival: per-instance vertex attributes.** This is the classic form, and needs no descriptor.
  A matrix takes four attribute locations, the skinned pipelines already use locations 3 and 4,
  and every replacement shader would change its vertex input as well as its sets.
- **Who writes the buffer.** `meshes()` and `casters()` take `Lit` by const reference today.
  The ADR settles whether they append objects into the frame's slot as they submit, or whether a
  walk fills a value type that the caller hands to `Lit`, as `Poses` does for palettes. Either
  way, the outline walk and the surface walk read the same objects, and the shadow walk may add
  its own.

#### Step 3 — Per-object data in a storage buffer

**Open.**

- `Lit::Object` moves into the buffer the ADR chose. It grows the way the palette does: by
  doubling, retiring the old buffer through the ring, and rewriting the binding.
- `lit.glsl` declares the buffer and an accessor for the current object. `skin.glsl` reads the
  first joint through it. Every lit shader, plain and skinned, changes together with its C++
  struct.
- Each item still draws one instance, at its own object. The item's push size is zero.
- Every lit and skin case passes unchanged, and every picture the device suite writes matches
  the step 1 baseline byte for byte. That is the test that nothing moved. Compare on the GPU that
  made the baseline.
- A new case draws more objects than the buffer starts with, across two frames in flight, and
  stays silent. It fails first against a buffer that does not grow.
- Update the comments in `Lit.h`, `lit.glsl`, `skin.glsl`, `DrawItem.h` (the 88-byte note),
  `Meshes.h` and `Poses.h`.

#### Step 4 — Instanced draws

**Open.**

- `meshes()` groups its entities by mesh entry, part and pipeline, and submits one item per
  group, with `instances` set to the group's size. A group's objects are contiguous in the
  buffer.
- Skinned entities group the same way. Each instance reads its own first joint from its object.
- An entry that does not resolve, and a skinned entity that `Poses` does not name, are still
  skipped, before grouping.
- A new case draws the step 1 crowd twice in one run. In one draw the entities share a mesh. In
  the other, each entity's mesh is the same model registered under its own key, so nothing
  groups. The pictures must match exactly, and the shared draw must have fewer items: six
  entities sharing a two-part mesh make two surface items and two outline items. The item count
  fails first against the ungrouped walk. Both draws run on one driver, so the case holds in CI.
- The step 1 cases pass unchanged.

#### Step 5 — Instanced shadow casters

**Open.**

- `casters()` groups its draws the same way, so the shadow pass costs an item per distinct part.
- `checkCasterDepths` places three blocks of one mesh, two of them casting, and checks for two
  items. After this step it expects one. The change to that count is the failing test.
- `a_skinned_caster_casts_its_pose` and the step 1 cases pass unchanged.

#### Step 6 — Close phase 1

**Open.**

- Delete the TODO.md entry "There is no instanced drawing".
- Update the documents that state the old layout:
  - [Lighting.md](../api/rendering/Lighting.md#replacing-the-lit-shaders): the binding table and
    the push constant row.
  - [Pipelines.md](../internals/realtime/Pipelines.md#descriptor-sets-and-push-constants): the
    set table, the rule that per-object data goes in push constants, the push block table and
    the shader notes.
  - [Renderers.md](../internals/realtime/Renderers.md): what a `Lit` slot holds, and the rule
    that a static item pushes `firstJoint = 0`.
  - [internals/realtime/README.md](../internals/realtime/README.md): the two rules that name
    push constants and `Lit::Object`.
- Set the ADR's status, and mark ADR-0008 and ADR-0071 as amended by it.
- Update the roadmap's "What exists" section.

### Phase 2 — The grade

#### Step 7 — A grade authored for display

**Open.**

- `grade.frag` encodes the scene's linear colour to sRGB before the lookup, and decodes the
  table's result back to linear before writing. The table keeps 16 entries a side, and the strip
  format does not change.
- The identity strip still grades within the tolerance the identity case allows.
- A new case grades known colours through a strip authored for display, such as one that halves
  each display value, and checks the display value of the result. It fails first against the
  linear lookup.
- Update [ColourAndPost.md](../api/rendering/ColourAndPost.md#grade), the comment in
  `grade.frag`, and `Grade.h`. Delete the TODO.md entry "The colour grade's table is sampled by
  linear colour".

A table larger than 16 entries a side is held. Encoding to sRGB is what puts the resolution back
in the shadows, and a larger table is due only if a strip shows banding.

### Phase 3 — The panned voice

#### Step 8 — Pan on a voice

**Open.**

- `Play` gains a `pan` from -1, left, to 1, right, with 0 the default. `Engine` gains
  `pan(voice, value)`.
- A free function converts a pan to a left and right gain. It is a balance law: the side
  being panned towards stays at full gain, and the other falls linearly to zero. A pan of 0 is
  therefore exactly an unpanned voice.
- A pan of 0 clears the track's stereo gains, so an unpanned voice is left exactly as SDL_mixer
  plays it. Any other pan calls `MIX_SetTrackStereo`. A value outside -1 to 1 is clamped.
- Headless tests cover the conversion at both ends, the centre and a clamped value. A case
  beside `audio_engine_play_without_a_device_test` covers `pan()` on an engine with no device and
  on a voice that does not exist.
- The sound itself cannot be tested without a device. It is checked by ear in step 9.
- Update [Audio.md](../api/engine/Audio.md) with the call and the pan law.

#### Step 9 — pong pans its sounds

**Open.**

- `event::kind::Sound` gains an optional pan, and `audio::Engine`'s handler passes it to `Play`.
- pong pans its paddle hit and its score by the side of the screen they happen on. The caller
  converts the position, because pong's camera decides where left is.
- pong's scene test checks the pan each event carries.
- Run pong and listen. The log stays free of validation and audio messages.

#### Step 10 — Close phase 3

**Open.** Delete the TODO.md entry "A voice has no pan". A listener with a position and distance
attenuation stays in TODO.md, rewritten to say that `MIX_SetTrack3DPosition` exists and fixes
the listener at the origin. It is due when a game needs sound in a 3D scene.

### Step 11 — Close the plan

**Open.** Confirm every entry this plan took is deleted from TODO.md, write the Outcome section,
move this plan to [completed/](completed/), and move the roadmap to
[roadmap/completed/](../roadmap/completed/).

## Open questions

- **Where the outline width lives.** `meshes()` takes one outline width for every entity, yet
  each object carries its own. Once per-object data is in a buffer, it could move to the scene
  uniform. Recommendation: leave it in the object, because a game may want a width per entity,
  and the cost is four bytes.
