# ADR-0030: Loaded Geometry — A Model Is One Interleaved Array With A Material That Names Its Texture

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Nothing in the tree loads geometry from a file. `voxel` builds its terrain procedurally, the
editor models with `brep::BRep`, and `asset::Type` knows png, jpeg, tga, wav, json and two
kinds of font. An application that wants to draw something an artist made has nowhere to put it.

Two types already answer to the word mesh and neither fits. `brep::BRep` is half-edge
topology — built to be *edited*, with `INVALID_ID` frozen into the project file format
([ADR-0018](0018-a-project-is-json-and-stores-topology-verbatim.md)) — and walking it per frame
to produce a vertex buffer is the wrong shape for something that never changes after load.
`render::realtime::vulkan::Mesh` is two device buffers and is deliberately layout-blind: it
takes bytes and a count because the stride belongs to the pipeline. Between the file and the
device there is nothing.

The second question is the material. glTF's base colour texture may be a URI beside the file or
an image embedded in the container, and `api/image` reads files only — `Reader::read` takes a
filename, and neither libpng nor libjpeg is pointed at a buffer anywhere in the tree. So a
loader that returns pixels either grows a second image decoder beside `api/image` or cannot
load half the files it is given.

## Decision

Loaded geometry is **`v3d::type::Model`**: one interleaved vertex array, one index run, and one
material. Every mesh and primitive in a file is merged into it, so a model is one draw, and a
file whose parts need different surfaces is several models.

The vertex layout — position, normal, uv — is **a contract between the loader and whatever
pipeline an app writes**, not something the device enforces. `vulkan::Mesh` stays layout-blind.

A material carries a colour and **the name of its texture, not its pixels**. Resolving that name
is the application's, through the asset manager — the shape
[ADR-0020](0020-a-theme-is-data-and-the-app-resolves-its-images.md) settles for themes. An image
a file *embeds* rather than names is reported and left empty, because reading one needs an image
reader that takes a buffer and that is `api/image`'s change to make, not a loader's to work
around.

## Alternatives Considered

### Alternative 1: A value type in `api/type`, materials naming their textures — **chosen**
- **Pros**: `api/type` links only glm, so a model reaches the offline renderers
  ([ADR-0024](0024-api-type-serves-both-renderers.md)) as easily as the realtime one, and the
  loader's suite runs in CI with no device. One image pipeline in the tree, not two. A texture
  named once is loaded once, by the thing that already caches.
- **Cons**: An app that wants to draw a textured model writes the resolve step itself. An
  embedded texture does not work at all until the image readers grow a memory source.
- **Why not**: n/a — chosen.

### Alternative 2: Load into `brep::BRep`
- **Pros**: One mesh type. The editor could open a glTF.
- **Cons**: Half-edge topology is for editing, and building it costs adjacency work no renderer
  needs. `BRep` carries no uv and no material, and its identifier space is part of a file
  format. Every frame would rebuild a vertex buffer from it.
- **Why not**: The two have different jobs. Making one serve both would constrain the editor's
  format to a renderer's needs.

### Alternative 3: The loader decodes the pixels and the material carries an `image::Image`
- **Pros**: One call gets a drawable model. Embedded and external images work the same way.
- **Cons**: Either `api/image` grows a memory source in the same change, or the loader carries
  its own decoder — and stb_image beside libpng and libjpeg is two answers to one question. A
  model also loads its texture again every time it is loaded, where the asset manager would have
  cached it.
- **Why not**: The decoding half is a real gap in `api/image` and deserves its own change with
  its own tests. Bundling it here would have hidden it.

### Alternative 4: Keep each glTF primitive as its own draw
- **Pros**: A file with several materials survives intact. No index rebasing.
- **Cons**: A model becomes a list of draws and a list of materials, and every consumer walks
  it. The common case — one object, one surface — pays for the general one.
- **Why not**: Deferred rather than rejected. Splitting a file into several `Model`s is the
  natural extension and needs no change to this type.

## Consequences

### Positive
- Something in the tree finally loads geometry, and its suite asserts the merge, the rebasing
  and the synthesised index run against a committed `.glb` whose generator sits beside it.
- `api/asset`, `api/type` and `api/image` each keep their own job, and there is still one image
  decoder in the tree.
- Nothing about a `Model` names a device, so it is readable by the offline renderers.

### Negative
- An embedded texture is not read. The loader says so rather than dropping it, but a `.glb`
  exported with textures packed in — which is the usual export — arrives untextured.
- Only the first material in a file survives the merge.
- The vertex layout is fixed at three attributes. A model needing tangents or a second uv set
  changes the struct, and every pipeline written against it.

### Risks
- The layout being a contract rather than a declaration means a mismatch between a loader and a
  pipeline is a wrong picture rather than a build failure. The mitigation is that there is one
  layout and it is in one header; a second one would want the declaration to move into the
  pipeline builder.
