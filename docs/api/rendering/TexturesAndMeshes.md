# Textures and meshes

How images and models get onto the GPU, how an app refers to them, and how it lets them go.

## Textures

`Textures` (from `renderer_->textures()` or `context->textures()`) uploads images and makes
them drawable.

```cpp
boost::shared_ptr<Textures> textures = renderer_->textures();
TextureHandle sprite = textures->texture(image);                     // a v3d::image::Image
TextureHandle atlas = textures->texture(pixels, width, height, 1);    // raw bytes; 1 channel for a glyph atlas
uint32_t slot = textures->slot(sprite);                               // for a DrawItem of your own
textures->release(sprite);                                            // see Releasing resources
```

- A texture is uploaded as display-space `UNORM` unless you pass
  `TextureFactory::Encoding::Srgb` (see [Colour](ColourAndPost.md#colour)).
- A single-channel image is read as a coverage mask: white, with the channel as alpha.
- **A texture is read linear and clamped to the edge** unless you pass a
  `vulkan::pipeline::Sampler::Spec` as the last argument. The default suits an atlas and a
  sprite. A surface meant to tile names a repeating address mode:

  ```cpp
  vulkan::pipeline::Sampler::Spec tiling;
  tiling.address = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  TextureHandle floor = textures->texture(image, TextureFactory::Encoding::Srgb, tiling);
  ```

  One sampler is made for each distinct spec and shared by every texture that names it. A
  texture is always read through its own sampler.
- `white()` is the 1×1 white texture. It is never released.
- An upload returns without waiting for the copy, and the texture is drawn from the next frame
  on.
- **Every texture is a slot in one array**, bound at set 1, which holds a few thousand textures
  or as many as the device allows, whichever is fewer. `capacity()` says how many. Registering
  one more than that throws.

A canvas names textures by `TextureHandle`, and the renderers turn each into its slot when they
submit. A `DrawItem` of your own names `material()`, the one material that binds the array and
is the same for every texture, and hands its shader `slot(texture)`, in a push constant or in
data it reads per object. **Read the slot each frame rather than keeping it**, because a texture
filled or replaced later moves to another slot. A shader declares the array as
`layout(set = 1, binding = 0) uniform sampler2D textures[]` and reads `textures[slot]`, with
`GL_EXT_nonuniform_qualifier` enabled. An index that may differ between the fragments of one
draw, such as one read per instance, is wrapped in `nonuniformEXT()`. An unset or released
handle's slot is the white texture's.

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
- **Reading and decoding are separate from uploading.** `decodeModel(assets, path)` reads a
  glTF file and decodes the albedo each material uses, and makes no device call. It returns a
  `DecodedModel`, which `add(decoded)` uploads without reading any file. `load(path)` is the
  two together, and skips decoding an albedo an entry already uploaded.
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

### Requesting a mesh

A registry made with an `asset::Worker` can request a path instead of loading it. The model is
decoded on the worker thread and uploaded by a later `poll()`, so a load costs the frame only
its upload.

```cpp
MeshRegistry meshes(logger(), renderer_->context(), assets(), worker_);
MeshHandle tree = meshes.request("models/tree.glb");  // resolves to nothing for now

void MyApp::tick(unsigned int delta) {
    worker_->poll();  // the decoded models arrive
    meshes.poll();    // and are uploaded, up to a byte budget
}
```

- **A requested handle resolves to nothing until a poll uploads it.** A draw skips such a
  handle, so a model still loading is not drawn. Asking for the path again returns the same
  handle.
- **`poll(budget)` uploads in request order**, at least one model when any is ready, and stops
  once the vertex, index and albedo bytes reach the budget. The default is
  `MeshRegistry::DEFAULT_UPLOAD_BUDGET`, 4 MiB. The rest wait for the next poll.
- **`pending(handle)` is true while a request is decoding or waiting for a poll.** A handle that
  is not pending and resolves to nothing failed or was released.
- **A file that cannot be read or uploaded is reported in the log.** Its handle resolves to
  nothing from then on, and the path can be requested again.
- **Releasing a pending handle** drops what the worker decodes for it.
- **`load()` of a pending path** uploads it at once under the same handle.
- `count()` leaves out pending requests, and `pending()` counts them.
- A registry made without a worker throws `std::logic_error` from `request()`.

A texture has no request of its own. Decode the image in a worker job and register it with
`Textures::texture(image)` in the job's completion.

A registered mesh is drawn by the [lit pass](Lighting.md#the-lit-pass). Geometry an app builds
and draws itself (voxel's chunks, for instance) is a `vulkan::memory::Mesh` owned by the app. A
`DrawItem` refers to its buffers directly and is valid only while the mesh is alive.
`Mesh::describe(&item)` fills an item's geometry fields.

## Releasing resources

**Release a handle once nothing more will be queued with it. Items queued before the release are
still drawn by their frame.** The GPU object behind the handle lives on until no frame in flight
can use it. A handle holds a slot and a generation. A released slot is reused with a new
generation, so an old handle never comes to mean something else. A released texture handle
resolves to nothing at once. A released material handle goes on resolving until the frame being
recorded at the release is submitted, so items queued before the release draw with it. It
resolves to nothing after that.

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
GPU is idle. The app lifecycle is described in [engine/](../engine/README.md).
