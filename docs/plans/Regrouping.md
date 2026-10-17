# Regrouping — `render/realtime`, libmoya, and three smaller moves

Drafted 2026-10-16 from a survey of `api/` and `moya/libmoya`. Eight steps: six move files,
one sweeps comments, and one updates the documents. No step changes behaviour.

| Step | | Step | |
|---|---|---|---|
| 1 `realtime/entity` | open | 5 `ui::component::menu` | open |
| 2 `realtime/lit` | open | 6 `libmoya/reyes` | open |
| 3 `realtime/canvas` | open | 7 libmoya comments | open |
| 4 `image/format` | open | 8 documents | open |

The rule is the one in [Conventions.md](../contributing/Conventions.md#directories): a directory
splits when its files serve different purposes, not when it reaches a size. A class in a
subdirectory drops the subdirectory's name, and a namespace follows its directory.
[ApiOrganisation](completed/ApiOrganisation.md) applied that rule to six libraries. This plan
applies it to the directories that have grown since.

## Context

### The root of `api/render/realtime` holds six jobs

It holds 26 classes. `vulkan/` below it is already split, and each of its five subdirectories
has its own page in [internals/realtime/](../internals/realtime/README.md). The root is not split,
and its classes fall into six groups. The user documents in [api/rendering/](../api/rendering/README.md)
already split along the same lines:

| Group | Classes | Outside references | Document |
|---|---|---|---|
| Contexts and the frame | Engine, Engine3D, Context3D, DeviceContext, Window, Frame, Pass, DrawItem, SortKey | many | FramesAndTargets.md |
| Resources | Handle, Registry, Textures, MeshRegistry, DecodedModel | a few each | TexturesAndMeshes.md |
| Canvases | Canvas, CanvasStacks, LineCanvas, WorldCanvas, DepthOrder | about 290, and 24 in cozy | Canvas.md, LinesAndWorldQuads.md |
| Entity drawing | Meshes, Sprites, Particles, Poses | about 50 | Entities.md |
| Lighting | LitSettings, SceneUniforms, Shadow | about 90 | Lighting.md |
| Colour | Grade | 3 | ColourAndPost.md |

`Shadow.h` already declares `v3d::render::realtime::shadow` from the root directory. It is one
of the four headers in `api/` whose namespace does not follow its directory.

### `api/image` holds format structs beside its public API

`BmpFileHeader`, `BmpInfoHeader`, `BmpRgbQuad` and `JpegError` describe on-disk formats and a
libjpeg error manager. Only `image/reader/` and `image/writer/` include them. They sit at the root
beside `Image`, `Reader`, `Writer` and `TextureAtlas`, which every consumer uses.

### `ui/component/menu` declares the wrong namespace

`Menu`, `MenuBar` and `MenuItem` live in `api/ui/component/menu/` and declare
`v3d::ui::component`. They are the other three of the four mismatched headers. The fix renames
109 references.

### libmoya is one directory of three jobs

`moya/libmoya` holds 17 classes in `v3d::moya`:

- **The front end:** `RenderMan` (the RenderMan Interface C API), `RIBHandler`, `Renderer`,
  `RenderContext`, `Shading` and `FrameBuffer`.
- **The hiders:** the `Hider` base and `RayHider`. `RayHider` includes only `FrameBuffer` and
  `RenderContext`, and draws through `api/render/offline/trace`.
- **The Reyes pipeline:** `ReyesHider` (declared in `Hider.h`), `Bucket`, `ReyesPrimitive`,
  `Polygon`, `MicroPolygon`, `MicroPolygonGrid`, `GridShader`, `Vertex`, `Footprint` and
  `Samples`. [Reyes.md](../offline/Reyes.md) describes them as one pipeline.

The Reyes files are ten of the seventeen, and the `Reyes` prefix on two of them is a namespace
written by hand.

## Decisions

No ADR. A directory move can be undone with `git mv` and constrains no other code. The canvas
renames change public names, and cozy and `examples/` have to follow them. That is a cost, not
a design choice: the old and new names say the same thing.

ADRs that name a moved class are not edited. An ADR records what was decided when it was
decided.

## What blocks what

Steps 1 to 6 are independent of each other. Each lands as one commit with the tree building,
so a later step never edits a half-moved directory. Do them in order of cost, cheapest first,
so the method is settled before step 3.

Step 7 waits for step 6, so that the sweep sees the final paths and the move commit stays a
pure move. Step 8 records every step that landed.

```
1 entity ─┐
2 lit ────┤
3 canvas ─┤
4 format ─┼──> 8 documents
5 menu ───┤
6 reyes ──┴──> 7 libmoya comments ──> 8
```

## Steps

### Step 1 — `realtime/entity/`

```
realtime/entity/   meshes() casters() sprites() particles() poses() Poses   v3d::render::realtime::entity
```

`Meshes.h`, `Sprites.h`, `Particles.h` and `Poses.h` move. They hold the functions that walk the
ECS registry and submit draws. The function names stay: `entity::sprites(registry, ...)` reads
correctly.

`realtime/component/` stays where it is. It holds the ECS components, and apps name them as
`realtime::component::Sprite` beside `ecs::component::Transform`.

### Step 2 — `realtime/lit/`

```
realtime/lit/   Settings SceneUniforms pack() + the shadow functions   v3d::render::realtime::lit
```

- `LitSettings` becomes `lit::Settings`, by the rule that drops the directory's name.
- `SceneUniforms` keeps its name, because it mirrors the `Scene` block in `lit.glsl`.
- `Shadow.h` moves, and its nested `shadow` namespace goes. Its names take the word instead:
  `shadow::Bounds` becomes `lit::ShadowBounds`, and `fit`, `fitView`, `light` and
  `minimumRadius` become `lit::fitShadow`, `lit::fitShadowToView`, `lit::shadowLight` and
  `lit::minimumShadowRadius`.

`Grade` stays at the root. It is a colour pass, and a directory of one file is not worth
making.

`lit` is also the name of `vulkan::renderer::Lit`. That is deliberate: `lit::Settings` is how
`renderer::Lit` draws.

### Step 3 — `realtime/canvas/`

```
realtime/canvas/   Canvas Line World DepthOrder TransformStack ClipStack   v3d::render::realtime::canvas
                   <- was LineCanvas, WorldCanvas; CanvasStacks.h becomes Stacks.h
```

This is the largest step: about 290 references outside the library, and 237 mentions of the
three prefixed names. `canvas::Canvas` keeps its name, because the directory names the class's
noun.

`canvas::Line` and `canvas::World` match `vulkan::renderer::Line` and `vulkan::renderer::World`,
which draw them. Code that names both writes the namespace.

`examples/starter` changes in the same commit. Cozy changes when it next updates its submodule.

### Step 4 — `image/format/`

```
image/format/   BmpFileHeader BmpInfoHeader BmpRgbQuad JpegError   v3d::image::format
```

Eight includes change, all in `image/reader/` and `image/writer/`. The names keep their `Bmp`
and `Jpeg` prefixes, because those name the format, not the directory.

### Step 5 — `ui::component::menu`

`Menu`, `MenuBar` and `MenuItem` move into `v3d::ui::component::menu`, as `menu::Menu`,
`menu::Bar` and `menu::Item`. The files stay where they are, and are renamed to match.

`component::Type::MenuBar`, the enumerator the loader switches on, is not a class name and does
not change. Neither does any document a ui is loaded from.

### Step 6 — `libmoya/reyes/`

```
libmoya/         RenderMan RIBHandler Renderer RenderContext Shading FrameBuffer Hider RayHider
libmoya/reyes/   Hider Primitive Bucket Polygon MicroPolygon MicroPolygonGrid GridShader
                 Vertex Footprint Samples                               v3d::moya::reyes
                 <- ReyesHider leaves Hider.h; ReyesPrimitive becomes Primitive
```

`reyes::Hider` derives from `moya::Hider`. That is the same shape as `asset::Loader` above
`asset::loader::Json`: a base class above its implementations.

The root still includes the pipeline, because `RenderContext` holds a `Polygon` list and a
`Samples`. A Reyes file that includes the front end does so for `Shading` or `RenderContext`.
Both directions are expected.

`moya/tests` stays flat, and its includes change.

### Step 7 — libmoya's comments

A sweep by [the code-comments skill](../../.claude/skills/code-comments/SKILL.md), as its own
commit, checked by `node scripts/commentsonly.ts --base HEAD~1`. Two comments are already known
to be wrong:

- `Renderer.h` says the renderer's base is "the RenderEngine class". No such class exists.
- `RenderContext.h` describes the class in one tab-indented line, "holds the current graphics
  state", with blank lines around it.

`RenderMan.h` keeps its Pixar copyright notice.

### Step 8 — The documents

- [api/README.md](../api/README.md) for the shape of `api/`.
- [api/rendering/](../api/rendering/README.md) and
  [internals/realtime/README.md](../internals/realtime/README.md) for steps 1 to 3. Both name
  classes and paths.
- [api/ui/](../api/ui/README.md) and [UserInterface.md](../internals/UserInterface.md) for step 5.
- [offline/](../offline/README.md), Reyes.md above all, for step 6.
- This plan's entry in [README.md](README.md), which moves to the completed table when the plan
  closes.

`CLAUDE.md` changes only if its routing table names a moved path.

## Method

[ApiOrganisation](completed/ApiOrganisation.md) recorded what its renames cost. Each of these
held there and holds here:

- **Rewrite qualified names and include paths, not bare identifiers.** `Canvas` is a substring
  of `LineCanvas`, `WorldCanvas` and `CanvasStacks`. `Line`, `World` and `Hider` already name
  other classes. Rename the longest name first, and read every hit of the old name that is not
  the class itself.
- **Grep for forward declarations.** `api/ui` declares `class Canvas;`, `class Menu;` and
  `class MenuBar;` in five headers. A forward declaration left in the old namespace declares a
  second, unrelated type, and no include-based check finds it.
- **List what a header declares, not what it is called.** `Hider.h` holds `ReyesHider`, and
  `CanvasStacks.h` holds two classes with other names.
- **The CMake lists are written by hand.** [api/render/CMakeLists.txt](../../api/render/CMakeLists.txt)
  and [moya/libmoya/CMakeLists.txt](../../moya/libmoya/CMakeLists.txt) name every source. A missed
  line is a link error at the end of a long build.

## Verification

For each step:

- **Build** the whole tree: `scripts\build.cmd`. A wrong path or a missed source fails here.
- **Tests:** `scripts\test.cmd`. Every suite passes with no change beyond includes and
  qualification. A test that needs more means the step moved more than files.
- **Lint:** cpplint over the tree, and `node scripts/prose.ts` on the documents.
- **`examples/starter`** builds after steps 1 to 3, as [examples/README.md](../../examples/README.md)
  describes.

At the end, run the editor, one 2D game and odyssey once each with the validation layer on.
Nothing here changes a draw, so this checks that the tree links and starts.

## What this plan does not do

- **It does not move `Lexeme` and `Characters` out of `api/render/offline`.** They are what
  `rib/` and `sl/` share, and the root of the library is where shared files belong. Two files
  are not a directory.
- **It does not make a directory for models.** `MeshRegistry` and `DecodedModel` are two files,
  and they belong with `Textures` as what a context holds.
- **It does not split `sl/syntax`, `ui/component`, `event/kind` or `grid`.** Each holds one
  concept, repeated.
- **It does not merge the one-file `kind/` and `loader/` directories** in `asset/`,
  `asset/media/` and `audio/`. They are one pattern across three libraries.
- **It does not move `engine::IsometricController`.** [Input.md](../api/engine/Input.md)
  documents it as engine API.
- **It does not move a library boundary.** Every move is inside one `v3dlib_*` target.

## Open questions

- **Whether voxel should include `vulkan/memory/Mesh.h`.** It is the only app that reaches into
  `vulkan/`. A draw item names a mesh, so this may be correct. Settle it before anything in
  `vulkan/memory/` moves.
- **Whether `realtime/component/` belongs under `entity/`.** It stays separate here, because its
  types are ECS components and apps name them beside `ecs::component`.
