# Pipelines, descriptors and shaders

How a pipeline is built, how shader data is bound, how draw items are ordered, and how shaders
reach the binary.

## Building a pipeline

`vulkan::pipeline::Builder` describes a graphics pipeline one chained call at a time.

```cpp
pipeline::Builder(device)
    .name("quad")
    .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
    .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
    .vertexBinding(0, sizeof(Canvas::Vertex))
    .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Canvas::Vertex, position))
    .set(uniforms->layout())     // set 0 first: sets are numbered in the order added
    .set(textures->layout())     // set 1
    .push(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Push))
    .colourFormat(colour)
    .build(cache);
```

Defaults:

- dynamic viewport and scissor, so a resize needs no rebuild;
- one sample, triangle list, filled polygons;
- no culling, counter-clockwise front face;
- no depth test or write;
- straight alpha blending (`Builder::Blend` defaults: colour `SRC_ALPHA`/`ONE_MINUS_SRC_ALPHA`,
  alpha `ONE`/`ONE_MINUS_SRC_ALPHA`, op `ADD`);
- dynamic rendering, no render pass.

Notes:

- `colourFormats({...})` takes 0..N attachments. An empty list writes no colour: a shadow pass.
  `colourFormat(f)` is the one-attachment form. Every attachment gets the same blend state.
- **The colour format is required**, and a pipeline that tests or writes depth also needs
  `depthFormat()`. The recorder's check compares these against the pass at bind time.
- `blend(Blend{...})` sets custom factors and turns blending on. A pass compositing into
  something composited later needs `destinationAlpha = ZERO`; the default erodes the source's
  alpha.
- `depthBias(true)` sets `depthBiasEnable` and adds `VK_DYNAMIC_STATE_DEPTH_BIAS`, so the values
  come from the pass.
- `depthWriteDynamic(true)` adds `VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE`, so a pass may choose
  whether the pipeline writes depth. The write flag given to `depth()` is the pipeline's own
  value, used when the pass names none. `Line`'s and `World`'s depth pipelines opt in. `Lit`'s
  do not, because a lit surface is opaque and always writes.

  Background: [ADR-0085](../../adr/0085-rendering-a-pass-chooses-whether-depth-is-written.md)
- `layout(VkPipelineLayout)` compiles against a layout the caller owns, for several pipelines
  that share one layout under one bound set. `set()` is then read only to detect a set 2, and
  `push()` only for stage flags. The caller destroys that layout.
- Shader modules belong to the builder and are destroyed with it. The pipeline and its layout are
  returned for `Resources` to own. Building twice gives two identical pipelines.

**`rasterization()`, `colourBlend()` and `dynamics()`** return the state `build()` will use;
`build()` calls these same functions. They exist because a compiled `VkPipeline` cannot be read
back, and a wrong value is a wrong picture rather than an error. For example, a depth bias left
out of the dynamic list compiles and validates cleanly, then uses the zero in the create info,
so `vkCmdSetDepthBias` does nothing. Test pipeline state through these functions.

**Two variants per depth state.** Dynamic rendering matches a pipeline to the attachments of the
pass it draws into, so a pipeline built with no depth format cannot draw into a pass with one.
Each renderer compiles its pipelines twice and picks by `Pass::depth()`.

## Descriptor sets and push constants

Sets are organised by how often their contents change.

| Set | Changes | Holds | Owner |
|---|---|---|---|
| 0 | per pass, per frame | the camera: `view`, `projection`, `viewProjection` (three `mat4`) and `viewport` (`vec4`), 208 bytes std140 | `vulkan::frame::FrameUniforms` |
| 1 | per material | one combined image sampler at binding 0, fragment stage (`FullScreen` declares one per source) | `Textures` (and `FullScreen`, `Grade`) |
| 2 | per pass, only for pipelines that declare it | a lit scene: `Scene` uniform (binding 0), shadow map (binding 1), joint palette storage buffer (binding 2) | `vulkan::renderer::Lit` |
| push | per draw | transform, tint, flags, a skinned object's first joint | the item |

Rules:

- **Every pipeline declares set 0's layout first**, even one that reads nothing from it. A set
  bound for one pipeline then stays bound across a switch to another built against the same
  layout. Compatibility runs from set 0 upwards, so pipelines with nothing beyond set 0 (lines)
  stay compatible with ones that add set 1.
- **Per-object data goes in push constants, never in a set.** An item binds at most one set of
  its own (set 1), and merging two adjacent items is a comparison of two handles.
- A pipeline that declares set 2 must be drawn in a pass that names a scene set; the recorder
  checks this.

**Set 0.** `FrameUniforms` owns the layout (one uniform buffer at binding 0, vertex and fragment
stages) and a list of slots per frame in flight, each a small buffer and a set that points at it
permanently. The recorder writes each pass's camera into the next slot of the current frame and
binds it for every item in the pass. Slots are added as passes need them and never returned; the
pass count settles in the first few frames. A slot is written during recording, after the fence
wait, so nothing is reading it.

Who reads set 0:

- The quad pipeline declares it and reads nothing. A canvas pushes its own projection, so two
  canvases in one pass may map different spaces.
- Lines, world quads and the lit pipelines read it.
- voxel's terrain pipeline reads it and nothing else per draw except a 16-byte push constant
  (the chunk origin), with its block palette at set 1.

**Push constants.** `DrawItem::pushCapacity` is 128 bytes, the Vulkan minimum guarantee.
Current blocks:

| Pipeline | Block | Size |
|---|---|---|
| Quad | `mat4 projection; uint text` | 68 bytes |
| Lit | `Lit::Object`: `mat4 model; vec4 baseColour; float outline; uint firstJoint` | 88 bytes |
| Line, World, FullScreen | none | |

The block is copied into the item by value, so the unused part of the 128 bytes is copied per
item per frame whether or not a pipeline declared it.

**Descriptor pools.** `vulkan::pipeline::DescriptorPool` holds one layout and a list of Vulkan
pools, each created for a fixed number of sets. When the last pool is full, another is added. A
set handed back with `release()` is retired through the ring and then reused; the pools are not
created with the free flag. A reused set holds whatever was last written into it, so the caller
writes it before binding.

| Pool | Sets per Vulkan pool |
|---|---|
| set 0 (`FrameUniforms`) | 32 |
| set 1 (`Textures`) | 64 |
| `FullScreen` sources | 8 |
| set 2 (`Lit` scenes) | frames in flight |

Background: [ADR-0008](../../adr/0008-shaders-descriptor-sets-by-update-frequency.md),
[ADR-0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md)

## Draw items and the sort key

A `DrawItem` describes one draw: pipeline and material handles, vertex and index buffers with
offsets, index type, the push block and its size, vertex/index/instance counts and firsts, an
optional scissor, and the optional `record` callback.

**`SortKey`** packs four 16-bit fields into one `uint64_t`, coarsest first, so a pass sorts on a
single comparison:

| Bits | Field | Set by |
|---|---|---|
| 63–48 | `layer` | the caller (painter order) |
| 47–32 | `pipeline` | `Pass::submit`, from the pipeline handle's slot |
| 31–16 | `material` | `Pass::submit`, from the material handle's slot |
| 15–0 | `depth` | the caller (view depth quantised) |

Layer is first because 2D content is painter ordered. Pipeline then material groups the items
that can share bindings. `Pass::submit` overwrites the pipeline and material fields from the
item's handles, so a caller sets only layer and depth. Sorting is opt-in per pass and stable.

Handles are used instead of pointers because a pointer's value depends on the allocator, which
would reorder a frame differently on each run. There is no geometry field in the key and no
geometry handle; items hold raw `VkBuffer`s.

## Shaders

The engine's shaders are in [api/render/shaders](../../../api/render/shaders). They are compiled to
SPIR-V by `glslc` at build time and embedded in the library as `uint32_t` arrays. They are not
data files: CMake does not copy per-app data into the build tree, and a shader file beside an
executable would go stale silently. `v3d_add_shader`, `OUTPUT`, `DEFINES`, includes and depfiles
are described in [contributing/Build.md](../../contributing/Build.md#shaders).

| Shader | Used by |
|---|---|
| `quad.vert`, `quad.frag` | `Quad` |
| `line.vert`, `line.frag` | `Line` |
| `world.vert`, `world.frag` | `World` |
| `fullscreen.vert`, `grade.frag` | `FullScreen`, `Grade` |
| `lit/mesh.vert`, `lit/cel.frag`, `lit/outline.vert`, `lit/outline.frag`, `lit/shadow.vert` | `Lit` |
| `skinned_mesh.vert`, `skinned_outline.vert`, `skinned_shadow.vert` | `Lit`, the three lit vertex stages compiled with `SKINNED` |

The lit shaders share their blocks through includes:

- `lit/lit.glsl` declares the `Camera` (set 0), `Scene` (set 2, binding 0) and `Object` (push)
  blocks. Every lit shader includes it, so a block is written once.
- `lit/pose.glsl` defines `pose()`: the identity normally, or the weighted joint matrices when
  `SKINNED` is defined. When `SKINNED` is defined it also includes `lit/skin.glsl`: the palette
  at set 2, binding 2, and the joint and weight attributes at locations 3 and 4.

`Lit::Shaders::embedded()` returns the built-in SPIR-V; a consumer may pass its own (see
[api/rendering/Lighting.md](../../api/rendering/Lighting.md#replacing-the-lit-shaders)).
Pipeline creation is the only check that a replacement matches the layout. When changing a
block in `lit.glsl`, change `SceneUniforms` or `Lit::Object` to match; reordering members is not
caught by anything.
