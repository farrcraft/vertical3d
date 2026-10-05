# ADR-0023: Offline: RIB is the scene format

**Status**: accepted
**Date**: 2026-09-04
**Documented in**: [offline/Rib.md](../offline/Rib.md), [editor/Files.md](../editor/Files.md)

## Context

The offline renderer needs a scene description, and two candidates exist in the tree. RIB is
the text serialisation of the RenderMan Interface (RI) calls, and moya already declares those
calls. The editor's project file ([ADR-0018](0018-editor-projects-saved-as-json-with-exact-topology.md))
is the only source of modelled meshes, and `api/asset` already parses it. The project file holds
topology and a placement per mesh and nothing else: no lights and no materials.

## Decision

RIB is the scene description moya reads, through one reader in `api/render/offline` per
[ADR-0022](0022-offline-shared-library-with-no-vulkan.md). The editor's project format stays
internal to the editor. When the editor needs a scene rendered offline, it exports RIB, one way.

## Alternatives

### The editor's project JSON is primary
- **For**: It is the only real scene in the tree, it is already parsed, and there is no
  tokenizer to write. The first picture of a modelled mesh comes sooner.
- **Against**: Nothing shades until the editor's scene model gains lights and materials, which
  ties offline rendering to the editor's schedule. It also turns a document format into an
  interchange format, so every renderer feature pushes on a file the editor has to keep
  readable.
- **Rejected because**: A document format and a scene description change for different reasons.
  ADR-0018 stores topology exactly because the format is the editor's own.

### Both, through one intermediate scene representation
- **For**: Neither format constrains the other, and the renderer reads one thing.
- **Against**: A third representation whose shape nothing justifies yet, and two importers
  instead of one reader and one exporter.
- **Rejected because**: It is premature. An intermediate can still be added later behind the same
  reader interface.

## Consequences

- **Gains**:
  - A scene is a text file a test can commit, so every reference image has a `.rib` beside it.
  - The RI request set decides what a scene can say. It is a published specification, so the
    tree does not invent a format.
  - The editor's document format can change without breaking the renderer.
  - An unimplemented request is accepted and ignored, which is what the RI standard asks of a
    renderer that lacks a feature.
- **Costs**:
  - A full tokenizer is required before any geometry works: quoted strings, bracketed arrays and
    parameter lists typed by declaration.
  - The editor export is work the editor itself gains nothing from. A round trip through it is
    one way, and anything RIB can express that the project file cannot is lost.
  - Binary and gzipped RIB are not read.
- **Revisit when**: a second scene source appears that RIB cannot carry without loss, which is
  the case for an intermediate representation.
