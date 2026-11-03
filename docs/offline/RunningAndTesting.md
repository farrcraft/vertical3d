# Running and testing moya

How moya is laid out, and how to build and test it.

## Running moya

moya is three CMake targets, and it is built on one api library:

| Directory | Target | Contents |
|---|---|---|
| `moya/libmoya` | `v3dlib_moya` | The renderer. At its root, what both hiders share: the `Hider` base, and the polygons, vertices and curves they draw. Namespace `v3d::moya` |
| `moya/libmoya/ri` | `v3dlib_moya` | The RenderMan Interface: the RI C API, the handler RIB requests and C calls reach, and the C interface's stack of handlers. Namespace `v3d::moya::ri` |
| `moya/libmoya/state` | `v3dlib_moya` | The graphics state: the render context and each part of the state it holds. Namespace `v3d::moya::state` |
| `moya/libmoya/raytrace` | `v3dlib_moya` | The ray hider and the scene its rays meet. Namespace `v3d::moya::raytrace` |
| `moya/libmoya/reyes` | `v3dlib_moya` | The reyes hider's pipeline: buckets, dicing, grid shading and the samples. Namespace `v3d::moya::reyes` |
| `moya` | `moya` | The command-line driver, `moya.cxx`, and its demo scenes in `moya/data/` |
| `moya/tests` | `v3dtest_moya` | The test suite, ctest name `moya` |
| `api/render/offline` | `v3dlib_render_offline` | What the renderers share at their root: the camera and its options, the moving transform, an area light's geometry, points spread over a shape, and pi. Namespace `v3d::render::offline` |
| `api/render/offline/source` | `v3dlib_render_offline` | What the RIB and SL lexers share: the character stream, a token's kind and text, and its position. Namespace `v3d::render::offline::source` |
| `api/render/offline/film` | `v3dlib_render_offline` | Sampling and the film: the sampling options and pixel filters, the sampler, the film and the framebuffer. Namespace `v3d::render::offline::film` |
| `api/render/offline/texture` | `v3dlib_render_offline` | Textures and noise: a texture, the textures read for a frame, the search path, and the noise function. Namespace `v3d::render::offline::texture` |
| `api/render/offline/rib` | `v3dlib_render_offline` | The RIB lexer, reader and writer, and the handler they call. Namespace `v3d::render::offline::rib` |
| `api/render/offline/sl` | `v3dlib_render_offline` | The shading language at its root: its types, the shader library, and a shader bound and placed in a scene. Namespace `v3d::render::offline::sl` |
| `api/render/offline/sl/syntax` | `v3dlib_render_offline` | The syntax tree, one header for each kind of node. Namespace `v3d::render::offline::sl::syntax` |
| `api/render/offline/sl/compiler` | `v3dlib_render_offline` | Source to program: the lexer, the parser, the type checker, inference, the emitter and the built-in functions' signatures. Namespace `v3d::render::offline::sl::compiler` |
| `api/render/offline/sl/runtime` | `v3dlib_render_offline` | A program and the machine that runs it, the renderer it asks, the globals a hider binds, and the imager. Namespace `v3d::render::offline::sl::runtime` |
| `api/render/offline/trace` | `v3dlib_render_offline` | The ray tracer both hiders share. Namespace `v3d::render::offline::trace` |

The command-line driver, its options, what it prints and its log are in the user manual's
[CommandLine.md](../manual/moya/CommandLine.md). The rules a developer needs when running it:

- **Run moya from its own directory**, `out/build/x64-Debug/moya`. Started anywhere else, Windows
  cannot find its DLLs and shows a modal dialog. The process then waits on the dialog and uses
  no CPU.
- **Two demo scenes are copied beside the executable** at build time, from `moya/data/`:
  `test-scene-01.rib` (two quads under the reyes hider) and `raytrace-scene.rib` (a metal and a
  glass sphere under the ray hider). Neither names a `Display`, so pass `--output` to get a file.
- **The driver pins `--grid` and `--bucket` before the scene is read.** The context leaves a
  pinned size as it is, so the command line wins over the scene's `Option "limits"` while the
  scene is read and rendered.
- A Debug build is slower than a Release build. It renders `raytrace-scene.rib` at 256 by 192 in
  about six seconds.

moya is not RenderMan compliant, and does not aim to be. `moya/libmoya/ri/RIBHandler.h` explains
why: a second API beside the RI C API breaks the standard's "one true API" clause. The first
handler a process makes prints Pixar's copyright notice, as a program that is not compliant
must.

### Inside another program

A program that links `v3dlib_moya` renders a RIB stream with a `RIBHandler` of its own, as the
driver does. Nothing a handler holds is shared, so two renders on two threads do not meet. The
editor renders this way, as [editor/Rendering.md](../editor/Rendering.md) says.

- **A `Display` of type `framebuffer`** writes no file, and the picture stays in
  `context().picture()`.
- **`RenderContext::progress(callback)`** is told how much of the frame is done, from 0 to 1. The
  reyes hider tells it after each bucket and the ray-trace hider after each row. Progress
  reaches 1 at the end of the first pass over the buckets, and holds there through a pass
  repeated because a primitive split. The callback runs on the rendering thread.
- **A callback that returns false stops the render** there. The buckets or rows not yet rendered
  stay empty, no imager runs, no file is written, and `stopped()` says so until the next render.
- The reyes hider gathers every sample of the frame before it renders its first bucket, so a
  stop cannot come sooner than that.

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
  `Format`, and give it an absolute shader search path. Render it with the driver from the
  driver's directory.

[Testing.md](../contributing/Testing.md) covers the test framework, image comparison and the rest
of the suites.
