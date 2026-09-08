# Game Foundations — Writing A Document, A Quad In World Space, And The Halves That Stop Short

Drafted 2026-09-07 against this tree from outside it, and staged here the same day, **open**. Thirteen
steps across `api/asset`, `api/engine`, `api/event`, `api/render`, `api/ui` and `api/audio`. One of
them is a defect in an app that ships in this tree today, one is a feature this tree shipped and
cannot finish, and the rest are the shape of what a *game* needs from these libraries that a demo
does not.

The prompting case is the same game as
[UiFoundations](completed/UiFoundations.md) — cozy, built against the api in another repository —
now planning the milestones after its menus. **But the reason to do this here rather than there is
that almost none of it is that game's alone.** The editor writes a project file with
`std::ios::trunc` and no temp file, so an interrupted save destroys the previous project. Pong's
rebinding menu can capture a key since UiFoundations closed and still forgets it on exit, because
nothing in the api writes a document. `ui::Cursor` and `ui::Keys` shipped with `UiConsolidation`
and no app can drive the first without working around an event that drops the position SDL handed
it. `grid::Overlay` can outline a tile and cannot fill one, because the quad primitive is screen
space and the line primitive is world space and nothing is a textured quad in world space. And
`api/audio` can start a sound and cannot stop it, loop it, or turn it down.

The consuming game's own M2 plan refused four of the steps below on this tree's one-consumer rule,
and is superseded on all four by the second consumers named here. Nothing in that repository is
linked from here, because those paths do not resolve from this one.

## Context

### Nothing in the api writes a document

[`asset::Json`](../../api/asset/Json.h) holds a parsed `boost::json::object` and hands it out.
[`asset::JsonFile`](../../api/asset/JsonFile.h) opens a file and reads it. `config::Config` resolves
a document by type. **There is no write path anywhere in `api/asset`**, and the tree has one
consumer that needs one and has built its own:

[`editor/scene/Project.cxx`](../../vertical3d/src/scene/Project.cxx) writes a versioned JSON
project — a `version` field, a name, a mesh array — through 133 lines of pretty-printer in an
anonymous namespace and then:

```cpp
std::ofstream file(path, std::ios::binary | std::ios::trunc);
```

`trunc` empties the file before a byte of the new document is written. A crash, a full disk or a
lost drive between that line and the flush leaves the player with neither project. This is the
oldest and cheapest defect in the plan and it is not the game's.

The pretty-printer is the other half. `boost::json::serialize` puts a document on one line, which
is fine for a network payload and useless for a save file somebody has to diff or read. The editor
wrote a good one — scalars and vectors kept on a line, records compacted, floats through
`std::to_chars` so a coordinate does not print as `0.10000000149011612` — and it is reachable only
from that translation unit. The consuming game needs the same thing for a settings file and again
for a save format, and its save-format milestone chose JSON over SQLite and LevelDB specifically
because inspectability was worth more to it than throughput. A third copy is what this step prevents.

### A player's settings have a path and no store

[UiFoundations](completed/UiFoundations.md) closed the first half: `engine::userPath(org, app)`
says where a player's files go, creates the directory, and is safe before `SDL_Init`. Nothing uses
it.

The second half is what an app does when it gets there, and every app that grows a settings screen
will answer it the same way: read one small document, apply what it holds over the shipped
defaults, write it back when something changes. **Pong is the proof that the missing half is
missing.** Its Options menu has four `key_input` items; since UiFoundations they capture a key and
`engine::Engine::rebind()` makes it take effect; quit the app and all four are gone. The
[rebind() docstring says so itself](../../api/engine/Engine.h) — *"What is not done here is
remembering it across runs. A binding lives as long as the process unless the app writes it
somewhere, which engine::userPath() says where."*

That is not a game's problem. It is a sentence in an api header describing a hole in the api.

### `MouseButton` drops the position SDL gave it

[`ui::Cursor::press(point)`](../../api/ui/Cursor.h) takes a point.
[`event::MouseButton`](../../api/event/MouseButton.h) carries a button index and an edge and no
position. [`input::Mouse::handleEvent`](../../api/input/Mouse.cpp) reads `event.button.x` and
`event.button.y` — the position SDL puts on every button event — writes them into its own
`MouseState`, and then constructs the dispatched event without them:

```cpp
state_(glm::vec2(event.button.x, event.button.y));
dispatcher_->trigger<v3d::event::MouseButton>(
    v3d::event::MouseButton(event.button.button, context_, pressed));
```

The `MouseState` that has the answer is unreachable: `engine::Engine::inputEngine_` is private, and
`input::Engine` exposes no device even to something holding one. So **every consumer of `ui::Cursor`
must track the last `event::MouseMotion` itself** to reconstruct a number this library measured,
stored and discarded three lines earlier. There are no such consumers in this tree yet, which is
exactly why it should be fixed before there are several.

[`MouseMotion`](../../api/event/MouseMotion.h) already carries a position and a delta. The
asymmetry is an oversight rather than a design.

### A ui with no tab order and a scrollbar that scrolls nothing

Two entries on [TODO.md](../TODO.md), both declined for want of an app asking, and both now asked
for by the same settings screen:

- **The focus moves by press and by press alone**, so there is no tab order and a form cannot be
  filled in without a mouse. That is not only a gamepad problem: it is a text box a keyboard user
  cannot leave, and a check box a screen reader has no path to.
- **A `Scrollbar` scrolls nothing.** [`SelectList`](../../api/ui/component/SelectList.h) owns an
  `offset()` and a `content()` and scrolls itself; `Scrollbar` owns a range and a thumb and knows
  about no list. Joining them is a dozen lines of arithmetic, and the tree left it as the app's
  *"until an app asks upstream for the component"*. A resolution list and a key bindings list are
  two lists in one screen, and the editor's absent file chooser is a third.

The focus half is the one that cannot be done app-side without reaching past `Engine::focus()` into
containers the app does not own. The scrollbar half can be, twice, badly.

### Word wrap exists once, privately, in the wrong library half

[`Immediate.cpp`](../../api/ui/Immediate.cpp) has a 25-line greedy `wrap()` in an anonymous
namespace, tested through `ImmediateTest`, and documented down to which of the two wrong answers it
gives for a word wider than the line. It is good code and the retained component tree cannot reach
it: [`component::Label`](../../api/ui/component/Label.h) holds a `std::string` and
`ComponentRenderer` draws *"one line of text at the position it holds"*.

[UserInterface.md](../UserInterface.md) says the two ways to write a ui are for different jobs and
that a hud belongs to the retained one. A hud, a tooltip, an item description and a line of
dialogue are all more than one line, and all four are on the consuming game's roadmap. This is a
helper moving up a level, not a feature.

### There is no textured quad in world space

The realtime renderer has two primitives and they divide the space between them the wrong way for a
game drawn in a projection:

| | space | drawn through |
|---|---|---|
| [`Canvas`](../../api/render/realtime/Canvas.h) | canvas pixels | its own orthographic projection, ignoring the pass camera |
| [`LineCanvas`](../../api/render/realtime/LineCanvas.h) | world | the camera the pass carries |

So a rectangle can be textured or it can be in the world, and not both.
[ADR-0005](../adr/0005-one-batched-quad-primitive.md) is the reason — one batched quad primitive,
and the primitive was built for a ui. It has held well: a panel, a sprite and a glyph are one
pipeline, and [ADR-0036](../adr/0036-text-is-a-distinct-kind-of-quad.md) amended it without
splitting it.

What it does not cover is a sprite standing on a ground plane. The consuming game has settled, in a
record of its own, on a fixed 3/4 isometric orthographic camera and a stylized 2D look, which means every tree, cabin and character
in it is a textured quad at a world position, sorted by depth. Today that app would have to project
world to screen itself, every frame, per sprite, and then hand the result to a canvas that has its
own idea of what the projection is — reimplementing the camera it already has.

This tree wants the same thing from the other direction and says so on [TODO.md](../TODO.md):
*"there is no world space filled primitive, so `Overlay.h` outlines a tile and cannot fill one. A
filled highlight wants a third primitive beside the quad and the line, which is a decision rather
than an addition."* `grid::Overlay` has no consumer today and odyssey draws no highlight, so this
is a want rather than a shipping defect — but it is the same want, arrived at from a tile grid
rather than from a sprite, and that is the argument for settling it once.

### A sprite sheet has a packer and no reader

[`image::TextureAtlas`](../../api/image/TextureAtlas.h) packs regions and is what `api/font` builds
a glyph atlas with. [`QuadRenderer::texture(image)`](../../api/render/realtime/vulkan/QuadRenderer.h)
uploads one. `Canvas::rect` takes a uv pair. Every piece of drawing a sprite out of a sheet is
present.

What is absent is the table in the middle: **a document that says `cabin` is at these pixels in
this image**, loaded the way every other kind of config is. Without it each app invents a manifest
format, and the two that exist — a sound config and a ui config — show what that costs when there
are three.

### Audio can start a sound and cannot stop it

[`audio::Engine`](../../api/audio/Engine.h) is `initialize`, `load`, `addClip`, `playClip`. That is
the whole surface, and `playClip` is:

```cpp
// fire and forget: the mixer owns the playback, and a clip may overlap itself
return MIX_PlayAudio(mixer_, found->second->audio());
```

There is no volume, no loop, no stop, no pause, and no handle on a sound once it has started. Pong
is well served by that and nothing else would be. Ambience is a bed that loops and fades; music is
a track that stops when the act changes; a settings screen has a volume slider or it is not a
settings screen — and **the consuming game's M2 deferred its entire audio settings tab** on the
grounds that there was nothing behind it to set.

The library underneath already does all of it. SDL3_mixer's current API is tracks, not fire and
forget: `MIX_CreateTrack`, `MIX_SetTrackAudio`, `MIX_PlayTrack` with a properties id carrying
`MIX_PROP_PLAY_LOOPS_NUMBER` and a fade-in, `MIX_StopTrack` with a fade-out, `MIX_SetTrackGain`,
and `MIX_TagTrack` with `MIX_SetTagGain` — which is a mixer bus for the cost of a string. `api/audio`
reaches past all of it to `MIX_PlayAudio`, the convenience call the header describes as playing
*"without any management"*.

## Decisions

Recorded in [adr/](../adr/), not here. Two new records; the numbers are the next free at drafting
and [adr/README.md](../adr/README.md) is the authority if something takes them first.

| ADR | Decision |
|---|---|
| **0041** | A document is written whole or not at all — written by step 1 |
| **0042** | A textured quad in world space, and how it relates to ADR-0005 — written by step 10 |
| [0005](../adr/0005-one-batched-quad-primitive.md) | One batched quad primitive — **amended or extended** by 0042, and step 10 is where which of the two is settled |
| [0019](../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) | The ui is laid out by what draws it — unchanged; steps 7 and 9 keep the `Measure`/`Write` seam |
| [0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) | An app resolves what a config names — unchanged; step 12's sheet resolves its image the same way a theme does |
| [0027](../adr/0027-the-api-is-consumed-as-source.md) | The api is consumed as source — unchanged, and why a consuming game can be planned against unreleased api |
| [0040](../adr/0040-a-key-goes-to-a-focused-component.md) | A key goes to a focused component — **extended** by step 8, which gives the focus a second way to move |

## What blocks what

```
1. ADR-0041 ──> 2. asset writes a document ──┬──> 3. the editor's save stops truncating   (the defect)
                                             └──> 4. engine::Settings ──> 5. pong remembers

6.  MouseButton carries its point        (independent, smallest in the plan)
7.  wrap() is the library's              (independent)
8.  the focus moves without the mouse    (independent)
9.  a scrollbar scrolls a list           (independent)

10. ADR-0042 ──> 11. the world space quad ──> 12. a sprite sheet is a document

13. audio grows tracks                   (independent)
```

**Four groups, and only two of them have an order inside.** Steps 6 to 9 finish widgets and touch
nothing else; 13 touches only `api/audio`.

**Step 3 is the defect and should not wait behind the rest of its group.** It needs step 2 and
nothing after it, and until it lands the editor destroys a project on an interrupted save. Steps 4
and 5 are the capability, and step 5 is the behaviour change that proves it, kept separate for the
reason [UiFoundations](completed/UiFoundations.md) kept its step 6 separate: one concern per commit,
and *the library can now do this* is a different concern from *this app now does it*.

**Steps 10 to 12 are one concern in three commits**, the way that plan's steps 2, 4 and 5 were:
nothing draws a world space quad until 11 lands, and 12 is unusable without it.

**Step 6 is worth doing first whatever else happens.** It is a constructor argument and an accessor,
and every day it is not done is a day another app writes the workaround.

## Steps

### Step 1 — ADR-0041, a document is written whole or not at all

**Landed** as [ADR-0041](../adr/0041-a-document-is-written-whole-or-not-at-all.md).

The record comes first, per [sdlc.md](../sdlc.md).

Small as decisions go, and worth one because it is a rule about every file this tree will ever
write — a project, a settings document, a save — and because the wrong answer is the one that is
already shipping and looks fine until the day it does not.

What it has to settle:

- **Temp file plus rename**, and where the temp file goes. Beside the target, not in the system temp
  directory: a rename across volumes is a copy and is not atomic, which is the whole point.
- **What "whole" means when the rename fails.** The previous document survives and the caller is
  told; a partially written temp file is removed.
- **That readable output is the default and not an option.** A document a person cannot diff is a
  document nobody checks, and every writer this tree has is writing something a person will read.
- **Where the boundary sits between `api/asset` and a caller.** The library writes bytes and
  serializes a `boost::json::value`; what goes in the document is the caller's, exactly as reading
  works today.
- **Whether `std::filesystem` or `boost::filesystem`.** `api/asset` already links Boost.Filesystem
  for `Manager::path_`, so this is a consistency question rather than a dependency one.

### Step 2 — `api/asset` writes a JSON document, atomically and readably

**Landed** as `api/asset/Writer.h`, free functions rather than a class: there is no state to
hold, and the atomic byte write is not JSON's, so `JsonFile` was left alone. The serializer is
`serializeDocument` rather than `serialize`, because an unqualified `serialize` on a
`boost::json::value` resolves to boost's one-line one through ADL.

In [`api/asset/`](../../api/asset/), beside `Json.h` and `JsonFile.h`.

Two things, and they belong in one step because either alone is half a writer:

- **The atomic write.** A `write(path, bytes)` that goes to `path.tmp` and renames, per step 1.
- **The readable serializer.** Move the printer out of
  [`editor/scene/Project.cxx`](../../vertical3d/src/scene/Project.cxx) verbatim — `scalar`,
  `scalarArray`, `compact`, `indent`, `printNumber`, `printArray` and `print` — and give it a name
  in `v3d::asset`. It is already good and already has the reasoning in its comments; this is a move,
  not a rewrite, and reviewing it as a move is what keeps it one.

`JsonFile` gains the write side of what it already has for reading, or a `JsonWriter` sits beside
it — the class is a thin `FILE*` wrapper whose read half is *"based on the example JSON loading code
in the boost library"*, and it is not obvious that the writer should be inside it. Decide when the
code is open; note in passing that `JsonFile::open` uses `fopen_s`, which is MSVC's, and that the
tree is Windows-only today but need not add a second reason to stay that way.

**Additive.** Nothing calls it until step 3, so this lands without changing any app.

**Tests.** `api/asset/tests/` exists. The serializer is pure: a document round-trips through
`print` and `boost::json::parse` unchanged, a float prints short, a vector stays on one line. The
atomic half wants a temp directory: a write over an existing file leaves the old contents when the
rename is made to fail, and leaves no `.tmp` behind either way.

### Step 3 — The editor's project save stops truncating

**Landed.** Verified as the plan asked: a project written by the old path and by the new one
are byte for byte the same 2180-byte document, and the untouched-on-failure half is a case in
`ProjectTest` rather than a one-off check.

In [`editor/scene/Project.cxx`](../../vertical3d/src/scene/Project.cxx).

`Project::write` drops its `std::ofstream` and its copy of the printer and calls step 2. The diff
should be almost entirely deletions.

**Its own commit**, because it is a defect fix in an app and step 2 is a capability in a library,
and because this one deserves to be findable later by somebody asking when the editor stopped
eating projects.

**Verify by running it**, per [sdlc.md](../sdlc.md#4-verify): save a project, confirm the document
is byte-identical to what the old path produced, and confirm the file is untouched when the write
is made to fail.

### Step 4 — `engine::Settings`, a user's overlay over the shipped defaults

**Landed.** Two departures from the draft. The settings sit under a `settings` key rather than
at the root, so `version` belongs to the format and an app can still have a setting called
version. And `set()` takes a `boost::json::value` rather than an overload per type, because a
`const char*` argument binds to `bool` before it binds to `std::string`; the reads keep
distinct names — `text`, `number`, `integer`, `flag` — for the same reason.

In [`api/engine/`](../../api/engine/), beside [`Application.h`](../../api/engine/Application.h),
because that is where `userPath()` already is and because `api/engine` already names SDL3 as a
package where `api/asset` deliberately does not.

**An overlay of what the player changed, not a copy of what shipped.** One flat document, so that
deleting it is a reset and a setting nobody touched keeps tracking the shipped default rather than a
snapshot of it taken the first time the player opened the menu. A full shadow copy is the obvious
alternative and it freezes the defaults at the version a player first saved — add a binding in a
later build and no existing player ever sees it.

The surface is small and should stay small:

- `load()` from `userPath(org, app)`, through the same `asset::Json` loader everything else reads
  with. A missing file is not an error and is the common case.
- Typed reads with a default — a string, a number, a bool — because every caller has a shipped value
  to fall back to and none of them should be writing that fallback twice.
- `set()` and `save()`, through step 2. **Save on change, not on exit**: there is no exit path that
  reliably runs, and a crash after a rebinding should not lose the rebinding.
- **A version field, and a document from the future is left alone.** A build that reads a version it
  does not know runs on defaults and does not overwrite — that is the case a player who downgrades
  hits, and the case nobody writes the first time.

**What it does not do.** It does not know what a binding is, or a window size, or a volume. The
schema of the document is the app's, exactly as `config::Config` does not know what a camera profile
means. Applying a binding is already `Engine::rebind()`; applying a window size is already
`Window::request()`. This step is the file, and the file only.

**Tests.** `api/engine/tests/` exists and needs no window for this: a missing file yields defaults, a
round trip preserves what was set, an unknown key is ignored rather than dropped on the next write,
a future version is not overwritten, and a truncated document does not take the defaults down with
it.

### Step 5 — Pong remembers what it was told

**Landed**, with the org and app recorded in
[Architecture.md](../Architecture.md) where the next app will look. Verified by running it:
rebound Player 1 Up to `j` through the menu, confirmed the document, restarted and confirmed
`j` moves the paddle and `w` no longer does, deleted the document and confirmed `w` does again.

In [`pong/src/PongEngine.cxx`](../../pong/src/PongEngine.cxx).

`rebindPaddleKey` already calls `rebind()`. It gains a `set()` and a `save()`; `initialize()` gains
a `load()` and a `rebind()` per stored entry, immediately after `Engine::initialize()` returns.

**Four lines, and it closes a hole this tree has shipped in a settings screen since before
UiFoundations opened it half way.** It is also the second consumer that makes step 4 a library
rather than a guess: if the shape does not fit pong, it does not fit.

The org and app strings are a decision with a long tail — change them later and every player's
settings are orphaned — so pick them here deliberately and note them where the next app will look.

**Verify by running it**, which for this is: rebind a paddle, quit, restart, confirm the paddle
answers the new key, delete the file, restart, confirm it answers the shipped one.

### Step 6 — `MouseButton` carries the point SDL gave it

**Landed.** The constructor takes the point and the button case in `Mouse::handleEvent` was
braced so it can be a `const` local rather than a member of the enclosing function.

In [`api/event/MouseButton.h`](../../api/event/MouseButton.h) and
[`api/input/Mouse.cpp`](../../api/input/Mouse.cpp).

A `glm::vec2 position()` beside `button()` and `pressed()`, filled from the `event.button.x`/`y`
that `Mouse::handleEvent` already reads two lines above where it builds the event.

**The smallest step in the plan and the one with the widest reach.** Without it every app that wires
`ui::Cursor` — which is every app that grows a mouse-driven ui, and none have yet — tracks the last
motion in a member of its own to rebuild a number this library threw away. With it, `Cursor::press`
is fed from the event that caused it.

Take the constructor argument rather than a setter, so the event cannot exist without one.

**Tests.** `api/event/tests/` and `api/input/tests/` both exist. The event carries what it was
built with; that is the whole assertion, and it is worth having because the thing being prevented is
a field silently going back to being dropped.

### Step 7 — `wrap()` is the library's, and a label can be more than one line

**Landed.** `wrap()` is in `Text.h` beside `Measure` and `Write`, taking a `Measure` rather
than the `std::function<float(const std::string&)>` the private one took. A width that is not
positive is one row rather than a row per word, which is what a percentage of a zero width
parent would otherwise produce.

In [`api/ui/`](../../api/ui/), out of [`Immediate.cpp`](../../api/ui/Immediate.cpp)'s anonymous
namespace, and then [`component/Label.h`](../../api/ui/component/Label.h) and the label's draw path
in [`ComponentRenderer`](../../api/ui/ComponentRenderer.cpp).

Two halves:

- **Promote the helper.** `Text.h` is where the `Measure`/`Write` pair already lives and names no
  font type; a free `wrap(line, width, measure)` belongs beside them. `Immediate` keeps calling it
  and its tests keep passing, which is what makes this safe.
- **A label wraps when it has a width to wrap to.** A `Label` whose layout gives it an `Auto` width
  is one line, exactly as today. One that was given a width or a percentage wraps to it, and its
  `Auto` *height* becomes the rows it came to — which is the shape
  [ADR-0039](../adr/0039-layout-never-reads-the-box-it-wrote.md) already describes for a component
  that makes something of an axis itself, so nothing about layout changes to accommodate it.

**Do not add a rich-text component.** A run of text with per-span colour, a link, an inline icon —
those are a different thing and the moment to design one is when something asks. A wrapped paragraph
is the 90% and it is 30 lines.

**Tests.** `api/ui/tests/` needs no font: the measure callback is the caller's, so a test measures a
character as one unit and asserts where the breaks fall, and asserts the box a wrapped label was
left holding.

### Step 8 — The focus moves without the mouse

**Landed.** One departure: `Keys::press` takes a second argument saying whether shift is held.
A key name carries no modifier, `api/ui` cannot reach `api/input` without taking SDL with it,
and there is no key name for shift-tab — so the app that saw the key says. It defaults to
false, which is forward-only tab for a caller that ignores it. No `tabIndex` was added.

In [`api/ui/Engine.h`](../../api/ui/Engine.h) and [`Keys.cpp`](../../api/ui/Keys.cpp), extending
[ADR-0040](../adr/0040-a-key-goes-to-a-focused-component.md).

`Engine::focus()` holds one component at a time and a press is the only thing that calls it. It
needs a second caller: a traversal that finds the next `focusable()` component after the one that
has the focus, and gives it the focus.

The order is the question, and the answer that costs nothing is **the order the tree holds them
in** — the same walk that draws, which is depth order with add order between equal depths, and a
flow box's children in the order it was given them. A ui author who wants a different tab order
reorders the document, which is how it works everywhere else. Do not add a `tabIndex` until
something needs one; it is a field on every component to serve a case nobody has.

`Keys::press` grows the two keys that do it, and returns true for them so they do not also reach
the app's bindings. A ui with nothing focused stays untouched — that is the property that keeps a
game's movement keys working, and tab must not quietly take the focus onto the first widget of a
hud nobody is looking at.

**Tests.** Focus moves forward and backward through a tree of mixed focusable and unfocusable
components, wraps at each end, skips a hidden subtree, and does nothing at all in a ui where nothing
is focused.

### Step 9 — A scrollbar scrolls a list

In [`api/ui/component/Scrollbar.h`](../../api/ui/component/Scrollbar.h) and the renderer's draw path
for it.

A `Scrollbar` that has been told which component it scrolls reads that component's content extent
and offset instead of its own, and writes the offset back when its thumb is dragged.
[`SelectList`](../../api/ui/component/SelectList.h) already has both — `content()` is *"how tall all
the rows come to, which is what a scrollbar's content is"*, which is a sentence written for a
consumer that did not exist.

**Bind by pointer, not by name**, and hold it weakly the way `Cursor` holds a press and `Engine`
holds the focus: the list belongs to its container, and a bar outliving one that was unloaded should
not keep it alive.

**Keep the unbound bar exactly as it is.** A scrollbar with its own range is a perfectly good
progress-shaped control and something will want one.

The generalisation to resist is a `Scrollable` interface for every component that might one day
scroll. Two components and one relationship; the interface is worth having at three.

**Tests.** A bar bound to a list reports the list's content, a drag moves the list's offset, a list
with nothing to scroll draws a track and no thumb, and a bar whose list has gone does not crash.

### Step 10 — ADR-0042, a textured quad in world space

The record comes first, and this is the one in the plan that genuinely could go either way.

What it has to settle:

- **A third primitive, or a mode of the quad.** `LineCanvas` is the precedent for the first: a
  separate canvas, world coordinates, drawn through the pass camera, and
  [ADR-0011](../adr/0011-lines-are-the-second-primitive.md) is the record that made it a peer rather
  than a variant. The second is `Canvas` learning that a batch is world space and taking its
  projection from the pass, which is a smaller change and a larger claim, since `Canvas` currently
  *is* the definition of screen space in this tree.
- **Whether it amends [ADR-0005](../adr/0005-one-batched-quad-primitive.md) or sits beside it.**
  0005 says one batched quad primitive draws every 2D thing; 0036 amended it once already, for text.
  A world space quad is either the third amendment or the honest admission that 0005 was about the
  ui.
- **Depth.** A sprite standing on a ground plane has to sort against the other sprites. Whether that
  is the depth buffer, a painter's sort the caller supplies, or a sort key on the batch is the
  substantive question in this record, and the answer decides what the consuming game's world looks
  like. **A sort the caller supplies is the likely answer** — what a sprite's depth *means* in an
  isometric projection is the game's knowledge, not the renderer's — but it has to be said out loud,
  because the alternative is a renderer that quietly gets it wrong for the second app.
- **What it is not.** Not a sprite system, not a scene graph, not billboarding. A textured rectangle
  with four world corners.

Both of the tree's own wants should be in the context: a filled tile highlight for
[`grid::Overlay`](../../api/grid/Overlay.h), and a sprite at a world position for the game this plan
came from.

### Step 11 — The world space quad

In [`api/render/realtime/`](../../api/render/realtime/), as step 10 settled it.

The vertex format already carries position, uv and colour; the pipeline already samples a texture;
the pass already carries a camera at set 0 because `LineCanvas` draws through it. Most of this step
is deciding it, which is why step 10 is separate and comes first.

**Verify it against both consumers**, which is what stops it being shaped for one:

- `grid::Overlay` gains a filled tile, and the segment sink it hands geometry through is the model
  for how it should hand quads out — nothing in `api/grid` names a renderer, and that should stay
  true.
- A textured quad standing on a ground plane, drawn through a 3/4 isometric orthographic profile —
  which `config::CameraProfiles` reads and
  [`vertical3d/data/cameras.json`](../../vertical3d/data/cameras.json) can carry — sorts correctly
  behind and in front of another one.

CI renders nothing, so this is a run-and-look step with the validation layer through the logger, per
[sdlc.md](../sdlc.md#4-verify). The cpu half — the batching, the sort key, the geometry — is
testable without a device the way `Canvas` is, and should be.

### Step 12 — A sprite sheet is a document

In [`api/config/`](../../api/config/) for the type, and `api/render` or `api/image` for what it
resolves to — decide by which one can name the other without a new dependency edge, the way
[`config::CameraProfiles`](../../api/config/CameraProfiles.h) was placed.

A `sprite` config type beside `window`, `binding`, `ui`, `sound`, `camera` and `layout`: an image,
and a table of names over pixel rectangles in it. Loaded like every other config, resolved to a
texture by the app the way a theme's images are, per
[ADR-0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) — the library reads the
document and never touches the asset manager.

**Pixels in the document, uv at the call.** An author reads a sprite sheet in pixels and a shader
wants a fraction; converting at load means the document breaks if the sheet is ever rescaled, and
means nobody can read the numbers.

**Do not add animation here.** A named frame sequence and a frame rate is the obvious next field and
it is a second decision — what advances it, what happens at the end, whether a sprite has state —
and the consuming game does not reach that until M5. One document that says where a picture is, and
then see what asks for more.

### Step 13 — A clip loops, stops, and has a volume

In [`api/audio/`](../../api/audio/).

`playClip` moves off `MIX_PlayAudio` onto a track, and the four things a game needs follow from
having one:

- **A handle back.** `play()` returns something that can be stopped, so ambience can end when the
  scene does.
- **Looping and fades**, through the properties `MIX_PlayTrack` already takes —
  `MIX_PROP_PLAY_LOOPS_NUMBER`, and the fade-in and fade-out that make a bed start and end without a
  click.
- **Gain, per track.**
- **Buses, through tags.** `MIX_TagTrack` plus `MIX_SetTagGain` is a named group with a volume for
  the cost of a string, so `music`, `sfx` and `ambience` are three tags and a settings screen is
  three sliders. **This is the piece to get right**, because it is the difference between a volume
  control and a volume control per kind of sound, and every game wants the second.

**Keep `playClip` working.** Pong calls it, the sound config format does not change, and a one-shot
that nobody holds a handle to is still the common case — it just gets a track underneath it now.

`AudioClip` needs no change: it already loads against no mixer.

**Tests.** `api/audio/tests/` exists and cannot open a device in CI, which bounds this: the clip
table, the tag bookkeeping and the id lifetime are testable, and whether a sound is audible is not.
That is the same line [Testing.md](../Testing.md) already draws around `audio::Engine::initialize()`.

## Considered and not done

**Promoting `CommandDirectory` to `api/event`.**
[ADR-0017](../adr/0017-a-command-is-a-name-in-a-context.md) rejected this with a stated condition —
*"it moves when a second app wants it"* — and the consuming game's M1 plan predicted its M2 would be
the trigger. It read the code and concluded not yet: about twelve commands in two contexts that do
not overlap, answered by two `if` chains, where the editor's directory exists to serve 76 commands
with several ways into each. That reasoning holds from this side too. **The trigger is three ways
into one command** — a binding, a menu item and a line of dialogue — which is that game's M10. Left
where it is, in `vertical3d/src/command/`.

**Serializing a `config` document back out.** `UiConsolidation` declined it for having one consumer,
and the consuming game's M2 reached the same answer from the other direction: what an app writes is
a small overlay of its own shape, not a config document round-tripped, so the serializer nobody has
would not produce it. Step 4 is the thing that was actually wanted, and it is a different thing.

**A `tabIndex` on `Component`.** Step 8 traverses in document order. A field on every component to
serve a case nobody has is the definition of speculative, and adding one later is additive.

**Sprite animation.** Step 12's reasoning: it is a second decision and nothing asks yet.

**Asset hot reload.** On the consuming game's asset-pipeline milestone as *"hot-reload in debug
builds"*, and it is genuinely library-shaped — `asset::Manager` owns the cache and the paths. It is left out
because the hard half is not noticing a file changed, it is what a live `TextureHandle` does when
the image behind it is replaced, and that question belongs after step 11 has settled what a texture
is used for. Carry it to [TODO.md](../TODO.md) if this plan closes without it.

**Gamepad input.** `api/input` has a keyboard and a mouse. Step 8 removes the reason a gamepad
*could not* drive a ui, which is the part that is this tree's; nothing on the consuming game's
roadmap asks for a gamepad, and adding a device nobody has asked for is how a library grows a
surface it cannot test.

**ECS serialization for a save format.** EnTT has snapshot support and what a component *means*
across a version boundary is the app's, not the library's. The consuming game's M4 is where this is
answered and it should be answered there.

## Notes for the tree, whatever lands

Two corrections found while reading, neither of which is a step:

- **`Canvas` has a `scale()` now.** [TODO.md](../TODO.md) and
  [completed/UiFoundations.md](completed/UiFoundations.md) both carry *"`Canvas` still exposes
  `translate()` and no scale"* as an open item. `Canvas.h` declares `void scale(const glm::vec2&)`.
  Whichever landed it did not close the note.
- **`JsonFile::open` uses `fopen_s`**, which is MSVC's, as do all four image readers and writers.
  The tree is Windows-only and this is not urgent; it is worth knowing before step 2 adds a fifth.
