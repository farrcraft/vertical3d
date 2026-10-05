# A Lit Scene — Images, A Mesh Registry, And Passes The Recorder Draws

Drafted 2026-10-03 against `e5423af`, with milestone 3's work in the tree and uncommitted, and
closed the same day. Eleven steps across `api/render`, `voxel` and the documents, taking up
[milestone 4](../../roadmap/completed/m4-LitScene.md) of [the game engine roadmap](../../roadmap/completed/GameEngine.md).
Every step is in this tree. retcon is the evidence for the tier and adopts it after it ships; it
is neither changed nor run here.

The roadmap describes this milestone as a move: retcon's `engine/renderer/` comes across, and
retcon deletes its copy. The surveys say it cannot be a straight copy. **retcon declined this
tree's frame model** (its D8), because `Recorder` binds two descriptor sets and retcon's tier
binds three. So every retcon pass hand-records `vkCmdBeginRendering` with its own barriers,
outside `Frame`, `Pass` and `DrawItem`. Moving those classes unchanged would give the api two
ways to record a frame, with the pass order fixed in a class only one game uses. This plan moves
retcon's *content*: the image classes, the registry, the shaders, the cel and shadow arithmetic
and the settings. It writes that content into the frame model the rest of the api already draws
through. The recorder learns the third set, which is the one thing D8 objected to.

## What the surveys found

retcon was read at `3d22935`, which is still its HEAD, and it pins this tree at `13a9557`. This
tree was read in its working state.

**In retcon:**

* **The `gpu/` tier is small and depends only on this tree.** `GpuImage` (an image, its
  `memory::Allocation` and its view, from a `Spec`) and `Sampler` (a filter and a wrap) together
  run to about 270 lines. `Texture` is an RGBA8 `UNORM` `GpuImage` uploaded through
  `memory::Uploader::oneShot`, plus a 1×1 white. `Spirv` reads a file into words.
  `DescriptorAllocator` holds one pool for retcon's three fixed layouts. Each frame in flight
  gets a camera set and a scene set, and each material gets a set, up to 64.
* **The sets.** Set 0 is `CameraUbo`, which is `frame::FrameUniforms::Camera` member for member.
  Set 1 is one combined image sampler, the albedo. Set 2 is `SceneUbo` at binding 0 and the
  shadow map at binding 1:
  * `SceneUbo` holds `lightDir` (w is the fill strength), the cel thresholds, the cel bands,
    `lightViewProj` and `shadowParams`.
  * The push constant is 96 bytes: the model matrix, `baseColorFactor` and `outlineThickness`.
    It is why this tree's `DrawItem::pushCapacity` became 128.
* **Three pipelines share one layout.**
  * Cel is `mesh.vert` and `cel.frag`: a key light and a derived fill, three bands by threshold,
    and 3×3 PCF dropping one band in shadow.
  * Outline extrudes along the object-space normal with front faces culled, drawn black before
    cel.
  * Shadow is `shadow.vert` alone, with dynamic depth bias.
  * `SceneRenderSettings` is the look as plain structs, with no Vulkan in it.
* **The shadow is one 2048² orthographic map.**
  * `directionalLightMatrix(dir, centre, radius)` builds the light's matrix.
  * `fitShadowFrustum` takes the centroid of the casters' positions. Its radius is the farthest
    caster plus a three metre margin, and it is fitted once at scene load.
  * retcon's ADR-0021 records the map being shared by both frames in flight as a known
    simplification.
* **The post chain is one pass.** `OffscreenPass` is a colour and depth pair in the swapchain's
  format, never resized. `LutPipeline` draws a full-screen triangle that grades through a 16³
  LUT read from a 256×16 PNG strip. **The outline is not post-processing**: it is a mesh
  pipeline drawn in the scene pass, although the roadmap lists it in the chain.
* **`MeshRegistry` de-duplicates by path and never releases.**
  * The handle is a bare index.
  * Each entry holds a `memory::Mesh`, a material index and a base colour factor.
  * A mesh with a texture gets a new material slot whether or not another mesh named the same
    image.
  * Its vertex is `{vec3 position; vec3 normal; vec2 uv;}`, which is `type::Model::Vertex`
    exactly.
  * `GltfLoader` already parses through `v3d::asset::loader::Gltf`.
* **Shaders are loaded from a directory**: `RETCON_SHADER_DIR`, compiled by retcon's own CMake,
  "swappable without a relink". The camera block is repeated in three shaders and the scene block
  in two, which its handoff lists as a trap.
* **Colour.** retcon asks for `B8G8R8A8_SRGB`, and its offscreen target uses the same format. It
  writes linear light and lets the target encode it. **But its albedo is uploaded as `UNORM`**,
  so a display-space texture enters the lighting undecoded. Its reference capture has that baked
  in.
* **Its acceptance test is `--scene lookdev` against `phase1-grade.png`, at zero differing
  pixels.**
  * The scene is a ground plane, two characters and ten props under one key light, with the
    `urban-ruin` LUT.
  * The camera is orthographic at 45°, and the frame is 1280×720 after 30 warm-up frames.
  * No retcon test touches Vulkan.
* **retcon recorded that the look stays app-side.** Its migration plan's ledger C says
  `MeshPipeline`, `ShadowPass`, `LutPipeline` and `SceneRenderer` "stay app-side… the look is
  retcon's", and it says not to reopen that ledger or D8. This milestone reverses the first
  part. The reversal holds only if the look stays retcon's after the move, which is why steps 6
  and 7 make the look parameters and the shaders replaceable.

**In this tree:**

* **An image, its allocation and its view are open-coded three times**: in `frame::DepthBuffer`,
  in `frame::RenderTarget`, and in `memory::TextureFactory` (into the `pipeline::Texture` POD).
  `vkCreateSampler` is called at those same three sites. Two of them use identical settings.
* **Descriptor pools are written twice.** `FrameUniforms::addPool` (32 sets) and
  `Quad::addPool` (64) each grow a list of pools for one layout. voxel builds a third by hand,
  for one uniform buffer (`Renderer.cxx:161-253`).
* **`Recorder` binds sets 0 and 1 only** (`Recorder.cxx:281`, `:292`), so a set 2 has no path.
* **Nothing records `vkCmdSetDepthBias`.** `Builder::depthBias` compiles a pipeline with the
  dynamic state, and `PipelineBuilderTest` builds one. Nothing draws with it, and neither
  `Pass` nor `DrawItem` carries a bias.
* **No code anywhere puts a `type::Model` on the device.** voxel's chunk builder is the only
  caller of `memory::Mesh`'s constructor.
* **Two more target gaps beyond the roadmap's three.**
  * `RenderTarget::recreate` destroys its images immediately rather than through the ring.
  * A target has to have a colour image, so a shadow map has no target to be.
* **Nothing in the device suite draws a depth-only pass or reads a depth value back.**
* **Line drawing already covers `DebugLines`.** `renderer::Line` draws depth-tested lines from a
  `LineCanvas` at set 0 only, which is what the editor's viewports use. So `DebugLines` and
  `GridOverlay` do not move. The grid's `LineSink` adapters are three lines over `LineCanvas`,
  which a consumer writes.
* **No app here would draw a lit model.** voxel lights its own vertex layout per vertex, the
  editor is wireframe, and the rest are quads. The tier's in-tree consumer is therefore the device
  suite, and voxel consumes step 3.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--the-record-the-lit-tier-is-the-recorders) | The record: a pass carries a scene set at set 2 and a depth bias, and the lit tier draws through the recorder | `docs/adr` | **[0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md)** | done; accepted |
| [2](#step-2--an-image-and-a-sampler) | `memory::Image` and `pipeline::Sampler`, with `DepthBuffer`, `RenderTarget` and `TextureFactory` rewritten over them | `api/render` | — | done |
| [3](#step-3--a-descriptor-pool) | A descriptor pool for one layout, with `FrameUniforms`, `Quad` and voxel over it | `api/render`, `voxel` | — | done |
| [4](#step-4--the-recorder-binds-set-2-and-a-bias) | `Pass::scene` and `Pass::depthBias`, recorded | `api/render` | 0064 | done |
| [5](#step-5--a-depth-only-target) | A target with no colour image, and a device case reading its depth back | `api/render` | 0044 | done |
| [6](#step-6--a-model-onto-the-device) | `MeshRegistry`, `MeshHandle` and `component::Mesh` | `api/render` | **[0065](../../adr/0065-meshes-shared-registry-keyed-by-path.md)** | done; accepted |
| [7](#step-7--the-lit-pass-with-the-look-as-data) | The colour record, the shader rule, the cel and outline pipelines, and the walk | `api/render` | **[0066](../../adr/0066-lighting-light-in-linear-draw-to-srgb.md)**, **0067 (removed)** | done; accepted |
| [8](#step-8--a-shadow-map) | The shadow pass, the light's matrix and the fit, bound at set 2 | `api/render` | 0064 | done |
| [9](#step-9--targets-a-chain-can-use) | Double-buffered targets, a format check, and passes placed by what they read | `api/render` | **[0068](../../adr/0068-rendering-order-passes-by-what-they-read.md)** | done; accepted |
| [10](#step-10--the-chain-after-the-scene) | A full-screen pass and the LUT grade | `api/render` | 0068 | done |
| [11](#step-11--retcons-scene-reproduced-here-and-the-handoff) | retcon's look-dev scene reproduced in the device suite, and the handoff note | `api/render`, `docs` | — | done |

Step 1 comes first. Steps 2 and 3 are independent of it and of each other, and are the only
steps that remove code. Step 4 needs step 1, and step 5 needs step 2. Step 6 needs steps 2
and 3. Step 7 needs steps 4 and 6, step 8 needs steps 5 and 7, step 9 needs step 2, and step 10
needs steps 7 and 9. Step 11 needs everything.

---

### Step 1 — The record: the lit tier is the recorder's

**ADR-0064: what a pass binds, and who records a lit scene.** It goes in as `proposed` and is
accepted after step 7 draws through it.

**A pass may carry a scene set, bound at set 2 once for the pass, and a depth bias, recorded
once for the pass.** Both are per pass rather than per item, for the same reason set 0 is. A
light, its shadow map and a bias change when the scene does, not when the object does. So
binding them costs one bind per pass, and the sort key does not change.

**This amends [ADR-0008](../../adr/0008-shaders-descriptor-sets-by-update-frequency.md) rather than contradicting
it.** That record rejected a third set *per object*. A per-pass set is set 0's frequency split
in two: set 0 is the camera every pipeline shares, and set 2 is what only lit pipelines read.
retcon placed it at 2 for the reason this tree would: widening set 0 would change every shader
here. Compatibility runs from set 0 upwards, so a quad pipeline and a lit one still share set 0
within a pass.

**Alternatives the record weighs:**

* **retcon's `SceneRenderer`, moved as it is.** It is the least work and keeps retcon's
  capture unchanged by construction. But it is a second frame model beside the recorder, its
  pass order is a class rather than a frame, and nothing drawn by `Quad`, `World` or `Line`
  could share a pass with it. retcon draws its own canvas and grid for exactly that reason.
* **The scene data in set 0.** This needs no recorder change, but it widens a block that every
  pipeline in the tree declares.
* **The scene data in push constants.** `lightViewProj` alone is 64 bytes of the 128, and the
  shadow map cannot go there at all.

**What the record also says.** The lit tier's pass order belongs to the frame, as every other
pass's does. The look is data and the shaders are replaceable, which is what keeps retcon's
"the look is ours" true after the move. That half is ADR-0067's, and this record cites it.

### Step 2 — An image and a sampler

**The shape.** `vulkan/memory/Image.h` is retcon's `GpuImage`, renamed to sit beside
`Allocation`:

```cpp
class Image final {
 public:
    struct Spec {
        uint32_t width, height, depth = 1;     // a depth above 1 makes a 3D image and view
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t mipLevels = 1;
    };
    Image(const boost::shared_ptr<device::Device>& device, const Spec& spec);
    // move-only; image(), view(), spec()
};
```

`vulkan/pipeline/Sampler.h` holds one `VkSampler`, built from a filter, a wrap and a border.
The border has to be a parameter, because `DepthBuffer`'s sampler clamps to an opaque-white
border and the other two clamp to the edge. Today's three sites become three constructions of
one class.

**Rewritten over them:**

* **`DepthBuffer`** holds an `Image` and an optional `Sampler`.
* **`RenderTarget`** holds the same. Its `recreate()` hands the old ones to `Ring::retire`
  rather than destroying them. That closes the gap the survey found, and it is
  [ADR-0061](../../adr/0061-resources-explicit-release-generational-handles.md)'s rule applied to a resize.
* **`pipeline::Texture`** stops being a POD of raw handles. It holds a `boost::shared_ptr` to an
  `Image` and one to a `Sampler`. `owned = false` becomes "shares its owner's image", which a
  shared pointer says without a flag.
* **`TextureFactory`** builds an `Image` and uploads into it. Its single-channel swizzle moves
  into `Image::Spec` as an optional component mapping.

**retcon's `Texture` does not come as a class.** It is `TextureFactory::create` under another
name, and its 1×1 white is a call to that. A white texel *is* worth having, because a material
with no texture needs one. It becomes `TextureFactory::white()`.

**Tests.** No new picture. **The four pinned references are the acceptance test**, because
every one of them draws through a `RenderTarget` and a texture from the factory. If a picture
moves or the suite stops being silent, the rewrite changed something. `ReleaseTest` carries
the ring. One new device case resizes a target in flight and asserts silence, which fails if
`recreate()` destroys while a frame still reads.

**State: done.** All 25 suites pass, and the four pinned pictures came through unchanged on the
Radeon. cpplint is clean, and so is `out/build/verify`, which runs `/analyze` and clang-tidy with
warnings as errors. Three things came out differently:

* **`RenderTarget`'s constructor takes the ring**, as its second argument. A target has no other
  way to reach it, and only the device suite constructs one.
* **`TextureFactory::white()` was not added.** `Quad::white()` is already that texture, and it is
  what step 6's untextured material will name.
* **`TextureFactory` makes one sampler and shares it**, rather than one per texture as before. No
  picture moved, and a device's sampler count is a limit that a sampler per texture spends for
  nothing.

The resize case fails with the ring taken out of `RenderTarget::destroy`, and passes with it in.
A second case draws from a texture registered from a target after that target was resized. That
was a read of destroyed images before this step and is a read of the old ones now. Nothing could
have shown the first state failing, so that case pins the new contract rather than a fix.

### Step 3 — A descriptor pool

**The shape.** `vulkan/pipeline/DescriptorPool.h` hands out sets of one layout. It grows a
list of pools by a fixed count, and takes sets back through the ring, because a pool created
without `FREE_DESCRIPTOR_SET` cannot free one. That is the code `FrameUniforms::addPool` and
`Quad::addPool` each wrote, with `Quad`'s spare list made general. retcon's
`DescriptorAllocator` is the same thing fixed to three layouts. Here it is one class used once
per layout.

**Rewritten over it:**

* **`FrameUniforms`.** Its slots keep their buffers. Only the pool moves.
* **`Quad`'s material sets.** Its `spare_` becomes the pool's.
* **voxel's scene set.** voxel's `createUniforms` keeps its `DeviceBuffer` and loses its pool,
  about 30 lines.

**Tests.** A device case takes 100 sets of a one-binding layout from a pool sized 32, which
forces three grows. It returns half through the ring, and checks that the next 50 come from
the returns rather than from a fourth pool. Every existing device case draws through set 0 and
set 1, so they carry the rewrite. voxel is verified by running it and reading the log, per
[Testing.md](../../contributing/Testing.md#verifying-a-rendering-change).

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over
`v3dlib_render`, both render suites and voxel. voxel ran on the Radeon with validation on, and
its log was silent through shutdown. Two things came out differently:

* **The pool owns its layout.** It is built from the bindings the caller hands it, which also
  size each vulkan pool. That removed three more blocks of layout code, one each from
  `FrameUniforms`, `Quad` and voxel, beyond the three blocks of pool code the step was drafted
  to remove.
* **`FrameUniforms` takes the ring** in place of a frame count, because its pool needs one.
  `DeviceContext` is its only caller.

The device suite's three pool cases are the ones the step drafted, apart from a third that pins
the wait. Each was shown to fail with the fix taken out. With `release` doing nothing, the
reused sets are new ones and a fifth pool is created. With the set handed back at once rather
than through the ring, a set is reissued while a frame that began before its release is in
flight.

### Step 4 — The recorder binds set 2 and a bias

**The shape**, on `Pass`:

```cpp
Pass& scene(VkDescriptorSet set);                         // bound at set 2 for every item
Pass& depthBias(float constant, float slope, float clamp = 0.0f);
```

`Recorder` binds the scene set after set 0, for any pipeline whose layout declares a set 2. A
pipeline built without one keeps today's binding. It records the bias when it binds a pipeline
built with `depthBias(true)`. A pass that names a bias and draws an unbiased pipeline is not an
error, because a shadow pass may also draw debug geometry. **A pass that draws a biased
pipeline and names no bias is an error**, thrown at record time. Vulkan would otherwise draw
with whatever bias was last set, and nothing would report it.

`Pipeline` gains whether its layout declares set 2 and whether it is biased. `Builder` knows
both when it builds.

**Tests.** A headless case gives an unbiased pass a biased pipeline and expects a throw. That
check is made before any command buffer, so it needs no device. Two device cases:

* **A three-set pipeline.** It draws with a scene set whose uniform is a colour the fragment
  shader writes, into the flat-colour target `quad.png` already uses, and compares it against a
  new reference.
* **The same draw with no scene set named**, which expects a throw rather than a validation
  error.

**State: done.** All 25 suites pass. cpplint is clean, and so is a whole-tree pass of
`out/build/verify`, which was needed because `Pass.h` reaches every app. The set 2 binding was
shown to matter by taking it out: the validation layer reports set 2 as unbound, and every
texel comes back the clear colour. Three things came out differently:

* **The scene case pins texels, not a reference.** One flat colour, in channels at the ends of
  their range, under a triangle covering every pixel, is described completely by that one
  value. A committed picture would add a file and a CI round and assert nothing more. So this
  step blesses no picture, and the rule that the first new reference goes through a pull
  request alone moves to whichever later step blesses one first.
* **`Recorder::check(pass, pipeline)` is public and static.** The headless cases call it with
  a `Pipeline` value and a `Pass`, which is what "checked before any command buffer" needed.
  The recorder calls it whenever it binds a pipeline.
* **A pipeline declaring set 2 draws with set 1 left unbound** when its shaders read nothing
  there, and validation is silent about it. The scene case relies on that, so a lit pass with
  an untextured material will not need a placeholder set 1.

Two linker failures (`LNK1163`) turned up while building this step, one in each build directory,
each naming an object compiled in the same pass. Each object linked once it was deleted and
compiled again, and the source did not change. That is the build environment, not this step.

### Step 5 — A depth-only target

A shadow map is a target with depth and no colour, and `RenderTarget` requires a colour format.
**`RenderTarget` accepts `VK_FORMAT_UNDEFINED` for colour when depth is sampled**, and the
recorder skips the colour attachment for it. That is the `colourFormats({})` the builder
already allows. It is a smaller change than a second target class, and the recorder already
finds a target's writers and readers by identity.

**Tests: the depth the specification determines.** A depth-only pass draws a quad at constant
depth 0.25 and another at 0.75 into a D32 target, and reads the image back.

* **Constant-depth planes are exact.** Interpolation across a plane of one depth gives that
  depth at every covered sample, and a D32 float holds 0.25 and 0.75 exactly. So the case
  asserts the value at the centre of each quad and 1.0 (the clear) outside both.
* **A second case draws the same quads with a bias** and asserts that the biased depth is
  larger. The magnitude is the implementation's, so it pins only the direction.

**`frame::Capture` gains a depth source.** This is a raw float readback, compared in the test
rather than written to a PNG, because a reference picture of depth would round it.

**State: done.** All 25 suites pass. cpplint is clean, and so is a whole-tree pass of
`out/build/verify`. The whole device suite, 26 cases, is silent with
`VK_LAYER_VALIDATE_SYNC=1`, which this step needed because it adds the depth readback's
barriers. The depths are exact on the Radeon: a quarter and three quarters at the two quads'
centres, and the clear between them. The bias case fails with the recorder's constant zeroed
and passes with it in. Four things came out differently:

* **A frame can be recorded with no image of its own.** A shadow pass names its own target,
  so the frame-level transitions now skip an empty `Recorder::Target`. Guarding them pushed
  the frame's `record` past clang-tidy's cognitive complexity limit, and the target barriers
  moved into `openTarget` and `closeTarget` rather than taking a suppression.
* **`Quad::texture(target)` on a target with no colour gives back the white texture**, as
  `depthTexture` already did for a target with no depth to read. Otherwise it would have
  registered a texture with no image.
* **A sampled depth image carries `TRANSFER_SRC`**, which the readback needs. It is the same
  grant a target's colour image already had.
* **The depth cases skip, with a message, on a device with no sampled `D32_SFLOAT`.** Both the
  Radeon and lavapipe offer it, so neither skips. Any other depth format's copy is not the
  float a test compares, so `Capture` throws for one rather than returning bytes that look
  like depths.

### Step 6 — A model onto the device

**ADR-0065: a mesh can be registered, de-duplicated by path, and released.** It amends
[ADR-0010](../../adr/0010-meshes-owned-by-the-app-that-built-them.md) in the way that record's third
alternative foresaw: a cache built on top of app-owned meshes, owning `memory::Mesh` objects
the way a chunk does. A mesh still never goes in `Resources`, and the sort key still has no
geometry field. `DrawItem` keeps raw buffers, filled at walk time from the registry.

**The shape**, in `realtime/MeshRegistry.h`:

```cpp
using MeshHandle = Handle<MeshTag>;           // slot and generation, as ADR-0061's handles

class MeshRegistry final {
 public:
    MeshHandle load(const std::string& path);                  // de-duplicated by path
    MeshHandle add(const std::string& name, const type::Model& model);
    void release(MeshHandle handle);                           // geometry and material, via the ring
    const Entry* resolve(MeshHandle handle) const;             // nullptr once released
};
```

* **The entry holds:** the `memory::Mesh`, a `MaterialHandle` from `Quad`'s material path (white
  when the model names no texture), and the base colour factor.
* **Albedo is de-duplicated by its own path**, which retcon's is not. Two props sharing one
  atlas share one texture and one material, which is also what lets their draws sort together.
* **The vertex layout is `type::Model::Vertex`.** The api owns it already, and it is retcon's
  32 bytes field for field. One static assertion pins the size and the offsets the pipeline
  declares.
* **Loading goes through `asset::Manager`** with `Type::ModelGltf`, as retcon's `GltfLoader`
  does. Merged primitives and the first material are
  [ADR-0030](../../adr/0030-models-one-interleaved-array.md)'s, and
  splitting by material is milestone 5's.

**`component::Mesh`**, beside `component::Sprite`, is the shape
[milestone 3 handed over](RenderableComponent.md#step-6--the-mesh-component-handed-to-milestone-4):

```cpp
struct Mesh final {
    MeshHandle mesh;
    bool castsShadow = true;
};
```

**Linking.** `v3dlib_render` already links `v3dlib_asset` publicly, and the manifest already
lists it.

**Tests.** Two headless cases:

* the vertex layout's static assertions;
* a handle released and then resolved gives nothing, which is `Registry`'s generation rule and
  is shared with the texture handles.

Three device cases, on `three_primitives.glb` and `embedded_texture.glb` from
[`api/asset/tests/data`](../../../api/asset/tests/data/):

* loading one path twice gives one handle and one upload;
* two models naming one image share a material;
* a mesh released while a frame is in flight keeps the frame silent.

**State: done; ADR-0065 accepted**, and ADR-0010's header says it is amended. All 25 suites pass. cpplint is clean, and so is a whole-tree pass
of `out/build/verify`. There are four device cases:

* a path loaded twice is one upload and one handle;
* an image packed into a `.glb` is uploaded as the albedo;
* two models naming one image share its texture and its material, and the albedo outlives the
  first of them to be released;
* a mesh released while a frame draws it keeps the frame silent. This case fails with the ring
  taken out of `release`.

Five things came out differently:

* **The two headless cases are not separate cases.** The vertex layout is four
  `static_assert`s in `MeshRegistry.cpp`, which fail the build rather than a test. A released
  handle resolving to nothing needs a registry, and a registry needs a device, so the shared
  albedo case asserts it.
* **An albedo that cannot be found is drawn white and reported**, rather than failing the load.
  `three_primitives.glb` names an `albedo.png` that is not beside it, and so would a model whose
  artist renamed a file. A scene that loads with a white prop is easier to fix than one that
  does not load.
* **`add()` resolves a named texture from the asset manager's root, and `load()` resolves one
  beside the file**, as cgltf does.
* **The destructor releases nothing.** Releasing allocates, and clang-tidy holds a destructor
  to not throwing. It destroys its meshes at once, as `Resources` does, and an albedo stays
  registered with `Quad` until the context goes.
* **Linking did change.** `v3dlib_render` now names `v3dlib_type` itself, publicly and in the
  manifest, because `MeshRegistry.h` names `type::Model`. It had reached it only through
  `v3dlib_asset`, and the linking rules ask a library to name what it uses.

Albedo is uploaded `UNORM`, as every texture is today. Step 7's colour record changes that.

### Step 7 — The lit pass, with the look as data

Two records go in first, because the pass is written against both.

**ADR-0066: the lit tier assumes linear light.** Lighting is correct only in linear, so the
lit pipelines are written for a target that encodes on store:

* an `_SRGB` swapchain named under [ADR-0049](../../adr/0049-swapchain-caller-picks-the-format.md),
  and an offscreen target in an `_SRGB` format;
* **albedo uploaded as `_SRGB`**, so that a display-space texture is decoded before it is lit.
  `TextureFactory::create` gains an encoding argument whose default stays `UNORM`, so
  [ADR-0009](../../adr/0009-colour-display-space-unorm-swapchain.md) is unchanged for everything that
  is not lit.

The second bullet is where this tier and retcon's part. retcon uploads `UNORM` and its capture
holds that, so **adopting this moves retcon's reference once**, by exactly the albedo's
decoding. The record says so, so that retcon's adoption expects the diff rather than hunting
for it. The alternative is to copy retcon's defect so that its capture holds. Then every later
consumer inherits a tier that is not linear in the one place it claims to be.

**ADR-0067: the api's lit shaders are embedded, and a consumer may hand in its own.**

* The api's are compiled by `v3d_add_shader` like the rest, for the reason
  [RenderingPipeline.md](../../api/Rendering.md#shaders) gives: a shader on disk beside an
  executable goes stale silently.
* Every lit pipeline takes its modules as SPIR-V words, defaulting to the embedded ones, so a
  game that loads from a directory hands over what it loaded. retcon's "swappable without a
  relink" survives in the consumer.
* A shader that replaces one must declare the same sets and push constant. The record names
  that as the contract, and pipeline creation's validation is the check.
* retcon's `loadSpirv` comes across as `pipeline::spirv(path)`, because every consumer that
  loads from disk would write it again.
* **The repeated uniform blocks become one include**, `shaders/lit.glsl`, through glslc's
  `#include`. That closes the trap retcon's handoff lists.

**The shape**, beside `renderer::Quad`, `World` and `Line`:

* **`renderer::Lit`** owns the layout (sets 0, 1 and 2, and a 96-byte push constant) and the
  outline and cel pipelines. It also owns a `DescriptorPool` for set 2, and the scene uniform
  per frame in flight.
* **`realtime::LitSettings`** is retcon's `SceneRenderSettings`: the light, the bands and
  thresholds, the outline thickness, and the shadow's biases and strength, with retcon's
  defaults. One band and no outline is smooth shading. The lighting model is a parameter that
  admits nothing but cel today. A physically based model is a second value when someone wants
  one, and the roadmap asks that it be left room for and not answered.
* **`realtime::meshes(registry, alpha, meshRegistry, lit, pass)`** walks
  `view<const ecs::component::Transform, const component::Mesh>()` through
  `interpolated<Transform>`, as `sprites()` does. It submits an outline item and a cel item per
  entity into the pass. The layer orders outlines first, as retcon draws them.

**Tests.** Headless:

* `LitSettings` packs into the scene uniform at retcon's std140 offsets (0, 16, 32, 48 and 112).
  A shader reads those offsets, and nothing else would catch a reorder.
* the walk skips an entity whose handle was released.

On the device: the lit scene of step 11, early, so that this step is verified by silence and
by looking at it. **ADR-0064 is accepted here.**

**State: done. ADR-0064, ADR-0066 and ADR-0067 are accepted.** ADR-0008 and ADR-0009 now say they are amended. All 25 suites pass, cpplint is clean,
and so is a whole-tree pass of `out/build/verify`. The device suite draws a lit, outlined cube
from an entity, through the recorder, into an `R8G8B8A8_SRGB` target. It is silent, and the
centre pixel is the cube's top face, exactly `255,0,0`: the lit band of its own colour, since a
top face faces the key light. Some pixels are black, which is the outline. A second case shows
the walk skips an entity whose mesh was released. `LitSettings` has a headless case for where
`pack()` puts each value; the offsets are `static_assert`s. Touching `lit.glsl` rebuilds the four
shaders that include it and leaves `outline.frag` alone.

Six things came out differently:

* **The first picture was inside out, and the test caught it.** The cube came out black with red
  edges: the outline hull's near side covered the surface. The pipelines were right. The test
  camera was not: it was built with `glm::lookAt`, which looks along -z, while `api/type`'s
  cameras look along +z (ADR-0012), and that reverses which way round a face is on screen. The
  test now uses `type::camera::Isometric`, the camera a game draws through. Front faces are
  clockwise under it, as voxel's comment already said.
* **The outline's default thickness is under a pixel at the test's zoom**, and covered nothing.
  The case uses 0.1. On a mesh with a normal per face, the rim also breaks wherever two faces meet
  at the silhouette. That is the normal-extrusion technique, retcon's included, and
  RenderingPipeline.md now says so.
* **`Lit::scene()` is called while the frame is built, before the ring begins it.** It waits for
  its slot the way `Quad::submit` does, so called after `begin()` it waits on a fence that
  nothing will signal. The first run timed out on exactly that.
* **There is no lighting-model parameter.** The draft said one band and no outline is smooth
  shading. It is not: one band is flat colour. A smooth or physically based look is a different
  fragment shader, which ADR-0067's replaceable shaders already allow for. A parameter with one
  value would only have pretended to leave that room.
* **`pipeline::spirv(path)` was not added.** Nothing here loads a shader from disk, and retcon
  has its own `loadSpirv`. ADR-0067 says what a consumer hands over (words), not how it reads
  them.
* **The lit shaders draw through `camera.viewProjection`**, which the tree writes and retcon's
  never read. retcon's comment says its shaders compute `proj * view` on the device because
  switching changes the last bits of some vertices. That is a difference retcon's capture may
  show on adoption, beside the albedo's decoding, and step 11's handoff names it.

### Step 8 — A shadow map

**The shape.**

* **The arithmetic is headless.** `realtime::shadow::light(dir, centre, radius)` is retcon's
  `directionalLightMatrix`, and `realtime::shadow::fit(registry, alsoCover, margin)` is its
  `fitShadowFrustum`. The fit reads `Transform::position`, which is unchanged by milestone 3's
  record.
* **The pass is a `Pass`.** It names a step 5 depth-only target and a step 4 bias, and draws a
  `renderer::Lit` shadow pipeline. `meshes()` submits a shadow item for an entity with
  `castsShadow`.
* **The map reaches the lit pass at set 2, binding 1.** It goes through the target's sampler,
  in `DEPTH_READ_ONLY_OPTIMAL`, which ADR-0044 (removed)
  already leaves it in.
* **The recorder's existing rule orders it**: the shadow pass writes the target and the lit pass
  reads it. Until step 9 that order is the caller's, using `passBefore`.

**retcon's two known issues.** One map shared by both frames in flight is closed by step 9's
double-buffered target, so this step builds a single one and step 9 changes one argument. The
radius that has to grow by hand stays a known issue. Cascades are after the move, as the
roadmap says.

**Tests.** Headless:

* `light()` puts `centre` at the middle of clip space, depth one half;
* `light()` maps a point `radius` along the light's right axis to x = 1;
* `light()` takes the Z-up branch when the light points straight down;
* `fit()` gives the centroid and farthest-plus-margin of three casters, ignores one with
  `castsShadow` false, and changes nothing when there are none.

On the device: step 5's depth case, drawn through the shadow pipeline with the light's matrix,
asserts each quad's centre depth. Constant-depth planes stay exact under an orthographic
matrix.

**State: done.** All 25 suites pass. cpplint is clean, and so is `out/build/verify`. The whole
device suite, 34 cases, is silent with `VK_LAYER_VALIDATE_SYNC=1`, and the depths are exact on
the Radeon. There are six headless cases and two device cases:

* the three `light()` cases drafted above, and the `fit()` case, split in two;
* **a light is drawn through the matrix a `type::camera::Camera` builds** at the same eye and
  box, which is what lets the shadow pipeline cull as the cel one does;
* the depth case, with a third entity that casts nothing standing nearer the light over one
  quad. With `casters()` walking it, that quad's depth reads a half rather than three quarters;
* **a cube's shadow on the ground**, drawn twice, with the shadow's strength at zero and at one.
  Between the two, some white ground goes grey and no other pixel changes. With the bias and the
  normal offset taken out, 240 pixels of the cube change as well, which is acne, and the ground
  darkens over ten times as much. The picture goes to `data_out/lit_shadow.png`.

Four things came out differently:

* **retcon's matrix is not used as it is.** It builds the view with `glm::lookAt`, whose right is
  mirrored from this tree's cameras (ADR-0012, and step 7's inside-out cube). Under it the
  shadow pass would cull the faces the cel pass draws. `light()` builds the view as
  `type::camera::Profile::lookat` does, with retcon's eye, box, depth range and overhead case.
* **The shadow walk is a function of its own, `casters()`**, rather than `meshes()` submitting
  into two passes. It is the same walk with a different filter and pipeline, and a caller names
  each pass once.
* **`fit()` returns its bounds, or nothing when nothing casts**, where retcon's writes the
  renderer's members and leaves them alone. The caller keeps what it had in either case.
* **`Lit` takes the shadow map's depth format as a constructor argument**, beside the cel pass's.
  A shadow target's depth is sampled and need not be the format the scene's depth is, and a
  pipeline is compiled against one. `VK_FORMAT_UNDEFINED` builds no shadow pipeline.

Both passes bind one scene set, so in the shadow pass its binding 1 names the image being drawn
into. The shadow pipeline reads nothing at binding 1, and validation is silent about it.

### Step 9 — Targets a chain can use

**ADR-0068: a target may be one image per frame in flight, a pipeline is checked against what
it draws into, and a pass is placed by what it reads.** It amends
[ADR-0031](../../adr/0031-rendering-passes-draw-into-offscreen-targets.md). These are the roadmap's three
gaps:

* **Double-buffered.** `RenderTarget` takes a count, one or the ring's frames in flight. `Pass`
  draws into the current frame's image, and `previous()` names the other one for a pass that
  reads its own last frame. It is a count rather than a second class, because the recorder's
  writer-and-reader scan works the same per image.
* **The format check.** `Recorder` compares a pipeline's colour and depth formats with the
  target's when it binds, and throws on a mismatch. Under dynamic rendering a mismatch is a
  wrong picture and no validation error, so the recorder is the only place that can say so.
* **Placed by what it reads.** `Pass::reads(target)` declares a dependency. `Frame` orders its
  passes so that every writer of a target precedes every pass that reads it, keeping insertion
  order otherwise. A cycle is an error. `passBefore` is deleted, and `Engine3D` creating its
  colour pass in its constructor stops mattering. Nothing in the tree calls `passBefore`, so
  its removal breaks no app here.

**Tests.** Headless:

* the frame's ordering moves a reader after a writer added later;
* the ordering keeps the order of independent passes;
* the ordering throws on a cycle.

On the device:

* a two-image target alternates across frames;
* a pipeline built for `R8G8B8A8_UNORM` drawn into a `B8G8R8A8` target throws.

**State: done; ADR-0068 is accepted**, and ADR-0031's header says it is amended. All 25
suites pass, cpplint is clean, and so is `out/build/verify`. The whole device suite, 37 cases,
is silent with `VK_LAYER_VALIDATE_SYNC=1`. pong, voxel, tetris and the editor each ran, closed
through their own window, and logged no warning. The cases are the ones drafted, and each fails
with its part taken out:

* **headless:** a reader moves after a writer made later; independent passes keep their order;
  a chain of three is recorded in the order it reads; reading your own target orders nothing;
  a cycle throws. Three more check formats: a colour mismatch throws, an unstated format is not
  compared, and depth is compared only in a pass that tests it;
* **on the device:** a two-image target reads the frame before, three frames running, including
  the first frame, when no frame came before; a count other than one or the frames in flight
  throws; and the format mismatch throws. The lit scene's shadow case now makes the lit pass
  first and names the map in `reads()`.

With the frame recorded in creation order, the shadow case reads its map before it is written,
and validation reports it. With the target's images not made ready, the first frame samples an
image in `UNDEFINED`. With the format check taken out, nothing throws, and the validation layer
reports the mismatch itself, as `VUID-vkCmdDraw-dynamicRenderingUnusedAttachments-08910`.

Five things came out differently:

* **A format mismatch is a validation error after all.** The draft said a mismatch was a wrong
  picture and no error. That is only true with `dynamicRenderingUnusedAttachments` enabled,
  which this tree does not enable. The check stays, because it runs with the layers off and
  names the pass, and ADR-0068 says why.
* **Passes into one target keep their creation order whatever the reads move.** The draft said
  "insertion order otherwise", and a stable sort by reads alone moved an overlay drawn onto the
  window in front of the colour pass, which then cleared it. Each target, the window included,
  is now a chain in creation order, and that is one of the headless cases.
* **A target of more than one image starts cleared and readable.** Reading `previous()` on the
  first frame would otherwise sample an image in `UNDEFINED`. It is a clear by rendering, in one
  submission when the target is made or resized, since attachment usage is all the images have.
* **A slot is registered by number**: `Quad::texture(target, slot)`, with `current()` and
  `previous()` on the target. A registration names one image, so a reader of a two-image target
  holds a handle per slot.
* **The ordering is testable without a device.** `Frame::order` takes identities rather than
  targets, and `ordered()` hands it the passes'.

### Step 10 — The chain after the scene

**The shape.**

* **`renderer::FullScreen`** draws one triangle with a fragment shader the caller hands in, at
  set 1 (the source) and set 0 (the camera). It is a pipeline and a walk of one item. The LUT
  grade is one use, and the post passes the roadmap leaves for later are others.
* **`realtime::Grade`** is retcon's `LutPipeline` over it. It holds a 16³ 3D `Image` from a
  256×16 strip, or the identity when no path is given, and samples the scene with a nearest
  sampler.
* **The grade works in linear.** The scene target is `_SRGB` by ADR-0066 and decodes on sample.
  The LUT is indexed in linear, which is retcon's handoff gotcha 12, kept.

**Tests**, by [ADR-0054](../../adr/0054-testing-golden-images-hold-only-spec-exact-output.md):

* **A full-screen pass with a copying shader is the identity.** Its source is `quad.png`'s
  target, at one texel per pixel and sampled nearest, and its output equals `quad.png`
  byte-for-byte. That is a new case and no new reference.
* **The identity LUT.** Interpolating an identity linearly is the identity in arithmetic. But
  filtering precision is the implementation's, so the case is pinned only if it holds exactly
  on the Radeon *and* on lavapipe. Otherwise it asserts a difference of at most one per channel
  and holds no reference. There is one GPU on the authoring machine and no local lavapipe, so
  the second implementation answers only in CI, and this case's reference, if any, is blessed alone and
  goes through a pull request before anything else depends on it.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify`. The whole
device suite, 40 cases, is silent with `VK_LAYER_VALIDATE_SYNC=1`. Three headless cases and
three device cases:

* **headless:** the identity table's entries are their own positions; a strip baked from the
  identity reads back as the identity, in RGB and RGBA; and a strip of the wrong shape, or none,
  is no table;
* **a copying pass is the identity.** The quad case's picture, drawn into a target and copied
  through `FullScreen`, matches the committed `quad` reference byte for byte. No new reference;
* **the identity grade** leaves a sweep of every channel value within one step;
* **an inverting strip** gives the complement of every colour within one step, which pins the
  strip's slices, rows and columns end to end.

Both grade cases came out exact on the Radeon, a difference of zero. They still assert a step at
most and hold no reference, because lavapipe answers only in CI and the plan pins a filtered
picture only when both implementations agree. **No reference was blessed in this step.** If CI
shows lavapipe exact too, the tolerance can come down to zero.

Each case fails with its part taken out:

* with the copy pass's `reads()` removed, it samples the scene before the scene is drawn.
  Validation reports it, and the reference differs by 255;
* with red and green swapped in the strip reader, the headless cases fail, and the inverting
  grade misses by 253;
* with the shader's scale into texel centres removed, the identity grade moves a channel by 8.

Four things came out differently:

* **`FullScreen`'s set 1 is the caller's count of images, not one.** The grade reads the scene
  and its table, so a source is however many combined image samplers the spec declares, bound as
  one material.
* **`Grade` takes an image rather than a path.** Loading a strip is a game's, as retcon's
  `--lut` is, and taking the decoded image leaves the api with no asset path to resolve. The
  table's reordering, `Grade::table()`, is static, so the headless suite tests it.
* **The copy case reads with `texelFetch`.** Reading by position is exact on any implementation,
  where a nearest sampler at texel centres only should be, so the identity case asserts the
  reference with no tolerance.
* **The grade cases draw through `UNORM` targets.** The arithmetic is the same whether or not a
  target decodes. An sRGB chain only adds an encode at each end, which step 11's scene goes
  through.

### Step 11 — retcon's scene, reproduced here, and the handoff

**The roadmap's acceptance test cannot run here**: it is retcon's capture of retcon's art,
built against this tier with retcon's copy deleted. What stands in for it is the same scene as
a device case, `lit_scene`, built from the survey:

* a ground plane that casts no shadow, and props from the asset suite's fixtures standing in
  for retcon's, at set yaws through `aboutY`;
* `LitSettings` at retcon's defaults, `urban-ruin`'s role played by the identity LUT;
* an orthographic camera at 45° framing the props' centroid, at 1280×720;
* the frame order: shadow pass, then scene pass, then grade into a swapchain-format target.

It asserts validation silence, with `VK_LAYER_VALIDATE_SYNC=1` when run locally, and it always
writes `data_out/lit_scene.png` for a person to look at. It holds no reference: lighting and
PCF are what ADR-0054 excludes. **What it adds over a screenshot is the second driver**,
because CI runs it on lavapipe, so the tier is silent on two implementations before anyone
adopts it.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify`. The whole
device suite, 41 cases, is silent with `VK_LAYER_VALIDATE_SYNC=1`. The case is
`retcons_scene_is_drawn_and_silent`, in the lit suite. It draws a ground slab that casts nothing,
two upright figures, and ten props in a ring at their own yaws, at `LitSettings`' defaults with
retcon's three metre fit margin. The camera is the tree's isometric one at 45 degrees, and the
frame is 1280×720. The shadow map is 2048², the scene target is `B8G8R8A8_SRGB`, and the grade
draws the identity table into a second target of the same format.

The passes are made grade first, then lit, then shadow, and each names what it reads. The case
asserts that the frame records them shadow, lit, grade; that the frame is silent; and that over a
quarter of the picture is not the clear colour. The picture goes to `data_out/lit_scene.png`. The
cube case that had that name now writes `lit_cube.png`.

Three things came out differently:

* **There are no warm-up frames.** retcon draws thirty before its capture so that its scene has
  settled. Nothing here moves, so one frame is the scene.
* **The props are the asset suite's two models and a crate**, and the models are open,
  single-sided triangles. A triangle turned from the camera has its surface culled, and the back
  of its outline hull is what shows: a black triangle. The ground slab's far edge is outlined too.
  Both are the normal-extrusion outline on such geometry, which RenderingPipeline.md now says.
  retcon's art is closed meshes.
* **CI runs the case on lavapipe** as part of the device suite, which is what the plan wanted from
  it. That run is the first time the tier draws on a second implementation, and it happens on the
  pull request this work goes up in.

#### The handoff, for retcon when it adopts

Written here for retcon to read, not sent to it. Each of retcon's classes has a counterpart here:

| retcon | here |
|---|---|
| `GpuImage` | `vulkan::memory::Image` |
| `Sampler` | `vulkan::pipeline::Sampler` |
| `DescriptorAllocator` | a `vulkan::pipeline::DescriptorPool` per layout: `FrameUniforms`' camera, `Quad`'s material, `Lit`'s scene |
| `MeshRegistry` | `realtime::MeshRegistry`, with `MeshHandle`s that are released ([ADR-0065](../../adr/0065-meshes-shared-registry-keyed-by-path.md)) |
| `MeshPipeline` | `vulkan::renderer::Lit` |
| `ShadowPass` | a `Pass` on a `RenderTarget` with sampled depth and no colour, drawn by `realtime::casters()`, with `shadow::light()` and `shadow::fit()` |
| `OffscreenPass` | a `RenderTarget` |
| `LutPipeline` | `realtime::Grade`, over `vulkan::renderer::FullScreen` |
| `SceneRenderSettings` | `realtime::LitSettings` |
| `SceneRenderer` | a `Frame` of three passes, placed by `Pass::reads()` ([ADR-0068](../../adr/0068-rendering-order-passes-by-what-they-read.md)) |
| `MeshRenderer` | `realtime::component::Mesh`, beside milestone 3's `ecs::component::Transform` ([ADR-0063](../../adr/0063-ecs-draw-from-a-transform-plus-a-component-per-kind.md)) |

What adopting involves:

* **Adopting the tier means adopting `Frame` and the recorder**, which is what retcon's D8
  declined. [ADR-0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md) is the answer
  to it: set 2 and a depth bias are the pass's.
* **The transform turns by a quaternion, not a yaw.** `aboutY()` makes one from a yaw.
* **Shaders.** retcon keeps loading its shaders from its directory with its own `loadSpirv`,
  and hands the words to `Lit::Shaders`
  (ADR-0067, removed). They declare the blocks in
  `shaders/lit/lit.glsl`, which are retcon's own blocks, member for member. Its `lut.frag` can be
  handed to a `FullScreen` as it is, once its samplers say `set = 1`, because the scene at binding
  0 and the table at binding 1 are the layout `Grade` uses.
* **Calls that differ.** `Lit::scene()` is called while the frame is built, before the ring
  begins it. `Lit` takes the shadow map's depth format as well as the scene's. `Grade` takes the
  decoded strip rather than a path. `shadow::fit()` returns its bounds, or nothing, rather than
  setting them. The shadow map may be a target of one image per frame in flight, which closes
  the simplification retcon's ADR-0021 records.

**retcon's capture is expected to move, for two reasons and no others:**

* **The albedo is decoded before it is lit**
  ([ADR-0066](../../adr/0066-lighting-light-in-linear-draw-to-srgb.md)). retcon uploads it `UNORM`, so its
  capture has a display-space texture lit as though it were linear. This is the larger move, on
  every textured surface.
* **The vertex stage draws through `camera.viewProjection`**, which the host multiplies, where
  retcon's shaders multiply `proj * view` on the device. Its own comment says that switching
  changes the last bits of some vertices, so an edge pixel or two may move.

The shadow's matrix is built with this tree's camera basis, which is mirrored left to right
from `glm::lookAt`'s. The map is drawn and read through the same matrix, so that changes no
picture. **Any other difference is a defect in this tier**, and adopting in the order of these
steps says which step caused it.

---

## Sequence

**Steps 1, 2 and 3 first.** The record is written before its code, and the two refactors
remove code against a suite that already pins their output, so they go in while the tree's
pictures are the only thing they can break.

**Steps 4 and 5 next.** Both are small, and both are what the shadow map is written against.
Step 5's depth readback is the first time the suite asserts depth at all.

**Step 6, then step 7.** The registry gives the walk something to name. Step 7 carries two
records and accepts the first, so it is the plan's midpoint. If the frame model cannot take a
lit pass, that shows here, and the plan stops to rethink ADR-0064 rather than pressing on to
the shadow.

**Step 8, then step 9, then step 10.** The shadow map can be written on today's single-buffered
targets and `passBefore`, so it does not wait for step 9. The chain cannot.

**Step 11 last**, though its case is started in step 7 and grows with each step after.

## Verification

Per [sdlc.md](../../sdlc.md), every step that changes code: `ninja -C out/build/x64-Debug`,
`ctest`, cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all
of them, so every finding is the step's. Steps 2, 3 and 9 to 11 change what the device suite
draws through, so each also runs once with `VK_LAYER_VALIDATE_SYNC=1`, because they move
barriers.

**What can be pinned is pinned**:

* the four existing pictures, unchanged through steps 2 and 3;
* the scene set's texels in step 4;
* the depths of steps 5 and 8;
* the identity pass of step 10.

**What cannot be pinned is silence on two drivers and a picture a person looks at**: the lit
scene, the outline, the shadow's PCF and the grade. A reference blessed here is checked against
lavapipe only in CI, on a pull request, so the first new reference goes through one alone before
any other is blessed. Step 4 needed none, so that is step 10's identity LUT if it holds exactly. Nothing here is verified in another repository.

## What this does not do

* **It does not move `DebugLines` or `GridOverlay`.** `renderer::Line` is the same pipeline.
* **It does not instance.** That is [milestone 5](../../roadmap/completed/m5-SkeletalAnimation.md#4-instancing)'s.
* **It does not merge draws, add a second depth buffer, or put the 2D pass on set 0.** That is
  the rest of [RenderingPipeline.md](../../api/Rendering.md#what-is-not-built-yet)'s list.
* **It does not add cascades or a self-growing shadow radius.** Step 8 says why.
* **It does not light voxel.** voxel's vertex layout and its per-vertex lighting are its own.
  Step 3 is what it takes.
* **It does not add a second lighting model.** A smooth or physically based look is a different
  fragment shader, which ADR-0067's replaceable shaders allow for. Step 7 says why there is no
  parameter for one.

## When a step lands

Update the state in the table above.

* **Step 1** adds the ADR index row for 0064 as `proposed`, and the plans index says this plan
  is open.
* **Step 2** rewrites [RenderingPipeline.md](../../api/Rendering.md)'s offscreen-targets
  section where it describes `Texture::owned`. It also deletes "a depth target that can be
  read" from what is not built yet, which ADR-0044 built and the list never lost.
* **Step 3** deletes voxel's pool from the binding section's account of voxel.
* **Step 4** adds set 2 to the binding table.
* **Step 6** adds ADR-0065 and amends ADR-0010's header. It replaces "how this meets the ECS"'s
  last paragraph with the mesh component and its walk, and adds `MeshHandle` to the
  resource-handles section.
* **Step 7** adds ADRs 0066 and 0067 and accepts 0064. It adds a lit-pass section to
  RenderingPipeline.md and a line to its colour section, and adds the include to
  [Build.md](../../contributing/Build.md#shaders).
* **Step 9** adds ADR-0068, amends ADR-0031's header, and replaces the offscreen section's
  three consequences with what is now true.
* **Step 11** adds the device suite's new cases to [Testing.md](../../contributing/Testing.md).
* **When the plan closes**, [m4](../../roadmap/completed/m4-LitScene.md) moves to `roadmap/completed/` and
  points here as done. The roadmap's table row says so, and this file moves to
  [completed/](./).

## Outcome

Drafted and closed on 2026-10-03. It took up
[milestone 4](../../roadmap/completed/m4-LitScene.md) of
[the game engine roadmap](../../roadmap/completed/GameEngine.md). It rebuilt retcon's lit
rendering inside the frame model the rest of the api draws through, rather than copying
retcon's hand-recorded passes. It delivered:

- a scene set at set 2 and a depth bias in the recorder
  ([ADR-0064](../../adr/0064-lighting-lit-passes-use-the-shared-recorder.md)), which removed
  retcon's reason for refusing the frame model;
- images and samplers as classes;
- a model reaching the device through a registry
  ([ADR-0065](../../adr/0065-meshes-shared-registry-keyed-by-path.md));
- lighting in linear colour
  ([ADR-0066](../../adr/0066-lighting-light-in-linear-draw-to-srgb.md)), with replaceable
  shaders (ADR-0067, removed);
- a frame that places its passes by what they read
  ([ADR-0068](../../adr/0068-rendering-order-passes-by-what-they-read.md));
- retcon's look-dev scene as a device test case, and a handoff note for retcon to read when it
  adopts the work.

Three things came out differently from the plan:

- **A format mismatch is a validation error after all.** The recorder's own check is for runs
  without the validation layers, and for naming the pass.
- **Passes into one target keep their creation order, whatever the reads move.** A sort by reads
  alone broke this by putting an overlay under the colour pass.
- **retcon's light matrix could not be copied unchanged.** It is built with `glm::lookAt`, which
  is mirrored relative to this tree's cameras. A shadow pass culled with the cel pass's winding
  would have drawn the back faces.
