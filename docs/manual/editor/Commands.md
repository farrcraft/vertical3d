# Running commands by name

How to find a menu command by typing its name, and how to run commands as a script in the
console. A **command** is one action the editor can take, such as making a cube or setting a
key. Each menu item and each key runs one.

## Search for a command

1. Press F3, or click **Search** at the right end of the top bar. **Search Commands** on the
   Vertical3D menu and the Help menu does the same.
2. Type words from the command's menu path, such as `cube` or `poly cone`.
3. Press the up and down arrows to choose a row, then press Enter. Or click the row.

The list holds every item of the menu bar, and of the active area's header, that runs a command.
Each row shows the item's menu path, such as `Create > Poly > Cube`, and its key if it has one.

Each word you type has to be in the path, in any case. Rows whose own name starts with the
first word come first. An empty search lists every command. Press Escape to close the search
without running anything.

A command run from the search acts on the active area, as its key would.

## Open the console

Move the cursor over an area and press Shift+F4. The Animation workspace also has a console
as a tab along its foot.

The console has a list of what has run, newest at the foot, and a line at the bottom for typing.
Turn the mouse wheel over the list to scroll back.

## Run a script line

1. Click the line that reads "Type a script, and press Return".
2. Type a line of TypeScript, such as `create.poly.cube()`.
3. Press Return.

The line runs, and the list shows it, followed by each command it ran and what each said. The
up and down arrows step back and forward through the lines you have run. Each line runs where
the lines before it left off, so a name you declare stays for the next line.

## Name a command in a script

A command's script name is its name with dots between the parts. Each one is a function:

| Menu item | Script |
|---|---|
| Create > Poly > Cube | `create.poly.cube()` |
| Create > Group | `create.group({ items: ["Cube", "Cone"] })` |
| Animate > Set Key | `timeline.set({ items: ["Cube"], frame: 24 })` |
| Animate > Auto Key | `timeline.auto_key({ on: true })` |
| Modify > Extrude Face | `transform.poly.extrude_face({ parts: ["Cube.f[2]"] })` |
| Edit > Undo | `edit.undo()` |
| File > Save | `project.save()` |

A command that takes arguments takes one object, and each argument can be left out. One left
out is taken from the editor as it is: the selected items, the selected parts, or the current
frame. A toggle such as Auto Key flips when `on` is left out.

The console lists each command you run from a menu or a key as the script line that does the
same thing. Copy a line from the list to learn a command's script name and arguments.

The file `data/scripts/editor.d.ts`, beside the program, declares every command a script can
run.

## Name items and parts

- **An item** is named by its name, as the outliner shows it: `"Cube"`.
- **Parts** are named by the item's name, a dot, a letter and an index in brackets, as in
  `"Cube.f[2]"`. The letter is `v` for a vertex or a control point, `e` for an edge, `f` for a
  face and `h` for a Bezier handle. A run of parts gives the first and the last index, as in
  `"Cube.v[0:3]"`.
- **A frame** is a whole number.

A modelling command given parts selects those parts first, whatever the select mask, and runs
on them. The items that hold them are selected, and nothing else.

## When a command refuses

A command that cannot do what it was asked stops the script with an error. The error's message
is the text the notice would show. Catch the error to go on:

```ts
try {
    timeline.set({ items: ["Sphere"], frame: 1 });
} catch (error) {
    console.log("not keyed:", (error as Error).message);
}
```

`console.log` writes what it is given into the console's list.

## Example scripts

The directory `data/scripts/examples/` beside the program holds three scripts to read and copy
from:

| File | What it does |
|---|---|
| `keyframes.ts` | Keys the item named Cube at every twelfth frame from 1 to 49 |
| `primitives.ts` | Makes a plane, a cylinder and a cone, then undoes the cone |
| `refusals.ts` | Keys a list of items, and catches the refusal for each item the scene lacks |

The console runs one line at a time. To run an example, type its statements into the console's
line. A statement such as a loop has to fit on one line.
