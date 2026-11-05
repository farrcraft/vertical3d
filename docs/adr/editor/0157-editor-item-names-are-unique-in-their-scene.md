# ADR-0157: Editor: item names are unique in their scene

**Status**: accepted
**Date**: 2026-11-02
**Tags**: editor
**Supersedes**: ADR-0135
**Documented in**: [editor/Scene.md](../../editor/Scene.md#the-scene)

## Context

A script and a console line name the items they act on in text, and the console shows what was
run in a form that can be run again. A document id means nothing to the person reading or
writing a script, so the text has to be the name the outliner shows. Blender and Maya make names
unique for the same reason, and rename an item on a clash with a numbered suffix. Items still
refer to one another by document id, so nothing saved depends on a name.

## Decision

No two items in a scene share a name. An item that enters the scene with a name another holds
takes the lowest free number after it, as `Cube.001` follows `Cube`, and an item with no name is
called by its kind. A rename typed into the properties editor is made unique when its edit ends.

## Alternatives

### Names need not be unique, and the text names an item by name and id
- **For**: an import keeps a file's names exactly, and nothing renames an item but the user.
- **Against**: a script that names an item by name alone breaks when a later item takes the same
  name, and one written with an id cannot be read or run against another document.
- **Rejected because**: a script should name what the user sees in the outliner.

### Unique among siblings, named by a path
- **For**: a path of names refers to one item, as a USD prim path or a Maya DAG path does.
- **Against**: a script has to spell out the hierarchy to name one item, and grouping or
  reparenting changes the path of everything moved.
- **Rejected because**: a script would break each time the user reorganised the outliner.

## Consequences

- **Gains**: a name refers to one item, so a script, a console line and the outliner's filter
  all name it the same way.
- **Costs**: an import whose file repeats a name, and a document saved before this rule, take
  numbered suffixes. Every way an item enters the scene goes through the one rule in
  `Scene::add()`.
- **Revisit when**: a scene can hold items from more than one document, which would need names
  qualified by where they came from.
