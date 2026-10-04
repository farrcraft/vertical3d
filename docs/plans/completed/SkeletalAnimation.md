# Skeletal Animation — A Model In Parts, A Skin, Clips On The Step, And A Palette At Set 2

Drafted 2026-10-03 against `e5423af`, with milestones 3 and 4's work in the tree and
uncommitted, and closed the same day. Eight steps across `api/type`, `api/asset`, `api/ecs`, `api/render` and the
documents, and an eighth held, taking up [milestone 5](../../roadmap/completed/m5-SkeletalAnimation.md) of
[the game engine roadmap](../../roadmap/completed/GameEngine.md). Every step is in this tree. retcon is the
consumer this is written for and adopts it after it ships. It is neither changed nor run here.

The roadmap lists four sections in order: reading a skeleton, clips and a pose, skinning in the
lit pass, and instancing. The surveys keep that order and change three things about it. **The
first section is two steps.** Splitting a file by material is the same walk over the file as
reading a skin, but it changes what a registry entry is. A character needs parts before it needs
a skin, because a skinned character with two surfaces is one pose drawn twice. **The clock is
written here.** [MotionAndQueries](MotionAndQueries.md#step-6--a-sprite-clip) held the
sprite clip until a second consumer wanted its clock, and this sampler is that consumer.
**Instancing is held.** Nothing in either game draws enough characters for it to pay, and the
reasoning is at [step 8](#step-8--instancing-held).

## What the surveys found

retcon was read at `3d22935`, which is still its HEAD, and it pins this tree at `13a9557`. This
tree was read in its working state.

**In retcon:**

* **There is no rigged asset.** Neither `hero.glb` nor `zombie.glb`, the seven props, the test
  fixture, or the two source exports in `art/meshy/` contains a `skins` or an `animations` key.
  The export path removes them on purpose:
  * the runbook's export table says "Animation | off (nothing rigged yet)";
  * `tools/clean_model.py` sets `export_animations`, `export_skins` and `export_morph` to false.
* **The rig is to be Mixamo's.** `docs/art-production.md` names Mixamo as the primary animation
  source, rigged in Blender to a Mixamo-compatible skeleton, with Cascadeur for custom moves.
  It also says a clip is authored once on the 3D rig and the renderer handles orientation. No
  joint count, clip naming or skeleton naming is written down anywhere. A Mixamo skeleton is
  about 65 joints.
* **The clip lists are named, not timed.** Each hero needs idle, walk, run, attack, take damage,
  die and crouch. Each enemy needs idle, patrol, alert, attack, death and a special. Morph-target
  faces are "beyond current" scope.
* **A move is a teleport, and nothing chooses a clip.** `Encounter::syncTransforms()` writes a
  `Transform` from the grid tile and an eight-way `Facing`. The state a clip selector would read
  is already there as components: `Health`, `Downed`, `AwarenessLevel`, `AiIntent`, and the results
  of `resolveMove()` and `resolveAttack()`. retcon has no state machine.
* **The counts are small.** Four heroes at most, by its own design rule. Each map's encounter is
  four heroes, four enemies and a four-unit horde, so twelve characters. The horde's density is
  marked "Draft", with no number. A crowd unit's budget is 10k triangles, and every mesh is drawn
  twice, hull and surface.

**In this tree:**

* **The loader ignores nodes.** `Gltf::load` walks `data->meshes` and never `data->nodes`, so a
  mesh placed by a node's transform loads at its own origin. Every fixture and every retcon file
  so far is one mesh at the identity, so nothing has shown it. A skin's joints *are* nodes, so
  the hierarchy has to be read whatever else this plan does.
* **A model has one material by construction.** `type::Model` holds one `Material`, and
  `asset::kind::Model` holds one embedded image. `MeshRegistry::Entry` is one mesh, one material
  and one base colour, and `meshes()` submits one item per entity.
* **`type::Model` has one consumer.** Apart from its own tests, it is read by `MeshRegistry`
  alone. The offline renderers do not read it, so changing its shape moves the loader, the
  registry and the asset suite, and nothing else.
* **The recorder already draws instances.** `DrawItem` carries `instances` and `firstInstance`,
  and `Recorder` passes both to `vkCmdDrawIndexed`. The builder takes a vertex input rate.
  Nothing submits more than one instance.
* **Set 2 is bound once per pass, and both lit passes bind the same set.** Anything added to the
  scene set reaches the shadow pass and the cel pass alike. That is what keeps a skinned
  character from casting the shadow of its bind pose.
* **The push block has room.** `Lit::Object` is 84 bytes of `DrawItem::pushCapacity`'s 128.
* **`mesh.vert` inverts a matrix per vertex** for its normal matrix. That is retcon's shader too.
  A skinned vertex adds a weighted sum of four matrices to it, and neither is a cost at twelve
  characters.
* **ADR-0060's interpolation takes any type with an `interpolate()` beside it.** A playback state
  of a clip and a time qualifies once it has one.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--a-model-in-parts) | A model is parts over one vertex array, read through the node hierarchy | `api/type`, `api/asset` | **0069** | done; accepted |
| [2](#step-2--the-registry-draws-parts) | A registry entry is parts, and the walks submit one item per part | `api/render` | 0069 | done |
| [3](#step-3--reading-a-skin) | Skeletons, per-vertex influences, and a fixture that has them | `api/type`, `api/asset` | 0069 | done |
| [4](#step-4--the-record-where-animation-lives) | The record: where animation lives, and what is sampled when | `docs/adr` | **[0070](../../adr/0070-animation-is-sampled-from-playback-on-the-step.md)** | done; accepted |
| [5](#step-5--clips-a-clock-and-a-pose) | The clock, clips, sampling, blending and a palette | `api/type`, `api/asset` | 0070 | done |
| [6](#step-6--playback-on-the-step) | A playback component, advanced in `simulate()` and drawn between steps | `api/ecs` | 0070, 0060 | done |
| [7](#step-7--skinning-in-the-lit-tier) | The palette at set 2, the skinned shaders, and the walk that poses | `api/render` | **[0071](../../adr/0071-joint-palettes-are-a-storage-buffer-in-the-scene-set.md)** | done; accepted |
| [8](#step-8--instancing-held) | Instancing | — | — | held; in [TODO.md](../../TODO.md#lit-scenes) |
| [9](#step-9--a-figure-that-moves-and-the-handoff) | A rigged figure drawn mid-clip, and the handoff | `api/asset`, `api/render`, `docs` | — | done |

Steps 1 and 4 can be written in either order, and each records something its next step is
written against. Step 2 needs step 1, and step 3 needs step 1. Step 5 needs steps 3 and 4.
Step 6 needs step 5. Step 7 needs steps 2, 5 and 6. Step 9 needs everything.

---

### Step 1 — A model in parts

**ADR-0069: a model is parts that share one vertex array, and may carry a skin.** It amends
[ADR-0030](../../adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md), which deferred
this exact case rather than rejecting it: "Splitting a file into several `Model`s is the natural
extension". The record goes the other way from that sentence, and says why.

**A file whose parts need different surfaces is one model with several parts, not several
models.** A part is an index range and a material:

```cpp
struct Part final {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t material;   // into materials()
};
```

The vertex array and the index run stay merged, so a model is still one upload. `material()`
becomes `materials()`, and a model the loader reads always has at least one part.

**The alternative the record weighs is several models**, which ADR-0030 foresaw. A static prop
loses nothing by it. A character does: two models are two registry entries, and a skin shared
between them would need a pose written once and read by both. One model gives every part the
same joints and the same palette, which is what a skinned character is. A static prop would
need a split by material for one reason only, so the record keeps one answer for both.

**The skin half of the record** says a model may carry a `Skeleton` and an influence per
vertex, in an array beside `vertices()` that a static model leaves empty. It weighs the
roadmap's two layouts:

* **The same `Vertex` with joint attributes added.** It costs every static vertex in both games
  24 bytes it never reads, and it changes the 32-byte layout `MeshRegistry` asserts and retcon's
  vertex copies.
* **A second, parallel array — chosen.** A static model is unchanged to the byte. The device
  layout of a skinned vertex is the registry's to interleave at upload, because a vertex buffer's
  layout is the pipeline's (ADR-0030 already says so).

**The loader walks the scene, not the mesh list.** It reads the default scene's node hierarchy
(or the first scene, or every root when the file names none). Each mesh instance is placed into
the merge by its node's world matrix, with normals by its inverse transpose. That fixes the
survey's finding. A mesh named by two nodes is merged twice, as it is drawn twice. Primitives
that share a material join one part, in file order.

**`asset::kind::Model` carries an image per material**, rather than one. Where the file
embedded a material's image, it arrives decoded, and the image's place in the list is the
material's.

**Tests.** `make_model_fixture.py` gains a second fixture, `two_surfaces.glb`:

* two meshes under one parent node;
* each child is translated by a different whole number;
* the parent has a 90° turn;
* the two meshes name different materials, and a third primitive shares the first mesh's
  material.

Its cases assert:

* two parts, with their index ranges, materials and base colours;
* the positions placed by the parent's turn and each child's translation, exactly, since a 90°
  turn of whole numbers is exact in float;
* normals turned by the same matrix;
* a file with no scene still loads every root.

The existing `three_primitives.glb` cases are unchanged: one mesh at the identity, one material,
one part.

**State: done; ADR-0069 accepted**, and ADR-0030's header says it is amended. All 25 suites
pass, cpplint is clean, and so is a whole-tree pass of `out/build/verify`. Five new cases: the
two parts with their ranges and materials; each part indexing its own vertices; the normals
turned; the sceneless file reading the same; and a one-material file being one part. With the
node's matrix taken out of the merge, nine positions are wrong and the parts case fails. Three
things came out differently:

* **The fixtures have a script of their own**, `make_parts_fixture.py`, rather than a second
  output from `make_model_fixture.py`. The two files share nothing but the GLB packing.
* **`MeshRegistry` draws a model with its first material until step 2**, which is what it did
  before this step. A model built in code now names a material and a part, so the lit and
  registry device cases each gained two lines.
* **The placed positions are compared exactly, and they are exact.** The parent's turn is a
  matrix in the file rather than a quaternion, because a quaternion would put sin 45° in every
  term.

### Step 2 — The registry draws parts

`MeshRegistry::Entry` becomes the mesh and a list of parts. Each part holds its index range, its
albedo's texture and material handles, and its base colour. Albedo de-duplication by path is
unchanged, and is now per part. `meshes()` and `casters()` submit one item per part, using
`DrawItem::firstIndex` and `indices`, which the recorder already honours.

**retcon's handoff needs one line**: `Entry::material` and `Entry::baseColour` move onto a part.
Nothing else in the tier's surface changes.

**Tests.** The existing registry and lit cases carry the rewrite: their models are one part. Two
new device cases:

* **`two_surfaces.glb` is registered and drawn**, and the frame is silent. A pixel inside each
  part's footprint is that part's base colour in its lit band, which the existing cube case
  already uses to assert a colour.
* **Releasing an entry releases each part's albedo** when no other entry names it. This extends
  the existing shared-albedo case.

**State: done.** All 25 suites pass, cpplint is clean, and so is a whole-tree pass of
`out/build/verify`. ADR-0069 was accepted when this step began, rather than at step 3 as
drafted. Five new device cases:

* `two_surfaces.glb` is one entry of two parts, with their ranges and base colours;
* a part naming an image draws it while its sibling draws white, and the image goes with the
  entry;
* two parts naming one image share it, and it outlives the entry while another names it;
* a part reaching past the indices or the materials, and a model with no parts, are refused,
  with nothing registered;
* one model of a red cube and a green one draws the lit band of both colours. With a part's
  range ignored, every draw covers both cubes and the green pixels are none.

Three things came out differently:

* **The drawn case is a model built in code, not `two_surfaces.glb`.** The fixture's triangles
  face along z with normals along y, which is right for the loader's arithmetic and meaningless
  to light. Two cubes in one model say the same thing about the walk, and every pixel means
  something.
* **`add()` takes an image per material**, by the material's index, as `kind::Model` holds them,
  rather than one image. Nothing in the tree passed one.
* **A part outside its model is refused at registration**, which ADR-0069's risk asked of the
  loader's suite. A part built in code is outside that suite.

### Step 3 — Reading a skin

**The shape**, in `api/type` beside `Model`, glm only:

```cpp
struct Skeleton final {
    struct Joint final {
        std::string name;
        int32_t parent;              // -1 for a root; always before its children
        glm::vec3 translation;       // the joint's rest pose, local to its parent
        glm::quat rotation;
        glm::vec3 scale;
        glm::mat4 inverseBind;
    };
    std::vector<Joint> joints;       // in an order where a parent precedes its children
};

struct Influence final {
    glm::u16vec4 joints;             // into Skeleton::joints
    glm::vec4 weights;               // sums to one
};
```

`Model` gains `skeleton()` (empty for a static model) and `influences()`, which is parallel to
`vertices()` or empty.

**Reading.**

* `JOINTS_0` and `WEIGHTS_0` are read in any of the component types the specification allows,
  and the weights are normalised. A `JOINTS_1` set is reported and dropped, because four
  influences is the layout.
* The skin's joints are re-ordered so that a parent precedes its children, which is what lets a
  pose be made global in one pass. Each primitive's joint indices are remapped into that order.
* A file with more than one skin keeps the first and reports the rest.
* **A skinned mesh is not placed by its node's transform**, as the specification requires. Its
  joints place it. Static meshes in the same file are still placed by their nodes, per step 1.

**The fixture.** No rigged asset exists in either tree, so the suite's is generated:
`make_skin_fixture.py` writes `bending_strip.glb`.

* It is a strip of quads along +Y, with three joints in a chain at y = 0, 1 and 2.
* Each joint's translation is a whole number and its rotation is the identity, so every inverse
  bind matrix is an exact negated translation.
* Each vertex is weighted wholly to one joint, or half and half where two segments meet, so every
  weight is exact.

Step 5 adds its clip. Step 7's bind-pose case depends on every value here being exact.

**Tests**, headless:

* joint count, names, parents and inverse bind matrices;
* a file whose joints are listed child first comes back parent first, with its influences
  remapped (the fixture script writes a second, shuffled file for this);
* the influences of chosen vertices;
* weights read as unsigned bytes are normalised;
* a model with no skin has neither a skeleton nor influences.

**State: done.** All 25 suites pass, cpplint is clean, and so is a whole-tree pass of
`out/build/verify`. Six new cases, which are the ones drafted plus one for the mesh beside the
skin. With the joint remap taken out, the shuffled file's influences name the skin's indices
rather than the skeleton's, and its case fails. The plain file's case cannot catch that,
because its skin is already parent first. Four things came out differently:

* **A skeleton has a root matrix.** Whatever stands above the root joint, such as an armature's
  scale, is kept as `Skeleton::root`, since a clip that animates the root joint overwrites
  that joint's own transform. A Blender export of a Mixamo rig puts a scale of a hundredth
  there. The shuffled fixture stands its armature at z = 4 to pin it.
* **An unskinned mesh in a skinned file follows a joint.** Every vertex of a skinned model needs
  an influence, so a mesh with none, such as a weapon parented to a hand, is weighted wholly to
  the nearest joint above its node. It is moved into the space that joint's skinning matrix
  expects, so it stands where the file put it while the joint is at rest. A mesh under no joint
  follows the first root. The plain fixture's `tip` pins this.
* **A vertex whose weights sum to nothing follows the first root**, and is reported, as is a
  second set of influences and a mesh bound to another skin.
* **`Skeleton` has a header of its own**, and `Influence` is nested in `Model` beside `Vertex`.

### Step 4 — The record: where animation lives

**ADR-0070: the data and its sampling are `api/type`'s, playback is a component on the step, and
which clip plays is the game's.** It goes in as `proposed` and is accepted when step 7 draws
through it.

* **`api/type` holds the clock, the clip, the pose, sampling and blending.** All of them are glm
  and arithmetic. They are tested the way `Ray` is, and they are readable by an offline renderer
  that wants a frame of an animated model
  ([ADR-0024](../../adr/0024-api-type-serves-both-renderers.md)). The clock goes here too, and the
  sprite clip in [TODO.md](../../TODO.md#sprite-sheets) is written over it when it is taken up. Its
  region names are what would take it elsewhere, and a clock has none.
* **Playback is a component in `api/ecs`**, advanced in `simulate()`, and has an `interpolate()`
  so that [ADR-0060](../../adr/0060-a-moving-thing-keeps-its-previous-step.md) draws it between
  steps.
* **What is interpolated is the playback state, not the pose.** That state is a clip, a time and
  a fade. The pose is sampled once a frame, at draw time, from the interpolated state. This
  avoids two other designs. One keeps a pose per step, which is a `Previous` of 65 matrices per
  character, and slerping matrices is not a thing. The other samples twice and blends, which
  doubles the sampling for the same picture.
* **Which clip plays, and when it gives way, is the game's.** The api offers `play(clip, fade)`
  and nothing that chooses. The roadmap left this open until a game had written a state machine,
  and none has. retcon's selectors are its own components, so a graph of states written here
  would be guessing its edges.

**Alternatives the record weighs:**

* **A library of its own, `api/animation`.** It would be one more manifest entry under
  [ADR-0033](../../adr/0033-a-consumer-selects-the-api-libraries-it-wants.md) for code that has no
  dependency `api/type` and `api/ecs` do not already have.
* **Sampling on the device.** That means a compute pass writing the palette. It is fast at a
  thousand characters and pointless at twelve, and it would put clip data in buffers.
* **The pose sampled in `simulate()` and stored per step.** That is the first design the record
  avoids, above.

### Step 5 — Clips, a clock and a pose

**The shape**, in `api/type/animation/` (namespace `v3d::type::animation`):

* **`Clock`** is the time-keeping MotionAndQueries described. It holds a duration and whether it
  loops or clamps. `advance(time, step)` returns the new time and which marker times the step
  crossed, including the end of a clamped clip and every wrap of a looping one. It holds no
  state of its own, so playback and the sprite clip can both own their time.
* **`Clip`** is a name, a duration, and channels. A channel is a joint, a path (translation,
  rotation or scale), an interpolation (`STEP`, `LINEAR` or `CUBICSPLINE`), its key times, and
  its values. Rotation under `LINEAR` is a shortest-path slerp, which the specification requires.
* **`Pose`** holds a translation, a rotation and a scale per joint, in the skeleton's order.
  * `rest(skeleton)` gives the rest pose.
  * `sample(clip, time, skeleton, pose)` overwrites the joints the clip animates and leaves the
    rest.
  * `blend(a, b, weight)` mixes two poses, lerping translations and scales and slerping
    rotations.
  * `palette(skeleton, pose, out)` makes each joint global in one pass, which works because
    parents come first. It then multiplies by the inverse bind, giving the matrices a vertex is
    skinned by.

**The loader reads `animations`** into `Model::clips()`, by name, keeping only channels whose
target is a joint of the skin it kept. A channel on another node is reported and dropped, since
a node that is not a joint has nowhere to go in a pose. A channel on `weights`, which is a morph
target, is dropped silently: the roadmap leaves morph targets out.

**`bending_strip.glb` gains two clips.** `bend` turns the middle joint 90° about Z over one
second, linearly, with keys at 0 and 1. `step` holds the same turn under `STEP` interpolation.

**Tests**, headless and by hand, the way `Ray` is:

* **sampling `bend`:**
  * at 0 it gives the rest pose;
  * at 1 the middle joint is a quarter turn, and the top joint's global position is (-1, 1, 0)
    to within a float epsilon;
  * at 0.5 it is an eighth turn, and nothing else moves;
* **sampling `step`:** at 0.99 it gives the first key, and at 1 the second;
* **a cubic channel** gives the key values at its keys and, at a midpoint, the value the Hermite
  basis gives. The fixture script writes one cubic channel for this;
* **the palette:** at the rest pose, every matrix is the identity, exactly, because of the
  fixture's whole numbers. This is the property step 7's bind-pose case rests on;
* **blending:** half of `bend` at 1 and the rest pose is an eighth turn. A weight of 0 or 1 gives
  either pose exactly;
* **the clock:**
  * a looping clock wraps and reports the wrap;
  * a clamped clock stops at its duration and reports the end once;
  * a step that crosses a marker reports it;
  * a step longer than the clip reports every wrap it crossed.

**State: done; ADR-0070 accepted** when this step began, rather than at step 7 as drafted. All
25 suites pass, cpplint is clean, and so is a whole-tree pass of `out/build/verify`. There are
ten cases in a new `animation_test` in the type suite, built by hand against the strip's
skeleton, and three in the asset suite against the file. The asset cases check that the clips
are read, that the tip's channel is dropped, that a static model and an unanimated skin have no
clips, and that the file's bend swings the top joint to (-1, 1, 0) end to end. With the inverse
bind left out of the palette, the rest palette is not the identity and its case fails. Four
things came out differently:

* **`Clock` reports markers with `crossed(from, to, marker)`**, a count, rather than returning a
  list from `advance`. A marker belongs to whoever plays the clip, and a stateless count is
  what lets a sprite clip and a skeletal one share the clock.
* **`sample` takes no skeleton.** The pose it overwrites is already the skeleton's size, and a
  channel naming a joint the pose does not have is skipped.
* **The cubic fixture's tangents point opposite ways.** With both along +x, the Hermite terms
  cancel at the midpoint and give the linear answer, which would not tell cubic from linear.
* **The skeleton's root is in the palette**, which step 3 added. Its own case moves the root to
  z = 4 and finds the top joint there.

### Step 6 — Playback on the step

**The shape**, `ecs::component::Playback`:

```cpp
struct Playback final {
    uint32_t clip;          // into the model's clips()
    float time;             // unwrapped, so that a step across a loop interpolates forwards
    float duration;
    bool loops;
    uint32_t from;          // the clip being faded out of, when fading
    float fromTime;
    float fade;             // 0 to 1, how far the fade has gone; 1 when not fading
    float fadeDuration;
};

void play(Playback& playback, uint32_t clip, float duration, bool loops, float fade);
void advance(entt::registry& registry, float step);   // every Playback, called from simulate()
Playback interpolate(const Playback& from, const Playback& to, float alpha);
```

The clip's duration and looping are copied in by `play()`, so `advance` needs nothing but the
component, and `api/ecs` needs no registry of clips. A game calls `ecs::snapshot<Playback>`
beside its `snapshot<Transform>`.

**Time is kept unwrapped, and wrapped when it is sampled.** A step from 0.95 to 1.05 then
interpolates through 1.0 rather than backwards through 0.5. `interpolate()` across a `play()`
that changed the clip draws the current state: there is no meaningful halfway between two clips
except the fade, and the fade has its own weight. The step decides whether that is a snap or the
fade's own first frame, and a case pins whichever it is.

**Tests**, in the ecs suite:

* advancing by a step moves the time by the step;
* a clamped clip stops;
* a fade's weight moves from 0 to 1 over its duration, and then `from` is dropped;
* interpolating across a wrap moves forwards;
* interpolating across a `play()` gives the current clip;
* `snapshot` and `interpolated` work for `Playback` as they do for `Transform`, which the
  `Interpolable` concept checks at compile time.

**State: done.** All 25 suites pass, cpplint is clean, and so is a whole-tree pass of
`out/build/verify`. There are twelve cases in a new `playback_test` in the ecs suite. With the
unroll taken out of `interpolate()`, a rebased loop is drawn 2048 seconds away and its case
fails. Five things came out differently:

* **`api/ecs` links `api/type`**, in its CMake and in the manifest, for the clock. The library
  had needed only glm and EnTT.
* **The faded clip carries its own duration and looping**, as `fromDuration` and `fromLoops`,
  because it keeps playing while it fades out. The draft held only its time.
* **`play()` of the clip already playing does nothing**, so a game can call it every step from
  its selector without restarting the clip. Nothing playing is `Playback::none`, which draws
  the rest pose.
* **A long loop is rebased, which the draft only mentioned as ADR-0070's risk.** Past 4096
  seconds the time drops its whole loops. `interpolate()` and `crossed()` move the earlier
  step back by whole loops when the later one is smaller, so the step that rebased is still
  drawn forwards and still passes its markers.
* **`crossed(previous, current, marker)` is on the component**, so a game asks it rather than
  building a `Clock`. A step that changed clip passes no marker.

How a fade that ended within the step is drawn was left for this step to decide, and it draws
the clip alone. The weight it skips is under one step's worth of the fade.

### Step 7 — Skinning in the lit tier

**ADR-0071: a frame's joint palettes are a storage buffer in the scene set, and an item names
its offset.** It amends [ADR-0064](../../adr/0064-a-pass-carries-a-scene-set-and-a-depth-bias.md)
and cites [ADR-0008](../../adr/0008-binding-by-update-frequency.md). The roadmap left where the
palette binds to this step.

* **Set 2 binding 2 is a read-only storage buffer of every palette drawn this frame**, written
  once before the ring begins the frame, as `Lit::scene()` already is. Both lit passes bind that
  set, so the shadow pass is skinned by the same matrices as the cel pass, with nothing more to
  bind.
* **The push block gains the item's first joint**, a `uint`, which brings it to 88 bytes. A
  static item pushes zero and its pipeline never reads it.
* **Alternatives:**
  * **A uniform buffer per character at set 3.** That is a set per object, which ADR-0008
    rejected. 65 joints is 4 KiB, so sixteen characters would also exhaust a 64 KiB uniform
    range.
  * **A dynamic uniform offset on set 2.** It is bound per item, which breaks ADR-0064's bind
    once per pass, and it has the same size limit.
  * **Palettes in push constants.** Two joints fit.

**The shape.**

* **The registry uploads a skinned model interleaved**: `Model::Vertex` followed by its
  `Influence`, at 56 bytes. A second set of `static_assert`s pins that layout. The entry keeps
  the model's skeleton and clips on the cpu, behind a shared pointer, since every entity drawing
  it samples them. `MeshRegistry::clip(handle, name)` turns a clip's name into its index.
* **`Lit` gains three skinned pipelines**, `skinnedCel()`, `skinnedOutline()` and
  `skinnedShadow()`. They use the same layout, and three vertex shaders that include a
  `skin.glsl` beside `lit.glsl`. `Lit::Shaders` gains three entries, so ADR-0067's replacement
  still covers everything. The fragment stages are shared.
* **The palette buffer is per frame in flight.** It grows by doubling, and an outgrown buffer
  goes through `Ring::retire` ([ADR-0061](../../adr/0061-a-resource-is-released-explicitly.md)).
  `Lit::scene()` gains the palette: a span of matrices that it copies and binds.
* **`realtime::poses(registry, alpha, meshes)`** walks
  `view<const Transform, const component::Mesh, const ecs::component::Playback>()`. For each
  entity it:
  * samples the interpolated playback, with the fade;
  * appends that entity's palette to one frame-wide array;
  * returns the array and each entity's first joint, as a `Poses` value.

  A mesh with a skin and no `Playback` is drawn at its rest pose.
* **`meshes()` and `casters()` take the `Poses`.** They choose the skinned pipeline for an entry
  with a skin, and push the entity's first joint.

**Tests.** Headless:

* `poses()` gives consecutive offsets in the walk's order;
* an entity whose handle was released is skipped;
* a skinned mesh with no `Playback` gets the rest pose.

On the device:

* **At its rest pose, a skin draws what the unskinned mesh draws.** The rest palette is exactly
  the identity (step 5), and every weight is exact, so `bending_strip.glb` drawn skinned and drawn
  with its skin stripped are byte for byte the same picture on one driver. This compares two
  pictures, not a picture against a reference. It is the roadmap's identity check, and it fails
  on a wrong offset, a wrong joint remap, a wrong stride, or weights read from the wrong
  attribute.
* **A caster is skinned in the shadow pass.** A quad weighted wholly to one joint, and drawn
  through `skinnedShadow()` with that joint translated by a quarter in depth, reads back the
  depth LitScene's step 8 case reads with the quad moved by its `Transform`. Constant-depth
  planes stay exact. This is what fails if the shadow pass draws the bind pose.
* **`bend` at half a second is drawn and silent**, with `VK_LAYER_VALIDATE_SYNC=1` once locally,
  and written to `data_out/skinned_bend.png` for a person to look at.
* **A skinned mesh released while a frame draws it keeps the frame silent**, as the static case
  does.

**State: done; ADR-0071 accepted** when this step began, and ADR-0064's header says it is
amended. All 25 suites pass, cpplint is clean, and so is a whole-tree pass of
`out/build/verify`. The whole device suite is silent with `VK_LAYER_VALIDATE_SYNC=1`. There are
six cases in a new `skin_test`:

* two entities on one skin are posed end to end, and a static one is not posed;
* an entity with no playback, or playing nothing, stands at rest, where the palette is the
  identity, and a released one is not posed;
* **at rest, a skin draws what its unskinned mesh draws**, byte for byte over 1008 covered
  pixels on the Radeon. With the skinned stride set to the static one, 5752 pixels differ;
* the strip bent half way through `bend` is silent, and moves 446 pixels from the strip at rest;
* a skinned caster's depth is exactly three eighths, where its bind pose would read a quarter;
* a skinned mesh released in flight keeps the frame silent.

Five things came out differently:

* **The headless cases need a device.** `poses()` reads a `MeshRegistry`, and a registry needs a
  device, so all six cases are in the device suite.
* **The skinned caster is a model built in code**, a square on one joint whose rest pose stands
  half a unit along z. It needs no clip, and a shadow drawn in the bind pose reads a different
  exact depth.
* **`meshes()` and `casters()` take the `Poses` as a last argument that defaults to none.** A
  skinned entity they are not given a pose for is skipped, so every existing caller compiles
  unchanged and draws what it drew.
* **`Poses` keeps its starts in a vector of pairs**, not a map, because clang-tidy holds a
  returned value's move to not throwing and MSVC's maps allocate when moved. That is the same
  finding [LargeWorlds](LargeWorlds.md) recorded.
* **The palette buffer starts at 256 matrices** per frame in flight and doubles. An outgrown
  one is retired through the ring, although the slot's frame has already finished.

### Step 8 — Instancing, held

**Held, and here is why.** The roadmap puts instancing last so that skinning can say what an
instance carries. By this step it has: a model matrix, a base colour and a first joint, which is
`Lit::Object` as it stands. What instancing would change is where those live. They would move
from the push block into a per-frame storage buffer that `gl_InstanceIndex` reads, and
`meshes()` would group entities by entry and part. The recorder already draws instances, so the
change is the walk, the shaders and one more binding in set 2.

**What it buys is draw calls, and there are not enough of them to matter.** retcon draws twelve
characters and a few dozen props, each drawn two or three times, which is a few hundred draws a
frame. The cost per draw that instancing removes is recording a push and an indexed draw. No
measurement here shows that cost, and this tree has no profiler to take one with, which is
[milestone 7](../../roadmap/completed/m7-ShellAndShipping.md)'s. A storage buffer of objects would also replace
a push block retcon is about to adopt, before anything has asked for the change.

**The trigger is a count.** That might be retcon setting the horde density its own documents
leave as "Draft", a township population, or a profile that shows recording time. At that point
this step is drafted against the walk step 7 wrote, and the binding it adds is set 2's next. It
moves to [TODO.md](../../TODO.md) with that trigger when this plan closes.

### Step 9 — A figure that moves, and the handoff

**A real rig, drawn mid-clip.** `bending_strip.glb` proves the arithmetic and says nothing about
what an exporter writes. The case `a_rigged_figure_is_drawn_mid_clip` loads one rigged glTF
produced by real tools. It plays its first clip and draws three frames, at the start, a third
and two thirds through. It asserts silence, and that the three pictures differ. It writes each to
`data_out/` for a person to look at.

**The asset** is one of the Khronos glTF sample models: `RiggedFigure`, or `CesiumMan` if a
textured one is wanted. It is committed under `api/render/tests/data/` beside its licence and
attribution, because both are CC BY 4.0. Which one, and whether a CC BY file is acceptable in
the tree, is decided when the step is taken up. If neither is, a Blender export of the fixture
strip stands in, which still proves the exporter's conventions.

**State: done, with the Blender export.** A CC BY file was not wanted in the tree. All 25
suites pass, cpplint is clean, and so is a whole-tree pass of `out/build/verify`. The device
suite is silent with `VK_LAYER_VALIDATE_SYNC=1`.

`make_blender_fixture.py` runs inside Blender 5.2 and writes `blender_strip.glb`:

* the strip and its three bones;
* the armature scaled by a hundredth over bones a hundred units long, as a Mixamo rig arrives;
* two actions, each pushed to its own NLA track so that the exporter writes both as clips.

Three cases read it:

* **the asset suite reads the rig.** The joints come out parent first, the hundredth is the
  skeleton's root, and the joints stand a hundred units apart. The palette at rest is the
  identity to 1e-4. That shows the exporter's inverse bind matrices, its rest pose and the root
  agree with this tree's arithmetic, which a hand-written file cannot show. It also finds both
  clips by name;
* **the asset suite samples the bend at its end**, and top stands at (0, 1, 1). That is a
  quarter turn about glTF's x, which the exporter made of Blender's;
* **`an_exported_rig_is_drawn_mid_clip`** draws the start, a third and two thirds through the
  bend, silently. Each frame moves 150 and then 486 pixels from the one before, and goes to
  `data_out/blender_bend_*.png`.

What Blender writes that the hand-written fixtures do not:

* **Every action has a channel for every path of every joint.** Each is two `STEP` keys, except
  the bone that moves, whose rotation is sampled a key a frame as `LINEAR`.
* **A clip starts at its first frame.** Blender keys from frame 1, so `bend` runs from 1/24 s
  to 25/24 s. A clip's duration here is its last key, so the first 1/24 s holds the first key.
* **Joints are unsigned bytes, and the nodes are written child first**, while the skin lists
  them parent first.

The case was drafted as `a_rigged_figure_is_drawn_mid_clip` and is named for what it draws.

#### The handoff, for retcon when it adopts

Written here for retcon to read, not sent to it. The counterparts:

| retcon needs | here |
|---|---|
| a file whose surfaces differ | one `type::Model` in parts, drawn a part at a time ([ADR-0069](../../adr/0069-a-model-is-parts-over-one-array-and-may-carry-a-skin.md)) |
| a skeleton and weights | `type::Skeleton` and `Model::influences()`, read from the first skin |
| clips by name | `Model::clips()`, and `MeshRegistry::clip(handle, name)` for the index |
| playing, fading, footsteps | `ecs::component::Playback`: `play()`, `advance(registry, step)`, `crossed()` ([ADR-0070](../../adr/0070-animation-is-sampled-from-playback-on-the-step.md)) |
| the pose drawn and cast | `realtime::poses()`, handed to `Lit::scene()`, `meshes()` and `casters()` ([ADR-0071](../../adr/0071-joint-palettes-are-a-storage-buffer-in-the-scene-set.md)) |

What adopting involves:

* **The export settings change.** `clean_model.py` and the runbook turn skins and animations on,
  and keep four influences a vertex, which is Blender's default limit. An action is exported
  as a clip when it is on its own NLA track, as `make_blender_fixture.py` does.
* **`GltfLoader` copies each part**, and the influences when there are any. Its copy of a
  skinned vertex becomes 56 bytes, or it stops copying and takes the registry's entry.
* **Playback is a component.** `play()` is called from retcon's own selectors (awareness, intent,
  the results of a move) as often as it likes, since asking for the clip already playing does
  nothing. `snapshot<Playback>` is called beside `snapshot<Transform>`.
* **A move that is a teleport now has somewhere to go.** `syncTransforms()` writing a tile's
  position each step makes a figure walk between tiles only if the position is interpolated
  across the move. That is retcon's own Phase 8 problem, its ADR-0026, and not this tier's.
* **The push block is 88 bytes and the scene set has a third binding.** A shader of retcon's
  that replaces one of `Lit`'s declares `Object::firstJoint` and the palette at set 2, binding 2,
  per ADR-0067. The skinned ones include `skin.glsl`'s block.
* **A Mixamo rig's hundredth scale is the skeleton's root.** It needs no applying in Blender, and
  `blender_strip.glb` is the case that shows it comes through.

---

## Sequence

**Steps 1 and 2 first.** They change no behaviour for the one-part model every existing case
and every retcon file is. They close the node-placement gap the survey found, and they settle
the registry's shape before anything skinned depends on it.

**Step 3, then the record, then step 5.** Reading a skin is headless and is where most of the
risk is, as the roadmap says. The record is written once the data it describes exists and
before the code that samples it.

**Step 6, then step 7.** Step 7 is the plan's midpoint: if the scene set cannot take the
palette, it shows here, and the plan stops to rethink ADR-0071 rather than pressing on to a real
rig.

**Step 9 last.** Its case can be started as soon as step 7 draws, and the asset question is
answered then.

## Verification

Per [sdlc.md](../../sdlc.md), every step that changes code: `ninja -C out/build/x64-Debug`,
`ctest`, cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all
of them, so every finding is the step's. Step 7 moves what both lit passes bind, so it also runs
the device suite once with `VK_LAYER_VALIDATE_SYNC=1`.

**What can be pinned is pinned:**

* every value read in steps 1 and 3, and every sample, palette and blend in step 5, against
  numbers computed by hand from fixtures built to be exact;
* the bind-pose picture against the unskinned picture, byte for byte on one driver, and on
  lavapipe in CI;
* the skinned shadow's depth.

**What cannot be pinned is silence on two drivers and a picture a person looks at**: a bent
strip, and a real figure mid-clip. No new reference picture is blessed. The bind-pose case
compares two pictures, which needs none. Nothing here is verified in another repository, and
retcon's posed capture, which the roadmap names as the acceptance test, is retcon's to take once
it has a rigged character.

## What this does not do

* **It does not choose clips.** ADR-0070 records why a state machine is the game's.
* **It does not instance.** Step 8 says why, and what would bring it back.
* **It does not read morph targets.** Nothing in either game asks for a face, and retcon's art
  production calls them out of scope.
* **It does not do inverse kinematics, ragdolls or root motion.** That is the roadmap's list, and
  neither game has asked.
* **It does not attach a weapon to a hand.** retcon's content list asks whether a held weapon
  shows. A joint's global matrix is in the palette's arithmetic, so an attachment is a
  `Transform` composed with one. That is a function when a game asks for it, not a step here.
* **It does not animate the editor's meshes.**

## When a step lands

Update the state in the table above.

* **Step 1** adds the ADR index row for 0069 as `proposed`, and amends ADR-0030's header. It
  replaces [TODO.md](../../TODO.md#models)'s sentence that splitting a file by material is milestone
  5's. The plans index says this plan is open.
* **Step 2** updates [RenderingPipeline.md](../../RenderingPipeline.md)'s account of the mesh
  registry and the walk.
* **Step 4** adds ADR-0070 as `proposed`.
* **Step 5** rewrites TODO.md's sprite-clip entry: the clock it waited for now exists, and what
  remains is the regions.
* **Step 6** adds `Playback` to [ECSDesign.md](../../ECSDesign.md) beside `Transform`.
* **Step 7** adds ADR-0071, accepts 0070 and 0071, and amends ADR-0064's header. It adds the
  palette to RenderingPipeline.md's binding table and lit-pass section, and adds `skin.glsl` to
  [Build.md](../../Build.md#shaders).
* **Step 9** adds the new device cases to [Testing.md](../../Testing.md), and the licence of any
  committed asset to wherever the tree records third-party files.
* **When the plan closes**, [m5](../../roadmap/completed/m5-SkeletalAnimation.md) moves to
  `roadmap/completed/` and points here as done. Instancing moves to TODO.md with its trigger,
  the roadmap's table row says so, and this file moves to [completed/]().
