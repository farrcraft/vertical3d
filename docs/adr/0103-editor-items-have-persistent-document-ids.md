# ADR-0103: Editor: items have persistent document ids

**Status**: accepted
**Date**: 2026-10-24
**Documented in**: [editor/Scene.md](../editor/Scene.md#the-scene)

## Context

An item's `dag::Node` id comes from a process-wide counter and means nothing in another run.
Groups, material assignments, graph connections, constraint targets and keys on a parameter all
refer from one record to another, and those references have to survive a save. A position in a
list moves when an item before it is removed. A saved document is updated row by row, so a row
needs a key that does not change while the item exists.

## Decision

Each item has a document id, an integer the scene assigns when the item is first added and that
the document keeps for as long as the item exists. Ids are never reused within a document, so the
document stores the next id to assign. A saved reference names an item by its document id, and
the `dag::Node` id stays the key of a run's selection.

## Alternatives

### A UUID for each item
- **For**: unique across every document, so an item copied or linked between scenes keeps its id.
- **Against**: a 16-byte key in every row and reference, and a copy within one document still
  needs a new id.
- **Rejected because**: a reference across documents names the document too, so an id unique
  within one document is enough.

### Refer to items by their position
- **For**: nothing to assign or store.
- **Against**: a removal renumbers everything after it, so every reference has to be rewritten,
  and a row cannot be updated in place.
- **Rejected because**: an incremental save needs keys that do not move.

### The `dag::Node` id, saved
- **For**: one id per item.
- **Against**: the counter is shared by the whole process, so an id read from a file can collide
  with one already given out.
- **Rejected because**: the two ids answer to different lifetimes.

## Consequences

- **Gains**:
  - A reference survives a save, a removal and an undo, because an undone creation keeps its id.
  - A save updates, inserts and deletes rows by a key that does not move.
- **Costs**:
  - An item carries two ids, and code must use the document id for anything saved.
  - Merging or linking documents will need a remapping of ids.
- **Revisit when**: items are linked between documents often enough that remapping costs more
  than a wider id.
