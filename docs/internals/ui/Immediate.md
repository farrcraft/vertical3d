# The immediate layer

This document is for someone changing `api/ui`'s immediate layer. How to use it is in
[api/ui/ImmediateMode.md](../../api/ui/ImmediateMode.md). [README.md](README.md) covers the
rest of the library, and this document assumes it.

`Immediate.h` declares only what an app calls. An `Immediate` holds one `immediate::Context`,
which holds its state and the steps its widgets share: `place()`, `interact()`, `label()` and
the scrolling. The widgets are written in files by subject:

| File | Holds |
|---|---|
| [Immediate.cpp](../../../api/ui/Immediate.cpp) | the frame, ids, text, buttons and the other single-row widgets |
| [immediate/Context.cpp](../../../api/ui/immediate/Context.cpp) | layout, hit testing, ids and the drawing every widget shares |
| [immediate/Windows.cpp](../../../api/ui/immediate/Windows.cpp) | windows, and the scrolling a window and a table share |
| [immediate/TextField.cpp](../../../api/ui/immediate/TextField.cpp) | the text field, and the keys that reach it |
| [immediate/Popups.cpp](../../../api/ui/immediate/Popups.cpp) | the combo and the list it opens |
| [immediate/Tables.cpp](../../../api/ui/immediate/Tables.cpp) | tab strips and tables |

The layer keeps almost nothing between frames. These are members of the context:

- `hovered`: what the cursor was over last frame, which answers this frame's hover
- `hovering`: what it is over this frame, promoted at `end()`
- `active` and `pressAt`: what a press went down on, and where
- `wheeled`: the topmost scrolling region under the cursor, which takes the wheel
- `states`: a `Retained` per id, holding a window's fold, drag offset and scroll, a scrolled
  region's content height, a tab strip's selection, and the item a combo's list was clicked on
- `typing`, `edit` and `written`: the text field that has the keyboard, the line it is
  editing, and what it last wrote to the caller's string. A caller's string that differs from
  `written` was changed by the caller, and replaces the line
- `popup`: the combo whose list is open, and what the list holds
- `itemMin`, `itemMax` and `itemOwner`: the box of the item placed last, and which id had to
  hold the cursor last frame for `itemHovered()` to be true

`end()` draws the open combo's list, then takes the keyboard from a field that was not drawn
this frame, then ages out any `Retained` entry not touched for `retention` frames. The list is
drawn before `hovered` is promoted, so it takes the cursor from whatever it covers.

`place()` records each item's box, with the enclosing window's id as its owner, or nothing
outside a window. When `interact()` is offered exactly that box, the widget's own id becomes
the owner. A title bar or a scrollbar is offered a box `place()` never made, so it leaves the
last item alone.

A key reaches a field through `press()` between frames and edits `edit` at once. A return,
tab or escape sets `letGo` rather than taking the keyboard, so that the field writes what was
typed when it is next drawn, and gives the keyboard up after that.

`interact(id, min, max)` decides hover, hold and click for one box. A widget is hovered only
if the cursor is in its box now and it was the hovered id last frame. That second condition
is how a later window takes the cursor from an earlier one. A disabled widget is offered
nothing. `interact()` takes no account of clipping.

A window and a table each measure their content, clamp their scroll, draw a bar and take the
wheel, through `closeScroll()`. `scrollbar()` draws and drags the bar for both. A window
decides at its start whether it needs a bar (`Bar::WhenNeeded`, from last frame's content). A
table given a height always reserves the bar's width. A change to how scrolling feels has to
be checked in both paths.

The title-bar drag uses `dragThreshold` (3 pixels) in
[immediate/Windows.cpp](../../../api/ui/immediate/Windows.cpp). The offset is clamped every
frame against the current window size, so a window that grows near an edge moves under the
cursor.
