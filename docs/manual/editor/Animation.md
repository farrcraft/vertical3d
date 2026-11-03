# Animating

How to key an item's properties, edit the keys in the timeline and the graph editor, and play the
animation. [Concepts.md](Concepts.md#frames-keys-and-curves) defines frames, keys and curves.

The Animation workspace has a perspective view, the outliner and the properties editor, and a
strip along the foot with the timeline, the graph editor and the console as tabs. Click its tab
at the top of the window to use it.

## What can be keyed

| Item | Properties |
|---|---|
| Every item | Translation, Rotation and Scale, each X, Y and Z |
| Camera | Field of View, Extent, Near and Far |
| Light | Intensity, Red, Green, Blue, Cone and Soft Edge |

A rotation is keyed as three angles in degrees, turned about X, then Y, then Z.

## Set the frame

Any of these sets the current frame:

- Drag the **Frame** field in the timeline, or click it and type.
- Click or drag on the ruler at the top of the timeline or the graph editor.
- Choose an item from the **Animate** menu: **Next Frame**, **Previous Frame**, **First Frame**,
  **Last Frame**, **Next Key**, **Previous Key**, **First Key** or **Last Key**.
- Click **First**, **Previous Key**, **Next Key** or **Last** in the timeline.

The key items go to the keys of the selected items, or of every item when nothing is selected.
The frame stays within the range. The window's caption and the status bar show the frame.

## Key an item by hand

1. Set the frame.
2. Move, turn or scale the item, or change its values in the properties editor.
3. Choose **Animate > Set Key**.

Set Key keys every property of each selected item at the current frame. One undo takes the keys
away.

**Key a change before you change the frame.** If you change a keyed property and then change the
frame, the property shows its curve again, and your change no longer shows. The change is not
in the animation and is not rendered.

## Key changes as you make them

Choose **Animate > Auto Key**, or check **Auto Key** in the timeline. The menu item is checked
while auto key is on.

While auto key is on, each move with a manipulator and each edit in the properties editor keys
the properties it changed, at the current frame. The change and its keys are one step, which one
undo takes back. A rename, or a move of a mesh's vertices, sets no key.

Choose **Animate > Auto Key** again to turn it off. The project saves whether it is on.

## Set the frame range and rate

In the timeline, change the **Start**, **End** and **Rate** fields. Rate is in frames a second.
The change is one step of the history.

A new scene runs at 24 frames a second, over frames 1 to 120. The range cannot run backwards,
and the rate must be above zero. Keys stay on their frames when the rate changes, so the
animation plays faster or slower.

## Play the animation

Choose **Animate > Play**, or click **Play** in the timeline. Playback runs at the frame rate
and starts again from the first frame after the last. Choose **Animate > Stop**, or click
**Stop**, to stop.

## Edit keys in the timeline

The timeline shows a row for each selected item. A diamond marks each frame the item has a key
on, for any of its properties.

- **Click a diamond** to choose that key. Chosen keys are drawn in amber.
- **Drag a chosen key** along the row to move every chosen key by whole frames. A key moved onto
  another replaces it.
- **Press Delete**, or choose **Animate > Delete Keys**, to delete the chosen keys.
- **Click the ruler**, or a row away from its keys, to choose no key and set the frame there.

Moving and deleting keys are each one step of the history.

To show a timeline in an area, move the cursor over it and press Shift+F12.

## Edit curves in the graph editor

The graph editor draws each curve of the selected items over frames across and values up. A
curve of an X, Y or Z property is red, green or blue. Each key is a diamond.

- **Click a key** to choose it. The chosen key is drawn in amber with its handles.
- **Drag the chosen key** to move it to another frame and value. A key moved onto another
  replaces it.
- **Drag a handle** to change the slope of the curve through the key. Both sides of the key take
  the new slope.
- **Click Frame All** to fit the view to the selected items' curves.

Each move of a key or a handle is one step of the history.

To move around the graph:

| To | Do this |
|---|---|
| Slide the graph | Hold Left Ctrl and drag |
| Show more or fewer frames and values | Hold Left Alt and drag. Drag right for fewer frames, and up for fewer values |
| Zoom about the cursor | Turn the mouse wheel |

To show a graph editor in an area, move the cursor over it and press Shift+F6.

## See an item's keys

Select the item, and click the **Keys** tab in the properties editor. It lists each property
that has keys, with how many it has.
