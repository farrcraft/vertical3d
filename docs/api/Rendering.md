# Rendering

This document is for someone drawing from an app with `api/render/realtime`. It covers what an
app builds each frame, the three drawing primitives, meshes and the lit pass, post passes,
colour, and the rules an app has to follow. How the renderer works inside is in
[internals/RealtimeRenderer.md](../internals/RealtimeRenderer.md).

- [The objects an app touches](#the-objects-an-app-touches)
- [A frame and its passes](#a-frame-and-its-passes)
- [Choosing a target](#choosing-a-target)
- [2D: the quad canvas](#2d-the-quad-canvas)
- [Lines](#lines)
- [World quads and sprites](#world-quads-and-sprites)
- [Textures](#textures)
- [Meshes and the mesh registry](#meshes-and-the-mesh-registry)
- [The lit pass](#the-lit-pass)
- [Post passes and the colour grade](#post-passes-and-the-colour-grade)
- [Colour](#colour)
- [Cameras](#cameras)
- [Drawing ECS entities](#drawing-ecs-entities)
- [Reading a frame back](#reading-a-frame-back)
- [Releasing resources](#releasing-resources)
- [Resize and minimize](#resize-and-minimize)
- [What is not built yet](#what-is-not-built-yet)

All names below are in `v3d::render::realtime` unless a namespace is given. `vulkan::` means
`v3d::render::realtime::vulkan`.

## The objects an app touches

An app creates one `Engine3D` in its `start()` and calls `initialize(window())` on it. That
builds everything else.

| Object | What it is to an app |
|---|---|
| `Engine3D` | The renderer. It holds the frame being built and draws it with `renderFrame()`. |
| `DeviceContext` | Everything that needs only the GPU: textures, renderers, the frames in flight. `Engine3D::context()` returns it. |
| `Context3D` | A `DeviceContext` that draws into a window. It adds the swapchain and presenting. An app rarely names it. |
| `Frame` | Everything to draw for one image, as a list of passes. `Engine3D::frame()` returns it. |
| `Pass` | One pass of a frame: where it draws, how it clears, its camera, and the draw items in it. |
| `DrawItem` | A description of one draw. The primitives below create these for you. |

A *frame in flight* is a frame the CPU has finished recording but the GPU may still be drawing.
There are two. Anything an app hands to the renderer can still be in use by the GPU for up to
two frames after the app lets go of it. The renderer handles this for every object it owns.

`Engine3D` gives an app the three primitives and the texture store directly:

```cpp
renderer_->quads();       // vulkan::renderer::Quad - the 2D quad canvas
renderer_->lines();       // vulkan::renderer::Line - world-space lines
renderer_->worldQuads();  // vulkan::renderer::World - world-space textured quads
renderer_->textures();    // Textures - uploads images and makes them drawable
```

`lines()` and `worldQuads()` are built on their first call, so an app that never asks for one
pays nothing for it. `quads()` and `textures()` are null before `initialize()`.

## A frame and its passes

An app builds a frame during `render()` and draws it once at the end with `renderFrame()`. This
is the whole of a 2D app's drawing, taken from [examples/starter](../../examples/starter/src/AppEngine.cxx):

```cpp
bool AppEngine::render() {
    glm::ivec2 size;
    if (!renderer_->beginFrame(&size)) {
        return true;    // the window has no area; nothing to draw
    }
    canvas_.resize(size.x, size.y);
    canvas_.clear();
    canvas_.rect(glm::vec2(40.0f, 40.0f), glm::vec2(size.x - 40.0f, size.y - 40.0f),
        glm::vec4(0.9f, 0.4f, 0.2f, 1.0f));

    boost::shared_ptr<Pass> pass = renderer_->frame()->pass(Engine3D::colourPass);
    renderer_->quads()->submit(canvas_, pass.get());

    renderer_->renderFrame();
    return true;
}
```

`renderFrame()` records the frame, presents it, and empties every pass ready for the next frame.
A pass keeps its settings (target, clear colour, camera, depth) across frames. Only its draw
items are dropped.

### The colour pass

Every frame has a pass named `Engine3D::colourPass` (`"colour"`). It draws into the window and
clears to the colour set with `Engine3D::clearColour()`. A 2D app usually needs nothing else.

### Adding passes

`Frame::pass(name)` returns the pass with that name, creating it at the end of the list if
there is none. More drawing means more passes, not a different kind of frame. An editor draws
one pass per viewport of the same scene; a shadow map is a pass into an offscreen target.

A pass has these settings:

| Call | Effect | Default |
|---|---|---|
| `clearColour(colour)` / `keepColour()` | Clear the target before drawing, or draw over what is there. | Clears |
| `depth(bool)` | Test depth. 2D passes do not; they rely on drawing order. | Off |
| `target(renderTarget)` | Draw into an offscreen target instead of the window. | The window |
| `reads(renderTarget)` | Declare that this pass samples a target another pass draws. | None |
| `viewport(x, y, w, h)` | The region of the target to draw into, in pixels. Zero width or height means all of it. | All |
| `camera(view, projection)` | The camera for every item in the pass. | Identity |
| `sort(bool)` | Record items in sort-key order instead of submission order. | Off |
| `scene(set)` / `depthBias(...)` | Used by the lit pass. See [The lit pass](#the-lit-pass). | None |

Rules:

- **The first pass to draw into a target in a frame should clear it.** A pass that keeps colour
  draws over whatever the previous pass left. Depth is cleared exactly when colour is.
- **Leave sorting off for 2D.** An unsorted pass draws items in the order they were submitted,
  which is painter's order. Sorting groups items by pipeline and texture, which would put a
  panel on top of the text drawn on it.
- **Turn sorting on for a depth-tested scene with many objects.** Grouping lets the renderer
  skip rebinding the same pipeline and texture. The sort is stable, so equal keys keep their
  submission order.
- **A layer only matters in a sorted pass.** `submit(canvas, pass, layer)` takes a layer, and a
  sorted pass draws lower layers first. An unsorted pass ignores it.

### Pass order

Passes are recorded in the order they were created, with one exception. A pass that names a
target in `reads()` is recorded after every pass that draws into that target. So a scene pass
that reads a shadow map can be created first and still draw second.

- Passes that draw into the same target, the window included, keep their creation order.
- A pass that samples a target without naming it in `reads()` is recorded where it was created,
  and reads whatever the target holds at that point.
- Two passes that each read what the other draws throw `std::runtime_error`.
- A pass that names its own target in `reads()` is reading that target's previous frame. That
  orders nothing.

Background: [ADR-0068](../adr/0068-rendering-order-passes-by-what-they-read.md)

## Choosing a target

A pass draws into the window unless it names a `vulkan::frame::RenderTarget`. A render target
is an offscreen image that a later pass can sample.

```cpp
using vulkan::frame::RenderTarget;
auto context = renderer_->context();

// colour and depth, the size of a scene view
auto scene = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    width, height, VK_FORMAT_R8G8B8A8_SRGB, /* depth */ true);

// depth only, sampled later: a shadow map
auto map = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    2048, 2048, VK_FORMAT_UNDEFINED, /* depth */ true, /* sampledDepth */ true);
```

The constructor is `RenderTarget(device, ring, width, height, colour, depth = false,
sampledDepth = false, images = 1)`.

- **Size and format are yours.** A target does not follow the window. An app that wants one the
  window's size calls `recreate(width, height)` when `beginFrame()` reports a new size.
- **`VK_FORMAT_UNDEFINED` for colour means no colour image.** Such a target must have sampled
  depth, or the constructor throws; a target with nothing to read is useless.
- **`sampledDepth` can change the depth format.** Not every depth format can be both drawn into
  and sampled. A pipeline that draws into the target must be built against
  `target->depthFormat()`.
- **A pass that both samples a target's depth and draws into it is not allowed.** The renderer
  does not detect it; the validation layer reports it.

### Sampling a target

After the last pass that draws into a target, the target is left readable. To draw it, register
it with `Textures`:

```cpp
TextureHandle sceneTexture = renderer_->textures()->texture(*scene);   // its colour
TextureHandle shadowMap = renderer_->textures()->depthTexture(*map);   // its depth
```

`texture()` and `depthTexture()` return the white texture for a target with nothing of that
kind to read.

**Register again after a resize.** A registration keeps the target's old images alive and goes
on drawing them. After `recreate()`, release the old handle and register the target again.

### Reading last frame

A target built with `images` equal to the ring's frames in flight
(`context->ring()->framesInFlight()`) holds one image per frame. A pass draws into the current
one, and `previous()` is what the frame before drew. This is how a pass reads its own last
frame, for trails or feedback effects.

- Register each slot once: `textures->texture(*target, slot)` for every slot.
- Each frame, draw with the handle for `target->previous()`.
- Every slot starts cleared to transparent black and readable, so `previous()` can be read on the
  first frame.

### Pipelines and formats must match

A pipeline is built for one colour format and one depth format. **A pipeline drawn into a pass
with different formats throws `std::runtime_error` naming the pass.** The renderers
`Engine3D` gives you are built for the window's format. To draw into a target of another format,
build that renderer yourself against the target's formats (see
[The lit pass](#the-lit-pass) for an example with `World`).

## 2D: the quad canvas

Every 2D thing — a rectangle, a sprite, a glyph, a UI panel — is a textured quad. An app fills a
`Canvas` on the CPU and hands it to `quads()->submit(canvas, pass)`.

```cpp
Canvas canvas;
canvas.resize(width, height);    // the area drawn into, in pixels
canvas.clear();                  // at the start of every frame

canvas.rect(min, max, colour);                                  // untextured
canvas.rect(min, max, uv0, uv1, glm::vec4(1.0f), textureHandle); // textured; colour multiplies the texel
canvas.circle(centre, radius, 32, colour);
canvas.arc(centre, radius, 16, start, sweep, colour);           // a filled wedge
canvas.ring(centre, outer, inner, 16, start, sweep, colour);    // a band between two radii

canvas.push();
canvas.translate(glm::vec2(100.0f, 50.0f));
canvas.scale(glm::vec2(2.0f));
// ... drawn moved and scaled ...
canvas.pop();

renderer_->quads()->submit(canvas, pass.get());
```

- Coordinates are pixels with the origin at the top left and y down.
- Angles for `arc` and `ring` are radians. Zero points right and a quarter turn points down.
- An untextured quad is drawn against a 1×1 white texture, so it costs no extra draw.
- The canvas cuts a new draw only when the texture, the text flag or the clip changes. Group
  quads that share a texture to keep draws down.
- A canvas copies nothing to the GPU until `submit`. An empty canvas costs nothing.
- **Any number of canvases may be submitted in a frame**, into any passes. Each gets its own
  GPU buffers.
- A `Canvas` does not touch Vulkan, so it can be built and tested without a GPU.

### A canvas's own coordinate space

A canvas can draw in units of the game's own instead of pixels:

```cpp
canvas.space(glm::vec2(800.0f, 600.0f), Canvas::Fit::Contain);
```

- The space's origin is its top left and y grows down.
- `Fit::Stretch` fills the canvas and stretches the space where the aspect ratios differ.
- `Fit::Contain` keeps the space's aspect ratio, as large as fits, centred with empty bars on
  either side.
- `viewport()` returns where the space lands, in pixels. `toSpace(pixel)` converts a cursor
  position into the space's units.
- The space is kept across `clear()` and `resize()`. A size of zero goes back to pixels.

Each canvas carries its own projection, so a game in a space and a menu in pixels can be two
canvases in one pass. Pong draws its court and its menu this way.

### Clipping

```cpp
canvas.clip(glm::vec2(10.0f, 10.0f), glm::vec2(200.0f, 120.0f));
// ... everything here is cut to that rectangle ...
canvas.unclip();
```

- The rectangle is in the coordinates being drawn in. The current transform applies to it at
  the moment `clip()` is called, so translating afterwards moves the content, not the clip.
- A clip inside a clip is intersected with it, so an inner clip can only shrink the area.
- Clipping is done by the GPU per draw. A quad that crosses the edge is drawn whole and only
  the part inside appears.
- With a space set, the rectangle is in the space's units and is converted to pixels for you.
- The clip rectangle is placed relative to the image, not to the pass's viewport. A canvas
  drawn into a pass whose viewport is offset from the image's corner is clipped in the wrong
  place.

### Text

`Canvas::text(textBuffer, atlas)` copies text that a `v3d::font` text buffer has already laid
out. The atlas is a single-channel texture of signed distances. The quad shader turns the
distance into a smooth edge about one pixel wide at any scale. Text and other quads that
sample the same atlas are drawn as separate batches.

### User interface

`api/ui` draws onto a `Canvas` too, so a game and its menus share the same uploads and draws.
How to build a UI is in [UserInterface.md](UserInterface.md).

## Lines

`LineCanvas` draws one-pixel lines in **world space**, through the camera of the pass. The editor's
grid, axes, wireframes and manipulators are lines.

```cpp
LineCanvas lines;
lines.clear();
lines.line(from, to, colour);
lines.polyline(points, colour, /* closed */ true);
lines.box(min, max, colour);                                 // twelve edges
lines.circle(centre, axisU, axisV, radius, 32, colour);      // axisU and axisV need not be unit or perpendicular

lines.push();
lines.transform(modelMatrix);   // or translate(offset)
// ...
lines.pop();

renderer_->lines()->submit(lines, pass.get());
```

- A pass with no camera set draws the lines in clip space.
- **In a pass with depth, lines test and write depth**, so solid geometry in front hides them.
  To draw lines over a scene instead, submit them to a pass without depth.
- Lines are always one pixel wide.
- `LineCanvas::clip()` takes a rectangle in the pixels of the image, not in world space. The
  transform does not apply to it.

## World quads and sprites

`WorldCanvas` draws textured quads with four corners in **world space**, through the pass camera.
Use it for sprites standing on a ground plane, particles and tile highlights.

```cpp
WorldCanvas world;
world.clear();
world.tint(glm::vec4(1.0f, 0.8f, 0.6f, 1.0f));   // multiplies every quad added after it
WorldCanvas::Corners corners = {a, b, c, d};      // in order around the edge
world.quad(corners, colour);
world.quad(corners, uv0, uv1, glm::vec4(1.0f), textureHandle);   // uv0 at the first corner, uv1 at the third

renderer_->worldQuads()->submit(world, pass.get(), 0, vulkan::renderer::World::Blend::Alpha);
```

- Corners go in perimeter order, as `grid::tileCorners` returns them. Any convex quad comes out
  whole.
- `tint()` lasts until it is set again. `clear()` resets it to white.
- `Blend::Alpha` draws straight alpha over what is there. `Blend::Additive` adds the colour
  scaled by alpha and only ever lightens, so flames and sparks give the same result in any order.
  A canvas is drawn with one blend, so use one canvas per blend.
- A pass with no camera set draws world quads in clip space.
- World quads are not clipped by a scissor rectangle and are not turned to face the camera.
  For a billboard, build the corners from the camera's right and up vectors.
- Neither face of a world quad is culled, so its winding never hides it.
- A world canvas and a line canvas in the same pass are not sorted against each other by
  depth. The `layer` argument decides which is drawn first.

### Depth order

**Quads draw in the order they were added.** What "further away" means is the game's decision:
in an isometric view a sprite is behind another when its feet are further up the ground plane,
not when it is further from the camera. In a pass with depth, world quads test depth but do not
write it. Solid geometry hides a world quad, and one world quad never hides another.

`DepthOrder` sorts quads for you by a key you compute:

```cpp
DepthOrder order;
order.clear();
order.quad(key, corners, uv0, uv1, colour, texture);   // larger key = further away
order.into(&world);                                     // furthest first
```

Equal keys are grouped by texture and otherwise keep the order they were added. Rounding the
key to a tile row means fewer texture changes and so fewer draws.

### How the three primitives treat depth

| Primitive | In a pass with depth |
|---|---|
| `Canvas` quads | Neither test nor write. A UI stays on top of whatever the scene drew. |
| `LineCanvas` lines | Test and write. |
| `WorldCanvas` quads | Test, but do not write. |

## Textures

`Textures` (from `renderer_->textures()` or `context->textures()`) uploads images and makes
them drawable.

```cpp
boost::shared_ptr<Textures> textures = renderer_->textures();
TextureHandle sprite = textures->texture(image);                     // a v3d::image::Image
TextureHandle atlas = textures->texture(pixels, width, height, 1);    // raw bytes; 1 channel for a glyph atlas
MaterialHandle material = textures->material(sprite);                 // for a DrawItem of your own
textures->release(sprite);                                            // see Releasing resources
```

- A texture is uploaded as display-space `UNORM` unless you pass
  `TextureFactory::Encoding::Srgb` (see [Colour](#colour)).
- A single-channel image is read as a coverage mask: white, with the channel as alpha.
- `white()` is the 1×1 white texture. It is never released.
- **Upload at load time.** An upload waits for the GPU to finish the copy, so uploading during
  play stalls the frame.

A canvas names textures by `TextureHandle`. A `DrawItem` names a `MaterialHandle`, which binds
the texture for drawing. `material()` makes one on first use and keeps it.

## Meshes and the mesh registry

`MeshRegistry` turns a model into GPU buffers once and hands back a `MeshHandle`. A hundred props
drawn from one file are one upload.

```cpp
MeshRegistry meshes(logger(), renderer_->context(), assets());
MeshHandle crate = meshes.load("models/crate.gltf");    // relative to the asset manager's root
MeshHandle floor = meshes.add("floor", model);           // a type::Model built in code
const MeshRegistry::Entry* entry = meshes.resolve(crate);  // nullptr once released
meshes.release(crate);
```

- `load()` reads glTF. Asking for the same path again returns the same handle.
- `add()` registers a model built in code under a name. Names and paths share one namespace.
  It can take an image per material where you have pixels instead of a file name.
- An entry is drawn one *part* at a time. A part is a range of the indices, an albedo texture
  and a base colour, and is one draw.
- A texture a model names that cannot be found is logged and drawn white.
- Parts that name the same image share one texture, across all entries.
- A model with a part outside its indices or materials is refused with `std::runtime_error`.
- The vertex layout is `type::Model::Vertex`: position, normal and uv, 32 bytes. A model with a
  skeleton is uploaded as `MeshRegistry::SkinnedVertex`, 56 bytes, which adds four joint
  indices and four weights.
- `clip(handle, name)` returns the index of a skinned entry's animation clip, for
  `ecs::component::play()`.

A registered mesh is drawn by the [lit pass](#the-lit-pass). Geometry an app builds and draws
itself (voxel's chunks, for instance) is a `vulkan::memory::Mesh` owned by the app. A
`DrawItem` refers to its buffers directly and is valid only while the mesh is alive.
`Mesh::describe(&item)` fills an item's geometry fields.

## The lit pass

`vulkan::renderer::Lit` draws registered meshes with light, cast shadows and an outline. A lit
model is three things: a `MeshRegistry` entry, an entity with `ecs::component::Transform` and
`component::Mesh`, and a `Lit`.

The look is cel shading: a key light plus a fill from above and opposite it, quantised into
three flat bands. A cast shadow drops a fragment one band.

### Setting it up

A lit scene draws linear light into an sRGB target (see [Colour](#colour)). Build the targets and
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
[`api/render/shaders/lit/lit.glsl`](../../api/render/shaders/lit/lit.glsl) to get the shared
blocks rather than copying them. A replacement that declares a binding the layout lacks fails
when the pipeline is built. One that only reorders block members fails silently and draws the
wrong picture.

`Lit::sceneLayout()` returns set 2's layout, for a pipeline of your own that reads the same scene.

## Post passes and the colour grade

`vulkan::renderer::FullScreen` draws one triangle covering a target, with a fragment shader you
supply as SPIR-V. Use it for any pass over a whole image: a blur, a tone map, a vignette.

```cpp
vulkan::renderer::FullScreen::Spec spec;
spec.name = "vignette";
spec.fragment = vignetteSpirv;                // reads uv at location 0
spec.sources = 1;                             // images bound at set 1, bindings 0 upwards
spec.colour = swapchainFormat;                // what the pass draws into
vulkan::renderer::FullScreen post(device, cache, resources, ring, uniforms, spec);

MaterialHandle source = post.source({sceneTexture});   // once, and again after a resize
colour->reads(scene);
post.submit(source, colour.get());
```

- uv runs from 0 at the top left to 1 at the bottom right.
- The images are bound at set 1, one combined image sampler per binding. Set 0 is declared and
  need not be read.
- A source binds the images as they are now. After a target is resized, or a texture released,
  make a new source.
- Release every source with `post.release(source)` before the `FullScreen` is destroyed.

### Grade

`Grade` is a colour grade built on `FullScreen`: every pixel is looked up in a 16×16×16 colour
table.

```cpp
Grade grade(logger(), context, swapchainFormat, VK_FORMAT_UNDEFINED, strip);   // once
MaterialHandle source = grade.source(*scene);                                  // again after a resize

colour->reads(scene);
grade.submit(source, colour.get());
```

- The table comes from a 256×16 strip image: sixteen slices of blue side by side, red across each
  and green down it. No strip, or one of the wrong size, gives the identity table.
- The table is indexed by linear colour, so bake a strip for the linear light a lit scene holds.
- The scene is read one texel per pixel, so the grade must draw into a target the scene's size.
- `replace(texels)` changes the table from the next frame. Use it for a zone's own look, or lerp
  two tables yourself for a slow change. Call it before the frame's `submit()`. The texels are
  in the order `Grade::table()` returns. Sources keep their handles.
- `grade.release(source)` lets a source go. Release every source before the grade is destroyed.

## Colour

**By default, colour is display space.** The swapchain uses a `UNORM` format, so the value a
shader writes is the value on screen. Every colour and texture in an unlit app is authored in
display space, and textures are uploaded as `UNORM` to match. Blending happens in display space.

**An app that writes linear light chooses the swapchain format.** Pass a format to the
`Engine3D` constructor:

```cpp
renderer_ = boost::make_shared<Engine3D>(logger(), assets(), VK_FORMAT_B8G8R8A8_SRGB);
```

- If the surface offers that format in a non-linear sRGB colour space, the swapchain uses it.
- Otherwise it falls back to the `UNORM` default and logs a warning.
- **Build pipelines against `renderer_->context()->colourFormat()`**, the format that was
  actually chosen, never against the format you asked for.

**A lit scene works in linear light and draws into an `_SRGB` target**, which encodes the result
when it is stored. Its albedo textures are uploaded with `TextureFactory::Encoding::Srgb`, so
they are decoded to linear when sampled. `MeshRegistry` does this for you. A glTF base colour
factor is already linear. Every other texture keeps the `Display` default.

Background: [ADR-0009](../adr/0009-colour-display-space-unorm-swapchain.md),
[ADR-0066](../adr/0066-lighting-light-in-linear-draw-to-srgb.md)

## Cameras

A pass draws through the view and projection given to `Pass::camera(view, projection)`. Lines,
world quads and lit meshes use it; canvas quads do not. Build both matrices with
`v3d::type::camera::Camera`, which produces Vulkan clip space (y down, depth from 0 to 1).
The camera's conventions, including its handedness, are in [Types.md](Types.md).

For billboards, the walks below need the camera's right and up vectors and a depth axis.

## Drawing ECS entities

An entity is drawn from an `ecs::component::Transform` plus a component for the kind of drawing.
The drawing components are in `api/render/realtime/component/`. The api provides a function per
kind that walks the registry and submits the draws. Each reads the transform between the last two
simulation steps, using `Engine::alpha()`, for any entity whose game keeps a previous step. How
the ECS works is in [ECS.md](ECS.md).

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

## Reading a frame back

`vulkan::frame::Capture` copies a drawn image into CPU memory and writes it as a PNG. It is for
code that records frames itself, such as a headless context or a device test. `Engine3D` has no
capture hook.

It takes two calls, because a GPU submit has to complete between them:

```cpp
vulkan::frame::Capture capture(device, logger);
vulkan::frame::Capture::Source source;
source.image = target->image();
source.extent = target->extent();
source.format = target->format();
source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   // where the recorder left it
capture.record(commands, source);    // into the frame's command buffer, after recording
// ... submit, and wait for that submit to complete ...
capture.write("frame.png");
```

- A swapchain image can be captured with `record(commands, swapchain, imageIndex)` after the
  frame is recorded and before it is presented.
- The image is returned to the layout it arrived in, so a captured frame presents normally.
- **A capture recorded and never written costs a copy and is thrown away silently.**
- A source with `depth` set copies depth instead of colour, and `capture.depth()` returns one
  float per pixel. Only `VK_FORMAT_D32_SFLOAT` can be read this way.
- The readback buffer is allocated on the first `record()`.

Testing with captures is covered in [contributing/Testing.md](../contributing/Testing.md).

## Releasing resources

**A released handle stops working at once, and the GPU object behind it lives on until no frame
in flight can use it.** A handle holds a slot and a generation. A released slot is reused with
a new generation, so an old handle never comes to mean something else; it resolves to nothing.

| Resource | Release with |
|---|---|
| A texture, and its material | `textures->release(handle)` |
| A registered mesh | `meshRegistry.release(handle)`. Its albedos go with the last entry naming them. |
| A post pass source | `fullScreen.release(source)`, `grade.release(source)` |
| A target registration after a resize | `textures->release(handle)`, then register again |
| Pipelines | Never released. They live as long as the context. |
| Your own `vulkan::memory::Mesh` | Yours. Keep it alive while any draw item names it. |

**Release the renderer in `release()`.** The engine calls an app's `release()` before it
destroys the window. The GPU device keeps the window's surface alive, so the renderer must be gone
by then or the surface leaks. In `release()`:

1. Call `renderer_->shutdown()`. It waits for the GPU to finish everything in flight.
2. Drop everything you built from the context: a `MeshRegistry`, a `Lit`, a `Grade`, your
   render targets and your own renderers.

`MeshRegistry` destroys its meshes at once when it is destroyed, so destroy it only after the
GPU is idle. The app lifecycle is described in [Engine.md](Engine.md).

## Resize and minimize

- **`beginFrame(&size)` is how an app learns the window size.** No resize event reaches a
  renderer. Resize canvases, and recreate targets that track the window, when the size changes.
- **When the window is minimized, `beginFrame()` returns false.** It has already presented an
  empty frame. Draw nothing and return. Drawing starts again on its own when the window has an
  area.
- The swapchain is rebuilt when the GPU reports it out of date. A frame may be dropped when this
  happens; the next one draws at the new size.
- The renderers `Engine3D` gives you keep working across a resize. Their pipelines do not need
  rebuilding.

## What is not built yet

- **Merging draws.** Sorting groups items that could share a draw, but each item is still its
  own draw call.
- **More than one depth buffer for the window.** All passes into the window share one. The
  editor's four viewports work because their regions do not overlap and each pass clears its own.
  Two passes needing different depth over the same pixels would not work. An offscreen target
  has its own depth.
- **Wide lines.** Lines are one pixel.
- **Uploading during play.** Every upload waits for the GPU. There is no transfer queue.
- **A lit scene in an app.** The lit pass, shadows and the grade are exercised by the device
  tests in `api/render/tests/device`; no app in the tree draws one.
