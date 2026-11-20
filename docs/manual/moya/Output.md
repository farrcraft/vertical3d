# Choose the output file, its format and its size

moya renders a frame when it reads `WorldEnd`, and then writes every file the frame names.

## Name the output file

Name it in the scene, or on the command line:

```
Display "scene.png" "file" "rgb"
```

```
moya.exe --file C:\renders\scene.rib --output C:\renders\scene.png
```

- **`--output` replaces the file the scene's first `Display` names.** Use it to render one scene
  to several files without editing it.
- **A relative file name is written in the folder you run moya from.** Give a full path to write
  somewhere else.
- **A second `Display` whose name starts with `+` adds a file**, such as `"+scene.exr"`. A
  `Display` without the `+` replaces every file named before it.
- **A scene that names no `Display`** writes its picture to the `--output` file, or else to an
  EXR file named after the scene, in the folder you run moya from. `spheres.rib` writes
  `spheres.exr`. The two scenes in `moya/data/`,
  [test-scene-01.rib](../../../moya/data/test-scene-01.rib) and
  [raytrace-scene.rib](../../../moya/data/raytrace-scene.rib), name no `Display`.
- **A `Display` of type `"framebuffer"` writes nothing**, since moya opens no window. The log
  says so. Any other type writes a file.
- **A file of several frames** writes its files after each frame, so it leaves the last frame's.

## Choose the format

The file name's extension sets the format:

| Extension | Format |
|---|---|
| `.png` | PNG: 8 bits of red, green and blue, as a display shows the picture |
| Any other | OpenEXR: the picture in floating point, with an alpha channel |

- **A PNG is the picture as a screen shows it.** moya converts its colours through the ACES view
  for an sRGB display. A PNG holds no alpha.
- **An EXR file holds the light itself**, in the ACEScg colour space. Its alpha is the share of
  each pixel the scene covers. Open it in a compositor or an image viewer that reads EXR.
- **An EXR file a `Display` names holds 16-bit floats.** The file of a scene that names no
  `Display` holds 32-bit floats.
- **`--output` has to end in `.png` or `.exr`.**
- **A `Display` of type `"deepexr"`** writes a deep EXR file, which holds every surface each
  pixel sees at its own depth, for a compositor.

## Set the size

`Format width height pixelaspect` sets the size in pixels:

```
Format 640 480 1
```

- The default is 320 by 240, with square pixels.
- A side is from 1 to 65536 pixels. A larger side, or one that is not a number, is refused with
  a warning, and the size stays as it was.
- A side of zero or less is the default for that side.
- `--width` and `--height` on the command line replace the scene's `Format` size. Give both. The
  scene's pixel aspect still applies.

```
moya.exe --file C:\renders\scene.rib --output C:\renders\small.png --width 160 --height 120
```

## Render part of the picture

`CropWindow xmin xmax ymin ymax` renders only part of the picture. Each value is a fraction of
the width or the height, from 0 to 1, and y runs down from the top:

```
CropWindow 0.25 0.75 0.25 0.75
```

- **Only the pixels inside the window are rendered**, so the render takes a share of the time.
  Each pixel is the same as that pixel of a render of the whole picture.
- **A PNG holds the window alone**, at the window's size.
- **An EXR file holds the window at its place in the whole picture.** A viewer that reads EXR
  shows it where it lies, with the rest empty.
- A window with no area is logged, and the whole picture is rendered.

## Fill the background

Where nothing is drawn, a pixel is black, with an alpha of 0. The `background` imager puts a
colour behind the scene:

```
Imager "background" "background" [0.55 0.65 0.8]
```

- **The colour is laid behind the picture when a file is written.** Each pixel takes the colour
  in the share its alpha leaves uncovered, so edges blend into it. The alpha becomes 1.
- **The background lights nothing**, and no reflection shows it. Use a dome light, such as
  `PxrDomeLight`, for a sky that lights the scene.
- **Put the imager before `WorldBegin`.** RenderMan's `DisplayFilter
  "PxrBackgroundDisplayFilter"` with its `"color backgroundColor"` does the same.
- Any other imager is logged, and changes nothing.

## Add channels

An EXR file can hold more than the picture. The `Display` mode names its channels, apart by
commas, after the picture's own:

```
DisplayChannel "float z"
DisplayChannel "color diffuse" "string source" ["color lpe:C<.D>.*"]
Display "scene.exr" "file" "rgba,z,diffuse"
```

- **`rgb`, `rgba`, `Ci` and `a` name the picture itself.** Every EXR file holds it.
- **`DisplayChannel` declares a channel** by its type and name. Its `"string source"` says what
  the channel collects. A channel with no source collects what its name names.
- **`z`, `P` and `Nn`** collect the depth, the position and the normal where each camera ray
  first meets a surface.
- **A source of `lpe:` and a light path expression** collects the light of the paths it
  matches, in OSL's notation. `C<.D>.*` is the light whose first bounce off a surface is
  diffuse. A light's `"string lightGroup"` names a group an expression can match.
- **`"int denoise" [1]`** on a `DisplayChannel` writes the channel denoised beside it.
- **`SampleFilter "PxrCryptomatte"`** writes a Cryptomatte file of the objects or the
  materials, for picking mattes in a compositor.

[Rib.md](Rib.md#options) lists these requests.
