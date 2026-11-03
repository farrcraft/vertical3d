# Working in the viewports

How to move a viewport's camera, show a fixed view, and change how a viewport draws the scene.
A viewport's camera belongs to the view. It is not an item in the scene, and moving it changes
nothing you save.

## Which view a command acts on

A key acts on the view under the cursor. A menu item acts on the view the cursor was last over
before you moved to the menu. A menu in a viewport's own header acts on that view.

## Move the camera

Put the cursor over the view, then:

| To | Do this | In an orthographic view |
|---|---|---|
| Orbit | Drag with the middle mouse button | Nothing happens |
| Dolly, moving nearer or further | Hold Left Alt and drag with the left button. A drag right moves in | Zooms instead |
| Truck, sliding sideways or up and down | Hold Left Ctrl and drag with the left button | The same |

An orthographic view is not turned, so a fixed view such as Front keeps showing the front.

To drive the camera with the left button and no key held, choose a move from
**View > Navigate**: **Truck**, **Dolly**, **Zoom**, **Orbit** or **Pitch**. The move stays on,
and the menu checks it, until you choose it again or choose a manipulator.

- **Zoom** narrows the angle of view of a perspective view as you drag right, and shows less of
  an orthographic one.
- **Pitch** turns a perspective view to look up as you drag up.

The status bar at the foot of the window says what each mouse button does in the active view.

## Look along an axis

A perspective view has a navigation gizmo in its upper right corner. It is a ball at each end
of each axis, filled at the positive end and hollow at the negative. Click a ball to look from
that end of the axis, at the point the view was looking at. The camera keeps its distance.

## Show a fixed view

Choose **View > Camera** and then **Front**, **Back**, **Left**, **Right**, **Top**, **Bottom**
or **Perspective**. A viewport's header has the same items in its first view menu, named for the
camera the view shows.

To switch a view between orthographic and perspective, press P, or choose
**View > Camera > Orthographic**. The item is checked while the view is orthographic. The
camera stays where it is.

## Maximise a view

Press Ctrl+Space with the cursor over a view, or click the maximise button at the right end of
its header. The view takes the room of every area. Press Ctrl+Space again to give every area its
room back.

## Change how a view shades the scene

Choose from **View > Shading**, or from the shading menu in a view's header. The header's menu is
named for how the view draws: Flat, Smooth, Wireframe or Points.

| Item | What it does | Default |
|---|---|---|
| Flat | Lights each face evenly | Chosen |
| Smooth | Blends the light across the faces around each vertex | |
| Front Facing > Points, Lines, Fill | Draws the outward side of each face as its corners, its edges, or filled | Fill |
| Back Facing > Points, Lines, Fill | The same, for the inward side | Fill |
| Cull > None, Front, Back, Both | Leaves out no side, the outward side, the inward side, or both | Back |

The light comes from the eye. Each view keeps its own shading. The Shading workspace starts
with a smooth view.

Some graphics cards cannot draw points or lines. On such a card the side is drawn filled.

## Choose what a view draws

| To toggle | Key | Menu |
|---|---|---|
| The construction grid | G | View > Show > Grid |
| Meshes | M | View > Show > Mesh |
| Cameras | | View > Show > Camera |
| Lights | | View > Show > Light |
| Manipulator handles and a curve's Bezier handles | H | View > Show > Handle |

A viewport's header has an **Overlays** menu with the same five items. Each view keeps its own
set, and each starts with all five on. A curve is always drawn.

## Read the view

- **The grid** lies in the plane of the two axes an orthographic view looks across, and on the
  ground in a perspective view. Every tenth line is stronger. A line through the centre is the
  colour of its axis: red for X, green for Y and blue for Z.
- **The axis marker** in the lower left corner shows which way each axis points.
- **Selected items and parts** are drawn in amber.
- **The status bar** shows what is selected, the current frame, and the frames drawn each
  second. The rate is green when the editor keeps up with the screen, amber when it is slower,
  and red below half the screen's rate.
- **Statistics.** Press F12, or choose **Window > Statistics**, to show or hide the frame
  statistics over the window.
