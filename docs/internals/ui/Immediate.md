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
| [ImmediateWindows.cpp](../../../api/ui/ImmediateWindows.cpp) | windows and regions, and the scrolling a window and a table share |
| [ImmediateTextField.cpp](../../../api/ui/ImmediateTextField.cpp) | the text field, the number that can be typed into, and the keys that reach them |
| [ImmediatePopups.cpp](../../../api/ui/ImmediatePopups.cpp) | the combo and the list it opens |
| [ImmediateTables.cpp](../../../api/ui/ImmediateTables.cpp) | tab strips and tables |

The layer keeps almost nothing between frames. These are members of the context:

- `hovered`: what the cursor was over last frame, which answers this frame's hover
- `hovering`: what it is over this frame, promoted at `end()`
- `active` and `pressAt`: what a press went down on, and where
- `wheeled`: the topmost scrolling region under the cursor, which takes the wheel
- `states`: a `Retained` per id. It holds a window's fold, drag offset and scroll, a scrolled
  region's content height and a tab strip's selection. It also holds the item a combo's list
  was clicked on, and the transition of a widget's fill
- `typing`, `edit` and `written`: the text field that has the keyboard, the line it is
  editing, and what it last wrote to the caller's string. A caller's string that differs from
  `written` was changed by the caller, and replaces the line. `escaped` says the keyboard is
  being given up by escape, which a number takes as no change
- `popup`: the combo whose list is open, and what the list holds
- `itemMin`, `itemMax` and `itemOwner`: the box of the item placed last, and which id had to
  hold the cursor last frame for `itemHovered()` to be true. `itemFinished` says an edit of it
  ended this frame, and `place()` clears it

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

A window and a region open their body through `openBody()`, which sets the margins, decides on
a bar, clips and claims the wheel. A region is a window with no title bar, so its body starts
at its top.

A window and a table each measure their content, clamp their scroll, draw a bar and take the
wheel, through `closeScroll()`. `scrollbar()` draws and drags the bar for both. A window
decides at its start whether it needs a bar (`Bar::WhenNeeded`, from last frame's content). A
table given a height always reserves the bar's width. A change to how scrolling feels has to
be checked in both paths.

The title-bar drag uses `dragThreshold` (3 pixels) in
[ImmediateWindows.cpp](../../../api/ui/ImmediateWindows.cpp). The offset is clamped every
frame against the current window size, so a window that grows near an edge moves under the
cursor.
