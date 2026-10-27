# Animation and the timeline

The editor keys the properties of scene items on frames and plays them back. This page covers
what can be keyed, how a curve behaves, how the scene is evaluated at a frame, and the Timeline
menu. [Files.md](Files.md#scene-documents) covers how curves are saved.

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
manipulators, Set Key, the RIB export, the render window and the batch. A save writes the items'
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

Background: [ADR-0120](../adr/0120-editor-a-frame-is-evaluated-not-posed-into-items.md)

## The timeline

A document's `Timeline` holds its frame rate, the range of frames that plays, and the current
frame. The default is 24 frames a second over frames 1 to 120. Keys sit on frames, so a change
of rate leaves every key where it is and changes how fast they play. The window's caption shows
the current frame.

The Timeline menu:

| Item | Command | Effect |
|---|---|---|
| Set Key | `timeline::set` | Key every property of each selected item at the current frame, through the history, so one undo takes the keys away |
| Play, Stop | `timeline::playback::play`, `stop` | Start and stop playback. While playing, `simulate()` steps the frame at the frame rate and starts again from the first frame after the last |
| Next Frame, Previous Frame | `timeline::playback::next`, `previous` | Step one frame, held within the range |
| First Frame, Last Frame | `timeline::playback::first`, `last` | Go to either end of the range |
| Next Keyed Frame, Previous Keyed Frame | `timeline::playback::next_key`, `previous_key` | Go to the nearest keyed frame after or before the current one |
| First Keyed Frame, Last Keyed Frame | `timeline::playback::first_key`, `last_key` | Go to the earliest or the latest keyed frame |

The keyed frames are those of the selection, or of every item when nothing is selected. Each
move to a frame sets the frame, which drops every pending edit.

Background: [ADR-0105](../adr/0105-editor-animation-is-scalar-curves-on-item-properties.md)
