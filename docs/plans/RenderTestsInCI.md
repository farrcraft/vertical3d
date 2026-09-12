# Render Tests In CI — The Device Half Of The Tree, Under A Software Vulkan

Drafted 2026-09-11 against `88711c0`. Six steps across `api/render/realtime` and
`.github/workflows`, building what [ADR-0007](../adr/0007-ci-rendering-tests.md) decided on
2026-08-30 and nothing has since implemented.

## Why now, and why this is the largest hole

`api/render/realtime` is roughly 4,600 lines across 71 files, and it is the newest body of code
in the tree. [api/render/tests/](../../api/render/tests/) has ten files, and every one of them
covers the half that needs no device: the batching in `CanvasTest`, `LineCanvasTest` and
`WorldCanvasTest`, the channel order in `CaptureConvertTest`, the choice in
`SwapchainFormatTest`, the ordering in `SortKeyTest`. Everything below the recorder is asserted
by nothing.

[sdlc.md](../sdlc.md) is explicit about what stands in for those tests today: *"a rendering
change is verified by running the app and reading the log"*, and it names ADR-0007 as the
automation that has not been done. That is a human in the loop for every change to the newest
and least covered code in the repository.

**What raises this above the other open work is [CONTRIBUTING.md](../../CONTRIBUTING.md).** The
argument there is that engine changes must be made and verified *in this tree*, because
`V3D_BUILD_APPS` and `V3D_BUILD_TESTS` follow `PROJECT_IS_TOP_LEVEL` and a consumer reaching
this tree through `add_subdirectory` configures neither the applications nor one suite. That
argument is only as strong as this tree's own coverage, and the renderer is where it is
weakest. A consumer is told to come here to verify, and what they find here cannot verify a
frame.

## What the tree already has

The survey behind this plan found the architecture much closer to headless than
[TODO.md](../TODO.md) suggests. Four of the five pieces a render test needs are already
device-agnostic or window-agnostic, and were built that way for other reasons:

| Piece | State |
|---|---|
| [`device::Instance`](../../api/render/realtime/vulkan/device/Instance.h#L32) | Takes a list of extensions and nothing else. Already knows nothing about a window, and already routes the validation layer through the logger |
| [`frame::RenderTarget`](../../api/render/realtime/vulkan/frame/RenderTarget.h#L56) | Device, extent and format. Needs no swapchain — [ADR-0031](../adr/0031-a-pass-draws-into-a-target-it-names.md) |
| [`frame::Recorder::record`](../../api/render/realtime/vulkan/frame/Recorder.h#L57) | A static function over a `Target` struct of raw handles. No presenter, no chain |
| [`Frame`](../../api/render/realtime/Frame.cpp#L18) and `Pass` | `Frame` holds a `shared_ptr<Context>` and never dereferences it — it stores it and hands it back. The base [`Context`](../../api/render/realtime/Context.h) is an empty class with a virtual destructor |

So the untested code is not structurally window-bound. Three specific couplings are, and they
are what the first three steps cut.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--a-device-selected-without-a-surface) | `device::Device` takes an optional surface | `api/render/realtime` | cites 0007 | ✓ landed |
| [2](#step-2--a-capture-reads-any-image-not-only-a-chains) | `frame::Capture` reads any image, not only a chain's | `api/render/realtime` | cites 0050 | ✓ landed |
| [3](#step-3--the-validation-layer-is-something-a-test-can-assert-on) | A validation sink a test can assert against | `api/render/realtime` | cites 0007 | ✓ landed |
| [4](#step-4--a-context-with-no-window-under-it) | A headless context, and what it costs `Context3D` | `api/render/realtime` | **0051** | ✓ landed |
| [5](#step-5--the-first-suite-that-draws) | The suite: draw offscreen, assert silence, read back | `api/render/tests` | — | drafted |
| [6](#step-6--lavapipe-on-the-runner) | A pinned software ICD, and the guard that skips without one | `.github/workflows` | cites 0007 | drafted |

### What blocks what

Step 1 blocks 4, 5 and 6 and depends on nothing: every headless object needs a device, and a
device today needs a surface. It is the barrier in this plan, and it is the step to do first
even though it is not the largest.

Steps 2 and 3 are independent of step 1 and of each other. Both are small, both are useful to
an app before any test exists — a capture that can read a render target is what an offscreen
pass has wanted since ADR-0031, and a validation sink is worth having in the editor. **They can
land in either order and neither should wait behind the device work.**

Step 4 is the one with genuine design risk and is discussed at length below. Step 5 needs 1, 3
and 4; it needs 2 only for the cases that read a picture back, so a first suite asserting
silence alone can land before 2 does.

Step 6 is last for the reason ADR-0007's fifth alternative gives about golden images: get the
tests passing against a real driver locally first, then make them run on a software one. Doing
it the other way round debugs two unknowns at once — whether the test is wrong, and whether
lavapipe is.

**But step 6's risk should be checked first, out of order.** Whether lavapipe satisfies
[ADR-0002](../adr/0002-target-vulkan-1-3.md) is the one thing here that could invalidate a whole
step after the work is done, and it costs an hour and no code to find out — step 6 says how. Do
that before step 1, then order the rest as above.

**Golden images are not in this plan.** ADR-0007 defers them deliberately and argues they
"should not be the first thing built", because references generated by a software rasterizer
say nothing about hardware behaviour. The TODO's note that nothing blesses or compares a
reference picture is real, and it is the workstream *after* this one. Step 2 is what that
workstream will stand on. Worth knowing when it comes: the machinery already exists in the
offline renderers, where moya and talyn each compare against a committed PNG with
`image::compare` and write what they rendered to `data_out/` on a failure — see
[Testing.md](../Testing.md). A realtime golden image should reuse that convention rather than
invent a second one.

---

## Step 1 — A device selected without a surface

[`Device`](../../api/render/realtime/vulkan/device/Device.h#L51) takes a `Surface` and uses it
in three places that all assume it is there:

- [`findFamilies`](../../api/render/realtime/vulkan/device/Device.cxx#L128) calls
  `vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface_->handle(), ...)` unconditionally,
  so a null surface is a null dereference rather than a device with no present queue.
- `QueueFamilies::complete()` requires both graphics and present, so selection would reject
  every physical device on a machine with no surface to present to.
- `hasRequiredExtensions` requires `VK_KHR_swapchain`, which a headless device should not ask
  for and which a headless ICD is not obliged to advertise.

The shape is a constructor taking a possibly-null surface, `complete()` requiring a present
family only when there is a surface, and the swapchain extension being requested on the same
condition. `surface()` already returns a `shared_ptr` and can return a null one.

The risk to check while doing it: `presentQueue()` currently always returns a valid queue, and
callers may not expect `VK_NULL_HANDLE`. Every caller is inside this library, so the compiler
will not find them — grep rather than trust.

**Landed.** The grep found a smaller blast radius than the step expected: `presentQueue()` has
one caller, `Presenter`'s `vkQueuePresentKHR`, and `Device::surface()` has two, both in
`Swapchain`. All three are on the presenting path and none needed changing, because a headless
context builds neither class. `families()` is read elsewhere only for `.graphics`.

One thing came out differently, and it is the part worth keeping. The required-extension array
was read from two places — `hasRequiredExtensions` checked it and `createLogical` enabled it —
which is an agreement held by both naming the same array, and making the list depend on
presenting would have broken it silently. It is now a `deviceExtensions(bool)` function both
call, so a device cannot be selected on one list and created with another.

Verified against a real driver with a throwaway case, since nothing constructs a headless device
until step 4 and a suite that needs a GPU cannot be committed before step 5 writes its guard: a
device selects with no surface, reports `presenting()` false, has a null present queue and a
valid graphics queue, and the validation layer was enabled and silent throughout — which is what
says the swapchain extension is genuinely not needed rather than merely not asked for.

## Step 2 — A capture reads any image, not only a chain's

[`Capture::record`](../../api/render/realtime/vulkan/frame/Capture.cxx#L81) takes
`const Swapchain&` and an image index, and uses exactly three things from it: `extent()`,
`format()` and `images()[image]`. It also hardcodes `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` as the
layout to transition from and back to.

An overload taking `(VkImage, VkExtent2D, VkFormat, VkImageLayout)` is the whole change, with
the existing signature kept as a thin wrapper so nothing that captures a presented frame has to
move. A `RenderTarget` is left in `SHADER_READ_ONLY_OPTIMAL` rather than `PRESENT_SRC`, which
is why the layout has to become an argument rather than staying a constant.

This is what lets a test read back what it drew without a swapchain in the picture, and it is
the foundation the golden-image workstream will need.

**Landed**, but the step as drafted was wrong about its own size twice, and both are worth
recording because both were found by running it rather than by reading it.

**"An overload is the whole change" was not true: `RenderTarget` could not be captured at all.**
Its colour image is created `COLOR_ATTACHMENT | SAMPLED`, and `vkCmdCopyImageToBuffer` needs
`TRANSFER_SRC`, so the copy this step exists to make possible was invalid on the one class it
was meant to serve. The usage is now granted on the same terms `SAMPLED` already is — neither is
asked for, because a colour image that cannot be read is the narrower thing to be, and on a
desktop driver the cost is whichever compression it declines. Step 3's sink is what caught this,
on the first run, naming the VUID: two steps in, the thing being built is already the thing
finding the defects.

**The barrier either side of the copy could not stay as it was.** `transition()` had two
branches, and the restore branch was written for presentation alone — `BOTTOM_OF_PIPE` with an
empty access mask, correct because the semaphore presentation waits on is what orders it. A
target returning to a layout something in the same submit may sample or draw into has no such
semaphore, so it needs the transition complete and visible before any of them. There is a third
branch now, and the present reasoning is untouched.

`record()` takes a `Capture::Source` rather than four loose arguments, following
`Recorder::Target`, which is the same shape for the same reason. The `Swapchain` overload fills
one in and is otherwise unchanged.

Verified end to end against a real driver with sync validation on (`VK_LAYER_VALIDATE_SYNC=1`):
a pass clears an offscreen target red through the attachment path, the recorder's
`SHADER_READ_ONLY_OPTIMAL` hand-over is captured, and the written png decodes to 16x8 RGBA of
opaque red in every texel. Zero validation errors of either kind. Silence alone would not have
been enough here — the pixels are what say the copy read the right image in the right channel
order, and a capture that quietly read nothing would also be silent.

## Step 3 — The validation layer is something a test can assert on

This is the assertion ADR-0007 chose: *"assert the absence of validation-layer errors rather
than comparing rendered pixels"*. Nothing in the tree can currently make that assertion.
[`Instance::createMessenger`](../../api/render/realtime/vulkan/device/Instance.h#L72) routes
messages to the logger, which is right for a person reading a run and useless to a test, which
would have to scrape a log sink.

What a test needs is a count of messages at error severity, or the messages themselves. The
cheapest shape is a counter on `Instance` alongside the existing `validating()`, incremented by
the messenger. A callback seam is more flexible and more than this needs.

Two things to get right, both of which are traps rather than decisions:

- **`validating()` false must fail a test rather than pass it.** A suite that asserts "no errors
  were reported" passes trivially where the validation layer is not installed. The suite has to
  assert the layer is *on* before it asserts what it said. Instance.h's own comment already makes
  this point about the messenger: a layer with nowhere to report to is silent, and silence is
  indistinguishable from a clean run.
- **Synchronization validation is a separate net and is off by default.** `VK_LAYER_VALIDATE_SYNC=1`
  turns it on, per [Testing.md](../Testing.md), and it catches the class of defect — a barrier
  whose first scope misses a stage, a present not ordered after its transition — that is hardest
  to find by eye and most worth having in CI. Whether CI sets it is a decision for step 6; the
  sink in this step is what makes it observable either way.

**Landed**, as the counters the step recommended: `Instance::errors()`, `warnings()` and
`firstError()`. The first error is kept as well as counted, because a suite that fails on a
count alone reports a number and makes the reader go looking for the message, and keeping every
message for the life of an instance buys nothing over the logger that already has them.

One thing came out differently. The messenger's callback was a free function in an anonymous
namespace taking the logger as its user pointer, and it could not stay one: the user pointer now
has to carry the instance, and the counters it increments are private. It is a private static
member function of `Instance` instead, which is the change that lets it reach both without
making anything public that should not be.

Verified both directions, which matters more here than usual because a sink that silently counts
nothing is indistinguishable from clean code. A clean run reports zero errors, zero warnings and
an empty `firstError()` with `validating()` true; and a deliberately zero-sized `vkCreateBuffer`
is counted, with `firstError()` holding the layer's own text down to the VUID
(`VUID-VkBufferCreateInfo-size-00912`). The second case is the one that proves the wiring, and it
was only possible because step 1 landed first — it needs a device, and there is no window here.

## Step 4 — A context with no window under it

This is the step with real design risk, and the plan does not pretend to have settled it.

[`Context3D`](../../api/render/realtime/Context3D.cpp#L16) throws without a created window, and
then builds eleven things from it. Most need only the device: the pipeline cache, the pipeline
resources, the uploader, the depth format. Three need the chain or the window: the swapchain
itself, the presenter, and `resize()`. And the three renderers — `Quad`, `Line`, `World` — are
built against `swapchain_->format()`, which under dynamic rendering is the format of whatever
they draw into. A headless context would build them against its target's format instead, which
is the same parameter reaching them by a different route.

Two shapes, and the plan recommends the second:

1. **A sibling `ContextHeadless` beside `Context3D`.** Simple and additive; nothing existing
   changes. But the eleven-line constructor is duplicated, and the two will drift — the next
   member added to one is missing from the other, silently, because nothing links them.
2. **Lift what needs only a device into a shared base, with `Context3D` adding the chain and the
   presenter.** The base [`Context`](../../api/render/realtime/Context.h) exists and is empty,
   so there is a place to put it. `Frame` already holds the base and never dereferences it, so
   the seam is free on that side.

The cost of the second shape is the reason it is a step rather than an afternoon:
`voxel/src/Renderer.cxx:113` does a `dynamic_pointer_cast` to `Context3D`, and that is the
pattern an app follows to reach the renderers. If the renderers move to the base, every such
cast should be reaching the base instead — and finding the ones that should *not* move is the
actual work. **Do the survey of what each app reaches through its context before committing to
the split**, not after.

### The survey, done

Two findings, and the second one moves this step somewhere the draft above did not reach.

**What apps reach through a context is almost nothing, and almost all of it is device-only.**
`voxel/src/Renderer.cxx` is the only place in the tree that takes a context at all. It uses
`device()`, `uploader()`, `resources()`, `frameUniforms()`, `pipelineCache()` and
`depthFormat()` — every one of which needs a device and nothing more — and exactly one thing
that needs a chain: `swapchain()->format()`, read to build a pipeline against the colour format
it draws into. Under dynamic rendering that is "the format of whatever I am drawing into", which
for a headless context is its target's. So the app-facing half of the split is clean, and the
cast at line 113 would reach a base that carries a colour format instead of a chain.

**`Presenter` is what actually blocks a headless context, and it is two classes wearing one
name.** `Context3D` builds `Quad`, `Line`, `World` and `FrameUniforms` with the presenter, so
none of them can exist without a swapchain today. But what they use it for is only
`framesInFlight()`, `frame()` and `waitFrame()` — the in-flight ring, which is about pacing the
device and has nothing to do with presenting. The other half — `swapchain_`, `imageAvailable_`,
`renderFinished_`, `acquire()`, `present()`, `reset()`, `suboptimal_` — is the chain, and no
renderer touches it. The ring half is `pool_`, `commands_`, `inFlight_`, `framesInFlight_` and
`frame_`, and it needs a device and nothing else.

That is the line, and it falls one class lower than this step assumed. A headless context is
not reachable by adding a class beside `Context3D`; it is reachable by separating the ring from
the chain, after which the base context is almost free and `Context3D` is the base plus a
swapchain and a presenter.

**This is the ADR**, and it is a larger and riskier change than the step was drafted as, because
the ring is on the path every frame in the tree goes through. It is also the finding that says
why the step was worth surveying rather than starting: the draft would have built a second
context that could not construct a single renderer.

### 4a — the ring, split out ✓ landed

[ADR-0051](../adr/0051-the-in-flight-ring-is-not-the-swapchain.md) settles it, and
`vulkan::frame::Ring` now owns the command pool, the per-frame command buffers and the
per-frame fences. `Presenter` holds a ring and keeps the swapchain, the semaphores and
`acquire`/`present`. `Quad`, `Line` and `World` take a ring, and `FrameUniforms` takes the
ring's frame count — which it already wanted, having only ever been passed a number.

Nothing about the frame loop changes, so the whole of this step is a rewiring that has to be
proven not to have changed anything. Two details were worth getting right rather than moving:

- **`acquire()` waits the ring's fence before acquiring**, rather than leaving it to
  `Ring::begin()` which waits again. The image-available semaphore is per frame, so this slot's
  may still be pending from its last turn until that submission completes. `begin()` is what
  unsignals, so a chain found out of date in between leaves the ring as it was found — which is
  the invariant the old code kept by resetting the fence only after a successful acquire.
- **The fence is the seam.** The ring creates and waits it; `present()`'s submit signals it.
  That is the one thing about this split a reader has to be told, and it is why `fence()` is
  public at all.

Verified by running rather than by building, because the failure this risks is a hang and not a
compile error. A compile proves nothing here, and neither does an app that survives being killed
after six seconds — a ring whose fence is never signalled blocks on its third frame and looks
identical from outside. So the check was a temporary frame counter: pong presented **360 frames
in six seconds**, steady at 60, which is the ring wrapping 180 times with every turn waiting on
a fence the previous submit signalled across the new boundary. Zero validation messages of
either kind, with `VK_LAYER_VALIDATE_SYNC=1` on. voxel, which drives quads, lines and world
quads, is silent the same way.

### 4b — the context split ✓ landed

`DeviceContext` owns everything that needs only a device: the ring, the pipeline cache and
resources, the uploader, set 0, the depth format, the depth buffer, and all three renderers.
`Context3D` is that plus a window, a chain, a presenter and `resize()`. It is usable on its own,
so a headless context is `DeviceContext` told what it draws into rather than a fourth class.

Three things the draft had not settled:

- **The empty `Context` stays.** It was tempting to make it the device-owning base, since
  `Frame` holds one and never dereferences it - but `FrameTest` constructs a bare `Context`, and
  that suite runs in CI on a runner with no device. `Frame::context()` also has no callers
  anywhere in the tree, which argued for deleting it outright; it is public api and this tree is
  consumed as source by apps that are not in it ([ADR-0027](../adr/0027-the-api-is-consumed-as-source.md)),
  so "nothing here calls it" is not the same as "nothing calls it". Three levels, and the
  cheapest of them is the one a device-free test can build.
- **`depth()` is sized by `extent()`**, which is a description the context is given rather than
  a chain it asks. `Context3D` calls `describe()` after building its chain and again in
  `resize()`, which is the one place a chain is rebuilt. A headless context passes its target's
  size once.
- **Every renderer is lazy now**, `quads()` included, because a context is constructed before
  it has been told what it draws into and a pipeline is built against that format.

The payoff is visible in the one place that reaches into a context: `voxel/src/Renderer` now
casts to `DeviceContext` and asks it for `colourFormat()`, where it used to need a `Context3D`
so that it could ask the chain. Nothing else in the tree changed, which is what the survey
predicted.

**A pre-existing defect turned up, and is [in the TODO](../TODO.md) rather than fixed here.**
Both apps that ask for depth report ten `WRITE_AFTER_WRITE` hazards under
`VK_LAYER_VALIDATE_SYNC=1`: one depth image, two frames in flight, and a transition from
`UNDEFINED` whose source scope names nothing. It is not this step's - the baseline at `72ebb79`
reports the same ten with the same signature, which is worth stating plainly because a
refactor of the frame loop is exactly the change such a hazard would be blamed on.

Verified the same way 4a was, and the check mattered twice. A seven second run of voxel showed
zero errors and looked like a pass; it had drawn no frames at all, because terrain generation
had not finished. Given twenty seconds it draws 480 and reports the pre-existing ten. pong
draws 360 in six seconds with none, and the instrumentation also confirmed what a lazy renderer
puts at risk: `colourFormat` is 37 rather than `UNDEFINED`, so `describe()` ran before anything
was built against it.

**This step likely earns an ADR**, on the second shape rather than the first: what a context is
for, and where the line between "needs a device" and "needs a window" falls, is a decision a
future reader will ask about, and it constrains where every later renderer is built. That is two
of the three tests [adr/README.md](../adr/README.md) sets. Write it once the survey above has
been done and the line is actually known — an ADR drafted before then would be recording a guess
at the shape rather than the reasoning behind it.

## Step 5 — The first suite that draws

With 1, 3 and 4 in place: build an instance with no WSI extensions, select a surface-free
device, allocate a `RenderTarget`, record a `Frame` of known passes through `Recorder::record`,
submit, wait on a fence, and assert the validation sink is silent and the layer was on.

Start with the smallest frame that exercises a real pipeline — one cleared pass and one quad —
rather than something representative. ADR-0007 is explicit that a software rasterizer keeps
tests small, and the first suite's value is in proving the harness, not in coverage.

What to cover once the harness holds, roughly in order of what is hardest to see by eye: the
layout transitions either side of a frame, a pass drawing into a target that a later pass
samples ([ADR-0031](../adr/0031-a-pass-draws-into-a-target-it-names.md)), the scissor that
`vkCmdSetScissor` applies from a batch's clip rectangle
([ADR-0037](../adr/0037-clipping-is-a-scissor-the-batch-carries.md)), and the depth ordering of
a world quad against solid geometry
([ADR-0042](../adr/0042-a-textured-quad-in-world-space.md)) — which Testing.md currently calls a
run-and-look check.

The suite needs a guard so that a machine or a runner with no Vulkan device at all skips rather
than fails. `v3d_add_test` registers with ctest directly, so the guard is either a skip return
code the ctest entry knows about or a CMake condition — resolve it against how `v3d_add_test`
is written rather than assuming.

## Step 6 — Lavapipe on the runner

[ctest.yml](../../.github/workflows/ctest.yml) already does most of this: it runs on
`windows-latest`, installs a version-pinned Vulkan SDK with a cache keyed on the resolved
version, builds with vcpkg's binary cache, and runs ctest. What is missing is an ICD.

The work is fetching a Mesa Windows build, putting it where the loader finds it, and **pinning
the exact version** — ADR-0007 names this as a risk in as many words, because the build comes
from a third-party release rather than a package manager and CI will drift silently otherwise.
The existing SDK step is the pattern to copy: it resolves a version, keys a cache on it, and
fails loudly when the install is incomplete rather than proceeding with a missing binary.

ADR-0007 flags lavapipe's Vulkan 1.3 and dynamic rendering support as something that has to be
confirmed, since [ADR-0002](../adr/0002-target-vulkan-1-3.md) makes both mandatory. **That risk
can be retired before a line of this plan is written**, and should be. `Device::selectPhysical` already
rejects a physical device on exactly the three grounds that matter — `apiVersion` below
`VK_API_VERSION_1_3`, a missing required extension, and a missing required feature — at
[Device.cxx:220](../../api/render/realtime/vulkan/device/Device.cxx#L220). So the whole question
is answered by installing lavapipe locally, pointing the loader at it, and running any app in
the tree: either it selects a device or it finds none, and the log says which. An hour, no code,
and it is the only thing in this plan that could invalidate the last step.

If it does fall short, ADR-0007's own fallbacks are SwiftShader, and below that a headless device
with no swapchain and no dynamic rendering that still covers instance, device and resource
lifetime. Worth stating plainly either way: **steps 1 through 5 have value even if step 6 fails
entirely**, because they run against a real driver on a development machine, which is where a
rendering change is verified today by hand.

---

## What this plan does not take up

- **Golden images.** Deferred by ADR-0007 and discussed above. Step 2 is its foundation.
- **`Feature::Window`, `ui::TextRenderer` and `audio::Engine::initialize()`.** Testing.md lists
  all three beside `api/render` as waiting on ADR-0007. They wait on a window and a sound device
  rather than on a GPU, which is a different problem with a different answer, and folding them in
  would make this plan two workstreams wearing one title.
- **Anything in the clang-tidy backlog.** Unrelated, and large enough to drown this.
