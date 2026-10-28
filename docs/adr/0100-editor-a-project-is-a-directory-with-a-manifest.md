# ADR-0100: Editor: a project is a directory with a manifest

**Status**: accepted
**Date**: 2026-10-24
**Documented in**: [editor/Files.md](../editor/Files.md#projects)

## Context

A project grows content that is not the scene: textures and imported source files, caches of
evaluated geometry and bakes, rendered pictures, and recovery data from autosave. Textures are
edited in other tools and are only useful there as standard files. Caches and renders can be
large, can be rebuilt, and should be easy to delete or keep out of version control. Several
scenes of one project may share assets. A scene document that holds all of it grows with content
it does not need to save.

## Decision

A project is a directory. A JSON manifest at its root names the format version and the project's
scenes, and fixed subdirectories hold scenes, assets, caches, renders and autosave data. A scene
refers to a file of the project by its path relative to the project directory.

## Alternatives

### One file that holds everything, with assets packed inside it
- **For**: a project moves and is shared as one file, and nothing it refers to can go missing.
- **Against**: textures leave the tools that edit them, caches and renders inflate every save,
  and two scenes cannot share an asset.
- **Rejected because**: most of a large project's bytes are content that is better kept as
  ordinary files.

### A scene file with loose files beside it, and no project
- **For**: nothing new to define. Each scene stands alone, as the editor's files do today.
- **Against**: nothing says where an asset, a cache or a render belongs, so each feature invents
  a place, and shared assets and project settings have no home.
- **Rejected because**: every feature that writes files needs one agreed layout.

## Consequences

- **Gains**:
  - Assets stay ordinary files, shared between scenes and edited by other tools.
  - Caches, renders and autosave data can be deleted, and kept out of version control, by
    directory.
  - Each feature that writes files has a defined place to write them.
- **Costs**:
  - A project moves as a directory, and a reference breaks when a file in it is moved or deleted.
  - A save that writes several files is not one transaction, so a scene must never depend on an
    asset file changing in step with it.
- **Revisit when**: projects need to refer to files outside their directory, such as a shared
  asset library.
