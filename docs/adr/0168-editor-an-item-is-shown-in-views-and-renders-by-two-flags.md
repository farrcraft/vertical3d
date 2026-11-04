# ADR-0168: Editor: an item is shown in views and renders by two flags

**Status**: accepted
**Date**: 2026-11-04
**Documented in**: [docs/editor/Scene.md](../editor/Scene.md), [docs/editor/Files.md](../editor/Files.md), [docs/api/Interchange.md](../api/Interchange.md)

## Context

An artist hides an item to work around it, and leaves an item out of a render that is only a
guide, a proxy or a helper. These are two separate needs: a guide is wanted in the views and not
in the picture, and a finished prop may be hidden while another is worked on and still be
rendered. USD keeps `visibility` apart from `purpose`, glTF has `KHR_node_visibility`, and
Blender, Maya and Houdini each keep what the views show apart from what is rendered. The editor
reads both file formats, writes RIB, and saves a document whose format is hard to change once
written.

## Decision

Every item carries two flags of its own, shown in the views and shown in renders, both on for a
new item and both saved. An item is shown in the views, or in renders, only while it and every
item above it are, so hiding a parent hides what is beneath it. The views neither draw nor pick
an item they do not show, and a render, RIB export included, leaves out an item it does not
show.

## Alternatives

### One flag for both
- **For**: one control, one column, and nothing to explain.
- **Against**: a guide or a proxy cannot be seen while working and left out of the picture, and
  USD's purpose has nowhere to go on import.
- **Rejected because**: the two needs are both common, and every tool the editor reads from
  keeps them apart.

### RenderMan's split of render visibility into camera, indirect and transmission
- **For**: an item can be kept out of the camera's view and still cast shadows or show in
  reflections, which a ray traced render uses.
- **Against**: three render flags where a scanline render reads one, and none of the formats
  the editor imports carries them.
- **Rejected because**: the editor has no use for the split yet, and a render flag can later be
  refined into it without changing what the views flag means.

### Flags that do not pass to children
- **For**: each item says exactly what happens to it, with no walk up the tree.
- **Against**: hiding a group would leave its contents in view, and glTF and USD both hide what
  is beneath a hidden node.
- **Rejected because**: it reads files differently from the tools that wrote them, and hiding a
  rig would mean hiding each of its parts.

## Consequences

- **Gains**: hiding, isolating and leaving items out of renders work as in other 3D tools, and
  the visibility a glTF or USD file holds survives import.
- **Costs**: a document schema version, two columns on every item, and a walk up the tree to
  know whether an item shows. A curve keeps its own flag for whether a render draws it as
  ribbons, so a curve is rendered only when both say so.
- **Revisit when**: a renderer needs an item kept out of the camera while it still lights or
  reflects, or an exporter writes a format that splits render visibility further.
