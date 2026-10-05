# The Rendering Pipeline

What `api/render/realtime` does, as of 2026-09-06. Open questions are at the end.

The decisions behind its shape are [ADR-0001](../adr/0001-rendering-replace-opengl-with-vulkan.md) through
[ADR-0005](../adr/0005-2d-one-batched-quad-pipeline.md), plus
[ADR-0008](../adr/0008-shaders-descriptor-sets-by-update-frequency.md),
[ADR-0009](../adr/0009-colour-display-space-unorm-swapchain.md),
[ADR-0010](../adr/0010-meshes-owned-by-the-app-that-built-them.md),
[ADR-0011](../adr/0011-rendering-lines-as-a-world-space-primitive.md),
[ADR-0031](../adr/0031-rendering-passes-draw-into-offscreen-targets.md) and
[ADR-0042](../adr/0042-rendering-world-space-sprites.md). Those say why; this says what.

## The chain of objects

```
Window    ->  Context3D  ->  Frame  ->  Pass  ->  DrawItem
                  |
                  +-- vulkan::Swapchain   the images presented to the window
                  +-- vulkan::Presenter   acquire, submit, present, and the sync between them
                  |
              DeviceContext, the base - everything that needs only a device
                  +-- vulkan::Device      the gpu, its queues, and the 1.3 features
                  +-- vulkan::frame::Ring the frames in flight, and their buffers and fences
                  +-- vulkan::pipeline::Cache
                  +-- vulkan::Resources   pipelines, materials and textures, addressed by handle
                  +-- vulkan::FrameUniforms  set 0 - a camera per pass per frame in flight
                  +-- vulkan::Uploader    the one-shot queue everything device local is copied by
                  +-- vulkan::DepthBuffer the depth image, allocated the first frame a pass asks
                  +-- vulkan::renderer::Quad the 2D pipelines, and the geometry buffers they upload through
```

`realtime::Window` creates an `SDL_WINDOW_VULKAN` window and owns the `vulkan::Instance` and
`vulkan::Surface`. There is one window class, not a 2D one and a 3D one: every app has
presented through a swapchain since odyssey's port on 2026-09-01, and a 2D game differs only
in what its passes ask for, an orthographic projection and no depth. Everything that belongs
to the device is `DeviceContext`, and `Context3D` is that plus the window's chain and presenter
([ADR-0051](../adr/0051-frames-in-flight-ring-separate-from-presenting.md)). A context is told what it
draws into rather than asking a chain for it, which is what lets one exist with no window under
it at all. `Engine3D` drives one frame per tick.

There is no `VkRenderPass` and no `VkFramebuffer` anywhere. Passes draw through dynamic
rendering, straight into the swapchain image views, per
[ADR-0002](../adr/0002-vulkan-require-version-1-3.md).

## A frame

An app builds a frame during its tick. The engine records and presents it in one step at the
end. `Engine3D::frame()` is the frame being built; `Engine3D::renderFrame()` draws it and
empties it for the next one.

```
frame->pass("colour")->submit(item);   // during the tick, as many times as it likes
engine.renderFrame();                  // once, at the end
```

A `Frame` is a list of `Pass`es. `Engine3D` creates one, the `colour` pass, which draws
straight to the window. Everything else is more passes rather than a different kind of frame
([ADR-0003](../adr/0003-rendering-one-engine-for-2d-and-3d.md)): the editor builds one pass per viewport of the
same scene, and an offscreen target is a pass the colour pass names in `Pass::reads()`, which
records it first ([ADR-0068](../adr/0068-rendering-order-passes-by-what-they-read.md)).

A pass carries what varies between 2D and 3D drawing: whether it clears and to what, whether
it depth tests, what region of the target it draws into, the camera it draws through, and
whether its items are sorted.

A `DrawItem` describes one draw rather than performing it
([ADR-0004](../adr/0004-rendering-submit-draw-items-as-data.md)). It names its pipeline and material by
handle and carries a `SortKey`. The engine owns sorting, merging and recording, and
`Pass::submit` fills the key's pipeline and material in from the item's handles, so a caller
sets only the layer and the depth.

**Sorting is per pass and off by default.** `Pass::ordered()` hands the recorder either the
submission order or the sort key order; `Pass::sort(true)` asks for the second. The default
is submission order because 2D content is painter ordered: the key groups by pipeline and
material within a layer, so sorting a canvas of batches would put a panel over the text drawn
on it. Sorting is for a depth tested scene pass, where there is one item per object and
grouping lets the recorder skip binds. The sort is stable, so items with equal keys keep the
order they arrived in.

Either way, the recorder skips rebinding what is already bound. A pipeline, a descriptor set
and a vertex buffer are bound only when an item asks for a different one than the last item
did, so a run of quads sharing a texture costs one bind between them.

## Depth

`DeviceContext` owns one depth image, sized by what the context draws into and rebuilt with
it. It
is **allocated the first frame a pass asks for depth**, and not at all otherwise, so pong and
tetris pay nothing for it.

A pass with `depth(true)` gets it as a `pDepthAttachment` on its `VkRenderingInfo`. Depth is
cleared exactly when colour is, so a pass drawing on top of what the pass before it left
keeps that pass's depth too. The image is transitioned to `DEPTH_ATTACHMENT_OPTIMAL` once per
frame, from `UNDEFINED`: nothing carries depth between frames, so preserving the last frame's
contents is not worth a barrier, and the first pass to use it must clear.

### A depth image that is read as well as written

A `RenderTarget` built with `sampledDepth` allocates its depth image with sampled usage and a
sampler, and the recorder leaves it in `DEPTH_READ_ONLY_OPTIMAL` after the last pass that wrote
it — which is a shadow map, and is
[ADR-0044](../adr/0044-a-sampled-depth-target-is-read-only.md). `renderer::Quad::depthTexture()`
registers it, sharing the image the way a target's colour is registered.

**Asking for it changes the format.** A format the device will draw depth into is not
necessarily one it will let a shader read, so `DepthBuffer::chooseFormat(device, true)` walks
a shorter list, and a pipeline drawing into a sampled target has to be built against that
target's `depthFormat()`. The swapchain's depth buffer is unsampled and unchanged.

No app in this tree draws into one. The device suite does, into a target with sampled depth and
no colour, and reads back the depth it wrote.

Dynamic rendering matches a pipeline to the attachments of the pass it draws into, so a
pipeline built with no depth format cannot draw into a pass that has one. `renderer::Quad`
therefore compiles its pipeline twice, once each way, and picks between them from
`Pass::depth()`. Neither variant tests or writes depth: a ui drawn over a scene has to stay
on top of it whatever the scene left in the buffer.

## Building a pipeline

`vulkan::pipeline::Builder` describes a graphics pipeline one chained call at a time. Its
defaults are what every pipeline in this engine has agreed on: a dynamic viewport and scissor
so a resize costs no rebuild, one sample, one colour attachment, no culling, alpha blending,
and dynamic rendering rather than a render pass. How many colour attachments there are is a
property of the pass, so `colourFormats()` takes 0..N of them and an empty list is a pipeline
that writes depth and no colour - a shadow pass. `colourFormat()` is the one-attachment
spelling and is what everything here uses. Shader modules belong to the builder and are
destroyed with it; the pipeline and its layout are handed back for `Resources` to own.

Two knobs exist for consumers rather than for this tree, both defaulting to what it already
did. `blend()` also takes a `Builder::Blend` of four factors, whose defaults are the straight
alpha it has always applied — a pass compositing into something composited later names a
destination alpha of `ZERO`, where the default erodes the source's. And `depthBias(true)` sets
`depthBiasEnable` and puts `VK_DYNAMIC_STATE_DEPTH_BIAS` in the dynamic list, so the constant
and the slope are a scene's numbers set with `vkCmdSetDepthBias` rather than a pipeline's:
that is what a shadow pass needs to separate its own geometry from the surface tested against
it. Nothing here draws with either.

**Which is why the builder reports what it will build.** `rasterization()`, `colourBlend()` and
`dynamics()` return the three pieces of state a caller cannot otherwise see, and `build()`
assembles the pipeline out of those same three calls, so what is read is what is compiled. They
exist because nothing about a compiled `VkPipeline` says what it was built from, and a wrong
answer in any of them is a picture rather than an error: a depth bias left out of the dynamic
list compiles and validates in silence, then silently uses the zero in the create info, so
`vkCmdSetDepthBias` does nothing and a shadow does not shift. A compile cannot catch that and
neither can the validation layer, so the state is asserted directly.

```
pipeline::Builder(device)
    .name("quad")
    .shader(VK_SHADER_STAGE_VERTEX_BIT, code, sizeof(code))
    .vertexBinding(0, sizeof(Vertex))
    .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position))
    .set(uniforms->layout())     // set 0 first - they are numbered in the order they are added
    .set(textures->layout())     // set 1, every textured pipeline's - ADR-0082
    .push(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4))
    .colourFormat(swapchain->format())
    .build(cache);
```

## Memory

Every buffer and image gets its memory from `memory::Allocator`, which the `Device` owns and
builds once the logical device exists. It finds memory one of two ways
([ADR-0053](../adr/0053-memory-optional-vma-suballocation.md)): `Kind::Direct` is one device
allocation per resource and is what everything here uses, and `Kind::Suballocated` hands out
regions of larger blocks through the Vulkan Memory Allocator, which is what an application with
per-frame resources needs — `maxMemoryAllocationCount` is a real limit. A consumer names the
kind when it constructs its `Device`.

A resource therefore holds an `Allocation` rather than a `VkDeviceMemory`: a suballocated region
starts part way into its block and several share one, so mapping and freeing go through the
allocator that made it. **`Allocator::bind()` allocates and binds in one call** — a resource
creates itself, hands the handle over, and never sees a memory type.

## Buffers

There are two. Which one to use follows from how often the contents change.

- **`vulkan::Buffer`** is host visible and stays mapped for its whole life. Use it for a frame
  of geometry: a batcher rewrites the whole buffer every frame, so a staging copy would cost
  more than the slower reads do.
- **`vulkan::DeviceBuffer`** is device local and filled through a staging copy. Geometry built
  once and drawn for the life of the process pays for the copy at load time, then is read
  from the memory closest to the device every frame after.

Both go through `vulkan::Uploader`, which records, submits and waits for one command buffer.
Because it waits, a staging allocation never outlives the function that made it, and the
uploader is for load time work rather than for anything overlapping the frame loop.

`vulkan::Mesh` is the pair of device local buffers a draw reads: vertices, and indices where
the draw is indexed. `Mesh::describe(&item)` fills in a draw item's geometry fields and leaves
the caller to say what it draws with and where it sorts.

## 2D drawing: the batched quad

Every 2D thing in the engine — a rectangle, a sprite, a glyph — is one quad with a texture,
per [ADR-0005](../adr/0005-2d-one-batched-quad-pipeline.md). Lines are the other primitive and are
described below. The quad is split across the cpu/gpu line:

- **`realtime::Canvas`** accumulates the quads. It holds a vertex stream of position, uv and
  colour, an index stream, and the batches those are cut into; it cuts a batch where the bound
  texture, the text flag or the clip rectangle changes. It has a modelview stack of translates
  and scales that applies as vertices are added, and it produces the pixels-to-clip-space
  projection the pipeline is pushed. None of it touches vulkan, so the batching has unit
  tests.
- **`vulkan::renderer::Quad`** owns the one pipeline and a vertex and index buffer per frame
  in flight; the 1x1 white texture an untextured quad is drawn against, and the material it
  binds at set 1, are the context's `Textures`
  ([ADR-0082](../adr/0082-textures-owned-by-the-device-context.md)). `submit(canvas, pass)` uploads the canvas into the buffers belonging to the frame
  about to be recorded, and turns each batch into a `DrawItem`.

**A clip is batch state and the device scissors the draw**, per
[ADR-0037](../adr/0037-2d-clip-with-a-per-batch-scissor.md). `Canvas::clip` pushes a
rectangle, in the coordinates being drawn in and intersected with whatever is already clipped;
the batch carries it, `renderer::Quad` puts it on the `DrawItem`, and the recorder sets a dynamic
scissor per item and puts the pass's own region back for an item that names none. Nothing is
clipped on the cpu, so a quad straddling the edge is drawn whole and half of it lands.

**A canvas may draw in a space of its own**, per
[ADR-0075](../adr/0075-2d-a-canvas-may-have-its-own-coordinate-space.md). `Canvas::space(size, fit)` sets a
size in the game's units, with its origin at the top left, and either stretches it over the
canvas or contains it at its own aspect, centred with bars either side. `projection()` then
maps the space into `viewport()`, `toSpace()` maps a cursor back, and a clip is mapped out to
pixels because a scissor is in pixels. The projection stays a push constant per submit, so a
court in a space and a menu in pixels are two canvases in one pass, as pong draws them.

`LineCanvas` clips on different terms. It cuts its stream into batches the same way, but the
rectangle is in the pixels of the image drawn into and the modelview does not apply to it: a
line canvas is world space, so there is no transform there that a screen rectangle could go
through.

The buffers are per frame in flight because the device may still be reading the previous
frame's geometry. Each renderer streams through a `vulkan::frame::StreamRing`, whose `claim`
waits on the frame's fence before handing over a buffer. That is the same fence `acquire`
waits on, so it costs the frame nothing it was not going to pay. A frame takes as many sets as
it submits canvases, and the stream starts again from the first the first time it is claimed
from after the in-flight ring has begun another frame - so nothing has to be told a frame
ended, and a renderer an app built itself reuses its buffers the same as one the context holds.
A buffer the content outgrows is replaced by one twice the size, and the old one is retired
through the ring ([ADR-0061](../adr/0061-resources-explicit-release-generational-handles.md)) rather than
waited for.

Text goes through the same path. A `v3d::font` text buffer lays glyphs out into positions,
atlas coordinates and colours, and `Canvas::text` copies those into the stream against the
atlas texture. A single channel atlas is given an image view that swizzles its one channel
into alpha and ones into rgb, so the glyph samples as white with coverage and the shader
needs no branch for text.

The ui draws through the same canvas rather than a pass of its own.
`v3d::ui::paint::ComponentRenderer` adds its panels and highlights as quads and asks the app to write
its labels, so a game and its menu cost one upload and a draw per texture.

**Drawing the ui is also what lays it out**, per
[ADR-0019](../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) and
[ADR-0034](../adr/0034-ui-layout-is-resolved-while-drawing.md). A component holds other
components; `Component::layout()` says where it sits in the one holding it, as a length per
axis that is either pixels, a percentage of the parent or `Auto`; and the walk that draws a
container resolves each box against the box around it and leaves the component holding the
absolute result in `position()` and `size()`. That result is what `Container::pick` tests a
cursor against, so nothing is clickable until it has been drawn, and a component answers the
cursor only when it is `pickable()`. A `VerticalBox` or a `HorizontalBox` writes its
children's boxes itself rather than resolving them, because their order along the line is
what a flow list is for. A `SelectList` shows as many rows as its box has room for and a
`TabBar` walks only the page its chosen tab holds, so what is not on screen is neither drawn
nor laid out - and a component that was not laid out cannot be picked, which is ADR-0019 read
the other way round. `Panel`, `Bar` and `Scrollbar` round their corners with `Canvas::arc`,
which is the same triangle fan `circle` is built from and so stays inside the one batched
primitive of [ADR-0005](../adr/0005-2d-one-batched-quad-pipeline.md). A component cuts what it holds
off at its own box when it asks to, with `Component::clip(true)`; a `Scrollbar` is the
arithmetic of how far something is scrolled and leaves the input to whoever picked it.

**There is a second way to write a ui, onto the same canvas**, per
[ADR-0035](../adr/0035-ui-immediate-mode-beside-the-retained-tree.md). `v3d::ui::Immediate`
takes the same `Measure` and `Write` callbacks and is driven by calls rather than by a tree:
a window, a tab strip, a table, a button and a scrubbable int between `begin()` and `end()`,
each placed where a layout pen has got to and hit tested against the box it was just drawn
in. It is the shape a tool wants, because a panel written that way is a function of the state
it reads and cannot show something stale; a hud is the other shape and stays retained. Which
widget the cursor is on is settled at `end()` and used by the next frame, which is what lets
a window drawn later take the cursor from one under it. A window cuts what it holds off at its
own edges and scrolls it on the wheel; how tall the content is is measured as it is drawn, so
the bar appears on the frame after the one that overflowed.

## Line drawing

The second primitive, per [ADR-0011](../adr/0011-rendering-lines-as-a-world-space-primitive.md). The editor's
construction grid, axis decoration, wireframe display, selected-edge highlight and
manipulators are all made of it. It splits across the cpu/gpu line the same way:

- **`realtime::LineCanvas`** accumulates segments — `line`, `polyline`, `box` and `circle` over
  a modelview stack that applies as vertices are added. There is no index stream, and the only
  thing that cuts a batch is a clip changing, since there is no texture: an uncut canvas is one
  batch and one draw.
- **`vulkan::renderer::Line`** owns two pipelines and a vertex buffer per frame in flight.
  `submit(canvas, pass)` uploads and adds one `DrawItem` per batch.

Two things differ from the quad. Positions are in **world space**, and the transform is the
camera the pass carries at set 0 rather than a projection in a push constant. The quad
pipeline does not follow, because its projection belongs to a canvas rather than to a pass. The two
pipelines also differ in behaviour, not only in attachment format: the one built for a pass
with depth **tests and writes** it, so geometry in front of a wireframe occludes it, while the
quad's depth variant does neither. Lines drawn over a scene rather than into it go in a pass
without depth. That is the pass model choosing, not a flag on the renderer.

Lines are one pixel wide. `wideLines` is an optional device feature and the device does not
ask for it. The renderer is built on the first call to `DeviceContext::lines()`, the way the depth
buffer is, so an app that draws no lines pays nothing for it.

## World space quads

The third primitive, per [ADR-0042](../adr/0042-rendering-world-space-sprites.md): a textured
rectangle with four world corners, for a sprite standing on a ground plane and for a filled
tile highlight.

- **`realtime::WorldCanvas`** accumulates quads over a modelview stack of `glm::mat4`, which
  applies as vertices are added. A quad takes its four corners in perimeter order — the order
  `grid::tileCorners` hands them out in — and is fanned from the first, so any convex quad
  comes out whole. The stream cuts where the bound texture changes and nowhere else. A tint
  multiplies the colour of every quad added after it, which is how a game lays dusk or an
  act's palette over the whole world once rather than in every caller; `clear()` returns it to
  white, and the ui's `Canvas` has none.
- **`vulkan::renderer::World`** owns four pipelines and a pair of buffers per frame in flight,
  and takes its textures and its set 1 descriptors from the context's `Textures`
  ([ADR-0082](../adr/0082-textures-owned-by-the-device-context.md)) so that an atlas uploaded once serves
  both primitives out of one descriptor pool. The pipelines are two blends,
  each with and without depth. `World::Blend::Alpha` is straight alpha over what is there.
  `Additive` adds the colour by its alpha and keeps the destination's alpha, so a flame or a
  spark only lightens and two of them come out the same in either order. A canvas is submitted
  with one blend.

Positions are in world space through the pass camera at set 0, as lines are. **The order is
the caller's**: quads are drawn in the order they were added, because what a quad's depth means
is the game's — in an isometric projection a sprite is behind another when its feet are further
up the ground plane, not when it is further from the camera. The depth variant therefore
**tests without writing**, which is the third of the three answers the engine now has: lines
test and write, ui quads do neither, world quads test only. So solid geometry hides a world
quad and a world quad never hides another.

**`realtime::DepthOrder`** is how a caller puts its quads in that order without sorting them
itself. It collects quads with a key the caller computes, and hands them to a canvas largest
key first. Equal keys are grouped by texture and otherwise keep the order they were added in,
so a key quantised to a tile row cuts fewer batches than an exact one. The canvas is unchanged
and still draws whatever it is given in submission order.

There is no clip and no text branch. The renderer is built on the first call to
`DeviceContext::worldQuads()`, the way the line renderer is. So is the quad renderer, since a
context is built before it has been told the format its pipelines compile against.

## The lit pass

A registered model drawn with light is three things: a `MeshRegistry` entry, an entity carrying
`ecs::component::Transform` and `component::Mesh`, and a `vulkan::renderer::Lit`.

`Lit` owns the cel and outline pipelines, compiled against the colour and depth formats of
the pass they draw into, the shadow pipeline, compiled against the shadow map's depth format,
and the scene set each frame binds at set 2
([ADR-0064](../adr/0064-lighting-lit-passes-use-the-shared-recorder.md)). Every lit pipeline
declares the camera at set 0, the albedo at set 1 in the `Textures` material layout, and the scene
at set 2, with one push block holding the model matrix, the base colour and the outline's
thickness. Front faces are clockwise, because a model is wound counter clockwise seen from
outside and the cameras in `api/type` flip y into Vulkan's clip space
([ADR-0012](../adr/0012-camera-projection-targets-vulkan-clip-space.md)).

A frame of it, built during the tick as any frame is:

```
const glm::mat4 light = realtime::shadow::light(settings.light, bounds.centre, bounds.radius);
VkDescriptorSet scene = lit.scene(realtime::pack(settings, light, 1.0f / mapSize), quads->depthTexture(*map));

casting->scene(scene);                                  // the shadow pass
casting->depthBias(settings.constantBias, settings.slopeBias);
realtime::casters(registry, alpha(), meshRegistry, lit, casting);

pass->reads(map);                                       // which records the shadow pass first
pass->scene(scene);
realtime::meshes(registry, alpha(), meshRegistry, lit, settings.outline, pass);
```

`Lit::scene()` waits for the frame's slot and writes its uniform and its shadow map binding,
so it is called while the frame is built and before the ring begins it, the same rule
`Quad::submit` keeps. With no shadow map named, the white texture stands in, which reads as the
far plane, so nothing is in shadow. `meshes()` submits every entity's outline and then every
entity's surface, a draw per part of its entry, each drawn alpha of the way from its previous step
([ADR-0060](../adr/0060-ecs-interpolate-from-a-previous-step-component.md)).

**The shadow is a pass like any other.** Its target has sampled depth and no colour, and the
recorder leaves that depth read only once the pass has written it
([ADR-0044](../adr/0044-a-sampled-depth-target-is-read-only.md)), which is how the lit pass after
it samples the map through set 2. The lit pass names the map in `reads()`, so the frame
records the shadow pass first. `casters()` submits
every entity whose `castsShadow` is set, and the shadow pipeline is biased, so the pass names a
bias or the recorder throws. Both passes bind the same scene set.

**World quads go in the lit pass, after its meshes.** A `World` built against the scene target's
colour and depth formats submits into the same pass, and the pass records in submission order,
so its quads are depth-tested against everything `meshes()` drew and are graded with it. The
recorder binds the scene set only for a pipeline that declares one, so the world pipelines are
drawn in a pass that carries one. The engine's own `worldQuads()` is compiled against the
swapchain's format, which the recorder's check refuses in an sRGB scene target. The quads'
colours are linear there ([ADR-0066](../adr/0066-lighting-light-in-linear-draw-to-srgb.md)).

**A skinned model is drawn by the same walks, with the skinned pipelines.** `Lit` has a
skinned variant of the cel, outline and shadow pipelines. Their vertex is
`MeshRegistry::SkinnedVertex`: the model's 32 bytes, then four joints as unsigned shorts and four
weights, 56 bytes in all, which is how the registry uploads a model with a skeleton. Their
vertex stages include `shaders/lit/skin.glsl` beside `lit.glsl`, and move a vertex by the
weighted sum of its joints' matrices before the model matrix. The fragment stages are shared.

The matrices are a frame's palette
([ADR-0071](../adr/0071-skinning-joint-matrices-in-one-storage-buffer.md)).
`realtime::poses(registry, alpha, meshRegistry)` walks every entity whose entry has a skin. It
samples its `ecs::component::Playback`, drawn between steps, faded out of the clip it is leaving
([ADR-0070](../adr/0070-animation-cpu-sampling-playback-on-the-fixed-step.md)), or stands it at rest
when it has none. It returns a `Poses`, which is every palette end to end and where each one
starts. The palette goes to `Lit::scene()`, which writes it into a storage buffer at set 2,
binding 2, growing the buffer through the ring. The `Poses` goes to `meshes()` and `casters()`,
which push each skinned item's first joint. Both passes read the one buffer, so a shadow is cast
in the pose that is drawn:

```
const realtime::Poses poses = realtime::poses(registry, alpha(), meshRegistry);
VkDescriptorSet scene = lit.scene(realtime::pack(settings, light, 1.0f / mapSize), quads->depthTexture(*map),
    poses.palette());
realtime::casters(registry, alpha(), meshRegistry, lit, casting, poses);
realtime::meshes(registry, alpha(), meshRegistry, lit, settings.outline, pass, poses);
```

A skinned entity the `Poses` does not name is skipped, since it has no palette to be drawn with,
so walks given no `Poses` draw static models only. `MeshRegistry::clip(handle, name)` gives the
index `ecs::component::play()` takes.

`shadow::light` is an orthographic box a radius either side of a centre, seen from two radii
out towards the light, with the far plane four radii beyond the eye. It builds its view as
`type::camera::Camera` does, so a face is wound the same way under the light as under the scene's
camera and the shadow pipeline culls as the cel one does. `shadow::fit` gives the centre and
radius: the mean of the casters' positions, and the farthest of them plus a margin, which has to
cover a caster's size and the length of its shadow. Something that casts nothing, such as the
ground, stays out of the fit. A caller fits once, at scene load or when the casters move
somewhere new; the map does not follow them on its own.

**The look is `LitSettings`**: the light, the fill, two band thresholds, three band
multipliers, the light's colour and the shadow band's, the outline, and the shadow's biases and
strength, with no device in it. The light's colour multiplies the mid and lit bands, and the
shadow's colour the shadow band, so a scene's light changes over a day by changing two colours.
Both default to white, which leaves every band as it was. `pack()`
lays it out as `SceneUniforms`, which is the std140 `Scene` block. The cel shading is a key
light plus a fill from above and opposite it, quantised into three flat bands, with a cast
shadow dropping a fragment one band. A game that wants another look hands `Lit` its own
shaders, which declare the same blocks
([ADR-0067](../adr/0067-lit-shaders-are-embedded-and-replaceable.md)).

**The outline is a hull, not a post process.** Each vertex is pushed out along its normal before
the model matrix, and the hull is drawn with its front faces culled, so only a rim shows
around the silhouette. Its thickness is in the model's units, so it is thinner on screen the
further out the camera is. On a mesh with a normal per face rather than per vertex, the rim
breaks wherever two faces meet at the silhouette. On an open, single-sided mesh, a face turned
from the camera has its surface culled, so the back of its hull shows as a black face of its own
shape. The outline is meant for closed meshes.

## Passes over the whole target

**`vulkan::renderer::FullScreen` is a pass's worth of fragments over what an earlier pass drew.**
It is one triangle covering the target, drawn from three vertices and no vertex buffer, with a
fragment stage the caller hands in as SPIR-V. Its uv runs from 0 at the top left to 1 at the
bottom right. The images it reads are bound at set 1, one combined image sampler per binding,
and `source()` registers them as a material, so the item binds them as any draw binds a texture.
Set 0 is declared and need not be read. The pass that draws it names what it reads in
`Pass::reads()`, which records whatever drew those images first
([ADR-0068](../adr/0068-rendering-order-passes-by-what-they-read.md)).

A source names images as they are when it is made. A target that is resized is bound again, and
a source is released before the `FullScreen` that made it goes.

**`realtime::Grade` is the colour grade drawn with it**: every pixel of a scene looked up in a
16³ table. The table comes from a 256×16 strip, sixteen slices of blue side by side with red
across each and green down it, and a strip of any other shape, or none, gives the identity.
The table is linear colour and indexed by linear colour
([ADR-0066](../adr/0066-lighting-light-in-linear-draw-to-srgb.md)), since a lit scene's sRGB target decodes
when it is sampled. The scene is read nearest, so the grade draws into a target of the scene's
size, and the lookup is scaled into the table's texel centres so that black and white land on
entries rather than on their edges. Loading the strip is the caller's, so a game reads its look
from wherever it keeps it.

```
Grade grade(logger, context, swapchainFormat, depthFormat, strip);   // once
const MaterialHandle source = grade.source(*scene);                  // again after a resize

colour->reads(scene);
grade.submit(source, colour.get());
```

**A grade's table can be replaced.** `replace(texels)` makes a new table from texels in the
order `Grade::table()` gives them, rebinds every source to it, and releases the old table and its
materials through the ring ([ADR-0061](../adr/0061-resources-explicit-release-generational-handles.md)), so a
frame in flight finishes with the table it was recorded against. A source keeps the handle
`source()` gave it. A zone's own look is a swap, and a slow change between two looks is a lerp
of their texels that the game uploads as often as it likes. It is called before the frame's
`submit()`, since the material an earlier submission named is the one it lets go of.

## Shaders

The engine's shaders live in `api/render/shaders`, are compiled to SPIR-V by `glslc` at build
time, and are embedded in the library. They are not data files: they belong to the engine
rather than to any app, and CMake does not copy per-app data into the build tree, so a shader
sitting on disk beside an executable would go stale silently. `v3d_add_shader` does the
compiling, and `glslc -mfmt=c` writes the module out as a C initialiser list that the source
includes into a `uint32_t` array. See [Build.md](../contributing/Build.md#shaders).

## Colour

The swapchain is a `UNORM` format rather than an `_SRGB` one, so the colour a shader writes is
the colour that appears — see [ADR-0009](../adr/0009-colour-display-space-unorm-swapchain.md). Every
colour in the tree is authored in display space, and textures are uploaded as `UNORM` to
match.

**A consumer that writes linear light names its own format**, per
[ADR-0049](../adr/0049-swapchain-caller-picks-the-format.md). `Swapchain`, `Context3D` and
`Engine3D` take a preferred format, defaulting to none and therefore to the rule above; a
format the surface does not offer in a non-linear sRGB colour space falls back to it, with a
warning, so silence means the preference was met. An app on the engine shell names one where
it constructs its `Engine3D`, which is the whole of what it has to do. Nothing in this tree
passes one. **Build a pipeline against `Swapchain::format()` rather than against the
default** — that was always the contract under dynamic rendering, and it is now the only way
to be right.

**A lit scene is the exception: it computes in linear light and is drawn into an `_SRGB`
target**, which encodes on store
([ADR-0066](../adr/0066-lighting-light-in-linear-draw-to-srgb.md)). Its albedo is uploaded with
`TextureFactory::Encoding::Srgb`, so it is decoded before it is lit, and `MeshRegistry` is the
one place that does that. Every other texture keeps the `Display` default.

## What renderFrame does

1. `Presenter::acquire` waits on the frame's fence, takes the next swapchain image, and
   begins that frame's command buffer.
2. `Recorder::record` transitions the image to `COLOR_ATTACHMENT_OPTIMAL`, walks the passes —
   `vkCmdBeginRendering`, viewport and scissor, the items and the scissor any of them asks
   for, `vkCmdEndRendering` — then
   transitions the image to `PRESENT_SRC_KHR`. Both transitions are synchronization2 barriers,
   and like every layout transition the engine makes they are the named ones in
   `vulkan/memory/Barriers.h`, which decides the stages and access either side so that two
   places moving an image the same way cannot disagree. A `VkResult` that is not a success
   throws through `device::check`, naming the call and the result.
3. `Presenter::present` ends the buffer, submits it with `vkQueueSubmit2`, and presents.

**Every pass is timed on the device.** The ring owns a `vulkan::frame::Timings`, a timestamp query
pool per slot, and `Ring::begin()` reads what that slot timed the last time it was used and
resets it in the new command buffer. The recorder writes a timestamp either side of every pass,
under the pass's name. So `Engine3D::timings()` is the frame that is as old as the ring is deep,
and reading it never waits on the device. A caller recording its own commands into the ring's
buffer may open and close spans of its own. The device keeps its timestamp period and the
graphics family's valid bits, and a family that writes no timestamps leaves the timings off.
On the cpu, `engine::Statistics::scope(name)` times a span of the frame into a row of its own,
and `StatisticsOverlay` draws a line per span it is handed.

Two frames are in flight. What they are is a `vulkan::frame::Ring`, which needs a device and
nothing else ([ADR-0051](../adr/0051-frames-in-flight-ring-separate-from-presenting.md)): a command
buffer and a fence per frame, `frame()` to say which slot is being recorded, and `waitFrame()`
for anything else keeping a resource per frame in flight. Every renderer is built on the ring
rather than on the presenter, because sizing and indexing a geometry ring is pacing rather than
presenting.

What stays on the `Presenter` is what needs the chain: an image-available semaphore per frame,
and a render-finished semaphore per swapchain image rather than per frame, because presentation
waits on it and presentation is tied to the image.

The seam between them is the fence. The ring creates it and waits on it; the submit in
`present()` is what signals it. `acquire()` waits the ring's fence *before* acquiring rather
than leaving it to `Ring::begin()`, because the image-available semaphore is per frame and this
slot's may still be pending from its last turn — and it is `begin()` that unsignals the fence,
so a chain found out of date in between leaves the ring exactly as it was found.

### Reading a frame back

`vulkan::frame::Capture` copies a drawn image into a host visible buffer and writes it as a
png. It is two calls because the submit sits between them — `record()` into the frame's own
command buffer, `write()` once whatever the caller synchronises with says that submit has
completed ([ADR-0050](../adr/0050-a-frame-is-read-back-in-two-calls.md)).

`record()` takes a `Capture::Source` — an image, its extent, its format and the layout it is
in — so the same call reads a presented frame or an offscreen `RenderTarget`. The swapchain
overload fills one in: a chain image is captured between step 2 and step 3 above, where it is
in `PRESENT_SRC_KHR` and still acquired, which is the only point it may legally be read. A
target is captured in whatever layout the recorder left it, which for one a later pass samples
is `SHADER_READ_ONLY_OPTIMAL`. The image is handed back in the layout it arrived in either way.

The barrier either side of the copy is not the same for both. Returning a chain image to
`PRESENT_SRC_KHR` needs nothing made visible, because the semaphore presentation waits on is
what orders it; returning a target to a layout something in the same submit may sample or draw
into has no such semaphore, so that transition has to be complete and visible before any of
them. A target's colour image carries `TRANSFER_SRC` usage so that it can be copied out at all,
and so does a sampled depth image.

**Depth is read back as floats, not as a picture.** A source marked `depth` copies the depth
aspect instead of colour, and `depth()` hands back one float a pixel once the submit has
completed. Only `D32_SFLOAT` can be read this way, since its copy is exactly the float a test
compares. A png would round depth to eight bits. The device suite is what calls `Capture`, for
the pictures it pins under [ADR-0054](../adr/0054-testing-golden-images-hold-only-spec-exact-output.md)
and the depths it compares.

### Resize and minimize

The swapchain is rebuilt when presenting or acquiring reports it out of date. That is the only
reliable trigger: a resize event can arrive before or after the driver notices, and on some
drivers it does not arrive at all. `VK_SUBOPTIMAL_KHR` counts — the frame that saw it is still
drawn and presented, and the chain is rebuilt before the next one.

A window with no area has no swapchain at all. `Swapchain::create` leaves the chain empty
rather than failing, `Presenter::acquire` answers `Skip`, and `Engine3D` tries again once the
window has an area. That is what makes minimizing survivable.

## Binding by update frequency

Everything downstream depends on this being fixed, so it is fixed here:

| Set | Frequency | Holds |
|---|---|---|
| set 0 | per frame, bound once by the pass | camera, projection, viewport |
| set 1 | per material | the sampled texture and whatever else the material needs |
| set 2 | per pass, and only for a pipeline that declares it | a lit scene's light and shadow map, and the frame's joint palettes |
| push constants | per object | transform and tint, and where a skinned object's palette starts |

A draw item names a material, and the material owns its set 1. Anything that changes per
object goes in push constants rather than in a set. That keeps what an item binds of its own
at one set, and makes merging adjacent items a matter of comparing two handles.

**Set 2 is the pass's**, per [ADR-0064](../adr/0064-lighting-lit-passes-use-the-shared-recorder.md).
`Pass::scene()` names it, and the recorder binds it once for the pass, for any pipeline whose
layout declares a third set and for nothing else, so a quad drawn in a lit pass binds nothing
extra. `Pass::depthBias()` is the same arrangement for a depth bias: recorded whenever a
pipeline built with `Builder::depthBias(true)` is bound, and ignored by every other pipeline.
A pipeline that declares either one, drawn in a pass that names neither, throws at record
time. Vulkan would otherwise read whatever was left bound, or draw at whatever bias was last
set, and say nothing. `Recorder::check` is that test, and needs no device to ask.

The sort key is ordered to match: layer, then pipeline, then material, then depth. Sorting on
it groups exactly the draws that can share a binding.

**Set 0 is built.** `vulkan::FrameUniforms` owns the layout — one uniform buffer at binding 0,
visible to both the vertex and the fragment stage — and a slot per pass per frame in flight,
each a small buffer with a descriptor set that points at it permanently. `Recorder` writes a
pass's view, projection, their product and viewport rectangle into the next slot, then binds
it at 0 for every item in that pass. A slot is written during recording, after the presenter
has waited on that frame's fence, so nothing is reading what is overwritten.

Every pipeline in the engine declares that same layout at set 0, which makes them
interchangeable within a pass: a set bound for one stays bound across a pipeline change to
another built against the same layout. The quad pipeline declares it and reads nothing from
it, since a canvas carries its own orthographic projection in a push constant, and two canvases
in one pass may map different spaces
([ADR-0075](../adr/0075-2d-a-canvas-may-have-its-own-coordinate-space.md)). The line, world and lit
pipelines and voxel's terrain read it. Terrain reads nothing else per draw: one camera at set 0,
one block palette at set 1, and the chunk's origin in a 16 byte push constant. The line
pipelines read it and declare nothing else at all — no set 1 and no push constant — and are
still compatible for set 0 with the quad and terrain pipelines, because compatibility runs
from set 0 upwards.

## Offscreen targets

**A pass draws into the swapchain image unless it names a `vulkan::RenderTarget`**, per
[ADR-0031](../adr/0031-rendering-passes-draw-into-offscreen-targets.md). A target is a colour
`memory::Image`, optionally a depth buffer, and a `pipeline::Sampler`. A target given
`VK_FORMAT_UNDEFINED` for colour has no colour image at all, which is what a shadow map draws
into. It has to have sampled depth, since otherwise there would be nothing to read, and the
recorder begins its passes with no colour attachment. It is created with
sampled usage and sized in pixels rather than by the window. It is given the in-flight ring,
because what it lets go of on a resize or when it is destroyed is retired through the ring
rather than destroyed under a frame still drawing into it
([ADR-0061](../adr/0061-resources-explicit-release-generational-handles.md)).

`Recorder` scans the pass list and moves a target into the attachment layout before the first
pass that writes it, then into `SHADER_READ_ONLY_OPTIMAL` after the last - and a sampled depth
into `DEPTH_READ_ONLY_OPTIMAL`, which is the layout `pipeline::Texture::descriptor()` names for a
depth image wherever one is sampled, a canvas included. A target therefore
costs one pair of barriers however many passes draw into it, and every later pass can read it.
Register it with `Textures::texture(target)` to get a texture handle a canvas can
composite.

**A pass is placed by what it reads**
([ADR-0068](../adr/0068-rendering-order-passes-by-what-they-read.md)).
`Pass::reads(target)` says a pass samples a target, and `Frame::ordered()` records every pass
drawing into that target first. Passes into one target, the window included, keep the order
they were created in, and so does anything the reads do not order. Two passes each reading what
the other draws throw. A pass that samples a target and does not say so is recorded where it
was created, and reads whatever is there.

**A target may hold one image per frame in flight.** A pass draws into `current()`, and
`previous()` is what the frame before drew, which is how a pass reads its own last frame. A
reader registers each slot once, `Textures::texture(target, slot)`, and names the handle for
`current()` or `previous()` each frame. Each slot of such a target starts cleared to
transparent black and readable, so `previous()` can be read on the first frame.

**What `Resources` is given for a target shares its images**, and `recreate()` allocates new
ones. The registered texture keeps the old images alive and goes on drawing them, so after a
resize, release the old handle and register the target again.

A pipeline under dynamic rendering is built against the format of what it draws into. **The
recorder compares them when it binds a pipeline**: the colour format, and the depth format in a
pass that tests depth, wherever both sides state one. A mismatch throws, naming the pass. The
validation layer reports the same thing when it is on, as a draw. A target's colour format
defaults to the swapchain's.

## Resource handles

`DrawItem` refers to pipelines, materials and textures by handle, never by pointer. A pointer
sorts by whatever the allocator handed out, which reorders a frame differently on every run.
`vulkan::Resources` owns them, hands out the handles, and destroys whatever is left when the
context goes.

**A texture is released explicitly** ([ADR-0061](../adr/0061-resources-explicit-release-generational-handles.md)),
through `Textures::release`, which releases its material with it. A handle carries a slot
and a generation. A released slot is reused by the next registration with its generation moved
on, so a handle stops resolving the moment it is released, and it can never come to mean
whatever is put in its slot next. What it named is handed to the in-flight ring as a callback,
and `Ring::begin()` runs the callback once every frame that began before the release has
finished. A material's descriptor set comes back the same way and is written again for the next
material, because the pools cannot free a set. That is `pipeline::DescriptorPool`: one layout,
a list of vulkan pools grown a fixed count at a time, and a set handed back through the ring.
Set 0's slots, the materials and voxel's palette are each allocated from one. Pipelines are not
released.

**Geometry is not one of them.** A mesh is created and destroyed while the app runs, which a
registry that never frees cannot hold without leaking, and the sort key has no geometry field
to sort a handle on. So `vulkan::Mesh` is owned by whatever built it — a chunk, a model —
`DrawItem` keeps raw `VkBuffer`s, and a draw item is valid only while its mesh is alive. See
[ADR-0010](../adr/0010-meshes-owned-by-the-app-that-built-them.md).

**A model that is shared is registered.** `realtime::MeshRegistry` turns a glTF path, or a
`type::Model` added under a name, into a `memory::Mesh` once, and hands back a `MeshHandle` with
a slot and a generation the way `Resources` does
([ADR-0065](../adr/0065-meshes-shared-registry-keyed-by-path.md)). An entry holds the mesh and
its parts ([ADR-0069](../adr/0069-models-material-parts-over-one-vertex-buffer.md)). A part
is a range of the mesh's indices, its albedo's texture and material from the context's `Textures`, and
its base colour, and it is one draw. The material is the white one when the part's material
names no image, or names one that cannot be found, which is reported. Every part naming the same
image shares one texture and one material, across entries. A model whose part reaches past its
indices or its materials is refused. Releasing an entry retires its mesh through the ring, and
releases each albedo with the last part naming it. The
registry sits beside `Resources` rather than inside it. The sort key still has no geometry
field, and `DrawItem` still takes raw buffers, filled from the entry by `Mesh::describe()`. The
vertex layout is `type::Model::Vertex`: position, normal and uv in 32 bytes.

## What is not built yet

- **Merging.** Sorting groups the draws that could be merged, and nothing merges them.
  Adjacent items sharing a pipeline and a material still cost a draw call each.
- **A second depth buffer.** There is one per context, and the editor's four viewports share
  it. That works only because their regions do not overlap and each pass clears its own. Two
  passes wanting different depth over the same pixels would not work.

## How this meets the ECS

An entity is drawn from an `ecs::component::Transform` and a component per kind of drawing,
and the api walks them
([ADR-0063](../adr/0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md)). The
drawing components live in `realtime/component/`, beside the handles they name, so `api/ecs`
stays free of the device.

`realtime::sprites(registry, alpha, right, up, depthAxis, order)` is the walk for
`component::Sprite`. Every entity carrying both components becomes an upright quad facing the
camera, standing on the transform's position and spanned by the `right` and `up` the caller
passes. It goes into a `DepthOrder` keyed by `dot(position, depthAxis)`, so a whole world of
sprites reaches one `WorldCanvas` in the caller's order and cuts a batch only where the texture
changes. The transform is read through `interpolated<Transform>`, so an entity whose game called
`snapshot<Transform>` is drawn between steps
([ADR-0060](../adr/0060-ecs-interpolate-from-a-previous-step-component.md)). A sprite's rotation is
ignored, because a billboard faces the camera.

`realtime::particles(registry, alpha, right, up, depthAxis, order)` is the walk for
`component::Particles`, which is what the particles of an `ecs::component::Emitter` look like:
a texture, and a sprite clip played by a particle's age or over its whole life
([ADR-0072](../adr/0072-particles-an-emitter-component-owns-its-particles.md)). Each
particle is a quad centred where it stands, drawn alpha of the way from the previous position it
keeps itself, sized and coloured by the emitter's tracks at its life. It faces the camera, or is
stretched along its velocity as the camera sees it, which is a raindrop or a spark. A
snowflake's sway is added here, along the camera's right, and never reaches the simulation. Its key is
measured as a sprite's is, so particles and sprites handed to one `DepthOrder` sort among each
other. The walk needs no `Transform`, since a particle stands in the world where it was born.

`component::Mesh` is the other kind the record names: a `MeshHandle` from a `MeshRegistry` and
`castsShadow`, with the material on the registry entry rather than the entity. The walks that
draw it are [the lit pass](#the-lit-pass)'s: `meshes()` into the lit pass and `casters()` into a
shadow pass.
