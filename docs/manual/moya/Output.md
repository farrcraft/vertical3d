# Choose the output file, its format and its size

moya writes the picture when it reads `WorldEnd`. It writes a file only when one is named.

## Name the output file

Name it on the command line, or in the scene:

```
moya.exe --file C:\renders\scene.rib --output C:\renders\scene.png
```

```
Display "scene.png" "file" "rgb"
```

- `--output` replaces the file the scene's `Display` names. Use it to render one scene to
  several files without editing it.
- A relative file name is written in the directory moya was started in, which is moya's own.
  Give a full path to write somewhere else.
- With neither `--output` nor a `Display` of type `"file"`, moya renders and writes nothing.
  The two scenes in `moya/data/`, [test-scene-01.rib](../../../moya/data/test-scene-01.rib) and
  [raytrace-scene.rib](../../../moya/data/raytrace-scene.rib), name no `Display`, so render
  them with `--output`.
- A `Display` of any type other than `"file"` writes nothing.
- moya reads none of `Display`'s parameters, and logs each one it is given.

## Choose the format

The file name's extension sets the format:

| Extension | Format |
|---|---|
| `.png` | PNG |
| `.jpg`, `.jpeg` | JPEG |
| `.tga` | Targa |
| `.bmp` | Windows bitmap |

Upper and lower case are the same. A file with any other extension is not written, and the log
holds an `[error]` that says so. moya still exits with success, so check the log.

The file always holds red, green and blue, eight bits each, whatever mode `Display` names. It
holds no alpha channel and no depth. A colour above 1 is written as 1, and one below 0 as 0.

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

The picture keeps its full size. The pixels outside the window are black. Use it to look at one
part of a large picture without waiting for the rest. The ray-trace hider casts no rays outside
the window, so it saves time there.

## Fill the background

Where nothing is drawn, a pixel is black. The `background` imager paints a colour behind the
scene:

```
Imager "background" "background" [0.55 0.65 0.8]
```

It fills each pixel in proportion to how much of it the scene leaves uncovered, so edges blend
into it. Put it before `WorldBegin`.
