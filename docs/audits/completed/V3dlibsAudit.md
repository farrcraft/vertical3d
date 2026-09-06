# v3dlibs Audit

**Closed 2026-09-04. `v3dlibs/` is deleted.** All five items are worked off. What follows is
the record of what the tree held and where each piece went, kept because it is the only
account of that. Recover the sources from `git show 68821c4^:v3dlibs/...` if a question about
the original ever comes up.

Phase 1 of [plans/completed/Modernization.md](../../plans/completed/Modernization.md). Audited against the tree on
2026-08-31. Companion to [LuxaAudit.md](LuxaAudit.md), which closed the same day.

`v3dlibs/` is a migration in progress. This is the functional equivalence record that has to
close before the tree can be deleted: every library still in it, where it landed in `api/`
(or did not), and what has to be built before nothing is lost.

**Verdict up front: `v3dlibs/` cannot be deleted yet, but it is much closer than `luxa/`.**
Most of the tree is genuinely covered. Two things blocked deletion — `vertical3d/` still
includes six of its headers, and the test corpus had to move. The corpus moved on
2026-08-31, so `vertical3d/` is the only one left. Everything else is either covered,
deliberately replaced, or dead.

The audit also found that the *replacement* for the command layer was less finished than the
plan assumed, and that mouse input did not work at all. Those were `api/` bugs, not reasons
to keep `v3dlibs/`, and all of them are fixed as of 2026-08-31 — see the list at the end.

## Method

`v3dlibs/` is 3,993 lines across 46 source files, of which 1,016 lines are the test corpus.
All of it was read. Where a library was *replaced* rather than ported — the command layer —
the old interface was compared feature by feature against the new one, because a replacement
that drops a capability silently is the failure mode this audit exists to catch.

Files deleted from `v3dlibs/` in earlier commits were recovered with `git show` to establish
what has already moved. The relevant commits are `2972cd3`, `79952eb`, `911b727`, `1b3f760`,
`6f2ad72`, `81ca4cc`, `9ebe0ff`, `457ad92`, `6d86709` and `6cfb4b6`.

## What is left in the tree

```
v3dlibs/command/    Bind, BindLoader, Command, CommandDirectory, CommandInfo,
                    CommandTable, Event, StateController
v3dlibs/core/       Scene, SceneVisitor, CreatePolyCommandSet
v3dlibs/gui/        InputEventAdapter
v3dlibs/hookah/     Hookah, Window, drivers/sdl2/
v3dlibs/input/      InputDevice, KeyboardDevice, MouseDevice, and their listener interfaces
v3dlibs/component/  Component.h
v3dlibs/tests/      23 Boost.Test sources plus 9 image fixtures (moved out 2026-08-31)
```

Already migrated out, and out of scope here: `type`, `brep`, `image`, `font`, `gl`, `dag`,
`asset`, `audio`, `event`, `util`, `imagetool`, and the duplicate `vertical3d/` copy.

Like `luxa/`, the tree no longer compiles. `command/` and `input/` both include
`../event/EventInfo.h`, `../event/EventListener.h` and `../event/EventEmitter.h`, and
`v3dlibs/event/` was deleted in `6d86709`. `component/Component.h` includes
`../3dtypes/Vector3.h` from a directory that has not existed since `1b3f760`. Several tests
include `../3dtypes/` and `../image/ImageFactory.h`, both long renamed.

Two build-file problems worth knowing before anyone re-adds the tree:

- **`v3dlib_input` is declared twice** — once in `api/input/CMakeLists.txt` and once in
  `v3dlibs/input/CMakeLists.txt`. Harmless only because `v3dlibs/` is not in the build.
  `pong` links `v3dlib_input` and gets the `api` one.
- **`v3dlib_core` cannot build.** Its `CMakeLists.txt` still lists `Logger.h`/`Logger.cxx`,
  deleted in `81ca4cc`. Nothing links it any more - tetris did until 2026-08-31.

## Library by library

| v3dlibs | api equivalent | Verdict |
|---|---|---|
| `input/` | `api/input` | **Covered**, since `api/input/Mouse` and `MouseState` landed 2026-08-31 |
| `gui/InputEventAdapter` | `api/input` + `api/event` | **Covered.** The adapter's whole job was bridging two libraries that are now one path |
| `hookah/Window` | `render::realtime::Window` | **Partial.** Surface operations covered; four main-loop features are not |
| `hookah/Hookah.h` | — | Obsolete. A driver-abstraction factory for a driver choice that no longer exists |
| `hookah/drivers/sdl2/` | — | Dead. The last SDL2 code in the repo; delete with the tree |
| `command/` | `api/event` (+ a dead `api/config` class) | **Replaced, not ported.** Five capabilities have no successor |
| `core/Scene`, `SceneVisitor` | not `api/dag` | **Belongs to Phase 6.** See below |
| `core/CreatePolyCommandSet` | — | **Belongs to Phase 6.** Editor code |
| `component/Component.h` | `api/ecs` (entt) | Dead. Never built, superseded by entt |
| `tests/` | per-library `tests/` | **Must move before deletion** |

## The command layer: what the replacement does not do

The plan records that `command/` was replaced rather than ported — binding to `api/config`
(`BindingContext`), mapping to `api/event` (`Mapper`), dispatch to entt. Half of that is
right. The audit question posed was "does anything still need `CommandDirectory`,
`CommandTable` or `StateController`", and the answer is no — but five capabilities went with
them, and one of the two named replacements is not wired up at all.

### ~~`api/config::BindingContext` is dead code~~ — settled 2026-08-31

`Config::loadBindings` was commented out in its entirety, still written against the removed
`LOG_ERROR` macros, so nothing ever constructed a `BindingContext` and `Config::contexts_`
was never populated or read. The live binding path was elsewhere:
`v3d::engine::Engine::registerEventMappings` reads `mappings.json` into a
`v3d::event::Mapper`.

`Mapper` won, on the grounds that it is the one that runs. `BindingContext.{h,cpp}`, the
`contexts_` member and the commented-out loader are deleted.

### ~~Press and release are indistinguishable~~ — fixed 2026-08-31

The old `Bind` carried an `EventInfo` with a `MatchState` — `MATCH_STATE_ON`, `OFF` or
`ANY` — so a binding could fire on key down, key up, or both. The new path had no
equivalent: `Event::operator<`, the comparator behind `Mapper`'s map, compared only
`context::name`, so both edges found the same mapping and the destination carried no state.

`v3d::event::State` (`Any`/`Pressed`/`Released`) is now a field on `Event`, part of its
ordering, and settable from a binding's optional `"state"` — so a binding can take one edge
or both, which is `MatchState` back. `Engine::handleSourceEvent` stamps the source's edge
onto the destination, so a handler can also take both edges from one binding and branch on
`event.state()`. Pong's paddles do that now instead of toggling.

While fixing it, two related defects turned up in the same machinery:

- **A source could only have one destination.** `Mapper` stored `std::map<Event, Event>`, so
  `mappings_[source] = destination` silently overwrote. Pong binds `arrow_up` to both
  `pong::rightPaddleUp` and `ui::menuPrevious`; the second replaced the first, so **the right
  paddle had never worked**. `Mapper` now holds a `std::multimap` and returns every match,
  which is what the old `CommandDirectory::notify` did.
- **`Keyboard::handleEvent` cleared key state backwards.** The key-up branch tested
  `if (!state_.pressed(key))` before toggling, so releasing a held key left it marked held
  and releasing an unheld key marked it held. `KeyState` had never tracked anything
  correctly.

### ~~The bind parameter is gone~~ — fixed 2026-08-31

`Bind` carried a param string handed to the command callback, which is how one command served
many bindings — `toggleMenu` with param `game-menu`. A binding's destination now takes an
optional `"param"` (string, int or bool) which becomes the event's `EventData`, the same
field a menu item's value arrives in — the joint fix the [luxa audit](LuxaAudit.md) asked
for. The edge moved out of `data` and into `state` to make room for it; `Key` and
`MouseButton` no longer write a bool there either.

### State scoping is a stub

`CommandTable` was a named set of commands with its own binds; `StateController` was a stack
of them, so pushing a "menu" state could re-bind the same keys without disturbing the
gameplay bindings.

`v3d::event::Context` has an `active` flag that is the seed of this, and **nothing in the
tree ever sets or reads it** — `Engine::handleSourceEvent` walks every mapper unconditionally.
Today apps do the scoping by hand: `PongEngine::handleEvent` branches on
`event.context()->name()` and then re-checks menu visibility before each ui command. That
works for one app with two contexts and will not survive the editor.

### Enable/disable and invoke-by-name

`Command` wrapped a `boost::signals2` slot and could be disconnected, disabled and
re-enabled. That still has no successor, and nothing needs one yet.

`CommandDirectory::exec(CommandInfo, param)` let code invoke a command directly by name,
which is how `Luxa::ComponentManager::execCommand` worked. **Added 2026-08-31** as
`event::Engine::dispatch(context, name)` and its parameterised overload. Note that
`Menu::activate()` — the reason this was flagged urgent — turned out not to need it: it
holds a resolved `Event` and triggers that directly, which is better than a string lookup.
The capability exists for callers that only have names.

### One small bug

`Event::str()` dereferences `context_` unconditionally, and the single-argument
`Event(const std::string&)` constructor leaves it null. Any `Event` built that way faults on
comparison or logging.

## hookah: the window is covered, the loop is not

`render::realtime::Window` covers the surface: create, destroy, resize, width/height,
caption, cursor, warpCursor, and the raw `SDL_Window*`. `addInputDevice`/`device` are
correctly gone — `input::Engine` owns devices now — and `Hookah::Create3DWindow` was a
driver-abstraction factory for FLTK/SFML/SDL alternatives that no longer exist.

What did not survive into `v3d::engine::Engine::eventLoop`:

- ~~**Frame delta.**~~ **Restored 2026-08-31.** `Engine::eventLoop` measures elapsed
  milliseconds with `SDL_GetTicks` and `Engine::tick(unsigned int delta)` passes it on, the
  way `Hookah::Window::tick(delta)` did. Note that `TetrisScene::tick` and voxel's
  `Scene::tick`/`Player::tick` already took a delta and had simply been starved of one.
  Supplying it does not by itself make anything frame-rate independent: pong still moves the
  ball a fixed amount per tick and ignores the delta. Adopting it is gameplay work, per app.
- **Focus gating.** The old loop ran `drawFrame` only `if (active())`. `Window::active()` has
  no successor: `SDL_EVENT_WINDOW_FOCUS_LOST` and `_GAINED` are empty cases in
  `Engine::eventLoop`.
- **Dirty-draw.** `DRAW_DIRTY` plus `invalidate()` let an application redraw only on change —
  the mode an editor wants. The new loop always draws.
- **Blocking event handling.** `EVENT_HANDLING_BLOCKING` vs `NONBLOCKING`. Also an editor
  concern rather than a game one.

None of these block deleting `v3dlibs/`; the code to copy is four lines of `SDL_GetTicks`
arithmetic. They are listed because the plan currently records hookah as simply "covered by
`api/render/realtime/Window`", and three of the four are things the Phase 6 editor will
need.

## input: covered (`api/input/Mouse` did nothing until 2026-08-31)

`KeyboardDevice` and its `KeyState` map cleanly onto `api/input/Keyboard` and
`api/input/KeyState`, which is a faithful port. The `KeyboardEventListener` /
`MouseEventListener` interfaces are correctly replaced by the entt dispatcher, and
`gui/InputEventAdapter` — whose entire job was gluing the input listeners to the command
library's `EventListener` — has nothing left to glue.

`MouseDevice` was a different story, and the problem was in `api`, not in `v3dlibs`:

~~**`api/input/Mouse::handleEvent` is empty.**~~ **Fixed 2026-08-31.** All three cases —
`SDL_EVENT_MOUSE_BUTTON_UP`, `_DOWN` and `_MOUSE_MOTION` — used to `break` without doing
anything and return `true`, which told `input::Engine::filterEvent` the event was handled, so
`Engine::eventLoop` did `continue` and **every mouse event in the application was swallowed**.

It now triggers `event::MouseButton` (which existed, fully implemented, and had never been
constructed), a new `event::MouseMotion`, and a bindable source event per button so
`mappings.json` can bind `{"context": "mouse", "name": "left"}` the way it binds keys.
`input::MouseState` is the successor to `MouseDevice::MouseState`, tracking held buttons and
the cursor position, and is readable through `Mouse::state()`.

`api/ui` still has no hit-testing to feed with it — that stays open on the
[luxa audit](LuxaAudit.md).

## core: two files that belong to the editor, not to `api`

- **`Scene` and `SceneVisitor`** — **moved to `vertical3d/src/` on 2026-09-02**, per
  [ADR-0013](../../adr/0013-mesh-is-a-dag-node.md). What follows is what the audit found. They are
  27 and 40 lines: a flat container of BReps, cameras and
  camera profiles, plus a visitor that walks the meshes. The plan asks whether they should be
  checked against `api/dag`. They should not be folded into it. `api/dag` is 374 lines of
  skeleton — `Node` holds an id and nothing else, `Root` and `Group` hold
  `std::vector<Node*>` with no method to add a child, there is no traversal and no visitor —
  and **nothing in the repository uses `api/dag` at all**. Neither can host the other today.
  `Scene`'s only consumer is `vertical3d/`, its own header calls its design an open question,
  and the editor rewrite is where that question gets answered. Move it with the editor in
  Phase 6.
- **`CreatePolyCommandSet`** — **moved to `vertical3d/src/CreatePoly.cxx` on 2026-09-02.**
  `create_poly_cube`/`plane`/`cylinder`/`cone` — is editor
  modelling code. `docs/TODO.md` already tracks merging it with rigel's `libv3dcommand`. Also
  Phase 6.

## component/Component.h

A sketch of a component system — `Node`, `Aggregate`, `Collection`, `Position3`,
`Transform`, and `Camera`/`PolyShape` built from them — with a design-notes comment block
where the library layout would go. It is in no `CMakeLists.txt`, has never been built, calls
`addComponent` which is declared nowhere, and includes headers deleted in `1b3f760`. entt and
`api/ecs` are the answer to what it was reaching for. Delete it; nothing to salvage but the
comment, and `docs/ECSDesign.md` already covers that ground.

## Tests

**Done 2026-08-31.** `v3dlibs/tests/` is gone; the corpus now lives in six per-library
`tests/` directories built as `v3dtest_<lib>` and run by ctest. What follows is the plan it
was moved against, with what actually happened noted against it.

`v3dlibs/tests/` was the largest single reason not to delete the tree. 23 Boost.Test
sources, 1,016 lines, plus nine image fixtures under `tests/data/`. Their include paths were
stale — `../3dtypes/` and `../image/ImageFactory.h` were renamed years ago — so each move was
a path rewrite, not a copy. In the event almost every file needed more than that: the type
and brep tests were on `v3D::Vector3` rather than glm, the image tests on an `ImageFactory`
that now takes a logger, and the font and input tests on classes that no longer exist.

Where each file goes:

| Test | Destination |
|---|---|
| `3dtypesTest`, `AABBoxTest`, `ArcBallTest`, `Bound2DTest`, `CameraTest` | `api/type/tests/` |
| `FaceTest`, `HalfEdgeTest`, `VertexTest` | `api/brep/tests/` |
| `ImageTest`, `ImageReaderTest`, `ImageWriterTest`, `TextureTest` | `api/image/tests/`, with `tests/data/` |
| `FontTest` | `api/font/tests/` |
| `FontCacheTest` | `api/font/tests/`, rewritten — `FontCache` is now `TextureFontCache` |
| `KeyboardDeviceTest`, `MouseDeviceTest` | `api/input/tests/`, rewritten against `Keyboard`/`Mouse`/`KeyState` |
| `BindTest`, `CommandTest`, `CommandDirectoryTest`, `CommandInfoTest`, `EventInfoTest`, `InputEventAdapterTest` | Drop, or rewrite against `api/event` |
| `TestMain` | One per target |

The six command-layer tests are the ones the plan flags for deletion "unless the behaviour
survived". Three were rewritten rather than dropped: `BindTest` covered event-to-command
matching, which is `Mapper`'s job; `CommandDirectoryTest` covered dispatch, which is
`event::Engine`'s; and `EventInfoTest` covered a name/state/data triple that is now
`event::Event`. They became `MapperTest`, `EngineTest` and `EventTest`. `CommandTest`,
`CommandInfoTest` and `InputEventAdapterTest` were dropped — the first two are covered by
`EventTest`, and the adapter's job is now what `Keyboard::handleEvent` and
`Mouse::handleEvent` do, which `KeyboardTest` and `MouseTest` cover directly.

`MouseDeviceTest` was worth rewriting first — it would have caught the empty
`Mouse::handleEvent`.

Two more, `BRepTest.cxx` and `CameraProfileTest.cxx`, were deleted in `6cfb4b6`, a commit
whose message is "tab cleanup". That looked accidental, and it was — but recovering them
from `6cfb4b6^` turned up two empty `BOOST_AUTO_TEST_CASE` bodies and nothing else. The same
goes for `luxa/tests/` at `d31a2e9^`: `ButtonTest.cxx` is an empty case, `ComponentManagerTest.cxx`
is a zero-byte file, and `TestMain.cxx` configures log4cxx, which left the repo years ago.
Nothing was lost in either deletion. Both files were recovered to their new homes and given
real bodies; the luxa three were left in history, since `api/ui` needs tests written against
what it does now rather than an empty shell.

## Corrections to the plan's starting notes

- **`voxel/src/Controller.h` is already off legacy headers.** It includes only `api/engine`
  and `api/event`. The Phase 1 item is done; voxel's remaining breakage is api drift, not
  legacy includes.
- **`vertical3d/` is the only consumer left**, through six headers across eleven include
  sites: `core/Scene.h`,
  `core/SceneVisitor.h`, `core/CreatePolyCommandSet.h`, `hookah/Window.h`,
  `gui/InputEventAdapter.h` and `command/CommandDirectory.h`. Since `vertical3d/` is Phase 6,
  the last of `v3dlibs/` cannot go before the editor rewrite starts — unless `core/` is moved
  across first and the rest deleted ahead of it, which is the cheaper order.
- ~~**`tetris` links `v3dlib_core` and uses nothing from it.**~~ **Dropped 2026-08-31.** No
  tetris source mentions `v3d::core`, so the line went with no replacement. Tetris's link
  list is still wrong in the other direction: its sources include `api/engine`, `api/event`,
  `api/gl`, `api/log` and `api/render`, and it links none of them - only `v3dlib_image`. That
  will surface as unresolved externals once the missing `GLFontRenderer.h` include is dealt
  with and tetris reaches the link stage for the first time.
- **The `Logger` in `v3dlib_core`'s CMakeLists** was not just "no longer on disk" — it moved
  to `api/log` in `81ca4cc`, and the stale entry means the target cannot configure.

## What has to happen before `v3dlibs/` can be deleted

1. ~~Drop `v3dlib_core` from tetris's link list.~~ **Done 2026-08-31.**
2. ~~Move `v3dlibs/tests/` to per-library `tests/` directories with corrected include paths,
   and recover `BRepTest`/`CameraProfileTest` from `6cfb4b6^`.~~ **Done 2026-08-31.**
3. ~~Move `core/Scene`, `core/SceneVisitor` and `core/CreatePolyCommandSet` to wherever the
   editor rewrite wants them — not into `api/dag`.~~ **Done 2026-09-02.** All three are in
   `vertical3d/src/` as `v3d::editor`, recorded as
   [ADR-0013](../../adr/0013-mesh-is-a-dag-node.md). `Scene` lost its camera and profile lists,
   which the views and `CameraProfiles` already own. `core/` held nothing else — its
   `Logger` had moved to `api/log` — so the directory and `v3dlib_core` are gone with them.
4. ~~Delete `component/Component.h`, `hookah/Hookah.h` and `hookah/drivers/sdl2/` with the
   tree. Nothing to salvage. (Deleting `drivers/sdl2/` also closes the SDL3 workstream.)~~
   **Done 2026-09-04**, with the whole tree: `command`, `gui`, `hookah`, `input` and
   `component` were what was left of it. The SDL3 workstream is closed with them.
5. ~~Rewrite `vertical3d/`'s six legacy includes, or accept that the tree survives until
   Phase 6 begins.~~ **Done** - the rewrite of 2026-09-01 dropped all six, and with `luxa/`
   deleted on 2026-09-04 **nothing in the tree includes a `v3dlibs/` header any more**. Only
   item 4 was left, which is the deletion itself.

`v3dlibs/` was deleted on 2026-09-04.

Separately — these are `api/` defects the audit turned up, and none of them keeps
`v3dlibs/` alive. **6 to 11 and 13 were fixed on 2026-08-31**; only 12 is still open.

6. ~~Implement `api/input/Mouse::handleEvent`.~~ Done, with `event::MouseMotion` and
   `input::MouseState` alongside it.
7. ~~Make press and release distinguishable.~~ Done, as `event::State` on the event and in
   the mapping key, with an optional `"state"` on a binding's source.
8. ~~Give bindings and events a parameter.~~ Done, as an optional `"param"` on a binding's
   destination, landing in the same `EventData` a menu item's value uses.
9. ~~Decide between `config::BindingContext` and `event::Mapper`.~~ Done — `Mapper` won,
   `BindingContext` and the commented-out `Config::loadBindings` are deleted.
10. ~~Add a way to invoke a bound event by name.~~ Done, as `event::Engine::dispatch`.
    `Menu::activate()` turned out not to need it.
11. ~~Restore frame delta in `Engine::eventLoop` and pass it to `tick()`.~~ Done. Apps still
    have to *use* it; none does yet.
12. ~~Wire `event::Context::active`, or delete it, before the editor needs state scoping.~~
    Deleted 2026-09-04. Phase 6 closed without needing it, so the need this item anticipated
    did not arrive, and a flag nothing sets or reads is worse than no flag: it reads as
    working scoping. `Context` is now a name. Recover it from `git show 5413f5c:api/event/Context.h`
    if state scoping is ever wanted; what it would take is in "State scoping is a stub" above.
13. ~~Fix `Event::str()`'s null-context dereference.~~ Done.

Two more defects surfaced while fixing those, both in the same machinery and both fixed:
a source event could only reach one destination, because `Mapper` used a `std::map` whose
`operator[]` silently overwrote — which is why pong's right paddle had never worked; and
`Keyboard::handleEvent` cleared key state on the wrong condition, so `KeyState` never
tracked a key correctly.

## What running the salvaged tests found

Every one of these was found by getting a test to compile and run against the code, and
every one was fixed on 2026-08-31 unless marked otherwise. Nothing here was visible before,
because nothing had executed this code since the corpus stopped building.

- **`writer::Bmp::write` overran both buffers.** A single index walked the destination with a
  modulo test for the row padding while reading the source, which has none — so it wrote
  past the end of a `pad * height` buffer and read past the end of the image. It corrupted
  the heap on a 2x2 image. `reader::Bmp::read` had the mirror of the same walk and read
  `width * height * bpp/8` bytes from a file whose rows are padded, so every row after the
  first landed in the wrong place. Both now copy a row at a time. Neither was noticeable
  before because every fixture is one flat colour.
- **`writer::Jpeg::write` never closed its file.** The handle leaked and the tail of the
  image stayed in the stdio buffer, so anything reading the file back in the same process
  got a truncated jpeg.
- **`log::Logger`'s constructor threw on the second one built.** `spdlog::basic_logger_mt`
  rejects a second registration under the same name, and tetris builds a `Logger` in its
  `Controller` before `Engine::initialize` builds another — so tetris threw on startup. It
  now takes over the registered logger.
- **`font::TextureFont` could not load a face at all.** The constructor asked FreeType for a
  size a hundred times the real one — the upstream trick for reading metrics precisely — which
  overflows `FT_Set_Char_Size` at the 64x horizontal resolution it uses, so the request failed
  and ascender, descender, height and linegap were all zero. Metrics now come off the face at
  its own size, read as 26.6 fixed point.
- **`image::Texture`'s copy lost the image.** Neither the copy constructor nor assignment
  copied `image_`, and the copy constructor zeroed the dimensions, so a copied texture kept
  its id and reported itself as an empty 0x0. `release()` cleared the image but not the type
  that `isnull()` reads, so a released texture went on claiming it had something to draw.
- **`brep` had two different `INVALID_ID` values** — `1 << 30` in `HalfEdge.cxx` and
  `1 << 31` as `BRep::INVALID_ID` — so an unpaired half edge never compared equal to the
  sentinel `BRep` tested it against. One constant now, in `HalfEdge.h`.
- **`brep`'s `operator==` were non-const**, which under C++20 makes every `a == b` ambiguous
  with the reversed candidate the compiler synthesizes. Any C++20 caller comparing a `Vertex`
  or a `HalfEdge` failed to compile.
- **Uninitialized state in `api/type`.** `CameraProfile` left `rotation_` and `size_` unset
  and `Camera::createView` read the first of them; `Camera::pan`/`tilt` rotated an
  uninitialized quaternion; `AABBox`'s default constructor left both extents unset, which
  `BRep::bound` returns for an empty mesh.
- **`ArcBall::bounds` guarded against a zero divisor off by one** — it clamped the viewport
  to 1, and the scale divides by (extent - 1), so a 1x1 viewport still produced NaN for
  every mapped point.
- **`Font2D::glyph` dereferenced `begin()` on an empty map** when called before `build()`.
  It returns null now. It still falls back to the first glyph for a charcode it does not
  have, which does not agree with `width()` returning 0 for the same charcode — left alone,
  since something has to draw.

Two things the tests document rather than fix, both in `api/type`:
`CameraProfile::size_` has no setter anywhere, so `Camera::orthoFactor` always divides by
zero; and `Camera::rotate` ignores any quaternion with a zero component, which is most of
the axis-aligned ones.
