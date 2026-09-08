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
`BOOST_TEST_MODULE` define and nothing else.

`add_test` passes `--detect_memory_leaks=0`. Boost.Test otherwise reports a permanent false
positive for any suite that builds a `Logger`, because spdlog's registry outlives the report.

## What is covered

Everything except what needs a window, a GPU or a sound device. That leaves `api/render` below
the recorder, `Feature::Window`, `audio::Engine::initialize()` and `ui::TextRenderer`
uncovered, all of them waiting on [ADR-0007](adr/0007-ci-rendering-tests.md), which would
render against a software Vulkan implementation on a CI runner.

`ctest -N` lists what exists, and the test sources are the record of what each suite asserts. A
change with a testable cpu half is expected to bring cases with it.

Two seams keep the api libraries testable without a window, and both are worth preserving:
`ComponentRenderer` takes text measuring and writing as callbacks instead of depending on the
font library, and a strip is hit tested against the bounds a draw left on it, per
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md). `api/grid` and
`api/render/offline` name no device at all, so their suites run in CI where the realtime stack
cannot. The same is true of an app's own rules: `odyssey`'s suite covers its map format and the
route across it, and stands up neither a window nor a device to do it.

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

Input is asserted the same way, against the boxes a draw left: `CursorTest` presses and moves,
`TextBoxTest` types, and neither needs a window because `ui::Cursor` and `ui::Keys` are handed
a point and a key name rather than an SDL event.

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

## Verifying a rendering change

CI renders nothing, so a change below the recorder is verified by running the app and reading
the log. The Khronos validation layer is enabled when installed and `vulkan::Instance` routes
it through the logger, so a silent run is the signal. Without that messenger a loaded layer is
silent, which looks exactly like a clean run.

**Synchronization validation is off by default and is a separate net.** Set
`VK_LAYER_VALIDATE_SYNC=1` in the environment to turn it on. It reports hazards ordinary
validation does not: a barrier whose first scope misses the stage a semaphore is waited at, or
a present that is not ordered after the transition into `PRESENT_SRC`. Both of those were in
the tree and are fixed. Run it after touching a barrier, a layout or a semaphore stage, because
nothing else sees them.
