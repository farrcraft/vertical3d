# ADR-0069: Models: material parts over one vertex buffer

**Status**: accepted
**Date**: 2026-10-03
**Supersedes**: [ADR-0030](0030-models-one-interleaved-array.md)
**Documented in**: [api/Assets.md](../api/Assets.md)

## Context

A model that keeps one material loses every other surface in its file. A character is rarely one
surface, and all its surfaces are bent by one set of joints. Skinning adds a skeleton and a set
of joint influences per vertex, which a static model does not need. The vertex layout is an
agreement between the loader and every pipeline that draws a model, so changing it changes every
consumer.

## Decision

A model is one vertex array and one index run, divided into parts, where a part is an index range
drawn with one of the model's materials. A file with several surfaces is one model, uploaded once
and drawn with a draw per part. A model may carry a skin, a skeleton plus an influence per vertex
held in an array beside the vertices, so the vertex layout is unchanged and a static model leaves
the array empty.

## Alternatives

### One model per material
- **For**: `type::Model` keeps its shape, and a static prop loses nothing.
- **Against**: a character becomes several registry entries. A skin shared between them needs a
  pose written once and read by each, and a file is no longer one handle.
- **Rejected because**: skinned geometry is bent as one thing, and keeping its surfaces in one
  model keeps the pose single. One answer for static and skinned files is simpler than two.

### Joints and weights added to `Model::Vertex`
- **For**: one vertex layout, so one pipeline vertex description serves both kinds of model.
- **Against**: every static vertex carries 24 bytes it never reads, and the layout every pipeline
  and consumer is written against changes.
- **Rejected because**: an array beside the vertices costs a static model nothing.

## Consequences

- **Gains**:
  - A file with several surfaces loads whole.
  - One upload, one handle and one skeleton per file, and a part is an index range the recorder
    already knows how to draw.
  - A static model's vertex data is unchanged.
- **Costs**:
  - Every consumer walks a model's parts, including the common one-part case.
  - A part whose range is wrong draws another part's triangles rather than failing.
- **Revisit when**: a model needs parts that do not share a vertex layout, such as a surface with
  tangents beside one without.
