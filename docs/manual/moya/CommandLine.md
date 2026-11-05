# The command line

```
moya.exe --file <scene.rib> [--output <image>] [--width <n> --height <n>]
         [--grid <n>] [--bucket <n>] [--silent]
moya.exe --help
moya.exe --version
```

## Run moya from its own directory

Change to the directory that holds `moya.exe` before you run it, such as
`out/build/x64-Debug/moya`. Started anywhere else, Windows cannot find the libraries moya needs.
It shows a dialog about a missing DLL, and moya waits on that dialog without rendering.

Relative paths are read from that directory: the scene file, the output file, and the
directories on a search path. Give full paths to work with files elsewhere.

## Options

| Option | Meaning |
|---|---|
| `--file <path>` | The RIB scene to render. Without it, moya prints the help and exits |
| `--output <path>` | The image to write. It replaces the file the scene's `Display` names. Its extension sets the format: `.png`, `.jpg`, `.jpeg`, `.tga` or `.bmp` |
| `--width <n>` `--height <n>` | The picture's size in pixels, replacing the size the scene's `Format` names. Give both. Each is from 1 to 65536. The scene's pixel aspect still applies |
| `--grid <n>` | The number of micropolygons in a reyes grid, from 1 to 65536. Default 256. It wins over the scene's `Option "limits" "gridsize"` |
| `--bucket <n>` | The reyes bucket size, n by n pixels, from 1 to 65536. Default 16. It wins over the scene's `Option "limits" "bucketsize"` |
| `--silent` | Print no progress line |
| `--help` | Print the options and exit |
| `--version` | Print the version and Pixar's RenderMan copyright notice, and exit |

- `--width` or `--height` alone is refused, and so is a side outside its range. moya prints a
  line that says so and uses the scene's size.
- A `--grid` or `--bucket` outside its range is refused. moya prints a line that says so. The
  size then comes from the scene's `Option "limits"`, or is the default.
- An option moya does not recognise, or one with a value that is not a number where a number is
  needed, prints `error:` and the reason, and moya exits with failure.

## What moya prints

- **The RenderMan copyright notice**, every run, even with `--silent`. moya is not a RenderMan
  compliant renderer, and a renderer that is not compliant prints Pixar's notice.
- **`Rendering scene file: <path>`** before it starts, unless `--silent` is given.
- **`error reading rib file - <reason>`** when the scene cannot be opened or does not parse. The
  reason gives the line and column. moya exits with failure and writes no picture.

The picture is written when moya reads the scene's `WorldEnd`. moya exits once the whole file is
read.

## The log

moya writes its messages to `v3d.log`, in the directory it was started in. Each run adds to the
end of the file.

| Level | Means |
|---|---|
| `[info]` | How long the render took: `rendered 320 by 240 pixels in 5.570 s` |
| `[warning]` | Something in the scene moya skipped or changed, such as an unknown request, a parameter with no type, or a sphere under the reyes hider |
| `[error]` | Something that failed, such as a shader that does not compile, or a picture that could not be written |
| `[debug]` | Detail, such as which file a texture was read from |

A warning or an error does not stop the render, and moya still exits with success. Read the log
when a picture does not look as you expect.

## Exit status

moya exits with 0 when it read the whole scene. It exits with 1 when the scene could not be read
or parsed, or the command line was wrong.
