# Rendering

How to render the scene with moya, change what a render is made with, and render every frame of
the animation to files. moya is the renderer that comes with Vertical3D, and
[offline/](../../offline/README.md) describes it.

## Choose the camera

A render looks through one camera:

- If exactly one scene camera is selected, the render looks through it, as the current frame
  places it.
- Otherwise it looks through the active view's camera. That is the view the cursor was last
  over.

The picture has the size the render settings give, whatever the shape of the view.

## Render the current frame

1. Choose the camera as above.
2. Choose **Render > Render**.

The picture appears in a render view. If an area holds a render view, that area shows it. If no
area holds one, a window of its own opens, holding a render view. You can split that window and
add tabs to it as you can the main window. Its close button closes it, and the render goes on.

The picture appears when the render is finished. The top of the render view says what the
render is doing: Nothing rendered yet, Rendering with how much is done, Rendered, Stopped, or
that the render failed.

While a render view is shown, the editor renders again after each change to the scene, the
camera, the frame or the settings. A render already running is stopped and started again.

The render view has two buttons. **Render** starts a render. **Stop** stops it, and keeps the
last finished picture.

moya runs inside the editor while it renders into the render view. If moya fails, the editor
closes with it. Save your project before a long render.

The Rendering workspace shows a render view with the render settings beside it.

## Change the render settings

Choose **Render > Settings**, or press Shift+F10 with the cursor over an area.

| Setting | What it sets | Default | Range |
|---|---|---|---|
| Width, Height | The picture's size in pixels | 640 by 480 | 1 to 16384 each |
| Samples | How many samples are taken along each side of a pixel. More samples give smoother edges | 2 | 1 to 16 |
| Hider | How moya finds what each pixel sees: Reyes or Ray trace | Reyes | |

Drag a number, or click it and type. The change is taken when you let go or finish typing. A
number that is not whole, or is out of its range, is refused. Each change is one step of the
history, and the project saves the settings.

Reyes keeps every sample of the picture in memory at once. A large picture with many samples
needs a great deal of memory under Reyes. Ray trace does not.

## What a render shows

- Every mesh, in a plain matte surface. The editor has no materials.
- Each curve whose **Renders** box is checked, at its **Width**.
- Every light. A scene with no light is lit by a distant light from the camera.
- Cameras, groups and the grid are not drawn.

## Render every frame to files

Choose **Render > Batch Render**.

The batch renders each frame of the timeline's range from each camera in the scene. A scene
with no camera is rendered once for each frame, from the active view.

The pictures go into the `renders` directory of the open project, with a directory for each
camera. Each camera's directory is named by a number the scene gives the camera, such as
`camera3`. The active view's directory is named `view`. Each frame gives two files, such as
`camera3.0001.rib` and `camera3.0001.png`. The `.rib` file describes the frame for moya, and the
`.png` file is the picture.

While a batch runs, the caption says which frame is rendering, as in `rendering 3 of 120`. You
can go on working. A batch renders the animation as keyed, so a change you have not keyed is not
in it.

Choose **Render > Stop** to stop the render and the batch. The frame being rendered stops, and no
more frames are rendered.
