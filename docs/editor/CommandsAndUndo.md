# Commands and undo

How a menu item, toolbar button or key reaches the code that handles it, how a change is
made and recorded so it can be undone, and how the mouse reaches the tools.

## The document and its operations

`v3d::editor::Document` is what the editor edits: the scene, its `CommandStack`, and the
`Project` it is kept in, with the saved position, the autosave and the recovery that go with
them. It is in `src/document/`.

**Every change a user can ask for is an operation on the document.** An operation applies
itself, records itself in the history, and returns an `Outcome`: whether it did what it was
asked, and a message to show. An operation that cannot run changes nothing, records nothing,
and says why.

- **What an operation acts on is in its arguments or its name.** `group(items)` and
  `key(items, frame)` take the items. `createCamera(at, projection)` takes the view's camera
  profile. The modelling operations act on the scene's selected parts, as `splitEdges()` says.
  None of them reads the select tool, the active view or the pane under the cursor.
- **The command table supplies the arguments.** Each handler in `registerCommands()` reads the
  selection, the frame or the active view and passes them in. The select mask is a rule of the
  ui, not of the document, so the handler checks it before a modelling operation runs.
- **A gesture is not an operation.** A tool changes the document while the cursor moves, and
  records the gesture in `Document::history()` when it ends.
- **A frame is not a change.** `Document::goTo()` moves the current frame and poses the scene,
  and records nothing.

Opening, saving, the autosave and recovery are on the document too. What asks the user first,
the file chooser and the save prompt, stays in `Controller`.

## Undo

`v3d::editor::Command` is a record of a change that has **already been made**. It has
`undo()`, `redo()` and `name()`, and no `execute()`. `CommandStack::push` stores a command and
never applies it.

**One gesture is one command.** A manipulator drag changes the meshes on every motion event so
the user sees them move. `TransformTool` records one `TransformCommand` when the drag ends,
holding each moved item's `Placement` (translation, rotation and scale) from the start and the
end of the gesture. Undo puts every whole starting placement back, so one undo restores a whole
set.

A gesture on components records one `VertexCommand` instead. It holds each moved vertex's
position, and each recomputed face normal, from the start and the end of the gesture. Undo and
redo write the stored values back rather than applying an inverse motion, so they restore the
mesh exactly. A gesture that moved no vertex records nothing.

The rules:

- **The first do goes through `redo()`** where the change is not interactive.
  `Document::create()` builds a `CreateCommand`, calls `redo()` to add the item, and then pushes
  it. Creating and redoing use the same code. A `CreateCommand` takes an item of any kind.
- **Every path out of a drag records it.** `TransformTool::commit()` runs on release and on a
  mode change. It pushes only if the placement actually changed, so a handle grabbed and
  released without moving records nothing.
- **A command holds the mesh, not its id.** A mesh removed by an undo stays alive inside the
  command and returns with the same id. Later commands in the history still refer to it.
- **Selection is not history.** Undo does not restore what was selected. `CreateCommand::undo`
  clears the selection flags of the item it removes, and leaves any other selected item selected.
  `CreateCommand::redo` selects the new item and nothing else.
- **The stack has a capacity**, 64 by default. When it is full, the oldest command is dropped.
  An item held by a dropped command is freed then.
- **A new push discards the redo side.**
- **Opening a project clears the history**, so an open cannot be undone. It also cancels any
  drag under way without recording it.

**A topology change is undone from snapshots.** An operation that changes which parts a mesh
holds records a `TopologyCommand`. It holds the mesh's arrays from before the operation and from
after it, and undo and redo put one back whole, so the numbering comes back exactly. A
`TransformCommand` or `VertexCommand` deeper in the history therefore still names the right parts
when it is undone. Undo and redo of a topology change clear that mesh's component selection,
because after a renumbering an old flag could name another part. The object stays selected.

Background: [ADR-0098](../adr/0098-editor-topology-changes-undone-from-snapshots.md)

Background: [ADR-0016](../adr/0016-editor-undo-records-completed-changes.md)

### The saved position

`CommandStack` also records the position the history was at when the project was last saved.
The project is dirty at any other position, and `CommandStack::dirty()` says so.

- **Save and Save As call `markSaved()`** when the write succeeds. A failed write leaves the
  project dirty.
- **Opening a project clears the history**, and the empty history counts as saved.
- **An undo or a redo can reach the saved position again**, and the project is then clean.
- **A saved position can be lost.** A push after an undo past it discards the commands it lay
  among, and the oldest command dropped from a full stack can take it with it. A lost position is
  never reached again, so the project stays dirty until the next save.
- **The caption follows the flag.** It is refreshed after every command, from the same place
  `syncUi()` runs.

## Commands

A command is anything the user can ask the editor to do: create a cube, change the select mask,
undo, quit. It is identified by a context and a name written together as `"context::name"`, for
example `create::poly::cube` or `transform::rotate`. This is the string `event::Event::str()`
returns.

**The registry.** The engine's `event::Commands` maps each command string to a handler, and
runs the handler each command names. `Controller::registerCommands()` registers every handler
from the controller's constructor:

- `addPress(name, handler)` registers a handler that runs on the press only. A release of the
  same binding is still accepted, and does nothing, and so is a held key's repeat, so holding a
  key runs its action once.
- `add(name, handler)` registers a handler that receives the whole event, so it can tell a
  press from a release. The camera modifiers use it, because a camera mode lasts while its key
  is held.

Each handler calls `syncUi()` after it runs. That sets the check marks on menu items and
toolbar toggles from the editor's state: the active view's show flags, the select mask and the
transform mode.

**How a command arrives.** Three sources produce the same `event::Event`, so all three reach
the same handler:

- **A key or mouse binding** in `data/mappings.json`. The `source` names the key; the
  `destination` names the command's context and name.
- **A menu item** in `data/vgui.json`, with `context` and `command` fields.
- **A toolbar button** in `data/vgui.json`, with the same two fields.

**The order of events.** The engine handles each SDL event in this order:

1. `Controller::onEvent()` receives the raw SDL event first. The editor passes keyboard events
   to `ui::shell::Keyboard`, so a focused text box (such as the file chooser's name box) takes
   its keys. An event it takes never reaches the bindings.
2. A key or button is published as an `event::Source` to every listener on `sink<Source>`. The
   editor does not listen there.
3. Unless a listener consumed the source, the event router sends the commands bound to it on
   `sink<Event>`. The engine listens there and runs the handler the command names, so the
   editor receives commands and never raw keys.

Mouse motion and window resizes reach the editor as `event::kind::MouseMotion` and
`event::kind::WindowResize` on their own sinks, without bindings.

**Rules.**

- An unregistered command is logged as a warning: "nothing handles ...". A menu item for a
  feature that does not exist reports itself this way.
- A second registration of the same name is refused, and the first handler stays. `start()`
  logs it as an error.
- The editor suite checks that every binding in `data/mappings.json` names a registered
  command.
- Startup logs how many commands are registered and how many of them are on a menu.
- Command names follow the editor's original UI definition, so menu items and bindings use the
  same strings. `ui::quit` uses the `ui` context, as every app's application-level commands do,
  and the engine handles it.
  `edit::undo` and `edit::redo` were added for the history.

**Adding a command:**

1. If it changes the document, add an operation to `Document` that applies itself, records
   itself and returns an `Outcome`.
2. Register a handler in `Controller::registerCommands()` with `press()` or `hold()`. A handler
   that runs an operation passes it what it acts on, and reports its `Outcome`.
3. Bind it in `data/mappings.json`, add it to a menu or toolbar in `data/vgui.json`, or both,
   with the same context and name.
4. If it describes on/off state, mark it in `syncUi()`.

Playback, the Render menu and the mouse each have a controller in `src/control/`: `Playback`,
`RenderWindow` and `Pointer`. A command for one of them calls it.

## Tools

`v3d::editor::Tool` is the interface for something that holds a gesture of the mouse open
across events: `activate(name)` and `deactivate(name)`, `view(view)`, `takes(position)`,
`dragging()`, `motion(position)` and `button(button, pressed, position)`. `CameraControlTool`,
`SelectTool` and `TransformTool` implement it. `Tool` lives in the editor because no other app
holds a gesture open across events.

**`ToolRouter` gives the primary button to one tool at a time.** A press is offered to each tool
in the order they were added, and the first whose `takes()` says yes has the gesture. The
release goes to that tool alone. Every tool hears every motion and every change of view. The
editor adds the camera tool, then the transform tool, then the select tool, so a new tool has a
slot in that order. [PickingAndManipulators.md](PickingAndManipulators.md#picking) says what
each one takes.

`CommandStack` and `Tool` are in the editor rather than the api. No other app has a document
to undo.

Background: [ADR-0081](../adr/0081-input-key-events-and-commands-are-separate.md),
[ADR-0090](../adr/0090-input-commands-are-handled-through-one-registry.md)
