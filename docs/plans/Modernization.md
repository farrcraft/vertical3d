# Modernization Plan

Draft, 2026-08-30. Living document — update the state notes as things land.

Several rewrites are in flight at once. This is an attempt to work out what actually
blocks what, so they can be finished in an order that keeps the tree buildable instead
of half-finishing all of them at the same time.

## The rewrites

1. SDL2 → SDL3
2. Legacy `v3dlibs/` and `luxa/` folded into the `api/` layout
3. OpenGL → Vulkan, with GL support removed entirely
4. The 2D and 3D realtime engines consolidated into a single Vulkan engine
5. pong and tetris moved onto the new `api/` framework
6. voxel and odyssey after that — neither was built on the v3d frameworks, so both need
   api features that do not exist yet
7. moya and talyn — offline renderers, no realtime dependency

Plus two things that need backfilling throughout: documentation and tests.

## Decisions

Recorded in [docs/adr/](../adr/), not here. The ones that shape this plan:

| ADR | Decision |
|---|---|
| [0001](../adr/0001-vulkan-replaces-opengl.md) | Vulkan replaces OpenGL outright — no dual backend, no fallback |
| [0002](../adr/0002-target-vulkan-1-3.md) | Target Vulkan 1.3, for dynamic rendering and synchronization2 |
| [0003](../adr/0003-one-realtime-engine.md) | One realtime engine, with the render pass as the unit of variation |
| [0004](../adr/0004-operations-as-draw-data.md) | Operations are draw data; the engine sorts, merges and records |
| [0005](../adr/0005-one-batched-quad-primitive.md) | One batched quad primitive with an optional texture |
| [0006](../adr/0006-keep-both-pong-and-tetris.md) | Both pong and tetris are kept |
| [0007](../adr/0007-ci-rendering-tests.md) | Render tests run on a Windows runner against software Vulkan |

## Where things actually stand

Verified against the tree on 2026-08-30.

**SDL3 is done in everything that builds.** No SDL2 code remains in `api/` or in any app.
Two dead references are left: `talyn/CMakeLists.txt` links `${SDL2_LIBRARIES}`, an
undefined variable that now expands to nothing, and `odyssey/cmake/FindSDL2.cmake` is a
stale module nothing uses. The only real SDL2 code left is `v3dlibs/hookah/drivers/sdl2/`,
which is not built. Call this workstream finished once those three are deleted.

**The legacy migration is further along than it looks.** `v3dlibs/` and `luxa/` are not in
the root `add_subdirectory` list — nothing in the build has referenced them for some time.
`api/ui` is already a superset of `luxa/luxa` (Button, Icon, Label, Component and the menu
components, plus containers, styles and a dozen components luxa never had). The command and
binding layer from `v3dlibs/command` did not move across as-is; it was reworked into
`api/config` (`BindingContext`) and `api/event` (`Mapper`). Among the apps, only
`voxel/src/Controller.h` still includes legacy headers. pong's Luxa usage is commented out.

So this is mostly a deletion exercise, not a porting one — with one exception:
`v3dlibs/tests/` is the only test corpus in the repo and must be salvaged before the tree
goes.

**Vulkan is the critical path and nothing renders right now.** `Window3D` owns a
`vulkan::Instance` and `vulkan::Surface`; `Context3D` owns a `vulkan::Device` and
`vulkan::Swapchain`. Missing: render pass, framebuffers, command pool and buffers, sync
objects, and the acquire/submit/present loop. Meanwhile `Engine3D::renderFrame()` and every
app renderer still call OpenGL against a context that is no longer created. pong links and
does not draw. Until the frame loop exists, "it builds" is the only signal available.

**Build health.** Clean: all `api/` libraries, pong, talyn, v3dshell, imagetool. Broken:
tetris (missing header, dropped member, links a target that is never built), voxel (drifted
behind api changes), odyssey (see below).

**Odyssey is not blocked by Vulkan.** It runs on `Feature::Window2D` → `Engine2D` →
`Context2D`, which is SDL's own renderer, not GL and not Vulkan. Its only blocker is a
signature bug: `Operation::run` takes `shared_ptr<Context>` while `Operation2D::run` and
`Blit2DTexture::run` take `shared_ptr<Context2D>`, so nothing overrides the pure virtual and
every 2D operation is abstract. That is a small fix, and it unblocks a whole app.

**Voxel is blocked by Vulkan.** It uses `Feature::Window3D`.

**The 2D engine is small and half-finished.** All of `api/render/realtime/2D/` is roughly
610 lines of thin `SDL_Renderer` wrapping — `Context2D` is an `SDL_Renderer`, `Texture2D` is
an `SDL_Texture`, `Scene2D::collect()` returns an empty `Frame` with the real collection
commented out. Odyssey's entire use of it is: build a texture from a surface, cache it, and
emit one `Blit2DTexture` per renderable. So the consolidation port has a small surface, and
the one operation it needs — a textured quad — is the same primitive tetris needs anyway.

## What blocks what

The instinct is to queue everything behind Vulkan. That is wrong for about half the work.

Blocked by the Vulkan frame loop: pong's renderer, tetris's renderer, voxel, and odyssey's
eventual port onto the consolidated engine. Note that odyssey *building* is not blocked — only
its move off `SDL_Renderer` is, and the SDL path can keep running until the Vulkan one reaches
parity.

Not blocked by anything: deleting the legacy trees, the SDL2 leftovers, the `Operation`
signature fix and odyssey with it, tetris's config-format migration, the test framework, and
docs. Roughly half the outstanding work is in this bucket, and all of it makes the Vulkan
work easier to review by shrinking the noise around it.

The one hard ordering constraint inside the Vulkan work: the apps need batched quads, and
nothing provides them. `v3d::gl::Canvas` batches coloured quads but carries no texture
coordinates, so every tetris piece, all text, and every odyssey tile is unserved. Because the
engine is consolidating, this primitive has to satisfy all three apps — decide its shape
before building the frame loop around it, or it gets built twice.

## Phases

### Phase 1 — Clear the ground

None of this is blocked. It shrinks the surface area everything else has to work against.

- Delete `v3dlibs/`, `luxa/`, `rigel/`, `vertical3d/` — after salvaging `v3dlibs/tests/`
  and after `voxel/src/Controller.h` stops including legacy headers.
- Delete `talyn`'s `${SDL2_LIBRARIES}` link and `odyssey/cmake/FindSDL2.cmake`. SDL3
  workstream closed.
- Fix the `Operation::run` / `Operation2D::run` signature mismatch. Odyssey builds again.
- Make tetris compile: drop the missing `GLFontRenderer.h` include and the debug-text block
  (which is broken C++, not just outdated — it does pointer arithmetic on string literals),
  restore or remove `fonts_`, fix the link list. Target a green build, not a running game.
- Migrate `tetris/data/config.json` from the old inline `keys`/`menu` form to the
  `{"configs": [...]}` form that `Config::load` requires. Pong's `data/` is the reference.

Done when: the whole tree builds, and `api/` is the only library tree in the repo.

### Phase 2 — Vulkan to first pixel

- Dynamic rendering against the swapchain image views. No `VkRenderPass`, no
  `VkFramebuffer` — enable `dynamicRendering` through `VkPhysicalDeviceVulkan13Features`
  chained onto device creation, which also means moving from `pEnabledFeatures` to
  `VkPhysicalDeviceFeatures2`.
- Command pool and per-frame command buffers.
- Sync: image-available and render-finished semaphores, in-flight fences, frames-in-flight.
  Use `synchronization2` barriers rather than the 1.0 forms.
- `Engine3D::renderFrame()` acquires, records, submits, presents — and recreates the
  swapchain on `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR`. That is the reliable resize
  trigger; note `render::realtime::Engine::resize` is dead code that nothing ever connects.
- Build the frame as a list of passes from the start, even while there is only one.
  Retrofitting the pass model after the fact is the expensive version of this.
- The draw-item type and its sort key, including the 2D layer field. Recording walks each
  pass's queue in submission order for now; sorting and merging come later.
- Resource handles: a pipeline cache, a material or descriptor registry, and texture
  handles. Nothing can be sorted or merged until these exist, so they belong here rather
  than being deferred to the phase that first needs them.
- Bind by update frequency, and write the convention down: set 0 per-frame — camera,
  projection, viewport — bound once by the pass; set 1 per-material; push constants
  per-object for transform and tint. Everything downstream depends on this being fixed.

Done when: a window clears to a colour and survives a resize and a minimize.

### Phase 3 — pong as the pilot

pong is the only app already on the `api/` framework and the only one that builds, so it
defines what the api actually needs. Port it before generalising.

- The batched quad primitive: a Vulkan replacement for `v3d::gl::Canvas` plus its operation,
  the 1x1 white texture, and flush-on-texture-change. This serves every app, so build it
  once and build it properly.
- Text: a Vulkan path for `operation::TextureFont` and `TextureFontCache`, which is the same
  primitive drawing from the glyph atlas.
- Re-wire pong's UI to `v3d::ui`; delete the commented-out Luxa block.
- Delete `api/gl/` and drop GLEW and OpenGL from every link list once nothing references
  them.

Done when: pong plays, and no GL remains anywhere in the tree.

### Phase 4 — tetris

The quad primitive already exists from Phase 3, so what tetris adds is the sprite atlas.

- Pack the seven piece textures into one `v3d::image::TextureAtlas` at load time, so the
  board draws as a single batch instead of flushing per block.
- Replace the fixed-function draw path wholesale — `glBegin`/`glEnd`, `glTranslatef`,
  `glPushMatrix`, `glOrtho`. This is the oldest rendering code in the repo and none of it
  survives; rewrite against the Canvas equivalent rather than porting call by call.
- Rewrite the debug text against `TextureFont`.
- Load piece textures through `asset::Manager` instead of `image::Factory` with hardcoded
  relative paths.

Done when: tetris plays.

### Phase 5 — voxel and odyssey, and the engine consolidation

Voxel needs a 3D scene path — shaders, meshes, camera — that pong and tetris never exercised,
so it is the app that will drive real api growth. It needs its own survey before it can be
scoped.

Odyssey is where the consolidation actually lands. It should come out of Phase 1 building on
`SDL_Renderer`; whether it *runs* is a separate question nobody has asked yet. Once the
Vulkan path has textured-quad batching from Phase 4, port it:

- Replace its `Blit2DTexture` usage with the batched textured quad. That is close to the
  whole port — see the state notes above.
- Move it from `Feature::Window2D` to the single window and engine.
- Delete `api/render/realtime/2D/` and `operation/2D/`, collapse `Window2D`/`Window3D` into
  one `Window`, and drop the `Window2D`/`Window3D` feature flags for a single windowing flag.
- Add the sprite/orthographic pass properly, so a 2D game gets painter ordering and no depth
  buffer without special-casing the engine.

### Ongoing — tests

Deliberately not last. This is independent of the render rewrite and blocked by nothing.

There is no working test suite today: Boost.Test sources sit in `v3dlibs/tests/`, and
`pong/run-unit-tests.sh` and `tetris/run-unit-tests.sh` invoke a `unit_tests` binary that no
CMakeLists builds. The existing tests cover `api/type` (AABBox, ArcBall, Bound2D, Camera),
`api/brep` (Face, HalfEdge, Vertex), `api/image`, `api/font`, `api/input` and the old command
layer.

Three tiers, decided 2026-08-30:

1. **Unit tests per api library** — each `api/<lib>` owns its own `tests/` and its own
   target. This is where the bulk of coverage should live.
2. **An integration suite across libraries**, for the behaviour that only appears when
   several of them are wired together — asset loading through config, input through event
   mapping, a scene collecting into a frame.
3. **Per-app suites**, for app logic that is not in a library — tetris board and tetrad
   rules, pong scoring and collision, voxel chunk meshing.

The layout partly exists already: `moya/tests/` holds real tests (Polygon, RenderContext,
Renderer, ReyesPrimitive, Vertex), `tetris/tests/` has a bare `TestMain.cxx`, and the two
`run-unit-tests.sh` scripts expect a per-app binary. So tier 3 is a revival, not an invention.

Work:

- Split `v3dlibs/tests/` by target library and move each file to its api home before the
  legacy tree is deleted. Drop the ones covering the removed command layer
  (`BindTest`, `CommandTest`, `CommandDirectoryTest`, `CommandInfoTest`, `EventInfoTest`,
  `InputEventAdapterTest`) unless the behaviour survived into `api/config` or `api/event`,
  in which case rewrite them against the new interfaces.
- `enable_testing()` at the root plus `add_test` per target, so one `ctest` run covers
  everything and individual suites stay separately runnable.
- Add ctest to CI alongside cpplint.
- Start coverage on the libraries that need neither a window nor a GPU: `type`, `brep`,
  `dag`, `image`, `asset`, `config`, `event`, `input`, `font`. Those can run in CI from day
  one, which the render libraries cannot — see open question 4.

Two items on `docs/TODO.md` — "Get tests working again" and "integrate tests into github
actions" — are this workstream.

### Ongoing — documentation

`CLAUDE.md` now covers build, lint, architecture and conventions. The gaps worth filling,
in rough order of value:

- The rationale for the Vulkan move. Nothing anywhere records why, and the same is true of
  the SDL3 upgrade. This does not need to be a formal ADR, but the reasoning should exist
  somewhere before it is forgotten.
- `docs/RenderingPipeline.md` and `docs/ECSDesign.md` are design notes written as open
  questions. Once Phase 2 settles the `Frame`/`Operation` question, rewrite the first to
  describe what exists.
- `docs/Dependencies.md` still lists sdl2 and does not mention Vulkan.

## Where moya and talyn fit

Suggestion: they do not need to fit into the realtime stack at all, and trying to make them
is the wrong goal.

Both are offline renderers. talyn already links only `v3dlib_log` and `v3dlib_image`; moya
links `v3dlib_type` and its own `v3dlib_moya`. Neither touches the window, render engine,
input or UI layers, and neither should. The natural boundary is that `api/` contains a set of
libraries with no realtime dependency — `type`, `brep`, `dag`, `image`, `log`, `asset` — and
the offline renderers are consumers of exactly those.

Concretely that means: keep them building, let them take from the non-realtime libraries, and
do not schedule them into the Vulkan work at all. If anything, they are a useful forcing
function for keeping those libraries free of realtime dependencies.

## Open questions

None outstanding. The last one — how render tests run in CI — is settled in
[ADR-0001](../adr/0001-ci-rendering-tests.md), pending acceptance.
