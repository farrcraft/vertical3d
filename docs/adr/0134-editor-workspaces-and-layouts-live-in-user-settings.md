# ADR-0134: Editor: workspaces and layouts live in user settings

**Status**: accepted
**Date**: 2026-10-30
**Documented in**: [editor/Scene.md](../editor/Scene.md#viewports)

## Context

The editor's window is a tiling of areas the user arranges, and a user keeps several
arrangements for different work, such as modelling, animation and rendering. A project is a
directory of content that may be shared and kept under version control. Resizing an area is not
an edit to the scene, so it should not mark the project unsaved or change its files. A user who
returns to a project expects to find it arranged as it was left.

## Decision

Named workspaces, each an area layout, are kept in the user's settings, and the editor's data
gives the workspaces a new user starts with. The user's settings also record, for each project
by its path, the workspace it was last shown in and that arrangement, and opening the project
restores it. A workspace is shared with someone else by exporting it to a file and importing
that file.

## Alternatives

### Layouts saved with the project
- **For**: a project opens arranged for the work it holds, and a team that shares the project
  shares its arrangement.
- **Against**: opening someone else's project replaces the user's own layout, a workspace made in
  one project is missing from every other, and resizing an area changes the project's files.
- **Rejected because**: the arrangement belongs to the person working, not to the content, and
  tools that store it with the document need an option to ignore it on open.

### Workspaces in user settings, with no record per project
- **For**: one place holds every layout, and the project's files hold only content.
- **Against**: every project opens in the same arrangement, so returning to an animation means
  setting the workspace up again.
- **Rejected because**: reopening a project as it was left costs a small record in the settings,
  and leaves the project's files untouched.

### Each project's arrangement kept in its own directory
- **For**: the record moves with the project, and survives a different user or machine.
- **Against**: the project directory gains a file that is neither content nor shared, which
  every user has to keep out of version control.
- **Rejected because**: the arrangement is personal, and the project directory holds what the
  project is.

## Consequences

- **Gains**:
  - A user's workspaces follow them to every project.
  - A project reopens as it was left, and arranging areas never dirties it.
  - The project's files and its version history hold content only.
- **Costs**:
  - A project that is moved or renamed loses its arrangement, and opens in the last workspace
    used.
  - A project's arrangement does not travel to another user or another machine.
  - Records accumulate for projects that no longer exist, so the settings have to prune them.
  - A team's shared workspace goes through an export and an import, not through the project.
- **Revisit when**: projects are routinely shared with an arrangement that matters to their
  content, such as a review or a teaching setup.
