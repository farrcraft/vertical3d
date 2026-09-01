# Voxel Survey

Phase 5 of [plans/Modernization.md](plans/Modernization.md). Surveyed against the tree on
2026-08-31.

Voxel is the app that drives real api growth: it is the first thing in the repo that needs a
depth buffer, a second pipeline, static device-local geometry and a camera shared across many
draws. Pong and tetris exercised none of that. This is the record of what voxel is, what it
needs, and what it does wrong, so the phase can be scoped rather than guessed at.

**Verdict up front: voxel does not run, and has not for a long time.** It compiles and links,
which was the only signal anyone had. Run it and it exits 1 before a window opens. Fix that
and it segfaults inside `Renderer`'s constructor. The render port is the large piece of work,
but it is the fourth thing that has to be fixed, not the first.

> **Since the survey**, the first two of those and all six defects below are fixed — see the
> "Make it start" group in
> [plans/Modernization.md](plans/Modernization.md#phase-5--voxel-and-odyssey-and-the-engine-consolidation).
> Voxel now loads its config, opens a window and brings up the Vulkan device and swapchain,
> and dies at the `glGetString` call. Everything below is the state the survey found; the
> "What the api does not have yet" list is still entirely ahead.

## Method

`voxel/` is 2,824 lines across 43 files, plus a vendored `src/noise/noiseutils.{h,cpp}` of
3,839 lines that is libnoise's own utility layer and is excluded from lint. All 43 were read.
The api surfaces voxel would have to port onto — `api/render/realtime` and its `vulkan/`
subdirectory, `api/config`, `api/engine` — were read alongside them, so that "voxel needs X"
is a claim about what exists rather than about what was planned.

The failure modes below were observed, not inferred: `voxel.exe` was run from the build tree,
and then run again with a hand-written config placed in the build tree to see what was behind
the first failure. That probe was removed afterwards; the build tree is back to what
`v3d_add_shared_data` leaves there.

## How far it actually gets

Run from `out/build/x64-Debug/voxel`, the log ends here and the process exits 1:

```
[info] Initializing engine...
[info] Setting asset manager path to: ...\voxel\data/
[info] Looking for json asset at: ...\voxel\data/config.json
[error] Caught exception loading JSON asset: Could not open JSON file.
```

There is no `config.json` in the build tree, because **voxel never calls
`v3d_add_app_data`** — only `v3d_add_shared_data`, which brings the fonts and nothing else.
The `data/` directory beside the executable holds `fonts/` alone.

Give it the file and the next failure is worse. `voxel/data/config.json` is in the **old
inline `keys` format**, which `Config::load` does not merely reject — it reaches for
`doc.at("configs")`, which throws on the missing key, and nothing between there and `main`
catches it. Voxel is now the only app still on that format; tetris was migrated on
2026-08-31 and pong is the reference.

Give it a valid indirect config and the whole Vulkan stack comes up clean:

```
[info] Creating window 800 x 600
[info] Created vulkan instance with 3 extension(s), validation on
[info] Using vulkan device AMD Radeon RX 6600
[info] Created a vulkan swapchain of 3 images at 800 x 600
```

and then it segfaults. `Renderer`'s constructor calls `glGetString(GL_RENDERER)` and streams
the result into a log message. `Window3D::create` builds an `SDL_WINDOW_VULKAN` window and
its GL setup is commented out, so there is no current GL context and no `glewInit`;
`glGetString` returns null and `operator<<` on a null `const GLubyte*` takes the process
down. Every GL call after that line is against nothing.

So the ordering is: app data, then config format, then the render port. The first two are
half an hour of work and neither is blocked by anything.

## What voxel is

A first-person voxel terrain walker. Perlin heightmap in, chunked greedy-ish face culling,
one ADS-lit draw per chunk, WASD and mouselook, an F3-style debug overlay.

| Piece | Files | What it does |
|---|---|---|
| `Controller` | `Controller.{h,cxx}` | `v3d::engine::Engine` subclass. Config, Window3D, keyboard and mouse. Maps six movement commands and a debug toggle; warps the cursor to centre each frame for mouselook. |
| `Scene` | `Scene.{h,cxx}` | Builds the world in its constructor: a `TerrainMap`, then 16x4x16 = 1024 `Chunk`s keyed by Morton code. Owns the `Player`, and reaches through it for the camera. |
| `TerrainMap` | `voxel/TerrainMap.{h,cxx}` | libnoise Perlin over a 256x256 plane, rendered to a greyscale image read back as height. The only thing that needs `vendor/libnoise`. |
| `Chunk` / `Voxel` | `voxel/Chunk.{h,cxx}`, `voxel/Voxel.{h,cxx}` | 16³ of block type and position, sparse — only solid blocks are stored, each as its own `boost::shared_ptr<Voxel>`, in an `unordered_map` keyed by Morton code. 17 block types. |
| `MeshCache` / `MeshBuilder` | `voxel/MeshCache.{h,cxx}`, `voxel/MeshBuilder.{h,cxx}` | Face culling against neighbours within and across chunks, then triangles into fixed-capacity arrays. `build()` rebuilds at most 16 dirty chunks a tick. |
| `VertexBufferBuilder` / `ChunkBufferPool` | `engine/VertexBufferBuilder.{h,cxx}`, `voxel/ChunkBufferPool.{h,cxx}` | Turns a `MeshCache` into a `v3d::gl::VertexBuffer`; the pool holds one per chunk and draws all of them, in map order, with no culling. |
| `Renderer` | `Renderer.{h,cxx}` | Loads two GL programs through `asset::ShaderProgram`, sets 16 materials and one light as uniforms, sets GL state, draws the pool, then builds a `Frame` for the overlay. |
| `Camera` | `engine/Camera.{h,cxx}` | 472 lines. First-person and flight behaviours, velocity and acceleration, orthogonalising axes. |
| `DebugOverlay` | `DebugOverlay.{h,cxx}` | `operation::TextureFont` against a `v3d::gl::Program`, showing version, a 100-sample rolling fps and the player position. |
| `Light` / `Material` / `MaterialFactory` | `engine/*` | Thin setters that push a struct into a GL program's uniforms by name. |
| `Player` / `Item` / `GameState` / `World` | `game/*`, `voxel/World.{h,cxx}` | Movement integration and an inventory that nothing fills. `GameState` is three fields, one of which is read. `World` is an empty class with an empty `.cxx`. |

Two of voxel's five `target_link_libraries` entries are dead: **it names `v3dlib_audio` and
`soloud` and contains no audio code at all**, and it names `OpenGL::GL` and `GLEW::GLEW`
directly, which `v3dlib_gl` already links PUBLIC. (The audio half of that is wrong: voxel
names no audio code, but `v3dlib_asset`'s Wav loader calls `AudioClip::load`, so the link
needs it. `v3dlib_asset` links `v3dlib_audio` PUBLIC now, so no app names it transitively.) The `entt::registry` is threaded from
`Controller` into `Engine3D` and is otherwise unused — `Scene` is a hand-rolled object graph,
not an ECS.

## What the api does not have yet

This is the list that makes voxel a phase rather than a port. Each item is a thing pong and
tetris never asked for.

1. **A depth buffer.** `Pass::depth(bool)` is stored and read back and nothing else — there
   is no depth image anywhere in `api/render`, no `pDepthAttachment` on the
   `VkRenderingInfo` the recorder builds, and no depth format on the pipeline's
   `VkPipelineRenderingCreateInfo`. It has to be created with the swapchain, rebuilt with it
   on resize, and transitioned alongside the colour image. Until it exists no 3D scene can
   draw correctly.

2. **A way for an app to build a second pipeline.** `QuadRenderer` is the only pipeline in
   the tree and its ~150 lines of `VkGraphicsPipelineCreateInfo` are inline and unshared.
   `Context3D` does expose `device()`, `pipelineCache()` and `resources()`, so an app can
   build one — by copying all of it. A pipeline builder in
   `api/render/realtime/vulkan` is what stops the second pipeline being a fork of the first.

3. **Set 0, the per-frame descriptor.** [ADR-0008](adr/0008-binding-by-update-frequency.md)
   decided that camera and projection live at set 0, bound once per pass.
   `QuadRenderer::createLayouts` creates `frameLayout_` with **zero bindings** and nothing
   ever writes or binds a set against it. The quad pipeline pushes a mat4 per draw, which is
   right for 2D; a scene with a few hundred chunk draws sharing one view and projection is
   the case set 0 was decided for, and voxel is the first app to reach it.

4. **Device-local buffers with a staging upload.** `vulkan::Buffer` is host-visible, kept
   mapped, and its own header says static geometry "wants the opposite trade and is not what
   this is for". Chunk meshes are built once and drawn for the life of the process.
   `TextureFactory` already has the staging-buffer and one-shot-submit pattern, privately;
   generalising it is most of this item.

5. **Geometry as a resource.** `Resources` owns pipelines, materials and textures, and
   `DrawItem` names them by handle so the sort key means something. A chunk mesh is none of
   the three, and `DrawItem` takes raw `VkBuffer`s, so voxel would own the lifetime itself —
   the one class of GPU object in the frame that nothing outlives on the app's behalf.

6. **Sorting and merging.** Nothing sorts; the recorder walks each pass in submission order,
   which [ADR-0004](adr/0004-operations-as-draw-data.md) allowed for. A pong frame is a
   handful of items. Voxel submits one per non-empty chunk, and is the first frame where the
   sort key has to do the job it was designed for.

7. **App shaders through `v3d_add_shader`.** The helper takes any target, so voxel can call
   it. The shaders themselves have to be rewritten: `voxel_ads` is `#version 330` with a
   default-block `uniform mat4`, a `LightInfo` struct and a 16-element `MaterialInfo` array,
   all set by name at runtime through `v3d::gl::Program`. Vulkan GLSL needs explicit `set`
   and `binding`, the matrices in set 0 and the material table in a UBO or an SSBO.
   `voxel/data/shaders/` holds twelve GLSL files of which the app loads two; the other ten
   are earlier lighting experiments and should go with the port.

Items 1 through 3 are the ones that shape later work — a depth attachment, a pipeline
builder and a per-frame set are what the editor's multiple viewports need too, so building
them for voxel is building them for phase 6.

## Defects found

All in `voxel/`, all independent of the render port, and none of them could have been noticed
while the app could not start.

- **The world is 64 blocks tall and the terrain is 4.** `Scene` computes
  `worldHeight = 64 / chunkSize` — a count of **chunks**, 4 — and passes it to `Chunk` as
  `ceiling`, which uses it as `voxelHeight`, a count of **blocks**, to scale the heightmap:
  `blockHeight = (height / 255.0f) * voxelHeight`. So every column is somewhere between 0 and
  4 blocks tall in a world with 64 layers, the top three chunk layers are empty by
  construction, and 768 of the 1,024 chunks generate nothing. The terrain has no relief worth
  seeing. `ceiling` wants `worldHeight * chunkSize`.

- **Three of the six cross-chunk face checks test the wrong block.**
  `MeshBuilder::generateChunk` seeds `neighborBlockPosition` from the block's own position
  and then, for `LEFT`, `BACK` and `BOTTOM`, correctly moves it to the far edge of the
  neighbouring chunk. `RIGHT`, `FRONT` and `TOP` move the chunk but not the block, so they
  ask whether the block at 15 is solid when the adjacent one is at 0. Faces on those three
  seams are culled when they should not be and kept when they should not be. The three
  branches that do it right hardcode `15` where they mean `chunkSize - 1`.

- **The index buffer is the identity.** `MeshCache::addTri(v0, v1, v2)` calls `addVertex`
  three times and never looks for an existing vertex, so a quad emits six unique vertices and
  the triangle indices are exactly `0, 1, 2, 3, …` in order. `VertexBufferBuilder` uploads
  them anyway — a quarter of the mesh bandwidth carrying no information. An indexed quad
  over four vertices, or a non-indexed draw, both beat it.

- **`VertexBufferBuilder::build` reads two triangles it never uses.** `leftTri` and
  `rightTri` are assigned from `meshTris` inside the per-face loop and nothing reads them.

- **`GameState` leaves every field uninitialised.** Its constructor has an empty body and no
  initialiser list, so `paused()` returns whatever was on the stack until `pause()` is
  called. Nothing calls `pause()`.

- **`Chunk` and `MeshCache` both list their initialisers out of declaration order.** Neither
  is a live bug — every initialiser is a constant or a parameter — but both will warn under
  any build that enables `-Wreorder`'s MSVC equivalent, and `Chunk`'s body reads `size_`,
  which is the pattern that turns into one.

## Three things the survey corrects elsewhere

- **Odyssey does not call OpenGL.** `plans/Modernization.md` says "voxel and odyssey still
  call GL against a context nothing creates". There is no GL in `odyssey/src` at all — it
  draws through `Context2D`, which is `SDL_Renderer`. What odyssey has is a stale
  `OpenGL::GL` and `GLEW::GLEW` pair in its `CMakeLists.txt`. **Voxel is the only app that
  draws with `api/gl`.**

- **`api/gl` has two live consumers inside `api/` that the deletion item does not list.**
  `api/ui/style/property/Image` and `api/ui/component/Icon` each hold a
  `boost::shared_ptr<v3d::gl::GLTexture>`. Both are built. Deleting `api/gl` means porting
  them onto the texture handle the quad renderer uses, on top of the `asset::Shader`,
  `asset::ShaderProgram` and `operation::TextureFont` removals already named in the plan.
  This is consistent with [LuxaAudit.md](LuxaAudit.md), which found theme image loading
  unported.

- **Odyssey does not start, for the same reason voxel does not.** Checking the odyssey claim
  above was one command, and it answers the question the plan left open — "whether odyssey
  *runs* is a separate question nobody has asked". It exits 1 at the identical first step:
  no `v3d_add_app_data`, so `out/build/x64-Debug/odyssey/` has no `data/` at all. Behind that
  is a failure it has not reached — `odyssey/data/config.json` uses the correct indirect form
  but writes `"type": "bindings"`, and `config::stringToType` only knows `"binding"`, so
  `Config::load` would log "Unknown config type: bindings" and return false. Both fixes are
  minutes.

Also worth having written down: CLAUDE.md claims no app names OpenGL or GLEW any more, and
that no app still uses the older inline config format. Voxel and odyssey both name GL, and
voxel is still on the inline format.

## Scope

The phase items and their order are in
[plans/Modernization.md](plans/Modernization.md#phase-5--voxel-and-odyssey-and-the-engine-consolidation).
The short version: make it start, then give the api a depth buffer and a way to describe a
second pipeline, then port the renderer, then delete `api/gl`.

Nothing here is blocked by the Vulkan frame loop, which landed on 2026-08-31. Voxel is
blocked by not having been run.
