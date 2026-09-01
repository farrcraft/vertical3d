# Luxa Audit

Phase 1 of [plans/Modernization.md](plans/Modernization.md). Audited against the tree on
2026-08-31.

`luxa/` is a migration in progress, not dead code. This is the functional equivalence
record that has to close before the tree can be deleted: every piece still sitting in
`luxa/`, where it landed in `api/ui` (or did not), and what has to be built before nothing
is lost.

**Verdict up front: `luxa/` cannot be deleted yet.** Two pieces have no equivalent and one
is a stub. Everything else has either landed or turned out to be worth nothing.

## Method

`luxa/` is 1,952 lines across 20 files. All of it was read, and the pieces already migrated
were diffed against their `api/ui` counterparts to catch behaviour dropped in transit — the
audit question is not only "does a class of that name exist in api" but "does it still do
what it did".

Files deleted from `luxa/` in earlier commits were recovered with `git show` to check what
migrated and what was simply dropped. The relevant commits are `274886b`, `6636d57`,
`49fe874`, `d31a2e9` and `badf496`.

## What is left in the tree

```
luxa/luxa/Component.{h,cxx}          luxa/luxa/Button.{h,cxx}
luxa/luxa/ComponentManager.{h,cxx}   luxa/luxa/Icon.{h,cxx}
luxa/luxa/ComponentRenderer.{h,cxx}  luxa/luxa/Label.{h,cxx}
luxa/luxa/UILoader.{h,cxx}           luxa/luxa/Window.h
luxa/luxa/menu/MenuStack.{h,cxx}     luxa/uitest/*
```

The tree does not compile and has not for some time. `ComponentManager.h` and `.cxx`
disagree on the constructor signature, `Component.h` references `Theme`,
`v3d::command::CommandInfo` and `v3d::event::EventInfo` without including anything that
declares them, and half the includes (`style/Theme.h`, `menu/Menu.h`,
`v3dlibs/gl/GLFontRenderer.h`, `v3dlibs/hookah/Window.h`) point at files that no longer
exist. `luxa/luxa/Icon.h` has already been half-converted in place — it declares
`namespace v3d::ui::component` while its `.cxx` still says `namespace Luxa`. Treat the tree
as a reference text, not as buildable code.

## Component by component

| luxa | api equivalent | Verdict |
|---|---|---|
| `Component` | `v3d::ui::Component` | Covered for state; `draw`, `exec`, `notify` dropped |
| `Button` | `ui::component::Button` | Covered for state; `draw` and `notify` dropped |
| `Label` | `ui::component::Label` | Covered for state; `draw` dropped |
| `Icon` | `ui::component::Icon` | Covered for state; `draw` dropped |
| `Window` | — | Nothing to migrate (see below) |
| `menu/MenuStack` | — | Navigation already migrated; `draw` has no equivalent |
| `ComponentManager` | `ui::Engine` + `ui::Container` | **Partial** — input, themes, textures, images unported |
| `ComponentRenderer` | — | **Missing** — must be rebuilt, not ported |
| `UILoader` | `ui::Engine::load` | **Partial** — menus only; styles, fonts, components unported |
| `uitest/` | — | No equivalent; nothing depends on it |

Migrated before this audit and confirmed complete: `Menu`, `MenuItem`, `Theme`, `Style`,
`StyleProperty` (→ `style::Property`), `ButtonStyle`, `MenuStyle`, `ColorStyleProperty`,
`FontStyleProperty`, `ImageStyleProperty`, `Overlay`, `Container`, and the dozen component
stubs (`Dialog`, `Frame`, `CheckBox`, `InputBox`, `RadioButton`, `Scrollbar`, `SelectList`,
`Spinner`, `TabBar`, `TabPage`, `TextBox`, `ToolTip`, `HorizontalBox`, `VerticalBox`,
`MenuBar`, `PopupMenu`, `RadialMenu`). The stubs were empty in luxa and are empty in `api`,
so "migrated" means the empty class moved; `api/ui/component/HorizontalBox.h` is now
literally a `#pragma once`.

## The three things with no equivalent

### 1. `ComponentRenderer` — must be rebuilt on Vulkan, not ported

208 lines of immediate-mode OpenGL: `glOrtho`, `glPushMatrix`, `glPushAttrib`,
`glTexEnvi`, `glTranslatef`, `glColor3f`. None of it survives
[ADR-0001](adr/0001-vulkan-replaces-opengl.md), and it is already partly hollowed —
`drawTexturedQuad` and `drawTexture` have had their bodies removed with comments pointing
at `operation::GLTexturedQuad` and `operation::GLTexture`, and `prepare`'s background clear
is commented out in favour of `operation::Overlay`.

What it actually provides, stripped of the GL:

- **an orthographic UI pass** — `prepare`/`post` set up a top-left-origin ortho projection,
  disable depth and lighting, and enable alpha blending, then restore state. This is
  the "sprite/orthographic pass" Phase 5 already schedules; the UI is another client of it.
- **a matrix stack** — `push`/`pop`/`position`/`clear`, used by `Button::draw` to place
  its nine textured quads in component-local coordinates. Under
  [ADR-0004](adr/0004-operations-as-draw-data.md) this becomes a transform on the draw item,
  not renderer state.
- **textured quad and texture drawing** — already superseded by the batched quad primitive
  of [ADR-0005](adr/0005-one-batched-quad-primitive.md).
- **`getDefaultFont(style_class, theme)`** — resolves a `Font2D` by pulling the `label`/`font`
  property out of the theme's default style set for a class. This is theme-to-font
  resolution, it is genuinely useful, and it has no home in `api/ui` today. Note it targets
  `v3d::font::FontCache`, which no longer exists — the api equivalent is
  `v3d::font::TextureFontCache`.
- **`width`/`height`/`resize`** — the managed UI area, which nothing in `api/ui` tracks.

**Action:** do not port. Build UI drawing as operations against the Phase 3 batched quad,
and carry over only `getDefaultFont`'s resolution logic and the managed-area dimensions.
This work belongs in Phase 3 alongside re-wiring pong's UI, not in Phase 1.

### 2. `ComponentManager` — the half that is not `Container`

`ui::Engine` + `ui::Container` cover the component registry, name lookup and visibility.
The rest of `ComponentManager` has no equivalent anywhere:

- **Mouse hit-testing and hover/focus tracking.** `intersect(point)` walks components by
  z-index and returns the topmost hit; `motion` synthesises `enter`/`leave` on the component
  under the cursor; `buttonPressed`/`buttonReleased` forward to it. `v3d::ui::Component`
  still carries `bound()` and `depth()`, and **nothing in the entire tree reads either** —
  they exist solely as the input to this algorithm. Nothing else, including
  `api/input` and `api/event`, does UI hit-testing.
- ~~**The theme registry and active theme.**~~ **Covered 2026-08-31** — `ui::Engine` gained
  `theme(name)`, `activeTheme()` and `activeTheme(name)`. Which theme starts active is still
  a guess (the first one loaded), because the config has no field to say.
- **The texture registry.** `addTexture`, keyed by `style-name + property-name + source`.
- **Image loading.** `loadImage` via `image::Factory`. The api replacement should be
  `asset::Manager`, in line with the same change Phase 4 makes for tetris.
- **`resize`.** Forwards the window canvas size to the UI area.
- **`toggleComponentVisibility`.** Trivial, but the only thing that reads
  `Component::visible` on the luxa side.

Two of these carry bugs worth *not* reproducing: `intersect` requires `component` to be
non-null before it will ever assign to it, so it returns null unconditionally; and
`buttonPressed`/`buttonReleased` dereference its result without a null check.

**Action:** hit-testing and hover/focus have to be designed into `api/ui` before luxa goes,
and they need `api/input/Mouse::handleEvent` implemented first — it currently swallows every
mouse event, see [V3dlibsAudit.md](V3dlibsAudit.md). Texture and image registries should
resolve to `asset::Manager` rather than being ported.

### 3. `UILoader` — `ui::Engine::load` covers menus and nothing else

`ui::Engine::load` reads themes (names only) and containers, and dispatches on component
type — with exactly one arm, `"menu"`. `UILoader` loaded, from the same conceptual
document:

| `UILoader` | In `ui::Engine::load`? |
|---|---|
| `loadMenu` / `loadMenuItem` | Yes — the ptree→JSON port is complete and correct |
| `loadTheme` | Name only. Styles inside the theme are not read |
| `loadStyle` | No. Button/menu/label style classes and button state parsing are gone |
| `loadStyleProperty` | No. Image and font properties, and alignment parsing, are gone |
| `loadTextures` | No. The pass that walks image properties and loads their textures is gone |
| `loadFont` | No. Named font faces and sizes are never loaded |
| `loadButton` | No |
| `loadLabel` | No |
| `loadIcon` | No |
| `loadDefaultComponentAttributes` | No. position, style, name, visible are never read for a component |

`api/ui/style/` — `Theme`, `Style`, `Property`, `property/{Color,Font,Image}`,
`style/Button`, `style/Menu` — is fully migrated and fully unreachable: nothing constructs
a `Style` anywhere in the tree. `pong/data/vgui.json` reflects this, containing one
name-only theme and one menu.

**Action:** the JSON schema for styles, style properties, fonts and non-menu components has
to be designed and `Engine::load` extended. Until then `api/ui/style/` is dead weight that
looks live.

## What `Window` and `MenuStack` actually are

The plan's starting list flagged both as open questions. Both resolve smaller than expected:

- **`Window` is an empty stub.** `Window.h` is 18 lines declaring a constructor and
  destructor on a `Component` subclass, and there is no `Window.cxx`. There is nothing to
  compare against `ui::Container` and nothing to migrate. Delete it with the tree.
- **`MenuStack`'s navigation was never in `MenuStack`.** It lives in `Menu` — `next`,
  `previous`, `up`, `down`, `level`, `parent`, `active`, `activate` — and `Menu` migrated
  intact to `ui::component::Menu`, which additionally gained `navigate(Navigation, bool)`
  and `hasParent()`. `MenuStack` is a 45-line `draw()` and nothing else: it lays the active
  menu level out vertically, centres it, and prints a `" > "` marker beside the active item.
  That is the only unported thing in it, it is a layout-and-draw routine, and it belongs
  with the `ComponentRenderer` rebuild. Its loop is also written
  `for (size_t i = menu_count; i >= 0; i--)`, which never terminates — do not port it line
  by line.

So `ui::Navigation` needs no reconciliation against `MenuStack`. It is a superset of what
already worked.

## Behaviour lost in the pieces that already migrated

These are regressions in `api/ui` today, not things still sitting in `luxa/`. They are the
reason "the class exists in api" was not a sufficient test.

1. ~~**`Menu::activate()` dispatches nothing.**~~ **Fixed 2026-08-31.** `Menu` now takes an
   `entt::dispatcher` and sends the active item's bound event. The gap was worse than this
   audit first recorded: `Engine::loadMenu` never set `level_`, never set `active_`, and
   never set each item's owning menu, so `next()` returned false immediately, `active()`
   returned null, and `up()` could never find a parent — **the menu did not navigate
   either**. `next()`/`previous()` had also mistranslated the original's iterator
   wrap-around into `int`, running past the end of `items_` and decrementing to `-2`; and
   `active(int)` assigned an out-of-range index after rejecting it. `activate()` and
   `down()` now act on the current level rather than on the top-level menu, which is what
   made submenus work.
2. ~~**`MenuItem::param()` is gone.**~~ **Fixed 2026-08-31.** `MenuItem` carries a
   `v3d::event::EventData` value with `value()` accessors, `text()` appends it to the label
   for display, and `Menu::activate()` sends it as the event's data. The class comment now
   describes what is implemented. A binding's optional `"param"` reaches a handler through
   that same field, which is the joint fix the [v3dlibs audit](V3dlibsAudit.md) called for.
3. **Input capture for input-type menu items does not exist.** The old design terminated
   capture on the `ui::selectMenu` binding. `Menu::activate` documents this in a comment and
   implements nothing, so `setMaxScore` and the four key-binding menu items in
   `pong/data/vgui.json` are unreachable. The value field they would write to now exists;
   the capture does not.
4. **`Component::exec` and `Component::notify` are gone**, and with them `Button::notify` —
   the `enter`/`leave`/`button_pressed`/`button_released` → `STATE_HOVER`/`STATE_NORMAL`/
   `STATE_PRESS` transitions. `ui::component::Button` keeps `state()` and the `ButtonState`
   enum with nothing to drive them. The replacement is presumably the entt dispatcher, but
   no component subscribes to anything today.
5. ~~**`api/ui` still depends on OpenGL.**~~ Fixed 2026-09-01. `component/Icon.h` and
   `style/property/Image.h` now hold a `render::realtime::TextureHandle` instead of a
   `boost::shared_ptr<v3d::gl::GLTexture>`, which cost `v3dlib_ui` no new dependency - it
   already links `v3dlib_render` for `ComponentRenderer`'s canvas. **Nothing sets either
   handle yet**: uploading a theme's image through `QuadRenderer::texture` and drawing an
   `Icon` from it is still item 6 below.

## Salvage

- **`luxa/tests/` was deleted, not migrated.** `ButtonTest.cxx`, `ComponentManagerTest.cxx`
  and `TestMain.cxx` went in `d31a2e9`. The modernization plan records `v3dlibs/tests/` as
  the only test corpus in the repo; that is true of the working tree but not of the history.
  Recover them with `git show d31a2e9^:luxa/tests/ButtonTest.cxx` before writing `api/ui`
  tests from scratch.
- **`luxa/uitest/`** is a small harness that opened a window and exercised the widget set,
  with a TODO list of the widgets it wanted to cover — which is a fair description of the
  `api/ui` component stubs. Nothing depends on it and it references four headers that no
  longer exist. It is worth reading once for the widget checklist, then deleting.

## What has to happen before `luxa/` can be deleted

Ordered by when it can be done, not by size. Nothing in this list is blocked by Vulkan
except where noted.

1. ~~Wire `Menu::activate()` to the event dispatcher so action items fire their bound event.~~
   **Done 2026-08-31**, along with the menu navigation state it depended on.
2. ~~Restore a value field on `MenuItem` for input-type items.~~ **Done 2026-08-31.**
3. Design the JSON schema for themes, styles, style properties and fonts, and extend
   `ui::Engine::load` past the single `"menu"` arm. Until this lands, `api/ui/style/` is
   unreachable. (Phase 1 or 3.)
4. ~~Add a theme accessor to `ui::Engine` and an active-theme concept somewhere.~~
   **Done 2026-08-31.** `theme(name)`, `activeTheme()` and `activeTheme(name)`. With no
   field in the config naming the active theme, the first theme loaded is active; that
   choice belongs to item 3's schema work.
5. Decide where UI hit-testing, hover and focus live, and implement them against
   `Component::bound()`/`depth()` — both of which exist today with no reader. (Phase 1 or 3.)
6. Route component texture and image loading through `asset::Manager`. (Phase 3.)
7. Rebuild UI drawing as operations on the batched quad: the ortho UI pass, per-component
   transforms, `Button`'s nine-slice, `Label`/`Icon`, `MenuStack`'s vertical layout, and
   theme-to-font resolution. (Phase 3, blocked by Phase 2.)
8. ~~Move `Icon` and `style::prop::Image` off `v3d::gl::GLTexture`.~~ Done 2026-09-01, as
   part of deleting `api/gl` in phase 5.
9. Recover `luxa/tests/` from `d31a2e9^` into `api/ui/tests/`. (Tests workstream.)

Items 1, 2 and 4 are done. Items 5 and 9 do not require anything that does not already
exist. Items 3, 6, 7 and 8 are the real cost, and 7 is the one that has to wait.

Once 1–9 are done, `luxa/` — including `uitest/` — can be deleted in one commit.
