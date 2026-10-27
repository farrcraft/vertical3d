# Models

This page is for someone writing an app against the api. It covers `model::Model`, the geometry,
materials, skeleton and clips a model file gives, and the glTF loader that builds one. Drawing a
model is in [rendering/](rendering/README.md). Terms are defined in the
[glossary](README.md#glossary).

`v3dlib_model` holds `model::Model`, and depends on `animation`. The loader is in
`v3dlib_asset_media`.

- [Three things called a mesh](#three-things-called-a-mesh)
- [model::Model](#modelmodel)
- [Skeleton](#skeleton)
- [Loading glTF](#loading-gltf)

## Three things called a mesh

| Type | What it is |
|---|---|
| `brep::BRep` | Half-edge topology the editor models with. See [editor/](../editor/README.md). |
| `model::Model` | Interleaved vertices, indices, materials and parts: what a model file loads into. |
| The renderer's mesh | Two device buffers, made from a `model::Model`. See [rendering/](rendering/README.md). |

## model::Model

`model::Model` ([api/model/Model.h](../../api/model/Model.h)) holds one vertex array and one index
list for a whole file, so a model is one upload.

- **`Vertex`** is a position, a normal and a uv, interleaved: 32 bytes. This layout is an
  agreement between the loader and whatever pipeline draws it. The device does not check it;
  a device mesh takes bytes and a count, and the stride belongs to the pipeline.
- **`Material`** is a base colour and `baseColourTexture`, the name the file gave its image.
  The model names the image and does not hold its pixels. The app loads the image through the
  asset manager. An image embedded in the file arrives decoded on the asset instead; see
  [Loading glTF](#loading-gltf).
- **`Part`** is a range of the index list (`firstIndex`, `indexCount`) and a material index.
  Every index is in exactly one part, and a loaded model has at least one. Draw a model as one
  draw per part.
- **`skeleton()`, `influences()` and `clips()`** are empty for a static model. A skinned model
  has a `Skeleton`, one `Influence` per vertex in a separate array parallel to the vertices,
  and its animation clips.
- `vertexBytes()` is the size of the vertex array, for creating a device buffer. `empty()` is
  true when there is no geometry.

## Skeleton

`animation::Skeleton` ([api/animation/Skeleton.h](../../api/animation/Skeleton.h)) is the list
of joints a skinned model is bent by.

- **Every parent comes before its children.** A pose can therefore be made global in one pass
  from the front.
- Each `Joint` has a name, a parent index (or -1), a rest translation, rotation and scale local
  to its parent, and an `inverseBind` matrix. The inverse bind takes a vertex from model space
  into the joint's space as it stood when the model was bound.
- `root` is the transform above the root joints, such as an armature's scale. No clip animates
  it.
- An `Influence` names up to four joints and their weights, which sum to one.

## Loading glTF

The glTF loader ([api/asset/media/loader/Gltf.h](../../api/asset/media/loader/Gltf.h)) reads
`.gltf` and `.glb` files into an `asset::media::kind::Model`. Its `model()` is a `model::Model`,
described [above](#modelmodel). It merges the whole file into one model for drawing. To read a
file as a scene that keeps its nodes, cameras and lights apart, use
[interchange](Interchange.md) instead.

### What the loader builds

- **One model per file**, with one vertex array and one index list. The loader walks the file's
  scene from its root nodes and merges every mesh a node names, placed by the node's world
  matrix. Normals are turned by that matrix's inverse transpose.
- The scene is the file's default scene, or its first, or every root node when it has none.
- **A mesh named by two nodes is merged twice.** A file that reuses one mesh many times makes a
  larger upload than the file.
- **Primitives that share a material become one part**, in the order the loader reaches them.
  Parts are in the order their materials first appear. A primitive with no material uses a
  default one.
- Positions are required. Normals and uvs are read where a primitive has them, and are zero
  where it does not.
- A primitive with no indices gets a sequential run, so the model is always indexed.
- Triangle strips and fans become triangle lists, wound as glTF winds them. A primitive of
  points or lines is left out with a warning, because a model holds triangles.
- A file that does not read or parse gives no asset and a log line.
- **The file and every buffer it names are read through `asset::readFile`**, as any other asset
  is. A buffer's uri is resolved beside the file, with its percent-escapes decoded, so
  `external%20buffer.bin` is the file `external buffer.bin` next to the `.gltf`.
- **A file that fails validation gives no asset and a log line.** The loader validates before it
  reads an accessor, so an accessor that runs past its buffer view, or an index past the last
  vertex, rejects the whole file.

### Materials and textures

**Only the base colour is read**, from the metallic-roughness model.

- **A texture the file names arrives as a name**, in `Material::baseColourTexture`. The name is
  the file's uri with its percent-escapes decoded, so `my%20texture.png` arrives as
  `my texture.png`. It is relative to the model file. It is resolved through the asset
  manager and uploaded by whoever draws the model: the renderer's `MeshRegistry` does this for
  a model it loads. The model never holds the pixels of a named texture.
- **A texture the file embeds arrives decoded.** A `.glb` stores images in its own buffer, and a
  `.gltf` may inline one as a data uri, whose `data:image/png` or `data:image/jpeg` prefix
  states its type when the file gives no `mimeType`. Neither has a name to give, so the loader
  decodes it and `kind::Model::baseColourImage(material)` returns it. It is null for a named
  texture or no texture.
- An embedded image in a format glTF does not allow, one that does not decode, or one with
  neither a buffer view nor a uri, costs the model that texture, with a log line. The model
  still loads.

### Skins and animation

- **The first skin** any mesh in the scene is bound to becomes the model's skeleton. Its joints
  are reordered so that every parent comes before its children.
- **A skin with fewer inverse bind matrices than joints gives no asset** and a log line, and so
  does one whose inverse bind matrices are not 4x4 matrices. A skin with no inverse bind
  matrices binds each joint by the identity.
- Every vertex gets an influence. **Four influences per vertex are read**, and a second set of
  joints and weights is dropped.
- **A weight given to a joint the skin does not have is left out**, with a warning. The
  vertex's other weights are scaled to sum to one. A vertex left with no weight follows the
  first root.
- **A skinned mesh is placed by its joints, not by its node.**
- **An unskinned mesh in a skinned file follows the nearest joint above it rigidly**, or the
  first root joint if it is under none. It stands where its node puts it while that joint is
  at rest.
- **Every animation in the file becomes a clip**, keeping the channels that animate the
  skeleton's joints. A file with no skeleton has no clips.

### Drawing a model

A `model::Model` holds no device resources. To draw it, register it with the renderer's
`MeshRegistry`, which uploads it once per path. [rendering/](rendering/README.md) covers that.

Background: [ADR-0069](../adr/0069-models-material-parts-over-one-vertex-buffer.md)
