# Regrouping — `render/realtime`, libmoya, and three smaller moves

Drafted 2026-10-16 from a survey of `api/` and `moya/libmoya`, and **closed** the same day. Eight
steps: six move files, one sweeps comments, and one updates the documents. No step changes
behaviour.

| Step | | Step | |
|---|---|---|---|
| 1 `realtime/entity` | landed | 5 `ui::component::menu` | landed |
| 2 `realtime/lighting` | landed | 6 `libmoya/reyes` | landed |
| 3 `realtime/canvas` | landed | 7 libmoya comments | landed |
| 4 `image/format` | landed | 8 documents | landed |

The rule is the one in [Conventions.md](../../contributing/Conventions.md#directories): a directory
splits when its files serve different purposes, not when it reaches a size. A class in a
subdirectory drops the subdirectory's name, and a namespace follows its directory.
[ApiOrganisation](ApiOrganisation.md) applied that rule to six libraries. This plan
applies it to the directories that have grown since.

## Context

### The root of `api/render/realtime` holds six jobs

It holds 26 classes. `vulkan/` below it is already split, and each of its five subdirectories
has its own page in [internals/realtime/](../../internals/realtime/README.md). The root is not split,
and its classes fall into six groups. The user documents in [api/rendering/](../../api/rendering/README.md)
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
  `Samples`. [Reyes.md](../../offline/Reyes.md) describes them as one pipeline.

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
1 entity ───┐
2 lighting ─┤
3 canvas ───┤
4 format ───┼──> 8 documents
5 menu ─────┤
6 reyes ────┴──> 7 libmoya comments ──> 8
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

**Landed.** Build clean, 26 of 26 suites pass, cpplint clean. No app called these functions;
only the render tests did. Entities.md, Lighting.md and ECS.md name them as `entity::` now.

### Step 2 — `realtime/lighting/`

```
realtime/lighting/   Settings SceneUniforms pack() + the shadow functions   v3d::render::realtime::lighting
```

- `LitSettings` becomes `lighting::Settings`, by the rule that drops the directory's name.
- `SceneUniforms` keeps its name, because it mirrors the `Scene` block in `lit.glsl`.
- `Shadow.h` moves, and its nested `shadow` namespace goes. Its names take the word instead:
  `shadow::Bounds` becomes `lighting::ShadowBounds`, and `fit`, `fitView`, `light` and
  `minimumRadius` become `lighting::fitShadow`, `lighting::fitShadowToView`,
  `lighting::shadowLight` and `lighting::minimumShadowRadius`.

`Grade` stays at the root. It is a colour pass, and a directory of one file is not worth
making.

**Landed, as `lighting/` rather than `lit/`.** Apps and tests name their `renderer::Lit` `lit`,
so `lit.scene(lit::pack(settings, ...))` would compile and read badly. `lighting::` also
matches Lighting.md. Build clean, 26 of 26 suites pass, cpplint clean.

No app named these; the render tests and `renderer::Lit` did. Cozy does not use them either.
The tests name the namespace through an alias, `namespace lighting = ...`, as ShadowTest
already did for `shadow`. voxel has its own `SceneUniforms`, which is unrelated and unchanged.

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

**Landed.** 94 files changed. Build clean, 26 of 26 suites pass, cpplint clean over the tree,
and `examples/starter` builds. Outside the render library every use was already qualified, so
the rewrite there was mechanical. The forward declarations of `Canvas` in `api/ui` moved into
`canvas`.

A `using` for `canvas::Line` or `canvas::World` would collide with `renderer::Line` and
`renderer::World` in a test that names both. The tests that had one name the namespace through
an alias, `namespace canvas = ...`, instead. A renderer's `submit()` reads
`submit(const canvas::Line& canvas, ...)`, which compiles because a qualified name ignores the
parameter.

`prose.ts` diffs with renames off, so it read each moved file whole and found four long
sentences that predate the move. They are split. CI runs the same gate on a pull request.

### Step 4 — `image/format/`

```
image/format/   BmpFileHeader BmpInfoHeader BmpRgbQuad JpegError   v3d::image::format
```

Eight includes change, all in `image/reader/` and `image/writer/`. The names keep their `Bmp`
and `Jpeg` prefixes, because those name the format, not the directory.

**Landed.** Build clean, 26 of 26 suites pass, cpplint clean. The three Bmp headers declare
`bmp_file_header`, `bmp_info_header` and `bmp_rgb_quad`, not the names of their files, and
`JpegError.h` also declares `jpegErrorExit`. The two Bmp codecs and the two Jpeg codecs name
them as `format::` now. No document named them.

### Step 5 — `ui::component::menu`

`Menu`, `MenuBar` and `MenuItem` move into `v3d::ui::component::menu`, as `menu::Menu`,
`menu::Bar` and `menu::Item`. The files stay where they are, and are renamed to match.

`component::Type::MenuBar`, the enumerator the loader switches on, is not a class name and does
not change. Neither does any document a ui is loaded from.

**Landed.** 26 files changed. Build clean, 26 of 26 suites pass, cpplint clean. Every use
outside the directory was qualified, so `component::Type::MenuBar` could not match a rename.
Inside it, the rename skipped any name after `Type::`. Three `api/ui` headers forward-declared
`Menu` and `MenuBar` in their `component` block, and declare `Bar` and `Menu` in a
`component::menu` block now. `style::Menu` is a different class and is unchanged.

Cozy names the canvas and menu classes 23 times, and follows on its next submodule update.

### Step 6 — `libmoya/reyes/`

```
libmoya/         RenderMan RIBHandler Renderer RenderContext Shading FrameBuffer Hider RayHider
                 Polygon Vertex
libmoya/reyes/   Hider Primitive Bucket MicroPolygon MicroPolygonGrid GridShader Footprint
                 Samples                                                v3d::moya::reyes
                 <- ReyesHider leaves Hider.h; ReyesPrimitive becomes Primitive
```

`reyes::Hider` derives from `moya::Hider`. That is the same shape as `asset::Loader` above
`asset::loader::Json`: a base class above its implementations.

The root includes the pipeline, because `FrameBuffer` holds the buckets and `RenderContext`
holds the samples. A Reyes file that includes the front end does so for `Shading` or
`RenderContext`. Both directions are expected.

**Landed, with `Polygon` and `Vertex` at the root.** The plan put them in `reyes/`. The front
end builds them whichever hider is active, and `RenderContext::trace()` hands a `Polygon` to
the ray tracer, so they belong to both hiders. `Polygon` derives from `reyes::Primitive`, so
it can be diced. The ray path uses nothing else in `reyes/`.

Build clean, 26 of 26 suites pass, cpplint clean. `Footprint.h` and `MicroPolygon.h` also
declare `footprint()`, `shutterSlice()` and `covers()`, which the tests call as `reyes::` now.
`ReyesPrimitive.h` and `GridShader.h` forward-declared `RenderContext` inside their namespace
block. That declaration stays in `moya`. `RenderContext.h` declares `GridShader` inside
`reyes`. `moya/tests` stays flat. Reyes.md, RayTracing.md, ShadingLanguage.md, the offline
README and RunningAndTesting.md name the classes by their new names.

### Step 7 — libmoya's comments

A sweep by [the code-comments skill](../../../.claude/skills/code-comments/SKILL.md), as its own
commit, checked by `node scripts/commentsonly.ts --base HEAD~1`. Two comments are already known
to be wrong:

- `Renderer.h` says the renderer's base is "the RenderEngine class". No such class exists.
- `RenderContext.h` describes the class in one tab-indented line, "holds the current graphics
  state", with blank lines around it.

`RenderMan.h` keeps its Pixar copyright notice.

**Landed.** 27 files, comments only: `commentsonly.ts` finds no code change, and the build,
26 of 26 suites, cpplint and `prose.ts` are clean. About 160 comments were reworded and about
70 deleted, including commented-out code and the RI specification quoted at length in
`RenderMan.cxx`.

Forty-seven comments were false, and most described behaviour the code does not have. They
said that `prepareWorld()` freezes the options and `render()` releases the world. They said a
bucket's `left_` and `top_` hold its edges, and an unsupported projection falls back to
orthographic. The defects behind thirteen of them
are in [TODO.md](../../TODO.md), under moya's front end and reyes hider.

### Step 8 — The documents

Each step updates the reference documents that name what it moved, in the same commit. This
step checks the documents that describe the shape of the tree:

- [api/README.md](../../api/README.md) for the shape of `api/`.
- [api/rendering/](../../api/rendering/README.md) and
  [internals/realtime/README.md](../../internals/realtime/README.md) for steps 1 to 3. Both name
  classes and paths.
- [api/ui/](../../api/ui/README.md) and [UserInterface.md](../../internals/UserInterface.md) for step 5.
- [offline/](../../offline/README.md), Reyes.md above all, for step 6.
- This plan's entry in [README.md](../README.md), which moves to the completed table when the plan
  closes.

`CLAUDE.md` changes only if its routing table names a moved path.

**Landed.** Each step updated the documents for what it moved, so this step found nothing left
to change. None of the documents above lists a library's subdirectories, and `CLAUDE.md` names
no moved path.

## Method

[ApiOrganisation](ApiOrganisation.md) recorded what its renames cost. Each of these
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
- **The CMake lists are written by hand.** [api/render/CMakeLists.txt](../../../api/render/CMakeLists.txt)
  and [moya/libmoya/CMakeLists.txt](../../../moya/libmoya/CMakeLists.txt) name every source. A missed
  line is a link error at the end of a long build.

## Verification

For each step:

- **Build** the whole tree: `scripts\build.cmd`. A wrong path or a missed source fails here.
- **Tests:** `scripts\test.cmd`. Every suite passes with no change beyond includes and
  qualification. A test that needs more means the step moved more than files.
- **Lint:** cpplint over the tree, and `node scripts/prose.ts` on the documents.
- **`examples/starter`** builds after steps 1 to 3, as [examples/README.md](../../../examples/README.md)
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
- **It does not move `engine::IsometricController`.** [Input.md](../../api/engine/Input.md)
  documents it as engine API.
- **It does not move a library boundary.** Every move is inside one `v3dlib_*` target.

## Open questions

- **Whether voxel should include `vulkan/memory/Mesh.h`.** It is the only app that reaches into
  `vulkan/`. A draw item names a mesh, so this may be correct. Settle it before anything in
  `vulkan/memory/` moves.
- **Whether `realtime/component/` belongs under `entity/`.** It stays separate here, because its
  types are ECS components and apps name them beside `ecs::component`.

## Outcome

Drafted and closed on 2026-10-16. Six directories now hold one job each, and every header in
`api/` declares the namespace its directory names. No step changed behaviour: the build, 26 of
26 suites and cpplint were clean at every step, and `examples/starter` built after the canvas
move.

```
api/render/realtime/entity/     meshes() casters() sprites() particles() poses() Poses
api/render/realtime/lighting/   Settings SceneUniforms pack() fitShadow() shadowLight() ...
api/render/realtime/canvas/     Canvas Line World DepthOrder TransformStack ClipStack
api/image/format/               the bmp structs, JpegError
api/ui/component/menu/          menu::Menu menu::Bar menu::Item
moya/libmoya/reyes/             Hider Primitive Bucket MicroPolygon MicroPolygonGrid GridShader
                                Footprint Samples
```

Three things came out differently from the plan:

- Step 2 made `lighting/` rather than `lit/`. Code names its `renderer::Lit` `lit`, and
  `lit.scene(lit::pack(...))` reads badly.
- Step 6 left `Polygon` and `Vertex` at the root. Both hiders draw them.
- Each step updated its own documents, so step 8 had nothing left.

Two things are worth knowing before a move like these:

- `prose.ts` diffs with renames off, so it reads a moved file whole. Steps 3, 5 and 6 each
  split sentences that predate the move, because CI would have failed them on a pull request.
- A header declares more than its name says. `Footprint.h` holds two free functions, the Bmp
  headers declare snake-case structs, and `JpegError.h` holds `jpegErrorExit`. The compiler
  found each one.

Step 7 found forty-seven false comments in libmoya. The defects behind thirteen of them are open
in [TODO.md](../../TODO.md). Cozy names the moved canvas and menu classes 23 times, and follows when
it next updates its submodule.
