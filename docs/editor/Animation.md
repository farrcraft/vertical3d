# Animation and the timeline

The editor keys the properties of scene items on frames and plays them back. This page covers
what can be keyed, how a curve behaves, how the scene is evaluated at a frame, the Animate menu,
the timeline and graph editors, and auto key.
[Files.md](Files.md#scene-documents) covers how curves are saved.

## What is keyed

A key drives one number. Every property a curve can drive has a path:

| Kind | Paths |
|---|---|
| Every kind | `translation.x`, `.y`, `.z`; `rotation.x`, `.y`, `.z`; `scale.x`, `.y`, `.z` |
| Camera, as well | `lens.fov`, `lens.extent`, `lens.near`, `lens.far` |
| Light, as well | `emission.intensity`, `emission.red`, `emission.green`, `emission.blue`, `emission.cone`, `emission.delta` |

`properties(kind)` in `src/scene/Properties.h` lists them, and `declares(kind, path)` checks one.

**A rotation is keyed as three Euler angles, in degrees,** turned about x first, then y, then z.
An item holds its rotation as a quaternion, which more than one set of angles gives. A key takes
the set closest to the curves' values at that frame, adding whole turns where they bring it
closer. A rotation keyed at 170 and then 20 degrees further is keyed at 190 rather than -170.
`eulerNear()` does this, and `fromEuler()` turns angles back into a quaternion. Every item turns
in the same order. A rotation of a quarter turn about y locks x and z together, and z is then
taken as 0.

## Curves

A curve is a `v3d::animation::Curve`, which [Animation.md](../api/Animation.md#curve) describes.
Its keys sit on frames, each with its own interpolation (step, linear or Bezier) and its own
slopes. Before its first key a curve holds the first key's value, and after its last it holds
the last's.

The scene holds the curves. Each one is named by its item's document id and the path it drives,
as a parent is, so a key does not move its item's revision.

- `Scene::curve(id, path, curve)` sets a curve, and an empty curve removes it. It refuses a path
  the item's kind does not declare.
- `Scene::curves(id)` gives an item's curves by path.
- `Scene::keyedFrames(items)` gives every frame any of the items has a key on, rising.
- Removing an item removes its curves.

## Evaluating a frame

**An item holds only the values the user gave it.** A curve never writes into an item. What a
frame shows is an `Evaluation` the scene makes, and making one changes nothing in the scene. The
evaluation gives each item its `Pose`, the placement, lens and emission a curve can drive, and
its `Placed` matrices. Every reader takes those from an evaluation: the views, the picker, the
manipulators, Set Key, the RIB export, the render view and the batch. A save writes the items'
own values, and playback moves no revision.

- `Scene::evaluate()` evaluates the current frame, as the views show it.
- `Scene::evaluate(frame)` evaluates any frame of the animation. The batch writes each frame
  from one, so it never changes the frame the user is on.
- A keyed property takes its curve's value at the frame. Every other property takes its item's
  value.

**An edit to a keyed property is pending until the frame is set.** A keyed property whose value
in its item has changed since the frame was last set, or since the item was added, shows that
value in an evaluation of the current frame, over its curve. Changing an unkeyed property of the
same item does not make its keyed ones pending.

- Set Key keys the values the current frame shows, so it takes a pending edit into the curves.
- Setting the frame drops every pending edit, the current frame included, and each keyed
  property shows its curve again. The item keeps the value it was given, which shows again if
  the curve is removed.
- A rotation is pending as a whole. Its angles are read from one quaternion, so a turn about one
  axis can change all three.
- A pending edit is not part of the animation. `Scene::evaluate(frame)` does not show it, so the
  batch does not render it.
- An edit is noticed by its value. Giving a property back the value it held when the frame was
  set is not an edit.
- A frame is not a command, so undo does not step back through frames. Undo puts back an item's
  own values. Before the frame changes, that puts each keyed property back on its curve. After
  it, the values put back are an edit, and are pending until the frame is set again.
- **A drag starts where the item is shown.** The manipulator's target takes each object's
  placement from the evaluation, and writes the whole placement into the object as it moves.

Background: [ADR-0120](../adr/editor/0120-editor-a-frame-is-evaluated-not-posed-into-items.md)

## The timeline

A document's `Timeline` holds its frame rate, the range of frames that plays, the current
frame, and whether auto key is on. The default is 24 frames a second over frames 1 to 120, with
auto key off. Keys sit on frames, so a change
of rate leaves every key where it is and changes how fast they play. The window's caption shows
the current frame.

The Animate menu:

| Item | Command | Effect |
|---|---|---|
| Set Key | `timeline::set` | Key every property of each selected item at the current frame, through the history, so one undo takes the keys away |
| Auto Key | `timeline::auto_key` | Turn auto key on or off. The item is checked while it is on |
| Delete Keys | `timeline::delete_keys`, Delete | Delete the keys chosen in the timeline editor, as one change, and choose none |
| Play, Stop | `timeline::playback::play`, `stop` | Start and stop playback. While playing, `simulate()` steps the frame at the frame rate and starts again from the first frame after the last. `timeline::playback::toggle`, Space, stops playback while it runs and starts it otherwise |
| Next Frame, Previous Frame | `timeline::playback::next`, `previous` | Step one frame, held within the range |
| First Frame, Last Frame | `timeline::playback::first`, `last`, Shift+Left and Shift+Right | Go to either end of the range |
| Next Key, Previous Key | `timeline::playback::next_key`, `previous_key`, Up and Down | Go to the nearest keyed frame after or before the current one |
| First Key, Last Key | `timeline::playback::first_key`, `last_key` | Go to the earliest or the latest keyed frame |

The keyed frames are those of the selection, or of every item when nothing is selected. Each
move to a frame sets the frame, which drops every pending edit.

## The timeline editor

An area can hold a timeline, the `timeline` editor, which `TimelineEditor` draws in the
immediate layer.

- **The controls** are one row. A control that would pass the area's right edge starts a new
  row, and the range's two ends stay on one.
- **The transport** is five icon buttons: first frame, previous key, play, next key and last
  frame, which do what the Animate menu's items do. Each key button carries a key's diamond.
  Play shows a stop icon while the scene plays, filled as a checked toggle is. Each button's
  tooltip names what it does, and the key bound to its command when there is one.
- **The fields** set the frame, and the first and last frames of the range, labelled Range and
  to. Each is as wide as its label and a frame number. The frame moves as its field is dragged,
  and is not a change. The range is one change, `frame range`, recorded when a field's drag or
  typing ends.
- **The rate** is chosen from a menu of 12, 15, 23.976, 24, 25, 29.97, 30, 48, 50 and 60 frames
  a second, each named as `24 fps`. A scene whose rate is none of them has its rate offered
  among them. A choice is the same `frame range` change. `Document::timing()` refuses a rate
  that is not a positive number and a range that runs backwards. A new range holds the frame
  within it.
- **A check** turns auto key on or off.
- **The ruler** counts frames in steps of 1, 2 or 5 times a power of ten, the smallest that
  leaves room for each number. The range's first frame sits half a frame in from the left of the
  tracks, and its last half a frame in from the right.
- **A row** for each selected item shows its keys as diamonds. A key here is every key the item
  has on one frame, across its curves, a `scene::FrameKey`. The chosen keys are drawn in the
  selection's amber, and the playhead runs down the ruler and every row in the theme's
  highlight.
- **A press on the ruler**, or on a row away from its keys, chooses no key and moves the frame
  to where it landed. A drag scrubs.
- **A press on a key** chooses it, unless it is chosen already. A drag moves every chosen key
  along by whole frames, drawn where it will land, and moves them when it is let go. The move is
  one change, `move keys`, which `Document::moveKeys()` makes. A moved key keeps its value,
  interpolation and slopes, and replaces a key it lands on.
- **Delete Keys** deletes the chosen keys through `Document::deleteKeys()`, as one change,
  `delete keys`. A curve left with no keys is removed.
- The chosen keys are shared by every timeline editor. A key stops being chosen when its item
  stops being selected or its frame is no longer keyed.

## The graph editor

An area can hold a graph, the `graph` editor, which `GraphEditor` draws in the immediate layer.

- **The graph** draws each curve of the selected items over a grid of frames and values. A
  curve whose path ends in `x`, `y` or `z` is drawn in that axis's colour, as the views draw the
  axes, and every other curve in the theme's text colour. Each key is a diamond. The grid's lines
  are 1, 2 or 5 times a power of ten apart, the smallest step that leaves room for each number,
  and each value line is numbered.
- **The ruler** above the graph numbers its frames. A press on it moves the frame to where it
  landed, and a drag scrubs. The playhead runs down the ruler and the graph in the theme's
  highlight.
- **A press on a key** chooses that one key of that one curve, a `GraphEditor::CurveKey`. A press
  away from every key chooses none. A drag moves the chosen key by whole frames and by any value,
  drawn where it will land, and records the move when it is let go. A moved key keeps its
  interpolation and slopes, and replaces a key it lands on.
- **The chosen key** is drawn in the selection's amber with its handles. A key has a handle
  toward each neighbour the curve is a cubic to, reaching a third of the way to that neighbour
  along its slope. A drag on a handle gives both of the key's slopes the slope from the key to
  the handle, and the key stops being automatic. A handle stays on its own side of the key.
- A move or a turn is one change, `edit curve`, which `Document::reshape()` makes. It refuses a
  path the item has no curve on, and a curve the same as the one there.
- **The view moves as an orthographic viewport's does.** While the truck move is held (Left
  Ctrl) or latched, a drag moves the view. While the zoom or dolly move is (Left Alt), a drag
  right shows fewer frames and a drag up fewer values, about where the drag began. The wheel
  shows less about the cursor for a notch away from the reader, and more for a notch toward.
- **Frame All** fits the view to the selected items' curves, with a tenth of their span to spare
  each side and at least ten frames across. With no curves it shows the range, from -1 to 1. A
  new graph editor is fitted when it is first drawn. Each editor keeps its own view, and every
  graph editor shares the chosen key.

## Auto key

**Auto key keys an edit as it is recorded.** While it is on, an edit to a number a curve can
drive keys that number at the current frame, and the edit and its keys are one change, which one
undo takes back.

- Only the properties the edit changed are keyed, on each item it changed. A rotation is keyed
  whole, as it is pending whole. An edit that changes nothing a curve drives, such as a rename,
  sets no key.
- A manipulator's gesture and an edit in the properties editor are keyed. A gesture on
  components moves geometry, which no curve drives, so it sets no key.
- `command::Keyed::record()` makes the key when an edit is recorded, and returns the edit alone
  when auto key is off.
- Turning auto key on or off is a change to the document, which undo takes back. The project
  saves it.

Background: [ADR-0105](../adr/editor/0105-editor-animation-is-scalar-curves-on-item-properties.md)
