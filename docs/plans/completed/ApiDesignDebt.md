# API Design Debt — Twelve Defects, One Cycle, And Rules That Live In One Place

Drafted 2026-10-04 from [the api/ design review](../../audits/completed/ApiDesignReview.md), which is this plan's
context and is not restated here: every step names the review items it closes by their ids (D1,
A2, U1, …), and the review holds the evidence for each. Thirty-two steps in nine phases, touching
every library under `api/` and every app that subclasses `engine::Engine`.

The review's verdict decides the shape of the plan. The structure is sound; the debt is **a rule
written by hand in several places, which has already drifted**, and most of the twelve defects are
that drift showing. So the defects are fixed first, where each is a few lines, and the refactors
that follow are the ones that would have prevented them: one traversal, one set of shading
globals, one teardown path, one reset of a geometry ring.

## Decisions

Recorded in [adr/](../../adr/), not here. Numbers are the next free ones at the time of writing;
[adr/README.md](../../adr/README.md) is the authority.

| ADR | Decision | Step |
|---|---|---|
| **0079** | An asset loader is registered, and a media loader lives with its payload — revisits the consequence of ADR-0021 that the mixer rides along on everything linking `v3dlib_asset` | 11 |
| **0080** | The engine owns its lifecycle: `shutdown()` is not virtual, and an app supplies hooks | 14 |
| **0081** | A key and a command are different events — amends [ADR-0017](../../adr/0017-a-command-is-a-name-in-a-context.md), which recorded the single type's cost as a consequence, not a decision | 17 |
| **0082** | Textures and materials belong to the device context, not to the 2D renderer — amends [0042](../../adr/0042-a-textured-quad-in-world-space.md) and [0065](../../adr/0065-a-mesh-is-registered-by-path-and-released.md), whose reasoning (one shared set 1 pool) is kept | 23 |
| [0013](../../adr/0013-mesh-is-a-dag-node.md) | **Corrected** by steps 3 and 6: a copy does not get a new id today, and the dag skeletons it declined to grow are deleted |
| [0021](../../adr/0021-sdl3-mixer-replaces-soloud.md) | **Restored** by step 10: audio stops depending on asset, which is what its decision already says |
| [0030](../../adr/0030-a-model-is-an-interleaved-array-that-names-its-texture.md) | **Corrected** by step 12: either `Manager` caches or the Pro that says it does is struck |
| [0047](../../adr/0047-a-component-type-is-checked-by-the-compiler.md) | **Corrected** by step 20: the compiler names every place only once the predicates are a switch too |
| [0056](../../adr/0056-a-look-at-keeps-the-basis-it-built.md) | Enforcement **replaced** by step 1: one setter clears the cache, rather than every writer remembering to |
| [0059](../../adr/0059-disabled-is-a-property-of-a-component.md) | **Extended** by step 21 to strip buttons and menu items |

Whether step 18 (an engine without Vulkan) and step 22 (one keyboard model for the menu) earn an
ADR depends on which way they go; each step says.

## What blocks what

```
Phase 1 — defects                  Phase 2 — subtraction
1  camera rotation       ─┐        6  brep/dag dead code ──┐
2  image contracts        │        7  font/image/asset/type dead├──> everything after
3  brep splitEdge, Node   │        8  engine/event/render  │
4  editor main → run<>    │           dead code ───────────┘
5  scoped connections     │
   (each independent)    ─┘
                                   Phase 3 — the dependency graph
                                   9  link visibility + include check
                                   10 parsers take json; asset↔audio cycle broken
                                   11 ADR-0079: open registry, loaders move ──┐
                                   12 one loader contract, typed load<T>     │
                                                                             │
Phase 4 — engine                                                             │
13 binding subsystem → event::Bindings                                        │
14 ADR-0080: engine owns its lifecycle ──> 15 the shell finishes <───────────┘
16 protected state → accessors
17 ADR-0081: keys and commands apart
18 engine without Vulkan (decide)
19 logger and config types

Phase 5 — ui          Phase 6 — realtime      Phase 7 — offline      Phase 8 — geometry
20 one traversal      23 ADR-0082 Textures    26 shared globals      29 brep invariants
   + traits switch    24 StreamRing, grow        (fixes Oi)          30 transforms, tiles,
21 strips as children    retires              27 Tracer, Scene          ecs components
22 layout out of      25 layouts, barriers,   28 built-ins,          31 TextureFont, camera,
   paint, menus          SortKey, the rest       compiler, reader       type leftovers
                       (independent of each other, after phase 2)

Phase 9 — 32 the documents that own what moved <── everything
```

**Phase 1 goes first because it is cheap and it is broken.** Each step is a few lines plus a test
that would have caught it, and none depends on another. Three defects are deliberately left to
the refactor that removes their cause rather than patched twice: D2 and D3 to step 20, D6 to
step 24, D7 to step 26.

**Phase 2 before anything that renames or reshapes**, so nothing is refactored on its way to being
deleted. Step 7 in particular removes the `Font2D` loader that step 11 would otherwise move.

**Phase 3 is ordered within itself.** Step 9 first, because narrowing asset in step 11 removes
the transitive links that the wrong visibility currently relies on; correcting it first means
step 11's breakage is only the breakage it intends. Step 10 before 11 because breaking the cycle
from the audio side is one parameter type, and step 11 is then free to move the Wav loader
without a cycle to reason about.

**Step 15 waits on steps 11 and 14.** The shell takes over the sound resolver (which needs the
loaders where step 11 puts them) and the start-up sequence (which step 14 reshapes).

**Phases 5 to 8 are independent of each other** and can be taken in any order, or interleaved,
once phase 2 is in. Phase 4 and phase 5 touch the same apps' menu handling in steps 15 and 22;
whichever lands second rebases.

## Steps

### Phase 1 — Defects

#### Step 1 — A camera's rotation has one writer

D1, and the structural half of T5. In [`api/type/camera/`](../../../api/type/camera/).

**Closed.** `Profile`'s members are private; `turn()` is the one composing writer, and it
re-derives the normals. Three cases in `CameraTest.cxx`; ADR-0056 and Architecture.md amended.

Route every write of `Profile::rotation_` through one private setter that clears `basisValid_`,
and remove `Camera`'s friend access to the raw members. `rotate()` loses its
all-components-non-zero guard; an identity check, or none, replaces it. `pan()` and `tilt()` keep
the axes in step with the quaternion. ADR-0056's precision argument stands; its enforcement by
convention is what is replaced, and the ADR's consequences say so.

A test that loads a profile through `lookat()`, rotates it, and asserts `createView()` moved —
the case that would have caught this. Then run the editor and orbit the perspective view.

#### Step 2 — The image reader and writer keep their contracts

D4, D5. In [`api/image/`](../../../api/image/).

**Closed.** The jpeg error manager is one private header,
[`JpegError.h`](../../../api/image/JpegError.h), that the reader and the writer share; the writer
drops alpha a row at a time and removes a fragment it could not finish. `.jpeg` registers beside
`.jpg`. Cases in `ImageReaderTest.cxx` and `ImageWriterTest.cxx`, the 32 bit fixture among them.

`reader::Bmp` returns null where it throws. `writer::Jpeg` installs the same longjmp error manager
`reader::Jpeg` has, and converts RGBA to RGB explicitly (or refuses it with a log line — the step
decides, and the writer's comment says which). `Factory` keys formats on
`boost::filesystem::path::extension()`, lower-cased, so `.jpeg` resolves and a short name does not
throw. The `catch (const std::string&)` blocks in `imagetool` and `BitmapFont` go (the latter with
step 7).

Tests: a truncated bmp, an RGBA image written as jpeg, `photo.JPEG`, and a two-character name.

#### Step 3 — `splitEdge` splits the edge it is given, and a copied node is a new node

D8, D11. In [`api/brep/`](../../../api/brep/) and [`api/dag/`](../../../api/dag/).

**Closed.** All three edit operations deleted with the test that pinned the wrong result, since
nothing schedules a modelling operation. `dag::Node` cannot be copied, and nothing in the tree did;
`BRepTest` asserts it at compile time. ADR-0013 corrected.

Delete the `extrudeFace` and `splitFace` declarations — nothing defines or calls them. Fix
`splitEdge` to split both halves and keep pairs mutual, or delete it with the other two; it has
no caller either, and the step chooses on whether the editor's first modelling operation is near.
If kept, its test is rewritten against a two-face mesh and checks `pair`.

`dag::Node` either deletes its copy operations or gives a copy a fresh id; the step picks by
whether anything copies a `BRep` today. Correct ADR-0013's consequence line. Initialise `Face`'s
members (B1's one-line half).

#### Step 4 — The editor's `main` is `run<>`

**Closed.** Run and closed through its window, exit 0, log silent.

D9. [`vertical3d/src/main.cxx`](../../../vertical3d/src/main.cxx) becomes the one-line `main`
ADR-0028 already requires. `Controller::initialize()` already takes no arguments. Run the editor.

#### Step 5 — A listener holds its connection

D10, A6. Across `api/event`, `api/audio` and the five apps.

**Closed.** Ten production connections are scoped, each declared so it goes before the dispatcher
it points into. The test fixtures still connect locals that die with their own dispatcher, and are
left. All five apps run and close with a silent log.

Every `sink<…>().connect<…>(*this)` becomes an `entt::scoped_connection` member. `event::Engine`
becomes non-copyable. `audio::Engine`'s destructor calls `shutdown()`. After this step a
`disconnect` or a scoped connection exists for every connect in the tree, which a grep confirms.

### Phase 2 — Subtraction

Pure deletion, plus the one change a deletion makes safe. Nothing that draws or plays changes.
`git show` recovers anything; the review records what each piece was.

#### Step 6 — brep and dag lose what nothing uses

B3, B4, and B3's consequence for `dag::Transform`.

**Closed.** `api/dag` is `Node` and `Transform`, and `Transform` is not virtual. Editor.md and the
clang-tidy note follow; Conventions.md's example of a small directory that wants nothing done to it
is `api/grid` now.

Delete `WingedEdgeBRep`, `Edge` and their tests (938 lines), and the seven empty `dag` classes.
With the only override gone, `dag::Transform`'s members become non-virtual, which closes the door
ADR-0013 made `translation(v)` a setter to close. Whether `Node` and `Transform` then stay a
library of their own or move into `api/brep` is an open question below; this step does not move
them.

#### Step 7 — font, image, asset and type lose the OpenGL-era stack and the wrong helpers

T3, D12, the dead half of T6, and the asset half of A8.

**Closed.** `TextBuffer` stays rather than folding into `TextureTextBuffer`: `Canvas::text` takes
it, and it is the seam that lets render draw text and `CanvasTest` build some without FreeType.
`JsonFile` is `v3d::asset::JsonFile`. The ui's five `intersect` calls are `contains`.

Delete `font::Font2D`, `BitmapFont`, `BitmapTextBuffer`, `image::Texture`, `asset::kind::Font2D`,
`asset::loader::Font2D`, `asset::Type::Font2D` and `asset::Cache`. Remove `Loader::reset`,
`Loader::manager_` and `Manager::load(…, hasPath)`. Fold `TextBuffer` into `TextureTextBuffer` if
nothing else derives from it. Move `kind::JsonFile` out of `asset::kind`. Drop `font → type`
from the link line (A3's one deletion).

In `api/type`, delete `AABBox::origin()`, `Bound2D::expand`, `shrink` and `operator+=` with the
tests that assert their wrong numbers — none has a production caller — and `Plane::intersect`,
which duplicates `Ray::intersects`. The ui's five `Bound2D::intersect` calls become `contains`,
and `intersect` goes.

#### Step 8 — engine, event, config and render lose what nothing calls

The dead items of E9 and R7, and U7's unused include.

**Closed.** `Frame` held its context and nothing read it, so both went and `realtime::Context` with
them; `Engine3D::context()` is a `DeviceContext`. The unread registry went from the render engine's
constructor and from the four app renderers that only passed it on. **Moved to step 24:** the
unread logger on Quad, Line and World, since that step rewrites their constructors.

`event::Engine::dispatch`, `Event::operator()`, `api/config/Sounds.h`, `realtime::Context` (so
`Engine3D::context()` returns a `DeviceContext` and voxel's downcast goes), `realtime::Engine`'s
unread `registry_` (and either fold the base into `Engine3D` or give it a virtual destructor),
`Component.h`'s include of `style/Theme.h`, and the stale comments the review names
(`Accumulator.h:62`, `SortKey.h:21`).

### Phase 3 — The dependency graph

#### Step 9 — Link visibility matches the headers, and something checks it

A3. In the `CMakeLists.txt` of `render`, `ui`, `engine`, and in
[`cmake/`](../../../cmake/).

**Closed.** `v3d_api_verify_visibility` fails the configure in both directions, and was shown to by
breaking render's link to image on purpose. Build.md says so under the linking rules.

`render → image` and `ui → image` become PUBLIC, `engine → SDL3` PUBLIC, `render → ecs` PRIVATE.
Each CMake comment is corrected to what the headers do. Then add a configure-time check beside
`v3d_api_verify_manifest`: a header under `api/X` that includes `<api/Y/…>` implies a PUBLIC link
from X to Y. This is what lets step 11 narrow asset without discovering, one compile error at a
time, what was riding on it.

#### Step 10 — A parser takes a document, and audio stops depending on asset

A1, A7.

**Closed.** The cycle is gone and ADR-0021 says it was restored. Each test that built a
`kind::Json` only to have it unwrapped passes the object instead. Two sources called
`boost::json::value_to` with only `object.hpp` in reach, and linked only because another object in
the same library instantiated it; they include the whole of Boost.JSON now.

`audio::Engine::load`, `config::CameraProfiles`, `config::SpriteSheets` and `ui::Engine` take
`const boost::json::object&`; callers pass `json->document()`. `audio → asset` leaves the link
line, the cycle is gone, and the header comment and ADR-0021 are true again. A test of audio's
`load` no longer needs an asset manager.

#### Step 11 — ADR-0079: the registry is open and media loaders live with their payloads

A2, T1, T2, and B5's header half.

**Closed, except B5's header half.** ADR-0079 chose an asset core, `v3dlib_asset_media` for
pictures, models and typefaces, and the sound loader in `api/audio`: `render` loads models and `ui`
loads typefaces, so one media library holding sound too would have put the mixer back in their
closures. The starter's closure has no SDL3_mixer now and `config`'s needs only glm and spdlog,
which the manifest's closure says. `.bmp` and `.jpeg` load through the one image loader. **Moved to
step 15:** the pimpl that would make SDL3_mixer PRIVATE to audio. Only an app that plays sound
links audio now, so the mixer in its headers costs nobody else, and step 15 reshapes that engine
anyway.

Write ADR-0079 first. Then `Manager` gains `registerLoader(Type, loader, extensions)`, and
`loadTypeFromExt` becomes a lookup of what was registered — so the three extension maps that
disagree become one. The Png, Jpeg and Tga loaders collapse into one `loader::Image` over
`image::Factory`. The Wav loader moves to `api/audio` and the `TextureFont` loader to `api/font`
(or a thin `asset_media` library — the ADR decides), each registering itself. `audio::Engine` and
`AudioClip` move their `MIX_*` members behind a pimpl so SDL3_mixer can become PRIVATE.

Verify against the narrow-closure starter: its closure loses SDL3_mixer and Freetype, and
`config`'s loses everything but Boost.JSON.

#### Step 12 — One loader contract, typed loads and typed options

A4, A5, A8, the smaller asset defects, and ADR-0030's cache.

**Closed.** Every way into the manager answers a failure with null and a log line, and `load<T>()`
does the cast once and logs a file of the wrong kind; the eleven production casts use it. `Asset`
says its name and type. **The options went rather than becoming typed:** the typeface loader was
their only user, so a typeface stopped being an asset and `ui::TextRenderer` opens the face at the
path `Manager::path()` resolves - ADR-0079 says so. `JsonFile` went too: `asset::readFile()` reads
a whole file for the Json and Text loaders and the editor's project. The cache was decided against:
`MeshRegistry` already keeps what it uploaded by name, so ADR-0030's Pro is struck rather than made
true.

`Loader::load` returns null and logs on every failure; Text, the font loader, and `Manager`'s
unknown type and extension stop throwing, and `config/Config.cpp`'s comment about the throw goes.
`TextureFont`'s constructor initialises every member before it can return early. `JsonFile::read`
takes a mutable `error_code&`, and `loader/Json.cpp` stops on `ferror`. One `readFile(path)`
returning an optional buffer serves every loader. `Manager` gains
`template<class T> load<T>(name, options)` with options as a per-type struct, and the twelve
production `dynamic_pointer_cast` sites use it; loaders hold no per-call state.

Then decide the cache: a weak-reference map keyed by path, or strike ADR-0030's Pro. `MeshRegistry`
already de-duplicates, which argues for striking it.

### Phase 4 — Engine

#### Step 13 — Bindings are a class in `api/event`

E3, and the key-name half of E9.

**Closed.** `event::Bindings` reads the document, keeps the overrides and answers `sources()`;
`Engine` holds one and asks it for `held()`. It cannot name a key itself, since `input` is above
`event`, so the engine hands it a check over `input::isKeyName()` and `isButtonName()`, and every
app's bindings passed it. `event` links `log` now. The window config is read with guards, and
Architecture.md stops listing it as a trap.

Move binding-document parsing, the rebind overlay, source lookup and `held()` into
`event::Bindings`, which needs no SDL and is testable without an engine. `Engine` holds one and
forwards. `rebind()` returns false when nothing was rebuilt. `input::keyName` becomes a constexpr
table read both ways, and `Bindings` logs every source name it does not recognise. The window
config is read with the same guards as the bindings.

#### Step 14 — ADR-0080: the engine owns its lifecycle

E1, and the typed-flags half of E9.

**Closed.** `initialize()` takes nothing and calls the app's `start()`; `shutdown()` is private to
`run<T>` and calls the app's `release()` before the window goes; `features()` defaults to all four,
and only the starter asks for fewer. `Feature` and `DeviceType` are `type::Flags`. `EngineTest`
asserts through a concept that `shutdown()` is out of reach. CLAUDE.md's rule says what the type
enforces, and all five apps run and shut down once.

Write ADR-0080 first; the review's E1 sets out the options. The likely shape: `shutdown()` is
private to `run<T>` (or refuses while the loop runs) and calls a protected virtual `release()`
before tearing the window down; start-up is `features()` plus `start()`, with the base owning
`initialize()`; `initialize` takes `Feature`, not `int`, and `Feature`/`DeviceType` share one
flags template. Every app moves in the same commit — pong, tetris, voxel, odyssey, the editor and
`examples/starter` — and the five "quit(), not shutdown()" comments go because the mistake can no
longer be written. CLAUDE.md's rule about `shutdown()` is updated to say what the type now
enforces.

#### Step 15 — The shell finishes what ADR-0028 started

E2, B5's wiring half, and the cross-library F6.

**Closed, two parts differently.** The engine answers `ui::quit`; `GameMenu` answers
`ui::showGameMenu` and the navigation commands off the ui's dispatcher, and tetris's escape key
sends the same command as the others; `Engine::document()` is the one lookup of a config document,
so the ui and sound blocks each app copied are a line. SDL3_mixer is PRIVATE to audio, through
forward declared handles rather than a pimpl. **Audio did not join the engine behind a `Feature`
bit**: the engine would link audio and the mixer would be back in every closure ADR-0079 took it
out of. `audio::Engine::load(config, assets)` resolves clips through the manager instead, so pong's
lambda went all the same. **The statistics sample builder was not written**: ui and engine may not
depend on each other, so it has nowhere to live, and what it would replace is one aggregate in each
app.

`ui::shell::GameMenu` builds from the ui config and subscribes to its own navigate and toggle
commands; the engine answers a reserved quit command as it answers a window close; a
`StatisticsOverlay::Sample` builder lives on the ui side. Audio joins the shell behind a `Feature`
bit with the sound resolver built in, so pong's lambda goes, and `audio::Engine` and `AudioClip`
move their `MIX_*` members behind a pimpl so SDL3_mixer can become PRIVATE. Correct ECSDesign.md's
claim about who creates the audio engine. pong, tetris and voxel each lose their copy; the test is
that the three handlers that are left are what makes each game that game.

#### Step 16 — Engine state is reached through accessors

**Closed.** Eight members are private behind const accessors, and `registry_` stays protected as
Architecture.md decides. The one write an app made into the loop's own state, voxel timing its
remeshing, is `measure()`. tetris no longer builds a logger the engine then replaced.

E5. `registry_` stays protected, as Architecture.md decides; the other eight become const
accessors, non-null where a `Feature` guarantees it, and `accumulator_` private. tetris stops
assigning `logger_` in its constructor.

#### Step 17 — ADR-0081: a key and a command are different events

E4.

**Closed.** A key is an `event::Source`, sent only through `event::publish()`, which hands it to
the event engine for mapping once every listener has heard it and none consumed it. The first shape
queued the commands on the dispatcher, and the test written for pong's capture showed why that
fails: a dispatcher calls listeners last-connected first. ADR-0081 records both. No listener
filters by `Type` now, and ADR-0017 is amended.

Write ADR-0081, amending ADR-0017. Raw input and mapped commands become distinct types on
distinct sinks, so a listener cannot take both by accident, and the mapper delivers a command
after its source has been fully published rather than inside the publish. The editor's
`type() != Destination` guard and pong's capture path are rewritten against the two types;
ADR-0058's ordering note is revisited.

#### Step 18 — Decide whether the loop needs Vulkan

**Closed, decided against.** Nothing headless consumes the loop, in this tree or the two that adopt
it, so the split would cost a library or an interface for no consumer. ADR-0033's consequences say
so, and where to start if one appears.

E6. Either the engine holds its window through a narrow interface (or is handed one), so
`Window.h` leaves `Engine.h`; or the pure pieces — `Accumulator`, `Statistics`, `Settings`, the path
helpers — move to a library that does not link render. Or neither, recorded in ADR-0033's
consequences, if nothing headless is near. **This step may close as decided against**; it earns an
ADR only if it changes the graph.

#### Step 19 — The logger has a home, and config types are open

E7, E8.

**Closed.** `log::Logger::open()` says where the one log goes and `run<T>` calls it with the app
path, so the log lands beside the executable when an app is started from elsewhere, which was
checked by doing it; a `Logger` says in its header that it is a handle on a global, and `get()`
returns by value. Config files every document under the name its entry gives: a type the api does
not read is the app's own rather than a reason to refuse startup, so there is nothing to skip, and
odyssey's board loads through config.

The log sink is built once against `appPath()` (or `userPath()`), by `run<T>`, so `v3d.log` lands
where NewProject.md and the CI workflow say it does. Then either inject it for real or make it one
honest global accessor; `get()` returns by value. Config types are keyed by string, an unknown
one logs and is skipped, and odyssey's map loads through config like any other document.

### Phase 5 — ui

#### Step 20 — One traversal, and the predicates are a switch

U1, U2, and D2 and D3.

**Closed.** `ui::forEachDrawn()` is the one rule for which children are live and in what order, and
the draw walk, the pick and the tab order go through it; both new cases fail against the code
before it. `resolveComponentImages` does not: a page that is not up still needs its images for when
it is. `component::traits()` answers strip, flow, pages and text, and `Cursor::follow()` is an
exhaustive switch; ADR-0047 is corrected.

One function answers "the live children of X in draw order" — TabBar to its page, a Box in held
order, everything else by depth — and `Arranger::walk`, `Container::probe` (reversed),
`focusable` and `resolveComponentImages` all use it. Then the twenty unchecked `type() ==` tests
and `dynamic_cast` probes become one exhaustive `traits(Type)` switch, so C4062 covers them, and
ADR-0047 is corrected. Tests: a tab control on a hidden page is not focusable; a nested child
with out-of-order depth is picked where it is drawn.

#### Step 21 — Strip buttons and menu items honour the base properties

U3, U5.

**Closed, the lighter way.** A strip's buttons and a menu's items stay outside `children()` and are
adopted by their holder, so `usable()` walks up through it: a disabled toolbar disables its
buttons, a disabled item is neither lit nor sent, and the panel draws it disabled. A strip button
is read through `loadAttributes` like any other. Every command the ui sends goes through
`ui::input::send()`, the one place the unbound-event rule is written; ADR-0059's table has the new
rows. **Not done:** `visible` on a menu item, which wants the panel's rows to close up rather than
a check, and taking the dispatcher off `Toolbar` and `Menu`, which changes how a menu bar answers a
press for no defect.

Either strip buttons and menu items become real children loaded through the attribute path, or
`usable()` and the loader treat their owner as the parent; the step picks the one that leaves
fewer special cases in the walks. Either way: a disabled toolbar draws disabled, `enabled`/
`style`/`name`/`visible` load on a toolbar button, and a disabled menu item does not dispatch.
Commands go out through one `send(dispatcher, event)` and `Toolbar`/`Menu` stop holding a
dispatcher, which puts dispatch back where ADR-0038 says it is. ADR-0059's table gains the rows.

#### Step 22 — Paint stops doing layout, and the menu has one keyboard model

U4, U6, U7.

**Closed.** The arranger places a toolbar and its buttons, a menu bar's panels and a game menu's
level with their items, and the renderer paints the boxes it was given; `Arranger::place()` is the
one `place()` and `drawn()` the one size fallback. **U6 was decided as two models**, and
UserInterface.md says why: a game's menu is driven by commands its bindings send, which is what
lets it be rebound or driven by a gamepad. U7: `Dressing` has a caret, a placeholder and a tab
colour of its own, so an unthemed placeholder is readable; the thumb maths is three functions the
scrollbar and `Immediate` share, and `Immediate` closes a scrolled region in one place; a flow
box's child room and size are one helper each; `Component` cannot be copied; the loader tests the
tab bar's type.

Strip and menu-panel placement move into `Arranger`, so layout without a canvas places
everything; `ComponentRenderer` reads boxes only, the seven size fallbacks call `natural()`, and
`place()` exists once. `draw(Menu)` reuses `panel()` and plates through the theme. Then decide
U6: the game menu becomes focusable components under ADR-0040, or a note in UserInterface.md says
why it stays a separate model — an ADR only in the first case. The rest of U7: `Dressing` gains
named fields for its overloaded ones, `Immediate` gets one `ScrollRegion` and shares thumb maths
with `Scrollbar`, `arrange`/`wrapped` share child sizing, `Component` deletes its copy, and the
loader tests `Type::TabBar` rather than `"tabs"`.

### Phase 6 — render/realtime

#### Step 23 — ADR-0082: textures and materials belong to the context

R2.

**Closed.** `realtime::Textures` is built with the `DeviceContext`, owns set 1's pool and layout,
the white texture and the material map, and copies through the context's one uploader; `Quad`,
`World`, `Lit` and `MeshRegistry` take it, and every app and test asks `textures()`. A context with
no colour format registers a texture and a textured mesh and never builds the quad renderer, which
the device suite asserts. ADR-0042 and ADR-0065 are amended.

Write ADR-0082. A `Textures` service on `DeviceContext` owns the factory, set 1's pool, the white
texture and the texture-to-material map, using the context's uploader. Quad, World, Lit and
`MeshRegistry` depend on it, not on Quad. `RenderTarget::ready()` stops making an uploader per
call. Test: a default-constructed headless context registers a texture and a mesh.

#### Step 24 — A geometry ring resets itself, and growing one retires

R1, R5, and D6.

**Closed.** `vulkan::frame::StreamRing` holds a set of buffers per frame in flight and starts its
claims again the first time it is claimed from after `Ring::begun()` moves, so `Engine3D::endFrame()`
only resets the frame and no test calls a renderer's `endFrame()`. A buffer the content outgrows is
replaced and retired through the ring. Quad, Line and World hold one and lose the logger; the clip
to a scissor is `renderer::clip()`. `ReleaseTest` claims through a stream for many frames and holds
a constant count, and does again after growing. The draw-item helper did not come out: what the
three fill in differs in everything but the clip, so a helper would be a constructor with a flag
per renderer. `Buffer::grow` keeps its device wait, since its one caller left is `Capture`'s
readback, which waits for the device anyway.

Extract a `StreamRing` keyed by the in-flight ring's frame, which resets when its slot is reused;
Quad, Line and World hold one, and `Engine3D::endFrame()`'s list goes, along with the manual
`world.endFrame()` in `LitSceneTest`. `Buffer::grow` retires the old allocation through the ring
rather than waiting for idle. The shared scissor and draw-item helpers come out in the same
commit, and so does the logger the three renderers are handed and never read. Test: a `World` the app built, drawn for many frames, holds a constant buffer count.

#### Step 25 — Layouts, barriers, sort keys and the rest

R3, R4, R6, R7.

**Closed.** `pipeline::Texture::descriptor()` names the layout a texture is sampled in, and the
three image descriptor writes use it; a target's depth drawn on a canvas, which the validation
layer reported, is clean, and the device suite has the case. `vulkan/memory/Barriers.h` names the
nine transitions and records them; the recorder, `RenderTarget::ready`, `TextureFactory` and
`Capture` use it, and `RenderTarget` now makes the same transitions the recorder does rather than
a copy of them. Grade's table is `TextureFactory::volume`. `Pass::submit` sets the key's pipeline
and material, saturating a slot past sixteen bits, so a lit pass and `FullScreen` sort by them
now. `Frame::swapchainDepth()` is the one depth decision, and `Engine3D` no longer allocates the
window depth for a frame whose only depth test is offscreen. The canvases hold a `TransformStack`
and a `ClipStack`. `device::check` and `device::failure` replace 47 throw blocks. R7's vestigial
bases went in step 8. The golden images pass unchanged.

A texture answers its own `VkDescriptorImageInfo`, layout included, and the six descriptor writes
use it — which fixes a depth texture named on a canvas. One internal `Barriers.h` of named
transitions serves the recorder, `RenderTarget`, `TextureFactory`, `Grade` and `Capture`; Grade's
3D upload becomes a `TextureFactory` overload. `Pass::submit` derives `key.pipeline` and
`key.material`. The window-depth decision is one helper both `Engine3D` and `Recorder` call. The
canvases share a `ClipStack` and a transform stack. `device::check(result, what)` replaces the 45
throw blocks.

Run the device suite and the golden images; this is the step most likely to move a picture.

### Phase 7 — render/offline

#### Step 26 — Shading globals are bound in one place

O1, and D7.

**Closed.** `sl::Globals` resolves a program's globals once, and a machine cache holds it beside
the machine; `sl::Point` is what a hider fills per shading point, and `Globals::shine` runs a light
over a batch. `GridShader`, `HitShader` and `Imager` use them, so both hiders now bind `Oi` and
`du`/`dv` the same way. **D7 is documented rather than fixed:** honouring `Oi` under reyes needs a
sample to keep a list of surfaces and composite them, which is a feature rather than debt, so
OfflineRenderers.md says the reyes hider's samples are opaque and TODO.md holds the work. The
reference renders under both hiders pass unchanged.

A globals map in `api/render/offline/sl`, resolved once per program, plus one "run a light over
a batch" routine; `HitShader`, `GridShader` and `Imager` use them and supply only per-point data.
Then decide D7: the reyes hider honours `Oi`, or OfflineRenderers.md says it does not and
`TODO.md` tracks it. The first means `Bucket` composites by coverage rather than forcing it to 1.
Render the suites under both hiders before and after.

#### Step 27 — The tracer splits from the shader, and the scene takes any primitive

O2, O3.

**Closed.** `trace::Tracer` owns `see`, `transmitted`, `traced`, the depth and the machines, and
works out the motion poses once per `time()` rather than per ray. `HitShader` is a stack object
made per hit, so a traced ray's hit is shaded by another and nothing is saved or restored.
`Primitive` has a virtual `intersect` and `describe`; the scene holds one list, and `all<Kind>()`
is the typed view the tests read. A primitive the library has never seen is met and described,
which `TraceSceneTest` shows with a plane defined in the test. The reference renders under both
hiders pass unchanged.

A `Tracer` owns `see`, `transmitted`, `traced` and the depth; the per-hit `Renderer` adapter is a
stack object, so there is nothing to save and restore. `trace::Primitive` gains a virtual
`intersect` and `describe` (or a variant), the scene holds one list, and motion poses are computed
per sample time, not per ray. This is the shape the acceleration structure in TODO.md
needs, and it does not build that structure.

#### Step 28 — Built-ins, the compiler, and the reader

O4, O5, O6.

**Closed.** A `Signature` names its `Body`, so a function cannot be declared without one, and
`calculatenormal` - which had none and fell to the default - is a declared stub; a test holds the
source-written and stubbed sets to what the table says. `syntax::forEachChild` is the one list of
a node's children, and the varying inference is `sl::Inference`. `Machine::prepare` holds its
program, and `run()` and `initialise()` take none. The RIB reader forwards `AreaLightSource`,
`MakeTexture` and a motion block's later poses to `Handler` methods whose defaults are today's
fallbacks, and reads the name-and-parameters requests and its request groups through tables. The
lexers share `offline::Characters` and `offline::Lexeme`. `moya::Hider` is the interface, and
`RayHider` solves the camera to raster transformation for x and y at a depth rather than
rebuilding it from the field of view - a full inverse is ruined by RI's default clipping range.
`v3d_add_shader` takes `OUTPUT` and `DEFINES`, and the three skinned shaders are their rigid ones
compiled with `SKINNED`. OfflineRenderers.md's build paragraph names `api/CMakeLists.txt`.

A built-in's body is stored in its `Signature`, so registering one is one row and an unregistered
body is a build-time gap rather than a silent default. `forEachChild` on the syntax nodes, and
storage inference moves to its own class over the annotated tree. `Machine` binds its program at
`prepare`. The RIB reader forwards `AreaLightSource`, extra motion primitives and `MakeTexture` to
`Handler` methods whose default bodies do today's fallback; its request groups go through a
table. The two lexers share a header of character classes, position and escape decoding. The
hider becomes a small interface and `RayHider` inverts the camera-to-raster matrix.
`v3d_add_shader` takes a define and an output name, and the skinned shaders stop being copies.
Fix OfflineRenderers.md:45.

### Phase 8 — Geometry, ecs and the type leftovers

#### Step 29 — brep keeps its own invariants

B1, B2.

**Closed.** `BRep::validate()` owns the reference rule and `Project` reports what it names.
`faceLoop`, `loopSegment`, `ownsEdge`, `edgeSelected`, `center` and `faceUV` are const, `Index`
based functions in `api/brep/Topology.h`, so the editor's `MeshTopology` and its second copy of
"two halves are one edge" go, and so do the iterators, which nothing outside the tests used.
`findPair` is bounded, so a ring that does not close ends it. `Face()` already initialised its
edge. The editor's index casts are gone.

`BRep::validate()`, or a builder that checks on finish, and `Project` calls it instead of owning
the rule. Face-loop, segment and edge-identity queries move into `api/brep` as const,
index-based functions; the iterators are deleted or rebuilt on them; `Index` is used throughout,
which removes the editor's casts.

#### Step 30 — One transform, one tile coordinate

B6.

**Closed.** `type::Transform` is the TRS value, its matrix and its interpolation; the ecs
`Transform` component is it, and `dag::Transform` holds one, so `dag` requires `type`. The
editor's `Placement` stays its own: it is an undo record compared with a tolerance, not a
composition. odyssey stores a `grid::TileCoord` and `PositionFixed2D` is gone; `Position1D`,
`Position2D` and `Color3` are aggregates with a `value`, so all three copy. `grid` floods once,
for both `reachableTiles` and `DistanceField`, and `TileGrid::index` is public and is what the
walk indexes by.

`dag::Transform` holds the ecs TRS value rather than restating it. odyssey stores a
`grid::TileCoord` and `PositionFixed2D` goes. The class-style ecs components become aggregates,
or move into the one app each serves. `grid` gets one `flood` and exposes its index.

#### Step 31 — TextureFont, Camera and the type leftovers

T4, the rest of T5, T6.

**Closed.** Kerning is deleted rather than applied: it was built O(n²) on every glyph load and
never read, and applying it would move every line of ui text. `Markup` is a struct with defaults
holding only what `addText` honours, and a strikethrough is drawn in its own colour rather than
the overline's. `TextureFont::Freetype` releases what it opened on every path and when it goes;
`TextureFontCache` is given a depth, `1`, rather than the filtering enum, and its charcode buffer
went because nothing read it. A full atlas that left no room for the line glyph skips the lines
and still draws the glyph. `Camera` starts at identity and its three queries are const.
`Plane::distance(point)` is `signedDistance`. `Image` has no length constructor and no dimension
setters; the writers and the BMP reader keep their scratch in a byte vector or read the encoded
bytes in place, and `image::swapRedBlue` is the one swap - which carries the alpha a 32 bit TGA
used to lose. The outline path in `TextureFont` is still there and still unreachable.

Apply kerning in `addCharacter` or delete it; trim `Markup` to what is honoured; make FreeType an
RAII owner; pass the atlas depth as a depth; null-check the `black` glyph; give
`TextureFontCache` a deleted copy or no raw buffer. `Camera`'s matrices start at identity and its
queries are const. `Plane::distance(point)` is renamed, `Image`'s dimensions are immutable,
writers use a byte vector for scratch, and the RGB↔BGR swap is one function.

### Phase 9

#### Step 32 — The documents that own what moved

**Closed.** Each document was updated as its step landed, which the closing notes above name.
What was left at the end was ADR-0060 and ADR-0063, which still described `PositionFixed2D` as a
component and are amended in place. The review and this plan moved to `completed/`.

Architecture.md (the engine lifecycle, the shell, the event types), Build.md (the visibility
check, the new link lines), RenderingPipeline.md (textures, the ring), UserInterface.md,
OfflineRenderers.md, Editor.md, ECSDesign.md and NewProject.md, each as its step lands rather than
all at the end where that is practical. Then [the review](../../audits/completed/ApiDesignReview.md) moves to
`audits/completed/` and this plan to `plans/completed/`.

## Verification

Per [sdlc.md](../../sdlc.md) §4, for every step:

- **Build.** `scripts\build.cmd`. Steps 9 to 15 reach every app.
- **Tests.** `scripts\test.cmd`. Every defect in phase 1 and every bug a refactor fixes (D2, D3,
  D6, D7) arrives with a case that fails before the change.
- **Lint and analysis.** cpplint, `/W4 /WX`, and the `/analyze` and clang-tidy trees. Steps 14,
  20 and 25 are the large mechanical ones to watch.
- **Run.** The editor for steps 1, 3, 4, 14 and 20–22; pong, tetris and voxel for 14, 15 and 17,
  with the validation layer on and silent; moya under both hiders for 26–28.
- **Pictures.** The device suite and the golden images for 23–25. Those only run across drivers in
  CI, so phase 6 is verified on a PR.

## What this plan does not do

**It does not build anything the review found missing rather than wrong.** The acceleration
structure, area lights, instancing and asynchronous loading stay where TODO.md has them; steps
24, 27 and 28 give the first two a better place to land and stop there.

**It does not split `api/render` or `api/type`.** The review found their breadth ADR-decided and
saw no reason to reopen it.

**It does not make simulation in `tick()` impossible.** ADR-0032's reasoning holds: there is no
type-level fix, and step 14 does not pretend to one.

**It does not unify the two ui paradigms** — ADR-0035's Alternative 4 still stands — **nor turn
the ui loader into a registry**, which UiConsolidation declined for reasons step 20 does not
disturb.

## Open questions

Both are answered.

- **`dag` stays a library.** Step 30 gave `dag::Transform` the TRS value from `api/type`, so what
  is left in `dag` is the node identity and the placement a mesh is selected and moved by, which
  brep and the editor both name. Folding it into brep would make the editor's selection model a
  brep concern.
- **The reyes hider does not honour `Oi`.** Step 26 documented the difference between the hiders,
  and TODO.md holds compositing by coverage in `Bucket`.
