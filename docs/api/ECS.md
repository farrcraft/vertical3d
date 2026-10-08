# Entities and components

This page is for someone writing an app against the api. It covers how the tree uses
[EnTT](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system): the
registry an app's entities live in, `ecs::System`, the shared components in `api/ecs`, and
drawing entities between simulation steps. How entities are drawn is in
[rendering/](rendering/README.md). Terms are defined in the [glossary](README.md#glossary).

- [The registry](#the-registry)
- [Systems](#systems)
- [Components](#components)
- [Drawing between steps](#drawing-between-steps)
- [How entities are drawn](#how-entities-are-drawn)
- [Notes](#notes)

## The registry

`v3d::engine::Engine` holds one `entt::registry` by value, as the protected member `registry_`.
An app's engine subclass inherits it. There is no accessor, so only the subclass reaches it
directly, and it passes the registry to whatever else needs it: a pointer for a system, a
reference for the render functions.

An entity's components are added by the class that owns the entity's id, and read back through
the registry:

```cpp
registry_.emplace<v3d::ecs::component::Position2D>(id_, glm::vec2(0.0f));
...
auto& position = registry_.get<v3d::ecs::component::Position2D>(id_);
position.value += velocity * step;
```

Use `try_get` when a component may be missing. odyssey's renderer uses it to draw the player
only when the player has a position.

`api/ecs` (`v3dlib_ecs`) depends on `api/type`, glm and EnTT, and on no device. Game rules
written over it can be tested headless.

### Holding an entity across frames

**A stored `entt::entity` can come to name a different entity.** EnTT reuses a destroyed
entity's index for the next one it creates, with a new version. Hold an entity that may be
destroyed while you hold it in a `v3d::ecs::Ref`:

```cpp
v3d::ecs::Ref selected_;
...
selected_.set(clicked);
...
const entt::entity target = selected_.get(registry_);   // entt::null once it is destroyed
if (target != entt::null) { ... }
```

- `get()` compares the stored version with the registry's, so it reads `entt::null` even after
  the index has been reused.
- `clear()` holds nothing, as does a default `Ref`.
- **A `Ref` is for view state**: a selection, an inspector's target, the entity a camera follows.
  A container the game rules keep, such as a turn order, holds plain entities. Whatever destroys
  an entity removes it from such a container, and a `Ref` there would hide a missed removal.

## Systems

`v3d::ecs::System` ([api/ecs/System.h](../../api/ecs/System.h)) is a registry pointer and one
pure virtual function:

```cpp
virtual bool simulate(float step) = 0;   // seconds, one fixed step
```

A system runs on the fixed simulation step, so call it from the engine's `simulate()`. odyssey's
`system::Movement` is the subclass in the tree. Nothing else in the api schedules systems; the
app calls them in the order it chooses.

## Components

### Shared components

The components in [api/ecs/component/](../../api/ecs/component/) are the ones more than one app
can use. All are in `v3d::ecs::component`.

| Component | What it is |
|---|---|
| `Transform` | Where a thing stands in a 3D world: a position, a quaternion and a scale. It is `type::Transform`. |
| `Position1D` | A position along a line, `float value`. |
| `Position2D` | A position on a plane, `glm::vec2 value`. |
| `Color3` | A colour, `glm::vec3 value`. |
| `Playback` | Which animation clip an entity is playing, how far in, and the clip it is fading out of. |
| `Emitter` | A particle effect carried by an entity, and the particles it has made. |

Each is a plain value. Write its fields directly; there are no setters. `Transform`,
`Position1D`, `Position2D` and `Playback` each have an `interpolate()` beside them, so they can
be drawn between steps.

### Your own components

A component does not need to live in `api/ecs`. Define the rest beside the app's own code. pong
keeps `Score`, `Travel`, `Offset`, `PaddleSize`, `Size` and `Direction` in
[pong/src/component/](../../pong/src/component). Any type works as a component: odyssey's
player stands on a `grid::TileCoord`, the type its pathfinding already uses.

### Transform

`Transform` ([api/ecs/component/Transform.h](../../api/ecs/component/Transform.h)) is the
position of a thing's origin (a sprite's feet, a mesh's own origin), its rotation and its
scale. It is the same type an editor mesh's `dag::Transform` holds.

- `aboutY(radians)` builds a turn about `+Y`, for a world that turns about one axis. A positive
  angle turns `+Z` towards `+X`.
- `matrix()` and `interpolate()` are described in [Types.md](Types.md#transform).

The api has two ways to say where a thing is. `Transform` is for things drawn in a 3D world.
`Position1D` and `Position2D` are for simpler 2D games such as pong.

### Playback

`Playback` ([api/ecs/component/Playback.h](../../api/ecs/component/Playback.h)) says which of a
model's animation clips an entity is playing. Clips are identified by their index into the
model's `clips()`. It holds the clip, the time into it, its duration and whether it loops, plus
the same for the clip being faded out of, and the fade's progress.

- **`play(&playback, clip, duration, loops, fade)`** starts a clip from the beginning, fading
  from whatever was playing over `fade` seconds. Asking for the clip already playing changes
  nothing, so a game can call it every step from whatever chooses its clip. A fade of zero, or
  nothing playing, cuts.
- **`advance(registry, step)`** moves every entity's playback on by one step. Call it in
  `simulate()`, after the snapshot.
- **`crossed(previous, current, marker)`** counts how many times a step passed a marker time in
  the clip, such as a footstep. It is zero for a step that changed clip.
- **`Playback::none`** as the clip draws the skeleton's rest pose.
- It holds no pose. The renderer samples the pose from the interpolated playback each frame.
- Across a step that changed clip there is no halfway point, so interpolation gives the later
  step. The fade smooths the change.
- Which clip plays, and when it changes, is the game's decision. The api provides `play()` and
  nothing that chooses a clip.

A looping clip's time is kept unwrapped and grows without limit. At the fixed step, a float
stops resolving a millisecond after a little over two hours of continuous looping.

Background: [ADR-0070](../adr/0070-animation-cpu-sampling-playback-on-the-fixed-step.md)

### Emitter

`Emitter` ([api/ecs/component/Emitter.h](../../api/ecs/component/Emitter.h)) holds a
`type::effect::Emitter` description and a `type::effect::State` with the particles. Construct it
with a seed to make its particles reproducible. [Types.md](Types.md#particle-effects) describes
the description fields and the simulation.

- **`emit(registry, step)`** steps every emitter from its entity's `Transform`: the position is
  the emitter's origin and the rotation turns its spawn shape and direction. An emitter on an
  entity with no `Transform` stands at the origin.
- Call `emit()` from `simulate()` for effects that must be reproducible. A game that does not
  need that may call it from `tick()` with the frame time, and draw its particles at an alpha of
  one. A long frame then spawns all of that frame's particles at once and moves them in one
  large step.
- **Do not snapshot `Emitter`.** Each particle keeps its own previous position, and the renderer
  draws it between steps from that.
- When an effect fires is the game's decision: it sets `description.rate`, or calls
  `type::effect::burst()`.
- A particle's look is one texture and one clip per emitter. Smoke rising from a flame is a
  second emitter, on the same entity or a child.

Background: [ADR-0072](../adr/0072-particles-an-emitter-component-owns-its-particles.md)

## Drawing between steps

The loop simulates at 60 Hz and draws at the display's rate. Drawn as simulated, motion snaps
to the last step and judders on a faster display. To draw smoothly, keep each moving value's
state from before the last step and blend towards the current state by `Engine::alpha()`.
[engine/Loop.md](engine/Loop.md#the-loop) explains the loop and alpha.

[api/ecs/Previous.h](../../api/ecs/Previous.h) does this with a second component,
`ecs::Previous<T>`:

| Function | What it does |
|---|---|
| `snapshot<T>(registry)` | Copies every entity's `T` into its `Previous<T>`, adding one if missing, and removes `Previous<T>` from every entity that no longer has a `T`. |
| `settle<T>(registry, entity)` | Makes an entity's previous value equal to its current one. |
| `interpolated<T>(registry, entity, alpha)` | Blends previous to current by `alpha`. An entity with no previous value yet gives its current value. |

The rules:

- **Call `snapshot<T>` at the top of `simulate()`, before anything moves**, once for each type
  you draw between steps. Every entity with a `T` is snapshotted whether or not it is about to
  move, so a stopped entity is drawn where it stopped.
- **Call `settle<T>` after a teleport, once the new value is written.** It copies the value the
  entity has when it is called. An entity put somewhere rather than moved there is otherwise
  drawn sweeping across the screen for one frame, and nothing reports it.
- An entity that loses its `T` and is given one again in a later step is drawn at the new
  value, not blended from the value it lost. One that loses and regains it within one step keeps
  its `Previous<T>`, so call `settle<T>` for it.
- **`T` must be copyable and have an `interpolate(const T&, const T&, float)`** in its own
  namespace, found by argument-dependent lookup. A type without one fails to compile in
  `interpolated()`, rather than snapping silently. `grid::TileCoord` deliberately has none; a
  game that needs a unit to glide between tiles interpolates a position of its own.
- An entity created during a step has no previous value the first time it is drawn, and is drawn
  at its current value.

```cpp
bool MyEngine::simulate(float step) {
    v3d::ecs::snapshot<v3d::ecs::component::Transform>(registry_);
    v3d::ecs::snapshot<v3d::ecs::component::Playback>(registry_);
    // ... move things ...
    v3d::ecs::component::advance(registry_, step);
    return true;
}
```

pong snapshots `Position2D` and `Position1D` this way.

Background: [ADR-0060](../adr/0060-ecs-interpolate-from-a-previous-step-component.md)

## How entities are drawn

An entity is drawn from a `Transform` plus one component for each kind of drawing. The drawing
components live in `api/render/realtime/component/`, beside the renderer handles they name, so
`api/ecs` stays free of any device:

| Component | Draws the entity as |
|---|---|
| `realtime::component::Sprite` | An upright quad facing the camera, standing on the transform's position. |
| `realtime::component::Mesh` | A model from a `MeshRegistry`, lit and optionally casting a shadow. |
| `realtime::component::Particles` | The particles of the entity's `ecs::component::Emitter`. |

The api provides the functions that find these entities and draw them: `realtime::sprites()`,
`realtime::particles()`, `realtime::meshes()` and `realtime::casters()`. Each takes the registry
and `alpha()`, and reads `Transform` through `interpolated<Transform>`. An entity whose game
snapshots `Transform` is therefore drawn between steps with no further work.

A sprite ignores its transform's rotation, because a billboard faces the camera. A game shows
which way a sprite faces by choosing its region. A `Sprite` holds the region's texture
coordinates (`uv0`, `uv1`), not the region's name, so a game that reloads a sprite sheet must
look its regions up again and write them back. [rendering/](rendering/README.md) covers these
functions and the components in full.

Background: [ADR-0063](../adr/0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md)

## Notes

- A registry can call a listener when a component is constructed, destroyed or updated. Nothing
  in the tree uses this.
- The `entt::dispatcher` the engine creates is separate from the registry. The engine builds it
  in `initialize()` and hands it to the event and input engines. An app that plays sound builds
  an `audio::Engine` over the same dispatcher, so a sound event published anywhere reaches it.
  See [engine/Audio.md](engine/Audio.md#audio).
