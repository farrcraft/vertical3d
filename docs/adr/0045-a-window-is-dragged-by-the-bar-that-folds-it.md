# ADR-0045: Window Dragging — The Title Bar Both Folds And Moves, And The Caller Still Owns Where A Window Starts

**Date**: 2026-09-08
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0035](0035-an-immediate-mode-layer-over-the-same-canvas.md) gave `api/ui` an immediate
mode layer, and `Immediate::window` said in its own docblock that the layer "does not drag
one": a window is drawn where the caller puts it and folds when its title bar is clicked. An
app porting a devtools panel off Dear ImGui loses the affordance, because ImGui moves a window
by its title bar for free.

The bar is already spoken for. `interact()` on the bar returns a `clicked`, and that toggles
`Retained::collapsed`, so drag and fold want the same box. The layer has the ingredients for
the drag itself — `drag_` is the cursor's movement since the last frame and `active_` is what a
press went down on — and it already keeps a per-window `Retained` across frames, which is where
a displacement would live.

## Decision

**A window's title bar both folds it and moves it, told apart by whether the press moved.** A
press that stays inside a few pixels folds on release, as it always did; one that travels past
that threshold drags the window and does not fold. `window()`'s signature does not change: the
caller's `position` stays the window's anchor and the layer keeps a displacement from it in
`Retained`, clamped so the title bar cannot leave the canvas.

## Alternatives Considered

### Alternative 1: A movement threshold tells a drag from a click — **chosen**
- **Pros**: The existing contract survives — a click on the bar still folds, so no call site
  changes and no user has to relearn the panel. It costs one flag and one displacement on
  `Retained`, and reuses `drag_` and `active_` as they are. The threshold is the standard
  answer, so the feel is the one a reader of the code expects.
- **Cons**: The window lags the cursor by up to the threshold, because the travel spent
  deciding is not applied. A very slow drag of one or two pixels folds instead of moving.
- **Why not**: n/a — chosen.

### Alternative 2: A fold caret takes the fold, and the whole bar drags
- **Pros**: No ambiguity at all, and a caret is more discoverable than a click that folds
  something.
- **Cons**: It changes what a title-bar click does for every existing caller, and adds a widget
  to draw and to hit test. `voxel`'s F3 readout folds by clicking its bar today and would stop.
- **Why not**: It is a larger change that breaks a working affordance to avoid a threshold
  constant.

### Alternative 3: Double-click folds and a plain drag moves, as ImGui does
- **Pros**: Exactly what a devtools user coming from ImGui expects, and no threshold.
- **Cons**: The layer has no notion of a double click. `Input` would grow one, `interact()`
  would have to time or count presses, and every widget would carry the concept for the sake of
  one box.
- **Why not**: It puts a new input concept into the layer's narrowest waist to settle a
  question a constant settles.

### Alternative 4: The caller owns the position and does the dragging itself
- **Pros**: No change to `api/ui` at all. An app already gets the cursor.
- **Cons**: Every app that wants a movable window reimplements the threshold, the clamp and the
  per-window state, and it cannot know whether the press landed on the bar without duplicating
  the layer's hit testing.
- **Why not**: It is the case the layer exists to answer, and the answer is eleven lines here
  against a copy of `interact()` there.

## Consequences

### Positive
- A devtools panel ports off Dear ImGui without losing the affordance, which was one of the two
  behaviour losses a consuming app recorded against this layer.
- `window()`'s signature is unchanged, so no call site in the tree moves and a caller that
  positions its window every frame still works: the drag is a displacement from wherever the
  caller puts it, so a window pinned to a corner follows the corner and keeps the nudge a user
  gave it.
- The displacement lives in `Retained`, so it ages out on `Immediate::retention` like a fold and
  a scroll do — a panel behind a toggle comes back where it was dragged to.

### Negative
- **The window lags the cursor by the threshold.** The travel spent deciding whether a press is
  a drag is not applied to the window, so the point under the cursor at the end of the drag is
  not the point that was under it at the start. Three pixels, once per drag.
- **A one-pixel drag folds.** Below the threshold the press is a click, so a user who means to
  nudge a window and does not commit gets a folded one instead.
- A window can now be somewhere the caller did not put it, so a caller that reasons about where
  its window is — to draw something beside it, say — is reasoning about the wrong place. The
  layer answers the cursor for its own boxes and this does not affect that, but it is a fact the
  caller no longer knows.

### Risks
- **The clamp is what stops a window being dragged out of reach.** A displacement that put the
  title bar off the canvas would persist in `Retained` and could not be undone, because the bar
  is the only thing that drags. The clamp keeps the bar on the canvas, and is the thing to check
  first if a window ever goes missing. The escape hatch for a caller is a new id, which starts a
  window fresh.
- A window whose `size` changes between frames re-clamps against the new size, so a window that
  grows while dragged near an edge moves under the cursor. The alternative — clamping only the
  top left corner — lets a window leave the canvas entirely, which is worse.
