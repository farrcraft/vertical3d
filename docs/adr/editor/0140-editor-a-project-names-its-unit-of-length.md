# ADR-0140: Editor: a project names its unit of length

**Status**: accepted
**Date**: 2026-10-31
**Tags**: editor
**Documented in**: [docs/editor/Scene.md](../../editor/Scene.md), [docs/editor/Files.md](../../editor/Files.md)

## Context

The properties editor shows distances, and a distance needs a unit to be read. Files arrive in
different units: glTF is always in metres, and a USD stage names its own, falling back to
centimetres. A float is most precise near one, so a scene of a watch and a scene of a city each
want a unit near their subject. Blender, Houdini and USD each let a scene or a file name its unit,
and default to metres.

## Decision

A project names its unit of length, and a new one measures in metres. Changing the unit changes
what the numbers mean and moves nothing. A file is read in metres and scaled into the project's
unit as it is imported.

## Alternatives

### One unit is always one metre
- **For**: nothing to store, and it is what glTF uses and what the USD reader already converts to.
- **Against**: a project far from a metre in size loses precision, and nobody can work in the unit
  of the thing being made.
- **Rejected because**: established tools let a scene choose, and the editor should hold a part as
  well as a landscape.

### No unit
- **For**: nothing to store or show.
- **Against**: a distance cannot be read, and an import's size changes with its source without a
  word.
- **Rejected because**: the properties editor and the imports both need a unit to be correct.

## Consequences

- **Gains**: distances read in a unit, imports keep their real size, and a project can choose a
  unit near its subject.
- **Costs**: a project stores one more setting, and an import is copied and scaled when the unit is
  not metres. Changing the unit of a project that already holds geometry does not rescale it,
  which is right for a mislabelled project and wrong for one that should have been converted.
- **Revisit when**: someone needs to convert a project's geometry to a new unit, which would add a
  converting change beside the relabelling one.
