# Renderable Component — A Transform, A Sprite, And Who Walks Them

Drafted 2026-10-03 against `e5423af`, and closed the same day. Six steps across `api/ecs` and
`api/render`, taking up
[milestone 3](../../roadmap/completed/m3-RenderableComponent.md) of
[the game engine roadmap](../../roadmap/GameEngine.md). Every step is in this tree. cozy and retcon
are evidence for the shape and adopt it after it ships; neither is changed or run here.

The milestone is a decision, and most of this plan is the evidence for it. The code it adds is
small: one component in `api/ecs`, one in `api/render`, and a function that walks the second.
It exists because [milestone 4](../../roadmap/completed/m4-LitScene.md)'s mesh pass walks entities, and what
it walks for has to be settled before that pass is written to retcon's shape by default.

## What the surveys found

Both games were read at the commits the roadmap names. cozy is at `3217bc2`, with M5 step 5
uncommitted in its working tree, and retcon is at `3d22935`. Both are pinned to this tree's
`13a9557`, which predates [`Previous<T>`](../../../api/ecs/Previous.h).

* **retcon's answer is two components and nothing else**, by its ADR-0027. `Transform` is
  `position`, `yaw` and `scale` with a `matrix()`. `MeshRenderer` is a `MeshHandle` and
  `castsShadow`. The material lives on the mesh registry's entry rather than on the entity, so
  there is no material handle anywhere. `SceneRenderer` walks
  `view<const Transform, const MeshRenderer>()` three times a frame, for shadow, outline and cel.
  It draws in storage order, culls nothing and interpolates nothing, because every move is a
  teleport. `Transform` is derived: `Encounter::syncTransforms` writes it from the tile and the
  facing and never reads it back.
* **retcon rejected parenting** in its ADR-0038. An item is a value, not an entity parented to
  whoever holds it, because a pocket has no place in the world. Neither game's roadmap plans an
  attachment or a rotation other than yaw.
* **cozy has no appearance component at all.** `Position` is `glm::vec3 feet` on the XZ ground
  plane (its ADR-0001), and `Facing` is a direction in (x, z) kept for a walk cycle that has no
  art yet. `drawWorld` reads the player's `Position` by entity id and emits exactly two sprites.
  One is the player's marker. The other is an acorn at a constant position, which is not an
  entity. `drawSprite` billboards on the camera profile's `right()` and `up()`. It anchors at the
  feet, bottom-centre, and sizes from a height the caller gives and the region's aspect. Nothing
  sorts and nothing interpolates.
* **No app in this tree draws a world quad.** pong, odyssey and tetris each read one known
  entity by id and draw it themselves. [`Movement`](../../../odyssey/system/Movement.cpp) is the
  only `view<>` walk in any app, and it simulates rather than draws. So cozy is the only consumer
  that can prove the sprite half, and the roadmap asks for exactly that before the record is
  accepted.
* **This tree has no mesh handle.** [`Handle.h`](../../../api/render/realtime/Handle.h) defines
  pipeline, material and texture handles. Meshes are the app's
  ([ADR-0010](../../adr/0010-meshes-are-owned-by-the-app.md)), and the registry that would give a
  mesh a handle is milestone 4's second section. A mesh component can therefore be shaped here
  and built only there.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record) | The record, drafted as proposed | `docs/adr` | **0063** | done; accepted |
| [2](#step-2--a-transform) | `ecs::component::Transform`, with an `interpolate` | `api/ecs` | 0063 | done |
| [3](#step-3--a-sprite-and-the-walk-that-draws-it) | `realtime::component::Sprite`, and a walk that draws every sprite into one canvas | `api/render` | 0063 | done |
| [4](#step-4--cozys-scene-reproduced-here) | cozy's two sprites reproduced in a test through steps 2 and 3, before the record is accepted | `api/render` | 0063 | done |
| [5](#step-5--retcons-components-checked-against-it) | retcon's `Transform` and `MeshRenderer` checked against the record, from the survey | — | 0063 | done |
| [6](#step-6--the-mesh-component-handed-to-milestone-4) | The mesh component's shape handed to milestone 4 | `docs/roadmap` | 0063 | done |

Step 1 comes first. Steps 2 and 3 are in order, because the walk reads the transform. Step 4
needs both, and the record is accepted only after it. Steps 5 and 6 depend on the record's text
and nothing else.

---

### Step 1 — The record

**ADR-0063: what an entity carries to be drawn.** It answers the roadmap's four questions
together, as the roadmap asks. The record goes in as `proposed` and stays that way until step 4.

**Where a thing stands: a `Transform` in `api/ecs`, holding a position, a quaternion and a
scale.** The roadmap names the cost of a full rotation as a quaternion per entity that neither
game reads. The recommendation is to pay it, for two reasons.

- **Yaw does not interpolate correctly as a float.** A unit turning from 170° to -170° crosses
  20 degrees. Lerping the angle sweeps it 340 degrees the other way instead. Since
  [ADR-0060](../../adr/0060-a-moving-thing-keeps-its-previous-step.md), anything drawn between
  steps is interpolated through an `interpolate` beside its type. With yaw, that function would
  need to know the angle wraps; a slerp does not. retcon only gets away with a float because it
  teleports.
- **A component's representation is the expensive thing to change later.** Every game that
  reads `Transform` breaks when it changes. Milestone 5's skinned characters and anything that
  ever pitches would force the change. A quaternion costs twelve bytes more than a float per
  entity. Hundreds of entities is the scale either game reaches.

retcon's reason for a scalar is that combat can write a placement from a tile without matrices.
A free function `aboutY(radians)` returning the quaternion keeps that true, and is the one
change `syncTransforms` would need.

**No parent.** Neither game has one, and retcon recorded a reason against the one case it
considered. The trigger is the first attachment, which is likeliest to be milestone 5 holding
something in a hand. When that comes, a parent is a component of its own that the walk resolves,
so `Transform` does not change shape for it.

**What a thing looks like: a component per kind of drawing, living beside what it names.** A
sprite is a component of its own, not a mesh whose mesh is a quad. The roadmap points at the
reason: cozy's sprites are one stream of quads cut by texture, and an item per sprite is the
opposite of that batching. The mesh component is retcon's `MeshRenderer` with its name
generalised, and keeps the material on the registry entry where retcon has it.

Both live in `api/render/realtime/component/`, not in `api/ecs`. Each names a handle that
`api/render` defines, and `api/ecs` links glm and EnTT and nothing else, so putting them there
would take a library with no device dependency onto one with every device dependency.
`Transform` is the other way round: simulation code writes it, and a headless test of a game's
rules should not link Vulkan to place a unit.

**Who turns them into draw items: the api.** The roadmap says a walk in the api is what makes
the component worth having there at all. The walk is a function called from `render()`, not an
`ecs::System`. A system is `simulate(float)` on the fixed step
([ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md)), and drawing is once per frame
with `alpha()`.

**Interpolation: the walk reads `interpolated<Transform>`.** An entity with a
`Previous<Transform>` is drawn between steps. One without is drawn where it is, which is
ADR-0060 unchanged. A game opts in by calling `snapshot<Transform>` at the top of `simulate()`.

**What the record cites rather than restates:** ADR-0042 for the order being the caller's,
ADR-0060 for the previous step, and ADR-0010, which milestone 4's registry amends rather than
this record.

### Step 2 — A transform

**The shape**, in [`api/ecs/component/`](../../../api/ecs/component/) beside `Position2D`. It is a
struct rather than a class with accessors, because a game writes its fields directly, and that
is how both games have written theirs:

```cpp
struct Transform final {
    glm::vec3 position{0.0f};      // where the thing stands - a sprite's feet, a mesh's origin
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const;      // translate * rotate * scale, retcon's order
};

glm::quat aboutY(float radians);
Transform interpolate(const Transform& from, const Transform& to, float alpha);
```

`interpolate` mixes position and scale and slerps the rotation. Nothing in the tree's apps
moves to it, because pong is 1D and 2D and odyssey is a tile position. `Position1D`,
`Position2D` and `PositionFixed2D` stay as they are.

**Tests**, in a new `TransformTest.cpp` in [`api/ecs/tests`](../../../api/ecs/tests/):

- `matrix()` takes a point in the order translate, rotate, scale, checked against a scaled,
  quarter-turned and moved unit vector;
- `aboutY` turns +Z towards +X for a positive angle, which pins the hand against
  [ADR-0052](../../adr/0052-a-consumer-names-the-camera-hand.md)'s default rather than leaving it to
  whoever reads the sign first;
- `interpolate` at 0, 1 and one half, and at one half between 170° and -170° about Y gives 180°,
  not 0°. That case fails a lerped angle;
- `interpolated<Transform>` compiles. `PreviousTest` already covers the mechanism, so this only
  shows that `Transform` satisfies `Interpolable`.

### Step 3 — A sprite and the walk that draws it

**The shape**, in `api/render/realtime/component/Sprite.h` and a free function beside
[`DepthOrder`](../../../api/render/realtime/DepthOrder.h):

```cpp
struct Sprite final {
    TextureHandle texture;
    glm::vec2 uv0;                 // the region's top-left, as SpriteSheets::uv hands it out
    glm::vec2 uv1;
    glm::vec2 size;                // world units, width by height
    glm::vec4 tint{1.0f};
};

/**
 * Every entity with a Transform and a Sprite, as an upright quad facing the camera with its
 * bottom edge centred on the transform's position, keyed along depthAxis and added to order.
 **/
void sprites(const entt::registry& registry, float alpha, const glm::vec3& right,
    const glm::vec3& up, const glm::vec3& depthAxis, DepthOrder* order);
```

**The component holds what was resolved, not a name.** cozy calls `sheet.uv(name)` per sprite
per frame today. A component holding a region name would carry a string per entity and repeat
the lookup on every frame. The game resolves once, and again on hot reload, where it already
re-resolves its texture. The sprite clip held in [TODO.md](../../TODO.md#sprite-sheets) would
write `uv0` and `uv1` per step when it lands, so this field is that feature's target as well.

**The key is `dot(position, depthAxis)`, larger meaning further.** The order stays the caller's
(ADR-0042), so the caller chooses the axis. For cozy it is the camera's forward projected onto
the ground, which is the key [LargeWorlds' handoff](LargeWorlds.md#step-4--world-quads-in-depth-order)
already gave it. For a perspective camera it is the view direction. An axis rather than a
callback keeps the walk free of a call per sprite.

**The walk does not rotate the quad by the transform's rotation**, because an upright billboard
faces the camera whatever way the thing faces. A game chooses a facing's frames by writing the
uvs, which is what cozy's `Facing` is for. Scale multiplies `size`. A sprite lying flat on the
ground is cozy's `Backdrop` and stays its own code. The trigger for a flat sprite is a second
case.

**Linking.** `v3dlib_render` gains `v3dlib_ecs` as a public dependency, per
[Build.md](../../Build.md#linking-rules). It already links EnTT, and `v3dlib_ecs` adds glm, which it
also already has.

**Tests**, in a new `SpriteTest.cpp` beside
[`DepthOrderTest.cpp`](../../../api/render/tests/DepthOrderTest.cpp), headless like the canvas:

- one sprite at the origin with right = +X and up = +Y gives cozy's four corners in cozy's order,
  `feet ∓ across + rise` then `feet ± across`;
- two entities sharing a texture come out as one batch, and two keyed apart come out
  furthest-first;
- an entity with a `Previous<Transform>` is drawn at the blended position at alpha one half, and
  one without is drawn at its current one;
- an entity with a `Sprite` and no `Transform` is not drawn, and is not an error.

### Step 4 — cozy's scene, reproduced here

**This is the prototype the roadmap asks for before the record is accepted,** because cozy's
sprites are the case "a mesh or a quad" does not obviously fit. It is built in this tree, and
cozy takes the feature up once it has shipped, as every earlier round has reached it.

A case in `SpriteTest.cpp` builds cozy's frame from the survey above as entities. It uses
cozy's `isometric` camera profile at 45° elevation, a player whose sprite is 1.6 high at its
feet, and an acorn 0.9 high at (2.6, 0, -2.2), both on one texture. It asserts:

- two quads in one batch, which is what cozy logs today;
- each quad's corners equal what cozy's `drawSprite` computes from the same feet, height and
  basis (`feet ∓ across + rise`, `feet ± across`), worked by hand in the test rather than read
  from cozy;
- keyed along the ground-projected forward, the player at the origin comes first. From cozy's
  eye at (10, 14.142, -10) the acorn is nearer the camera, which the draft of this step had
  backwards.

If writing it needs anything the record does not give, the record changes before it is
accepted. That is the point of doing this before acceptance rather than after. A pixel
comparison is not part of it: a sprite is filtered and blended, which
[ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md) does not allow
a reference to hold, and the geometry is what the record decides.

**Then ADR-0063 is accepted.**

**The handoff note.** For cozy, when it takes this up: a world sprite is an entity carrying a
[`v3d::ecs::component::Transform`](../../../api/ecs/component/Transform.h) whose position is its
feet, and a [`realtime::component::Sprite`](../../../api/render/realtime/component/Sprite.h)
holding the sheet's texture, the region's `uv0` and `uv1` from `SpriteSheets::uv`, and its size
in world units: `drawSprite`'s height, and the width it computes from the region's aspect. One
call, `realtime::sprites(registry_, alpha(), profile.right(), profile.up(), ground, &order)`,
replaces every `drawSprite` call. `ground` is the camera's direction with y zeroed and
normalised, which is ADR-0001's key. Then `order.into(&spriteQuads_)` as before. The corners
are the ones `drawSprite` builds, and a sheet shared by every sprite stays one batch. `Position`
can keep its meaning and write `Transform::position` each step, or be replaced by it. Calling
`snapshot<Transform>` at the top of `simulate()` draws the player between steps
([ADR-0063](../../adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md)).

### Step 5 — retcon's components checked against it

No code. The roadmap's test is that retcon's `MeshRenderer` and `Transform` could be replaced by
the api's without `SceneRenderer` changing shape. This step checks the record against the
survey above rather than against retcon's tree:

- `transform.matrix()` is unchanged at every call site;
- `syncTransforms` writes `rotation = aboutY(facingYaw(facing))` in place of `yaw = ...`;
- `MeshRenderer` becomes the mesh component under a new name, with the same two fields;
- `fitShadowFrustum` reads only `position`, which is unchanged.

The result goes into the record's consequences, along with anything that does not map. Whether
and when retcon adopts it is retcon's decision, once milestone 4 has shipped the mesh component
with a handle.

### Step 6 — The mesh component, handed to milestone 4

[m4-LitScene.md](../../roadmap/completed/m4-LitScene.md)'s second section says the registry's handles are what
milestone 3's component names. That sentence becomes the component itself: a `MeshHandle` and
`castsShadow`, in `realtime/component/`, built with the registry, with the material on the
registry entry. The walk the lit pass does is the same shape as step 3's, over
`view<const Transform, const Mesh>()`.

---

## Sequence

**Step 1 first, as `proposed`.** Every ADR in this tree is written before its code, and this one
also stays open for revision until step 4.

**Steps 2 and 3 next, in order.** Both are headless and both are small. Step 2's slerp case is
what the rotation choice is defending, so it lands with the component.

**Step 4 straight after step 3.** It accepts the record. Steps 5 and 6 can go before or after
it, because neither depends on what the prototype finds about sprites.

## Verification

Per [sdlc.md](../../sdlc.md), each step that changes code: `ninja -C out/build/x64-Debug`, `ctest`,
cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all of them,
so every finding is the step's. Steps 2 to 4 are wholly headless, so nothing in this plan needs
the device suite or lavapipe. Nothing here is verified in another repository.

## What this does not do

- **It does not draw a mesh.** There is no handle to draw one with until milestone 4.
- **It does not parent anything.** Step 1 says what the trigger is.
- **It does not move the editor onto entities.** The roadmap leaves that to the editor.
- **It does not cull.** A walk that culls is a frustum test per entity, and step 3's sprites are
  cheap enough that a test per sprite costs nearly what it saves. A game culls by region, as
  LargeWorlds' handoff says.
- **It does not add a sprite clip.** That stays in TODO.md behind its second consumer, and step
  3's uvs are where the clip will write.

## When a step lands

Update the state in the table above.

- **Step 1** adds the ADR index row, as `proposed`.
- **Step 2** adds `Transform` to [ECSDesign.md](../../ECSDesign.md)'s list of components.
- **Step 3** adds `api/ecs` to `api/render`'s dependencies in [Build.md](../../Build.md) if that
  document lists them.
- **Step 4** sets ADR-0063 to accepted. It replaces RenderingPipeline.md's
  [still-open section](../../RenderingPipeline.md#how-this-meets-the-ecs) with what was
  decided, and ECSDesign.md's opening paragraph stops calling the question open. It also writes
  cozy's handoff note, to be read when cozy takes the feature up: `Transform`, `Sprite` and
  `sprites()`, the axis to pass, and `snapshot<Transform>` to interpolate.
- **Step 6** edits m4-LitScene.md as the step says.
- **When the plan closes**, the roadmap's [m3](../../roadmap/completed/m3-RenderableComponent.md) moves to
  `roadmap/completed/` and points here as done, the roadmap's table row says so, and this file
  moves to [completed/](./).
