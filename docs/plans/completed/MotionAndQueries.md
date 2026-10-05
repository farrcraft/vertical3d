# Motion And Queries — A Plane, A Frustum, A Ground Pick, Overlap, And Drawing Between Steps

Drafted 2026-10-03 against `13a9557`. Six steps across `api/type`, `api/ecs`, `moya` and `pong`,
taking up [milestone 1](../../roadmap/completed/m1-MotionAndQueries.md) of
[the game engine roadmap](../../roadmap/completed/GameEngine.md). Five are open and one is held behind a
named trigger.

**Closed on 2026-10-03.** Steps 1 to 5 landed and pong was watched; step 6 went to TODO.md with
its trigger. [plans/README.md](../README.md) records what came out differently.

Every piece here is something a consumer has written for itself or stubbed. moya has a `Plane`
and a `Frustum` that are general geometry living in a renderer. retcon has a ground pick of its
own (`engine/view/Picking`: `screenRay` and `intersectHorizontalPlane`) beside this tree's
`Camera::ray()`, which already does the first half of it. pong writes its box tests by hand,
because [`Bound2D`](../../../api/type/geometry/Bound2D.h) tests only a point and `AABBox` tests
nothing. And [`Engine::alpha()`](../../../api/engine/Engine.h) has been in the loop since
[ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md) with nothing reading it, so every
world drawn in this tree, and in both games, is snapped to the last 60 Hz step.

**What is due first is the ground pick.** cozy's M5 step 10 is a click in the world, and cozy's
M6 is where overlap and culling start to matter. Nothing here waits on a decision except
step 5, which carries one.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--plane-moves-to-apitype) | `Plane` moves to `api/type/geometry`, and moya keeps its clip | `api/type`, `moya` | — | done |
| [2](#step-2--a-ray-meets-a-plane) | `Ray::intersects(Plane)`, which is the ground pick | `api/type` | — | done |
| [3](#step-3--frustum-moves-to-apitype-and-is-told-its-depth-range) | `Frustum` moves to `api/type/geometry` and is told its clip space's depth range | `api/type`, `moya` | cites 0024 | done |
| [4](#step-4--a-box-overlaps-a-box) | `Bound2D` and `AABBox` overlap and containment, and pong adopts them | `api/type`, `pong` | — | done |
| [5](#step-5--a-moving-thing-keeps-its-previous-step) | A component's previous step is kept, and pong draws between steps | `api/ecs`, `pong` | **0060** | done |
| [6](#step-6--a-sprite-clip) | A sprite clip | to be decided | — | moved to [TODO.md](../../TODO.md#sprite-sheets) with its trigger |

Step 1 blocks 2 and 3, both of which need a `Plane` in `api/type`. Steps 4 and 5 are
independent of everything, including each other.

---

### Step 1 — `Plane` moves to `api/type`

`moya/libmoya/Plane.h`, now [`api/type/geometry/Plane.h`](../../../api/type/geometry/Plane.h), is a plane held as its equation —
normal in A to C, negated distance in D, so that `distance(p)` is the equation applied to `p`.
It classifies a point and an `AABBox`, intersects a ray and an edge, and normalises. All of that
is geometry; one member is not. `clip(const boost::shared_ptr<Polygon>&)` is a Sutherland-Hodgman
clip of moya's own `Polygon`, and it is the reason `Plane.h` includes `Polygon.h`.

**The shape.** `v3d::type::geometry::Plane`, in `api/type/geometry/Plane.h` and `.cxx`, with
every member except `clip`. `api/type` links glm alone and keeps doing so. The clip stays in
moya as `Polygon::clip(const v3d::type::geometry::Plane&)`, beside the `Polygon::split(Plane)`
that is already there; `Frustum::clip` calls it per plane.

Two things change on the way:

- **`operator[]` gets a const overload.** The non-const one is how `Frustum` writes the
  equation and stays; a reader cannot use it today.
- **The `HalfSpace` enum keeps its aliases.** `OUTSIDE` and `NEGATIVE` are one value, as are
  `INSIDE`/`POSITIVE` and `CROSSING`/`ON_PLANE`, because a frustum asks about a box and a
  clipper asks about a point and each reads naturally in its own caller. Splitting them is a
  separate question with nothing waiting on it.

**Tests.** moya's `PlaneTest.cxx` had six cases. The three that are
the plane — representation, from three points, intersecting an edge — move with it to
`api/type/tests/PlaneTest.cxx`, by `git mv` so their history follows. The three clip cases stay
in moya, in a new `moya/tests/PolygonClipTest.cxx`. Added in `api/type`: a box
on each side of a plane and one straddling it, since `classify(AABBox)` is what step 3 culls
with and nothing tests it directly today.

**The trap.** [`Polygon.h`](../../../moya/libmoya/Polygon.h) forward-declares `class Plane;` in
`v3d::moya`. Left alone it still compiles wherever `Polygon.h` is included without `Plane.h`
and fails wherever both are, because it declares a second `Plane` in the wrong namespace —
which is what [ApiOrganisation](ApiOrganisation.md) recorded at nearly every one of
its moves. It becomes a forward declaration in `v3d::type::geometry`.

### Step 2 — A ray meets a plane

[`Camera::ray()`](../../../api/type/camera/Camera.h) turns a window point into a world-space ray,
orthographic included, by unprojecting both depths. What a ground pick then needs is where that
ray crosses the ground, and `Ray` can meet an `AABBox` and a triangle but not a plane.

**The shape.** One overload, in the form `Ray` already uses:

```cpp
/**
 * Where this ray crosses a plane.
 *
 * A ray lying in the plane, or parallel to it, does not cross it, and neither does one that
 * would have to run backwards to reach it: a click on the sky is not a click on the ground
 * behind the camera.
 *
 * @param plane the plane, whose normal need not be unit length
 * @param distance set to how far along the ray the crossing is, in the ray's own units
 * @return whether the ray crosses the plane at or ahead of its origin
 **/
bool intersects(const Plane& plane, float* distance) const;
```

and `ray.point(distance)` is the world position, which `Ray` already has. A ground pick is
`camera.ray(cursor, viewport)` and this against a `Plane` built from `(0, 1, 0)` and the
ground's height. Nothing more is added: a `groundPick()` helper would be three lines whose only
content is which way is up, and that is the caller's.

**Tests**, in [`RayTest.cxx`](../../../api/type/tests/RayTest.cxx): a ray straight down onto the
ground, a ray parallel to it, a ray pointing away from it, a plane whose normal is not unit
length, and the case that is the reason for the step — an orthographic camera's ray through
two different window points, which must land at two different ground points with the same
distance along each.

**Downstream.** retcon's `intersectHorizontalPlane` is this, and its `screenRay` is
`Camera::ray()` once its isometric camera has applied itself to one. Whether retcon adopts them
is retcon's to decide; what this step owes it is a note in the handoff that both now exist here.

**The handoff note.** For retcon: `engine/view/Picking`'s two halves now exist in `api/type`.
`screenRay` is [`Camera::ray(cursor, viewport)`](../../../api/type/camera/Camera.h) on a camera that
`camera::Isometric::apply()` has written to, and `intersectHorizontalPlane` is
[`Ray::intersects(const Plane&, float*)`](../../../api/type/geometry/Ray.h) against a
[`Plane`](../../../api/type/geometry/Plane.h) built by `calculate(glm::vec3(0, 1, 0), glm::vec3(0, height, 0))`,
with `ray.point(distance)` as the ground position. A ray parallel to the ground, or one that
would reach it only behind its origin, answers false. Adopting them is retcon's decision under
its ADR-0041.

### Step 3 — `Frustum` moves to `api/type`, and is told its depth range

`moya/libmoya/Frustum.cxx`, now [`api/type/geometry/Frustum.cxx`](../../../api/type/geometry/Frustum.cxx), extracted six planes from a matrix by the
Gribb-Hartmann method and classifies a box against them. **Its near plane is right for moya and
wrong for every camera in the realtime tree**: it is the w row plus the z row, which is the near
plane of a clip volume whose depth runs −1 to 1, and its own comment says so. `type::camera::Camera`
builds Vulkan clip space ([ADR-0012](../../adr/0012-camera-builds-vulkan-clip-space.md)), whose depth
runs 0 to 1, and there the near plane is the z row alone. A frustum extracted from a realtime
camera as written today puts its near plane behind the camera, so it keeps things it should cull
and never culls anything it should keep — which is the direction that looks right.

**The shape.** `v3d::type::geometry::Frustum`, holding six `Plane`s in a `std::array` rather than
a `std::map<std::string, Plane>`, and told which depth range the matrix it is given builds:

```cpp
enum class Depth {
    ZeroToOne,      // Vulkan, and every camera in api/type
    MinusOneToOne   // OpenGL, and moya's RenderContext
};

explicit Frustum(const glm::mat4& viewProjection, Depth depth = Depth::ZeroToOne);
int intersect(const AABBox& box) const;
const std::array<Plane, 6>& planes() const noexcept;
```

That is [ADR-0024](../../adr/0024-api-type-serves-both-renderers.md)'s rule applied a second time:
a convention one consumer needs becomes a parameter rather than a second copy of the type. The
default is the realtime one, because that is what `api/type`'s own cameras build, and moya
names the other. `intersect` becomes const, which it was not for no reason.

moya's `Frustum::clip` is a loop over the planes calling step 1's `Polygon::clip`, and it moves
into moya beside it as a free function over `planes()`. It is called only from moya's tests.

**Tests.** moya's three `FrustumTest.cxx` cases move with the
type and name `MinusOneToOne`. The cases that are the point of the step are new, and build a
`type::camera::Camera`, perspective and then orthographic, rather than a matrix by hand:

- a box just in front of the near plane is `INSIDE`, and one just behind the eye is `OUTSIDE`;
- the same box extracted with the wrong `Depth` gives a different answer, so the parameter is
  shown to matter rather than assumed to;
- a box on each side of each of the other four planes, and one straddling each.

moya's [`ReferenceTest`](../../../moya/tests/ReferenceTest.cxx) renders through
`RenderContext`'s culling, so its two pictures not changing is what shows moya's own behaviour
survived.

**Nothing in the realtime tree culls yet.** That is [milestone 2](../../roadmap/completed/m2-LargeWorlds.md#culling);
this step makes it possible and leaves voxel submitting every chunk.

### Step 4 — A box overlaps a box

`Bound2D` can say whether a point is inside it, through a non-const `intersect` that treats the
edge as inside. `AABBox` can be built and extended and cannot be asked anything.

**The shape.**

| Type | Added | Edge |
|---|---|---|
| `Bound2D` | `bool overlaps(const Bound2D&) const`, `bool contains(const glm::vec2&) const` | closed |
| `AABBox` | `bool overlaps(const AABBox&) const`, `bool contains(const glm::vec3&) const` | closed |

Closed on both, because `Bound2D::intersect` already is and two answers in one library about
whether a shared edge counts would be one more thing to check. `intersect` stays and becomes a
call to `contains`; it is not removed in this step because the editor's ui and pong may reach
it, and finding out is not this step's work.

**pong adopts it.** [`PongScene.cxx:85-112`](../../../pong/src/PongScene.cxx#L85) is two copies of a
box test, each spelt out with `ballSize() / 2.0f` five times. They become the ball's box
overlapping a paddle's. **The test is one-sided in x on purpose**: the ball bounces when it has
reached the paddle's line *or passed it*, so a fast ball that crosses the paddle between two
steps still bounces rather than tunnelling through. A paddle box the paddle's own width would
lose that, so the box pong builds runs from the paddle's face out past the court's edge. That is
pong's knowledge and is written as such in pong; the api answers overlap and nothing else.

**Tests**, in [`Bound2DTest.cxx`](../../../api/type/tests/Bound2DTest.cxx) and
[`AABBoxTest.cxx`](../../../api/type/tests/AABBoxTest.cxx): disjoint on each axis, overlapping, one
inside the other, and sharing exactly an edge — which overlaps, by the rule above. pong's half
is verified by [`PongSceneTest.cxx`](../../../pong/tests/PongSceneTest.cxx), whose collision, miss
and scoring cases pass unchanged, plus a case for a ball already past each paddle's face; the
suite compiles `PongScene.cxx` itself, so it links `v3dlib_type` as the app does.

### Step 5 — A moving thing keeps its previous step

**This is the step that carries a decision, and it earns ADR-0060.** What has to be settled is
where the state a renderer blends between lives, and who keeps it. The loop calls `simulate()`
zero or more times a frame and then `render()` once
([ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md)), so a renderer that wants to draw
between the last two steps needs the state before the last step as well as after it, and nothing
holds that today.

**The recommendation.** A component template and two functions in `api/ecs`:

```cpp
namespace v3d::ecs {

// What T was at the start of the most recent simulation step.
template <typename T> struct Previous final { T value; };

// Copy every entity's T into its Previous<T>, emplacing one where there is none. Called at the
// top of simulate(), before anything moves.
template <typename T> void snapshot(entt::registry& registry);

// The previous and current step blended by alpha, or the current one alone for an entity
// that has no previous yet - one created during the step it is first drawn after.
template <typename T> T interpolated(const entt::registry& registry, entt::entity entity, float alpha);

};  // namespace v3d::ecs
```

with `interpolated` blending through an overload `interpolate(const T&, const T&, float)` found
by argument-dependent lookup. `api/ecs` provides it for `Position1D` and `Position2D`;
`PositionFixed2D` is a tile and is deliberately not given one. A game's own component — cozy's
`Position`, retcon's `Transform` — gets interpolation by writing that one function beside it.

**Why a component rather than a pair inside a type**, which is the alternative and the simpler
one to write. A pair inside the type has to be shifted every step whether or not the thing
moved, or a thing that stopped is drawn still sliding toward where it stopped; the shift then
has to be remembered per field by every caller. A snapshot at the top of the step covers every
entity carrying `T` in one call, whatever moved and whatever did not, and leaves the game's own
component types alone.

**A teleport is the one thing it gets wrong unless told.** A ball put back on the centre spot
after a point would be drawn for one frame streaking across the court. `settle<T>(registry,
entity)` sets the previous to the current, and is what a reset calls.

**What the ADR does not decide** is what an entity's transform is, which is
[milestone 3](../../roadmap/completed/m3-RenderableComponent.md)'s. This works over whatever `T` that turns
out to be, and is meant to.

**pong adopts it.** The ball and both paddles are `Position2D`/`Position1D` components already.
`PongEngine::simulate` snapshots them first, the ball's reset after a point settles, and
`PongRenderer::drawBall` and `drawPaddle` read `interpolated` at `alpha()` rather than the
component.

**Tests**, in [`api/ecs/tests`](../../../api/ecs/tests): a snapshot then a move, interpolated at an
alpha of 0, 1 and between; an entity created after the snapshot interpolating to its current
value; a settle after a move drawing no motion; and a component type with no `interpolate`
failing to compile, which is a `static_assert` in the template rather than a test. pong's half is
a person watching the ball on a display faster than 60 Hz, where the stepping is visible before
and gone after.

### Step 6 — A sprite clip

**Held, and here is why.** The roadmap puts a sprite clip in this milestone because its
time-keeping — advancing on the fixed step, looping, clamping, an event at a named point — is the
same as a skeletal clip's ([milestone 5](../../roadmap/completed/m5-SkeletalAnimation.md)) and a particle's
lifetime ([milestone 6](../../roadmap/completed/m6-Effects.md)), and is worth writing once. But there is one
consumer, and that consumer has said in its own M5 plan that a walk cycle is "perhaps thirty
lines, entirely this game's", and that nothing in the engine should have to do it for one game.
It also deferred its walk cycle because there is no walking art.

**The trigger is a second consumer.** Either cozy's walk cycle is written and something else
wants the same clock — milestone 6's particles are the likeliest — or milestone 5 is taken up and
its sampler needs a clock to be written against. At that point this step is drafted against what
cozy wrote rather than ahead of it, and what decides where it lives is whether it is glm-only,
which `api/type` requires, or holds region names from `api/config`, which would make it a library
of its own under [ADR-0033](../../adr/0033-a-consumer-selects-the-api-libraries-it-wants.md).

Until then this plan closes without it, and it moves to [TODO.md](../../TODO.md) with its trigger.

---

## Sequence

**Steps 1 and 2 first, together**, because they are the ground pick and that is what cozy needs
next. Step 1 is a move with no behaviour change, step 2 is one function on top of it, and the
two are two commits.

**Step 3 after them.** It needs step 1 and nothing needs it yet — culling is milestone 2's — but
it finishes moving moya's geometry while the move is fresh, and its depth-range bug is cheaper to
fix before anything culls with it than after.

**Steps 4 and 5 in either order, any time.** Each is a library change and a pong change, and each
pong change is its own commit after the library's, so a pong that plays wrongly says which half
did it. Step 5's ADR is written and accepted before its code, as every ADR here is.

## Verification

Per [sdlc.md](../../sdlc.md), each step: `ninja -C out/build/x64-Debug`, `ctest`, cpplint, and the
`/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all of them, so every finding is
the step's. Steps 1 and 3 touch moya, and **the analysis gates have to be run with apps on** —
[OfflineRenderingPhase3](OfflineRenderingPhase3.md) found they had only ever seen
`api/`.

Steps 1 to 3 are verified by moya's existing suite passing unchanged — its reference pictures
cull through the moved `Frustum` — plus the new `api/type` cases. Step 4's library half and step
5's are headless. Their pong halves are the one thing a person has to watch, and each step says
what to watch for.

Nothing here draws differently on a device except pong after step 5, and that difference is the
point of the step rather than something a reference could pin
([ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)).

## What this does not do

- **It does not cull anything.** Step 3 makes a correct frustum available; using it is
  [milestone 2](../../roadmap/completed/m2-LargeWorlds.md#culling).
- **It does not respond to a collision.** Step 4 answers whether two boxes overlap. What happens
  when they do is a game's rule, as pong's one-sided paddle shows.
- **It does not give voxel collision.** `Player::checkWorldCollision` is a lookup in voxel's own
  chunk storage, not a box test, and overlap does not help it.
- **It does not give odyssey smooth movement.** odyssey's path follower replaces a tile every
  0.15 seconds, so what it would blend is a position along its own path, not the loop's last two
  steps.
- **It does not settle the transform.** Step 5 is generic over a component type on purpose.

## When a step lands

Update the state in the table above, and set ADR-0060's status when step 5 lands.

- **Steps 1 and 3** move the geometry [Architecture.md](../../Architecture.md#geometry) describes;
  it gains `Plane` and `Frustum`, and the frustum's depth range is an invariant worth its own
  bullet under [Invariants that bite](../../Architecture.md#invariants-that-bite).
  [OfflineRendering.md](../../roadmap/completed/OfflineRendering.md#what-the-api-already-provides) gains them
  in its list of what the api provides.
- **Step 2** is the ground pick, and the handoff note to retcon is written here, when it lands.
- **Step 5** changes what [Architecture.md](../../Architecture.md#the-loop-has-two-virtuals-and-they-mean-different-things)
  says about `alpha()` — "nothing reads it yet" stops being true — and
  [ECSDesign.md](../../ECSDesign.md) gains `Previous<T>`. Delete [TODO.md](../../TODO.md#the-game-loop)'s
  game-loop entry rather than marking it.
- **When the plan closes**, step 6 moves to TODO.md with its trigger, the roadmap's
  [m1](../../roadmap/completed/m1-MotionAndQueries.md) points here as done, and this file moves to
  [completed/]().
