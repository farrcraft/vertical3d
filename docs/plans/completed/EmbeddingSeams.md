# Embedding Seams — What An App That Brings Its Own UI, Renderer And `main` Needs

Drafted 2026-09-07 against `11ae25c`, and **closed** the same day. Seven steps across
`api/engine`, `api/ui`, `api/image`, `api/asset` and `api/render/realtime`.

The prompting case is a game built against this api in another repository, whose own
`vertical3d-upstream.md` measured this tree at `c4bb7aa` and listed nine things it needed
changed *here* rather than worked around there. That list is not linked from this file, because
its paths do not resolve from this one; what it found is restated below where the reasoning is
worth keeping.

**But the reason to do this here is not that one consumer asked.** Every entry is the same
shape: a place where an `api/` library assumed the app it was hosting was one of the four in
this tree. `engine::Engine` can only host an app whose entire input model is `mappings.json`.
`ui::TextRenderer` is device-free in every line but one, and that one line is what stops an app
with its own renderer from using the class at all. `Immediate` cannot be asked whether it wants
the cursor. Each is a seam that was never cut because nothing outside the tree had ever pulled
on it, and the four apps in the tree will not notice any of them being cut.

## What has already landed

Two of the nine entries closed between `c4bb7aa` and `11ae25c`, as part of
[GameFoundations](GameFoundations.md), and are **not** steps here:

| Was | Now |
|---|---|
| The retained tree does not wrap text | [`ui::wrap`](../../../api/ui/Text.h) is used by [`Arranger.cpp:135`](../../../api/ui/Arranger.cpp#L135) to measure a `Label`'s rows and by [`ComponentRenderer.cpp:248`](../../../api/ui/ComponentRenderer.cpp#L248) to draw them |
| No world-space filled primitive | [`grid::fillTile` and `fillTiles`](../../../api/grid/Overlay.h#L71) over the textured world-space quad of [ADR-0042](../../adr/0042-a-textured-quad-in-world-space.md) |

That is worth saying plainly rather than leaving the reader to diff two commits: a list written
against a tree that is still moving goes stale in the direction of *less* work, and the two
entries that went are the two that were nearest to being worked around downstream.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--an-app-sees-an-event-before-the-bindings-do) | An app-visible SDL event seam on `engine::Engine` | `api/engine` | **0043** | ✓ landed |
| [2](#step-2--runt-forwards-what-an-app-was-built-with) | `run<T>` forwards constructor arguments | `api/engine` | — | ✓ landed |
| [3](#step-3--textrenderer-takes-its-atlas-upload-as-a-seam) | `ui::TextRenderer` takes its atlas upload as a callback | `api/ui` | cites 0019 | ✓ landed |
| [4](#step-4--immediate-can-say-whether-it-wants-the-cursor) | `Immediate::capturing()` | `api/ui` | cites 0038 | ✓ landed |
| [5](#step-5--a-widget-can-be-made-narrower-than-its-row) | `Immediate::nextItemWidth()` | `api/ui` | — | ✓ landed |
| [6](#step-6--an-image-reader-that-can-be-pointed-at-a-buffer) | A reader that decodes from memory, and the model half behind it | `api/image`, `api/asset` | — | ✓ landed |
| [7](#step-7--a-depth-target-that-can-be-sampled) | A sampled depth target | `api/render/realtime` | **0044** | ✓ landed |

Steps 1–5 were independent of each other and of 6 and 7. Step 1 was the only one that stopped
work that was otherwise ready to start; step 3 was the only one accruing a duplicate *while it
waited*.

## What came out differently

**Step 6 did not land in `api/type`.** The plan weighed two homes for a decoded embedded image
and recommended the worse of them. `type::Model::Material` is the wrong place because
`v3dlib_type` links glm and nothing else, so a material holding an image would take `api/image`
into every consumer of a mesh, both offline renderers included. It went on
`asset::Model::baseColourImage()` instead, one layer out, in a library that already links
`api/image` — and an embedded texture is a fact about how a file was packaged rather than about
a surface, so that is where it belonged anyway.

**Step 6 also found a defect it was not looking for.** `image::reader::Png` never installed a
libpng error handler, so a corrupt or truncated png aborted the process rather than returning
nothing. That was survivable while every png came from a file the tree shipped; a reader that
can be pointed at arbitrary bytes cannot afford it. It now sets `png_jmpbuf` and its owning
objects are declared above the `setjmp`, the way the jpeg reader already did.

**Step 5 was a wrong measurement as well as a missing control.** `itemWidth()` had to be
measured from where the widget will actually be placed, which on a `sameLine()` row is past the
last widget rather than at the row margin. Without that, an unqualified full-width widget after
`sameLine()` overflows whether or not anything asked for a width.

**Step 1's `route()` returns void.** The plan had it returning whether to keep polling, which
would have moved the `quitting_` check from before the event is handled to after it — a
behaviour change dressed as an extraction. The loop keeps its original guard.

**Step 7 has no headless case, and the plan was wrong to promise one.**
`DepthBuffer::chooseFormat` takes a `VkPhysicalDevice` and asks it for format properties, so
the new branch needs a device like everything else below the recorder. It is verified by
ADR-0007's standing answer instead, and only for the unsampled path: nothing in this tree
constructs a `RenderTarget` at all, so the sampled half is exercised only outside it.

---

### Step 1 — An app sees an event before the bindings do

[`Engine::eventLoop()`](../../../api/engine/Engine.h#L55) is not virtual and
[`handleEvent`](../../../api/engine/Engine.h#L162) is private, so an `SDL_Event` reaches exactly
two places and a subclass is not one of them:

```cpp
while (SDL_PollEvent(&event) != 0 && !quitting_) {
    if (inputEngine_ && inputEngine_->filterEvent(event)) {
        continue;
    }
    handleEvent(event);
}
```

`input::Engine::filterEvent` turns an event into a command through `mappings.json`. That is a
complete input model and a good one, and it is the *only* one. An app embedding a UI toolkit it
did not write — Dear ImGui's SDL3 backend, RmlUi, anything else that wants events rather than
polled state — has nowhere to put `ProcessEvent`, and no amount of `SDL_GetKeyboardState` gets
it there: polling misses a press and a release that land inside one frame, and a backend with no
polled mode has to be reimplemented to be fed by hand.

**The shape.** One virtual, defaulting to "did not take it":

```cpp
/**
 * Offered every SDL event before the input devices see it.
 *
 * This is where an app puts a ui it did not write. Returning true consumes the event, so the
 * input engine never maps it to a command - a click that both presses a button and gives an
 * order is what that prevents, and it is the rule ui::Cursor::press() already applies inside
 * this library.
 *
 * @return whether the app took the event
 **/
virtual bool onEvent(const SDL_Event& event);
```

called from the loop as:

```cpp
if (!onEvent(event) && inputEngine_ && inputEngine_->filterEvent(event)) {
    continue;
}
handleEvent(event);
```

**The ordering is the whole of the decision, and is what earns
ADR-0043.** Two things are settled and neither is obvious:

- *The app goes before the bindings*, because the app is the outer layer — it drew over the
  scene, so it is what the cursor is pointing at. This is the same rule
  [ADR-0038](../../adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md) applies inside
  `api/ui`, extended one layer out.
- *`handleEvent` runs whatever the app returns*, because quit, resize and focus are window
  facts rather than input. An app that consumed a resize would be a window that never resized,
  and an app that consumed a close request would be a window that could not be closed. The
  alternative — run `handleEvent` first and let the app see what is left — was weighed and is
  worse in one specific way: it puts the engine's handling ahead of a UI that may want to veto a
  close, which is a thing an app with unsaved work legitimately wants to do later.

**Testing.** `eventLoop()` cannot be driven in a test: it renders. So the body of the inner
`while` moves to a `protected bool route(const SDL_Event&)` that `eventLoop()` calls once per
polled event, and [`EngineTest.cpp`](../../../api/engine/tests/EngineTest.cpp) drives that through
the `TestEngine` subclass it already has. Three cases and the ADR is covered: an `onEvent`
returning true stops the bindings seeing the event, one returning false does not, and
`SDL_EVENT_QUIT` still quits in both.

**What it unblocks.** The whole of the consuming game's engine-adoption block, which is
otherwise driving its own loop — legal, since `Accumulator` is public and
[ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md)'s fixed step comes with it, but it
declines the one thing that block exists to adopt.

### Step 2 — `run<T>` forwards what an app was built with

[`engine::run<T>`](../../../api/engine/Application.h#L64) is an app's whole `main` and hands the
engine one thing:

```cpp
T engine(appPath(executable));
```

An app that parses a command line into an options struct before the engine exists has nowhere
to hand it over, and writes its own `main` mirroring `run<T>`'s try/catch and its
shutdown-outside-the-loop ordering — about twenty lines, and twenty lines whose whole value is
that they match this file and will stop matching it.

```cpp
template <typename T, typename... Args>
int run(const char* executable, const std::string& name, Args&&... args) {
    T engine(appPath(executable), std::forward<Args>(args)...);
```

Every call site in the tree is unchanged; the four apps still pass nothing. It rides with step 1
because it is the same header neighbourhood and the same reader.
[`ApplicationTest.cpp`](../../../api/engine/tests/ApplicationTest.cpp) gains a case that a moved-in
argument arrives.

### Step 3 — `TextRenderer` takes its atlas upload as a seam

[`ui::TextRenderer`](../../../api/ui/TextRenderer.h) is the reference implementation of the
`Measure`/`Write` pair, and by [ADR-0019](../../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) that pair names no font type so that
drawing a ui costs no device. Every part of the class holds to that — `font::TextureFontCache`
packs into a CPU `image::TextureAtlas`, `font::TextureTextBuffer` lays a string out,
`Canvas::text()` copies the result into the stream — and then the constructor takes a
`vulkan::QuadRenderer` and uses it on [one line](../../../api/ui/TextRenderer.cpp#L91):

```cpp
atlas_ = quads->texture(cache_->atlas()->image());
```

That one line is what stops an app with its own renderer from using the class — and an app with
its own renderer is exactly the app that needed `Measure` and `Write` to be callbacks in the
first place. The consuming repository is carrying a copy of the other ~170 lines with that line
changed, right now, and every day it does is a day the copy can drift.

**The shape.** Take the upload the way the ui already takes text:

```cpp
/**
 * How an atlas image becomes a texture the canvas can name.
 *
 * The one thing in this class that needs a device, so it is the one thing handed in. An app
 * drawing the canvas with its own renderer supplies its own; Engine3D's is
 * quads->texture(image).
 **/
typedef std::function<v3d::render::realtime::TextureHandle(
    const boost::shared_ptr<v3d::image::Image>&)> Upload;
```

**One thing to decide, and the recommendation is the less obvious one.** The cheap version keeps
the `QuadRenderer` constructor as a thin overload that builds that lambda, so no call site
changes. **Do not.** There are exactly four call sites, all identical —
[voxel](../../../voxel/src/Renderer.cxx#L122), [tetris](../../../tetris/src/Renderer.cxx#L65),
[pong](../../../pong/src/PongRenderer.cxx#L38),
[the editor](../../../vertical3d/src/Renderer.cxx#L68) — and keeping the overload leaves
`vulkan::QuadRenderer` forward-declared in an `api/ui` header, which is the thing ADR-0019 was
already claiming was not there. Replace the parameter and write the lambda four times; `api/ui`
then names no Vulkan type anywhere and the claim is true rather than nearly true.

`api/ui` keeps its `Canvas` include and so keeps its `v3dlib_render` dependency: `Canvas` is
CPU-only by design and `TextureHandle` is an opaque `uint32_t`. This step is about the device,
not the library edge.

### Step 4 — `Immediate` can say whether it wants the cursor

There is no `WantCaptureMouse` equivalent: nothing on [`Immediate`](../../../api/ui/Immediate.h)
says whether the cursor is over something it drew, so an app cannot ask the layer whether a
click has already been spent, and answers it by keeping its own list of the window rectangles it
passed in. That is exact for an app that places every window itself and wrong for one that lets
the layer decide.

It costs no new state, because the answer is already computed.
[`Immediate::window`](../../../api/ui/Immediate.cpp) sets `hovering_` to the window's id when the
cursor is inside it, `interact()` sets it for a widget, and
[`end()`](../../../api/ui/Immediate.cpp#L221) rolls it into `hovered_` for the next frame. So:

```cpp
/**
 * Whether the cursor is over something this layer drew, or is dragging something it drew.
 *
 * Answered from what the previous frame found, the same way a widget's own hover is: an
 * app asks this before it draws, and what it is asking about has not been drawn yet.
 **/
bool capturing() const noexcept;
```

returning `hovered_ != 0 || active_ != 0`. The `active_` half is what stops a scrubber being
dragged from losing the cursor the moment the drag leaves the widget's box.

This is the immediate layer's half of the rule
[ADR-0038](../../adr/0038-a-cursor-is-routed-by-the-library-that-drew-it.md) already states for the
retained tree, so it cites that ADR rather than earning one.
[`ImmediateTest.cpp`](../../../api/ui/tests/ImmediateTest.cpp) covers it: over a window, off it,
and held through a drag that leaves the widget.

### Step 5 — A widget can be made narrower than its row

[`selectable`](../../../api/ui/Immediate.cpp#L539), [`dragInt`](../../../api/ui/Immediate.cpp#L558)
and [`progressBar`](../../../api/ui/Immediate.cpp#L584) each size themselves to
`row_.right - row_.margin` — all the room the row has left — so two of them cannot share a row.
The first takes the width and the second is placed past the right edge and clipped, which is to
say **it is simply not on screen and nothing reports anything**. That is how this was found, and
it is the argument for fixing it rather than documenting it.

```cpp
/**
 * How wide the next widget should be, instead of the rest of the row.
 *
 * Consumed by the widget that follows and forgotten after it, so it is set again for each
 * one - which is what lets two scrubbers share a row without either of them owning a width.
 *
 * @param width in pixels, or nothing at all to go back to the rest of the row
 **/
void nextItemWidth(float width);
```

held in one `float nextWidth_` and read through a private `float itemWidth() const` that returns
`row_.right - row_.margin` when nothing was asked for, and resets. `separator()` keeps taking
the row: a rule that stops halfway across is not a narrower rule, it is a wrong one. `end()`
clears the pending width the way it clears the rest of the per-frame state, so a caller that
sets one and then returns early does not leak it into the next frame.

### Step 6 — An image reader that can be pointed at a buffer

[`asset::loader::Gltf`](../../../api/asset/loader/Gltf.cpp#L181) reports an image embedded in a
`.glb` and leaves it empty:

```cpp
logger_->get()->warn("The base colour texture of {} is embedded in the file and was not read - "
    "an image reader that takes a buffer is what that needs", name);
```

because [`image::Reader::read`](../../../api/image/Reader.h) takes a `std::string_view filename`
and every implementation opens it — [Png](../../../api/image/reader/Png.cxx#L31) and
[Jpeg](../../../api/image/reader/Jpeg.cxx#L70) through `fopen_s`,
[Bmp](../../../api/image/reader/Bmp.cxx#L123) and [Tga](../../../api/image/reader/Tga.cxx#L31) through
an `fstream`. Until that changes, a consumer keeps its own glTF loader for the embedded case,
which is two glTF loaders in one tree.

**The shape.** Invert which one is primitive. `read(const std::uint8_t* data, std::size_t size)`
becomes the virtual each format implements, and the path overload on the base slurps the file
and calls it. Every format has the memory entry point already: `png_set_read_fn`,
`jpeg_mem_src`, and for BMP and TGA a pointer walk that is *simpler* than the stream code it
replaces. `Factory` gains a `read(buffer, size, std::string_view kind)` that picks the reader by
the same extension key it already maps, since a buffer carries no name.

Reading the whole file into memory first is the cost, and it is not one: these are textures, the
tree loads them at start-up, and every reader already materialises the decoded image — which is
larger — in the same breath.

**The half the upstream list did not name.**
[`type::Model::Material`](../../../api/type/Model.h#L63) holds `std::string baseColourTexture` —
*a name*, resolved by whoever loads the model. A decoded embedded image has no name, so a
buffer reader alone still leaves `Gltf` with nowhere to put the result. Two answers, and this
step has to pick one:

- **The material carries an optional image.** `boost::shared_ptr<image::Image> baseColourImage`
  beside the name, set only for the embedded case. Small, honest about the two cases being
  different, and every consumer of `Material` grows a branch.
- **The loader registers the image under a synthetic name** and fills in the existing field, so
  every consumer stays as it is and the file that decoded the image is the only file that knows
  it was embedded.

The second is the better shape and the harder one, because `asset::Manager` has no place to put
an image that did not come from a path. **Recommendation: take the first, and say in the
docblock that it is the smaller of the two answers to a question `asset::Manager` will have to
answer properly when a second embedded asset kind appears.** A `Material` with two ways to name
a texture is a wart; an `asset::Manager` grown a side table on the strength of one caller is a
design.

[`ImageReaderTest.cxx`](../../../api/image/tests/ImageReaderTest.cxx) already reads each format
from [`data/`](../../../api/image/tests/data); the cases are the same files read again as buffers,
asserting the two paths agree pixel for pixel through [`image::Compare`](../../../api/image/Compare.h).

### Step 7 — A depth target that can be sampled

The largest item, and the only one that is a design change rather than a seam. Recorded in
[TODO.md](../../TODO.md) already.

[`RenderTarget`](../../../api/render/realtime/vulkan/RenderTarget.h) allocates a
[`DepthBuffer`](../../../api/render/realtime/vulkan/DepthBuffer.h) at the target's size and
[`Recorder`](../../../api/render/realtime/vulkan/Recorder.cxx#L151) transitions it for the pass to
test against. But the image is created with
[`usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`](../../../api/render/realtime/vulkan/DepthBuffer.cxx#L82)
and nothing else, there is no sampler, and `RenderTarget::texture()` describes the colour image
only. So a shadow map has somewhere to be written and nothing to read it with — the target's
whole reason for existing, applied to the one attachment it does not apply to.

Four changes, and they are in order:

1. **`DepthBuffer::chooseFormat` learns a second question.** It walks candidates for
   `optimalTilingFeatures & DEPTH_STENCIL_ATTACHMENT`; a sampled buffer also needs
   `SAMPLED_IMAGE`. That is a *different format choice*, which is why this is not a flag on the
   image creation and why the buffer has to know at construction whether it will be sampled.
2. **A sampled `DepthBuffer` carries a sampler and adds `VK_IMAGE_USAGE_SAMPLED_BIT`.** The
   sampler is not the colour one: a shadow map wants clamp-to-border with a white border, so
   that everything outside the light's frustum is lit rather than shadowed.
3. **`RenderTarget` exposes `depthTexture()`** beside `texture()`, with the same contract — a
   view and a sampler to bind, not an allocation to own, per
   [`Resources`](../../../api/render/realtime/vulkan/Resources.h#L59).
4. **`Recorder` transitions the depth image after the last pass that wrote it**, the way it
   already does for [colour](../../../api/render/realtime/vulkan/Recorder.cxx#L157). Not to
   `SHADER_READ_ONLY_OPTIMAL` but to `VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL`, which is the
   layout a depth aspect is sampled in, and only when the target was built sampled — a target
   whose depth nothing reads should not pay a barrier for it.

**This earns ADR-0044**, on two counts that a later reader will ask about:
that asking for a sampled depth target changes which format the device gives you and so which
pipelines have to be built against it, and that the read-only layout is a promise the recorder
makes on the app's behalf — a pass that both samples a depth target and writes it is a hazard
this design forbids rather than detects.

**Verification is the awkward part.** Everything below the recorder needs a window and a GPU
([Testing.md](../../Testing.md)), so this is verified by
[ADR-0007](../../adr/0007-ci-rendering-tests.md)'s standing answer: run an app and read the log,
with a silent validation layer as the signal. The layer is exactly the right instrument here —
a sampled image in the wrong layout, a missing usage bit and a format without the sampled
feature are all things it says out loud. `chooseFormat`'s new branch is the one half with no
device in it and gets a case.

---

## Sequence

**Step 1 first and alone**, because it is the only thing blocking work that is otherwise ready,
and because its ADR wants to be argued on its own rather than in a batch. **Step 2 with it** —
same header, same reader, and it is four lines.

**Step 3 next**, because it is the only entry whose cost is growing: a copy of `TextRenderer`
exists downstream today and every week it lives is a week it can drift from this one.

**Steps 4 and 5 together.** Both are `Immediate`, both are small, both are covered by cases in
one test file, and neither blocks anything.

**Step 6 and step 7 last, in either order.** Step 6 is behind a decision this plan recommends
but does not settle; step 7 is behind nothing but is the largest thing here, and the consumer
that wants it wants it after work that is itself behind step 1.

Steps 1–5 are five commits and none of them changes behaviour for the four apps in this tree.
That is the point of the ordering as much as the blocking is: the risky work is the last two,
and by then the seam work is landed and being used.

## Verification

Per [sdlc.md](../../sdlc.md), each step: `ninja -C out/build/x64-Debug`, `ctest`, cpplint, and the
`/W4 /WX`, `/analyze` and clang-tidy gates, all of which the tree is clean at so every finding
is the step's. New cases go in
[`api/engine/tests`](../../../api/engine/tests) (steps 1 and 2),
[`api/ui/tests`](../../../api/ui/tests) (steps 4 and 5) and
[`api/image/tests`](../../../api/image/tests) (step 6). Step 3 has no new behaviour to cover and is
verified by the four apps still drawing text. Step 7 is the render-verification case above.

## What this does not do

- **It does not adopt the consuming app's input model, or any part of it.** Step 1 gives an app
  a place to stand; what it does there is its own.
- **It does not double-buffer a render target**, which [TODO.md](../../TODO.md) also carries and
  which step 7 will make more obviously missing rather than less.
- **It does not settle where a decoded asset with no path lives.** Step 6 works around that
  question deliberately and says so in the code.
- **It does not touch `api/ui`'s dependency on `v3dlib_render`.** Step 3 removes the last Vulkan
  *type* from the library; `Canvas` and `TextureHandle` stay, because they are the CPU-side
  contract the ui is built on rather than a device leaking in.

## When a step lands

Update the state note in the table above, set the ADR's status if the step carried one, and for
step 7 delete [TODO.md](../../TODO.md)'s depth-target line rather than marking it done. Steps 1, 3
and 7 each move something a document owns: [Architecture.md](../../Architecture.md) for the loop's
new seam, [UserInterface.md](../../UserInterface.md) for `TextRenderer`'s constructor and
`Immediate`'s two additions, and
[RenderingPipeline.md](../../RenderingPipeline.md) for a target whose depth can be read.
