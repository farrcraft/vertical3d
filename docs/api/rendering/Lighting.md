# Lighting

The lit pass draws meshes with lights, shadows and skinning. Meshes come from the mesh
registry described in [TexturesAndMeshes.md](TexturesAndMeshes.md). The lighting settings and
the shadow functions are in `realtime::lighting`, in `api/render/realtime/lighting/`.

## A lit scene

`LitScene` is the lit pass with everything around it: the shadow map, the sRGB target the scene
is drawn into, the lit renderer, and the grade that carries the scene into the app's own pass.
An app builds one and calls `draw()` once a frame.

```cpp
auto context = renderer_->context();
scene_ = boost::make_shared<LitScene>(*context, LitScene::Spec{.width = width, .height = height,
    .output = context->colourFormat(), .shadowTexels = 2048, .cascades = 3});

void MyApp::tick(unsigned int delta) {
    glm::ivec2 size;
    if (!renderer_->beginFrame(&size)) {
        return;
    }
    const auto cascades = lighting::fitCascades(camera.projection() * camera.view(),
        lighting::splitView(near, 120.0F, 3, 0.75F), settings.light, {.texels = 2048});
    boost::shared_ptr<Frame> frame = renderer_->frame();
    Pass* lit = scene_->draw(frame.get(), frame->pass(WindowRenderer::colourPass).get(), registry(),
        *meshes_, {.view = camera.view(), .projection = camera.projection(), .settings = settings,
        .shadows = *cascades, .alpha = alpha(), .poses = entity::poses(registry(), alpha(), *meshes_)});
    // anything else drawn into the colour pass goes after the grade, such as the ui
    renderer_->renderFrame();
}
```

- **`Shot::shadows` is what the shadows are drawn through.** `std::monostate` draws none. A
  `glm::mat4` from `lighting::shadowLight()` is one map, and every caster is drawn into it. A
  `lighting::Cascades` from `fitCascades()` draws a shadow pass for each cascade, and leaves out
  a static caster that cannot reach it.
- **`draw()` builds the passes in an order that records correctly.** Each cascade's shadow pass
  draws into its layer of the map. The lit pass reads the map, and the pass the grade goes into
  reads the scene target, so the frame records shadow, lit, then the app's pass, whatever order
  the passes were made in.
- **The passes are named after the scene**: `Spec::name`, which defaults to `"lit"`, and
  `"lit shadow 0"` upwards. Two scenes in one frame have passes of their own.
- **The grade covers the whole of the pass it goes into.** Call `draw()` before anything else
  is submitted to that pass.
- **`draw()` returns the lit pass**, for draws of the app's own after the meshes, such as world
  quads built against `target()`'s formats.
- **Call `resize()` when the window changes size.** The scene target is replaced, and the grade
  reads the new one from the next frame.
- A scene built with `shadowTexels` of zero has no map. A frame that names shadows for it
  throws, and so does one that names more cascades than the map has layers.
- `lit()` and `grade()` give the parts the scene holds, for an app that adds draws to the lit
  pass or changes the grade's table.

The sections below describe those parts, for an app that composes the passes itself.

## The lit pass

`renderer::Lit` draws registered meshes with light, cast shadows and an outline. A lit
model is three things: a `MeshRegistry` entry, an entity with `ecs::component::Transform` and
`component::Mesh`, and a `Lit`.

The look is cel shading: a key light plus a fill from above and opposite it, quantised into
three flat bands. A cast shadow drops a fragment one band.

### Setting it up

A lit scene draws linear light into an sRGB target (see [Colour](ColourAndPost.md#colour)).
Build the targets and the renderer once:

```cpp
auto context = renderer_->context();
auto scene = boost::make_shared<RenderTarget>(*context,
    RenderTarget::Spec{.width = width, .height = height, .colour = VK_FORMAT_R8G8B8A8_SRGB, .depth = true});
auto map = boost::make_shared<RenderTarget>(*context,
    RenderTarget::Spec{.width = mapSize, .height = mapSize, .colour = VK_FORMAT_UNDEFINED, .depth = true, .sampledDepth = true,
        .images = 1, .layers = cascades});  // a layer per cascade

renderer::Lit lit(*context, {.colour = scene->format(), .depth = scene->depthFormat(), .shadow = map->depthFormat()});
```

Pass `VK_FORMAT_UNDEFINED` as the last format for a scene with no shadows. A shadow map of one
cascade has one layer, which is the default.

**A camera on `Profile::Hand::DirectionCrossUp` needs a counter-clockwise front face.** That
hand mirrors the image, which reverses the winding of every face on screen. `Lit` culls with a
clockwise front face unless told otherwise, which is right for the default hand:

```cpp
renderer::Lit lit(*context,
    renderer::Lit::Spec{.colour = scene->format(), .depth = scene->depthFormat(), .shadow = map->depthFormat(),
        .front = VK_FRONT_FACE_COUNTER_CLOCKWISE});
```

The cel, outline and shadow pipelines all cull by this face. The light's matrix therefore has to
be built in the camera's hand too: pass the hand to `lighting::shadowLight()`.

### Each frame

With one shadow map fitted by `fitShadow` or `fitShadowToView`:

```cpp
lighting::Settings settings;
const glm::mat4 light = lighting::shadowLight(settings.light, bounds);
VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, light, 1.0F / mapSize),
    context->textures()->depthTexture(*map));

boost::shared_ptr<Pass> pass = frame->pass("lit");       // may be created first
boost::shared_ptr<Pass> casting = frame->pass("shadow");
casting->target(map);
casting->depth(true);
casting->scene(sceneSet);
casting->depthBias(settings.constantBias, settings.slopeBias);
entity::casters(registry, alpha(), meshes, &lit, casting.get());
```

With cascades, there is a shadow pass for each, drawing into its own layer:

```cpp
const std::vector<float> splits = lighting::splitView(near, 120.0F, 3, 0.75F);
const std::optional<lighting::Cascades> cascades = lighting::fitCascades(
    camera.projection() * camera.view(), splits, settings.light, {.texels = mapSize});
VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, *cascades, 1.0F / mapSize),
    context->textures()->depthTexture(*map));

for (uint32_t cascade = 0; cascade < cascades->count; cascade++) {
    boost::shared_ptr<Pass> casting = frame->pass("shadow " + std::to_string(cascade));
    casting->target(map);
    casting->layer(cascade);
    casting->depth(true);
    casting->scene(sceneSet);
    casting->depthBias(settings.constantBias, settings.slopeBias);
    entity::casters(registry, alpha(), meshes, &lit, casting.get(), poses,
        {.cascade = cascade, .covers = cascades->bounds[cascade], .towards = settings.light});
}
```

Then the lit pass, either way:

```cpp

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
- **A shadow draw pushes its cascade.** The shadow pipelines declare one push constant,
  `Lit::Cascade`, which picks the matrix the draw goes through. `casters()` pushes the cascade
  its `Casting` names, which is zero by default.
- **The shadow map is read as an array of layers.** `scene()` takes the depth of a target built
  with sampled depth, of one layer or several, and throws for any other texture. With no shadow
  map, a map of one layer cleared to the far plane stands in, and nothing is shadowed.
- `meshes()` submits every entity's outline, then every entity's surface. Each entity is drawn
  between its last two simulation steps, using `alpha`.
- Entities that draw the same registry entry are drawn together: one draw per part, with an
  instance per entity. A scene's draw count follows how many distinct entries it shows, not how
  many entities. A copy of a model registered under a second key is a second entry, and is not
  grouped with the first.
- `casters()` submits only entities whose `component::Mesh::castsShadow` is set, grouped as
  `meshes()` groups them.
- **`casters()` culls against a cascade** when its `Casting` names the sphere the cascade
  covers. A static caster is left out when a sphere around its model, scaled by its transform,
  does not meet the box the cascade is drawn through (`lighting::casts()`). A skinned caster is
  always drawn, because its pose can reach past the bind pose its sphere is taken from. With no
  sphere named, every caster is drawn.
- An entity whose mesh handle was released is skipped.

### Fitting the shadow map

`lighting::fitShadow(registry, alsoCover, {.margin, .alpha})` returns a sphere around every shadow
caster: the mean of their positions, and the farthest of them plus `margin`. The margin has to
cover a caster's size and the length of its shadow. Things that cast nothing, such as the
ground, stay out of the fit. `alsoCover` adds points that must be inside the map, such as where
a character is walking to. `fitShadow` returns nothing when no entity casts. The radius is never
below `lighting::minimumShadowRadius` (0.01), so one caster with no margin still gives a usable
matrix. Each caster is read where it is drawn, `alpha` of the way from its previous step, as the
lit pass draws it. Pass `engine::Engine::alpha()`; the default of one reads the current step.

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

`lighting::shadowLight(towards, bounds, hand)` builds the matrix the shadow pass draws
through: an orthographic box around the sphere, seen from the light. `hand` is the camera's
`Profile::Hand`, and defaults to `UpCrossDirection`. A face is wound the same way under the
light as under a camera of that hand, so the shadow pipeline culls the same faces the cel
pipeline does. A light built in the other hand stores each caster's far side in the map.

**A map fitted by `fitShadow` does not follow the casters.** Fit it at scene load, or when the
casters move somewhere new. A map fitted by `fitShadowToView` every frame follows the camera
instead.

### Cascades

One map spread over a long view gives every texel a large piece of the world, and a near
shadow loses its detail. Cascades split the view by distance into up to
`lighting::maximumCascades` (4) slices, each with a layer of the shadow map of its own. A near
slice is small, so its texels are small.

- **Distances are measured from the camera's near plane, along its view direction.** A split is
  where a cascade ends.
- **`lighting::splitView(near, distance, count, lambda)`** gives the splits by the practical
  split scheme. `near` is the camera's near plane, its distance from the eye. Lambda 1 gives
  logarithmic splits, which keep each cascade's far end the same multiple of its near end, as
  the eye measures them. Lambda 0 gives cascades of equal length. Between, it blends the two;
  0.5 to 0.8 is usual. The last split is `distance`. It returns nothing for a near plane or a
  distance that is not positive and finite, or a count outside 1 to 4.
- **A game can give its own splits** instead, increasing and above zero.
- **`lighting::fitCascades(viewProjection, splits, towards, fit)`** fits a cascade to each slice,
  as `fitShadowToView` fits one map: a sphere whose radius holds still as the camera turns,
  with its centre moved to a whole texel. It returns the matrix each cascade is drawn through,
  the sphere it covers, where it ends, and the near plane. `CascadeFit` holds the layer's
  width in texels, the blend and the camera's hand.
- **Each cascade starts inside the one before it.** Cascade i covers from split i - 1, less the
  blend, to split i. The blend is a fraction of the split, 0.1 by default, held between 0 and
  0.5.
- **The lit pass picks a fragment's cascade by its distance from the near plane**: the first
  whose split is beyond it. Past the last split there is no shadow. Over the last part of a
  cascade, the blend, the fragment fades from that cascade's shadow into the next's, so the
  change in detail does not show as a line.
- **The normal bias grows with the cascade.** `Settings::normalBias` is for the first
  cascade, and each later one moves the lookup further, by its radius over the first's,
  because its texels are wider.
- `fitCascades` returns nothing for splits that are empty, more than 4, not increasing, or not
  above zero, a light direction that is zero or not finite, fewer than 4 texels, or a matrix
  that cannot be inverted.
- **One map is a single cascade.** `pack(settings, light, texel)` packs one matrix as a cascade
  that reaches every distance, so a map fitted by `fitShadow` or `fitShadowToView` is drawn as
  before.
- Each cascade's shadow pass is a pass of its own, so the pass timings show what each costs.

Background: [ADR-0115](../../adr/0115-rendering-cascaded-shadows-in-a-layered-map.md)

### Skinned models

Skinned models are drawn by the same calls with one more argument. Compute the poses once per
frame and pass the same `Poses` to all three:

```cpp
const entity::Poses poses = entity::poses(registry, alpha(), meshes);
VkDescriptorSet sceneSet = lit.scene(lighting::pack(settings, light, 1.0F / mapSize),
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
| `normalBias` | How far a lookup moves along the normal before reading the shadow map, in the first cascade. |
| `shadowStrength` | How much of a band a shadow removes. Zero still draws the shadow pass but ignores it. |

`lighting::pack(settings, lightViewProjection, texel)`, or `pack(settings, cascades, texel)`,
turns the settings into the `SceneUniforms` that `scene()` takes. Changing `colour` and `shadowColour` over time is how a
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
models and graded with them. `WindowRenderer::worldQuads()` is built for the window's format, so it
cannot draw into an sRGB scene target. Build a `World` against the target's formats:

```cpp
renderer::World litQuads(*context, renderer::World::Spec{.colour = scene->format(), .depth = scene->depthFormat()});
// ... after meshes():
litQuads.submit(worldCanvas, pass.get());
```

Colours given to world quads in this pass are linear.

### Replacing the lit shaders

`Lit` takes its shaders as SPIR-V words, defaulting to the ones built into the library
(`Lit::Shaders::embedded()`). A game can load its own, for example from disk, and pass them to
the constructor:

```cpp
renderer::Lit::Shaders shaders = renderer::Lit::Shaders::embedded();
shaders.cel = loadSpirv("shaders/my_cel.frag.spv");
renderer::Lit lit(*context,
    {.colour = scene->format(), .depth = scene->depthFormat(), .shadow = map->depthFormat(), .shaders = shaders});
```

A replacement must declare the same sets as the built-in shaders. A shadow stage declares one
push constant, a `uint` naming its cascade, at offset 0, and no other stage declares any:

| Binding | Holds |
|---|---|
| set 0, binding 0 | The camera (`Camera` block) |
| set 1, binding 0 | Every texture, an array of combined image samplers. An object's albedo is the one at its `albedo` slot |
| set 2, binding 0 | The scene (`Scene` block, laid out as `SceneUniforms`, with a light matrix, a far end and a radius for each of 4 cascades) |
| set 2, binding 1 | The shadow map, a `sampler2DArray` with a layer per cascade |
| set 2, binding 2 | The joint palette, a read-only storage buffer (skinned shaders) |
| set 2, binding 3 | The objects, a read-only storage buffer of `Lit::Object`: model matrix, base colour, outline, first joint, albedo slot (96 bytes each) |

Each vertex stage is given only the attributes the built-in one reads. Location 0 is the
position, 1 the normal and 2 the uv, and a skinned stage also gets the joints at 3 and the
weights at 4.

| Stage | Locations |
|---|---|
| `mesh`, `skinnedMesh` | 0, 1 and 2 |
| `outlineVertex`, `skinnedOutline` | 0 and 1 |
| `shadow`, `skinnedShadow` | 0 |

A replacement that reads a location its stage is not given is a validation error when the
pipeline is built. One that leaves a location unread is a warning.

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
