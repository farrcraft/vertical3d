# Animation and particle effects

This page is for someone writing an app against the api. It covers `api/animation`, which holds
skeletons, poses, clips, curves and tracks, and the particle effects in `api/ecs/effect`. Both
depend on glm alone and hold no device, so both can be used and tested headless. Which clip plays
and when an effect fires is the game's decision. [ECS.md](ECS.md) covers the components that
step these from entities. Terms are defined in the [glossary](README.md#glossary).

- [Animation](#animation)
- [Particle effects](#particle-effects)

## Animation

Animation data and arithmetic are in `v3d::animation`
([api/animation/](../../api/animation/), `v3dlib_animation`). Playing an animation on an entity is
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
| `Curve` | One number keyed on frames, each key with its own interpolation and slopes, which an animator shapes. |

### Clock

A `Clock` keeps no time of its own. Whoever plays something keeps the time and passes it to
the clock's functions.

- `advance(time, step)` returns the time a step later. A looping clock's time is kept
  **unwrapped**, so two steps either side of the loop point still interpolate forwards. A clamped
  clock holds at its duration.
- `sample(time)` wraps an unwrapped time into `[0, duration]`. Pass its result to
  `animation::sample()`.
- `crossed(from, to, marker)` counts how many times a step passed a marker time, such as a
  footstep. A step that ends exactly on the marker passes it; one that starts on it does not. Each
  marker is therefore reported once however the steps fall. A marker at the duration is reported
  once when a clamped clip stops, and on every wrap of a looping one. A step that starts or ends at
  a time that is not finite passes nothing, nor does a marker that is not a number. A count too
  large for a `uint32_t` is held at its largest value.
- `finished(time)` is true when a clamped clip has reached its end. A looping one never has.
- A duration of zero or less is a clock that never moves.

### Curve

A `Curve` is what an editor keys. Each `Curve::Key` has a frame, a value, an interpolation and
an in and an out slope, in value per frame.

- A key's interpolation says how the curve leaves it for the next key. `Step` holds the value,
  and `Linear` draws a straight line. `Bezier` draws a cubic Hermite segment, whose end slopes
  are the key's out slope and the next key's in slope.
- Before the first key the curve holds the first key's value, and after the last it holds the
  last's. A curve with no keys is 0, and a frame that is not finite reads as the first key.
- An automatic key has its slopes worked out whenever the keys change. Its slope is that of the
  line through the keys either side, and it is flat at a peak, a trough, the first key and the
  last, so the curve never overshoots those. A key that is not automatic keeps the slopes it was
  given.
- `set(frame, value)` replaces the value of the key on that frame, keeping its interpolation and
  slopes, or adds an automatic Bezier key. Two frames within `SAME_FRAME` are one frame.
- Construction throws for keys that are not finite or do not rise in frame.

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

Particle simulation is in `v3d::ecs::effect` ([api/ecs/effect/](../../api/ecs/effect/)). The
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
