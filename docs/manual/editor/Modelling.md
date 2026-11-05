# Modelling

How to add meshes, select them and their parts, move them with the manipulators, and change
their shape with the modelling operations. [Concepts.md](Concepts.md) defines items,
components and select masks.

## Add a primitive

Choose **Create > Poly** and then **Cube**, **Plane**, **Cylinder** or **Cone**. The keys 1, 2,
3 and 4 do the same.

- The new mesh sits at the centre of the scene and is one unit across.
- The cylinder and the cone are open at the ends. They have no cap faces.
- The new mesh is selected, and nothing else is.
- One undo removes it.

The editor has no command that deletes an item. To take back an item you have just made, undo.

## Choose what a click selects

Choose a mask from the **Select** menu, or from the mask menu in a viewport's header. The mask
menu is named for the mask in use, such as Object. The keys 5, 6, 7 and 8 choose Object,
Vertex, Edge and Face.

| Mask | A click selects |
|---|---|
| Object | Any item |
| Mesh, Curve, Camera, Light | An item of that kind only. A light behind a mesh can be clicked under the Light mask |
| Vertex | A vertex of a selected mesh, or a control point of a selected curve |
| Edge | An edge of a selected mesh |
| Face | A face of a selected mesh |
| Handle | A handle of a Bezier point on a selected curve |

The mask is the same in every view. Choosing a new mask clears the selected components.

## Select items

With a mask that selects items, such as Object:

- **Click** an item to select it and nothing else.
- **Click empty space** to select nothing.
- **Hold Left Shift and click** an item to add it to the selection, or to take it out if it is
  selected already.
- **Drag a rectangle** from empty space to select every item wholly inside it. Hold Left Shift
  to add to the selection instead of replacing it. A rectangle never takes anything out.
- **Click a row in the outliner** to select its item. Left Shift works there too.

A camera or a light is selected by clicking the lines it is drawn with.

## Select components

1. Select the mesh or the curve with the Object mask.
2. Choose the Vertex, Edge or Face mask, or Handle for a Bezier curve's handles.
3. Click a part to select it and no other part.

The rules for items apply to parts as well. Left Shift adds or takes away, a rectangle selects
every part inside it, and a click on empty space clears the parts but keeps the items.

You can select components of several items at once. Select all the items first.

A click, and a rectangle, also find parts that other surfaces hide. A vertex or an edge counts
as clicked when it is within 5 pixels of the cursor.

## Move, rotate and scale

Choose a manipulator from the **Modify** menu or from the tool shelf down the left of the
window:

| Item | Key | Handles |
|---|---|---|
| Select | Q | None |
| Move | W | An arrow along each axis, and a centre handle |
| Rotate | E | A ring around each axis, and a centre handle |
| Scale | R | A handle along each axis, and a centre handle |

The handles sit at the centre of what is selected. For one item, that is its origin. For
several items, or for components, it is the average of their positions. The handle under the
cursor is drawn highlighted. Drag it:

- **Move.** An axis handle moves along its axis. The centre handle moves in the plane of the
  screen.
- **Rotate.** A ring turns about its axis as the cursor sweeps around the centre. The centre
  handle tumbles freely. A ring seen edge on cannot be grabbed.
- **Scale.** An axis handle scales along that axis of the item. The centre handle scales all
  three ways, and grows as you drag right or up. A scale never goes below 0.01, so a drag
  cannot mirror an item.

Several items move together about one centre. A rotate or a scale of several items also moves
each one around the centre.

What moves follows the mask. Under Object, the selected items move. Under Light, only the
selected lights move, and a selected mesh stays where it is. Under Vertex, Edge or Face, the
selected parts move, and the item itself stays. Moving a mesh's parts reshapes the mesh.

One drag is one step of the history. A handle grabbed and let go without moving records
nothing.

## Change a mesh's shape

The modelling operations are on the **Modify** menu and on the tool shelf. Each one needs a
particular mask, and refuses with a notice under any other.

| Operation | Mask | Key | What you select | What it does |
|---|---|---|---|---|
| Extrude Face | Face | | One or more faces | Makes a copy of each face joined to its outline by a ring of new faces |
| Inset Face | Face | I | One or more faces | Makes a smaller face inside each face, a quarter of the way to its centre |
| Split Face | Vertex | | Two vertices of one face, not next to each other | Joins the two vertices with a new edge, making two faces |
| Split Edge | Edge | | One or more edges | Adds a vertex at the middle of each edge |
| Merge Vertices | Vertex | J | Two or more vertices of one mesh | Merges them into one vertex at their centre |

### Extrude a face

1. Select the mesh, then press 8 for the Face mask.
2. Click the face, or Shift-click several.
3. Choose **Modify > Extrude Face**.
4. Press W, and drag a handle to pull the faces out.

The extruded faces stay selected and have not moved yet. Faces that share an edge extrude as
one piece.

### Inset a face

1. Select the faces under the Face mask.
2. Press I, or choose **Modify > Inset Face**.
3. To change how far it is inset, press R and drag the centre handle.

Each face is inset on its own. The smaller faces stay selected.

### Split a face

1. Under the Vertex mask, select two vertices on one face that are not joined by an edge.
2. Choose **Modify > Split Face**.

The two vertices stay selected.

### Split an edge

1. Under the Edge mask, select one or more edges.
2. Choose **Modify > Split Edge**.

Both halves of each edge are selected afterwards, so you can split again.

### Merge vertices

1. Under the Vertex mask, select two or more vertices of one mesh.
2. Press J, or choose **Modify > Merge Vertices**.

The merged vertex stays selected. The merge is refused if it would leave an edge shared by more
than two faces, or a face that meets itself. Merging the two corners across a face is refused.

## Undo and redo

| To | Choose | Or press |
|---|---|---|
| Undo the last change | **Edit > Undo** | Ctrl+Z |
| Redo it | **Edit > Redo** | Ctrl+Y or Ctrl+Shift+Z |

A notice names what was undone or redone. The history keeps the last 64 changes. Making a new
change after an undo drops the changes you could have redone. Undo does not change what is
selected. Opening a project clears the history.
