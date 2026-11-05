# ADR-0135: Editor: item names are labels that need not be unique

**Status**: superseded
**Date**: 2026-10-30
**Tags**: editor
**Superseded by**: ADR-0157
**Documented in**: [editor/Scene.md](../../editor/Scene.md#the-scene)

## Context

An outliner lists the scene's items, and its filter matches text, so every item needs a name the
user can read and change. glTF and USD files name their nodes, and an import keeps those names.
Items already refer to one another by document id, so nothing saved depends on a name. Some
tools make names unique so that a name or a path can refer to an item, and then every create,
import, rename, paste and undo has to keep them unique.

## Decision

Every item has a name, any text and possibly empty, that the document saves. Two items may share
a name, and nothing renames an item but the user. A writer whose format needs unique names, such
as a USD exporter, makes them as it writes and leaves the scene's names as they are.

## Alternatives

### Unique in the scene
- **For**: a name refers to one item, so a script or an expression can name it.
- **Against**: every way an item enters the scene has to rename on a clash, and an undo that
  brings back a removed item may find its name taken.
- **Rejected because**: references are by document id, so uniqueness buys nothing the scene
  needs.

### Unique among siblings
- **For**: a path of names refers to one item, as a USD prim path does.
- **Against**: grouping and reparenting can put two items with one name under one parent, and
  then have to rename one of them.
- **Rejected because**: the same as above, and a move would change a name the user chose.

## Consequences

- **Gains**:
  - An import keeps a file's names exactly, duplicates included.
  - A rename is one value on one item, and undoes as one.
- **Costs**:
  - A name does not identify an item, so a user looking for one of two items called Cube has to
    tell them apart some other way.
  - An exporter to a format with unique names has to make them, and they can differ from the
    scene's.
- **Revisit when**: something the user writes, such as an expression or a script, has to name
  an item.
