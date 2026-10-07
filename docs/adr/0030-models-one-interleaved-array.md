# ADR-0030: Models: one interleaved array

**Status**: superseded
**Date**: 2026-09-06
**Superseded by**: [ADR-0069](0069-models-material-parts-over-one-vertex-buffer.md)
**Documented in**: [api/Assets.md](../api/Assets.md)

## Context

An app that wants to draw geometry an artist made needs a type to load a file into before it is
uploaded. `brep::BRep` is half-edge topology built for editing, and its identifiers are part of
the editor's project format ([ADR-0018](0018-editor-projects-saved-as-json-with-exact-topology.md)).
The device mesh is two buffers and takes bytes and a count, because the vertex stride belongs to
the pipeline. A glTF material's texture may be a file beside the model or an image embedded in
it, and `api/image` reads only from files.

## Decision

Loaded geometry is `v3d::type::Model`: one interleaved vertex array, one index run and one
material, with every mesh in a file merged into it so that a model is one draw. The vertex layout
is an agreement between the loader and the app's pipeline, not something the device enforces. A
material names its texture rather than holding pixels, and the app resolves the name through the
asset manager.

## Alternatives

### Load into `brep::BRep`
- **For**: one mesh type in the tree, and the editor could open a glTF file.
- **Against**: building half-edge adjacency costs work no renderer needs. `BRep` carries no uv and
  no material, and a vertex buffer would be rebuilt from it every frame.
- **Rejected because**: editing and drawing are different jobs, and serving both would tie the
  editor's file format to a renderer's needs.

### The loader decodes the texture, and the material holds an image
- **For**: one call gives a drawable model, and embedded and external images work the same way.
- **Against**: either `api/image` grows a memory source in the same change, or the loader carries
  a second image decoder beside libpng and libjpeg.
- **Rejected because**: reading an image from memory is a gap in `api/image` that deserves its own
  change and tests.

### Keep each glTF primitive as its own draw
- **For**: a file with several materials loads intact.
- **Against**: a model becomes a list of draws and a list of materials that every consumer walks,
  so the common one-surface case pays for the general one.
- **Rejected because**: splitting a file into several models could be added later without
  changing this type.

## Consequences

- **Gains**:
  - A model is one upload and one draw, and names no device type, so the offline renderer can
    read it.
  - The tree keeps one image decoder.
- **Costs**:
  - Only the first material in a file survives the merge.
  - An embedded texture is not read, so a `.glb` with packed textures loads untextured.
  - A mismatch between the loader's layout and a pipeline draws a wrong picture rather than
    failing to build.
- **Revisit when**: a file needs more than one surface, as a skinned character does.
