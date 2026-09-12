# Testing

Boost.Test, one binary per api library from `api/<lib>/tests/`, plus one per app that has logic
worth covering. The binaries are registered with ctest and run in CI on every push by
[.github/workflows/ctest.yml](../.github/workflows/ctest.yml).

```
ninja -C out/build/x64-Debug                       # tests build with everything else
ctest --test-dir out/build/x64-Debug --output-on-failure
ctest --test-dir out/build/x64-Debug -R image      # one suite
out/build/x64-Debug/api/image/tests/v3dtest_image.exe --run_test=texture_test
```

`v3d_add_test(<lib> <sources>)` builds `v3dtest_<lib>`, links the framework, and adds the ctest
entry with the working directory beside the executable so a suite's fixtures resolve. Link the
library under test yourself in `api/<lib>/tests/CMakeLists.txt`. `TestMain` carries the
`BOOST_TEST_MODULE` define and nothing else - except `render_device`'s, which needs a `main` of
its own so that it can decide whether to run at all.

`add_test` passes `--detect_memory_leaks=0`. Boost.Test otherwise reports a permanent false
positive for any suite that builds a `Logger`, because spdlog's registry outlives the report.

## What is covered

Everything except what needs a window or a sound device, and the beginnings of what needs a
GPU. `Feature::Window`, `audio::Engine::initialize()` and `ui::TextRenderer` are uncovered and
stay that way: a software Vulkan implementation answers none of them.

**`api/render` below the recorder has its own binary, `v3dtest_render_device`**, from
`api/render/tests/device/`. It draws for real - a surface-free device
([ADR-0007](adr/0007-ci-rendering-tests.md)), a `DeviceContext` with no window under it
([ADR-0051](adr/0051-the-in-flight-ring-is-not-the-swapchain.md)), a frame recorded into a
`RenderTarget`, and `vulkan::frame::Capture` reading it back
([ADR-0050](adr/0050-a-frame-is-read-back-in-two-calls.md)). Each case asserts both halves:
that the validation layer had nothing to say, and that the pixels are what was drawn. What a
case compiles rather than draws is here for the same reason - a pipeline shape no renderer in
this tree builds needs a device to reject it.

It is a second binary rather than more cases in `v3dtest_render`, because that one must keep
running where there is no GPU. **A run with no device exits 77 and ctest reports the suite as
`Skipped`**, which `set_tests_properties(render_device PROPERTIES SKIP_RETURN_CODE 77)` is
what arranges. The probe is in `main` rather than a per-case skip on purpose: a binary whose
every case skipped exits zero and reads as a pass, which is the same trap as a validation layer
that was never installed reporting no errors. CI installs lavapipe per ADR-0007, so a skip there
is a failure rather than a pass: locally a machine may have no device, but the runner was given
one.

`ctest -N` lists what exists, and the test sources are the record of what each suite asserts. A
change with a testable cpu half is expected to bring cases with it.

Two seams keep the api libraries testable without a window, and both are worth preserving:
`ComponentRenderer` takes text measuring and writing as callbacks instead of depending on the
font library, and a strip is hit tested against the bounds a draw left on it, per
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md). `api/grid` and
`api/render/offline` name no device at all, so their suites run in CI where the realtime stack
cannot. The same is true of an app's own rules: `odyssey`'s suite covers its map format, the
route across it and how far sight reaches over it, and stands up neither a window nor a device
to do it.

All three canvases are cpu side and are covered as such: `CanvasTest`, `LineCanvasTest` and
`WorldCanvasTest` assert the batching, the transform stack and the geometry without a device.
What none of them can assert is what the pipeline then does with it - that a world quad is
hidden behind solid geometry and never behind another world quad, per
[ADR-0042](adr/0042-a-textured-quad-in-world-space.md), is a run-and-look check like every
other question below the recorder.

Clipping is asserted where it is decided rather than where it takes effect: the cases check the
rectangle a batch carries out of `Canvas`, out of `LineCanvas` and out of a ui draw, per
[ADR-0037](adr/0037-clipping-is-a-scissor-the-batch-carries.md), and the `vkCmdSetScissor` that
acts on it is in the recorder and needs a device like everything else there.

`api/audio` draws the same line around the device: the clip table, the `Play` defaults and the
voice bookkeeping are asserted, and whether a sound is audible is not. An engine that opened no
device gives back no voice, so every case in `EngineTest` runs in CI.

Input is asserted the same way, against the boxes a draw left: `CursorTest` presses and moves,
`TextBoxTest` types, and neither needs a window because `ui::Cursor` and `ui::Keys` are handed
a point and a key name rather than an SDL event.

`Engine::eventLoop()` renders and so cannot be driven at all, which is why the order of
[ADR-0043](adr/0043-an-app-sees-an-event-before-the-bindings-do.md) lives in
`Engine::route()`: one polled event offered to the app, the bindings and the engine, callable
from a subclass with no window in sight. `EngineTest` drives it directly.

## Suites with something to know about them

- **The moya and talyn suites each render against a committed PNG**, in `moya/tests/data/` and
  `talyn/tests/data/`. They compare with `image::compare`, which reports the worst pixel and by
  how much rather than only that two images differ. A failing case, or a missing reference,
  writes what it rendered to `data_out/` beside the executable. That is also how a reference is
  regenerated when a change is meant to alter the picture. **Each PNG has a `.rib` beside it
  describing the same scene**, so the file path and the code path are pinned to one picture and
  a divergence between them fails.
- **Voxel's suite must link `libnoise`.** `Chunk` is built against a `TerrainMap`, and the
  vtable of the flat one a test supplies refers to the perlin implementation whether or not a
  case generates noise.
- **A round trip cannot see a symmetric orientation fault.** A writer and a reader that both
  reverse their rows return the image they were given, so
  `imagewriter_jpeg_orientation_test` goes through libjpeg directly on one side of each check.
- **Every image reader decodes from memory, and the path form is written in terms of it**, so
  `imagereader_buffer_matches_the_path` reads each fixture both ways and compares them pixel
  for pixel. That is the case that would catch a format whose two entry points drifted apart -
  which cannot happen while there is only one, and is why there is only one.
- **`GltfTest` needs two fixtures because a texture arrives two ways.**
  `three_primitives.glb` names its image and `embedded_texture.glb` carries pixel.png in a
  bufferView; each is generated by the script beside it in `api/asset/tests/data/`.

## Verifying a rendering change

CI renders, against lavapipe on the runner per ADR-0007. What it renders is two cases, a clear
and a quad, so a change below the recorder is still verified by running the app and reading the
log: the suite catches a frame that cannot be drawn or read back at all, not a frame that is
drawn wrongly.

The Khronos validation layer is enabled when installed and `vulkan::Instance` routes it through
the logger, so a silent run is the signal. Without that messenger a loaded layer is silent,
which looks exactly like a clean run. `Instance::errors()` and `firstError()` are the same thing
counted, which is what `render_device` asserts on rather than scraping the log.

**Synchronization validation is off by default and is a separate net.** Set
`VK_LAYER_VALIDATE_SYNC=1` in the environment to turn it on. It reports hazards ordinary
validation does not: a barrier whose first scope misses the stage a semaphore is waited at, a
present that is not ordered after the transition into `PRESENT_SRC`, or a layout transition
whose first scope names a stage and no access bit, so the write it performs is not ordered
after the last frame's write to the same image. All three were in the tree and are fixed. Run
it after touching a barrier, a layout or a semaphore stage, because nothing else sees them.
