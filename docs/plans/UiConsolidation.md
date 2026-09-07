# UI Consolidation — The Draw Path, The Widget Set Nothing Drives, And One Theme For Two UIs

Drafted 2026-09-06 from an architecture review of `api/ui`. **Open.** Twelve steps across
`api/ui`, one app data file, and the editor's cursor routing.

`api/ui` is ~9,100 lines of library and ~3,750 of tests, and a third of the library is three
files: [`ComponentRenderer.cpp`](../../api/ui/ComponentRenderer.cpp) at 1124,
[`Engine.cpp`](../../api/ui/Engine.cpp) at 912 and [`Immediate.cpp`](../../api/ui/Immediate.cpp)
at 762. It grew fast — [ADR-0034](../adr/0034-a-component-has-children-and-a-box.md),
[0035](../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md),
[0036](../adr/0036-text-is-a-distinct-kind-of-quad.md) and
[0037](../adr/0037-clipping-is-a-scissor-the-batch-carries.md) all landed on 2026-09-06 — and
what it has not had since is a pass over its shape.

**Three of the twelve steps are defects that ship in this tree today**, and one of them makes a
documented feature do nothing. The rest is the library's shape: a draw path that allocates per
component per frame, a widget set whose interaction half has no consumer, and a physical design
that puts the JSON loader into every app that wants to draw a string.

## Context

### A translucent panel is opaque, and `window(alpha)` does nothing

[`plateBox()`](../../api/ui/Painter.cpp) fills the **whole** box with the outline colour and
then fills the inset over it. Shipped defaults are `border` at alpha 1.0 and `panel` at alpha
0.92, on both sides of the library — [`ComponentRenderer::Style`](../../api/ui/ComponentRenderer.cpp)
and [`Immediate::Style`](../../api/ui/Immediate.cpp) declare the same two values. So a panel's
interior is `0.92 × panel + 0.08 × border`, which is **fully opaque**: the scene behind never
shows through, and the panel is tinted 8% toward its own border.

[`fillBox()`](../../api/ui/Painter.cpp) goes to some trouble to lay its three bands and four
wedges down without overlapping — *"which matters because a box is usually drawn with an alpha,
and anything drawn twice under one would show"* — and then `plateBox` does exactly that at the
scale of the whole box.

The sharpest case is [`Immediate::window()`](../../api/ui/Immediate.cpp), which takes an `alpha`
argument, multiplies the panel colour by it, and hands the result to `plateBox`. **The argument
is inert.** A window at alpha 0.5 is as opaque as one at 1.0 and differs only in hue.

[`style/property/Color.h`](../../api/ui/style/property/Color.h) states the intent this defeats:
*"a panel that lets the scene through is a colour rather than a mode."*

### The draw path allocates per component, per frame

Every drawn component runs [`lookup()`](../../api/ui/ComponentRenderer.cpp), which calls
[`Theme::getStyleSet()`](../../api/ui/style/Theme.cpp) — a `std::vector` of every matching style,
built by linear scan with `std::string` comparison, of which `front()` is kept and the rest
discarded — plus a `std::string(name)` allocation from the `string_view` on the way in. Then it
runs four to seven `readColour`/`readMetric` calls, each of which constructs a
`pair<std::string, std::string>` key ([`Style.cpp`](../../api/ui/Style.cpp)) and does a
`dynamic_pointer_cast` to recover a type the class string already named. A select list does seven
of these per frame, a check box six, a panel four.

Three more on the same path:

- **Text measurement allocates.** `Measure` takes `const std::string&` while components store
  text and hand back `string_view`, so [`ComponentRenderer.cpp`](../../api/ui/ComponentRenderer.cpp)
  reads `measure_(std::string(label->text()))` at three sites.
- **`natural()` for a `SELECT_LIST` measures every item, every frame**, to find the widest row —
  a hundred-row list is a hundred text measurements per frame for a number that changes only when
  `items()` is called, which is its one mutator.
- **`ordered()` allocates and sorts on every call** — both
  [`Container::ordered()`](../../api/ui/Container.cpp) and the free
  [`ordered(children)`](../../api/ui/Component.cpp) — once per parent node per frame in `walk()`,
  and again per frame in `pick()`.

The cost is `O(components × styles × properties)` with an allocation at every level. It is
invisible at ten components and degrades exactly where a HUD does, which is what ADR-0034 built
the box model for.

### Fourteen component types load, and nothing drives their cursor half

`Container::pick()`, `SelectList::at()`, `TabBar::at()`, `Scrollbar::drag()`, `CheckBox`, `Bar`,
`Panel`, `Box` and `Icon` have **no caller anywhere in the tree**. Only `MenuBar` and `Toolbar`
— in the editor — and `GameMenu` are driven.

That is not merely disuse; there is a hole under it. A `Button` in a `Container` carries an
`event()`, and nothing dispatches it: `Toolbar::press()` dispatches for a button in a strip and
`MenuBar::press()` for an item in a panel, but a button in a tree has no equivalent. `Container`
does not even hold a dispatcher. So `pick()` hands the app a `shared_ptr<Component>` and the app
downcasts it, decides what a press means, and dispatches — per app.

Meanwhile the editor already hand-writes the routing that does exist.
[`Controller::uiMotion`](../../vertical3d/src/Controller.cxx) offers the cursor to the menu bar
first *"because an open panel is drawn over a toolbar"*, then tells every toolbar to `leave()` or
`motion()`; [`uiPress`](../../vertical3d/src/Controller.cxx) does the same ordering again for a
press. That ordering rule belongs to the library — it is a fact about how the library draws —
and [ADR-0028](../adr/0028-an-apps-shell-belongs-to-the-api.md) is the standing answer for what
every app repeats.

### Two `Style` structs, one style class, and defaults that differ by 2×

[`ComponentRenderer::theme()`](../../api/ui/ComponentRenderer.cpp) and
[`Immediate::theme()`](../../api/ui/Immediate.cpp) both read the `"ui"` style class, with
overlapping key names — `line-height`, `padding`, `bar-height`, `radius`, `scrollbar-width` —
into two different structs whose defaults are roughly a factor of two apart:

| | `ComponentRenderer` | `Immediate` |
|---|---|---|
| `lineHeight` | 34 | 18 |
| `padding` | 24 | 6 |
| `barHeight` | 28 | 22 |
| `radius` | 0 | 3 |

Which is right for each: a game HUD is read at a glance and a tool panel is read closely.
ADR-0035 nonetheless records as a positive consequence that *"a theme's 'ui' style dresses
both"* — true mechanically, and false in effect, because a theme that sets `line-height` for one
breaks the other.

**This is latent rather than live.** Only the editor defines a `"ui"` style at all
([`vertical3d/data/vgui.json`](../../vertical3d/data/vgui.json)), it sets six colours and no
metrics, and those six are byte-identical to the defaults they override. It becomes real the
first time anything themes a metric, or the first time anything drives `Immediate` — which is
step 11.

`Immediate::theme()` also duplicates `lookup()` inline instead of calling it, so the two will
drift on their own.

### Three types called `Style`, and three namespace rules in one directory

`v3d::ui::Style` (a bag of properties), `v3d::ui::style::Theme`'s notion of one, and
[`ComponentRenderer::Style`](../../api/ui/ComponentRenderer.h) (a struct of colours and metrics)
are three concepts within one letter of each other. It already costs a docblock:
`lookup()` has to say *"the return type is the library's Style and not this class's."* When a
signature needs a paragraph to say which type it returns, the naming is the defect.

[`Conventions.md`](../Conventions.md) says namespaces mirror the `api/` path. In
`component/menu/`, `Menu.h`, `MenuBar.h` and `MenuItem.h` are `v3d::ui::component` while their
sibling [`Type.h`](../../api/ui/component/menu/Type.h) is `v3d::ui::menu`; in `style/property/`
everything is `v3d::ui::style::prop`, an abbreviation the path does not have. Three rules, one
subtree.

The headers are wrong in the other direction too.
[`ComponentRenderer.h`](../../api/ui/ComponentRenderer.h) includes `Engine.h` and fourteen
component headers purely to name types in signatures, and `Engine.h` pulls `boost::json`, EnTT,
the event engine and the logger behind it. Then
[`TextRenderer.h`](../../api/ui/TextRenderer.h) includes `ComponentRenderer.h` — only to name
the `Measure` and `Write` typedefs. So every app renderer that wants to draw a string compiles
the JSON loader and the whole widget set. All of it is forward-declarable.

`Measure` and `Write` are themselves declared twice as unrelated types, on
[`ComponentRenderer`](../../api/ui/ComponentRenderer.h) and on
[`Immediate`](../../api/ui/Immediate.h). They interoperate only because both are `std::function`
of the same signature, so `TextRenderer::measure()` returning the `ComponentRenderer` one and
being handed to `Immediate` reads as a mistake that happens to compile.

### Dead weight

[`Overlay.h`](../../api/ui/Overlay.h) has no consumer, opens with a stray doubled `/**`, and its
premise — a transparent *mode* against a colour mode — is contradicted outright by `Color.h`
above. [`Menu::navigate()`](../../api/ui/component/menu/Menu.cpp) ignores its `wrap` argument,
moves nothing, and returns `true` for any valid enum value, under a header that promises
*"Navigate changes the currently active menu item… @return false when no navigation was
possible"*; `GameMenu` calls `next()`/`previous()` and routes around it. `Dialog`, `Spinner` and
`TextBox` are `Component` subclasses with no members; `InputBox.h` and `ToolTip.h` are a
`#pragma once` and a copyright header; `PopupMenu` and `RadialMenu` are `using Menu::Menu`.

[`component/Type.h`](../../api/ui/component/Type.h) advertises all twenty-five of them, so the
enum claims a widget set nearly twice the size of the one with a loader — in `SCREAMING_CASE`
inside an `enum class`, with a `TYPE_` prefix, a gap at 9, and `HORIZONTAL_FRAME`/`VERTICAL_FRAME`
naming classes actually called `HorizontalBox` and `VerticalBox`.

[TODO.md](../TODO.md) already records the stubs and the undriven `Immediate`. It does not record
that two ad-hoc replacements for `Immediate` exist in the tree —
[`voxel/src/DebugOverlay.h`](../../voxel/src/DebugOverlay.h) and
[`api/ui/StatisticsOverlay.h`](../../api/ui/StatisticsOverlay.h) are both a rolling frame average
rendered as lines of text, which is the panel the immediate layer was built for.

### Two small ones

[`Component`](../../api/ui/Component.cpp)'s constructor initialises in an order that does not
match its declaration order. Harmless today because no initialiser reads another, and MSVC does
not warn — but it is a `-Wreorder` on any clang build and a footgun the moment one of them does.
And `Component::lastID` is a public, non-atomic, never-reset mutable global, which also means no
test can assert on an id.

## Decisions

Recorded in [adr/](../adr/), not here.

| ADR | Decision |
|---|---|
| **0038** | Who turns a cursor into a command — written by step 8, and the reason step 9 is the api's rather than each app's |
| [0019](../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) | The ui is laid out by what draws it — unchanged. Step 7 moves the walk into a class of its own; it does not move it out of the draw |
| [0034](../adr/0034-a-component-has-children-and-a-box.md) | A component has children and a box — unchanged |
| [0035](../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md) | An immediate mode layer over the same canvas — **corrected** twice by this plan: its state map is not pruned (step 3), and its "ui" style does not dress both sides (step 10) |
| [0020](../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) | A theme is data and the app resolves its images — unchanged; step 10 splits a class within it, not the rule |
| [0028](../adr/0028-an-apps-shell-belongs-to-the-api.md) | What every app repeats belongs to the api — the argument step 9 rests on |

0038 is the next free number; [adr/README.md](../adr/README.md) is the authority, and `0026` is a
reserved gap rather than an available one.

## What blocks what

```
Phase 1 — subtract and repair            Phase 2 — names and headers
1. delete the dead ──────────────────┐
2. plateBox stops filling under ──┐  ├──> 5. namespaces and the three Styles ──┐
3. Immediate prunes its state     │  └──> 6. Measure/Write get a header ───────┤
4. Component's construction       │                                            │
                                  │       Phase 3 — the draw path              │
                                  │       7. a Dressing resolver that caches <─┘
                                  │       8. the layout walk comes out
                                  │
   Phase 4 — what the library is for
   9.  ADR-0038 ──> 10. a container dispatches ──> 11. the editor stops routing
   12. the "ui" class splits <── 13. Immediate gets a consumer
                                  │
   Phase 5 — 14. docs/UserInterface.md <─── everything
```

Steps 1–4 are independent of each other and of everything else; each is small and separately
shippable, and **step 2 is the one to do first** because it is a visible defect with a two-line
fix. Step 1 goes before step 5 so that nothing gets renamed on its way to being deleted.

Steps 5 and 6 both rewrite header text across the library and should land before Phase 3 rather
than after, so the draw path is written once against the final names. Step 6 in particular
changes the `Measure` typedef, which step 7 changes again — doing them in the other order edits
the same declaration twice.

Step 7 before step 8, because the resolver *is* the style-resolution half of the split: extracting
it leaves `ComponentRenderer` holding layout and paint, which is what step 8 separates. Doing
them the other way round means splitting a class and then reaching back into both halves.

Steps 12 and 13 are one concern in two commits and 13 comes first: the metric collision is latent
until something drives `Immediate`, so porting a consumer onto it is what makes the split
demonstrable rather than theoretical.

Step 14 is last because it documents the result.

## Steps

### Step 1 — Delete `Overlay`, `Navigation`, and the five empty components

**Open.** In [`api/ui/`](../../api/ui/) and [`CMakeLists.txt`](../../api/ui/CMakeLists.txt).

`Overlay.{h,cpp}`, `Navigation.h` with `Menu::navigate()`, `component/{Dialog,InputBox,Spinner,
TextBox,ToolTip}.h` and `component/menu/{PopupMenu,RadialMenu}.h`, and their entries in
[`component/Type.h`](../../api/ui/component/Type.h).

None of them has a consumer, `Overlay`'s premise is contradicted by `Color.h`, and
`Menu::navigate()` documents behaviour it does not have. They cost nothing to reintroduce when
something asks — `git show` recovers a header — and today they misrepresent what the library
does, which is what makes the widget set look twice its real size to a reader and to the enum.

Keep the [TODO.md](../TODO.md) entry that records the intent behind `TextBox` and friends; it is
the part worth surviving, and it already names the thing a text box actually needs (nothing
routes a key to a focused component).

Take the chance to tidy [`CMakeLists.txt`](../../api/ui/CMakeLists.txt) in the same commit: it is
space-indented for eleven entries and then flush left, has `style/Button.h` twenty lines from
`style/Button.cpp`, and jams four files onto its closing line. It is the file where a missing
source is a link error rather than a compile one.

Drop `v3dlib_ui` from [`odyssey/CMakeLists.txt`](../../odyssey/CMakeLists.txt) in the same
commit. It links the library and includes nothing from it, which is the linking rule in
[Build.md](../Build.md#linking-rules) read backwards — an app names the targets it uses, and
this one does not use this.

**Pure subtraction.** Nothing that draws today changes.

### Step 2 — `plateBox` stops filling under the interior

**Open.** In [`api/ui/Painter.cpp`](../../api/ui/Painter.cpp).

Draw the border as bands around the interior rather than as a full box behind it, so that a
`panel` colour with alpha lets the scene through as `Color.h` says it should, and
`Immediate::window(alpha)` starts meaning something.

The rounded case is the reason it was written this way — an outline that traces a rounded corner
is more than four rectangles. `fillBox` already builds a rounded box out of three bands and four
wedges, and a rounded *ring* is the same construction with an inner radius: four arcs swept
between two radii, and four bands of the border's width. Either extend `Canvas::arc` to take an
inner radius, or accept a mitred approximation at the corners and record why beside it.

**Do this first.** It is the smallest step, it is visibly wrong on screen today, and every later
step that touches drawing wants it already true.

**Tests.** `api/ui/tests/` draws onto a cpu-side `Canvas`, so the assertion is on the primitives:
a bordered box emits no primitive covering the interior. Verify by eye as well — the editor draws
its menu panels over a viewport, which is where an opaque "translucent" panel shows.

### Step 3 — `Immediate` prunes what it stopped drawing, or ADR-0035 says it does not

**Open.** In [`api/ui/Immediate.cpp`](../../api/ui/Immediate.cpp) and
[ADR-0035](../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md).

ADR-0035 records as a consequence that *"the map is cleared of anything not drawn for a frame, so
the cost is bounded."* There is no `erase` anywhere in the file — `state_` is only ever inserted
into through `state_[id]`. A panel generating ids from changing text grows it without limit.

Either implement it — mark each `Retained` with the frame it was last touched in `interact()` and
sweep in `end()` — or amend the ADR to say the map is not pruned and why that is acceptable. The
first is a dozen lines and matches what is already written down, so it is the default; the ADR is
only wrong if the sweep turns out to cost more than the growth.

Per [sdlc.md](../sdlc.md) §5, the ADR gets the correction either way. Superseding does not delete,
and neither does correcting: leave the consequence and note what was actually built.

**Tests.** `ImmediateTest.cpp` already drives frames without a device. Draw a window with a new
id each frame for a hundred frames and assert the map is bounded.

### Step 4 — `Component`'s construction

**Open.** In [`api/ui/Component.{h,cpp}`](../../api/ui/Component.h).

Reorder the initialiser list to match the declaration order, and make `lastID` a private
implementation detail rather than a public mutable global — a function-local counter in the
constructor's translation unit, or an atomic if it stays a member.

Neither is a bug today. Both are the kind of thing that becomes one silently: an initialiser that
reads another member, or a second thread building a component. The id being untestable is the
present-day cost.

**Additive.** No behaviour changes.

### Step 5 — Namespaces mirror the path, and the three `Style`s get distinct names

**Open.** Across [`api/ui/`](../../api/ui/) and every consumer.

`v3d::ui::menu` becomes `v3d::ui::component::menu` and `v3d::ui::style::prop` becomes
`v3d::ui::style::property`, per [Conventions.md](../Conventions.md). `ComponentRenderer::Style`
becomes `ComponentRenderer::Dressing` — the struct of colours and metrics a component is drawn
with, which is what step 7 caches — leaving `v3d::ui::Style` as the only `Style` in the library.

Restyle `component::Type`'s enumerators to house style in the same commit, now that step 1 has
removed the ones with no loader: `Type::Menu`, `Type::Button`, `Type::HorizontalBox`. Enum class
scoping keeps `Type::Menu` unambiguous against `component::Menu`, and the explicit numbering can
go with the `TYPE_` prefix — nothing serializes the enum, since the config names types as strings
in [`Engine::buildComponent`](../../api/ui/Engine.cpp).

**This is the noisiest diff in the plan and the least interesting**, so land it while nothing else
is in flight. It touches four apps only where they name a menu type.

### Step 6 — `Measure` and `Write` get a header, and `ComponentRenderer.h` forward-declares

**Open.** New `api/ui/Text.h`, plus [`ComponentRenderer.h`](../../api/ui/ComponentRenderer.h),
[`Immediate.h`](../../api/ui/Immediate.h) and [`TextRenderer.h`](../../api/ui/TextRenderer.h).

One pair of typedefs in `v3d::ui`, taken by both consumers, so that
`TextRenderer::measure()` returns the type `Immediate` accepts rather than a different type that
happens to have the same signature.

Then `ComponentRenderer.h` forward-declares `Engine`, `Container` and the fourteen component
types it names in signatures, and drops the includes. Nothing in its header needs a definition —
every use is behind a `shared_ptr` or a reference — and the `.cpp` includes what it actually
touches.

The win is off every app's build: `TextRenderer.h` stops dragging `Engine.h`, and `Engine.h`
stops dragging `boost::json` and EnTT into four app renderers that only wanted to draw a string.

**Additive.** Consumers that relied on the transitive includes get an error naming exactly what to
add, and there are four of them.

### Step 7 — A `Dressing` resolver that caches, and `Measure` takes a `string_view`

**Open.** New `api/ui/style/Resolver.{h,cpp}`, plus
[`ComponentRenderer.cpp`](../../api/ui/ComponentRenderer.cpp),
[`Immediate.cpp`](../../api/ui/Immediate.cpp), [`Style.h`](../../api/ui/Style.h) and
[`Text.h`](../../api/ui/Text.h).

The single highest-value change in the plan. A resolver holds the active theme and a
`map<pair<class, name>, Dressing>`; `Dressing` is the plain struct of colours and metrics each
`draw()` currently rebuilds from scratch. The map is cleared when `theme()` is set, which is the
only thing that can invalidate it. That removes the vector, the string allocations, the map
lookups and the RTTI from the per-frame path in one move, and replaces `lookup()` plus four to
seven `readColour`/`readMetric` calls with one hash.

In the same commit, because they are the same path:

- `Measure` takes `std::string_view`, deleting the three `std::string(...)` allocations in
  `ComponentRenderer` and whatever `Immediate` has. `TextRenderer::width()` widens to
  `wchar_t` internally already, so the signature change stops at the boundary.
- `SelectList` caches its widest row, invalidated in `items()` — its one mutator — so
  `natural()` stops measuring N strings per frame to compute a number that did not change.
- `Container::ordered()` and the free `ordered()` return without allocating when nothing has a
  non-zero depth, which is every container in the tree today.

`Immediate` takes the same resolver, which is what deletes its inline copy of `lookup()`.

**Measure before and after.** This is a performance step and the tree has no benchmark; a frame
time from the editor with its menu bar and toolbars up, before and after, is the evidence. It is
also the step most likely to be *not worth it* at present component counts — say so in the commit
if the numbers say so, because the allocation removal stands on its own and the cache may not.

**Tests.** A resolver is pure logic given a theme: assert that a second lookup of the same
(class, name) returns the same `Dressing`, that `theme()` invalidates, and that an unnamed style
falls back to the first of its class as `lookup()` does today.

### Step 8 — The strip arithmetic has one implementation

**Closed, and narrower than it was drafted.** `stack()` places the strips and `insets()` asks
it what they took, so the rule exists once. That closed a defect neither half had been blamed
for: `draw()` advanced past a left toolbar by the box it was *last* drawn in, which is nothing
until it has been drawn once, so two left strips were placed on top of each other on the first
frame - while `insets()`, advancing by what the strip would be drawn at, reported the right
answer and disagreed. `draw()` also walked `ordered()` where `insets()` walked `components()`,
so a depth would have made them disagree again.

**The class split was not done, and should not be.** Layout and paint in this library are
mutually recursive by design: the walk resolves a box and paints it, and painting a tab bar
walks the page it holds. Splitting them means a callback through the hottest walk in the
library to buy a smaller file - and the specific defect the split was meant to close, the two
implementations of the strip rule, is closed without it. `natural()` and `arrange()` stay
where the walk that calls them is. Reopen it if a test ever needs layout without paint, which
is the one benefit that would have been real; nothing has asked.

In [`ComponentRenderer.{h,cpp}`](../../api/ui/ComponentRenderer.h).

`natural()`, `arrange()`, `walk()` and `insets()` become a class that resolves boxes and writes
them onto components, and `ComponentRenderer` becomes the paint half that it calls. This does not
move layout out of the draw — [ADR-0019](../adr/0019-the-ui-is-laid-out-by-what-draws-it.md)
stands, and one walk still decides both what is drawn and what is clicked — it moves it out of a
1124-line class that also does theme ingestion, eleven paint routines, strip stacking, nine-slice
skinning and menu panel placement.

The specific defect it closes: `insets()` re-derives the strip arithmetic that `draw(Container&)`
already does, so the rule that a menu bar takes the top and a left toolbar takes what is left has
two implementations that must agree. After the split there is one, and `insets()` asks it.

While in here, consider whether `natural()` should be a virtual on `Component` taking a `Measure`.
Its switch-plus-`dynamic_cast` is one of five places that must be edited in step with each other
to add a widget — the others being the enum, the loader's `if`-chain in
[`buildComponent`](../../api/ui/Engine.cpp), `walk()`'s switch and a `draw()` overload — and none
of the five is checked against the others by the compiler. `natural()` is the one of them that
needs no canvas and no Vulkan, so it is the one that can become virtual without putting drawing
knowledge into a component. **Do not do the same to `paint()`** — keeping the renderer out of the
components is deliberate.

**Additive.** A pure move; every app draws identically.

**Tests.** `LayoutTest.cpp` and `ComponentRendererTest.cpp` already cover both halves and should
need no change, which is the check that the move was a move.

### Step 9 — ADR-0038, who turns a cursor into a command

**Open.** [adr/](../adr/), per [sdlc.md](../sdlc.md) — the record comes first.

Today three answers coexist: `Toolbar` dispatches its own button's event, `MenuBar` dispatches its
own item's, and a component in a `Container` has no one to dispatch for it, so `pick()` returns a
`shared_ptr<Component>` and the app is left holding it. Meanwhile the editor writes the ordering
rule between the three by hand.

What it has to settle:

- **Whether a `Container` gains a dispatcher and a `press()`/`motion()` pair, symmetric with
  `Toolbar`, or whether a separate router takes an `Engine` and offers the cursor to everything in
  the right order.** The second is the one this plan expects: the ordering between a menu panel, a
  toolbar and a container tree is a fact about how the library draws, and it is currently written
  in [`vertical3d/src/Controller.cxx`](../../vertical3d/src/Controller.cxx) — which is the
  signature [ADR-0028](../adr/0028-an-apps-shell-belongs-to-the-api.md) names.
- **What a press on a picked component means**, given that a `CheckBox` "does not own the state it
  shows" per ADR-0019 — the router dispatches the component's event and something else answers by
  setting `checked()`. That rule is already recorded; what is new is who sends the event.
- **What a component with no event does**, and whether a press that lands on a non-dispatching
  component is consumed or falls through to the scene under it. The editor needs the answer:
  a click on a HUD panel must not also rotate the camera.
- **Whether hover is the router's**, since `Toolbar::motion()` writes `ButtonState::Hover` onto
  its buttons and no other component has a hover state at all.

### Step 10 — A container dispatches what it picked

**Open.** In [`api/ui/`](../../api/ui/), as step 9 settles it.

The capability. Whatever shape ADR-0038 chose, the outcome is that a `Button`, `CheckBox`,
`RadioButton` or `SelectList` in a container answers a click, which none of them does today.
`SelectList::at()`, `TabBar::at()` and `Scrollbar::drag()` get their first caller here.

This is what makes the fourteen loadable component types usable, and it is the reason the library
exists rather than an optimisation of it.

**Tests.** The whole path is cpu-side: build a container, draw it onto a `Canvas` to place it,
drive a press at a point, and assert the dispatcher saw the event. `ToolbarTest.cpp` and
`MenuBarTest.cpp` are the pattern.

### Step 11 — The editor stops routing the cursor by hand

**Open.** In [`vertical3d/src/Controller.cxx`](../../vertical3d/src/Controller.cxx).

`uiMotion` and `uiPress` become a call into the router, and the ordering comment moves to where
the ordering now lives. The behaviour change, and a separate commit from the capability.

**Verification is by running the editor.** The menu bar and toolbars should behave exactly as they
do now — an open panel still takes the cursor from a toolbar under it, and a strip the cursor has
left still drops its hover.

### Step 12 — Something drives `Immediate`

**Open.** In [`voxel/src/`](../../voxel/src/) and [`api/ui/StatisticsOverlay.h`](../../api/ui/StatisticsOverlay.h).

`voxel`'s [`DebugOverlay`](../../voxel/src/DebugOverlay.h) — 123 lines producing a build string, a
rolling frame rate and a player position as lines of text — becomes a call into `Immediate` and is
deleted. It is exactly the panel ADR-0035 argued for, and it was written in the same tree in the
same week.

Nothing has driven `Immediate` since it landed, so this is the step that finds out whether the
layer works. Expect to fix things here rather than in the ports that follow.

`StatisticsOverlay` is the second candidate and the harder call: [TODO.md](../TODO.md) already
notes that only pong draws it while three other apps hold the `TextRenderer` it needs, so the
answer might be to give it to the other three rather than to reimplement it. Decide with the
evidence from `DebugOverlay`, and do not do both in one commit.

**Verification is by running voxel**, F3 on and off, against a screenshot of what it draws today.

### Step 13 — The theme's `"ui"` class splits

**Open.** In [`api/ui/`](../../api/ui/), [`vertical3d/data/vgui.json`](../../vertical3d/data/vgui.json)
and [ADR-0035](../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md).

Two style classes — `"ui"` for the retained chrome and something else for the immediate layer's —
so that a theme can set `line-height` for a HUD without setting it for a debug window. The
alternative is one class of colours plus a scale factor per side, which is fewer keys and less
expressive; step 12's experience should decide which.

One data file changes, and only if it grows a metric: the editor's `"ui"` style sets six colours,
all of them equal to the defaults they override, so today it is a no-op restatement. Say so in the
commit — a reader will otherwise assume the theme is load-bearing.

ADR-0035's consequence that *"a theme's 'ui' style dresses both"* gets the correction, alongside
step 3's.

**Tests.** `ThemeTest.cpp` is the home: a theme setting a metric on one class leaves the other's
default alone.

### Step 14 — `docs/UserInterface.md`

**Open.** New [`docs/UserInterface.md`](../UserInterface.md), plus
[`docs/README.md`](../README.md) and [`CLAUDE.md`](../../CLAUDE.md).

Eight ADRs, two paradigms, thirty classes and no owning document —
[`Architecture.md`](../Architecture.md) names `api/ui` once, in passing, and the CLAUDE.md routing
table has rows for the realtime renderer, the offline renderers and the editor but not for this.

What it owns: the two ways to write a ui and which to reach for, the box model and how a `Length`
resolves, what a theme's style classes are and which component reads which, the `Measure`/`Write`
seam and why the library names no font type, and what the draw walk leaves on a component. Cite
the ADRs rather than restating them.

Add the routing row to CLAUDE.md. Half the findings in this plan's Context are things a reader
would otherwise rediscover.

## Verification

Per [sdlc.md](../sdlc.md) §4:

- **Build.** `ninja -C out/build/x64-Debug`. Everything — steps 5, 6, 11 and 12 each reach an app.
- **Tests.** `ctest --test-dir out/build/x64-Debug --output-on-failure`. `api/ui/tests/` already
  has thirteen files and needs no window or device, so every step above except 2's rounded ring
  and 12's port has a cpu-side assertion available. New cases: the resolver cache (7), the
  `Immediate` state sweep (3), a container dispatching a press (10), and the theme class split
  (13).
- **Lint.** cpplint, plus `/W4 /WX`, `/analyze` and clang-tidy. The tree is clean at all four, so
  every finding is this plan's. Step 5 is the one to watch — a namespace move is where an
  unqualified name quietly binds to something else.
- **Run.** Steps 2, 11 and 12 are visual and CI renders nothing. The editor is the test for 2 and
  11 — its menu panels draw over a viewport, which is where an opaque "translucent" panel shows —
  and voxel for 12. Validation layer silent throughout.

## What this plan does not do

**It does not build the missing widgets.** `TextBox`, `Dialog`, `Spinner` and `ToolTip` are
deleted in step 1 rather than implemented. A text box in particular needs something the library
does not have — nothing routes a key to a focused component — and that is a piece of work with its
own shape, not a widget. [TODO.md](../TODO.md) keeps the entry.

**It does not join a scrollbar to a select list.** TODO records that the two side by side is the
app's arithmetic today, and that it is one component waiting to be asked for. Step 10 gives the
list a click, which is the prerequisite; the join is a separate change and no app has asked.

**It does not fix hover being a frame late in `Immediate`.** ADR-0035 records it as a known
consequence of deciding the cursor's owner as the frame is drawn, and it is the mechanism that
lets a window drawn later take the cursor from one under it. Changing it means changing that, and
nothing has complained yet.

**It does not unify the two paradigms.** ADR-0035's Alternative 4 weighed and rejected that, and
nothing found in this review disturbs the reasoning. Step 13 makes them share a theme correctly;
it does not make them one thing.

**It does not touch `Engine`'s loader.** [`Engine.cpp`](../../api/ui/Engine.cpp) is 912 lines and
its `buildComponent` is a fourteen-branch `if`-chain, which is one of the five places that must be
edited together to add a widget. It is also the least costly of the five to get wrong — a
misspelled type name logs an error and loads nothing — and splitting it is a change with no
consumer pressure behind it. Reopen it when a registry has a second reason to exist.

## Open questions

- **Is step 7's cache worth it at the component counts this tree actually draws?** The allocation
  removal is worth it regardless; the map may not be. Measure before committing to it, and be
  willing to land the `string_view` and `natural()` halves alone.
- **What is the rounded ring in step 2?** An inner radius on `Canvas::arc`, which is the correct
  answer and touches `api/render`, or a mitred approximation in `Painter`, which keeps the change
  inside `api/ui`. The second is likely indistinguishable at a one-pixel border and a three-pixel
  radius, which is what everything in the tree draws.
- **Does the router in step 9 belong in `api/ui` or `api/engine`?** It needs an `entt::dispatcher`
  and a cursor, which both libraries have. `api/ui` keeps the ordering rule beside the drawing it
  is a fact about; `api/engine` is where an app's shell already lives per ADR-0028. ADR-0038
  settles it.
- **Should `natural()` become virtual on `Component`?** Raised in step 8 and deliberately left
  open there, because the answer depends on whether step 10 gives components more behaviour of
  their own or less.
