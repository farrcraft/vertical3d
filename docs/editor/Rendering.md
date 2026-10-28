# Rendering

The editor renders its scene with moya, the offline renderer, in two ways:

- Render View, in a window of the editor's ui, while the user works.
- Batch Render, as files, for every frame of the timeline.

This page covers both, the render settings, and what the editor hands moya.
[Files.md](Files.md#rib-export) covers the RIB it writes, and [offline/](../offline/README.md)
covers moya.

## The two ways in

**An interactive render goes through a renderer interface.** `Delegate` in `src/render/` is
what the editor drives:

- `render(scene, camera, settings)` starts a render, cancelling one already running.
- `update(scene, camera, settings)` starts again when anything the picture shows has changed,
  and does nothing otherwise. What the picture shows is each item's document id, revision and
  parent, the values an evaluation of the current frame gives each item, the camera's placement
  and lens, and the settings. A change of frame or a pending edit renders again too.
- `cancel()` stops a running render, and keeps the last finished picture.
- `status()` gives the render's state, how much of it is done, and a count that moves with each
  new picture. `picture()` gives the latest one.

A render reads the scene when it starts, so an edit made while it runs is not in its picture.

**moya is the first renderer behind it.** `MoyaDelegate` links `v3dlib_moya` into the editor.
It records the scene's RIB requests in a `rib::Recorder` on the caller's thread, then replays
them into a moya handler of its own on a thread of its own. No RIB text is written or parsed.
The picture stays in that handler's framebuffer, with no file between them. moya reports its
progress after each bucket of the reyes hider and each row of the ray-trace hider, and stops
there when cancelled. moya resolves its samples into pixels only at
the end of a frame, so the picture arrives when the render finishes, not bucket by bucket.

**A batch writes files and runs moya.exe.** `Batch` writes one RIB file per frame per camera,
then runs `moya.exe` on each in turn on a thread of its own. The files stay, so a farm or another
RenderMan renderer can render them instead. `runProgram()` in `src/render/Process.h` starts the
program, waits for it, and ends it when a stop is asked for.

A renderer failure during Render View takes the editor down with it, because moya runs in the
editor's process. One during a batch fails only that frame.

Background: [ADR-0106](../adr/0106-editor-a-renderer-interface-for-interactive-a-process-for-batch.md)

## What is rendered

The camera is the one selected scene camera, or the active view when there is not exactly one.
`shotFrom()` in `src/render/Shot.h` picks it, and `shoot()` sends the scene from it to a RIB
handler as the settings say. `writeShot()` is `shoot()` into a `rib::Writer`, which makes a RIB
file of it for a batch and for the export. The camera takes the picture's size and frame aspect
from the settings, so the picture is not stretched to the shape of the view.

**Render settings** belong to the document and are saved with it:

| Setting | Default | Range |
|---|---|---|
| Width and height | 640 by 480 | 1 to `RenderSettings::LARGEST_SIDE`, 16384, each |
| Samples along each side of a pixel | 2 | 1 to `RenderSettings::MOST_SAMPLES`, 16 |
| Hider | `hidden`, reyes | `hidden` or `raytrace` |

The reyes hider keeps every sample of the frame in memory at once, so a large picture at many
samples takes a great deal of it. The ray-trace hider keeps nothing per sample.

## The Render menu

| Item | Command | Effect |
|---|---|---|
| Render Settings | `render::settings` | Put up the settings, filled from the document's. Apply takes them through the history, so one undo puts them back, and refuses a field that is not a whole number or settings out of range |
| Render View | `render::view` | Open the render window and render into it |
| Batch Render | `render::batch` | Render every frame of the timeline's range from each scene camera into the project's `renders/` directory |

**The render window** shows what the render is doing, then its picture, fitted to the window
without changing its shape. While it is open, the editor brings the render up to date each tick,
so an edit renders again. Its buttons render again, stop the render, and close the window, which
stops the render too.

**A batch** names each camera by its document id, as `camera3`. A scene with no camera is
rendered from the active view, as `view`. Each camera has a directory under `renders/`, holding
`camera3.0001.rib` and the `camera3.0001.png` moya.exe writes from it, for every frame. The RIB
files are all written when the batch starts, each from an evaluation of its frame. Writing them
changes nothing in the scene, and a pending edit is not in them. The caption shows which frame
is rendering. A stop ends the frame being rendered and renders no more.

**moya.exe is found beside the editor.** The build copies it there, with the libraries it loads,
through `v3d_copy_program`.
