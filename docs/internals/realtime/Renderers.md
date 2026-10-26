# The renderers

The classes in `vulkan/renderer` that turn a canvas, lines, world quads or lit meshes into draw
items.

## The renderers

Each renderer in `vulkan/renderer` takes the context's device, pipeline cache, resources, ring
and frame uniforms, plus the colour and depth formats it draws into. Each compiles a variant per
depth state.

| Renderer | Pipelines | Depth variant | Notes |
|---|---|---|---|
| `Quad` | 2 | neither tests nor writes | one vertex format (position, uv, colour); untextured quads sample white; `text` push flag selects the distance-field branch; into an `_SRGB` format, `quad.frag` is compiled with `LINEARISE` and decodes before it writes |
| `Line` | 2 | tests and writes, unless the pass turns writing off | `LINE_LIST`, no index buffer; positions in world space through set 0 |
| `World` | 4 (alpha and additive, each with and without depth) | tests, does not write unless the pass turns writing on | additive: colour added by source alpha, destination alpha kept |
| `Lit` | cel, outline, shadow, and a skinned version of each | tests and writes | front face given at construction, clockwise by default; back faces culled (outline culls front); shadow pipeline has depth bias and a push constant, the cascade; built only when given a shadow format |
| `FullScreen` | 1 | neither | one triangle from three vertices and no vertex buffer; caller's fragment SPIR-V |

`DeviceContext` builds `Quad`, `Line` and `World` on first request against its colour format and
`depthFormat()`. `Quad` is lazy too, because `Context3D` has no format until the
swapchain exists.

**Quad text.** A batch carries a text flag, and `Canvas` never merges across it. The fragment
shader thresholds a text batch's sampled distance at 0.5 with `smoothstep`, using `fwidth` for a
one-pixel edge at any scale. Other batches return `colour * texel`.

**Lit front faces are clockwise by default.** A model is wound counter-clockwise seen from
outside, and the cameras in `api/type` flip y into Vulkan clip space. A camera on
`Profile::Hand::DirectionCrossUp` also mirrors x, which reverses the winding again, so `Lit`
takes the front face as its last constructor argument. All six pipelines cull by it.
`lighting::shadowLight()` builds its view the way `type::camera::Camera` does in the hand it is
given, so faces wind the same under the light and the shadow pipeline culls as the cel one does.

**`Lit::scene()`** writes the frame's slot: a uniform buffer (`SceneUniforms`, std140, matching
the `Scene` block in `lit.glsl`), the shadow map binding, and the palette storage buffer, and
empties the slot's objects. It waits on `Ring::waitFrame()` first, so it is called while the
frame is built and before the ring begins it. It also makes that slot the one `Lit::object()`
adds to, so the walks run after it. The palette and object buffers are one per frame in flight,
grow by doubling, and retire an outgrown buffer through the ring. An object buffer that grows
copies the objects already added, because the frame's draws name them by index. A static
object has `firstJoint = 0`, and its pipeline ignores it.

The shadow map binding holds a 2D array view, a layer per cascade. With no map, it holds a map of
one layer and one texel that `Lit` owns, cleared to the far plane at construction, so the
binding always has a view of the kind the shader reads. Each cascade is drawn by a pass into its
own layer, and the shadow pipelines read the cascade's matrix by the index a draw pushes.

**Skinned vertex.** `MeshRegistry::SkinnedVertex` is `type::Model::Vertex` (32 bytes) followed by
`type::Model::Influence`: four joints as unsigned shorts and four float weights, 56 bytes in all.

**`FullScreen` sources** are allocated from its own `DescriptorPool`, one binding per source.
A depth image is bound in `DEPTH_READ_ONLY_OPTIMAL`. A source must be released before the
`FullScreen` goes, since the set belongs to its pool. `Grade` holds two `FullScreen`s, one for a
scene that holds display values and one for an `_SRGB` scene, which is read decoded. Both are built
for the grade's target, so they decode the result before an `_SRGB` target stores it. A source
belongs to the one that matches its scene's format. `Grade` keeps a map from the handle a caller
holds to the current material; `replace()` creates a new table, rebinds every source to new
materials, and retires the old table and materials through the ring. `replace()` uploads through
`Uploader::submit`, which does not wait for the upload. `Grade`'s
destructor releases its table and every source still in the map, before its `FullScreen` goes.
`source()` releases the scene's registration if binding the material throws.

Background: [ADR-0005](../../adr/0005-2d-one-batched-quad-pipeline.md),
[ADR-0011](../../adr/0011-rendering-lines-as-a-world-space-primitive.md),
[ADR-0036](../../adr/0036-text-sdf-glyphs-through-the-quad-shader.md),
[ADR-0042](../../adr/0042-rendering-world-space-sprites.md),
[ADR-0071](../../adr/0071-skinning-joint-matrices-in-one-storage-buffer.md)
