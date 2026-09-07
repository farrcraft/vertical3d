# UI Foundations — Resolution-Independent Text, Input Capture, And Where A Player's Settings Live

Drafted 2026-09-06 against this tree from outside it, staged and **closed** here the same day.
Nine steps across `api/font`, `api/render`, `api/ui` and
`api/engine`. Two of them are defects in apps that ship in this tree today, and one is a
constraint that four apps are each working around separately.

The prompting case is a game being built against the api in another repository, which is
building menus and a settings screen and needs text at more than one size, a key rebinding
screen that can capture a key, and somewhere to write what the player chose. **But most of what
follows is not for that app's benefit.** Pong ships a rebinding menu whose items do nothing when
activated. Pong, tetris, voxel and the editor each hardcode a font size because a glyph cannot be
scaled. `Window::resize()` does not resize a window.

## Context

### One size per atlas, worked around four times

[`ui::TextRenderer`](../../../api/ui/TextRenderer.h) rasterizes a font at a size fixed in its
constructor and packs it into a 512×512 single-channel atlas whose dimensions are hardcoded.
Nothing scales a glyph afterwards — [`Canvas`](../../../api/render/realtime/Canvas.h) has a modelview
stack but exposes only `translate()` — so a second size is a second instance and a second atlas.

Every app that draws text has met this and stopped at one size:

| App | Size | |
|---|---|---|
| **pong** | 28 | [`PongRenderer.cxx`](../../../pong/src/PongRenderer.cxx) — *"Nothing scales a glyph, so this is also the size…"* |
| **tetris** | 22 | [`Renderer.cxx`](../../../tetris/src/Renderer.cxx) |
| **voxel** | 18 | [`Renderer.cxx`](../../../voxel/src/Renderer.cxx) |
| **editor** | 15 | [`Renderer.cxx`](../../../vertical3d/src/Renderer.cxx) — the same comment, worded differently |

Four apps, four constants, the same note written twice. That is the signature of something the
library should be doing.

The consequence is not only that a heading cannot be larger than its body text. The ui lays out in
canvas pixels and glyphs are rasterized at a fixed pixel size, so **the same text is physically
half as large on a 4K display as on 1080p**, and no app in the tree can offer a text size setting
without rebuilding an atlas.

Signed distance field glyphs are the standard answer: one atlas serves every size, and the size
becomes an argument. FreeType has rendered them natively since 2.11 and vcpkg supplies 2.14.3, so
the rasterizer is a render mode at an existing call site — `FT_Glyph_To_Bitmap` in
[`TextureFont.cxx`](../../../api/font/TextureFont.cxx).

What makes it more than a render mode is the shader.
[`quad.frag`](../../../api/render/shaders/quad.frag) is shared by panels, sprites and glyphs, and
[ADR-0005](../../adr/0005-one-batched-quad-primitive.md) is explicit that this costs no branch:
untextured quads sample a 1×1 white texture, and a single-channel atlas reaches alpha through a
swizzled view. A distance field needs a `smoothstep` around its threshold, which would corrupt
every non-text quad if applied unconditionally. So the primitive has to learn which of its batches
are text, and that is a decision ADR-0005 has to be amended for.

### An overflowing atlas reports success

Each glyph is packed with a pixel of padding, so 95 ASCII glyphs fit comfortably around 32 px and
tightly nearer 48. Past that, `TextureFont::loadGlyphs` logs *"Texture atlas is full!"* per glyph,
counts them into a `missed` variable **that is never read**, and returns true. `TextRenderer`
then reports `loaded()`, text draws with characters missing, and `width()` under-measures so the
layout around it is wrong too. The four apps above are all under the ceiling, so this is latent —
and SDF glyphs need padding for their distance range, which raises the floor and makes it
reachable.

### A menu can hold an input item and cannot capture one

`key_input` and `numeric_input` are menu item types the ui config schema knows, `ui::Engine`
loads, and [`component::menu::Menu::activate()`](../../../api/ui/component/menu/Menu.cpp) branches
on — into a comment saying that nothing captures input, so activating one does nothing rather
than dispatching a stale value.

This is not hypothetical. [`pong/data/vgui.json`](../../../pong/data/vgui.json) has four
`key_input` items under Options for rebinding the paddles, and
[`PongEngine::handleUiEvent`](../../../pong/src/PongEngine.cxx) answers all four with a comment
saying they arrive with nothing to bind. **Pong ships a settings screen that silently does
nothing.**

### Nowhere to write, and no way to apply what was written

An app that lets a player rebind a key has nowhere to put the result. The asset manager is rooted
at one directory — the one that ships with the game and is overwritten from source on every build
— and no api code writes a document back out.

`SDL_GetPrefPath` is the answer to the first half and nothing in the tree calls it. It belongs
here rather than in an app: the consuming game's rule is that SDL is the api's to call, and
`api/engine` already names SDL3 as a package where `api/asset` deliberately does not — the
offline renderers depend on that, since `render_offline` names neither SDL nor Vulkan.

The second half is narrower than it looks. A binding override can be applied by an app after
`initialize()`, because [`event::Engine::addMapper`](../../../api/event/Engine.h) stores by name and
a mapper called `global` replaces the one the engine built. A **window size** cannot:
`Window::resize()` records a width and a height without calling `SDL_SetWindowSize`, which is
correct for its only current caller — the loop, recording a resize SDL has already performed —
and leaves no way to ask for one.

## Decisions

Recorded in [adr/](../../adr/), not here.

| ADR | Decision |
|---|---|
| **0036** | Text is a distinct kind of quad, and the primitive carries which — written by step 1, amending [0005](../../adr/0005-one-batched-quad-primitive.md) |
| [0005](../../adr/0005-one-batched-quad-primitive.md) | One batched quad primitive with an optional texture — **amended**, not superseded: one pipeline still draws every 2D thing |
| [0019](../../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) | The ui is laid out by what draws it — unchanged; step 5 keeps `Measure`/`Write` as the seam |
| [0020](../../adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md) | A theme is data and the app resolves its images — unchanged; step 8 is the same division applied to paths |

0036 is the next free number; [adr/README.md](../../adr/README.md) is the authority and `0026` is a
reserved gap rather than an available one. The draft said 0034, which
[0034](../../adr/0034-a-component-has-children-and-a-box.md) and
[0035](../../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md) took while it was staged.

## What blocks what

```
1. ADR-0036 ──> 4. the pipeline knows a text batch ──┐
                                                     ├──> 5. size at the call ──> 6. four apps move
2. glyphs are distance fields ───────────────────────┘

3. an overflowing atlas fails      (independent, and 2 makes it reachable)
7. a menu item captures input      (independent)
8. userPath()                      (independent)
9. a window can be resized         (independent)
```

Steps 7, 8 and 9 touch nothing the text work touches and can land in any order around it. Step 3
should land **before** step 2 rather than after: it is a one-line defect today and a diagnostic
once glyphs start needing more atlas.

Steps 2, 4 and 5 are not independently useful — text is wrong on screen until all three land —
so they are one concern arriving in three commits, and step 6 is the behaviour change to four
apps that follows. **That separation is the point:** steps 2–5 change what the library can do and
step 6 changes what four apps look like, and one concern per commit means they do not travel
together.

## Steps

### Step 1 — ADR-0036, text is a distinct kind of quad

**Closed.** [ADR-0036](../../adr/0036-text-is-a-distinct-kind-of-quad.md). It settled the flag in the
push constant over a second pipeline: the branch is uniform across a draw, and the alternative
multiplies two pipelines into four and pays a bind per frame to avoid it.
[ADR-0005](../../adr/0005-one-batched-quad-primitive.md) carries the amendment note.

The record comes first, per [sdlc.md](../../sdlc.md).

It amends [ADR-0005](../../adr/0005-one-batched-quad-primitive.md) rather than superseding it: one
pipeline still draws every 2D thing, and a glyph is still a quad with a texture. What changes is
the claim that no branch is needed, and that claim is load-bearing enough to be worth a record.

What it has to settle:

- **How a batch says it is text.** A flag in the push constant, or a second pipeline selected per
  batch. [`DrawItem`](../../../api/render/realtime/DrawItem.h) carries a 128-byte push block using 64
  for the projection, and `QuadRenderer::submit` already builds one item per batch, so the room
  and the granularity both exist. A second pipeline is the alternative and has precedent — the
  renderer already builds two and picks by whether the pass has depth — but it doubles to four
  and adds switches per frame to save a branch that is uniform across a draw.
- **That the push range grows to the fragment stage**, which it does not reach today: the quad
  pipeline declares `VK_SHADER_STAGE_VERTEX_BIT` only.
- **What a text batch may not merge with.** `Canvas::open()` extends the current batch when the
  texture matches the last one; text and non-text must not merge even if a handle collides.
- **Why the branch is acceptable**, given ADR-0005 chose the white-texture trick specifically to
  avoid one.

### Step 2 — Glyphs are distance fields, and the atlas is sized by its caller

**Closed.** `spread` is a `TextureFont` constructor argument, zero meaning coverage as before, and
`FT_RENDER_MODE_SDF` is reached through `FT_Render_Glyph` rather than the load flags - a distance
field is built from the outline, so `FT_LOAD_RENDER` had to stop rendering one first. The atlas
dimensions, the base size and the spread are all `ui::TextRenderer` constructor arguments.

In [`api/font/TextureFont.cxx`](../../../api/font/TextureFont.cxx) and
[`api/ui/TextRenderer.h`](../../../api/ui/TextRenderer.h).

`FT_Glyph_To_Bitmap` gains `FT_RENDER_MODE_SDF` for the single-channel case. FreeType's SDF
renderer is documented as slow; that is irrelevant for ~95 glyphs once at startup, and the `bsdf`
path from an existing bitmap is available if it is not.

An SDF glyph needs padding for its distance range where packing adds one pixel today, so each
glyph grows by twice the spread in both axes. **That makes 512×512 too small at a useful base
size, so the atlas dimensions stop being hardcoded and become constructor parameters** with the
current values as defaults. Base size and spread are the two knobs; both belong in the signature
rather than in the body.

**Tests.** `api/font/tests/` and `api/ui/tests/` both exist. Packing is pure logic given a font
file: assert that a known charset fits a given atlas at a given size and spread, and that the
glyph metrics come back in atlas units.

### Step 3 — An overflowing atlas is a failure, not a log line

**Closed.** `loadGlyphs` returns false when anything was missed and logs the count once rather than
per glyph. It also fixed a second symptom nothing had named: `TextureFont::glyph()` calls
`loadGlyphs` for one charcode and returned `glyphs_.back()` on true, so a glyph that would not fit
came back as whichever glyph had been packed last.

In [`api/font/TextureFont.cxx`](../../../api/font/TextureFont.cxx).

`missed` is incremented and never read. It should reach the caller — `loadGlyphs` returning false,
or the count — so that `TextRenderer::loaded()` can stop reporting success for a font whose glyphs
are partly absent.

Smallest step in the plan and the one most worth doing first: it is the diagnostic every later
step wants when the atlas budget changes underneath it.

**Tests.** An atlas deliberately too small for the charset, asserting the failure is reported.

### Step 4 — The quad pipeline knows a text batch

**Closed.** Landed with step 2 rather than before it. It could not be additive as drafted: the same
step both sets the flag in `Canvas::text()` and makes the shader act on it, so a commit with only
one half draws every glyph wrong. Steps 2 and 4 together move text from correct to correct.

In [`api/render/realtime/Canvas.h`](../../../api/render/realtime/Canvas.h),
[`vulkan/QuadRenderer.cxx`](../../../api/render/realtime/vulkan/QuadRenderer.cxx) and
[`shaders/quad.frag`](../../../api/render/shaders/quad.frag), as step 1 settled it.

`Canvas::Batch` carries the flag, `Canvas::text()` sets it, `open()` refuses to merge across it,
`submit()` writes it into the push block, and the fragment shader branches — sampling as it does
today, or taking the distance and `smoothstep`ing it against a width derived from the screen-space
derivative so that a glyph is crisp at any scale.

**Additive.** Nothing sets the flag until step 5, so every app draws identically the moment this
lands.

### Step 5 — `TextRenderer` takes its size at the call

**Closed.** The size defaults to the base rather than being required, so no call site broke - which
left step 6 free to be what it should be, four apps choosing a size, rather than four apps being
repaired. The ratio is computed in `TextureTextBuffer::addCharacter` from `markup.size_` against
`font->size()`, both of which it already held, so no field was added to carry it.

In [`api/ui/TextRenderer.h`](../../../api/ui/TextRenderer.h).

One atlas at one base size, and `draw()` and `width()` take the size they are wanted at.
`glyph->advance_`, and the width, height and offset that
[`TextureTextBuffer::addText`](../../../api/font/TextureTextBuffer.h) positions from, become ratios
of requested size to base size.

`measure()` and `write()` keep their shape — per
[ADR-0019](../../adr/0019-the-ui-is-laid-out-by-what-draws-it.md) `ComponentRenderer` names no font
type, and it should stay that way — so they close over the size the caller wants. An app drawing
a ui at one size and a heading at another asks for two callback pairs from one `TextRenderer`.

The pair has a second consumer since this was drafted:
[`ui::Immediate`](../../../api/ui/Immediate.h) takes the same two callbacks per
[ADR-0035](../../adr/0035-an-immediate-mode-layer-over-the-same-canvas.md). That is an argument
for the seam rather than against it — both consumers take a size that is already closed over,
and neither learns a font type — but it is a third and fourth call site for step 6 to find.

**This breaks four call sites**, which is what step 6 is.

### Step 6 — Pong, tetris, voxel and the editor move onto it

**Closed.** Each app constructs its `TextRenderer` at the default base and passes its own size to
`measure()`, `write()` and `draw()`. The editor gained the `uiScale` the plan predicted, and
`ui::StatisticsOverlay` gained a size of its own - it had been drawing at `text_->size()`, which
used to be the drawn size and is now the base.

The behaviour change, and a separate commit from the capability.

Each app's `fontSize` constant stops being the size the atlas was built at and becomes the size it
asks for. The editor gains the most: its `style.lineHeight`, `padding`, `barHeight` and
`panelPadding` are all multiples of `fontSize`, so a ui scale becomes one number.

**Verification is visual and per app.** Text should look identical at the size each app uses
today, which is the check that the metrics ratio is right.

### Step 7 — A menu item can capture input

**Closed.** `Menu::capture()` fills the item and `activate()` both opens and closes a capture.
A `KeyInput` ends at the first value rather than waiting for the second activation the plan
described, because a binding is one key; the other two input types wait as drafted. Navigation is
inert while a capture is open and `up()` abandons it.

Making pong's four items *take effect* needed one thing the plan did not name:
`v3d::engine::Engine::rebind()`, because `event::Mapper` cannot be read back and so the global
mapper has to be rebuilt from config with the overrides applied.

In [`api/ui/component/menu/Menu.cpp`](../../../api/ui/component/menu/Menu.cpp) and
[`api/ui/Engine.h`](../../../api/ui/Engine.h).

Activating an `Input`, `NumericInput` or `KeyInput` item puts the menu into capture: subsequent
source events go to the item rather than to navigation, and the next activation dispatches the
item's command carrying what was captured as the event's data. The comment in `activate()`
describes this design already; it is a description of what to build.

The captured value for a `KeyInput` is a key **name**, per
[`api/input/Keyboard.cpp`](../../../api/input/Keyboard.cpp)'s table, because that is what a binding
document holds and what makes the result writable.

**Pong is the proof.** Its four Options items should bind a paddle key and take effect, which
they do not today.

### Step 8 — `userPath(org, app)`

**Closed.** `SDL_GetPrefPath` needs no `SDL_Init` - confirmed against the SDL3 header, which also
guarantees the trailing separator `appPath()` promises and creates the directory. A null return is
an empty string and a log line.

In [`api/engine/Application.h`](../../../api/engine/Application.h), beside `appPath()`.

The symmetric question to the one that header already answers. `appPath(argv[0])` says where an
app reads its assets from; `userPath(org, app)` says where it writes the player's files, wrapping
`SDL_GetPrefPath` and the `SDL_free` its result needs.

A null return is an empty path and a log line, not a throw: a game that cannot find a settings
directory should run with its defaults. Confirm during this step whether the call is safe before
`SDL_Init`, since the natural call site is early.

Nothing else is needed to read or write there — `asset::Manager` takes its root as a constructor
argument, so a second manager on this path reads through the same `asset::Json` loader.

`run<T>(executable, name)` is already handed an app name and currently spends it on one log line,
which is worth noticing but not worth changing here: an app that wants a user path can ask for
one, and threading it through the engine is a change with no second caller yet.

### Step 9 — A window can be resized after it exists

**Closed.** `Window::request()` alongside `Window::resize()`. It deliberately does not write the
recorded size: SDL answers with an event the loop already turns into a `resize()` call, so a size
the window manager refused is never reported as one the window has.

In [`api/render/realtime/Window.h`](../../../api/render/realtime/Window.h).

`resize(width, height)` records a size. Its only caller is the loop, telling the window what SDL
has already done, so it is correct — and it means there is no way to *ask* for a size, which is
what an app restoring a remembered window size needs.

The fix is a second method that calls `SDL_SetWindowSize` and lets the resulting event update the
recorded size through the existing path, rather than making `resize()` do both. Two verbs, because
one of them is an answer and the other is a request.

## Verification

Done, on 2026-09-06. Build and `ctest` clean at 24 of 24, cpplint clean over the tree, and all
four apps run with the validation layer on and silent. Text was checked by eye in pong, tetris and
the editor at each app's own size, and the editor was run at `uiScale` 2 to see a glyph scaled up
from the same atlas - which is what says the distance field is doing its job rather than the size
merely being passed around.

New cases: `api/font/tests/TextureFontCacheTest.cxx` for the overflow of step 3 and the packing
budget of step 2, a new `api/font/tests/TextureTextBufferTest.cxx` for the size ratio of step 5,
`api/ui/tests/GameMenuTest.cpp` for the capture of step 7, and
`api/engine/tests/ApplicationTest.cpp` for step 8.

Per [sdlc.md](../../sdlc.md) §4:

- **Build.** `ninja -C out/build/x64-Debug`. Everything, since step 6 touches four apps.
- **Tests.** `ctest --test-dir out/build/x64-Debug --output-on-failure`. New cases in
  `api/font/tests` (packing, and the overflow of step 3) and `api/ui/tests` (size-relative metrics,
  and menu capture). The shader is not testable here; step 4 is verified by running.
- **Lint.** cpplint, plus `/W4 /WX`, `/analyze` and clang-tidy. The tree is clean at all four.
- **Run.** Text is what this plan is about and CI renders nothing, so the four apps are the test.
  Each should look identical to today at its current size, then visibly correct at another —
  the editor at twice its ui scale is the sharpest check that SDF is doing its job. Pong is the
  proof for step 7. Validation layer silent throughout.

## What this plan does not do

**It does not write a config document.** An app can read a user file through a second
`asset::Manager` and write one with `boost::json`, which is what the prompting game will do.
Serializing config back out is plausibly the api's job eventually, but there is one consumer
today, and [ADR-0016](../../adr/0016-undo-records-what-has-already-happened.md) and
[ADR-0017](../../adr/0017-a-command-is-a-name-in-a-context.md) both settle that one consumer is not
a library. It moves when a second app wants it.

**It does not promote `CommandDirectory` to `api/event`.** ADR-0017's Alternative 3 named the
trigger — *"It moves when a second app wants it"* — and a game with menus and bindings naming the
same commands is about to be that app. But it is not one yet, and this plan is already nine steps.
Reopen it on its own.

**It does not build the missing ui components.** `Dialog`, `InputBox`, `TextBox`, `CheckBox`,
`RadioButton`, `Scrollbar`, `SelectList`, `Spinner`, `TabBar` and `ToolTip` are headers with no
loader and no renderer. Step 7 makes the *menu* input types work, which is what a rebinding screen
needs; a settings screen wanting a checkbox is a separate piece of work with its own shape.

**It does not add multi-channel SDF.** Single-channel distance fields soften sharp corners at
large scales. msdfgen would fix that and is a new dependency for a problem no app in the tree has
at the sizes it draws.

**It does not change how text is laid out.** No shaping, no kerning beyond what the font metrics
give, no bidirectional text, and the `char`-to-`wchar_t` widening in `TextRenderer::draw` still
means the charset is effectively Latin-1. Those matter for localization and none of them is what
this plan is for.

## Open questions

- **Base size and spread.** ~~What single base size serves 12 px to 200 px acceptably?~~
  **48 and 8**, picked by packing printable ascii at each of 32, 48 and 64 against spreads of 4, 8
  and 12 and seeing what fit. Recorded beside the constants in
  [`TextRenderer.cpp`](../../../api/ui/TextRenderer.cpp).
- **Does the atlas still fit at that base size?** ~~It should not have to.~~ **It does, and the
  default stayed 512.** It is close, though: 48 with a spread of 12 overflows it, and so does 64
  with a spread of 8. Both halves are asserted in `texturefont_distance_field_packing_test`, so the
  margin is a thing a change trips over rather than a thing it discovers in a screenshot.
- **Whether `Canvas` should expose a scale** on its modelview stack, independently of any of this.
  It has `push`, `pop` and `translate` and no scale. Still open, and still not this plan's — the
  glyph metrics scale on the cpu in `TextureTextBuffer::addCharacter`, which is the layout's own
  business, so nothing here needed the transform to do it. Carried to
  [TODO.md](../../TODO.md).
