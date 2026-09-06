# ADR-0023: Offline Scene Description — RIB Is What Both Renderers Read, And The Editor Exports To It

**Date**: 2026-09-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Phase 2 of [the offline rendering roadmap](../roadmap/OfflineRendering.md) is blocked on which
scene description is primary, because that decides where the reader lives and what the phase
costs.

There are two candidates in the tree. talyn has a `RIBReader` that recognises twenty-odd
requests and acts on one of them, and moya has the RI entry points that reader should be
calling — RIB is a serialisation of the RI procedure calls, so the two halves of one design are
sitting in different applications. The other is the editor's project file
([ADR-0018](0018-a-project-is-json-and-stores-topology-verbatim.md)), which is the only actual
scene in the tree and the only source of meshes that is not hand-written, and which `api/asset`
already parses.

The project file carries topology and a placement per mesh and **nothing else**. The editor's
`Scene` has no lights and no materials, so a renderer reading it directly renders grey until
the editor's scene model grows the things phase 3 needs.

## Decision

RIB is the scene description both offline renderers read, and the reader lives in
`api/render/offline` per [ADR-0022](0022-offline-rendering-shares-an-api-library.md), dispatching
requests onto an interface each renderer implements. The project format of ADR-0018 stays
**internal to the editor**; when the editor needs a scene rendered offline it gains a RIB
export, and the renderers learn nothing about its document format.

## Alternatives Considered

### Alternative 1: RIB, with a RIB export from the editor — **chosen**
- **Pros**: One reader serves both renderers, and it lands where both can reach it. The RI
  surface moya already declares in full stops being dead code — requests have somewhere to
  land. A scene becomes a text file a test can commit, which is what a reference-image
  comparison needs. The editor's document format stays free to change without breaking a
  renderer.
- **Cons**: A real tokenizer has to be written before phase 2 produces anything, and the export
  is work the editor itself gets nothing from.
- **Why not**: n/a — chosen.

### Alternative 2: The editor's project JSON is primary
- **Pros**: The only real scene in the tree, already parsed by `api/asset`, and no tokenizer to
  write. First picture from a modelled mesh comes sooner.
- **Cons**: It couples the offline work to the editor's schedule — nothing shades until the
  editor's `Scene` grows lights and materials, which is phase 3 work in a different area. And it
  turns a document format into an interchange format, so every renderer feature would push back
  on the file the editor has to keep readable.
- **Why not**: A document format and a scene description have different reasons to change, and
  ADR-0018 stores topology verbatim precisely because it is the editor's own.

### Alternative 3: Both, through one intermediate scene representation
- **Pros**: Neither format constrains the other, and the renderers read one thing.
- **Cons**: A third representation whose shape nothing justifies yet, plus two importers instead
  of one importer and one exporter. It is the design that looks right before either renderer has
  shaded a pixel and gets rewritten once one has.
- **Why not**: Premature. Nothing forecloses it — an intermediate can be introduced later behind
  the same reader interface.

### Alternative 4: talyn keeps its own reader and moya is driven through the RI calls
- **Pros**: What the tree does today, and it needs no shared library.
- **Cons**: Two tokenizers, the harder half written twice, and talyn's reader has no path to the
  geometry requests it recognises and discards. moya would be reachable only from C++ driver
  code.
- **Why not**: It is the status quo, and the status quo is why neither renderer can be handed a
  scene.

## Consequences

### Positive
- Both renderers are driven the same way, so one committed `.rib` scene is a test fixture for
  both, and the phase 1 reference images have somewhere to come from.
- The RI request set decides what a scene can say, which is a specification someone else wrote
  and argued over. Neither renderer has to invent a format.
- moya's existing entry points define the reader's interface, so the reader has a target before
  talyn implements any of it. Unimplemented requests are no-ops — which is what the RI standard
  asks of a renderer that does not support a feature.

### Negative
- The current whitespace tokenizer is not enough: quoted strings, bracketed arrays and typed
  parameter lists all have to be handled before a single geometry request works.
- Hand-written RIB is the only scene until the editor's exporter exists, so phase 2 starts with
  fixtures a person typed.
- A round trip through the editor loses whatever RIB can express and the project file cannot,
  and gains nothing the editor can read back. The export is one way.

### Risks
- RIB parameter lists are typed by declaration — `RiDeclare` and the inline form — so a reader
  that guesses types gets stuck at the first parameter that is not `P`. Mitigation is to build
  the declaration table with the tokenizer rather than after it.
- Binary and gzipped RIB are part of the format and are not in scope. A file that is not ASCII
  should be rejected with a message that says so, not parsed into nonsense.
