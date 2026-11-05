# Menus

Every menu in the editor, and what each item does. A key in the Key column runs the item too.
[Bindings.md](Bindings.md) lists every key.

While a menu is open, the arrow keys move through it. A check mark beside an item says that its
setting is on, or that it is the choice made.

A menu item that acts on a view acts on the view the cursor was last over. An item that acts on
an area acts on the area the cursor was last over.

## The menu bar

The menu bar runs along the top of the window. It holds the Vertical3D menu, then File, Edit,
Create, Select, Modify, View, Animate, Render, Window and Help. After the menus come a tab for
each workspace, the open project's name, and the search. An amber dot follows the name while
the project has unsaved changes. The search shows a magnifier, **Search commands** and its key,
or only the magnifier and the key when the window is narrow.

### Vertical3D

The first menu shows the editor's mark.

| Item | Key | What it does |
|---|---|---|
| Search Commands | F3 | Opens the command search. [Commands.md](Commands.md#search-for-a-command) |
| Quit | Escape | Closes the editor. Asks first if the project has unsaved changes |

### File

| Item | Key | What it does |
|---|---|---|
| Open | Ctrl+O | Opens a project by its `project.json`. Asks first if the project has unsaved changes |
| Save | Ctrl+S | Saves the open project |
| Save As | | Saves the scene as a new project, in a new directory, and makes it the open project |
| Import > glTF | | Adds the scene of a `.gltf` or `.glb` file |
| Import > USD | | Adds the scene of a `.usd`, `.usda`, `.usdc` or `.usdz` file |
| Export > RIB | | Writes the scene as `export.rib` beside the program |
| Quit | Escape | Closes the editor, as on the Vertical3D menu |

[Projects.md](Projects.md) describes each of these.

### Edit

| Item | Key | What it does |
|---|---|---|
| Undo | Ctrl+Z | Takes back the last change |
| Redo | Ctrl+Y, Ctrl+Shift+Z | Makes the change undo took back |
| Hide | Ctrl+H | Hides the selected items from the views |
| Show All | Ctrl+Shift+H | Shows every hidden item in the views |
| Isolate | Alt+H | Shows only the selected items, the items above them and what is under them |

### Create

Each item makes a new item, selects it, and selects nothing else. One undo removes it.

| Item | Key | What it does |
|---|---|---|
| Poly > Cube | 1 | Adds a cube at the centre of the scene |
| Poly > Plane | 2 | Adds a plane at the centre of the scene |
| Poly > Cylinder | 3 | Adds a cylinder with no caps at the centre of the scene |
| Poly > Cone | 4 | Adds a cone with no cap at the centre of the scene |
| Curve > Bezier, NURBS, Catmull-Rom, Poly | | Adds a curve of that type at the centre of the scene |
| Camera > Perspective, Orthographic | | Adds a camera at the view's eye, looking the way the view looks |
| Light > Point, Distant, Spot | | Adds a light at the view's eye, shining the way the view looks |
| Group | | Puts the selected items under a new group |

[Modelling.md](Modelling.md#add-a-primitive) and [Scene.md](Scene.md) describe them.

### Select

Each item is a select mask. The one in use is checked. [Modelling.md](Modelling.md) describes
selecting.

| Item | Key | A click selects |
|---|---|---|
| Object | 5 | Any item |
| Mesh | | A mesh |
| Curve | | A curve |
| Camera | | A camera |
| Light | | A light |
| Vertex | 6 | A vertex of a selected mesh, or a control point of a selected curve |
| Edge | 7 | An edge of a selected mesh |
| Face | 8 | A face of a selected mesh |
| Handle | | A handle of a Bezier point on a selected curve |

### Modify

| Item | Key | What it does |
|---|---|---|
| Select | Q | Shows no manipulator. A click only selects |
| Move | W | Shows the Move manipulator |
| Rotate | E | Shows the Rotate manipulator |
| Scale | R | Shows the Scale manipulator |
| Extrude Face | | Extrudes the selected faces. Needs the Face mask |
| Inset Face | I | Insets each selected face. Needs the Face mask |
| Split Face | | Joins two selected vertices of one face with an edge. Needs the Vertex mask |
| Split Edge | | Splits each selected edge at its middle. Needs the Edge mask |
| Merge Vertices | J | Merges the selected vertices into one. Needs the Vertex mask |

[Modelling.md](Modelling.md#change-a-meshs-shape) describes the operations.

### View

Every item acts on one view, and each view keeps its own settings.
[Views.md](Views.md) describes them.

| Item | Key | What it does |
|---|---|---|
| Camera > Front, Back, Left, Right, Top, Bottom, Perspective | | Shows that fixed view |
| Camera > Orthographic | P | Switches the view between orthographic and perspective |
| Shading > Flat, Smooth | | Lights each face evenly, or blends the light across faces |
| Shading > Front Facing > Points, Lines, Fill | | Draws the outward side of faces as corners, edges, or filled |
| Shading > Back Facing > Points, Lines, Fill | | Draws the inward side of faces as corners, edges, or filled |
| Shading > Cull > None, Front, Back, Both | | Leaves out no side of the faces, the outward side, the inward side, or both |
| Show > Mesh | M | Shows or hides meshes |
| Show > Camera | | Shows or hides cameras |
| Show > Light | | Shows or hides lights |
| Show > Handle | H | Shows or hides the manipulator and a curve's Bezier handles |
| Show > Grid | G | Shows or hides the construction grid |
| Navigate > Truck, Dolly, Zoom, Orbit, Pitch | | Makes a left drag with no key held move the camera that way, until the item is chosen again or a manipulator is chosen |

### Animate

[Animation.md](Animation.md) describes animating.

| Item | Key | What it does |
|---|---|---|
| Set Key | | Keys every property of each selected item at the current frame |
| Auto Key | | Turns auto key on or off. Checked while it is on |
| Delete Keys | Delete | Deletes the keys chosen in the timeline |
| Play | Space | Plays the frame range at the frame rate, starting again after the last frame |
| Stop | Space | Stops playback |
| Next Frame, Previous Frame | | Steps one frame, staying within the range |
| First Frame, Last Frame | Shift+Left, Shift+Right | Goes to the start or the end of the range |
| Next Key, Previous Key | Up, Down | Goes to the nearest keyed frame after or before the current one |
| First Key, Last Key | | Goes to the earliest or the latest keyed frame |

The key items use the keys of the selected items, or of every item when nothing is selected.

### Render

[Rendering.md](Rendering.md) describes rendering.

| Item | Key | What it does |
|---|---|---|
| Render | | Renders the current frame with moya, and shows the render view |
| Batch Render | | Renders every frame of the range from each scene camera into the project's `renders` directory |
| Stop | | Stops the render and the batch |
| Settings | | Shows the render settings |

### Window

| Item | Key | What it does |
|---|---|---|
| Next Workspace | Ctrl+Page Down | Shows the next workspace, wrapping after the last |
| Previous Workspace | Ctrl+Page Up | Shows the workspace before, wrapping before the first |
| Save Workspace | | Keeps the arrangement shown as the workspace's own |
| Reset Workspace | | Puts the workspace back as the editor comes with it |
| Export Workspace | | Writes the workspace shown to a `.json` file |
| Import Workspace | | Reads the workspaces in a `.json` file, and shows the first |
| Maximise Area | Ctrl+Space | Gives the active area the room of every area, or gives every area its room back |
| Statistics | F12 | Shows or hides the frame statistics |

[Workspace.md](Workspace.md) describes workspaces.

### Help

| Item | Key | What it does |
|---|---|---|
| Search Commands | F3 | Opens the command search |

## The tool shelf

The tool shelf runs down the left of the window. Each button shows a tooltip when the cursor
rests on it. A rule sets the manipulators apart from the modelling operations under them.

| Button | Same as |
|---|---|
| Select, Move, Rotate, Scale | The Modify menu's items of those names. The one in use is checked |
| Extrude Face, Inset Face, Split Face, Split Edge, Merge Vertices | The Modify menu's items of those names |

## An area's header

Each area has a header of menus along its top. The first menu is named for the editor the area
shows. [Workspace.md](Workspace.md#use-an-areas-header) lists its items.

A viewport's header has four more menus. Each acts on that view, whatever view is active.

| Menu | Named for | Items |
|---|---|---|
| Camera | The camera the view shows, such as Front | The View menu's Camera items |
| Shading | How the view draws: Flat, Smooth, Wireframe or Points | The View menu's Shading items |
| Mask | The select mask in use, such as Object | The Select menu's items |
| Overlays | Shown as an icon | Grid, Meshes, Cameras, Lights and Handles, as on the View menu's Show submenu |

The name of the camera menu is set on a pill while its area is active.
