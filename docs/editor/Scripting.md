# Scripting

How a script names the commands it runs and what they act on, how the editor lists what was
run, and what runs a script. Read
[CommandsAndUndo.md](CommandsAndUndo.md) first for how a command is registered and reached from
a menu or a key.

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
as a notice, as it does from a menu, and `run()` returns that notice's text as its refusal.

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
- **A console line** is listed as typed before what it ran. What a script printed or a line
  came to is listed as said, and the problem a script stopped on as failed.
- The line goes in before the command runs, and is written again with its filled arguments
  once it has, so what the command said follows it.
- A command that cannot be run as a script is not listed.
- The journal keeps the last 1000 entries, and `revision()` moves with every change to it.

## The console

An area can hold a console, the `console` editor, which `control::ConsoleEditor` draws in the
immediate layer. Shift+F4 shows one in the area under the cursor.

- **The list** is the journal, newest at the foot, above the line. A typed line is set after a
  `>` in the active text, a command in the secondary text, a refusal and a problem in red, and
  the rest in the ordinary text. An entry over lines takes a row for each. The wheel scrolls
  back through it, and an entry added brings it to the foot again.
- **The line** takes TypeScript. Return runs it through `app::Scripting`, empties it, and keeps
  the keyboard for the next line. A line of nothing but spaces runs nothing.
- **The arrows.** While a console's line holds the keyboard, the controller gives it the up and
  down arrows before the ui and the bindings. Up steps back through the lines run before, and
  down steps forward, past the newest to the line being written. Every console shares the lines
  run, and each keeps its own line being written and its own scroll.

## Running a script

A script is TypeScript, run by `v3dlib_script` as [api/Scripting.md](../api/Scripting.md)
describes: its types are stripped, and the JavaScript left runs on QuickJS-ng.

`app::Scripting` holds the editor's runtime. The controller builds it over the command table,
and binds it once every command is registered.

- **Each command is a function by its path.** `timeline.set({ items: ["Cube"], frame: 24 })`
  runs `timeline::set` as `CommandTable::run()` does. A command takes one object of arguments,
  or none. Anything else, or a refusal from `run()`, is an `Error` in the script with the
  refusal's text, so a script stops where a menu would show a notice, unless it catches it. A
  command's call comes to `undefined`.
- **`console.log`** lists what it is given in the journal, a string as it is and anything else
  as JSON, joined by spaces.
- **A console line** is listed in the journal as typed. Then come the commands it ran and what
  they said, then the value it came to, or the problem it stopped on as `line:column` and the
  message. Every line runs in the scope the lines before it left.
- **A startup script** is the file the command line names after `--script`.
  `Scripting::startup()` runs it once, at the end of the controller's `start()`, in the scope
  the console's lines share. A missing file name, a file that cannot be read, and a script that
  stops on a problem are logged, and `start()` fails. The editor then does not open on a scene
  the script did not finish. The command line is read as
  [api/engine/Lifecycle.md](../api/engine/Lifecycle.md#the-command-line) describes, and the
  editor reads `--script` from what the engine leaves.

**The declarations.** `data/scripts/editor.d.ts` declares every command a script can run, for
`tsc` or an author's editor to check a script against. `script::declarations()` writes it
from the command table: each command is a function in the namespace of its path, and takes an
optional object of its arguments, each optional. A boolean is `boolean`, a number or an
integer is `number`, text and an item are `string`, and items and parts are `string[]`. The
editor suite checks the shipped file against the commands, so a command added or changed means
writing the file again.

**The examples.** `data/scripts/examples/` holds scripts that show how one is written: keying
items, making primitives and taking one back, and catching a refusal. They ship with the editor.
CI checks each against `editor.d.ts`, and the editor suite runs each on a document that holds a
cube. An example fails there when a problem stops it, or when it runs no command, so an example
that no longer works does not ship.

Background: [ADR-0153](../adr/0153-editor-scripts-are-typescript-run-on-quickjs.md)
