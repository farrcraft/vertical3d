# Skeletal Animation

Milestone 5 of [the game engine roadmap](GameEngine.md). Characters that move by bending rather
than by sliding: skins and clips read from glTF, a skinned vertex in the lit pass, a pose sampled
and blended on the cpu, and enough instancing that a crowd of them is affordable. **This exists
in neither tree.** It is the largest gap either game has, and it is retcon's.

It waits on [milestone 4](m4-LitScene.md), because a skinned vertex is a vertex layout, a pipeline
and a variant of the lit pass, and those have to exist somewhere before skinning can be written
against them. It shares its time-keeping with [milestone 1](completed/m1-MotionAndQueries.md#a-sprite-clip)'s
sprite clip.

## What exists

* **glTF is read here, and stops at a static mesh.** `asset::loader::Gltf` reads a file into a
  `type::Model` whose vertex is a position, a normal and a uv
  ([Model.h](../../api/type/Model.h)). Joints, weights, skins, nodes and animations are not
  read. Only the first material in a file is kept, because a
  merge is one draw, so a file whose parts need different surfaces has to become several models
  and nothing splits one.
* **retcon parses through it.** Its `GltfLoader` is a conversion from the api's model into its
  own vertex layout (its ADR-0020, amended), so a loader that reads skins here is one retcon
  already calls.
* **retcon's art is waiting for it.** Its content list asks each hero for idle, walk, run,
  attack, take damage, die and crouch, and each zombie for idle, patrol, alert, attack and death
  (`docs/game/content/3d-models.md`). Its ADR-0020 chose glTF partly because it carries
  skinning, and expected the engine work to follow. Its handoff notes say that nothing is
  animated, a move is a teleport, and real-time play makes that more visible rather than less
  true.
* **Nothing draws many of one mesh.** The lit pass retcon has, and that milestone 4 moves, draws
  an item per entity. A horde is retcon's own escalation rule, and it is many of the same few
  meshes.

## What it needs

In order.

### 1. Reading a skeleton

The node hierarchy, skins (joints and inverse bind matrices), and per-vertex joints and weights,
from the same loader. A model whose file has no skin loads as it does today. This is also where
the first-material rule is replaced by splitting a file into a model per material, because a
character is rarely one surface and the split is the same walk over the file.

Whether a skinned vertex is a second `type::Model` layout or the same layout with joint
attributes that a static mesh leaves empty is
[ADR-0030](../adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md)'s to amend,
and the cost of the second is four bytes of joints and sixteen of weights on every static
vertex in both games.

### 2. Clips, and a pose

A clip is a set of channels, each a joint's translation, rotation or scale keyed in time;
sampling one at a time gives a pose, and a pose is a matrix per joint. Blending two poses is
what makes a walk become a run without a pop, and crossfading between clips is the blend over
time.

**The time-keeping is milestone 1's.** Advancing a clip on the fixed step, looping, clamping and
reporting an event at a named time are what the sprite clip already does; a skeletal clip is
the same clock over continuous tracks rather than a list of regions. Interpolating a pose by
`alpha()` between two steps is [milestone 1's interpolation](completed/m1-MotionAndQueries.md#interpolation)
applied to joints instead of a transform.

### 3. Skinning in the lit pass

A variant of milestone 4's mesh pipeline that reads joints and weights and a palette of joint
matrices per draw. The palette is per instance and per frame, which by
[ADR-0008](../adr/0008-binding-by-update-frequency.md)'s rule makes it neither the camera nor
the material, so where it binds is the step's to settle — and the shadow pass needs the same
variant, or a skinned character casts the shadow of its bind pose.

### 4. Instancing

One draw for many instances of one mesh, each with its own transform and, when skinned, its own
offset into the joint palettes. This is what makes a horde or a township's population a cost
proportional to the number of meshes rather than the number of characters. It is last because it
changes how milestone 4's pass submits, and that is easier to judge once skinning has said what
an instance carries.

## What this needs decided

* **What an animation state machine is, and whether it is the api's.** Which clip plays, and
  when it gives way to another, is a game's rule — retcon's turn-based moves and its real-time
  horde want different ones. What the api owns is sampling and blending. A graph of states is
  the line to draw, and it is better drawn after one game has written one.
* **Morph targets.** glTF carries them and retcon's ADR-0020 names them beside skinning. Nothing
  in either game asks for a face, so they are left out until something does.

## Verification

Reading is headless and is where most of the risk is: a model of a few joints with a known pose
at known times, sampled and compared against matrices computed by hand, the way `Ray` is tested.
Blending and crossfading are the same at the pose level.

Skinning on the device cannot be pinned to a picture
([ADR-0054](../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)) except for
the case that is the identity — a skin at its bind pose must draw the picture the unskinned mesh
draws, which is a strong check that the palette and the weights are wired correctly. Beyond
that, retcon's reference capture with a character posed at a fixed time is the acceptance test,
as it is for [milestone 4](m4-LitScene.md#verification).

## Not in this milestone

* **Inverse kinematics, ragdolls and root motion.** Each is a game asking for more than clips,
  and neither game has asked.
* **Animating the editor's meshes.** The editor models with `brep::BRep` and has no use for a
  skin.
