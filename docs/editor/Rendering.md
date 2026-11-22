# Rendering

The editor renders its scene with moya, the offline renderer, in two ways:

- Render, into a render view in an area or in a window of its own, while the user works.
- Batch Render, as files, for every frame of the timeline.

This page covers both, the render settings, and what the editor hands moya.
[Files.md](Files.md#rib-export) covers the RIB it writes, and [offline/](../offline/README.md)
covers moya.

## The two ways in

**An interactive render goes through a renderer interface.** `Delegate` in `src/render/` is
what the editor drives:

- `render(scene, camera, settings)` starts a render from its first sample, and starts one already
  running again.
- `update(scene, camera, settings)` hands the render whatever its picture shows that has changed,
  and does nothing otherwise. What the picture shows is each item's document id, revision and
  parent, the values an evaluation of the current frame gives each item, the camera's placement
  and lens, and the settings. A change of frame or a pending edit is handed over too.
- `cancel()` stops a running render. Its latest picture stays, and holds what was sampled before
  the stop.
- `resume()` lets a stopped render sample on from the samples its picture holds, and it ends on
  the picture of a render never stopped.
- `status()` gives the render's state, how much of it is done, the samples each pixel of the
  latest picture holds and those it holds when done, and a count that moves with each new
  picture.
- `show(showing)` says how often pictures come, as the least seconds between two, and whether
  they are denoised and show the albedo and the normal too. A running render takes it from its
  next picture, and a render that has finished or stopped makes its latest picture again.
- `picture(channel)` gives the latest picture in a channel: the beauty, the denoised beauty, the
  albedo or the normal. A picture that does not show the channel gives its beauty, so a coarse
  picture and one not yet denoised are the beauty in every channel.

**An edit reaches a running render.** The render takes it between two passes without being
cancelled, and samples the edited scene from its first sample. A render that had finished or
stopped starts again.

**moya is the first renderer behind it.** `MoyaDelegate` links `v3dlib_moya` into the editor and
renders with moya's path tracer through one render session, kept for as long as the editor
renders, on a thread of its own. No RIB text is written or parsed, and no
file stands between the render and its picture.

- **An edit is committed to the retained scene, and the session applies it.** Each `update()`
  brings moya's retained scene in step with the editor's through a `SceneTranslator`, on the
  caller's thread, changing only what moved, and commits the change as one batch. The session
  drops the pass in progress, applies the batch, and rebuilds only what it touched. The render's
  thread owns the retained scene: it applies every batch, and is the only thread that reads the
  scene while a render runs. [The scene](../offline/Scene.md) describes what it holds.
- **`render()` starts a new session.** The one before ends, and the new one builds every
  structure from nothing.
- **A picture follows each pass**, at most one every interval `show()` names, and
  `Delegate::kInterval` seconds until it names one, but for the first of each edit and the last.
  The first pictures after an edit are coarse, each block a sample. A cancel drops the pass in
  progress before its next tile, and the latest picture then holds every sample before it.
- **Progress is counted in samples**: the samples the latest picture holds over those the render
  takes. A coarse picture leaves it where it was. A render whose pixels all stop early ends at 1.
- **The picture is the beauty as a display shows it**, through the colour configuration's
  default display and view at 8 bits a channel, as moya writes a PNG output. The picture a render
  ends on and moya.exe's render of the scene's RIB export give the same bytes, after any edits.
  The denoised beauty and the albedo are shown the same way, and the normal is each component
  from -1 to 1 as 0 to 255.
- **The film holds the denoiser's guides.** The translator sets the options' `denoise`, which
  the RIB export does not, so that a picture can be denoised from the albedo and the normal and
  can show them. The beauty's bytes do not depend on it.
- **The editor carries moya's CPU libraries and MaterialX's standard libraries beside it**, as
  moya.exe does, since the device and the materials load them from there.

- **The translator makes what the RIB export makes through moya's RIB client**, so the two
  describe alike. The options, the camera and the output come first. Each mesh is a prototype
  and an instance, each run of a curve's curves is a prototype and an instance, and each light
  is one of UsdLux's. Every surface takes one `matte` material, and a scene with no light gets the
  camera's distant light. Every instance is two-sided, as RI's default `Sides` is, since the
  export names none.
- **Each object is named after its item**, and kept by its kind and name from one render to the
  next. A curve's second run is its name and `2`, and the camera's light is `camera light`.
- **An update sets only what changed.** A parameter is set when its value differs from the one
  set before, so moving one item sets its instance's transformation alone. An object no item
  makes any more is removed. An update that changes nothing commits nothing.

**A batch writes files and runs moya.exe.** `Batch` writes one RIB file per frame per camera,
then runs `moya.exe --file <job>.rib --silent` on each in turn on a thread of its own. moya.exe
renders each by path tracing and writes the PNG the file's `Display` names. A frame is rendered
when moya.exe ends with 0 and its PNG exists. The files stay, so a farm or another
RenderMan renderer can render them instead. `runProgram()` in `src/render/Process.h` starts the
program, waits for it, and ends it when a stop is asked for.

A renderer failure during Render View takes the editor down with it, because moya runs in the
editor's process. One during a batch fails only that frame.

Background: [ADR-0106](../adr/editor/0106-editor-a-renderer-interface-for-interactive-a-process-for-batch.md),
[ADR-0253](../adr/offline/0253-offline-a-render-is-a-session-that-takes-edits-between-passes.md)

## What is rendered

The camera is the one selected scene camera, or the active view when there is not exactly one.
`shotFrom()` in `src/render/Shot.h` picks it, and `shoot()` sends the scene from it to a RIB
handler as the settings say. `writeShot()` is `shoot()` into a `rib::Writer`, which makes a RIB
file of it for a batch and for the export. The camera takes the picture's size and frame aspect
from the settings, so the picture is not stretched to the shape of the view. `shotCamera()`
gives the camera both a shot and the translator look through.

- **Each item's block is named after the item**, by `Attribute "identifier"`. A light is
  written at world level, so its name is set there before it, and the camera's light is named
  `camera light`.
- **A mesh is one `PointsPolygons`**: every vertex, and the triangles each face is cut into,
  since RI draws a polygon only when it is flat and convex.

**Render settings** belong to the document and are saved with it:

| Setting | Default | Range |
|---|---|---|
| Width and height | 640 by 480 | 1 to `RenderSettings::LARGEST_SIDE`, 16384, each |
| Samples along each side of a pixel | 2 | 1 to `RenderSettings::MOST_SAMPLES`, 16 |

## A rendered view

A 3D view in rendered shading shows moya's picture of the scene in place of its own shading.
`view::RenderedViews` keeps one render session for each such view, through a `MoyaDelegate` of its
own, and the controller ticks it with every view each window shows.

- **A view's session renders from the view's camera**, and each tick hands it the scene and that
  camera, so an orbit, a pan or a zoom is an edit of that session alone. The render settings are
  the scene's, but for the size.
- **The picture is the view's size divided by its pixel size**, rounded up. A view through a scene
  camera renders the camera's frame in place of the whole view. Each side is at least 1 and at
  most `RenderSettings::LARGEST_SIDE`. The pixel size is `Display::pixelSize`, which the view's
  state holds as `pixelSize`.
- **A view that is not shown pauses its session.** The render stops, keeps its picture, and
  samples on once the view is shown again, through `Delegate::resume()`. A view that leaves
  rendered shading, or is gone, ends its session.
- **Each new picture is uploaded as the view's texture**, in the Renderer's slot of the view's name.
  Every session runs on oneTBB's one pool of threads, beside the render view's.
- **The overlays are tested against the scene's depth.** The view draws its meshes filled, for
  their depth alone, then the picture over them, then its grid and wireframe. The picture
  replaces the meshes' colour and leaves their depth, so a line behind a surface is hidden as it
  is in a shaded view. The handles are drawn over all of it, as in any view. Where moya renders
  what the view does not rasterise, such as displacement or a volume, the lines meet the
  rasterised surface. Until its first picture, a rendered view is shaded flat.

Background: [ADR-0256](../adr/editor/0256-editor-a-rendered-view-has-its-own-session-and-raster-depth.md)

## The Render menu

| Item | Command | Effect |
|---|---|---|
| Render | `render::view` | Render the shot, and show the render view |
| Batch Render | `render::batch` | Render every frame of the timeline's range from each scene camera into the project's `renders/` directory |
| Stop | `render::stop` | Stop the render and the batch |
| Settings | `render::settings` | Show the render settings |

To show an editor, Render and Settings choose the first area's tab that holds one. When no area
holds one, Render opens a window of its own showing a render view, and Settings adds one as a tab
of the active area. Render adds a tab too when no window can be opened.

**The render's window** is a second window of the platform's, 800 by 600 at first, holding one
area that shows the render view. It is a window of areas as [Scene.md](Scene.md#other-windows)
describes, so it can be split and hold tabs. Render brings the render up to date in it while it
is open, as in an area. Its close button closes it and leaves the render running.

## The render view

The render view (`render-view`) is an editor an area holds. Its first row has a Render button, a
Stop button, the channel it shows, and what the render is doing with the samples its picture
holds. Below them is the latest picture, fitted to the rest of the area without changing its
shape. While any area shows one, the editor brings the render up to date each tick, so an edit
renders again, and each picture is shown as it comes. `RenderView` in `src/control/` draws it,
and owns the renderer and the batch.

- **Every render view shows the same render, each in a channel of its own**: Beauty, Denoised,
  Albedo or Normal. The channel is kept in the editor's state as `channel`, with the value
  `beauty`, `denoised`, `albedo` or `normal`, so a saved workspace keeps it. A state that names
  none of them is the beauty. The channel is not a render setting, and choosing one does not
  render again.
- **Pictures are denoised, and show the albedo and the normal, only while a view shows that
  channel.** A channel first shown once the render has finished is made from its latest picture,
  without sampling again.
- **The render view sets how often pictures come**, from the average time between its ticks, and
  never more often than one every `RenderView::LEAST_INTERVAL` seconds, a tenth. The interval is
  asked for again only once the average moves by half of it.

**The render settings** (`render-settings`) are an editor too. The width, the height and the
samples are numbers that are dragged or typed. A change is taken when its drag or its typing
ends, and a number that is not a whole one within its range is refused. Each change goes
through the history, so one undo puts it back.
`RenderSettingsEditor` in `src/control/` draws it.

Background: [ADR-0154](../adr/editor/0154-editor-the-render-view-is-an-editor-in-an-area-or-a-window.md)

**A batch** names each camera by its document id, as `camera3`. A scene with no camera is
rendered from the active view, as `view`. Each camera has a directory under `renders/`, holding
`camera3.0001.rib` and the `camera3.0001.png` moya.exe writes from it, for every frame. The RIB
files are all written when the batch starts, each from an evaluation of its frame. Writing them
changes nothing in the scene, and a pending edit is not in them. The caption shows which frame
is rendering. A stop ends the frame being rendered and renders no more.

**moya.exe is found beside the editor.** The build copies it there, with the libraries it loads,
through `v3d_copy_program`.
