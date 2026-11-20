# Editor Scripting: Scenes a Script Can Finish

What the editor needs so that a script can build a whole scene and render it, with or without a
window, and in what order. It is written for whoever picks up the editor's scripting next.

Every section has been started. [EditorScripting](../plans/EditorScripting.md) is their plan.

A script can already create primitives, select, key and undo, as
[Scripting.md](../editor/Scripting.md) describes. It cannot place what it creates, cannot choose
how a key interpolates, and runs only once the editor's window is open. Every item also turns in
one fixed order, which a script that sets a rotation would have to work around.

## What it needs

### 1. Rotation order

Every item turns about x, then y, then z. An item that locks in that order cannot be keyed out of
it: a quarter turn about y locks x and z together, and `eulerNear()` then takes z as 0. Each item
needs an order of its own. The order is saved with the item, and `eulerNear()`, `fromEuler()`,
the properties editor and every keyed rotation use it.

- The item's quaternion stays the rotation it holds. The order says only how the three keyed
  angles become that quaternion, and how a quaternion becomes angles again.
- A change of order is undoable. It can keep the item where it is and re-key its rotation in
  the new order, or keep the keys and move the item. Which one it does is a decision for an ADR.
- The order is a new field in every kind's payload, and so a new version of every kind, with a
  step that migrates version 1 to the order the editor uses today.
- glTF holds rotations as quaternions, and the RIB export writes each item's whole matrix, so
  neither needs the order.

### 2. Shared meshes in a scene document

Two mesh items may share one `BRep`, but the scene store writes each item's geometry in full,
so they read back as two copies. Keeping them shared needs the geometry as a record of its own,
which the mesh items refer to by id, and a new mesh version.

This does not depend on section 1. It is here because both change what the document stores, and
one round of codec and migration work can take both.

### 3. Placing an item by value

No command sets an item's translation, rotation or scale to a value. A script that builds a
scene leaves every item where its create command put it. The command takes items and any of the
three, sets them as one change through the history, and refuses an item that cannot take them.

- A rotation is given as angles in degrees, in the item's own order from section 1, so the
  numbers a script passes are the numbers the properties editor shows.
- Whether the values are in the item's parent's space or the scene's space is part of the
  command's design, as the manipulators' Global and Local spaces are part of theirs.
- With auto key on, the command keys what it set through `command::Keyed::record()`, as an edit
  in the properties editor does.

### 4. A key's interpolation

The graph editor draws step, linear and Bezier keys, and turns a key's slopes, but no command
sets a key's interpolation. A key made in the editor is always Bezier, and only an imported key
can be step or linear.

- A command sets the interpolation of the graph editor's chosen key, and of keys named by item,
  path and frame, so a script can call it too.
- New keys take an interpolation of their own, from a setting `timeline::set` and auto key both
  read.

### 5. Running without a window

`--script` runs a TypeScript file once the editor has started, in its window. A headless mode
opens the project, runs the script against it and quits, with no window or device, so a script
can batch edits, exports and renders on a machine with no GPU.

- The engine starts an app with no window when the app does not ask for `Feature::Window`. The
  editor asks for it today, and has to choose its features from its command line.
- `app::Context` holds the views, the tools and the render view, and every command reads what it
  needs from it. Each command that works on the document has to run with none of the views
  there, and each that needs a view has to refuse rather than fail.
- `RenderView` owns the batch. The batch moves to where a headless run can reach it.
- The process exits with a failure when the script stops on a problem, so a build or a farm can
  tell.

### 6. Batches in parallel

A batch renders one frame at a time. Its frames are independent files, and could run as several
moya.exe processes at once. The number of processes is a setting of the batch. Stop stops all
of them, and the status counts frames from every process.

moya's CPU device takes every thread by default, so a process for each core would give each
process every thread. moya.exe therefore needs an option for its thread count, and the batch
gives each process a share. A GPU is held by one process, so frames on a GPU share it rather
than run beside each other.

## The order

Section 1 comes before section 3, because section 3's rotation is given in the item's order.
Section 2 goes with section 1, so the document changes once. Section 4 can go at any point.
Section 5 comes after sections 3 and 4, so a headless script can do all a windowed one can.
Section 6 comes last, because a headless batch on a build machine is where it pays most.

## Sources

- Blender's rotation modes:
  <https://docs.blender.org/api/current/bpy.types.Object.html#bpy.types.Object.rotation_mode>
- USD's rotation orders: <https://openusd.org/release/api/class_usd_geom_xform_common_a_p_i.html>
- Blender in the background with a script:
  <https://docs.blender.org/manual/en/latest/advanced/command_line/arguments.html>
- Houdini's hython: <https://www.sidefx.com/docs/houdini/hom/hython.html>
