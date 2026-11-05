# Concepts

This page explains the words the rest of the manual uses. Each section defines one idea and
says how it shows in the editor.

## Projects and scenes

A **project** is a directory on disk. It holds a file named `project.json`, the scene, and the
pictures a batch render writes. You open a project by choosing its `project.json`.

The **scene** is everything you model: the items, their keys, the render settings and the unit
of length. A project holds one scene, and saving the project saves it.

The editor always has one project open. When it starts, that project is a directory named
`project` in a `Vertical3D` folder in your documents, and it starts empty. The window's caption
names the open project, and ends in ` *` while it has changes you have not saved.

## Items

An **item** is one thing in the scene. There are five kinds:

| Kind | What it is |
|---|---|
| Mesh | A surface made of faces. A face is a flat polygon bounded by edges, and edges meet at vertices |
| Curve | One or more curves of one type: poly, Bezier, Catmull-Rom or NURBS |
| Camera | A point of view a render can be made from, perspective or orthographic |
| Light | A point, distant or spot light |
| Group | An empty item that other items are placed under |

Every item has a name, a translation, a rotation and a scale. No two items in a scene share a
name.

An item can be placed under another item, its **parent**. It then moves, turns and scales with
its parent. The items under a parent are its **children**. In the editor, Create > Group is how
you give items a parent. An imported file can bring its own parents with it.

## Selection and select masks

The **selection** is what the next command acts on. You select by clicking in a view, by
dragging a rectangle in a view, or by clicking a row in the outliner.

The **select mask** decides what a click selects. There are nine masks, in two sets:

- **Object, Mesh, Curve, Camera and Light** select whole items. Object selects any kind of item.
  Each of the others selects only its own kind.
- **Vertex, Edge, Face and Handle** select parts of items. Vertex, Edge and Face select the
  parts of a mesh, and Vertex also selects a curve's control points. Handle selects the handles
  of a Bezier curve. These parts are called **components**.

You select an item before you can select its components. Changing the mask clears the selected
components but keeps the selected items.

## Manipulators

A **manipulator** is a set of handles drawn over the selection. You drag a handle to move,
turn or resize what is selected. There are three: Move, Rotate and Scale. A fourth mode,
Select, shows no handles. The red handle works along the X axis, the green along Y, and the blue
along Z.

## Undo

Each change you make is one step of the **history**. Undo takes back the last step, and redo
makes it again. A drag of a handle is one step, however long it lasts. Selecting is not a
change, so undo does not change what is selected.

## Areas, editors and workspaces

The window below the menu bar is divided into **areas**. Each area shows one **editor**. These
are the editors:

| Editor | What it shows |
|---|---|
| Viewport | The scene, through a camera of its own |
| Outliner | The scene's items as a tree |
| Properties | The selected item's name and values |
| Timeline | The frames, and the keys of the selected items |
| Graph Editor | The animation curves of the selected items |
| Console | A line for typing scripts, and a list of what has run |
| Render View | The latest picture moya rendered |
| Render Settings | The size, samples and hider of a render |

An area can hold several editors as **tabs**, and shows one at a time.

A **workspace** is a named arrangement of areas. The editor comes with five: Modelling,
Perspective, Animation, Shading and Rendering. A tab for each sits at the top of the window.

The **active area** is the one the cursor was last over. A viewport in it is the **active
view**. A key such as P acts on the active view. A menu item acts on the view the cursor was
over before you moved to the menu.

## Frames, keys and curves

Animation happens over **frames**. The timeline has a frame rate and a range of frames that
plays. A new scene runs at 24 frames a second over frames 1 to 120.

A **key** fixes the value of one property at one frame. The properties that can be keyed are
the translation, rotation and scale of any item, a camera's lens, and a light's emission. The
keys of one property make a **curve**. Between keys, the curve gives the value in between. The
graph editor draws each curve.

A property with a curve shows the curve's value at the current frame. If you change a keyed
property and then change the frame, the property goes back to its curve. Your change is kept
only if you key it first, or if **auto key** is on. Auto key keys each change as you make it.

## Notices

When a command finishes, a **notice** appears for a few seconds at the foot of the active area.
A green check says the command was done. A red cross says it was refused, and the text says why.
A change you can undo ends with "Ctrl+Z to undo".
