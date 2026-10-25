# Animation and the timeline

The editor keys the properties of scene items on frames and plays them back. This page covers
what can be keyed, how a curve behaves, how the scene is posed at a frame, and the Timeline
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

A curve is a `v3d::type::animation::Curve`, which [Types.md](../api/Types.md#curve) describes.
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

## Posing

`Scene::pose(frame)` sets every property that has a curve to the curve's value at that frame.
A property with no curve keeps its value. A pose writes into the items, so the viewports, the
picker, the manipulators and the export all see the posed scene through the same placement
they always read.

**An edit holds until the next pose.** A drag on a keyed item moves it, and it stays there until
the frame changes. Set Key at that frame keeps the edit. Changing the frame without a key puts
each keyed property back on its curve, and an edit to a property with no curve stays. A pose is
not a command, so undo does not step back through frames. An undo after the frame has changed
puts back the values the change started from, and they hold until the next pose.

Reading a document poses the scene at the document's frame.

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
move to a frame poses the scene there.

Background: [ADR-0105](../adr/0105-editor-animation-is-scalar-curves-on-item-properties.md)
