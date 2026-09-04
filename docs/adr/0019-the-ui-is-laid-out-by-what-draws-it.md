# ADR-0019: Menu Bar — The UI Is Laid Out By What Draws It, And Hit Tested Against Those Bounds

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`rigel/docs/xml/gui.xml` is the editor's whole UI definition, and its menu section is the
last thing in it that had not been translated. The camera profiles, the viewport layout and
the key bindings landed on 2026-09-01, and [ADR-0017](0017-a-command-is-a-name-in-a-context.md)
gave every menu item something to invoke. What was left was the menu tree itself — 73 of
gui.xml's 79 distinct command strings hang off a `<menuitem>` — and there was nothing in the
tree that could draw one.

`api/ui` has a menu, and it is the wrong shape for this. `ui::component::Menu` is a stack of
items with an active index and `next`/`previous`/`up`/`down`; `ComponentRenderer` draws the
active level as one panel centred on the canvas. That is a game's pause menu — pong, tetris
and voxel each drive one from the keyboard — and it draws exactly one panel at a time, has no
notion of a horizontal strip, and never asks where the cursor is. `component::MenuBar` existed
as a class with a constructor and nothing else.

The hard part is not drawing a bar. It is that a mouse driven menu has to answer *where the
cursor is*, and the components have no layout. `Component` carries a position, a size and a
`bound()`, and nothing in the library had ever written to them: the centred menu computes its
panel's corner inside `draw()` from the canvas size and throws it away. `HorizontalBox.h` and
`VerticalBox.h` are a `#pragma once` and a copyright header — there is no layout engine to add
a bar to.

## Decision

**Drawing is what lays the ui out, and a component is left holding the bounds it was drawn
in.** `ComponentRenderer` writes a position and a size onto the menu bar, onto each dropped
panel and onto every item in one, and `MenuBar::motion` and `MenuBar::press` answer the cursor
by testing it against those. Nothing is hit until something has been drawn.

Four things follow from it.

- **A menu bar is `api/ui`'s, not the editor's.** It is a widget, and a widget that reads a
  JSON tree through `ui::Engine` like the menu beside it. This is the opposite call from the
  construction grid of [ADR-0011](0011-lines-are-the-second-primitive.md), and for the
  opposite reason: a grid's extent and spacing are a modeller's policy, where a strip of
  labels that drops panels is not the editor's idea of anything.
- **A menu is drawn twice and owns only one of the rectangles.** Its label in the strip and
  the panel it drops are both on screen at once, so the label's bounds live on the bar, which
  is the component that draws the strip, and the menu's own bounds are the panel. One
  component cannot hold two rectangles, and the first version of this that tried left a press
  on an open menu's label landing inside its own dropped panel.
- **A check or radio item does not own the state it shows.** Activating one dispatches its
  command like an action item and marks nothing; whatever answers the command sets
  `checked()`. `Controller::syncMenu` reads the editor back after every command and after the
  active view changes, so the mark cannot disagree with what it describes — which it would,
  the first time a key binding invoked the same command the menu does.
- **The cursor is offered to the bar before the tools.** `Controller::handleMotion` and
  `Controller::drag` give the menu the event first and stop if it took it, except while a
  gesture is under way — a drag that wanders under the bar is not interrupted by it. A press
  the bar took is remembered, so the release that ends it does not reach three tools that
  never saw the press.

## Alternatives Considered

### Alternative 1: The renderer writes the bounds, and the component hit tests against them — **chosen**
- **Pros**: One computation decides both what is drawn and what is clicked, so they cannot
  disagree — the commonest defect in a hand rolled menu is a hit box that has drifted from its
  label. It uses `Component::position`, `size` and `bound()`, which exist for this and had no
  writer. It costs no layout engine: the panel's width comes from measuring its labels, which
  the renderer is already doing to draw them.
- **Cons**: A component that has never been drawn is not clickable, which is a silent failure
  rather than an error — a bar built but not yet rendered takes no press. The bounds are stale
  by one frame after a resize, and a click in that frame is tested against where the ui was.
  Layout and drawing cannot be separated later without moving both.
- **Why not**: n/a — chosen.

### Alternative 2: Give `HorizontalBox` and `VerticalBox` real implementations, and lay out in a pass of its own
- **Pros**: The conventional shape, and the one the empty headers were reserving. Layout would
  be answerable without a canvas, and a component would know its size before anything drew it.
- **Cons**: It is a layout engine — sizing policies, a measure and an arrange pass, and a
  notion of what a component's natural size is — for one consumer with one arrangement.
  Measuring still needs the app's font, so the pass takes the same callback the renderer does
  and the two have to agree about metrics anyway.
- **Why not**: Far more machinery than a strip of labels justifies, and it does not remove the
  agreement it is meant to remove.

### Alternative 3: Immediate mode — hand the cursor into `draw()` and answer the hit there
- **Pros**: No stored bounds at all, and no way for them to go stale. The smallest thing that
  could work.
- **Cons**: A press would only be answered on the frame it was drawn, so the controller would
  have to hold the event until the renderer ran and the ui would act a frame late. It also
  puts input handling in the renderer, which then needs the dispatcher.
- **Why not**: The ordering is backwards — the editor answers input as it arrives and draws
  afterwards.

### Alternative 4: Put the menu bar in `vertical3d/` beside the other editor policy
- **Pros**: The reasoning of ADR-0016 and ADR-0017 — one consumer is not a library — and the
  editor is the only thing with menus.
- **Cons**: It would need its own copy of the menu tree loader, the item types and the event
  binding, all of which `ui::Engine` and `ui::component` already have and none of which is
  editor specific. The one consumer argument applies to behaviour that encodes what an editor
  is; a row of labels does not.
- **Why not**: It would duplicate the library rather than use it.

## Consequences

### Positive
- The menu section of `gui.xml` is translated: `vertical3d/data/vgui.json` is nine menus over
  75 commands, drawn as a bar with dropped panels and flyouts three deep. That is the last of
  item 1 of [docs/RigelSurvey.md](../RigelSurvey.md).
- A key binding, a menu item and — when there is one — a toolbar button reach the same handler
  with nothing added for the second and third, which is what ADR-0017 was for.
- Verified against a run on 2026-09-02: the bar drawn over four viewports, Create > Poly > Cube
  making a cube, View > Show showing marks against exactly the three flags the editor tracks,
  the grid toggling off in the active view with its mark clearing, `render::settings` logging
  itself as unregistered, and Project > Quit shutting down cleanly — validation silent
  throughout.
- Ten cases cover the bar, none of which needs a window or a device: opening and closing,
  sliding between menus, the flyout's placement out of the parent's edge, an item's command
  being sent, the mark following `checked()`, the dismissing press being consumed and nothing
  more, and the untouched bar taking no press at all.
- The count of gui.xml's commands is corrected here: it has **79** distinct command strings,
  73 of them on a menu and six more only on a toolbar or a binding. ADR-0017 and the survey
  say 51, which is wrong; the registrations they describe are unchanged.

### Negative
- **20 of the editor's 24 registered commands are on a menu, and 55 of the menu's 75 are not
  registered.** Every one of those logs itself as unregistered when clicked, which is honest
  and is what makes the translation checkable, but a user meets a menu where most items do
  nothing. The four registered commands with no menu item are `view::drag` and the three
  camera modifiers, which are held rather than invoked.
- **The two toolbars are still untranslated.** `component::Button` has no event and no
  renderer, `HorizontalBox` and `VerticalBox` are empty headers, and gui.xml's top toolbar
  gives its nine buttons neither a name nor an icon — so translating it is inventing labels,
  not translating. The four icons the left toolbar names are in `rigel/icons/`.
- The empty `<menu name="Light">` under Create is dropped rather than translated. A submenu
  with no items opens onto nothing.
- gui.xml's Quit sits at the menu bar level, which a GTK bar allows and this one does not —
  the bar holds menus. It is the last item of the Project menu instead.

### Risks
- The bar consumes a press anywhere on screen while a menu is open, to dismiss it. A click
  meant for the viewport under an open menu therefore selects nothing, which is the
  conventional behaviour but is the one place the menu can eat an input the user meant for
  the scene.
- Radio items are drawn with the same square mark as check items — nothing distinguishes the
  two visually. None of gui.xml's radio groups has a handler yet, so no radio item has ever
  been marked; it matters the first time one is.
- `ui::style::Theme` still cannot supply any of this. The bar's height, colours and padding
  are `ComponentRenderer::Style` defaults that the editor overwrites from its font size, for
  the reason recorded on that struct: the style properties have no readers. A themed ui
  replaces that, and the layout it produces has to keep agreeing with the hit test.
