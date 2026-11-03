# Keys and mouse bindings

Every key and mouse binding in the editor. The editor reads them from
[mappings.json](../../../vertical3d/data/mappings.json) in its `data` directory.

A key that acts on a view or an area acts on the one under the cursor. While a text field holds
the keyboard, such as the outliner's Filter or the console's line, keys go to the field and run
no binding.

The Alt, Ctrl and Shift bindings that are held are the left-hand keys only.

## Keys

| Key | Menu item | What it does |
|---|---|---|
| 1 | Create > Poly > Cube | Adds a cube |
| 2 | Create > Poly > Plane | Adds a plane |
| 3 | Create > Poly > Cylinder | Adds a cylinder |
| 4 | Create > Poly > Cone | Adds a cone |
| 5 | Select > Object | A click selects any item |
| 6 | Select > Vertex | A click selects a vertex or a control point |
| 7 | Select > Edge | A click selects an edge |
| 8 | Select > Face | A click selects a face |
| Q | Modify > Select | Shows no manipulator |
| W | Modify > Move | Shows the Move manipulator |
| E | Modify > Rotate | Shows the Rotate manipulator |
| R | Modify > Scale | Shows the Scale manipulator |
| I | Modify > Inset Face | Insets the selected faces |
| J | Modify > Merge Vertices | Merges the selected vertices |
| G | View > Show > Grid | Shows or hides the grid in the view |
| M | View > Show > Mesh | Shows or hides meshes in the view |
| H | View > Show > Handle | Shows or hides handles in the view |
| P | View > Camera > Orthographic | Switches the view between orthographic and perspective |
| Delete | Animate > Delete Keys | Deletes the keys chosen in the timeline |
| F3 | Help > Search Commands | Opens the command search |
| F12 | Window > Statistics | Shows or hides the frame statistics |
| Shift+F4 | | Shows a console in the area |
| Shift+F5 | | Shows a viewport in the area |
| Shift+F6 | | Shows a graph editor in the area |
| Shift+F7 | | Shows a properties editor in the area |
| Shift+F9 | | Shows an outliner in the area |
| Shift+F10 | | Shows the render settings in the area |
| Shift+F11 | | Shows a render view in the area |
| Shift+F12 | | Shows a timeline in the area |
| Ctrl+Z | Edit > Undo | Takes back the last change |
| Ctrl+Y, Ctrl+Shift+Z | Edit > Redo | Makes the change undo took back |
| Ctrl+O | File > Open | Opens a project |
| Ctrl+S | File > Save | Saves the open project |
| Ctrl+Space | Window > Maximise Area | Maximises the area, or gives every area its room back |
| Ctrl+Page Down | Window > Next Workspace | Shows the next workspace |
| Ctrl+Page Up | Window > Previous Workspace | Shows the previous workspace |
| Escape | File > Quit | Quits, asking first if the project has unsaved changes |

Escape does something else in two cases. While the command search is open, it closes the
search. While a divider is being dragged, it puts the divider back.

## Keys held with the mouse

| Hold | With | What it does |
|---|---|---|
| Left Shift | A left click, or a drag from empty space | Adds to the selection, or takes the clicked item or part out of it |
| Left Alt | A left drag in a view | Dollies a perspective view, or zooms an orthographic one |
| Left Ctrl | A left drag in a view | Trucks the view sideways or up and down |
| Ctrl | A drag of a divider | Snaps the divider to tenths of the space |

Left Ctrl starts a truck only during a drag. A Ctrl key such as Ctrl+Z still works while the
cursor is over a view.

## The mouse

| Input | Where | What it does |
|---|---|---|
| Left click | A view | Selects what is under the cursor, or nothing |
| Left drag on a handle | A view | Moves, turns or scales the selection |
| Left drag from empty space | A view | Draws a rectangle, and selects what is inside it |
| Left drag | A view, with a move chosen on View > Navigate | Moves the camera that way |
| Left click on a ball | A perspective view's navigation gizmo | Looks along that axis |
| Middle drag | A perspective view | Orbits the camera |
| Left click | A row of the outliner | Selects its item |
| Left drag | A divider between areas | Resizes the areas either side |
| Double click | A divider | Gives the areas either side the same room |
| Right click | While dragging a divider | Puts the divider back |
| Left drag | A tab | Moves its editor to another area, or to a new area at an edge |
| Left drag | A key in the timeline or the graph editor | Moves the key |
| Left click or drag | The ruler of the timeline or the graph editor | Sets the frame |
| Left Ctrl and drag | The graph editor | Slides the graph |
| Left Alt and drag | The graph editor | Shows more or fewer frames and values |
| Wheel | The graph editor | Zooms about the cursor |
| Wheel | The console's list | Scrolls back through what has run |

## In the command search

| Key | What it does |
|---|---|
| Up, Down | Chooses the row above or below, wrapping at either end |
| Enter | Runs the chosen command and closes the search |
| Escape | Closes the search and runs nothing |

## In the console

| Key | What it does |
|---|---|
| Return | Runs the line |
| Up, Down | Steps back or forward through the lines run before |
