# api/ Design Review

A design-principles review of every library under `api/`, against the tree at `7beb5f3`
(2026-10-04): SRP, DRY, OCP, LSP, ISP, DIP, coupling, ownership and dead abstraction. Unlike the
other records here it is not about a tree being deleted; it is an itemised list of debt, kept here
because the list is the point and it will be worked off item by item.
[plans/ApiDesignDebt.md](../../plans/completed/ApiDesignDebt.md) is the plan that works it off.

## Method

All sixteen libraries were read, about 60k lines, in seven slices: `render/realtime`,
`render/offline` (with the hiders in `moya/libmoya`), `ui`, `engine`/`event`/`input`/`config`/`log`,
`asset`/`image`/`font`/`type`, `brep`/`grid`/`ecs`/`dag`/`audio`, and the edges between libraries.
Each slice was checked against the ADRs first: a shape an ADR decides is not listed here unless the
code no longer matches the ADR's own reasoning, and that is said where it applies. House style is
out of scope. Every defect in the first section was confirmed by reading the code a second time;
none was built or run.

## Verdict

The large-scale structure is sound. Dependencies mostly run one way, the leaves (`log`, `type`,
`dag`, `grid`, `event`) are clean, there is one math vocabulary (glm, with no wrapper types),
resources are handle-addressed with ring-deferred destruction, `ui` reaches `render` through a
narrow seam, and RIB parsing is separate from scene building.

The debt sits one level down and has one recurring shape: **a rule written in several places by
hand, which has already drifted.** Three ui tree walks disagree about tab pages; the two hiders bind
shading globals separately and disagree about `Oi`; four writers of a camera rotation where three
clear the cache; three renderers reset by a hard-coded list; six apps tearing down in the right
order by copying the same block. The highest-value changes are the ones that make each such rule
live in one place.

The second theme is `api/asset`: a closed loader registry that drags every media backend into every
closure, closes the tree's one dependency cycle, and has four failure conventions.

The third is dead OpenGL-era and speculative surface: about 940 lines of `WingedEdgeBRep`, seven
empty `dag` classes, a second and third font stack, `image::Texture`, `asset::Cache`.

## Defects found

These are bugs, not design notes, and are worth fixing ahead of anything else.

| # | Where | Defect |
|---|---|---|
| D1 | [Camera.cxx:305](../../../api/type/camera/Camera.cxx#L305) | `Camera::rotate` never clears `Profile::basisValid_`. Every config profile goes through `lookat()`, which sets it, so `createView()` uses the cached basis and **the editor's perspective arcball does nothing**. The all-components-non-zero guard also drops any single-axis rotation. ADR-0056 named a fourth writer forgetting the cache as its risk; this is it. |
| D2 | [ui/Engine.cpp:207](../../../api/ui/Engine.cpp#L207) | `focusable()` has no TabBar case, so tab order reaches controls on hidden pages; `shell::Keyboard` turns text input on for them and typing goes into a box nobody can see. |
| D3 | [ui/Container.cpp:66](../../../api/ui/Container.cpp#L66) | `probe()` picks children in reverse add order with no depth sort, while `Arranger::walk` draws by depth. A nested child with an out-of-order `depth` is drawn on top and clicked underneath — against ADR-0019's one walk for both. |
| D4 | [image/writer/Jpeg.cxx:37](../../../api/image/writer/Jpeg.cxx#L37) | The jpeg writer uses libjpeg's default error manager, whose `error_exit` calls `exit()`. An RGBA image sets `input_components = 4` with `JCS_RGB`, which libjpeg rejects — so converting an RGBA png to jpg ends the process. |
| D5 | [image/reader/Bmp.cxx](../../../api/image/reader/Bmp.cxx) | Throws `runtime_error` on truncated and 32-bit files; `Reader.h` promises null, and `Factory::read` does not catch. `Factory` also keys formats on the last three characters (`.jpeg` → `peg`; a short name throws `out_of_range`). |
| D6 | [Engine3D.cpp:190](../../../api/render/realtime/Engine3D.cpp#L190) | Geometry rings reset only for the three renderers `endFrame()` names. A `World` a game builds for the lit pass — the route RenderingPipeline.md documents — never resets, and allocates a new host-visible buffer pair every frame. |
| D7 | [moya/libmoya/GridShader.cxx](../../../moya/libmoya/GridShader.cxx) | The reyes hider never binds or reads `Oi` and forces opacity to 1 ([Bucket.cxx:303](../../../moya/libmoya/Bucket.cxx#L303)); the ray hider honours it. A shader setting `Oi` is translucent under `"raytrace"` and opaque under `"hidden"`, contradicting OfflineRenderers.md's "holds under either". |
| D8 | [BRep.cxx:342](../../../api/brep/BRep.cxx#L342) | `splitEdge` sets the new vertex on the copy (so it splits the *next* edge), duplicates `pair_`, and writes to the local after `push_back`. [BRepTest.cxx:130](../../../api/brep/tests/BRepTest.cxx#L130) asserts the wrong result on a mesh with no pairs. `extrudeFace`/`splitFace` are declared and never defined. Latent: no app calls them. |
| D9 | [vertical3d/src/main.cxx](../../../vertical3d/src/main.cxx) | The editor writes its own `main` and calls `shutdown()` inside the `try`, so an exception from the loop skips teardown — the case `run<T>` exists to handle. ADR-0028 already requires `run<>`. |
| D10 | [audio/Engine.cpp:66](../../../api/audio/Engine.cpp#L66) | Connects a dispatcher sink to `*this`; neither `shutdown()` nor the destructor disconnects it, and the destructor does not call `shutdown()`. No `disconnect` or `scoped_connection` exists anywhere in the tree (see A6). |
| D11 | [dag/Node.cxx:10](../../../api/dag/Node.cxx#L10) | A copied `Node` keeps its id, so a copied `BRep` aliases the original in the editor's scene and selection. ADR-0013's consequence "a copy duplicates the geometry under a second id" is wrong. |
| D12 | [AABBox.cxx:37](../../../api/type/geometry/AABBox.cxx#L37), [Bound2D.cxx:16](../../../api/type/geometry/Bound2D.cxx#L16) | `AABBox::origin()` returns the size; `Bound2D::expand`/`shrink` contradict their comments; the tests assert the wrong numbers. No production caller. |

Smaller ones, listed with their library below: `TextureFont`'s constructor leaves three members
uninitialised on a face failure; `JsonFile::read` never surfaces an error and `loader/Json.cpp` loops
forever on `ferror`; `Buffer::grow` waits for device idle mid-tick; a shadow map named as a texture is
described in the wrong layout; `rebind()` returns true when nothing was rebuilt.

## Cross-cutting

**A1. asset ↔ audio is a cycle, against ADR-0021.** `audio/Engine.h` includes `asset/kind/Json.h`
only to type one parameter of `load()`; asset links audio for the Wav loader. Both CMake files link
PUBLIC, and [audio/CMakeLists.txt:9](../../../api/audio/CMakeLists.txt#L9) admits it. Have `load()`
take `const boost::json::object&` — this restores ADR-0021's own reasoning rather than changing it.

**A2. asset is a hub with a closed registry.** [Manager.cpp:30](../../../api/asset/Manager.cpp#L30)
constructs all nine loaders and offers no way to register one, so asset links audio (SDL3_mixer),
font (Freetype) and image (png, jpeg), and everything above asset inherits them — `config`, which
only reads JSON, needs the mixer. That undercuts ADR-0033's "install only the closure you use".
Give `Manager` a `registerLoader`, keep Json and Text in asset, and move media loaders beside their
payloads. This also breaks A1 from the other side, and reopens a consequence ADR-0021 accepted.

**A3. Link visibility has drifted from the headers.** `render → image` and `ui → image` are PRIVATE
but `Grade.h` and `TextRenderer.h` include `Image.h`; `engine → SDL3` is PRIVATE but `Engine.h`
names `SDL_Event`; `render → ecs` is PUBLIC with no header use; `font → type` is unused. Each
CMake comment says the opposite of the code. It compiles only because asset exports image and
render exports SDL3 — so A2 would break it. `v3d_api_verify_manifest` checks link lines against
the manifest, never includes against link lines.

**A4. Asset loading has four failure conventions.** Null (Png, Jpeg, Tga, Wav, Json, Gltf); throws
(Text, unknown type or extension in `Manager`); silent null with no log (the font loaders on a
missing option); a non-null asset broken inside (`Font2D` ignores `build()`; `TextureFont` returns
early from its constructor). Twelve production sites repeat `dynamic_pointer_cast<kind::X>(load(…))`,
where a wrong cast looks like a missing file. One contract — null and a log line — plus a typed
`load<T>(name)` covers both. `Manager` also does not cache, which ADR-0030 lists as a Pro.

**A5. Loader options are a string-keyed bag on a shared loader.**
[TextRenderer.cpp:67](../../../api/ui/paint/TextRenderer.cpp#L67) sets `"fontSize"` and `"spread"` on
the Manager's one `TextureFont` loader, and they stay set for the next caller. `"fontSize"` is read as
`unsigned` by one loader and `float` by another; a mismatch throws `bad_variant_access`. Pass typed
options with the request.

**A6. Nothing disconnects a dispatcher listener.** Raw `*this` is connected in `event::Engine`,
`audio::Engine` and all five apps, against a `shared_ptr` dispatcher that can outlive them. Safe
today only because everything dies together; ADR-0074 already rebuilds renderers mid-run. Hold
`entt::scoped_connection` members.

**A7. A config document crosses boundaries in two shapes.** `audio`, `config/CameraProfiles`,
`config/SpriteSheets` and `ui::Engine` take `shared_ptr<asset::kind::Json>`; `ui::Loader`,
`engine` and `asset::Writer` take `boost::json`. The first form is the only reason audio — and
partly config — depend on asset. Parsers should take `const boost::json::object&`.

**A8. Reading a whole file is written five times with three contracts** — `JsonFile`, `loader/Text`,
`image::Reader`, `BitmapFont`, `sl::ShaderLibrary`.

## By library

### engine, event, input, config, log

- **E1 (high) — the base class relies on call-super, enforced by copies.** `shutdown()` is public
  and virtual; six apps repeat `if (renderer_) renderer_->shutdown(); return Engine::shutdown();`
  and five handlers carry a comment saying "quit(), not shutdown()". Apps hide `initialize(int)`
  with `bool initialize()`, and three of them omit `override` on `render`/`shutdown`. Make
  `shutdown()` non-virtual and reachable only from `run<T>`, calling a protected `release()` hook
  first — or put window teardown in `~Engine`, so C++ destruction order does it. The same shape fits
  start-up. ADR-0032's tick/simulate split has no type-level fix and is not in scope.
- **E2 — pong, tetris and voxel still copy the shell.** The `GameMenu` + ui-config block, the "ui"
  context handler (toggle, quit, navigate), the statistics sample and the identical `Feature` mask
  appear three to five times. Let `ui::shell::GameMenu` subscribe to its own commands and the engine
  answer a reserved quit. Extends ADR-0028; ADR-0074 rules out putting it on `engine::Engine`.
- **E3 — `Engine` is also a binding subsystem.** About 120 of ~400 lines parse binding documents,
  hold the rebind overlay and answer `held()`. Move them to an `event::Bindings` that needs no SDL.
  The window config is read with throwing `.at()` while bindings are guarded.
- **E4 — raw keys and commands are one `Event` type on one sink.** Every listener filters at
  runtime, and because `event::Engine` re-triggers commands inside the publish, a later listener
  sees the command before the key. ADR-0017 records the double-handling defect this causes as a
  consequence, not a decision. Give them distinct types and deliver commands after the source.
- **E5 — `Engine` hands out nine mutable protected members**, most null until `initialize()` and
  some depending on `Feature` flags. Only `registry_` is protected by decision.
- **E6 — `engine` requires Vulkan** through the concrete `Window` in `Engine.h`. `Accumulator`,
  `Statistics`, `Settings` and the path helpers cannot be had without it.
- **E7 — the logger is a global behind an injected pointer**, and its file is `v3d.log` in the
  working directory, not beside the executable as NewProject.md and the CI workflow say.
- **E8 — config types are a closed enum and an unknown type fails start-up**, so an app cannot add
  a document of its own (odyssey loads its map around it).
- **E9 (low)** — `Feature`/`DeviceType` are untyped `int` masks with copied operators; key names map
  one way only, so a typo in `mappings.json` is a silent dead binding; `event::Engine::dispatch`,
  `Event::operator()`, `config/Sounds.h` and `Context`-as-object are unused.

### render/realtime

- **R1 — Quad, Line and World repeat one streaming skeleton**: the six-collaborator constructor,
  ring sizing, `claim`/`endFrame`/`cursor_`, the wait-grow-write preamble, pipeline choice, clip to
  `VkRect2D`, and an unread `logger_`. A `StreamRing` owned by the in-flight ring would also fix D6
  without anyone calling `endFrame()`.
- **R2 — `renderer::Quad` is the texture and material registry.** `MeshRegistry`, `Lit` and `World`
  get albedo, set 1's layout and the white texture from the 2D renderer, so a default
  `DeviceContext` (colour `VK_FORMAT_UNDEFINED`) cannot register a texture — `quads()` compiles a 2D
  pipeline that `Builder` refuses. That contradicts ADR-0051's "a headless context can build every
  renderer". `TextureFactory` and `RenderTarget::ready()` also make their own uploaders. ADR-0042
  and 0065 name Quad as the source, but the reasoning (one shared set 1 pool) holds equally for a
  `Textures` service on `DeviceContext`.
- **R3 — the sampled-image layout is decided in three places.** `FullScreen` and `Lit` derive it
  from the aspect; `Quad::material` hard-codes `SHADER_READ_ONLY`, while `Quad::depthTexture()`
  hands out a depth image the recorder leaves in `DEPTH_READ_ONLY`. Six hand-written descriptor
  writes. Let a texture answer its own `VkDescriptorImageInfo`.
- **R4 — image barriers are built by hand in six files**, and `RenderTarget::toReadable`
  re-spells `Recorder::closeTarget`'s masks so that one "looks like" the other. `Grade` carries its
  own 3D staging upload. One internal `Barriers.h` of named transitions.
- **R5 — `Buffer::grow` calls `vkDeviceWaitIdle` on the frame path**, the option ADR-0061 rejected;
  `Lit::reserve` already retires through the ring instead.
- **R6 — `SortKey`'s pipeline and material fields are copied by hand** from the item, with a
  truncating cast, at four sites, and missing from lit meshes and `FullScreen` — so sorting a lit
  pass groups nothing. `SortKey.h` still says nothing sorts. Derive them in `Pass::submit`.
- **R7 (low)** — `Context` and `realtime::Engine` are vestigial bases (voxel downcasts
  `context()`; `Engine` has a non-virtual destructor and an unread `registry_`); the window-depth
  decision is computed with different predicates in `Engine3D` and `Recorder`; clip and transform
  stacks are copied across the canvases; 45 hand-written `VkResult` throw blocks want a
  `device::check`.

### render/offline

- **O1 — shading-point setup is copied between `trace::HitShader` and `moya::GridShader`**:
  running a light, `put()`, the per-program machine cache, and binding thirteen globals by string
  literal per hit. D7 is the drift. A shared globals map resolved once per program, plus a
  "run light over a batch" routine, leaves each hider supplying only per-point data. ADR-0077
  points this way.
- **O2 — `HitShader` is the shading callback, the ray compositor and a trace service for the other
  hider**, with `hit_`/`placement_`/`depth_` saved and restored by hand on three paths (one failure
  path restores only two). Split out a `Tracer`; make the per-hit adapter a stack object.
- **O3 — `trace::Scene` is closed to new primitives**: parallel vectors per type, one pointer per
  type in `Nearest`, a branch in `fill`, and motion poses re-inverted on every ray. A virtual
  `intersect` or a variant, and poses computed per sample time — the shape the planned acceleration
  structure needs.
- **O4 — a built-in is registered in five places**, and its body is found by `strcmp` over ~57 names
  on every call. A name in the signature table but not in `lookup` compiles and returns its default.
  Store the body in the `Signature`.
- **O5 — `sl::Compiler` is four passes in one 1,222-line class**, and adding a syntax node kind means
  editing eight switches. A generic `forEachChild` and inference in its own class.
- **O6 (low-medium)** — `Machine::prepare(program)` and `run(program)` take the program twice and
  never check it is the same; the RIB reader decides renderer capability (motion primitives,
  `AreaLightSource`, `MakeTexture`) instead of forwarding to `Handler` with default bodies as
  ADR-0025 does elsewhere; reader request groups repeat the same parse-and-forward; the two lexers
  and `Token.cxx` are copies; the hider is a `bool` and `RayHider` rebuilds the projection
  analytically instead of inverting the one `RenderContext` builds; skinned shaders are near-copy
  files because `v3d_add_shader` takes no defines; OfflineRenderers.md:45 names the wrong CMake file.

### ui

- **U1 (high) — three tree walks encode "which children, in what order" separately** (`Arranger::walk`,
  `Container::probe`, `focusable`), which produced D2 and D3. One "live children in draw order"
  function, used by all three and `resolveComponentImages`.
- **U2 — ADR-0047's compiler-checked switches cover only the switches.** About twenty unchecked
  `type() ==` tests and `dynamic_cast` probes hold the rest of the per-type rules, which is where U1
  drifted. One exhaustive `traits(Type)` switch would bring them under C4062.
- **U3 — strip buttons and menu items are held outside `children()`**, so a disabled toolbar's
  buttons draw enabled, `enabled`/`style`/`name`/`visible` on a toolbar button are dropped by the
  loader, a disabled menu item still dispatches, and every walk needs a special case. Contradicts
  ADR-0059's "disabling a component disables what it holds".
- **U4 — paint does layout.** Strip contents and menu panels are placed only in
  `ComponentRenderer`, so layout without a canvas — which UserInterface.md promises — leaves them
  unplaced; seven `draw` overloads repeat the size fallback, and `place()` is duplicated.
- **U5 — four places dispatch commands with a hand-copied null guard**, and `Toolbar` and `Menu`
  each hold a dispatcher, against ADR-0038's "the Cursor dispatches".
- **U6 — the game menu keeps a second keyboard model** (levels, navigation, value capture) beside
  `TextBox` + `Keys`. Either fold it under ADR-0040 or record why not.
- **U7 (low)** — `draw(Menu)` copies `panel()` and ignores theme border and radius; `Dressing.track`
  and `.mark` mean different things per style class; `Immediate` duplicates its scroll region and
  the `Scrollbar` thumb maths; `arrange` and `wrapped` repeat child sizing; `Component` is copyable
  with a raw `parent_`; the loader compares `"tabs"` as a string after parsing; `Component.h`
  includes `style/Theme.h` unused.

### asset, image, font, type

- **T1 — adding a format edits three switches**, and the extension maps in `Manager`,
  `image::Factory` and `Gltf` already disagree (`.bmp` reads but does not load; `.jpeg` is missing;
  `.txt` has a loader and no extension). Let a loader declare its extensions.
- **T2 — the Png, Jpeg and Tga asset loaders are identical** but for the reader class; so are the
  five `kind::` wrappers. One `loader::Image` over `image::Factory`, which `Gltf` already uses.
- **T3 — three font stacks, one used.** `Font2D` (leaks `FT_Library` on every early return),
  `BitmapFont`/`BitmapTextBuffer` (dereferences a null image), and the GL-shaped `image::Texture`
  have no consumer. Retire them with `asset::*::Font2D` and `asset::Cache`.
- **T4 — `TextureFont` carries features it never applies**: kerning built O(n²) on every glyph miss
  and never read; eight markup fields unread; an outline path that cannot be reached; a filtering
  enum passed as an atlas depth; mixed manual and RAII FreeType cleanup; a raw buffer with no
  deleted copy.
- **T5 — `Camera` and `Profile` store one orientation three ways** (D1 is the result), and `pan`/
  `tilt` leave the axes stale. One representation, or one private setter. `Camera`'s matrices also
  start uninitialised and its queries are non-const.
- **T6 (low)** — `Plane::intersect` duplicates `Ray::intersects`; writers use `Image` as a scratch
  buffer; `Image` has dimension setters that do not reallocate; the RGB↔BGR swap is written four
  times; `Loader::reset`, `Loader::manager_` and `load(…, hasPath)` are unused; `JsonFile` is not an
  asset kind.

### brep, dag, grid, ecs, audio

- **B1 — every topology invariant is left to the caller.** Raw inserts and public setters on every
  `HalfEdge` field; the only mesh validation is in the editor
  ([Project.cxx:240](../../../vertical3d/src/scene/Project.cxx#L240)); `Face()` leaves `edge_`
  uninitialised. A `BRep::validate()` or a checking builder.
- **B2 — the editor re-implements traversal.** The library's iterators need a `shared_ptr`, are
  non-const and loop forever on a broken ring, so the editor hand-walks `next()` with a guard and
  encodes "two halves are one edge" twice; 22 index casts. Move face-loop and edge-identity queries
  into `api/brep`.
- **B3 — `WingedEdgeBRep` and `Edge` are 938 lines with no consumer**, a parallel copy of `BRep`.
  Its `Transform` override changes `translation(v)` from a set to an add on selection — what
  ADR-0013 made it a setter to avoid — which argues for making `dag::Transform` non-virtual.
- **B4 — seven of `dag`'s nine classes are empty or inaccessible** and unused; ADR-0013 rejected
  growing it but did not decide to keep them.
- **B5 — pong wires audio itself.** The engine construction and the `Resolve` lambda are the generic
  adapter, written in the app; ECSDesign.md says `Engine::initialize` creates the audio engine, and
  it does not. `MIX_*` types sit in public headers, which is why SDL3_mixer is PUBLIC.
- **B6 (low)** — `dag::Transform` and `ecs::component::Transform` both compose T·R·S;
  `PositionFixed2D` duplicates `grid::TileCoord`; the four class-style ecs components each have one
  consumer and two are uncopyable; `grid`'s flood is written twice.

## Strengths worth keeping

- `Registry<Tag, Resource>` with generational `Handle<Tag>`, shared by `Resources` and
  `MeshRegistry`; `Ring::retire` as the one deferred-destruction seam.
- `Recorder::check` and the CPU canvases: device-free, so the batching and validation are tested.
- `pipeline::Builder` builds from the same accessors its tests assert.
- `rib::Reader` → `rib::Handler`, a small `sl::runtime::Renderer` seam, and no globals in the
  offline library.
- `ui`'s one-way, narrow dependency on render with no Vulkan type in a ui header; weak back-
  references throughout; cached style resolution.
- `Accumulator`, `Statistics` and `Settings`: pure, final and tested; `quit()` as a flag and `run<T>`
  owning the shutdown order are the right "make the mistake impossible" moves — E1 asks for more.
- `api/grid` (device-free, predicate-driven, 73+ cases) and `api/ecs` (`Previous<T>` behind a
  concept) are the best-shaped libraries in the tree.
- One math vocabulary, and genuine reuse of `type` by the offline tracer and the ui.
- The library manifest and `v3d_api_verify_manifest` catch link drift.

## Suggested order

1. The defects, D1–D12. Most are a few lines; D2/D3 are best fixed by U1.
2. A1 then A3, then A2: break the cycle, correct visibility so nothing relies on a transitive link,
   then narrow asset. A4, A5 and T1–T3 fall out of the same work.
3. E1 and E2: make the teardown rule structural and finish the shell, so CLAUDE.md's two costliest
   rules stop depending on copies.
4. U1–U3 and O1: the single-traversal and single-binding refactors that the drift came from.
5. R1–R3: a `StreamRing` and a `Textures` service on `DeviceContext`.
6. Deletions: B3, B4, T3, the dead items in E9 and T6. Cheap, and they shrink what the rest has to
   keep consistent.

Items that change a decision — A2 against ADR-0021's accepted consequence, E4 against ADR-0017's
single event type, U2 against ADR-0047's count — want an ADR alongside.
