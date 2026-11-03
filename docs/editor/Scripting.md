# Scripting

How a script names the commands it runs and what they act on, and how the editor lists what
was run. Read [CommandsAndUndo.md](CommandsAndUndo.md) first for how a command is registered and
reached from a menu or a key.

## Commands a script can run

A script runs the same commands a menu or a key does, by the same name. Its path is the
command's name with each `::` a dot, so `create::poly::cube` is `create.poly.cube()`.

**Each command declares what it takes.** A command a script can run is registered through
`CommandTable::script()`, with a `script::Parameter` for each argument it takes, by name and
type. A menu or a key runs it with nothing given. A script passes an object of arguments, and
may leave any of them out. The handler fills each one left out from the editor's state through
the `script::Arguments` it is given, so one handler serves both. A value filled is kept as if it
had been given.

| Type | A script writes | Filled from, typically |
|---|---|---|
| `Boolean` | `true` or `false` | the opposite of a toggle's state |
| `Number` | any finite number | |
| `Integer` | a whole number an `int` holds, without a point. A frame is one | the current frame |
| `Text` | a string | |
| `Item` | an item's name | |
| `Items` | an array of item names | the selected items |
| `Parts` | an array of parts, in the form below | the selected parts |

`CommandTable::run()` runs a command as a script does. It refuses, and runs nothing, when no
command a script can run has the name, when an argument names no parameter, or when a value is
not of its parameter's type. A command that runs and then cannot do what it was asked says so
as a notice, as it does from a menu.

The commands a script can run are those of `EditCommands`, `PlaybackCommands` and
`RenderCommands`, the select masks, the transform modes, and `project::save`. A command that
takes the whole event, such as a held camera move or an area's command, is not one, nor is a
dialog's button.

| Command | Takes |
|---|---|
| `timeline::set` | `items`, `frame` |
| `timeline::auto_key` | `on` |
| `create::group` | `items` |
| `transform::poly::split_edge`, `split_face`, `extrude_face`, `inset_face`, `merge_vertices` | `parts` |

Every other command a script can run takes nothing.

**A modelling operation and its parts.** From a menu or a key, a modelling operation needs the
select mask that selects the parts it works on, as
[Scene.md](Scene.md#modelling-operations) says, and runs on the selected parts. From a script it
selects the parts it names first, whatever the mask. The items that hold them are selected, and
nothing else of any item. Selection is not history, so undo leaves it as the script left it.

Background: [ADR-0152](../adr/0152-editor-scripts-call-named-commands-with-typed-arguments.md)

## Text forms

`script/Text.h` reads and writes what a script names.

- **An item** is named by its name, which is unique in its scene, as
  [Scene.md](Scene.md#the-scene) says. A name no item has is refused, saying which.
- **Parts** of one item, in a run of indices, are written as the item's name, a dot and a
  letter for the kind of part. The index, or the first and last of the run, follows in
  brackets: `Cube.f[2]`, `Cube.v[0:3]`. The letters are `v` for a vertex or a curve's control
  point, `e` for a half edge, `f` for a face and `h` for a curve's handle. The form is read from
  the end, so a name may hold dots and brackets, as `Cube.001.v[0:3]` does. Selected parts are
  written in rising runs, each kind of each item apart.
- **A frame** is a whole number.

Parts that cannot be selected are refused, and the selection is left as it was. They are a text
not in the form, an item that does not exist, a kind of part the item lacks, or an index past
its parts.

## The journal

`script::Journal` lists what the editor ran and what it said, in the order it happened. The
console shows it. `CommandTable::journal()` holds the editor's one journal, and the `Context`
points at it.

- **A command** a script can run is listed as the line that runs it again, whichever of a menu,
  a key or a script ran it, as `timeline.set({ items: ["Cube"], frame: 24 })`. The arguments
  are written in the order of the parameters, filled ones included, so running the line again
  acts on the same items at the same frame. A toggle is listed with the state it set, as
  `timeline.auto_key({ on: true })`, so the line sets it rather than flipping it.
- **A notice** follows the command that posted it, as done or refused. `Context::report()` and
  `Context::refuse()` list it.
- The line goes in before the command runs, and is written again with its filled arguments
  once it has, so what the command said follows it.
- A command that cannot be run as a script is not listed.
- The journal keeps the last 1000 entries, and `revision()` moves with every change to it.
