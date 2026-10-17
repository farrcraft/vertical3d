# Lighting

The lit pass draws meshes with lights, shadows and skinning. Meshes come from the mesh
registry described in [TexturesAndMeshes.md](TexturesAndMeshes.md). The lighting settings and
the shadow functions are in `realtime::lighting`, in `api/render/realtime/lighting/`.

## The lit pass

`vulkan::renderer::Lit` draws registered meshes with light, cast shadows and an outline. A lit
model is three things: a `MeshRegistry` entry, an entity with `ecs::component::Transform` and
`component::Mesh`, and a `Lit`.

The look is cel shading: a key light plus a fill from above and opposite it, quantised into
three flat bands. A cast shadow drops a fragment one band.

### Setting it up

A lit scene draws linear light into an sRGB target (see [Colour](ColourAndPost.md#colour)).
Build the targets and the renderer once:

```cpp
auto context = renderer_->context();
auto scene = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    width, height, VK_FORMAT_R8G8B8A8_SRGB, true);
auto map = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    mapSize, mapSize, VK_FORMAT_UNDEFINED, true, true);

vulkan::renderer::Lit lit(context->device(), context->pipelineCache(), context->resources(),
    context->ring(), context->frameUniforms(), context->textures(),
    scene->format(), scene->depthFormat(), map->depthFormat());
```

Pass `VK_FORMAT_UNDEFINED` as the last format for a scene with no shadows.

**A camera on `Profile::Hand::DirectionCrossUp` needs a counter-clockwise front face.** That
hand mirrors the image, which reverses the winding of every face on screen. `Lit` culls with a
clockwise front face unless told otherwise, which is right for the default hand:

```cpp
vulkan::renderer::Lit lit(context->device(), context->pipelineCache(), context->resources(),
    context->ring(), context->frameUniforms(), context->textures(),
    scene->format(), scene->depthFormat(), map->depthFormat(),
    vulkan::renderer::Lit::Shaders::embedded(), VK_FRONT_FACE_COUNTER_CLOCKWISE);
```

The cel, outline and shadow pipelines all cull by this face. The light's matrix therefore has to
be built in the camera's hand too: pass the hand to `lighting::shadowLight()`.

### Each frame

```cpp
lighting::Settings settings;
const glm::mat4 light = lighting::shadowLight(settings.light, bounds.centre, bounds.radius);
VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, light, 1.0f / mapSize),
    context->textures()->depthTexture(*map));

boost::shared_ptr<Pass> pass = frame->pass("lit");       // may be created first
boost::shared_ptr<Pass> casting = frame->pass("shadow");
casting->target(map);
casting->depth(true);
casting->scene(sceneSet);
casting->depthBias(settings.constantBias, settings.slopeBias);
entity::casters(registry, alpha(), meshes, &lit, casting.get());

pass->target(scene);
pass->depth(true);
pass->reads(map);                                         // records the shadow pass first
pass->camera(camera.view(), camera.projection());
pass->scene(sceneSet);
entity::meshes(registry, alpha(), meshes, &lit, settings.outline, pass.get());
```

(`entity::meshes()` is the free function in `api/render/realtime/entity/`, and `meshes` the
registry.)

Rules:

- **Call `Lit::scene()` once per frame, while building the frame**, before `renderFrame()`. A
  second call in the same frame replaces the first.
- **Call `scene()` before `casters()` and `meshes()`.** Each draw's object (its model matrix,
  base colour, outline and first joint) is added to the frame's scene, and the draw names it by
  index. A walk before any `scene()` throws. A walk before this frame's `scene()` adds to the
  scene of a frame that may still be drawing.
- **Both passes name the same scene set.** A pass that draws a lit pipeline without a scene set
  throws when the frame is recorded.
- **The shadow pass names a depth bias.** The shadow pipeline is built with depth bias, and a
  pass without one throws.
- With no shadow map passed to `scene()`, the white texture stands in and nothing is shadowed.
- `meshes()` submits every entity's outline, then every entity's surface. Each entity is drawn
  between its last two simulation steps, using `alpha`.
- `casters()` submits only entities whose `component::Mesh::castsShadow` is set.
- An entity whose mesh handle was released is skipped.

### Fitting the shadow map

`lighting::fitShadow(registry, alsoCover, margin, alpha)` returns a sphere around every shadow
caster: the mean of their positions, and the farthest of them plus `margin`. The margin has to
cover a caster's size and the length of its shadow. Things that cast nothing, such as the
ground, stay out of the fit. `alsoCover` adds points that must be inside the map, such as where
a character is walking to. `fitShadow` returns nothing when no entity casts. The radius is never
below `lighting::minimumShadowRadius` (0.01), so one caster with no margin still gives a usable
matrix. Each caster is read where it is drawn, `alpha` of the way from its previous step, as the
lit pass draws it. Pass `Engine::alpha()`; the default of one reads the current step.

`lighting::fitShadowToView(viewProjection, distance, towards, texels)` fits the map to what the
camera sees instead, for a world larger than one view. Call it every frame with the camera's
matrix.

- The sphere encloses the camera's view volume from the near plane out to `distance` along each
  edge, or to the far plane where that is nearer. Shadows beyond `distance` are not drawn.
- The radius depends only on the shape of the view volume, so the map's texels keep their size
  as the camera moves and turns.
- The centre is moved across the light to a whole number of texels. A point fixed in the world
  therefore stays at the same place within its texel, and a shadow's edge does not crawl as the
  camera moves. `texels` is the shadow map's width, at least 4.
- It returns nothing for a distance that is not finite and positive, a light direction that is
  zero or not finite, fewer than 4 texels, or a matrix that cannot be inverted.

`lighting::shadowLight(towards, centre, radius, hand)` builds the matrix the shadow pass draws
through: an orthographic box around the sphere, seen from the light. `hand` is the camera's
`Profile::Hand`, and defaults to `UpCrossDirection`. A face is wound the same way under the
light as under a camera of that hand, so the shadow pipeline culls the same faces the cel
pipeline does. A light built in the other hand stores each caster's far side in the map.

**A map fitted by `fitShadow` does not follow the casters.** Fit it at scene load, or when the
casters move somewhere new. A map fitted by `fitShadowToView` every frame follows the camera
instead.

### Skinned models

Skinned models are drawn by the same calls with one more argument. Compute the poses once per
frame and pass the same `Poses` to all three:

```cpp
const entity::Poses poses = entity::poses(registry, alpha(), meshes);
VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, light, 1.0f / mapSize),
    context->textures()->depthTexture(*map), poses.palette());
entity::casters(registry, alpha(), meshes, &lit, casting.get(), poses);
entity::meshes(registry, alpha(), meshes, &lit, settings.outline, pass.get(), poses);
```

- `poses()` samples each skinned entity's `ecs::component::Playback`, blending out of the clip
  it is leaving. An entity with no playback stands in its rest pose.
- **Pass the same `Poses` to `scene()`, `casters()` and `meshes()`.** A mismatch makes a draw
  read another entity's joints.
- A skinned entity the `Poses` does not name is skipped. Calls given no `Poses` draw static
  models only.
- A shadow is cast in the same pose that is drawn.

### Settings

`lighting::Settings` is plain data with no GPU in it, so a game can read its look from a file.

| Field | Meaning |
|---|---|
| `light` | Direction towards the key light, in world space. Keep it well off the camera's axis. |
| `fill` | Strength of the fill light added before banding. |
| `shadowThreshold`, `litThreshold` | Where the light crosses from the shadow band to the mid band, and mid to lit. |
| `bands` | What the albedo is multiplied by in the shadow, mid and lit bands. |
| `colour`, `shadowColour` | The light's colour (mid and lit bands) and the shadow band's colour. White leaves the bands unchanged. |
| `outline` | How far the outline is pushed out, in model units. Zero draws no outline. |
| `constantBias`, `slopeBias` | Depth bias for the shadow pass. Too little and a surface shadows itself; too much and a shadow detaches from its caster. |
| `normalBias` | How far a lookup moves along the normal before reading the shadow map. |
| `shadowStrength` | How much of a band a shadow removes. Zero still draws the shadow pass but ignores it. |

`lighting::pack(settings, lightViewProjection, texel)` turns the settings into the
`SceneUniforms` that `scene()` takes. Changing `colour` and `shadowColour` over time is how a
scene's light changes through a day.

### The outline

The outline is a second copy of the model, pushed out along each vertex normal and drawn with
its front faces culled, so only a rim shows around the silhouette.

- Its thickness is in model units, so it gets thinner on screen as the camera moves away.
- On a mesh with one normal per face, the rim breaks where two faces meet at the silhouette.
- On an open, single-sided mesh, the back of the hull shows as a black face. Use the outline
  on closed meshes.

### World quads in a lit scene

World quads can be drawn in the lit pass after its meshes, so they are depth-tested against the
models and graded with them. `Engine3D::worldQuads()` is built for the window's format, so it
cannot draw into an sRGB scene target. Build a `World` against the target's formats:

```cpp
vulkan::renderer::World litQuads(context->device(), context->pipelineCache(), context->resources(),
    context->ring(), context->frameUniforms(), context->textures(), scene->format(), scene->depthFormat());
// ... after meshes():
litQuads.submit(worldCanvas, pass.get());
```

Colours given to world quads in this pass are linear.

### Replacing the lit shaders

`Lit` takes its shaders as SPIR-V words, defaulting to the ones built into the library
(`Lit::Shaders::embedded()`). A game can load its own, for example from disk, and pass them to
the constructor:

```cpp
vulkan::renderer::Lit::Shaders shaders = vulkan::renderer::Lit::Shaders::embedded();
shaders.cel = loadSpirv("shaders/my_cel.frag.spv");
vulkan::renderer::Lit lit(..., scene->format(), scene->depthFormat(), map->depthFormat(), shaders);
```

A replacement must declare the same sets as the built-in shaders, and no push constants:

| Binding | Holds |
|---|---|
| set 0, binding 0 | The camera (`Camera` block) |
| set 1, binding 0 | The albedo, a combined image sampler |
| set 2, binding 0 | The scene (`Scene` block, laid out as `SceneUniforms`) |
| set 2, binding 1 | The shadow map |
| set 2, binding 2 | The joint palette, a read-only storage buffer (skinned shaders) |
| set 2, binding 3 | The objects, a read-only storage buffer of `Lit::Object`: model matrix, base colour, outline, first joint (96 bytes each) |

A vertex stage reads its object at `gl_InstanceIndex`, because a draw names its object as its
first instance. A fragment stage has no instance index, so its vertex stage passes the index
down as a flat output. The built-in `mesh.vert` passes it at location 3, and `cel.frag` reads
it there.

Front faces are clockwise. Include
[`api/render/shaders/lit/lit.glsl`](../../../api/render/shaders/lit/lit.glsl) to get the shared
blocks rather than copying them. In a vertex stage it also defines `object` as the stage's own
entry. A replacement that declares a binding the layout lacks fails when the pipeline is built.
One that only reorders block members fails silently and draws the wrong picture.

`Lit::sceneLayout()` returns set 2's layout, for a pipeline of your own that reads the same scene.
