# ADR-0016: Editor: undo records completed changes

**Status**: accepted
**Date**: 2026-09-02
**Documented in**: [The editor](../Editor.md)

## Context

The editor creates and moves meshes, and both have to be undoable. A create is one action, but a
transform is a drag: the manipulator changes the mesh on every motion event so the user sees it
move, per [ADR-0015](0015-editor-manipulators-edit-the-object-transform.md). By the time a change
could be recorded, it has already been made, many times over for one drag. The undo history
should hold one step per user action, not one per motion event.

## Decision

A command is a record of a change that has already been made, not a request to make one:
`v3d::editor::Command` has `undo()` and `redo()` and no `execute()`, and the command stack never
applies anything. One gesture is one command, and the tool that owns the gesture decides where
it begins and ends. The stack belongs to the editor, not the api.

## Alternatives

### Every change goes through a command, and the stack executes it
- **For**: one path for every change, so none can escape the history. It is the textbook command
  pattern.
- **Against**: a drag would be a command per motion event, filling the stack with hundreds of
  steps. Alternatively the tool would buffer the gesture and apply nothing until release, so the
  user would see nothing move. Merging commands afterwards needs a rule for the gesture boundary
  anyway.
- **Rejected because**: it makes interactive changes, most of a modeller's work, the awkward
  case.

### Snapshot the whole scene before each change
- **For**: trivially correct, including for operations not yet written.
- **Against**: each snapshot is a deep copy of every mesh, and `BRep` cannot be copied. A
  snapshot cannot say what changed, so an undo cannot be labelled.
- **Rejected because**: the cost depends on the document's size, not the change's.

### Put the command model in `api/event`
- **For**: `api/event` already turns input into named commands, and every app would get undo.
- **Against**: no other app has a document to undo. The stack would be an empty generic
  interface, or it would bring the editor's scene into a library the games link.
- **Rejected because**: one consumer does not justify a library. It can move when a second
  editor-like app needs it.

## Consequences

- **Gains**:
  - One drag is one undo step however many motion events it took.
  - Code that makes a change only has to report what it did, so manipulators, primitives and
    future modelling operations need nothing special.
  - A record of the start and end states is exact, with no need to replay input.
- **Costs**:
  - Changes can be made outside the stack, so nothing structurally stops a change that forgets
    to record itself.
  - Selection is not history, so an undo can leave a different thing selected than before.
  - A command holds the mesh it acts on, so a mesh removed by an undo stays in memory until its
    command leaves the stack.
  - A modelling operation that changes topology will need its own command type that records the
    topology it changed.
- **Revisit when**: a second app needs undo, or topology-changing operations make change records
  expensive.
