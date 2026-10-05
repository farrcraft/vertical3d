# Running and testing moya

How to run moya on a scene, and how to build and test it.

## Running moya

moya is three CMake targets, and it is built on one api library:

| Directory | Target | Contents |
|---|---|---|
| `moya/libmoya` | `v3dlib_moya` | The renderer: graphics state, both hiders, the RI C API |
| `moya/moya` | `moya` | The command-line driver |
| `moya/tests` | `v3dtest_moya` | The test suite, ctest name `moya` |
| `api/render/offline` | `v3dlib_render_offline` | The RIB reader, the shading language, sampling and the film, textures and the ray tracer. Namespace `v3d::render::offline` |

Run the driver from its own build directory:

```
cd out/build/x64-Debug/moya/moya
moya.exe --file data/raytrace-scene.rib --output scene.png
```

| Option | Meaning |
|---|---|
| `--file <path>` | The RIB scene to render, relative to the working directory. Without it, moya prints the help and exits |
| `--output <path>` | The image to write. It replaces whatever file the scene's `Display` names |
| `--grid <n>` | Micropolygons per grid. Default 256 |
| `--bucket <n>` | Bucket size, n by n pixels. Default 16 |
| `--help` | Print the options |
| `--version` | Print the version and Pixar's RenderMan copyright notice |

- **Run moya from its own directory.** Started anywhere else, Windows cannot find its DLLs and
  shows a modal dialog. The process then waits on the dialog and uses no CPU.
- **Two demo scenes are copied beside the executable** at build time, from `moya/moya/data/`:
  `test-scene-01.rib` (two quads under the reyes hider) and `raytrace-scene.rib` (a metal and a
  glass sphere under the ray hider). Neither names a `Display`, so pass `--output` to get a file.
- **moya writes a file only when one is named**, by `--output` or by
  `Display "name.png" "file" "rgb"`. A `Display` of any other type writes nothing.
- **The output format comes from the file extension**: bmp, jpeg, png or tga, written through
  `image::Factory`. The image is always three colour channels, whatever mode `Display` names.
- **The picture is written when `WorldEnd` is read**, as the RI standard specifies.
- **`--grid` and `--bucket` are applied before the scene is read.** An `Option "limits"` in the
  scene replaces them.
- A scene that does not parse prints `error reading rib file` with the reason and exits with a
  failure code. Any exception is printed to stderr.
- A Debug build is slow. A 256 by 192 ray traced scene takes about a minute.

moya is not RenderMan compliant, and does not aim to be. `moya/libmoya/Renderer.h` explains
why: a second API beside the RI C API breaks the standard's "one true API" clause.

## Building and testing

- **`api/render/offline` is added by `api/CMakeLists.txt`** from the library manifest, like
  every api library. It inherits nothing from the `api/render` directory above it, so it sets
  `/utf-8` in its own CMakeLists. spdlog's bundled fmt has a `static_assert` that fails without
  that flag.
- **`v3dlib_render_offline` links `v3dlib_log`, `v3dlib_image`, `v3dlib_type` and glm**, and
  never Vulkan or SDL. A realtime dependency arriving through one of those libraries breaks its
  build, and its CI suites, first.
- **Two suites cover the offline code**: `render_offline` tests the library alone, and `moya`
  tests the renderer. Run either with `ctest -R`.
- **The moya suite renders against committed PNGs** in `moya/tests/data/`: `reference-*` under
  the reyes hider and `raytrace-*` under the ray hider. Each PNG has a `.rib` beside it
  describing the same scene, so the file route and the code route are pinned to one picture.
- **A failing case writes what it rendered to `data_out/`** beside the test executable. That is
  also how a reference is regenerated when a change is meant to alter the picture.
- **Fixtures are copied only when the test target relinks.** Editing a `.rib`, `.sl` or `.png`
  alone leaves the old copy in place, and the suite tests the old file. Touch a source file of
  the suite, or copy the fixture into `out/build/x64-Debug/moya/tests/data/` by hand.
- **To look at a reference at a useful size**, copy its `.rib` to a scratch directory, raise its
  `Format`, give it an absolute shader search path, and render it with the driver from the
  driver's directory.

[Testing.md](../contributing/Testing.md) covers the test framework, image comparison and the rest
of the suites.
