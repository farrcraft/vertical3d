# ECS Design

The ECS is [entt](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system).
This document is what the tree does with it. What an entity carries to be drawn is
[ADR-0063](adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md), and
[RenderingPipeline.md](RenderingPipeline.md#how-this-meets-the-ecs) describes the walk that
draws it.

## What exists

`v3d::engine::Engine` holds the `entt::registry` by value as a protected member, so an app's
`Controller` inherits it and passes `&registry_` to whatever needs it — the render engine takes
a raw `entt::registry*`. There is no accessor; only a subclass reaches it.

`v3d::ecs::System` ([api/ecs/System.h](../api/ecs/System.h)) is all the system support there
is: a registry pointer and a `virtual bool simulate(float step)`.
`odyssey::system::Movement` is the one subclass in the tree.

`api/ecs/component/` holds the components more than one app could want, and there are six:
`Color3`, `Playback`, `Position1D`, `Position2D`, `PositionFixed2D` and `Transform`. An app defines the rest
beside its own code — pong has `Score`, `Travel`, `Offset` and `PaddleSize`; odyssey has `Size`
and `Direction`.

`Transform` is where a thing stands in a 3D world: a position, a quaternion and a scale, with
`aboutY()` for a world that turns about one axis
([ADR-0063](adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md)). The
components that say how a thing is drawn are not here but in `api/render/realtime/component/`,
beside the handles they name, and `realtime::sprites()` walks every entity carrying a
`Transform` and a `Sprite`.

`Playback` is which of a model's clips an entity is playing, how far in, and the clip it is
fading out of ([ADR-0070](adr/0070-animation-is-sampled-from-playback-on-the-step.md)). A game
calls `play()` from whatever decides its clip, and `advance(registry, step)` in `simulate()`.
It holds no pose, and it is drawn between steps as a `Transform` is, so a game snapshots both.
`crossed(previous, current, marker)` says whether a step passed a time in the clip, such as a
footstep. It is the one component here that needs `api/type`, for the clock.

An entity's components are emplaced by the class that owns the entity id, and read back
through the registry:

```
registry->emplace<v3d::ecs::component::Position2D>(id_, 0.0f, 0.0f);
...
v3d::ecs::component::Position2D& position = registry_->get<v3d::ecs::component::Position2D>(id_);
```

`try_get` is the guarded form, and odyssey's renderer uses it to draw the player only when the
component is there.

[api/ecs/Previous.h](../api/ecs/Previous.h) keeps what a component was before the last
simulation step, so a renderer can draw between steps
([ADR-0060](adr/0060-a-moving-thing-keeps-its-previous-step.md)). `snapshot<T>(registry)` at the
top of `simulate()` copies every entity's `T` into its `Previous<T>`; `settle<T>` makes the two
equal after a teleport; `interpolated<T>(registry, entity, alpha)` blends them through an
`interpolate(const T&, const T&, float)` found beside `T`. `Position1D` and `Position2D` have
one, and `PositionFixed2D` deliberately does not, so `interpolated` refuses it at compile time.

## Notes

- A registry can bind a listener to component and entity lifecycle events: on construct, on
  destroy, and on update (patch). Nothing in the tree does yet.
- The `entt::dispatcher` beside it is not part of the ECS work. `Engine::initialize` creates it
  and hands it to the event, input and audio engines, which is how a sound event reaches
  `audio::Engine`.
