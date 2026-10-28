# User Interface Internals

This document is for someone changing `api/ui` itself. It covers how the library is split into
classes, how a frame is laid out and drawn, styles, and what to touch when adding a component
type. How to use the library is in [api/ui/](../../api/ui/README.md), and this document assumes
it.

How input reaches a component is in [Input.md](Input.md), and the immediate layer is in
[Immediate.md](Immediate.md).

- [The classes](#the-classes)
- [Layout and drawing are one pass](#layout-and-drawing-are-one-pass)
- [The layout rule: never read an earlier frame's box](#the-layout-rule-never-read-an-earlier-frames-box)
- [Draw order and hit testing](#draw-order-and-hit-testing)
- [Styles and the Resolver](#styles-and-the-resolver)
- [Adding a component type](#adding-a-component-type)
- [Known limitations](#known-limitations)

## The classes

| Class | File | Responsibility |
|---|---|---|
| `Interface` | [Interface.h](../../../api/ui/Interface.h) | the loaded ui: containers, themes, the active theme, and the focus |
| `Loader` | [Loader.h](../../../api/ui/Loader.h) | reads a JSON document into containers and themes; used once by `Interface::load()` |
| `loader::ThemeReader`, `loader::ComponentReader` | [loader/](../../../api/ui/loader) | the two halves of the `Loader`: themes and their styles, and containers and their components |
| `loader::optionalString()` and the rest | [loader/Members.h](../../../api/ui/loader/Members.h) | how both readers read a member, which is where the rule for a bad member lives |
| `Container` | [Container.h](../../../api/ui/Container.h) | a named, show-and-hide group of root components; finds a component by name and picks one under a point |
| `Component` | [Component.h](../../../api/ui/Component.h) | a layout, children, flags, and the box the last draw gave it |
| `Layout`, `Length` | [Layout.h](../../../api/ui/Layout.h), [Length.h](../../../api/ui/Length.h) | the requested box, and how it resolves against a parent |
| `forEachDrawn` | [DrawOrder.h](../../../api/ui/DrawOrder.h) | the order a component's live children are visited in |
| `component::*` | [component/](../../../api/ui/component) | one class per type, plus `Type`, `name()`, `parse()` and `traits()` in [Type.h](../../../api/ui/component/Type.h) |
| `Arranger` | [Arranger.h](../../../api/ui/Arranger.h) | resolves every box in a tree and calls back to paint each one |
| `paint::ComponentRenderer` | [paint/ComponentRenderer.h](../../../api/ui/paint/ComponentRenderer.h) | paints each component type; owns an `Arranger` and a `style::Resolver` |
| `paint::Dressing` | [paint/Dressing.h](../../../api/ui/paint/Dressing.h) | the plain struct of colours and metrics both ui models draw with, and the one set of defaults |
| `paint::fillBox`, `strokeBox`, `plateBox` | [paint/Painter.h](../../../api/ui/paint/Painter.h) | box drawing shared by both ui models |
| `paint::Measure`, `paint::Write`, `paint::wrap` | [paint/Text.h](../../../api/ui/paint/Text.h) | the text callbacks, and greedy word wrapping over them |
| `paint::TextRenderer` | [paint/TextRenderer.h](../../../api/ui/paint/TextRenderer.h) | one font, one SDF atlas, and the callbacks over them |
| `style::Theme`, `Style`, `Property` | [style/](../../../api/ui/style) | the data a theme document holds |
| `style::Resolver` | [style/Resolver.h](../../../api/ui/style/Resolver.h) | turns a theme into a cached `Dressing` per class and style name |
| `input::Cursor` | [input/Cursor.h](../../../api/ui/input/Cursor.h) | routes mouse points: hover, press, drag, focus, caret placement |
| `input::Keys` | [input/Keys.h](../../../api/ui/input/Keys.h) | routes key names and composed text to the focused component |
| `input::LineEdit`, `input::edit()` | [input/LineEdit.h](../../../api/ui/input/LineEdit.h) | one line of text with a caret and a selection, and the edit a key names on it; a `TextBox` and `Immediate::textField()` both edit through them |
| `input::command()`, `input::send()` | [input/Command.h](../../../api/ui/input/Command.h) | which event a component sends when activated, and the one place events are sent |
| `Immediate` | [Immediate.h](../../../api/ui/Immediate.h) | the immediate-mode layer: only what an app calls |
| `immediate::Context` | [immediate/Context.h](../../../api/ui/immediate/Context.h) | everything an `Immediate` holds, and the layout, hit testing and drawing its widgets share |
| `shell::Screen` | [shell/Screen.h](../../../api/ui/shell/Screen.h) | builds the text renderer, component renderer, overlay and immediate layer over an `WindowRenderer` |
| `shell::Keyboard` | [shell/Keyboard.h](../../../api/ui/shell/Keyboard.h) | SDL adapter for `input::Keys`, and for an immediate layer's text field |
| `shell::GameMenu`, `shell::StatisticsOverlay`, `shell::FileChooser`, `shell::Prompt` | [shell/](../../../api/ui/shell) | shared game shell pieces |

Boundaries the split keeps:

- **Reading a document is the `Loader`'s.** `Interface.h` declares `Container` and
  `style::Theme`, and includes no component header. It names `boost::json::object` only for the
  document `load()` is handed. An app that draws a ui does not compile against every component.
- **Box arithmetic is the `Arranger`'s, and painting is `ComponentRenderer`'s.** They are
  joined by an `Arranger::Paint` callback. The renderer contains no layout arithmetic, and
  the arranger names no renderer.
- **No `api/ui` header names a Vulkan or SDL type.** `shell::Screen` declares `WindowRenderer`
  rather than including it. `shell::Keyboard` declares `SDL_Event`. `TextRenderer` takes its
  atlas upload as a callback. The clipboard and the text measure are callbacks. This keeps
  the library testable without a window or a device.
- **`v3dlib_ui` links `v3dlib_render` and `v3dlib_input`.** `shell::Screen` therefore cannot
  live in `api/render`, which would make a cycle. `v3dlib_input` is linked for
  `input::keyName()` alone.
- **`api/ui` sits below `api/engine`.** It cannot name `engine::Statistics`, so
  `StatisticsOverlay::Sample` is a copy the app fills.

## Layout and drawing are one pass

The ui is laid out by the same pass that draws it. `ComponentRenderer::draw(canvas,
container)` calls `Arranger::walk()` for each root component. `walk()` does this for one
component:

1. Return at once if the component is hidden.
2. Write the box it was given onto the component with `Arranger::place()`.
3. Call the `Paint` callback, if there is one.
4. Push a clip if the component asks for one and there is a canvas.
5. Visit its live children through `forEachDrawn()`, working out each child's box:
   - a tab bar's page gets `Arranger::page()`, the room under the tab strip
   - a flow box's child gets the box `Arranger::arrange()` computed for it
   - any other child gets `child->layout().resolve(parentBox, natural(child, parentBox))`
6. Pop the clip.

A root component's parent box is the whole canvas.

Because one pass decides both what is drawn and where it can be clicked, a hit box cannot
drift from what is drawn. The cost is that nothing has a box until it has been walked.

`walk()` accepts a null canvas and an empty `Paint`. It then resolves every box and draws
nothing. Layout can be checked in a test, or asked for before drawing, this way.

`Arranger::natural(component, room)` returns a component's natural size, used for an `Auto`
extent. Its cases are listed in the user document. A component with no natural size returns
`room.size()`. A list records its widest row on itself as a side effect. It keeps the width
with the serial of the `Arranger` that measured it. Each `Arranger` holds one measure and takes a
serial no other arranger has. A rescale builds a new renderer and so a new arranger, and the
list measures again. New rows clear the width too.

`Arranger::lineRoom()` is the room a flow box offers each child: zero along the line, the full
extent across it. `arrange()` and `wrapped()` write the children's boxes. `box()` is a flow
box's own natural size: the room it is offered, or for a wrapping box the line it is given and
the depth its lines reach.

Strips are placed separately. `Arranger::stack()` returns each toolbar with its corner, the
container's menu bars, and the total insets. Both `ComponentRenderer::draw()` and
`ComponentRenderer::insets()` call it, so the strips are drawn where the app was told they
would be. `Arranger::strip()`, `panel()` and `centred()` place a toolbar, a dropped menu panel
and a game menu level.

`Arranger::place()` takes a `Component&`, because `Menu::size()` is its item count and hides
the base class's box size.

## The layout rule: never read an earlier frame's box

**Nothing in layout reads a box that an earlier pass wrote.** `position()` and `size()` are
output only. `Layout::resolve()` and `Arranger::natural()` must not read them.

Concretely:

- An `Auto` extent is the component's natural size in the room it is offered, passed to
  `natural()` as an argument. It is never `Component::size()`.
- An `Auto` position is zero offset from the anchored corner. It is never
  `Component::position()`.

With this rule, layout is a pure function of the tree and the canvas size. A tree laid out
twice lands in the same place, the first frame matches every later one, and a resize places
every child against the new size. There is no dirty flag and no warm-up frame.

Breaking the rule brings back two defects. An `Auto` extent read from `size()` is zero on the
first frame. An `Auto` position read from `position()` sticks at the canvas origin, because the
origin is where the component was on the first frame.

What the rule allows: a component may keep a measurement it took during the layout pass, if
that measurement depends only on its own content and the measure. A `SelectList` keeps its
widest row, keyed by the arranger that measured it, and its row height. A `TextBox` keeps the
pen position of its line for caret placement. A `TabBar` keeps where its tabs were drawn for
hit testing. None of these feed back into a box.

## Draw order and hit testing

`ComponentRenderer::draw(canvas, engine)` draws each visible container in the order the
document listed them. Within a container:

1. Components in `Container::ordered()` order, which is by `depth` with insertion order kept
   between equal depths. The sort runs on every call, because a depth can change after a
   component is added. Strips are skipped here. A `Menu` is drawn centred with
   `draw(canvas, menu)`; everything else goes through `walk()`.
2. Toolbars, at the corners `stack()` gave them.
3. Menu bars, last, so an open menu's panel covers the strips below.

After a component is painted, `ComponentRenderer::ring()` traces the focus ring around it if
it is focused. The ring is drawn by the renderer rather than by each component's paint
function, so every control shows focus the same way. `ringed(type)` maps a component type to
the style class whose `focus` and `focus-width` it uses. A type with no class uses the base.

`forEachDrawn()` in [DrawOrder.h](../../../api/ui/DrawOrder.h) is the single statement of which
children are live and in what order:

- a tab bar visits only its selected page
- a flow box visits its children in the order it holds them
- anything else visits its children by depth

The draw pass, `Container::pick()` and `Interface::tabOrder()` all use it. A control on a page
that is not shown is therefore neither drawn, picked nor focused.

`Container::pick(point)` walks the same order in reverse:

- Root components are tried last-drawn first.
- Within a component, its live children are tried last-drawn first, and the component itself
  last.
- A hidden or disabled component is skipped with everything it holds.
- A component that is not `pickable()` is passed over without hiding what is under it.
- The test is against `bound()`, the box the last draw wrote.

A hidden component returns from `walk()` before `place()`, so it keeps the box it had when it
was last drawn. `pick()` skips it by its `visible()` flag, not by its box.

## Styles and the Resolver

`style::Resolver` holds the theme it was last given, a base `Dressing`, and one map per
`Resolver::Class` from style name to resolved `Dressing`.

- `chrome()` reads the theme's `ui` style into the base through `style::readBase()`.
  `Immediate::theme()` reads the `tools` style through the same function, so the two base
  classes cannot drift apart in their keys. A key added to `Dressing` is added to `readBase()`
  and to the table in [Themes.md](../../api/ui/Themes.md#the-base-classes-ui-and-tools).
- `resolve(class, name)` returns the cached answer, or computes one with `dress()`: a copy of
  the base, overwritten by whatever that class's style names, plus `focus` and `focus-width`.
- The cache is dropped when the theme changes and when the base is taken by non-const
  reference. `ComponentRenderer::dressing()` returns the non-const base, so an app that
  writes the dressing every frame turns the cache off.
- `lookup(theme, class, name)` returns the named style, or the first style of the class when
  the name is empty.
- Button skins are not cached. `ComponentRenderer::skin()` walks the theme's button styles of
  the component's style name and picks the one whose `style::Button::State` matches `look()`.
- `style::Button::State` is the theme's own enum (normal, hover, press, disabled).
  `component::Button::ButtonState` is the cursor's transient state. `look()` in
  [ComponentRenderer.cpp](../../../api/ui/paint/ComponentRenderer.cpp) is the one place they are
  mapped, with `usable()` deciding disabled.
- `Resolver::classes` is counted from the last enumerator, `Button`. A class added after
  `Button` indexes past the array.

A theme property is found by two strings, its name and its class. A typo in either is a
property silently not found.

## Parents and held items

`Component::parent()` is what `usable()` walks to find a disabled ancestor. `add()` sets it for a
child in `children()`. A component that holds items outside `children()`, such as a toolbar's
buttons, a menu's items, a menu bar's menus or a submenu item's submenu, calls `adopt()` on each
so the item inherits from it.

The parent is a raw pointer. An app may keep an item after its holder is destroyed, so every
holder calls `disown()` on each adopted item from its own destructor, and `~Component` does the
same for `children()`. A holder that replaces an item, as `menu::Item::submenu()` does, disowns the
one it replaces. A new holder type does all three.

## Adding a component type

Every `switch` over `component::Type` lists every enumerator and has no `default:` label.
MSVC warning C4062 reports an unhandled enumerator in such a switch. The tree turns it on with
`/w14062` and builds with `/WX`, so adding an enumerator fails the build at every switch that
has to decide something about it. Do not add a `default:` to any of these switches: it
silences the check for that switch, and nothing reports that it has.

The steps:

1. Add the enumerator to `component::Type` in [Type.h](../../../api/ui/component/Type.h). Keep
   the enum alphabetical. `parse()` walks the enumerators up to `VerticalBox`, the last one.
   A type added after `VerticalBox` would be skipped, so `TypeTest` sweeps past the end of the
   enum to catch it.
2. Write the class in `component/`, and add both files to
   [CMakeLists.txt](../../../api/ui/CMakeLists.txt). A control sets `pickable(true)` and
   `focusable(true)` in its constructor.
3. Build. The compiler then names every switch below:

| Switch | File | Decides |
|---|---|---|
| `name()` | [component/Type.cpp](../../../api/ui/component/Type.cpp) | the document's `type` string, or empty if a document cannot name it |
| `traits()` | [component/Type.cpp](../../../api/ui/component/Type.cpp) | whether it is a strip, a flow box, holds pages, or takes text |
| `loader::ComponentReader::buildComponent()` | [loader/ComponentReader.cpp](../../../api/ui/loader/ComponentReader.cpp) | how it is built from a document entry |
| `Arranger::natural()` | [Arranger.cpp](../../../api/ui/Arranger.cpp) | its natural size |
| `ComponentRenderer::paint()` | [paint/ComponentRenderer.cpp](../../../api/ui/paint/ComponentRenderer.cpp) | how it is drawn |
| `ringed()` | [paint/ComponentRenderer.cpp](../../../api/ui/paint/ComponentRenderer.cpp) | which style class its focus ring uses |
| `Cursor::act()` | [input/Cursor.cpp](../../../api/ui/input/Cursor.cpp) | what a press on it does |
| `Cursor::follow()` | [input/Cursor.cpp](../../../api/ui/input/Cursor.cpp) | whether a held press drags it |
| `Keys::act()` | [input/Keys.cpp](../../../api/ui/input/Keys.cpp) | which keys it takes |
| `input::command()` | [input/Command.cpp](../../../api/ui/input/Command.cpp) | which event activating it sends |

Places the compiler does not name:

- **A style class of its own.** Add an enumerator to `style::Resolver::Class` before
  `Button`, its name in `Resolver::named()`, and its keys in `Resolver::dress()`. `named()`
  and `dress()` are exhaustive over `Class`, so they are named once the enumerator exists.
  `ringed()` is checked only from `component::Type` to `Class`: a class nothing rings is not
  an error.
- **The paint overload.** `ComponentRenderer` has one `draw()` overload per type, called from
  `paint()`.
- **Tests** in [api/ui/tests/](../../../api/ui/tests), and the component table and theme tables
  in [api/ui/](../../api/ui/README.md).

Group the enumerators a switch does nothing for under one comment saying why. A reader of
`input::command()`, for example, can then see which types carry no command.

A registry of per-type functions was considered as a way to reduce the number of places. It
was not adopted, because `paint()` and `natural()` read private state of the renderer and the
arranger.

Background: [ADR-0047](../../adr/0047-code-exhaustive-enum-switches.md)

## Known limitations

- **Cursor and draw disagree about container order.** Containers are drawn in document order,
  so a later container draws on top. `Cursor::press()` and `motion()` offer the point to
  containers in the same document order, so an earlier container takes a press where it
  overlaps a later one drawn above it. Within a container the order is correct.
- **A component disabled while focused keeps `focused()`.** It draws no ring and takes no
  key, but `Interface::focused()` still returns it and `onFocus()` was not called. Moving the
  focus from `Component::enabled()` would need the component to know its engine.
- **A clip is a rectangle.** A rounded panel clips its children to its box. Lifting this
  would need the batch to carry a radius and the quad shader to mask a rounded rectangle.
  `Dressing::radius` defaults to 0 and no theme in the tree rounds a clipping panel.
- **Picking ignores clipping**, in both models. A child clipped out of view still takes a
  press or a hover where its box is.
- **A clip is in canvas pixels and the scissor in framebuffer pixels.** They agree only while
  the ui is drawn into a pass covering the whole target.
- **Nothing checks that a child's box lies inside its parent's.**
- **A parent cannot size itself to its children**, except a wrapping flow box across its
  lines.
- **`Auto` means two things.** Outside a flow box an `Auto` extent fills; along a flow box's
  line it is content-sized. Nothing in the type says so.
- **Drawing calls `usable()` per component per frame**, walking the parent chain.
- **A per-state button style carries images only.** A theme cannot name a disabled plate
  colour.
- **`TextBox::at()` calls `Measure` once per character boundary**, which is quadratic in the
  line length. It runs per click, not per frame.
- **Selected text keeps its colour.** The selection is a highlight behind a line drawn whole.
- **Radio menu items use the same square mark as check items.**
- **A button in the tree has a hover state, but no other tree component does.**
- **`Immediate` stores one `Retained` per id**, so ids built from changing text churn the
  map until `retention` ages them out.
- **`TextRenderer`'s upload has no test.** Its measuring and drawing are tested with a
  stand-in upload, and the upload itself needs a device.
