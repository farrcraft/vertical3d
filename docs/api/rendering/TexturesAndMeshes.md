# Textures and meshes

How images and models get onto the GPU, how an app refers to them, and how it lets them go.

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
  `TextureFactory::Encoding::Srgb` (see [Colour](ColourAndPost.md#colour)).
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

A registered mesh is drawn by the [lit pass](Lighting.md#the-lit-pass). Geometry an app builds
and draws itself (voxel's chunks, for instance) is a `vulkan::memory::Mesh` owned by the app. A
`DrawItem` refers to its buffers directly and is valid only while the mesh is alive.
`Mesh::describe(&item)` fills an item's geometry fields.

## Releasing resources

**Release a handle once nothing more will be queued with it. Items queued before the release are
still drawn by their frame.** The GPU object behind the handle lives on until no frame in flight
can use it. A handle holds a slot and a generation. A released slot is reused with a new
generation, so an old handle never comes to mean something else; it resolves to nothing.

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
