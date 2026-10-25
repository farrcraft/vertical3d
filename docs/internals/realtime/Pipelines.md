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
    .vertexBinding({.binding = 0, .stride = sizeof(Canvas::Vertex)})
    .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Canvas::Vertex, position)})
    .set(uniforms->layout())     // set 0 first: sets are numbered in the order added
    .set(textures->layout())     // set 1
    .push({.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .size = sizeof(Push)})
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
| 1 | never, for a textured pipeline; per material for a full-screen one | a textured pipeline's: every texture, an array of combined image samplers at binding 0, fragment stage, indexed by slot. A full-screen pipeline's: one combined image sampler per source | `Textures`; `FullScreen`, `Grade` |
| 2 | per pass, only for pipelines that declare it | a lit scene: `Scene` uniform (binding 0), shadow map (binding 1), joint palette storage buffer (binding 2), object storage buffer (binding 3) | `vulkan::renderer::Lit` |
| push | per draw | a projection or an origin, and flags | the item |

Rules:

- **Every pipeline declares set 0's layout first**, even one that reads nothing from it. A set
  bound for one pipeline then stays bound across a switch to another built against the same
  layout. Compatibility runs from set 0 upwards, so pipelines with nothing beyond set 0 (lines)
  stay compatible with ones that add set 1.
- **Per-object data never goes in a set of its own.** An item binds at most one set of its own
  (set 1), and merging two adjacent items is a comparison of two handles. A pipeline outside a
  lit scene pushes its per-object data. A lit pipeline pushes nothing, and reads its object from
  the scene's object buffer at set 2 by instance index. A lit draw names its object as its first
  instance, and `Lit::object()` returns that index.
- **A texture is a slot, not a set.** Every textured item names the one material `Textures`
  holds, so set 1 is bound once while the pipeline stays bound, and the item carries its
  texture's slot: a quad and a world quad in a push constant, a lit object in its
  `Lit::Object::albedo`. The binding is partially bound and may be written while frames that
  bind it are in flight, so a slot is written only when it is free: one released is freed
  through the ring. A device without runtime descriptor arrays, partially bound bindings,
  updates while pending, and dynamic and non-uniform indexing of sampled image arrays is not
  selected.
- **A full-screen pass binds its sources in a set of its own** at set 1, because a source mixes
  two-dimensional images, depth images in their read-only layout and a grade's volume table,
  and names them as they are when it is made.
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
| Quad | `mat4 projection; uint text; uint texture` | 72 bytes |
| World | `vec4 tint; uint texture` | 20 bytes |
| Line, FullScreen, Lit | none | |

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
| set 1 (`Textures`) | one set, the texture array, in a pool of its own |
| `FullScreen` sources | 8 |
| set 2 (`Lit` scenes) | frames in flight |

Background: [ADR-0008](../../adr/0008-shaders-descriptor-sets-by-update-frequency.md),
[ADR-0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md),
[ADR-0088](../../adr/0088-lit-per-object-data-in-a-storage-buffer.md),
[ADR-0109](../../adr/0109-textures-one-bindless-array-a-draw-names-a-slot.md)

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

**The api reads no shader from disk.** A game that wants to swap a shader without relinking reads
the SPIR-V words itself and hands them to `Builder::shader()`, or to `Lit::Shaders` for the lit
pipelines. A loader in the api would make every app ship and find shader files, to serve the one
that wants to replace them.

| Shader | Used by |
|---|---|
| `quad.vert`, `quad.frag` | `Quad` |
| `linear_quad.frag` | `Quad` drawing into an `_SRGB` format: `quad.frag` compiled with `LINEARISE` |
| `line.vert`, `line.frag` | `Line` |
| `world.vert`, `world.frag` | `World` |
| `fullscreen.vert`, `grade.frag` | `FullScreen`, `Grade` |
| `grade_linear_scene.frag`, `grade_linear_target.frag`, `grade_linear.frag` | `Grade`: `grade.frag` compiled with `LINEAR_SCENE` for an `_SRGB` scene, `LINEAR_TARGET` for an `_SRGB` target, or both |
| `lit/mesh.vert`, `lit/cel.frag`, `lit/outline.vert`, `lit/outline.frag`, `lit/shadow.vert` | `Lit` |
| `skinned_mesh.vert`, `skinned_outline.vert`, `skinned_shadow.vert` | `Lit`, the three lit vertex stages compiled with `SKINNED` |

`srgb.glsl` holds the sRGB decode and encode. `quad.frag` and `grade.frag` include it, so the
two agree to the bit.

The lit shaders share their blocks through includes:

- `lit/lit.glsl` declares the `Camera` (set 0) and `Scene` (set 2, binding 0) blocks, the
  `Object` struct, and the `Objects` buffer of them (set 2, binding 3). In a vertex stage,
  `object` is the entry at `gl_InstanceIndex`. `cel.frag` reads the entry at the index `mesh.vert`
  passes it at location 3. Every lit shader includes `lit.glsl`, so a block is written once.
- `lit/pose.glsl` defines `pose()`: the identity normally, or the weighted joint matrices when
  `SKINNED` is defined. When `SKINNED` is defined it also includes `lit/skin.glsl`: the palette
  at set 2, binding 2, and the joint and weight attributes at locations 3 and 4.

`Lit::Shaders::embedded()` returns the built-in SPIR-V; a consumer may pass its own (see
[api/rendering/Lighting.md](../../api/rendering/Lighting.md#replacing-the-lit-shaders)).
Pipeline creation is the only check that a replacement matches the layout. When changing a
block in `lit.glsl`, change `SceneUniforms` or `Lit::Object` to match; reordering members is not
caught by anything. `Lit::Object` is padded to 96 bytes, the std430 stride of an array of
`Object`, and a static assert holds it there.
