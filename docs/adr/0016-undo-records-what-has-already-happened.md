# ADR-0016: Undo — A Command Records What Has Already Happened, And A Gesture Is One Of Them

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor creates meshes, selects them and moves them, and none of it can be taken back.
Undo is on the phase 6 list and rigel contributes nothing to it: a case-insensitive search
for undo or redo over the whole tree returns nothing, so this is a design rather than a
fold-in. What rigel does have is a `Command` and a `CommandDirectory` — those went to
`v3dlibs/command` and from there to `api/event`, where they became named events dispatched
by context. Neither ever had an inverse.

The shape of the problem is set by how a change is made rather than by what it changes. A
create is one keypress. A transform is a drag: `TransformTool` applies a manipulator to the
mesh on every motion event, because a gesture cannot wait for its own end to show what it is
doing, and per [ADR-0015](0015-manipulators-write-the-object-transform.md) each `apply()`
measures one event's worth of movement and adds it. By the time anything could be recorded,
the change has been made — several hundred times over for one drag of the mouse.

## Decision

A **command is a record of a change that has already been made**, not a request to make one.
`v3d::editor::Command` has `undo()`, `redo()` and `name()`, and no `execute()`:
`CommandStack::push` never applies anything. **One gesture is one command**, and the tool
that owns the gesture is what knows where it begins and ends.

`CommandStack` is a pair of stacks — what has been done, and what has been undone out of it.
Pushing abandons the undone side, because a new change is a branch of the history that was
never taken. The stack has a capacity and drops the oldest first, so that a session cannot
grow one without bound.

Two commands exist. `CreateCommand` holds the scene and the mesh; `TransformCommand` holds
the mesh and a `Placement` — translation, rotation and scale together — from either end of
one gesture. Three things follow:

- **A command holds the object it acts on rather than its id.** A mesh taken out of the
  scene by an undo stays alive in the command that removed it and comes back with the id it
  had, so a transform command deeper in the history still names the same object. Looking a
  mesh up by id would find nothing and silently undo nothing.
- **The first do goes through `redo()`.** `Controller::createPoly` builds the command, calls
  `redo()` and pushes it, so creating a mesh and redoing one cannot drift apart. This costs
  nothing where a change is not interactive, and is not available where it is — a drag has
  already happened by the time the command exists.
- **Every path out of a drag commits.** `TransformTool::commit()` is called by the release
  and by a mode change that drops the drag alike, and pushes only if the placement actually
  moved. A change that reached the mesh but not the stack is one undo cannot reach.

A gesture is undone by putting the whole placement back rather than the part that moved,
which keeps the record independent of which manipulator made it.

**Selection is not history.** Undo does not restore what was selected before a command; it
only keeps the scene's own invariant that at most one mesh is selected, which is why
`CreateCommand::undo` clears the flag on the mesh it removes.

The stack is the editor's, not the api's — like the construction grid of
[ADR-0011](0011-lines-are-the-second-primitive.md) and the picker of
[ADR-0014](0014-picking-is-a-cpu-ray-cast.md), it is policy over a document, and the editor
is the only thing in the repository that has one.

## Alternatives Considered

### Alternative 1: A command records a completed change; the stack never executes — **chosen**
- **Pros**: It is the only shape an interactive gesture fits. The tool applies the drag as it
  happens and hands over a record at the end, so one drag is one undo step rather than one
  per motion event. It needs nothing of the code that makes the change: a manipulator, a
  primitive constructor and a future modelling operation all just do their work and say what
  they did. And a record of two end states is exact — replaying a gesture from its input
  would have to reproduce the camera, the viewport and the cursor path that produced it.
- **Cons**: There are two ways to make a change — through the command, as a create does, and
  beside it, as a drag does — so nothing structurally prevents a change that forgets to
  record itself. The stack cannot verify that what it holds ever happened.
- **Why not**: n/a — chosen.

### Alternative 2: A command is the only way to change anything, and the stack executes it
- **Pros**: One path in, so a change cannot escape the history. It is the textbook command
  pattern, and the shape rigel's `Command` was heading for.
- **Cons**: A drag would have to be a command per motion event, which is hundreds of undo
  steps for one gesture and a stack that fills with them; or the tool would have to buffer
  the gesture and issue one command at the end, applying nothing meanwhile — a manipulator
  that shows nothing until the mouse is released. Coalescing after the fact needs a rule for
  which commands merge, which is the gesture boundary again by a longer route.
- **Why not**: It makes the interactive case, which is most of a modeller, the awkward one.

### Alternative 3: Snapshot the whole scene before each change
- **Pros**: Trivially correct, and correct for operations nobody has written yet — a
  modelling operation that rebuilds a mesh's vertex array needs no thought at all.
- **Cons**: A snapshot is a deep copy of every mesh in the document, and there is no copy
  constructor for a `BRep` that would survive its half edge pointers. At a drag's release it
  would copy the scene to record a change of nine floats. Nothing in a snapshot says what
  changed, so a menu can never label an undo.
- **Why not**: The cost is set by the size of the document rather than the size of the
  change, and the deep copy does not exist to be written cheaply.

### Alternative 4: Put the command model in `api/event` beside the dispatcher
- **Pros**: `api/event` already turns input into named commands, and rigel's `Command` lived
  in the library that became it. Every app would get undo.
- **Cons**: No other app has a document to undo against — a game's state is the game. The
  stack would either be generic to the point of being an empty interface, or drag the
  editor's scene into a library four games link.
- **Why not**: One consumer is not a library. It moves to the api when a second editor-shaped
  app wants it.

## Consequences

### Positive
- A create and a transform can be taken back and put back. Verified against a run on
  2026-09-02: a cylinder created, undone — the mesh leaves all four views — undone again
  against an empty history, and redone, with the validation layer silent.
- One drag is one undo step however many motion events it took, and a handle grabbed and
  released without moving records nothing.
- A mode change that drops a drag still commits what the drag wrote, so the two ways a
  gesture can end agree.
- Fifteen cases cover the stack, the two commands and the tool's gesture boundary, over the
  existing suite. All of it runs without a window or a device.

### Negative
- Nothing but a create and a transform is undoable. A selection change is not history at all,
  so an undo can leave a different thing selected than the user had — most visibly after
  undoing a create, which leaves nothing selected.
- A command that fails to record itself is indistinguishable from a change that did not
  happen. The discipline is per operation, and the only thing enforcing it is that every path
  out of a drag goes through one function.
- The undo stack holds meshes that are no longer in the scene, so a create undone and then
  pushed off the end of the stack by 64 further commands is only freed at that point.
- `Scene::add` appends, so a mesh redone into the scene comes back last rather than where it
  was. Nothing depends on the order today; a project file that records one would.

### Risks
- The capacity is a fixed 64 with no way to configure it. A document big enough for that to
  matter is a document the editor cannot yet save.
- Undo and redo are on z and y, unmodified. `api/event`'s mappings bind one key with one
  state and have no notion of a chord, so ctrl-z is not expressible — and control is already
  the truck camera modifier. This is the same placeholder the create and select mask keys
  are, and it goes when the menus land.
- A modelling operation that edits geometry will not fit `TransformCommand`, and the
  placement pair is not the pattern to copy for it: it will need to record the topology it
  changed. The stack itself does not care.
