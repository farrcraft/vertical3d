# Commands and undo

How a menu item, toolbar button or key reaches the code that handles it, and how a change is
recorded so it can be undone.

## Undo

`v3d::editor::Command` is a record of a change that has **already been made**. It has
`undo()`, `redo()` and `name()`, and no `execute()`. `CommandStack::push` stores a command and
never applies it.

**One gesture is one command.** A manipulator drag changes the mesh on every motion event so
the user sees it move. `TransformTool` records one `TransformCommand` when the drag ends,
holding the mesh's `Placement` (translation, rotation and scale) from the start and the end of
the gesture. Undo puts the whole starting placement back.

The rules:

- **The first do goes through `redo()`** where the change is not interactive.
  `Controller::createPoly` builds a `CreateCommand`, calls `redo()` to add the mesh, and then
  pushes it. Creating and redoing use the same code.
- **Every path out of a drag records it.** `TransformTool::commit()` runs on release and on a
  mode change. It pushes only if the placement actually changed, so a handle grabbed and
  released without moving records nothing.
- **A command holds the mesh, not its id.** A mesh removed by an undo stays alive inside the
  command and returns with the same id. Later commands in the history still refer to it.
- **Selection is not history.** Undo does not restore what was selected. `CreateCommand::undo`
  clears the selection flag of the mesh it removes, so that at most one mesh stays selected.
- **The stack has a capacity**, 64 by default. When it is full, the oldest command is dropped.
  A mesh held by a dropped command is freed then.
- **A new push discards the redo side.**
- **Opening a project clears the history**, so an open cannot be undone. It also cancels any
  drag under way without recording it.

A future modelling operation that changes topology will need its own command type that records
the topology it changed. `TransformCommand` does not fit that case.

Background: [ADR-0016](../adr/0016-editor-undo-records-completed-changes.md)

## Commands

A command is anything the user can ask the editor to do: create a cube, change the select mask,
undo, quit. It is identified by a context and a name written together as `"context::name"`, for
example `create::poly::cube` or `transform::rotate`. This is the string `event::Event::str()`
returns.

**The directory.** `v3d::editor::CommandDirectory` maps each command string to a handler.
`Controller::registerCommands()` registers every handler at startup:

- `addPress(name, handler)` registers a handler that runs on the press only. A release of the
  same binding is still accepted, and does nothing.
- `add(name, handler)` registers a handler that receives the whole event, so it can tell a
  press from a release. The camera modifiers use it, because a camera mode lasts while its key
  is held.

`Controller::handleEvent` calls `invoke()` and does nothing else. After every command it calls
`syncUi()`, which sets the check marks on menu items and toolbar toggles from the editor's
state: the active view's show flags, the select mask and the transform mode.

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
3. Unless a listener consumed the source, the event engine sends the commands bound to it on
   `sink<Event>`. The editor's `handleEvent` listens there, so it receives commands and never
   raw keys.

Mouse motion and window resizes reach the editor as `event::kind::MouseMotion` and
`event::kind::WindowResize` on their own sinks, without bindings.

**Rules.**

- An unregistered command is logged as a warning: "no command is registered as ...". A menu
  item for a feature that does not exist reports itself this way.
- A second registration of the same name is refused, and the first handler stays. The
  controller logs it as an error.
- Startup logs how many commands are registered and how many of them are on a menu.
- Command names follow the editor's original UI definition, so menu items and bindings use the
  same strings. `ui::quit` uses the `ui` context, as every app's application-level commands do.
  `edit::undo` and `edit::redo` were added for the history.

**Adding a command:**

1. Register a handler in `Controller::registerCommands()` with `press()` or `hold()`.
2. Bind it in `data/mappings.json`, add it to a menu or toolbar in `data/vgui.json`, or both,
   with the same context and name.
3. If it describes on/off state, mark it in `syncUi()`.

**Tools.** `v3d::editor::Tool` is the interface for something that keeps receiving mouse input
while it is active: `activate(name)`, `deactivate(name)`, `motion(position)` and
`button(button, pressed, position)`. `CameraControlTool`, `SelectTool` and `TransformTool`
implement it. `Tool` lives in the editor because no other app holds a gesture open across
events.

`CommandDirectory`, `CommandStack` and `Tool` are all in the editor rather than the api. No
other app has a document to undo or several ways to invoke one command.

Background: [ADR-0081](../adr/0081-input-key-events-and-commands-are-separate.md)
