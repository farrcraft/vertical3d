# Working with the scene

How to find and name items, group them, add cameras, lights and curves, and edit their values.
[Concepts.md](Concepts.md) defines items and parents.

## Find an item in the outliner

The outliner lists every item under a row named Scene. An item with children has a mark before
its name.

- **Click a row** to select its item and nothing else.
- **Hold Left Shift and click a row** to add its item to the selection, or take it out.
- **Click the mark** before a row to hide or show the items under it. The mark on the Scene row
  folds every item.
- **Type in the Filter field** at the top to show only the rows whose names hold every word you
  type, in any case. Each row that matches is shown with the rows above it.

Each row has an icon for its item's kind. A mesh is orange, a curve green, a camera grey, a
light yellow and a group a darker grey.

To show an outliner in an area, move the cursor over it and press Shift+F9.

## Rename an item

1. Select the item, and only that item.
2. In the properties editor, click the **Item** tab.
3. Click the **Name** field, type the new name, and press Return or click away.

No two items in a scene share a name. If the name is taken, the editor adds a number such as
`.001` when you finish typing. A new item is named for its kind, such as `Cube` or
`Point Light`, numbered the same way when that name is taken.

## Edit an item's values

Select one item. The properties editor shows it under three tabs:

- **Item** holds the item's kind, its name, and the values below.
- **Mesh**, for a mesh, counts its vertices and faces.
- **Keys** lists each property that has keys, with how many it has.

The Item tab shows these values:

| Heading | Fields | Shown for |
|---|---|---|
| Translation | X, Y, Z, in the scene's unit of length | Every item |
| Rotation | X, Y, Z, in degrees | Every item |
| Scale | X, Y, Z | Every item |
| Lens | Field of View in degrees, Extent, Near and Far | A camera |
| Projection | Perspective or Orthographic | A camera |
| Emission | Intensity, Red, Green, Blue, and Cone and Soft Edge in radians | A light |
| Type | Point, Distant or Spot | A light |
| Render | Renders, and Width | A curve |

To change a number, drag across its field, or click it and type. A drag changes a distance by a
hundredth of a unit for each pixel, and an angle by half a degree. The views follow as you
drag. When you let go, or finish typing, the change is one step of the history.

A value that would make a camera's lens, a light or a curve invalid is not taken.

A camera and a light ignore their scale.

With more than one item selected, the properties editor says how many are selected. With
nothing selected, it shows the scene's settings.

## Set the unit of length

1. Click empty space in a view, so that nothing is selected.
2. In the properties editor, choose a unit from the **Unit** list under Scene.

The units are Millimetres, Centimetres, Metres, Kilometres, Inches and Feet. A new scene is in
metres. Changing the unit changes the label on each distance. Nothing in the scene moves. An
imported file is scaled into the scene's unit.

## Group items

1. Select the items to group.
2. Choose **Create > Group**.

A new group is made at the average of the items' origins, and the items go under it. Each item
stays where it is. The group is selected afterwards, so moving it moves them all. Undo takes the
items back out and removes the group.

If an item and its parent are both selected, the parent goes in the group, and the item comes
with it.

## Add a camera

1. Move the cursor over the view you want to look from.
2. Choose **Create > Camera > Perspective** or **Create > Camera > Orthographic**.

The camera is placed where the view's eye is, looking the way the view looks. It takes the
view's lens. It is drawn as a pyramid for perspective or a box for orthographic, with a
triangle above the picture that points up.

A render uses a scene camera when exactly one is selected. [Rendering.md](Rendering.md) says
more.

## Add a light

1. Move the cursor over the view you want to light from.
2. Choose **Create > Light** and then **Point**, **Distant** or **Spot**.

The light is placed at the view's eye, shining the way the view looks. A point light is drawn
as a star of three lines, a distant light as three parallel lines, and a spot light as a cone.

A scene with no light is rendered with a distant light from the camera.

## Add and shape a curve

1. Choose **Create > Curve** and then **Bezier**, **NURBS**, **Catmull-Rom** or **Poly**.
2. The curve appears at the centre of the scene as an S shape, three units long along X.

To move its points:

1. With the curve selected, choose the Vertex mask (press 6). A box is drawn at each control
   point.
2. Select points, and move them with the Move, Rotate or Scale manipulator.

A Bezier point carries its two handles with it. To move a handle itself:

1. Make sure the view shows handles. Press H to turn them on or off.
2. Choose **Select > Handle**. A line is drawn from each Bezier point to each of its handles.
3. Select a handle, and move it.

A handle the editor placed for you changes type when you move it, so that it keeps where you put
it. If both handles of a point are aligned, the other handle turns to stay in line.

A curve is not rendered unless you ask for it. Select the curve, and check **Renders** on the
Item tab. **Width** sets how thick it renders.
