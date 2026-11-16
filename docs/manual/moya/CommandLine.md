# The command line

```
moya.exe --file <scene.rib> [--output <image>] [--width <n> --height <n>] [--silent]
         [--checkpoint <file> [--checkpoint-interval <seconds>] [--resume]] [--denoise]
moya.exe --file <scene.rib> --scene
moya.exe --file <scene.rib> --hits [--output <image.exr>]
moya.exe --maketexture <image> <texture> [--swrap <mode>] [--twrap <mode>] [--filter <name>]
         [--swidth <n>] [--twidth <n>] [--colorspace <space>]
moya.exe --help
moya.exe --version
```

## Where moya reads and writes

moya runs from any directory. Run it from a folder you can write to, such as one that holds your
scenes, and give `moya.exe` by its full path, such as `"C:\Program Files\Vertical3D\moya.exe"`.
In the Flatpak, run `flatpak run --command=moya com.farrcraft.Vertical3D` from a folder in your
documents, the only folder the Flatpak can see.

Relative paths are read from the folder you run moya from: the scene file, the output file, and
the directories on a search path. Give full paths to work with files elsewhere.

## Options

| Option | Meaning |
|---|---|
| `--file <path>` | The RIB scene to render. Without it, moya prints the help and exits |
| `--output <path>` | The image to write, in place of the file the scene's first `Display` names. It has to end in `.exr` or `.png` |
| `--width <n>` `--height <n>` | The picture's size in pixels, replacing the size the scene's `Format` names. Give both. Each is from 1 to 65536. The scene's pixel aspect still applies |
| `--silent` | Print no progress line |
| `--scene` | Read the scene and print what it holds, in place of rendering. It prints after each frame the file describes |
| `--hits` | Read the scene and write what its rays hit, in place of rendering, to the `--output` file or to `hits.exr`. The `--output` file has to end in `.exr`. The EXR holds each pixel's `depth`, its surface's `normal`, its `instance`, its `face` and its `coverage`. It is written after each frame, so a file of several frames leaves the last |
| `--checkpoint <file>` | Write a checkpoint of the render to the file after each pass, as an EXR file, so a render that is stopped can continue. A file of several frames leaves the last frame's |
| `--checkpoint-interval <seconds>` | Write the checkpoint at most once in the given seconds, in place of after every pass. The last pass is always written. Needs `--checkpoint` |
| `--resume` | Continue the render the `--checkpoint` file holds, at the pass after its last, and write the same picture a render that never stopped writes. A checkpoint of another scene, or of the same scene changed, is refused. In a file of several frames the first frame resumes, and the frames after it start afresh. Needs `--checkpoint` |
| `--denoise` | Denoise the beauty after the last pass, and write it beside the noisy beauty in each EXR output as `beauty_denoised`, as the scene's options' `denoise` asks |
| `--maketexture <image> <texture>` | Make a texture from an image, in place of rendering, and write it to the texture file as a tiled, mipmapped EXR file. The image is an `.exr`, `.png`, `.jpg`, `.jpeg`, `.tga` or `.bmp` file. A colour image is converted into ACEScg |
| `--swrap <mode>` `--twrap <mode>` | How the texture is read beyond its edge along s and along t: `black`, `clamp` or `periodic`. Default `black` |
| `--filter <name>` | The filter each smaller level of the texture is made with: `box`, `triangle`, `catmull-rom`, `gaussian` or `sinc`. Default `box` |
| `--swidth <n>` `--twidth <n>` | The filter's width along s and along t, in texels of the level being made, above zero and at most 64. Default 1, which with the box averages two by two texels |
| `--colorspace <space>` | The colour space the image is read in, by its name in the ACES configuration, such as `Raw` for an image that holds data and is not converted. By default an EXR file is in the space its chromaticities name, and any other image is in sRGB |
| `--help` | Print the options and exit |
| `--version` | Print the version and Pixar's RenderMan copyright notice, and exit |

- `--width` or `--height` alone is refused, and so is a side outside its range.
- `--scene` and `--hits` are refused together. Each takes its size from the scene's `Format` and
  renders no picture, so `--width`, `--height`, `--checkpoint`, `--checkpoint-interval`,
  `--resume` and `--denoise` are refused with either. `--output` is refused with `--scene`, which
  writes no file. With `--hits`, an `--output` that does not end in `.exr` is refused, and
  otherwise one that ends in neither `.exr` nor `.png`.
- `--checkpoint-interval` and `--resume` are refused without `--checkpoint`, and an interval below
  zero is refused. A `--resume` whose checkpoint is missing or belongs to another scene prints
  `error:` and the reason, and moya exits with failure.
- Each refusal above prints a line that says which option was refused, and moya exits with
  failure.
- `--maketexture` is refused with `--file`, `--output`, `--denoise` and every option that renders,
  and has to name two files. `--swrap`, `--twrap`, `--filter`, `--swidth`, `--twidth` and
  `--colorspace` are refused without it, and so is a wrap mode or filter not in its list, or a
  width that is not above zero and at most 64. moya prints a line that says which, and exits with
  failure. A texture that cannot be made, from an image that does not read or in a colour space
  the configuration lacks, prints `error making the texture - ` and the reason, and moya exits
  with failure.
- An option moya does not recognise, or one with a value that is not a number where a number is
  needed, prints `error:` and the reason, and moya exits with failure.

## What moya prints

- **The RenderMan copyright notice**, before every render, even with `--silent`. moya is not a
  RenderMan compliant renderer, and a renderer that is not compliant prints Pixar's notice.
- **`Rendering scene file: <path>`** before it starts, unless `--silent` is given.
- **`error reading rib file - <reason>`** when the scene cannot be opened or does not parse. The
  reason gives the line and column. moya exits with failure and writes no picture.
- **`error writing the picture - <path>`** when a picture could not be written, once the whole
  scene is read. The log names each file that was not written. moya exits with failure.

## What moya writes

moya renders each frame with its path tracer when it reads the frame's `WorldEnd`, and exits once
the whole file is read.

- **Every file the scene's `Display` requests name**, each as its name says: a `.png` file
  through the display, and any other as EXR, which holds the beauty in ACEScg with the channels
  its mode names. `--output` replaces the first `Display`'s file.
- **A scene that names no `Display`** writes its beauty as floats to the `--output` file, or to
  an EXR file named after the scene in the folder you run moya from. `spheres.rib` writes
  `spheres.exr`.
- **A file of several frames** writes its files after each frame, so it leaves the last frame's.

## The log

moya writes its messages to `v3d.log`, in the folder you run it from. Each run adds to the
end of the file.

| Level | Means |
|---|---|
| `[info]` | How long the render took: `rendered 320 by 240 pixels in 5.570 s` |
| `[warning]` | Something in the scene moya skipped or changed, such as an unknown request, a parameter with no type, or a shader moya does not have |
| `[error]` | Something that failed, such as a picture that could not be written |
| `[debug]` | Detail, such as which file a texture was read from |

A warning or an error does not stop the render. moya still exits with success, unless the
error is a picture that could not be written. Read the log when a picture does not look as you
expect.

## Exit status

moya exits with 0 when it read the whole scene and wrote every picture, or made the texture
`--maketexture` names. It exits with 1 when the scene could not be read or parsed, a picture could
not be written, a texture could not be made, or the command line was wrong.
