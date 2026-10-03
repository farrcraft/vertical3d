# A Lit Scene

Milestone 4 of [the game engine roadmap](GameEngine.md). What a 3D game needs from the renderer
above the plumbing: images, samplers and textures as classes, a model onto the device, a lit
mesh pass, a shadow map and a chain of passes after the scene. **None of it would be new code.**
retcon has written all of it against this tree's device tier, in `engine/renderer/`, and its own
architecture document lists what it has as "what vertical3d has no class for". This milestone
moves that tier here and has retcon delete its copy.

It waits on [milestone 3](m3-RenderableComponent.md), because the pass walks entities and what it
walks for is that record's answer, and on [milestone 2](completed/m2-LargeWorlds.md)'s resource lifetime,
which is now decided ([ADR-0061](../adr/0061-a-resource-is-released-explicitly.md)): the image,
sampler and texture classes moved here retire what they own through `frame::Ring::retire`, which
takes a callback and needs no change for them.

## What exists

Here:

* **The device tier.** Device, allocator ([ADR-0053](../adr/0053-a-consumer-chooses-how-memory-is-found.md)),
  swapchain and presenter, the in-flight ring, `memory::Mesh`, `DeviceBuffer`, `Buffer`, the
  uploader, `pipeline::Builder` and the pipeline cache. retcon uses all of it.
* **Set 0 is the camera** ([ADR-0008](../adr/0008-binding-by-update-frequency.md)), held per pass
  by `vulkan::FrameUniforms`. Set 1 is the material, and a material is one texture.
* **Targets and depth.** A pass draws into a target it names
  ([ADR-0031](../adr/0031-a-pass-draws-into-a-target-it-names.md)); a target's depth can be
  sampled ([ADR-0044](../adr/0044-a-sampled-depth-target-is-read-only.md)); a pipeline can be
  depth-only and carry a depth bias. That is a shadow map's plumbing, and nothing here draws
  one.
* **No image or sampler class.** The image, its allocation and its view are open-coded in
  `frame::DepthBuffer`, `frame::RenderTarget` and the `pipeline::Texture` POD, and
  `vkCreateSampler` is called in three places, each holding a bare `VkSampler`.
* **A model stops at the cpu.** `asset::loader::Gltf` reads glTF into a `type::Model`
  ([ADR-0030](../adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md)), and
  nothing takes one onto the device. `memory::Mesh` takes bytes and indices, so the step is an
  app's four lines, and a helper here would need a vertex layout the api does not own. retcon's
  `GltfLoader` already parses through it and converts to its own vertex layout.
* **Two consumers write raw Vulkan for want of this.** retcon's `gpu/` tier, and voxel, which
  creates its own descriptor set layout, pool and uniform buffer for an untextured material
  ([`Renderer.cxx:145-243`](../../voxel/src/Renderer.cxx)).
* **Three gaps in targets**, none met yet because nothing in this tree draws into one: a target is
  single-buffered, nothing catches a pipeline built for one format drawing into a target of
  another, and `Frame::passBefore` exists because `Engine3D` creates its colour pass in its
  constructor.

In retcon, at `3d22935`:

* **`renderer/gpu/`** — `GpuImage`, `Sampler`, `Texture`, `Spirv`, `DescriptorAllocator`,
  `FrameData`. Each an RAII wrapper over one Vulkan object.
* **`renderer/passes/`** — `MeshPipeline` (cel bands and a fill light), `ShadowPass` (one
  directional map, fitted by `fitShadowFrustum`), a normal-extrusion outline, `OffscreenPass`,
  `LutPipeline` (a colour grade from a LUT image), `DebugLines`, `GridOverlay`, and above them
  `SceneRenderer`, the only type that knows the pass order, and `SceneRenderSettings`, the look
  as parameters with no Vulkan in it.
* **`renderer/MeshRegistry`** — geometry and albedo de-duplicated by asset path, handed out as
  a handle, so entities sharing a model share one upload.
* **Three descriptor sets by frequency**: this tree's set 0 member for member, the material at
  set 1, and the scene — light, cel bands and the shadow map — at set 2, placed there so as not
  to widen set 0 for every shader in this tree.

## What it needs

In order, because each is what the next is written against.

### 1. Image, sampler and texture

`GpuImage`, `Sampler` and `Texture` move here, and `DepthBuffer`, `RenderTarget`,
`TextureFactory` and `pipeline::Texture` are rewritten over them. This is the step that removes
code from this tree as well as adding it, and the three sampler sites become one class.
`DescriptorAllocator` comes with them, because a material and a scene set need one and voxel's
hand-built pool is the same thing written once more.

### 2. A model onto the device

A helper that takes a `type::Model` to a `memory::Mesh`, and a registry that de-duplicates by
path the way retcon's does. The vertex layout the api does not own is what has kept this an
app's job; moving retcon's gives it one, and it is the same position, normal and uv that
`type::Model` already holds. The registry's handles are what
[milestone 3](m3-RenderableComponent.md)'s component names.

### 3. A lit mesh pass, with the look as data

retcon's mesh pipeline with set 2 as the scene. **The cel look is a choice of parameters, not a
pass**: bands, fill and outline are what `SceneRenderSettings` already separates from the
Vulkan, and a game that wants smooth shading sets one band. Whether the tree ever wants a
second lighting model — physically based materials, which retcon's own ADR-0020 chose glTF for
— is a question this step should leave room for and not answer.

### 4. A shadow map

retcon's `ShadowPass` on ADR-0044's target. retcon's handoff notes are the starting list: one
map shared by both frames in flight, and a fitted radius that has to grow by hand when the
scene grows past look-dev scale. A township or a world map larger than one mission is where
the second bites, and cascades are the usual answer — after the move, not as part of it.

### 5. A chain after the scene

The offscreen colour target, the LUT grade and the outline, as passes a consumer orders. This
is where the three target gaps above are closed, because a post chain is the first thing in
either tree that reads a target another pass wrote: a double-buffered target for a pass that
reads its own last frame, a format check, and a frame that lets a pass say where it belongs so
`passBefore` can go.

## What this needs decided

* **Whether this moves before [milestone 5](m5-SkeletalAnimation.md) is written.** The roadmap's
  case is that skinning is written against a mesh tier, and written against retcon's it is
  written twice. The move itself is retcon's to ask for under its ADR-0041, as a handoff, the
  way its previous rounds reached this tree.
* **Shaders on disk or embedded.** This tree embeds SPIR-V at build time with
  `v3d_add_shader`; retcon loads it from a directory so a shader can be swapped without a
  relink. A lit tier needs one rule for the shaders it ships and has to let a consumer bring its
  own, which is what a game's look is.
* **Colour space.** This tree authors colour in display space
  ([ADR-0009](../adr/0009-colour-authored-in-display-space.md)) and presents through a `UNORM`
  chain unless a consumer names another ([ADR-0049](../adr/0049-a-consumer-chooses-the-swapchain-format.md)).
  retcon asks for `B8G8R8A8_SRGB` and lights in linear. Lighting is only correct in linear, so
  the lit tier assumes a consumer that chose sRGB, and that assumption should be written into
  the record that accepts the tier rather than discovered by the next one to use it.

## Verification

**retcon's reference capture is the acceptance test**, and it is better than anything this tree
has for the purpose: a pixel comparison of its own scene, lit, shadowed and graded, which its
own qa policy calls the renderer's only net. The move is done when retcon builds against
this tree's tier with its copy deleted and that capture does not change. Each step above is a
separate change to retcon's pointer, so a capture that moves says which step moved it.

Here, by [ADR-0054](../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md), a lit
picture cannot be pinned — lighting is arithmetic the specification leaves to the
implementation, and so is filtered sampling. What the device suite can pin is what the
specification determines: a depth-only pass's depth at known vertices, a post pass that is the
identity, and a LUT that is the identity. Those catch a pass that reads the wrong target or
writes the wrong format, which is most of what goes wrong in a chain. Everything else is
validation silence and a screenshot, per [Testing.md](../Testing.md).

## Not in this milestone

* **Instancing.** One mesh per entity is retcon's tier today and what moves. Instancing is
  [milestone 5](m5-SkeletalAnimation.md#4-instancing)'s, because a horde of skinned characters is
  what needs it.
* **Merging draws, a second depth buffer, and the 2D pass reading set 0** — the rest of
  [RenderingPipeline.md](../RenderingPipeline.md#what-is-not-built-yet)'s list. None is needed
  by a lit scene, and each is its own change.
* **Ambient occlusion, fog, night and weather passes.** retcon lists them as polish, and they
  are passes a chain allows rather than pieces of it.
