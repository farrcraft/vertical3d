# ADR-0069: Models: material parts over one vertex buffer

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

Amends [ADR-0030](0030-models-one-interleaved-array.md).

## Context

[ADR-0030](0030-models-one-interleaved-array.md) merged a file into
one vertex array, one index run and one material, keeping the first material and deferring the
rest: "splitting a file into several `Model`s is the natural extension". Skeletal animation
([SkeletalAnimation](../plans/completed/SkeletalAnimation.md)) is what makes the rest due. A character is
rarely one surface, and its surfaces are bent by one set of joints. The loader also walks the
file's mesh list and never its nodes, so a mesh a node places loads at its own origin, and a
skin's joints are nodes. `type::Model` is read by the loader's suite and `MeshRegistry` alone.

## Decision

**A model is parts over one vertex array and one index run.** A part is an index range and a
material, and a model holds a list of materials. A file whose parts need different surfaces is
one model, still one upload, drawn as a draw per part. Primitives sharing a material join one
part.

**The loader reads the file's scene, not its mesh list.** Each mesh a node names is merged
where the node's world matrix places it, with its normals turned by that matrix's inverse
transpose.

**A model may carry a skin**: a skeleton, and an influence per vertex in an array parallel to
the vertices that a static model leaves empty. `Model::Vertex` is unchanged. How a skinned
vertex is laid out on the device is the pipeline's, as every vertex layout already is.

## Alternatives Considered

### Alternative 1: One model per material
- **Pros**: `type::Model` keeps its shape, and the split is the one ADR-0030 foresaw. A static
  prop loses nothing by it.
- **Cons**: A character is several registry entries, and a skin shared between them needs a
  pose written once and read by each. A file stops being one handle.
- **Why not**: Skinned geometry is one thing bent together. Keeping its surfaces in one model is
  what keeps the pose single, and one answer for static and skinned files is simpler than two.

### Alternative 2: Parts in one model — **chosen**
- **Pros**: One upload and one handle per file, one skeleton per model, and a part is an index
  range the recorder already draws.
- **Cons**: Every consumer of a model walks its parts, including the one-part case.
- **Why not**: n/a — chosen.

### Alternative 3: Joints and weights added to `Model::Vertex`
- **Pros**: One layout, so one pipeline vertex description for both kinds.
- **Cons**: Every static vertex in every consumer carries 24 bytes it never reads, and the
  32-byte layout the registry asserts and retcon copies changes.
- **Why not**: The parallel array costs a static model nothing at all.

## Consequences

### Positive
- A file with several surfaces loads whole, where it lost all but its first.
- A mesh a node moves, turns or scales is placed where the file puts it.
- A static model is unchanged to the byte.

### Negative
- `material()` becomes `materials()`, and the registry, its walks and retcon's `GltfLoader`
  change with it.
- A mesh named by two nodes is merged twice, so a file that reuses a mesh many times is a
  larger upload than the file.

### Risks
- A part is an index range into a shared array, so a part whose range is wrong draws another
  part's triangles rather than failing. The loader's suite asserts every range against a
  fixture whose parts are told apart by position.
