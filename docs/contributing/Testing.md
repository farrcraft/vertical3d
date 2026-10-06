# Testing

This document is for contributors. It explains how to run the tests, how the suites are
organised, how to write a test, and how to check a rendering change, including the tests that
draw on a GPU.

- [Running the tests](#running-the-tests)
- [How the suites are organised](#how-the-suites-are-organised)
- [Writing a test](#writing-a-test)
- [The render device suite](#the-render-device-suite)
- [Reference images](#reference-images)
- [Verifying a rendering change](#verifying-a-rendering-change)
- [Traps](#traps)

## Running the tests

The tests use Boost.Test. Each suite is one executable, registered with ctest, and they build
with everything else.

```
ninja -C out/build/x64-Debug                       # builds the tests too
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite, by name
ctest --test-dir out/build/x64-Debug -N            # list the suites
```

`ctest` needs a developer environment. From a plain shell, `scripts\test.cmd` runs the same
`ctest` command and passes any arguments through, so `scripts\test.cmd -R image` runs one suite.

To run one test case, run the suite's executable directly with Boost.Test's `--run_test`:

```
out/build/x64-Debug/api/image/tests/v3dtest_image.exe --run_test=compare_identical_test
```

CI runs every suite in [.github/workflows/ctest.yml](../../.github/workflows/ctest.yml) on each
pull request and on each push to `main`.

## How the suites are organised

- **One suite per api library**, in `api/<lib>/tests/`, built as `v3dtest_<lib>`. A library nested
  inside another one's directory has its own suite: `asset_media` is in
  `api/asset/media/tests/` and `render_offline` is in `api/render/offline/tests/`.
- **One extra suite for the realtime renderer on a real device**, `render_device`, from
  `api/render/tests/device/`. See [The render device suite](#the-render-device-suite).
- **One suite per app that has logic worth testing**: moya, odyssey, pong, tetris, vertical3d
  and voxel, each in `<app>/tests/`.

`ctest -N` lists every suite. The test sources are the record of what each suite checks. A change
with a testable CPU-side part is expected to add test cases.

### What has no automated test

These have no automated test:

- `Feature::Window`, which opens the SDL window. A CI runner has no display.
- `audio::Engine::initialize()`, which opens the audio device. The rest of `api/audio` is tested,
  but not whether a sound is audible. An engine with no device gives back no voice, so every
  case but one runs without a device. `audio_engine_lets_the_dispatcher_go_test` needs SDL's
  audio subsystem to start, and fails on a machine where it cannot.
- The upload in `ui::paint::TextRenderer`, which needs a device. Its measuring and layout are
  tested with an upload that returns a handle and draws nothing.

`Engine::eventLoop()` renders, so a test cannot drive it. The order in which an event reaches the
app, the bindings and the engine is in `Engine::route()`, which a test calls directly with no
window.

## Writing a test

Add the sources to the suite's `tests/CMakeLists.txt`:

```cmake
v3d_add_test(image
	"TestMain.cxx"
	"CropTest.cxx")
target_link_libraries(v3dtest_image PRIVATE v3dlib_image)
```

`v3d_add_test(<lib> <sources>)` does the following:

- Builds `v3dtest_<lib>` and links Boost.Test.
- Adds the repository root to the include path, so a test includes its subject as
  `<api/<lib>/...>` like any other file.
- Registers the ctest entry `<lib>`, with the working directory set to the executable's
  directory. Fixtures copied beside the executable then resolve by a relative path.
- Passes `--detect_memory_leaks=0`. Without it, Boost.Test reports a false leak in every suite
  that builds a `Logger`, because spdlog's registry is destroyed after the report.

You link the library under test yourself, and every other library whose header the suite
includes, rather than reaching it through the library under test. Each suite's `TestMain`
defines `BOOST_TEST_MODULE` and nothing else. The exception is `render_device`, whose `main`
checks for a device first.

A suite with fixture files copies a directory of them beside the executable in a `POST_BUILD`
command, which the suite's `tests/CMakeLists.txt` writes. Most suites copy their `tests/data/`
to `data/`. Three copy something else:

- `engine` copies `api/engine/tests/fixtures/` to `fixtures/`.
- `asset_media` copies the asset suite's `api/asset/tests/data/` to `data/`.
- `render_device` copies `api/render/tests/device/data/` to `data/`.

See [Traps](#traps) for what the copy means when you add a fixture.

### Patterns the suites use

- **Test the decision, not the effect.** For example, the 2D clipping tests check the scissor
  rectangle that a batch carries out of `Canvas`, `LineCanvas` or a UI draw. The
  `vkCmdSetScissor` call that applies it needs a device.
- **Replace a platform dependency with a callback.** `ComponentRenderer` takes text measuring and
  text drawing as callbacks. `TextBoxTest` measures every character as ten pixels and keeps its
  clipboard in a `std::string`. Input tests hand `ui::Cursor` and `ui::Keys` a point or a key
  name rather than an SDL event.
- **Seed every random number.** `type::Random` gives the same sequence on any standard library,
  so particle and weather tests assert exact values.
- **Check an invariant, not a layout.** `textureatlas_regions_do_not_touch` fills every region
  and then checks that the texels around each one are still zero. The test passes wherever the
  packer puts the regions.
- **A round trip cannot find a symmetric fault.** A writer and a reader that both flip rows
  return the image they were given. `imagewriter_jpeg_orientation_test` calls libjpeg directly
  on one side of each check.
- **Round-trip through the file format.** `sprite_sheets_round_trip_test` serializes the
  document, parses the text again, and then loads it. This catches a key written under the wrong
  name or a number that changed type.
- **Write a picture for a person to look at** when a frame cannot be checked exactly. Write it to
  `data_out/` beside the executable, and assert what can be asserted: validation silence and a
  few properties of the picture.

## The render device suite

`v3dtest_render_device` tests the realtime renderer on a real Vulkan device. Its cases create a
device with no window surface, record frames into an offscreen render target, and read the pixels
back with `vulkan::frame::Capture`. Every case asserts two things:

- The validation layer reported nothing. The test reads the count from
  `vulkan::device::Instance::errors()` and `firstError()` rather than from the log.
- The pixels or values are what was drawn. A few cases compare against a committed reference
  image (see [Reference images](#reference-images)). The rest check chosen pixels or values by
  hand.

Some cases build a pipeline rather than drawing, because only a device can reject a pipeline
shape that no renderer in the tree uses.

Cases that cannot be checked exactly, such as a lit and shadowed scene, a skinned mesh mid-clip,
or a particle effect, assert validation silence and a few properties. They write what they drew to
`data_out/` (for example `lit_scene.png`, `skinned_bend.png`, `fire_*.png`) for a person to
inspect.

### Where it runs

- **In CI**, the runner has no GPU. CI installs lavapipe, Mesa's software Vulkan driver, and
  points the Vulkan loader at it. Every case in the suite runs there, with synchronization
  validation on (`VK_LAYER_VALIDATE_SYNC=1`).
- **Locally**, the suite runs on your GPU.

### When there is no device

`main` checks for a usable device before Boost.Test starts. With no device, the executable exits
with code 77, and ctest reports the suite as `Skipped` (set by
`set_tests_properties(render_device PROPERTIES SKIP_RETURN_CODE 77)`).

The check is in `main` rather than in each case because a suite whose every case skips exits
with zero, and ctest would report a pass.

**In CI a skip is a failure.** A separate workflow step runs the executable again and fails the
job on exit code 77, because lavapipe was installed and the loader should have found it. Locally
a skip is correct, since a machine may have no Vulkan device.

This is a separate executable from `v3dtest_render`, which tests the renderer's CPU-side code,
so that `v3dtest_render` runs on any machine.

Background: [ADR-0007](../adr/0007-ci-render-tests-on-software-vulkan.md)

## Reference images

### Realtime references

`api/render/tests/device/data/` holds four reference images: `quad.png`, `textured_quad.png`,
`world_near_first.png` and `world_far_first.png`. A case compares its capture with
`checkReference()`, at a tolerance of zero.

A reference must be identical on every conformant Vulkan driver, including lavapipe in CI and
your GPU locally. So a reference may contain only output that the Vulkan specification fixes
exactly:

- Geometry that is axis-aligned and lies on whole-pixel boundaries, so no pixel is partly
  covered.
- Colour channels of exactly 0.0 or 1.0, or texels sampled at one texel per pixel.
- Blending only where the result equals the source, such as an opaque source.

A reference must not contain any of these, because the specification allows drivers to differ
on them:

- A partly covered pixel.
- Partial alpha.
- A magnified or minified texture sample.
- A multisample resolve.
- An interpolated channel value that is not at 0.0 or 1.0.

A texture and a render target are read through a linear sampler, the default
`vulkan::pipeline::Sampler::Spec`. A textured reference must therefore draw its texture at
exactly one texel per pixel. Only two samplers use nearest filtering: the one `Grade` reads the
scene through, and the one a sampled `DepthBuffer` is read through. A case that draws through
those is still bound by the rules above.

A case that needs anything outside these rules checks chosen pixels by hand and has no
reference.

`checkReference()` writes what was drawn to `data_out/<name>.png` whether it matched or not. To
change a reference on purpose, copy that file over the committed one. The test never writes to
`data/`.

Background: [ADR-0054](../adr/0054-testing-golden-images-hold-only-spec-exact-output.md)

### moya references

The moya suite renders scenes on the CPU and compares them with committed PNGs in
`moya/tests/data/`:

- `reference-*.png` are rendered with the Reyes hider.
- `raytrace-*.png` are rendered with the ray tracing hider.
- Each PNG has a `.rib` file beside it that describes the same scene. The suite renders it from
  the file and from code, so both paths are tied to one picture.

The comparison is `image::compare` at a tolerance of 1, which absorbs floating-point rounding
differences between compilers. It reports the worst pixel and by how much. A failing case, or a
case with no reference yet, writes what it rendered to `data_out/` beside the executable. Copy that
file over the reference to change it on purpose.

## Verifying a rendering change

CI checks the four reference images and every other device case. That does not cover anything
blended, filtered or antialiased, which is most of what the renderer draws. Check a rendering
change locally by running an app and reading its log.

1. **Run the app and read `v3d.log`.** The log is written beside the executable. When the Khronos
   validation layer is installed, the renderer turns it on and sends its messages to the log
   through `vulkan::device::Instance`. A clean run has no validation messages, so a silent log
   is the result you want. Without that routing, a loaded layer prints nothing, which looks the
   same as a clean run.
2. **Run with synchronization validation** after changing a barrier, an image layout or a
   semaphore wait stage. Set `VK_LAYER_VALIDATE_SYNC=1` in the environment. It reports hazards
   that ordinary validation does not, such as:
   - a barrier whose first scope misses the stage that a semaphore is waited at;
   - a present that is not ordered after the transition to `PRESENT_SRC`;
   - a layout transition whose first scope names a stage but no access, so its write is not
     ordered after the previous frame's write to the same image.

   CI runs the test suites with it on. It does not run the apps.
3. **Run the render device suite** on your GPU: `ctest --test-dir out/build/x64-Debug -R
   render_device`.
4. **Look at the picture last.** Screenshots and the `data_out/` images are for checking what
   the log and the tests cannot.

## Traps

- **A new or changed fixture is copied only when its suite relinks.** A suite copies its
  fixture directory in a `POST_BUILD` command, which runs only when the executable is relinked.
  Adding a reference image and rebuilding copies nothing. Touch one of the suite's sources, or
  copy the file by hand into the directory the suite copies to, beside the executable.
  [Writing a test](#writing-a-test) lists which directory each suite copies. A fresh CI
  checkout always relinks.
- **voxel's suite must link libnoise**, even for cases that generate no noise. `Chunk` is built
  against a `TerrainMap`, and the vtable of the flat map a test supplies refers to the Perlin
  implementation. See [Dependencies.md](Dependencies.md#building-libnoise).
- **Run the moya executable from its own directory.** It is
  `out/build/x64-Debug/moya/moya/moya.exe`. Started from another directory it can stop on a
  missing-DLL dialog and use no CPU, which looks like a hung render.
- **A debug moya render is slow.** A 256x192 ray traced scene takes about a minute.
