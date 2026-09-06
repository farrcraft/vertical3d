# ECS Design

The ECS is [entt](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system).
This document is notes plus what the tree actually does today; the design question it was
written around — what a renderable component looks like — is still open. See
[RenderingPipeline.md](RenderingPipeline.md#still-open-how-this-meets-the-ecs).

## What exists

`v3d::engine::Engine` holds the `entt::registry` by value as a protected member, so an app's
`Controller` inherits it and passes `&registry_` to whatever needs it — the render engine takes
a raw `entt::registry*`. There is no accessor; only a subclass reaches it.

`v3d::ecs::System` ([api/ecs/System.h](../api/ecs/System.h)) is all the system support there
is: a registry pointer and a `virtual bool tick()`. `odyssey::system::Movement` is the one
subclass in the tree.

`api/ecs/component/` holds the components more than one app could want, and there are four:
`Color3`, `Position1D`, `Position2D` and `PositionFixed2D`. An app defines the rest beside its
own code — pong has `Score`, `Travel`, `Offset` and `PaddleSize`; odyssey has `Size` and
`Direction`.

An entity's components are emplaced by the class that owns the entity id, and read back
through the registry:

```
registry->emplace<v3d::ecs::component::Position2D>(id_, 0.0f, 0.0f);
...
v3d::ecs::component::Position2D& position = registry_->get<v3d::ecs::component::Position2D>(id_);
```

`try_get` is the guarded form, and odyssey's renderer uses it to draw the player only when the
component is there.

## Notes

- A registry can bind a listener to component and entity lifecycle events: on construct, on
  destroy, and on update (patch). Nothing in the tree does yet.
- The `entt::dispatcher` beside it is not part of the ECS work. `Engine::initialize` creates it
  and hands it to the event, input and audio engines, which is how a sound event reaches
  `audio::Engine`.
