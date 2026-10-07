# Frames, passes and targets

How an app builds what it draws each frame: a frame made of passes, the image each pass draws
into, and what happens when the window changes size. The objects named here are introduced in
[README.md](README.md).

## A frame and its passes

An app builds a frame during `render()` and draws it once at the end with `renderFrame()`. This
is the whole of a 2D app's drawing, taken from [examples/starter](../../../examples/starter/src/AppEngine.cxx):

```cpp
bool AppEngine::render() {
    glm::ivec2 size;
    if (!renderer_->beginFrame(&size)) {
        return true;    // the window has no area; nothing to draw
    }
    canvas_.resize(size.x, size.y);
    canvas_.clear();
    canvas_.rect(glm::vec2(40.0f, 40.0f), glm::vec2(size.x - 40.0f, size.y - 40.0f),
        glm::vec4(0.9f, 0.4f, 0.2f, 1.0f));

    boost::shared_ptr<Pass> pass = renderer_->frame()->pass(Engine3D::colourPass);
    renderer_->quads()->submit(canvas_, pass.get());

    renderer_->renderFrame();
    return true;
}
```

`renderFrame()` records the frame, presents it, and empties every pass ready for the next frame.
A pass keeps its settings (target, clear colour, camera, depth) across frames. Only its draw
items are dropped.

### The colour pass

Every frame has a pass named `Engine3D::colourPass` (`"colour"`). It draws into the window and
clears to the colour set with `Engine3D::clearColour()`. A 2D app usually needs nothing else.

### Adding passes

`Frame::pass(name)` returns the pass with that name, creating it at the end of the list if
there is none. More drawing means more passes, not a different kind of frame. An editor draws
one pass per viewport of the same scene; a shadow map is a pass into an offscreen target.

A pass has these settings:

| Call | Effect | Default |
|---|---|---|
| `clearColour(colour)` / `keepColour()` | Clear the target before drawing, or draw over what is there. | Clears |
| `depth(bool)` | Test depth. 2D passes do not; they rely on drawing order. | Off |
| `target(renderTarget)` | Draw into an offscreen target instead of the window. | The window |
| `reads(renderTarget)` | Declare that this pass samples a target another pass draws. | None |
| `viewport(glm::vec4(x, y, w, h))` | The region of the target to draw into, in pixels. Zero width or height means all of it. | All |
| `camera(view, projection)` | The camera for every item in the pass. | Identity |
| `sort(bool)` | Record items in sort-key order instead of submission order. | Off |
| `scene(set)` / `depthBias(...)` | Used by the lit pass. See [The lit pass](Lighting.md#the-lit-pass). | None |

Rules:

- **The first pass to draw into a target in a frame should clear it.** A pass that keeps colour
  draws over whatever the previous pass left. Depth is cleared exactly when colour is.
- **Leave sorting off for 2D.** An unsorted pass draws items in the order they were submitted,
  which is painter's order. Sorting groups items by pipeline and texture, which would put a
  panel on top of the text drawn on it.
- **Turn sorting on for a depth-tested scene with many objects.** Grouping lets the renderer
  skip rebinding the same pipeline and texture. The sort is stable, so equal keys keep their
  submission order.
- **A layer only matters in a sorted pass.** `submit(canvas, pass, layer)` takes a layer, and a
  sorted pass draws lower layers first. An unsorted pass ignores it.

### Pass order

Passes are recorded in the order they were created, with one exception. A pass that names a
target in `reads()` is recorded after every pass that draws into that target. A scene pass that
reads a shadow map can therefore be created first and still draw second.

- Passes that draw into the same target, the window included, keep their creation order.
- A pass that samples a target without naming it in `reads()` is recorded where it was created,
  and reads whatever the target holds at that point.
- Two passes that each read what the other draws throw `std::runtime_error`.
- A pass that names its own target in `reads()` is reading that target's previous frame. That
  orders nothing.

Background: [ADR-0068](../../adr/0068-rendering-order-passes-by-what-they-read.md)

## Choosing a target

A pass draws into the window unless it names a `vulkan::frame::RenderTarget`. A render target
is an offscreen image that a later pass can sample.

```cpp
using vulkan::frame::RenderTarget;
auto context = renderer_->context();

// colour and depth, the size of a scene view
auto scene = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    width, height, VK_FORMAT_R8G8B8A8_SRGB, /* depth */ true);

// depth only, sampled later: a shadow map
auto map = boost::make_shared<RenderTarget>(context->device(), context->ring(),
    2048, 2048, VK_FORMAT_UNDEFINED, /* depth */ true, /* sampledDepth */ true);
```

The constructor is `RenderTarget(device, ring, width, height, colour, depth = false,
sampledDepth = false, images = 1)`.

- **Size and format are yours.** A target does not follow the window. An app that wants one the
  window's size calls `recreate(width, height)` when `beginFrame()` reports a new size.
  `recreate()` throws on a zero dimension or a failed allocation, and the target then keeps its
  old images and size.
- **`VK_FORMAT_UNDEFINED` for colour means no colour image.** Such a target must have sampled
  depth, or the constructor throws; a target with nothing to read is useless.
- **`sampledDepth` can change the depth format.** Not every depth format can be both drawn into
  and sampled. A pipeline that draws into the target must be built against
  `target->depthFormat()`.
- **A pass that both samples a target's depth and draws into it is not allowed.** The renderer
  does not detect it; the validation layer reports it.

### Sampling a target

After the last pass that draws into a target, the target is left readable. To draw it, register
it with `Textures`:

```cpp
TextureHandle sceneTexture = renderer_->textures()->texture(*scene);   // its colour
TextureHandle shadowMap = renderer_->textures()->depthTexture(*map);   // its depth
```

`texture()` and `depthTexture()` return the white texture for a target with nothing of that
kind to read. Registering the same image again returns the handle it already has, so calling
either every frame costs nothing. That handle is shared by everything that registered the
image, and releasing it releases it for all of them.

**Register again after a resize.** A registration keeps the target's old images alive and goes
on drawing them. After `recreate()`, release the old handle and register the target again.

### Reading last frame

A target built with `images` equal to the ring's frames in flight
(`context->ring()->framesInFlight()`) holds one image per frame. A pass draws into the current
one, and `previous()` is what the frame before drew. This is how a pass reads its own last
frame, for trails or feedback effects.

- Register each slot once: `textures->texture(*target, slot)` for every slot.
- Each frame, draw with the handle for `target->previous()`.
- Every slot starts cleared to transparent black and readable, so `previous()` can be read on the
  first frame.

### Pipelines and formats must match

A pipeline is built for one colour format and one depth format. **A pipeline drawn into a pass
with different formats throws `std::runtime_error` naming the pass.** The renderers
`Engine3D` gives you are built for the window's format. To draw into a target of another format,
build that renderer yourself against the target's formats (see
[The lit pass](Lighting.md#the-lit-pass) for an example with `World`).

## Reading a frame back

`vulkan::frame::Capture` copies a drawn image into CPU memory and writes it as a PNG. It is for
code that records frames itself, such as a headless context or a device test. `Engine3D` has no
capture hook.

It takes two calls, because a GPU submit has to complete between them:

```cpp
vulkan::frame::Capture capture(device, logger);
vulkan::frame::Capture::Source source;
source.image = target->image();
source.extent = target->extent();
source.format = target->format();
source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   // where the recorder left it
capture.record(commands, source);    // into the frame's command buffer, after recording
// ... submit, and wait for that submit to complete ...
capture.write("frame.png");
```

- A swapchain image can be captured with `record(commands, swapchain, imageIndex)` after the
  frame is recorded and before it is presented. **This needs `swapchain.copyable()`.** The chain
  is built with `TRANSFER_SRC` usage only where the surface supports it, and otherwise the call
  throws `std::runtime_error`.
- The image is returned to the layout it arrived in, so a captured frame presents normally.
- **A capture recorded and never written costs a copy and is thrown away silently.**
- A source with `depth` set copies depth instead of colour, and `capture.depth()` returns one
  float per pixel. Only `VK_FORMAT_D32_SFLOAT` can be read this way.
- The readback buffer is allocated on the first `record()`.

Testing with captures is covered in [contributing/Testing.md](../../contributing/Testing.md).

## Resize and minimize

- **`beginFrame(&size)` is how an app learns the window size.** No resize event reaches a
  renderer. Resize canvases, and recreate targets that track the window, when the size changes.
- **When the window is minimized, `beginFrame()` returns false.** It has already presented an
  empty frame. Draw nothing and return. Drawing starts again on its own when the window has an
  area.
- The swapchain is rebuilt when the GPU reports it out of date. A frame may be dropped when this
  happens; the next one draws at the new size.
- The renderers `Engine3D` gives you keep working across a resize. Their pipelines do not need
  rebuilding.
