# Rendering

These pages are for someone drawing from an app with `api/render/realtime`. How the renderer
works inside is in [internals/realtime/](../../internals/realtime/README.md).

All names are in `v3d::render::realtime` unless a namespace is given. `vulkan::` means
`v3d::render::realtime::vulkan`.

| Page | Read it to |
|---|---|
| This page | Learn the objects an app holds, how cameras reach a pass, and what is not built |
| [FramesAndTargets.md](FramesAndTargets.md) | Build a frame from passes, draw offscreen, read a frame back, and handle resize and minimize |
| [Canvas.md](Canvas.md) | Draw 2D quads and text, in pixels or in a coordinate space of your own |
| [LinesAndWorldQuads.md](LinesAndWorldQuads.md) | Draw lines and textured quads in world space, and order them by depth |
| [TexturesAndMeshes.md](TexturesAndMeshes.md) | Upload textures, load models through the mesh registry, and release both |
| [Lighting.md](Lighting.md) | Draw a lit scene, and lit, shadowed and skinned meshes |
| [ColourAndPost.md](ColourAndPost.md) | Get colour right, and add full-screen passes and a colour grade |
| [Entities.md](Entities.md) | Draw ECS entities with the api's sprite, mesh and particle functions |

## The objects an app touches

An app creates one `WindowRenderer` in its `start()` and calls `initialize(window())` on it. That
builds everything else.

The constructor takes two optional choices after the logger. The first is the colour
format to present through, covered in [ColourAndPost.md](ColourAndPost.md). The second is how the
device finds memory. It defaults to regions of larger blocks, which suits streaming and keeps
an app far under the device's limit on allocations. A case that wants one allocation per
resource passes `vulkan::memory::Allocator::Kind::Direct`:

```cpp
renderer_ = boost::make_shared<WindowRenderer>(logger(), VK_FORMAT_UNDEFINED,
    vulkan::memory::Allocator::Kind::Direct);
```

| Object | What it is to an app |
|---|---|
| `WindowRenderer` | The renderer. It holds the frame being built and draws it with `renderFrame()`. |
| `DeviceContext` | Everything that needs only the GPU: textures, renderers, the frames in flight. `WindowRenderer::context()` returns it. |
| `WindowContext` | A `DeviceContext` that draws into a window. It adds the swapchain and presenting. An app rarely names it. |
| `Frame` | Everything to draw for one image, as a list of passes. `WindowRenderer::frame()` returns it. |
| `Pass` | One pass of a frame: where it draws, how it clears, its camera, and the draw items in it. |
| `DrawItem` | A description of one draw. The drawing primitives create these for you. |

A *frame in flight* is a frame the CPU has finished recording but the GPU may still be drawing.
There are two. Anything an app hands to the renderer can still be in use by the GPU for up to
two frames after the app lets go of it. The renderer handles this for every object it owns.

`WindowRenderer` gives an app the three primitives and the texture store directly:

```cpp
renderer_->quads();       // renderer::Quad - the 2D quad canvas
renderer_->lines();       // renderer::Line - world-space lines
renderer_->worldQuads();  // renderer::World - world-space textured quads
renderer_->textures();    // Textures - uploads images and makes them drawable
```

`lines()` and `worldQuads()` are built on their first call, so an app that never asks for one
pays nothing for it. `quads()` and `textures()` are null before `initialize()`.

## Cameras

A pass draws through the view and projection given to `Pass::camera(view, projection)`. Lines,
world quads and lit meshes use it; canvas quads do not. Build both matrices with
`v3d::type::camera::Camera`, which produces Vulkan clip space (y down, depth from 0 to 1).
The camera's conventions, including its handedness, are in [Types.md](../Types.md).

To draw billboards, the entity functions in [Entities.md](Entities.md) need the camera's right
and up vectors and a depth axis.

## What is not built yet

- **Merging draws.** Sorting groups items that could share a draw, but each item is still its
  own draw call.
- **More than one depth buffer for the window.** All passes into the window share one. The
  editor's four viewports work because their regions do not overlap and each pass clears its own.
  Two passes needing different depth over the same pixels would not work. An offscreen target
  has its own depth.
- **Wide lines.** Lines are one pixel.
- **A transfer queue.** Uploads run on the graphics queue. None waits for the GPU, and
  `Textures` and `MeshRegistry` spread theirs over frames under a budget.
- **A lit scene in an app.** The lit pass, shadows and the grade are exercised by the device
  tests in `api/render/tests/device`; no app in the tree draws one.
