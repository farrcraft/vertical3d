# ADR-0218: Editor: shared geometry is a record mesh items refer to

**Status**: accepted
**Date**: 2026-11-10
**Tags**: editor, files
**Documented in**: [docs/editor/Files.md](../../editor/Files.md), [docs/editor/Scene.md](../../editor/Scene.md)

## Context

Several mesh items can show one geometry. Import makes them from a glTF mesh that several nodes
name, and a scene of repeated parts needs them. The scene document writes each mesh item's
geometry in full, so a shared mesh reads back as copies, and what an edit of it changes is
undefined. moya's scene holds geometry as prototypes that instances place, and Blender's mesh
data and Maya's shape nodes are likewise apart from the objects that show them. The document's
format is hard to change once projects are saved in it.

## Decision

A mesh's geometry is a record of its own in the document, and each mesh item refers to one by
id. An edit of shared geometry changes every item that shows it, and a command gives an item a
copy of its own. A geometry no item refers to is not saved.

## Alternatives

### A mesh item that refers to another mesh item
- **For**: no new record, and one item stays the source the others follow.
- **Against**: deleting, renaming or reparenting the source breaks every item that follows it,
  and the source is treated differently from the items that share it.
- **Rejected because**: sharing is a relation among equals, and every major tool keeps the
  geometry apart from the items that show it.

### Copies merged on save
- **For**: the scene needs no notion of sharing, and the document is as small as with a record.
- **Against**: two meshes that happen to be equal become one on reload, and then edit together,
  so a save changes what the scene does.
- **Rejected because**: whether two items share geometry is the user's choice, and a save must
  keep it rather than infer it.

### A copy on edit
- **For**: editing one item never changes another by surprise, as USD's instances refuse edits
  until they are made their own.
- **Against**: a change to fifty repeated parts has to be made fifty times, and sharing lasts
  only until the first edit.
- **Rejected because**: changing every copy at once is the reason geometry is shared, and Blender
  and Maya both edit shared geometry in place.

## Consequences

- **Gains**: an import keeps its sharing through a save, a repeated part is stored and edited
  once, and moya receives one prototype for it.
- **Costs**: a new schema version and a new mesh version with their migrations. An edit can change
  items the user is not looking at, so the outliner and properties editor have to show that a
  mesh is shared.
- **Revisit when**: geometry needs to be shared across documents, which would make it a file of
  its own rather than a record.
