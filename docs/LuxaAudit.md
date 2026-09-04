# Luxa Audit

**Closed 2026-09-04. `luxa/` is deleted.** All nine items are worked off; the last two
landed as [ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md). What
follows is the record of what the tree held and where each piece went, kept because it is
the only account of that. Recover the sources from `git show <commit>^:luxa/...` if a
question about the original ever comes up.

Phase 1 of [plans/Modernization.md](plans/Modernization.md). Audited against the tree on
2026-08-31, rechecked on 2026-09-04 against `api/ui` as the menu bar and the two toolbars
left it. The recheck's findings are marked with their date; anything unmarked still reads
as it did on the first pass.

`luxa/` is a migration in progress, not dead code. This is the functional equivalence
record that has to close before the tree can be deleted: every piece still sitting in
`luxa/`, where it landed in `api/ui` (or did not), and what has to be built before nothing
is lost.

**Verdict up front: `luxa/` cannot be deleted yet.** Two pieces have no equivalent and one
is a stub. Everything else has either landed or turned out to be worth nothing.

**Verdict on the recheck: one piece is left, and it is the theme.** Drawing was rebuilt on
the batched quad and hit-testing arrived with it, both under
[ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md), so items 5 and 7 of the list
below are answered — differently than this audit imagined them, which is recorded where each
is discussed. What is still missing is everything under `api/ui/style/`: no config arm reads
a style, nothing in the tree constructs one, `prop::Color` holds a colour that no
constructor, setter or accessor ever touches, and `ComponentRenderer::Style` is a struct of
hardcoded colours and metrics standing in for the theme that cannot supply them. Images are
the same gap seen from the other side — `Icon` and `prop::Image` each hold a texture handle
nothing sets, which is why the editor's toolbars are labelled rather than iconic.

**That last piece landed the same day**, as ADR-0020: `Engine::load` reads a theme's styles
and their colours, numbers, fonts and images; `ComponentRenderer::theme()` reads the `ui`
style into what it draws with, overriding only what the theme names; and
`Engine::resolveImages()` hands every source the config named to a callback the app supplies,
which is how a `prop::Image` and an `Icon` get the texture they hold. `Button`, `Label` and
`Icon` are loadable and drawn, a button with images is a nine slice, and
`vertical3d/data/vgui.json` carries the editor's chrome colours as the reference for the
schema.

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

As the recheck of 2026-09-04 and the work that closed it leave it:

| luxa | api equivalent | Verdict |
|---|---|---|
| `Component` | `v3d::ui::Component` | Covered. `draw` is the renderer's, `exec`/`notify` are not coming back |
| `Button` | `ui::component::Button` | Drawn and hovered as part of a `Toolbar`; `STATE_PRESS` and `STATE_INACTIVE` drive nothing |
| `Label` | `ui::component::Label` | Covered — a `"label"` component is loaded and drawn as a line of text |
| `Icon` | `ui::component::Icon` | Covered — it names its source, the image pass writes the handle, and it draws |
| `menu/MenuStack` | `ComponentRenderer::draw(canvas, menu)` | Covered — the centred vertical layout is drawn |
| `ComponentManager` | `ui::Engine` + `ui::Container` + the components | Covered — input on the drawn components, images through `resolveImages()` |
| `ComponentRenderer` | `v3d::ui::ComponentRenderer` | Rebuilt on the batched quad; menus, bars, toolbars, buttons, labels and icons |
| `UILoader` | `ui::Engine::load` | Covered — six component arms, plus themes, styles and the four property kinds |

Migrated before this audit and confirmed complete: `Menu`, `MenuItem`, `Theme`, `Style`,
`StyleProperty` (→ `style::Property`), `ButtonStyle`, `MenuStyle`, `ColorStyleProperty`,
`FontStyleProperty`, `ImageStyleProperty`, `Overlay`, `Container`, and the dozen component
stubs (`Dialog`, `Frame`, `CheckBox`, `InputBox`, `RadioButton`, `Scrollbar`, `SelectList`,
`Spinner`, `TabBar`, `TabPage`, `TextBox`, `ToolTip`, `HorizontalBox`, `VerticalBox`,
`MenuBar`, `PopupMenu`, `RadialMenu`). The stubs were empty in luxa and are empty in `api`,
so "migrated" means the empty class moved; `api/ui/component/HorizontalBox.h` is now
literally a `#pragma once`.

`Overlay` is complete in the same sense: the class holds a mode and a colour that nothing
reads. The renderer draws a panel behind a menu out of its own `Style` rather than out of an
overlay, so the class is a candidate for the style schema to absorb or for deletion.

## The three things with no equivalent

All three are built as of 2026-09-04: the renderer on 2026-09-02, hit-testing with it, and
the loader's style half with ADR-0020.

### 1. ~~`ComponentRenderer` — must be rebuilt on Vulkan, not ported~~

**Rebuilt 2026-09-02**, as `v3d::ui::ComponentRenderer` over `render::realtime::Canvas`, and
extended with the toolbars on 2026-09-04. Four of the five things below resolved rather than
landing as written, which is what the rebuild bought:

- **the orthographic UI pass** is not a pass. The ui is added to whatever canvas the app is
  already filling, so it costs the frame no pass and no draw of its own.
- **the matrix stack** is gone with the immediate mode it served. A quad is emitted at the
  pixel it belongs at.
- **textured quad and texture drawing** are the batched quad, as predicted.
- **`getDefaultFont`** was not ported and should not be. Text measuring and writing are
  `Measure` and `Write` callbacks the app supplies, which is what keeps `v3dlib_ui` free of
  the font library and makes the whole renderer testable without a window or a device.
  Resolving a face out of the theme returns with the style schema, item 3 below.
- **`width`/`height`/`resize`** are `Canvas::width()`/`height()`. The managed area is
  whatever the app is drawing into, so there is nothing to keep in sync.

What did not come across, and waits on items 3 and 6: `Button`'s nine-slice, `Label` and
`Icon`. Nothing constructs any of the three from a config, so nothing can be drawn from one.

The original finding follows.

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

- ~~**Mouse hit-testing and hover/focus tracking.**~~ **Covered for what is drawn,
  2026-09-04**, and not by an `intersect(point)` over the container. Per ADR-0019 a
  component holds the bounds its last draw left on it, and each drawn component answers the
  cursor against its own: `MenuBar::motion`/`press` test the labels in the strip and the
  rows of the panels below them, and `Toolbar::motion`/`press`/`leave` test the buttons in
  the strip, leaving the one under the cursor in `STATE_HOVER` and the rest normal. `bound()`
  has readers now; **`depth()` still has none** — an app decides the order components are
  offered the cursor in, which for the editor is the menu bar first because an open panel is
  drawn over the strips under it. Three things the luxa version had are still absent: a
  z-ordered walk, focus, and `enter`/`leave` as events rather than as a state a strip writes.
  `intersect(point)` walked components by z-index and returned the topmost hit; `motion`
  synthesised `enter`/`leave` on the component under the cursor;
  `buttonPressed`/`buttonReleased` forwarded to it.
- ~~**The theme registry and active theme.**~~ **Covered 2026-08-31** — `ui::Engine` gained
  `theme(name)`, `activeTheme()` and `activeTheme(name)`. Which theme starts active is still
  a guess (the first one loaded), because the config has no field to say.
- **The texture registry.** `addTexture`, keyed by `style-name + property-name + source`.
  Still nothing, 2026-09-04. `api/ui` has no image path at all: `prop::Image::texture()` and
  `component::Icon`'s handle are both set by nobody.
- **Image loading.** `loadImage` via `image::Factory`. The api replacement should be
  `asset::Manager`, in line with the same change Phase 4 makes for tetris.
- ~~**`resize`.** Forwards the window canvas size to the UI area.~~ Not needed, 2026-09-04 —
  the renderer reads `Canvas::width()`/`height()`, so there is no second area to resize.
- ~~**`toggleComponentVisibility`.**~~ Covered, 2026-09-04 — `Component::visible()` is read
  by `ComponentRenderer::draw` and by `insets()`, and `Container::visible()` gates the
  container.

Two of these carry bugs worth *not* reproducing: `intersect` requires `component` to be
non-null before it will ever assign to it, so it returns null unconditionally; and
`buttonPressed`/`buttonReleased` dereference its result without a null check.

**Action:** hit-testing and hover/focus have to be designed into `api/ui` before luxa goes,
and they need `api/input/Mouse::handleEvent` implemented first — it currently swallows every
mouse event, see [V3dlibsAudit.md](V3dlibsAudit.md). Texture and image registries should
resolve to `asset::Manager` rather than being ported.

**Action as of 2026-09-04:** hit-testing is done for the components that are drawn and needs
extending only when something else is. The registries are what is left, and they belong with
item 3's schema — a texture is loaded because a style property names it.

### 3. `UILoader` — `ui::Engine::load` covers menus and nothing else

`ui::Engine::load` reads themes (names only) and containers, and dispatches on component
type — with exactly one arm, `"menu"`. As of 2026-09-04 it has three: `"menu"`, `"menubar"`
and `"toolbar"`, the last two carrying `loadCommand`, which reads the `context`/`command`
pair a menu item and a toolbar button both name. Themes are still names only. `UILoader`
loaded, from the same conceptual document:

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

Unchanged on 2026-09-04, and two details sharpen it:

- **Nothing still constructs a `Style` or a `Property`.** `Engine::load`'s
  `make_shared<style::Theme>` is the only construction anywhere under `style/`, and a theme
  with no styles in it answers `getStyleSet` with nothing.
- **`prop::Color` cannot carry a colour.** Its `color_` has no constructor argument, no
  setter and no accessor, so the class is unreachable rather than merely unread. That is why
  `ComponentRenderer::Style` exists: a struct of hardcoded metrics and colours standing in
  for a theme that has no way to supply them, which is where every ui colour in the tree
  currently comes from.
- **What the schema has to cover shrank.** `loadDefaultComponentAttributes`' position is now
  an output of drawing rather than an input to it (ADR-0019), and `name` and `visible` are
  read for a container. What is left is genuinely the theme: styles, style properties,
  fonts, images, and a field naming the active theme.

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
3. **Input capture for input-type menu items does not exist.** Still true 2026-09-04. The old design terminated
   capture on the `ui::selectMenu` binding. `Menu::activate` documents this in a comment and
   implements nothing, so `setMaxScore` and the four key-binding menu items in
   `pong/data/vgui.json` are unreachable. The value field they would write to now exists;
   the capture does not.
4. **`Component::exec` and `Component::notify` are gone**, and with them `Button::notify` —
   the `enter`/`leave`/`button_pressed`/`button_released` → `STATE_HOVER`/`STATE_NORMAL`/
   `STATE_PRESS` transitions. `ui::component::Button` keeps `state()` and the `ButtonState`
   enum with nothing to drive them. The replacement is presumably the entt dispatcher, but
   no component subscribes to anything today.
   **Half answered 2026-09-04, and not by the dispatcher.** The strip owns the transition:
   `Toolbar::motion` writes `STATE_HOVER` on the button under the cursor and `STATE_NORMAL`
   on the rest, and `leave()` clears them all. A component still subscribes to nothing, and
   `exec`/`notify` are not coming back. `STATE_PRESS` and `STATE_INACTIVE` drive nothing —
   a press sends its command outright, so there is no held-down state to draw, and nothing
   makes a button insensitive.
5. ~~**`api/ui` still depends on OpenGL.**~~ Fixed 2026-09-01. `component/Icon.h` and
   `style/property/Image.h` now hold a `render::realtime::TextureHandle` instead of a
   `boost::shared_ptr<v3d::gl::GLTexture>`, which cost `v3dlib_ui` no new dependency - it
   already links `v3dlib_render` for `ComponentRenderer`'s canvas. **Nothing sets either
   handle yet**: uploading a theme's image through `QuadRenderer::texture` and drawing an
   `Icon` from it is still item 6 below.

## Salvage

- ~~**`luxa/tests/` was deleted, not migrated.**~~ **There is nothing there, 2026-09-04.**
  Read out of `d31a2e9^`, `ButtonTest.cxx` is 104 bytes holding one `types_test` with an
  empty body, `ComponentManagerTest.cxx` is a zero-byte file, and `TestMain.cxx` is the
  module boilerplate. This is the same dead end the three `v3dlibs` recoveries were. `api/ui`
  has its own suite instead — `ComponentRendererTest`, `MenuBarTest` and `ToolbarTest`,
  written against what the library does now.
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
3. ~~Design the JSON schema for themes, styles, style properties and fonts, and extend
   `ui::Engine::load` past the single `"menu"` arm.~~ **Done 2026-09-04**, as
   [ADR-0020](adr/0020-a-theme-is-data-and-the-app-resolves-its-images.md). A theme carries
   styles, a style carries colours, numbers, fonts and images — each read as the kind of the
   array it was written in — and a button style carries the state it dresses.
   `ComponentRenderer::theme()` reads the `ui` style into the colours and metrics it draws
   with and overrides only what the theme names, so a theme carrying nothing draws what it
   always drew. `prop::Color` gained the colour it never had — with alpha, because the ui is
   drawn over the scene — and `prop::Number` is new, so a metric is as much a theme's as a
   colour is. `loadDefaultComponentAttributes` came with it as `loadAttributes`.
4. ~~Add a theme accessor to `ui::Engine` and an active-theme concept somewhere.~~
   **Done 2026-08-31.** `theme(name)`, `activeTheme()` and `activeTheme(name)`. With no
   field in the config naming the active theme, the first theme loaded is active; that
   choice belongs to item 3's schema work.
5. ~~Decide where UI hit-testing, hover and focus live, and implement them against
   `Component::bound()`/`depth()` — both of which exist today with no reader.~~ **Settled
   2026-09-04 by [ADR-0019](adr/0019-the-ui-is-laid-out-by-what-draws-it.md).** Hit-testing
   lives on the component that was drawn, against the bounds its draw left on it, and the
   menu bar and the toolbars both answer the cursor that way. Hover is a state a strip
   writes on its buttons. Two things the luxa version had are deliberately not there:
   `depth()` still has no reader, because an app orders the offer rather than a z-index
   doing it, and there is no focus — nothing in the tree takes the keyboard through the ui.
   Both return with the first component that needs them.
6. ~~Route component texture and image loading through `asset::Manager`.~~ **Done
   2026-09-04**, and through a callback rather than a dependency: `Engine::resolveImages()`
   hands every source the config named to whatever the app supplies, which for the editor is
   `asset::Manager::load` and `QuadRenderer::texture`. That is the same seam text measuring
   and writing already use, and it is why the schema, the image pass and the drawing are all
   covered without a device. `Icon` names its source and holds the handle; `prop::Image`
   already did; and `component::Button` gained both, which is what made the editor's left
   toolbar iconic the same day - gui.xml's four icons, recovered out of `rigel/icons/`.
7. ~~Rebuild UI drawing as operations on the batched quad: the ortho UI pass, per-component
   transforms, `Button`'s nine-slice, `Label`/`Icon`, `MenuStack`'s vertical layout, and
   theme-to-font resolution.~~ **Done for what can be constructed, 2026-09-02 and
   2026-09-04.** `ComponentRenderer` draws menus, menu bars with their panels and flyouts,
   and toolbars, all onto the app's canvas as batched quads. The ortho pass and the
   per-component transforms turned out not to be needed and theme-to-font resolution became
   the `Measure`/`Write` callbacks. The nine-slice, `Label` and `Icon` **landed with 3 and 6
   on 2026-09-04**: all three are loadable component types now, a label is a line of text at
   the position it holds, an icon is one textured quad, and a button is nine quads over the
   images its style names for its state — or the flat one the toolbars draw, when the theme
   names none.
8. ~~Move `Icon` and `style::prop::Image` off `v3d::gl::GLTexture`.~~ Done 2026-09-01, as
   part of deleting `api/gl` in phase 5.
9. ~~Recover `luxa/tests/` from `d31a2e9^` into `api/ui/tests/`.~~ **Struck 2026-09-04 —
   there is nothing to recover.** Two of the three files are empty and the third is
   boilerplate; see Salvage above. `api/ui/tests/` covers the library as it stands.

Items 1, 2 and 4 are done. Items 5 and 9 do not require anything that does not already
exist. Items 3, 6, 7 and 8 are the real cost, and 7 is the one that has to wait.

**Where the list stands on 2026-09-04: all nine are closed**, 3 and 6 as one piece of work
because a style property names an image, so the schema that reads the property is what says
which textures to load. `api/ui/style/` is reachable, `ComponentRenderer::Style` is a set of
defaults a theme overrides, and `Button`, `Label` and `Icon` are drawn.

Once 1–9 are done, `luxa/` — including `uitest/` — can be deleted in one commit. **Deleted
2026-09-04.**
