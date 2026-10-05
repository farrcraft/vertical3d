# Lighting

The lit pass draws meshes with lights, shadows and skinning. Meshes come from the mesh
registry described in [TexturesAndMeshes.md](TexturesAndMeshes.md).

## The lit pass

`vulkan::renderer::Lit` draws registered meshes with light, cast shadows and an outline. A lit
model is three things: a `MeshRegistry` entry, an entity with `ecs::component::Transform` and
`component::Mesh`, and a `Lit`.

The look is cel shading: a key light plus a fill from above and opposite it, quantised into
three flat bands. A cast shadow drops a fragment one band.

### Setting it up

A lit scene draws linear light into an sRGB target (see [Colour](ColourAndPost.md#colour)). Build the targets and
the renderer once:

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

### Each frame

```cpp
LitSettings settings;
const glm::mat4 light = shadow::light(settings.light, bounds.centre, bounds.radius);
VkDescriptorSet sceneSet = lit.scene(pack(settings, light, 1.0f / mapSize),
    context->textures()->depthTexture(*map));

boost::shared_ptr<Pass> pass = frame->pass("lit");       // may be created first
boost::shared_ptr<Pass> casting = frame->pass("shadow");
casting->target(map);
casting->depth(true);
casting->scene(sceneSet);
casting->depthBias(settings.constantBias, settings.slopeBias);
casters(registry, alpha(), meshes, lit, casting.get());

pass->target(scene);
pass->depth(true);
pass->reads(map);                                         // records the shadow pass first
pass->camera(camera.view(), camera.projection());
pass->scene(sceneSet);
meshes(registry, alpha(), meshes, lit, settings.outline, pass.get());
```

(`meshes()` here is the free function `realtime::meshes`, and `meshes` the registry.)

Rules:

- **Call `Lit::scene()` once per frame, while building the frame**, before `renderFrame()`. A
  second call in the same frame replaces the first.
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

`shadow::fit(registry, alsoCover, margin)` returns a sphere around every shadow caster: the mean
of their positions, and the farthest of them plus `margin`. The margin has to cover a caster's
size and the length of its shadow. Things that cast nothing, such as the ground, stay out of the
fit. `alsoCover` adds points that must be inside the map, such as where a character is walking
to. `fit` returns nothing when no entity casts.

`shadow::light(towards, centre, radius)` builds the matrix the shadow pass draws through: an
orthographic box around the sphere, seen from the light.

**The map does not follow the casters.** Fit it at scene load, or when the casters move
somewhere new.

### Skinned models

Skinned models are drawn by the same calls with one more argument. Compute the poses once per
frame and pass the same `Poses` to all three:

```cpp
const Poses poses = realtime::poses(registry, alpha(), meshes);
VkDescriptorSet sceneSet = lit.scene(pack(settings, light, 1.0f / mapSize),
    context->textures()->depthTexture(*map), poses.palette());
casters(registry, alpha(), meshes, lit, casting.get(), poses);
meshes(registry, alpha(), meshes, lit, settings.outline, pass.get(), poses);
```

- `poses()` samples each skinned entity's `ecs::component::Playback`, blending out of the clip
  it is leaving. An entity with no playback stands in its rest pose.
- **Pass the same `Poses` to `scene()`, `casters()` and `meshes()`.** A mismatch makes a draw
  read another entity's joints.
- A skinned entity the `Poses` does not name is skipped. Calls given no `Poses` draw static
  models only.
- A shadow is cast in the same pose that is drawn.

### LitSettings

`LitSettings` is plain data with no GPU in it, so a game can read its look from a file.

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

`pack(settings, lightViewProjection, texel)` turns the settings into the `SceneUniforms` that
`scene()` takes. Changing `colour` and `shadowColour` over time is how a scene's light changes
through a day.

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

A replacement must declare the same sets and push block as the built-in shaders:

| Binding | Holds |
|---|---|
| set 0, binding 0 | The camera (`Camera` block) |
| set 1, binding 0 | The albedo, a combined image sampler |
| set 2, binding 0 | The scene (`Scene` block, laid out as `SceneUniforms`) |
| set 2, binding 1 | The shadow map |
| set 2, binding 2 | The joint palette, a read-only storage buffer (skinned shaders) |
| push constant | `Lit::Object`: model matrix, base colour, outline, first joint (88 bytes) |

Front faces are clockwise. Include
[`api/render/shaders/lit/lit.glsl`](../../../api/render/shaders/lit/lit.glsl) to get the shared
blocks rather than copying them. A replacement that declares a binding the layout lacks fails
when the pipeline is built. One that only reorders block members fails silently and draws the
wrong picture.

`Lit::sceneLayout()` returns set 2's layout, for a pipeline of your own that reads the same scene.
