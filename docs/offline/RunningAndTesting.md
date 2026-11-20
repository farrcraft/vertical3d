# Running and testing moya

How moya is laid out, and how to build and test it.

## Running moya

moya is three CMake targets, and it is built on one api library:

| Directory | Target | Contents |
|---|---|---|
| `moya/libmoya` | `v3dlib_moya` | The renderer. At its root, the directory of the running executable, beside which the CPU device and the material library find their files. Namespace `v3d::moya` |
| `moya/libmoya/ri` | `v3dlib_moya` | The RenderMan Interface: the scene client that RIB requests reach. Namespace `v3d::moya::ri` |
| `moya` | `moya` | The command-line driver, `moya.cxx`, and its demo scenes in `moya/data/` |
| `moya/tests` | `v3dtest_moya` | The test suite, ctest name `moya` |
| `api/rib` | `v3dlib_rib` | RIB: the lexer, reader, writer and recorder, the handler they call, and the names and limits of the sampling requests. Namespace `v3d::rib` |
| `api/rib/source` | `v3dlib_rib` | What the RIB lexer reads through: the character stream, a token's kind and text, and its position. Namespace `v3d::rib::source` |

The command-line driver, its options, what it prints and its log are in the user manual's
[CommandLine.md](../manual/moya/CommandLine.md). The rules a developer needs when running it:

- **Run moya from its own directory**, `out/build/x64-Debug/moya`. Started anywhere else, Windows
  cannot find its DLLs and shows a modal dialog. The process then waits on the dialog and uses
  no CPU.
- **Two demo scenes are copied into `data/moya/` beside the executable** at build time, from
  `moya/data/`: `test-scene-01.rib` (two quads) and `raytrace-scene.rib` (a metal and a glass
  sphere). Neither names a `Display`, so each writes its beauty to an EXR file named after it in
  the working directory, unless `--output` names another.
- **The driver renders through the scene client and the path tracer**, as `--scene` and `--hits`
  read through the scene client. `--width` and `--height` set the scene client's resolution
  before the scene is read, so they win over the scene's `Format`.
- A Debug build is slower than a Release build. It renders `raytrace-scene.rib` at 320 by 240 in
  about 25 seconds.

moya is not RenderMan compliant, and does not aim to be. It prints Pixar's copyright notice before
every render, as a program that is not compliant must.

### Inside another program

A program that links `v3dlib_moya` holds a `scene::Scene` of its own and renders it with
`integrate::render()`. Nothing one scene holds is shared, so two renders of two scenes on two
threads do not meet.

- **A RIB stream fills the scene through a `ri::SceneClient`**, as the driver does. The function
  the client's `frame()` is given runs after each `WorldEnd`. The driver applies the scene there,
  renders it, and writes each of its outputs.
- **The editor fills its scene from its own document**, with no RIB between, as
  [editor/Rendering.md](../editor/Rendering.md) says.
- **A `Display` of type `framebuffer`** writes no file. The picture is the snapshot
  `integrate::render()` returns.
- **`integrate::passesOf(scene)`** gives the passes a render takes when no pixel stops early. The
  `stop` function of the `integrate::Checkpoints` a render is given is asked before each pass,
  with the passes the film holds, so a caller reports progress from it. It runs on the rendering
  thread.
- **A `stop` that answers true ends the render** there. The render writes its checkpoint, if it
  keeps one, and returns the snapshot of the passes it took, as
  [Integrator.md](Integrator.md#checkpoint-and-resume) says.

## Building and testing

- **`api/rib` is added by `api/CMakeLists.txt`** from the library manifest, like every api
  library.
- **`v3dlib_rib` links `v3dlib_log`, `v3dlib_type` and glm**, and never Vulkan or SDL. A realtime
  dependency arriving through one of those libraries breaks its build, and its CI suites, first.
- **Two suites cover the offline code**: `rib` tests the RIB library alone, and `moya` tests the
  renderer. Run either with `ctest -R`. `v3dfuzz_rib` fuzzes the reader, with the `rib` suite's
  fixtures in `api/rib/tests/data/` as its corpus.
- **The moya suite renders against committed EXRs** in `moya/tests/data/`, each the path
  tracer's render of the `.rib` beside it, compared within five standard errors.
- **`foundation-chart.png` is moya's chart**: 24 colours with no geometry, sampled
  through the filter, the sampler and the spectrum table, and shown through the sRGB display. It
  has no `.rib`, since the chart has no geometry for a RIB file to describe. An RGB build draws
  no wavelengths, so its suite leaves this case and the EXR references out.
- **A failing case writes what it rendered to `data_out/`** beside the test executable. That is
  also how a reference is made again when a change is meant to alter the picture.
- **Fixtures are copied only when the test target relinks.** Editing a `.rib`, `.mtlx` or `.exr`
  alone leaves the copy in the build in place, and the suite tests that copy. Touch a source file of
  the suite, or copy the fixture into `out/build/x64-Debug/moya/tests/data/` by hand.
- **To look at a reference at a useful size**, copy its `.rib` to a scratch directory, raise its
  `Format`, and give it an absolute shader search path. Render it with the driver from the
  driver's directory.

[Testing.md](../contributing/Testing.md) covers the test framework, image comparison and the rest
of the suites.
