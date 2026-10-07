# Drawing ECS entities

The api draws entities from the ECS registry for you. The components themselves are described
in [ECS.md](../ECS.md); this page covers the functions that draw them.

## Drawing ECS entities

An entity is drawn from an `ecs::component::Transform` plus a component for the kind of drawing.
The drawing components are in `api/render/realtime/component/`. The api provides a function per
kind that walks the registry and submits the draws. Each reads the transform between the last two
simulation steps, using `Engine::alpha()`, for any entity whose game keeps a previous step. How
the ECS works is in [ECS.md](../ECS.md).

| Component | Function | Draws into |
|---|---|---|
| `component::Sprite` | `sprites(registry, alpha, right, up, depthAxis, &order)` | a `DepthOrder` |
| `component::Particles` | `particles(registry, alpha, right, up, depthAxis, &order)` | a `DepthOrder` |
| `component::Mesh` | `meshes(...)` and `casters(...)` | a lit pass and a shadow pass |

```cpp
DepthOrder order;
order.clear();
const glm::vec3 right = camera.profile().right();
const glm::vec3 up = camera.profile().up();
sprites(registry, alpha(), right, up, depthAxis, &order);
particles(registry, alpha(), right, up, depthAxis, &order);
world.clear();
order.into(&world);
renderer_->worldQuads()->submit(world, pass.get());
```

- **Sprite**: an upright quad facing the camera, its bottom edge centred on the transform's
  position. Width scales by the transform's x scale and height by y. The transform's rotation
  is ignored; choose the region to show which way the sprite faces.
- **Particles**: the particles of an `ecs::component::Emitter`, each a quad centred where it
  stands, sized and coloured by the emitter's tracks over its life. A particle faces the camera,
  or with `Facing::Velocity` is stretched along its velocity (rain, sparks). A sprite clip plays
  by the particle's age, or once over its life with `overLife`. Sway is applied here along the
  camera's right and never reaches the simulation. Particles need no `Transform`.
- The key is the distance along `depthAxis`, larger being further. Use the camera's forward
  flattened onto the ground for an orthographic view of a ground plane, or the view direction for
  a perspective view. Sprites and particles in one `DepthOrder` sort among each other.
- **Mesh**: a `MeshHandle` and `castsShadow`. The material belongs to the registry entry, so two
  entities drawing one model look the same.
