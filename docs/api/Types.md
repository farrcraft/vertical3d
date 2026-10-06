# Types and maths

This page is for someone writing an app against the api. It covers `api/type`: geometry
queries, random numbers, typed flags, cameras, transforms, models and skeletons, animation and
particle effects. Terms are defined in the [glossary](README.md#glossary).

- [What the library is](#what-the-library-is)
- [Geometry queries](#geometry-queries)
- [Random numbers](#random-numbers)
- [Flags](#flags)
- [Cameras](#cameras)
- [Transform](#transform)
- [Models](#models)
- [Animation](#animation)
- [Particle effects](#particle-effects)

## What the library is

`v3dlib_type` depends on glm and nothing else. It holds no device, window or file handle, so
everything in it can be used and tested headless, and the offline renderer uses it as well as
the realtime one. The namespaces are `v3d::type` and, below it, `geometry`, `camera`,
`animation` and `effect`.

What `api/type` does not do:

- It does not load files. [Assets.md](Assets.md) covers loading a model.
- It does not draw. [rendering/](rendering/README.md) covers that.
- It does not decide when things happen. Which animation clip plays, and when an effect fires,
  is the game's decision. [ECS.md](ECS.md) covers the components that step these types from
  entities.

## Geometry queries

The queries are in `v3d::type::geometry` ([api/type/geometry/](../../api/type/geometry/)).

| Type | What it answers |
|---|---|
| `Ray` | Where it meets an `AABBox`, a triangle or a `Plane`. |
| `Plane` | The signed distance to a point, and which side of it a point or a box is on. |
| `Frustum` | Whether a box is inside, outside or crossing the six planes of a view-projection matrix. |
| `AABBox` | An axis-aligned 3D box. Whether it contains a point or overlaps another box. |
| `Bound2D` | A 2D rectangle. Whether it contains a point or overlaps another rectangle. |

### Ray

- The constructor normalises the direction, so a distance along the ray is in world units.
- `transformed(matrix)` moves a ray into another space. The direction is transformed as a
  vector and the origin as a point, and **neither is renormalised**. A distance found in a
  mesh's own space is then comparable with one found in world space. To test a world ray against
  a mesh, pass the inverse of the mesh's matrix.
- `intersects(box, &distance)` is a slab test. A ray that starts inside the box hits it at
  distance zero. A ray whose origin or direction is not finite misses every box.
- `intersects(a, b, c, &distance)` tests a triangle from either side. The overload with `u` and
  `v` also returns barycentric weights: the hit is `a + u * (b - a) + v * (c - a)`. Use them to
  interpolate a normal or a texture coordinate.
- `intersects(plane, &distance)` is true only when the ray crosses the plane at or ahead of its
  origin. A ray parallel to the plane, or one that would have to run backwards, does not cross
  it.
- Every hit is at a non-negative distance. Each `distance` pointer may be null.

### Plane, boxes and bounds

- A `Plane` is stored as one equation, `ax + by + cz + d`. `normal()` is unit length only if
  the plane has been normalised.
- `classify()` on a point or a box returns `POSITIVE`/`INSIDE`, `NEGATIVE`/`OUTSIDE` or
  `ON_PLANE`/`CROSSING`.
- For `AABBox` and `Bound2D`, **edges and faces count as inside**. Two boxes that touch at a
  face overlap, and a point on an edge is contained.

### Frustum

`Frustum(viewProjection, depth)` extracts the six planes of a matrix. Each plane faces inward.
The frustum is in whatever space the matrix reads: a projection alone gives eye space, and a
projection times a view gives world space.

**Tell it the depth range of the matrix.** The default, `Depth::ZeroToOne`, matches the
matrices `type::camera::Camera` builds. moya's matrices use `Depth::MinusOneToOne`. With the wrong one,
the near plane is placed behind the true one. The frustum then keeps boxes it should cull, which
looks correct and is hard to notice.

`intersect(box)` is conservative. A box near a frustum corner can be outside the frustum while
inside every plane separately, and it is reported as `CROSSING`. The test never culls a box it
should keep.

### Picking the ground

To find where a click lands on the ground, cast `Camera::ray()` and intersect it with a
`Plane`. There is no helper for this, because which way is up is the caller's choice. Code that
clips a renderer's own primitive against a plane belongs to that renderer, as moya's
`Polygon::clip` does.

## Random numbers

`v3d::type::Random` ([api/type/Random.h](../../api/type/Random.h)) is the api's source of random
numbers.

- **A seed fixes every value it gives**, on every compiler and standard library. The values are
  derived in the class, not through `<random>`, whose distributions differ between standard
  libraries. A test can therefore pin exact values.
- The generator is splitmix64. Its whole state is one 64-bit integer, and every seed is valid,
  including zero.
- `state()` returns a seed that resumes the sequence from where it stands. Store it in a save
  file to restore the sequence.
- `next()` gives 64 bits. `below(n)` gives a whole number in `[0, n)` with no modulo bias, and
  throws for `n` of zero. `unit()` gives a float in `[0, 1)`. `range(low, high)` gives a float
  in `[low, high)`.
- `inside(min, max)` gives a point in a box. `cone(axis, angle)` gives a unit direction spread
  evenly within `angle` radians of `axis`.
- It is not thread-safe, and the values depend on the order of calls.

## Flags

`v3d::type::Flags<E>` ([api/type/Flags.h](../../api/type/Flags.h)) is a set of an enum's bits.
It replaces an `int` mask: it cannot hold a bit of another enum or a stray number. An enum opts
in by declaring one `operator|`:

```cpp
enum class Feature : uint32_t { Window = 1 << 0, Config = 1 << 1 };
constexpr type::Flags<Feature> operator|(Feature a, Feature b) noexcept {
    return type::Flags<Feature>(a) | b;
}
```

A single enumerator converts to a set of one. `has(bit)` tests a bit and `empty()` tests for
none. `engine::Features` is the example in the tree.

## Cameras

The camera classes are in `v3d::type::camera` ([api/type/camera/](../../api/type/camera/)):

- `Profile` holds a camera's settings: the eye position, the basis (`up`, `right`,
  `direction`), the rotation, the clipping distances, the vertical field of view in degrees,
  the orthographic zoom, the pixel aspect ratio, the viewport size and the handedness.
- `Camera` holds a profile and builds the projection and view matrices from it.
- `Isometric` is an orbit around a target on the ground, snapped to four azimuths.
- `ArcBall` turns a mouse drag into a rotation for `Camera::rotate()`.

`config::CameraProfiles` reads named profiles from a config document.
[engine/Config.md](engine/Config.md#camera-profiles) describes the format.

### Clip space and matrices

- **`Camera` builds Vulkan clip space**: y points down the screen, and depth runs from 0 at the
  near plane to 1 at the far plane. This holds for both perspective and orthographic
  projections.
- The camera looks along `+z` of its own basis, so a point in front of it has a positive view
  `z`.
- `project()` and `unproject()` are inverses of each other.
- **The matrices are cached.** Call `createProjection()` and `createView()` after changing the
  profile. `projection()`, `view()` and `ray()` read the cached matrices.
- `ray(point, viewport)` casts a world-space ray from a window pixel. It starts at the near plane
  and has a unit direction. An orthographic camera gives parallel rays.
- `orthoFactorHorizontal()` and `orthoFactorVertical()` are the world units one viewport pixel
  covers. They are zero until the profile has a viewport size, set with `Profile::size()`.

### The basis and the rotation

A profile holds the three basis vectors and the rotation as separate state.

- `lookat(center)` is the one call that sets all four together. Use it to aim a camera.
- Setting `up()`, `right()` or `direction()` directly does not change the rotation.
- `rotation(q)` sets the rotation and does not change the basis vectors.
- `turn(q)` composes a rotation onto the current one and re-derives the basis vectors from the
  result. `Camera::pan()`, `Camera::tilt()` and `Camera::rotate()` all go through `turn()`, so
  a dolly or truck after a turn moves along the view's new axes.

**A view built through `lookat()` matches `glm::lookAt` exactly**, element for element, for the
same hand. `lookat()` keeps the basis matrix it built and `createView()` uses it directly, rather
than rebuilding it from the quaternion. After any other change to the rotation, `createView()`
rebuilds from the quaternion, which agrees to about 2e-6 per element. This matters only to a
consumer that compares rendered frames at zero tolerance. The up vector `lookat()` produces is
unit length to within float rounding, as `glm::lookAt`'s is.

### Handedness

**The default basis is `right = up × direction`.** That is the opposite hand to `glm::lookAt`,
which builds `right = direction × up`. From the same eye, centre and up, the two give opposite
right vectors and the same up vector, so each image is the horizontal mirror of the other. A
mirror also reverses the winding of every front face.

The screen's right is `Profile::right()`. Do not copy a world axis from code written for
`glm::lookAt`: a camera move built on the wrong hand moves the scene the wrong way, and nothing
else looks wrong.

An app whose geometry is wound for `glm::lookAt` chooses the other hand:

```cpp
profile.hand(v3d::type::camera::Profile::Hand::DirectionCrossUp);
profile.lookat(center);   // the hand takes effect here
```

- **Set the hand before `lookat()`.** The basis vectors are not recomputed until `lookat()` or
  `turn()` runs, so a hand set on a camera that is already aimed leaves `right()` reporting the
  old hand.
- The hand is copied with the profile, by `clone()` and by assignment.
- The rotation is a proper rotation in both hands. For `DirectionCrossUp`, `right()` reports the
  negated vector and `createView()` negates view `x`. `pan()`, `tilt()` and the view are
  otherwise unaffected.
- Nothing in this tree uses `DirectionCrossUp`, so `right()` in this tree's code always means
  `up × direction`.

`Isometric` carries its own hand. Its `right()` and `forward()` follow it, and `apply()` writes
it onto the camera along with everything else. Set the hand on the orbit, not on the profile.

Background: [ADR-0012](../adr/0012-camera-projection-targets-vulkan-clip-space.md),
[ADR-0052](../adr/0052-camera-selectable-handedness.md)

### Isometric

`Isometric` ([api/type/camera/Isometric.h](../../api/type/camera/Isometric.h)) places an
orthographic camera on an orbit around a target on the ground.

- `rotate(steps)` turns by quarter turns, counterclockwise for positive steps. `azimuth(index)`
  jumps to one of the four azimuths. Both wrap.
- `pan(delta)` moves the target in the view's own axes: `x` along `right()` and `y` along
  `forward()`, in world units.
- The default elevation is 45 degrees. A true isometric projection is about 35.26 degrees;
  pass `atan(1 / sqrt(2))` to `elevation()` for it.
- `zoom()` is the orthographic half height, clamped to `[2, 40]`. `distance()` only decides
  what the near and far planes cut, and must clear the tallest thing between eye and target.
- `apply(camera)` writes the eye, orientation, zoom, orthographic flag and hand onto a camera.
  It does not set clipping, pixel aspect or viewport size, and does not rebuild the matrices.

## Transform

`v3d::type::Transform` ([api/type/Transform.h](../../api/type/Transform.h)) is a position, a
quaternion rotation and a scale. ECS entities and editor meshes are both placed by it.

- `matrix()` scales first, then rotates, then translates. Scaling first keeps a non-uniform
  scale along the object's own axes. It normalises the rotation first, so a quaternion that has
  drifted off unit length through repeated turns still only turns. A rotation of zero length
  is no rotation.
- `interpolate(from, to, alpha)` lerps position and scale and slerps the rotation the short way
  round.

## Models

### Three things called a mesh

| Type | What it is |
|---|---|
| `brep::BRep` | Half-edge topology the editor models with. See [editor/](../editor/README.md). |
| `type::Model` | Interleaved vertices, indices, materials and parts: what a model file loads into. |
| The renderer's mesh | Two device buffers, made from a `type::Model`. See [rendering/](rendering/README.md). |

### type::Model

`type::Model` ([api/type/Model.h](../../api/type/Model.h)) holds one vertex array and one index
list for a whole file, so a model is one upload.

- **`Vertex`** is a position, a normal and a uv, interleaved: 32 bytes. This layout is an
  agreement between the loader and whatever pipeline draws it. The device does not check it;
  a device mesh takes bytes and a count, and the stride belongs to the pipeline.
- **`Material`** is a base colour and `baseColourTexture`, the name the file gave its image.
  The model names the image and does not hold its pixels. The app loads the image through the
  asset manager. An image embedded in the file arrives decoded on the asset instead; see
  [Assets.md](Assets.md#models-and-gltf).
- **`Part`** is a range of the index list (`firstIndex`, `indexCount`) and a material index.
  Every index is in exactly one part, and a loaded model has at least one. Draw a model as one
  draw per part.
- **`skeleton()`, `influences()` and `clips()`** are empty for a static model. A skinned model
  has a `Skeleton`, one `Influence` per vertex in a separate array parallel to the vertices,
  and its animation clips.
- `vertexBytes()` is the size of the vertex array, for creating a device buffer. `empty()` is
  true when there is no geometry.

### Skeleton

`type::Skeleton` ([api/type/Skeleton.h](../../api/type/Skeleton.h)) is the list of joints a
skinned model is bent by.

- **Every parent comes before its children.** A pose can therefore be made global in one pass
  from the front.
- Each `Joint` has a name, a parent index (or -1), a rest translation, rotation and scale local
  to its parent, and an `inverseBind` matrix. The inverse bind takes a vertex from model space
  into the joint's space as it stood when the model was bound.
- `root` is the transform above the root joints, such as an armature's scale. No clip animates
  it.
- An `Influence` names up to four joints and their weights, which sum to one.

## Animation

Animation data and arithmetic are in `v3d::type::animation`
([api/type/animation/](../../api/type/animation/)). Playing an animation on an entity is
`ecs::component::Playback`, covered in [ECS.md](ECS.md#playback).

| Type or function | What it is |
|---|---|
| `Channel` | One joint's translation, rotation or scale keyed in time, as glTF stores it. Interpolation is step, linear or cubic spline. |
| `Clip` | A named set of channels and a duration. |
| `sample(clip, time, &pose)` | Writes the joints a clip animates into a pose at a time. Joints the clip does not animate are left as they were. Times before the first key hold the first key; times after the last hold the last. A time that is not finite reads as the first key. |
| `Pose` | Every joint's local translation, rotation and scale, in skeleton order. |
| `rest(skeleton)` | The skeleton's rest pose. Sample a clip over this. |
| `blend(from, to, weight)` | Mixes two poses: lerps translations and scales, slerps rotations. Weight 0 is `from`, 1 is `to`. |
| `palette(skeleton, pose, &out)` | The matrices a vertex is skinned by: each joint made global, then multiplied by its inverse bind. A skeleton in its bind pose gives identity matrices. `out` is resized, so it can be reused across frames. |
| `Clock` | A duration and whether it loops. It holds no time itself. |
| `SpriteClip` | A sequence of sprite sheet regions, each shown for its own duration, over a `Clock`. |
| `Track<T>` | A value keyed in time and lerped, such as a colour over a particle's life. |

### Clock

A `Clock` keeps no time of its own. Whoever plays something keeps the time and passes it to
the clock's functions.

- `advance(time, step)` returns the time a step later. A looping clock's time is kept
  **unwrapped**, so two steps either side of the loop point still interpolate forwards. A clamped
  clock holds at its duration.
- `sample(time)` wraps an unwrapped time into `[0, duration]`. Pass its result to
  `animation::sample()`.
- `crossed(from, to, marker)` counts how many times a step passed a marker time, such as a
  footstep. A step that ends exactly on the marker passes it; one that starts on it does not.
  So each marker is reported once however the steps fall. A marker at the duration is reported
  once when a clamped clip stops, and on every wrap of a looping one. A step that starts or ends
  at a time that is not finite passes nothing, nor does a marker that is not a number. A count
  too large for a `uint32_t` is held at its largest value.
- `finished(time)` is true when a clamped clip has reached its end. A looping one never has.
- A duration of zero or less is a clock that never moves.

### SpriteClip and Track

- `SpriteClip` holds its frames as resolved uv pairs, not region names. Build it from the sprite
  sheet when the sheet loads, and build it again when the sheet reloads. `frame(time)` returns
  the frame showing at an unwrapped time. Construction throws for no frames or a frame with no
  duration.
- `Track<T>` takes keys in rising time. A time that is not finite reads as the first key.
  Without a period it holds the first key before it and the last after it. With a period it
  wraps, and the last key blends into the first, so a day cycle needs no repeated key at
  midnight. `T` is anything `glm::mix` accepts. Construction throws for no keys, keys out of
  order, or a key outside the period.

Background: [ADR-0070](../adr/0070-animation-cpu-sampling-playback-on-the-fixed-step.md)

## Particle effects

Particle simulation is in `v3d::type::effect` ([api/type/effect/](../../api/type/effect/)). The
component that steps an emitter from an entity is in [ECS.md](ECS.md#emitter), and drawing
particles is in [rendering/](rendering/README.md).

### Emitter and State

An `Emitter` describes what is spawned and how it moves and looks. A `State` holds the
particles an emitter has made and its own `Random`.

| Emitter field | Meaning |
|---|---|
| `rate` | Particles per second. |
| `cap` | How many may live at once. Spawns beyond it are dropped. Default 256. |
| `lifeMin`, `lifeMax` | Lifetime in seconds, drawn between these. |
| `shape`, `extent` | Where particles are born: `Point`, `Sphere` (radius `extent.x`) or `Box` (half size `extent`). |
| `direction`, `spread` | The centre of the launch cone, and its half angle in radians: 0 is along `direction`, pi is any way. |
| `speedMin`, `speedMax` | Launch speed in units per second. |
| `acceleration` | Gravity or a steady wind, in world space. |
| `drag` | The fraction of its velocity a particle loses per second. |
| `sway`, `swayRate` | A sideways drift added only when drawn, like a snowflake's. It never affects the simulation. |
| `size`, `colour` | `Track`s over a particle's life, from 0 at birth to 1 at death. |

- **The spawn shape and direction are in the emitter's own space**, turned into the world by
  the orientation passed to `step()`. A muzzle flash therefore follows the way its gun faces.
  `acceleration` is in world space, because gravity does not turn with the emitter.
- **A particle's position is in world space.** It stays where it was born when the emitter
  moves on.
- **A seeded `State` gives the same particles on every run.** Pass the seed to its constructor.

### The functions

| Function | What it does |
|---|---|
| `step(emitter, &state, origin, orientation, seconds)` | Moves the existing particles, then spawns the particles `rate` adds up to over the step. |
| `burst(emitter, &state, origin, orientation, count)` | Spawns `count` particles at once, up to the cap. |
| `travel(emitter, &state, seconds, wind)` | Ages, moves and removes particles without spawning. |
| `spawn(emitter, &state, position, orientation)` | Adds one particle at a world position, unless at the cap. |
| `owing(&state, rate, seconds)` | How many whole particles a rate has earned, keeping the fraction. A rate below zero, or not a finite number, earns nothing. |

- Each `Particle` keeps `previous`, its position before the last step. A renderer draws it
  between `previous` and `position` by alpha. A particle born this step has `previous` equal
  to `position`.
- A particle whose life ends is removed by swapping the last particle into its place. The order
  of particles is therefore not the order they were born in.
- `Particle::life()` is how far through its life a particle is, from 0 to 1.

### Weather

`Weather` describes rain or snow over a region that the caller moves with the view.
`fall(emitter, &weather, &state, minimum, maximum, seconds)` steps it:

- `intensity` eases towards `target` at `ease` per second. The game sets `target` and `wind`.
  The target is held between 0 and 1, a target that is not finite is ignored, and an ease
  below zero moves nothing. An intensity set outside 0 to 1 by the game is not clamped; it
  eases towards the target from wherever it is.
- `density` is particles per second per square unit of ground at full intensity. Particles are
  born across the region's top face (the `maximum` height). `+y` is up.
- A particle that falls below `minimum` is removed. One that leaves across a side re-enters at
  the opposite side, with its previous position moved too, so nothing streaks across the view
  as the region moves.

When an effect fires and what the weather does are the game's decisions. The api provides a
rate, a burst and an intensity, and nothing that chooses them.

Background: [ADR-0072](../adr/0072-particles-an-emitter-component-owns-its-particles.md)
