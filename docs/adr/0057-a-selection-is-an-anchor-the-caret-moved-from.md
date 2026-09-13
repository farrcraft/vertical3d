# ADR-0057: Text Editing — A Selection Is An Anchor The Caret Moved From, And The Cursor Measures Text To Place One

**Date**: 2026-09-13
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`ui::TextBox` held one line, a byte offset for the caret, and nothing else. That was enough to
type, to backspace and to walk the caret with the arrows, and it was not enough for anything a
user of a text field expects beyond that: there was no run to cut, nothing to copy, no way to
replace what was there by typing over it, and no way to put the caret where a click landed.

Two seams were in the way of the last of those. `ui::Cursor` routes a press by the boxes the
last draw left ([ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md)) and a box is the one
component a press can land in the *middle* of — finding the character under a point needs the
width of a run of text, which only the app can answer, through the `paint::Measure` callback
both renderers already take. And a clipboard is the platform's: `api/ui` names no SDL type
anywhere, and reaching for `SDL_GetClipboardText` would make it the first place that did.

`ui::Keys` names an operation and the component carries it out
([ADR-0040](0040-a-key-goes-to-a-focused-component.md)), so whatever shape the selection takes
has to be expressible as operations a key can name.

## Decision

A selection is an anchor the caret has moved away from: two byte offsets, equal exactly when
nothing is selected. Every operation that moves the caret says whether the anchor comes with
it, and a cut, a copy and a paste are then a run of bytes and an insertion, which the box
already had.

`ui::Cursor` takes the same `paint::Measure` the renderers take, so a press places the caret
and a drag selects; `ui::Keys` takes a `Clipboard` pair of callbacks, so the four chords an
editor answers work without `api/ui` learning what a clipboard is. Both are optional: a cursor
given no `Measure` and a router given no `Clipboard` behave as they did before either existed.

## Alternatives Considered

### Alternative 1: A selection is its own range, beside the caret
- **Pros**: reads directly — `selectionBegin()`, `selectionEnd()`, `hasSelection()`; nothing
  about a caret move has to think about an anchor.
- **Cons**: three pieces of state that can disagree. Every edit has to decide what the range
  means afterwards, and the characteristic defect is a stale range pointing into text that has
  been retyped — the same class of bug as a retained ui showing a number two frames old.
- **Why not**: the anchor makes "nothing is selected" a place rather than a flag, so there is
  no third state to keep in step and no way to express a selection the caret is not on an end
  of.

### Alternative 2: The box measures its own text
- **Pros**: `at()` would need no argument, and nothing about `ui::Cursor` would change.
- **Cons**: a component would hold a font callback, which is the one thing ADR-0019's seam
  keeps out of the tree of components — and every box would hold its own copy of the app's.
- **Why not**: the callback belongs to whatever draws, and what draws hands it in. The box is
  left holding the pen the last draw wrote on it, which is the same thing a `SelectList` does
  with the height of a row.

### Alternative 3: `api/ui` reads the clipboard itself
- **Pros**: nothing to wire; cut, copy and paste work the moment a `ui::Keys` exists.
- **Cons**: `api/ui` names no vulkan type and no SDL type, which is what makes it testable
  with no window and usable by an app bringing its own platform layer
  ([ADR-0028](0028-an-apps-shell-belongs-to-the-api.md) is the shape that assumes it).
- **Why not**: one `#include <SDL3/SDL_clipboard.h>` would cost the library that property for
  two calls the app can pass in.

### Alternative 4: A modifier is a key, so `press("ctrl+c")`
- **Pros**: one argument, and no modifier state to carry.
- **Cons**: `api/input` names keys one at a time, so the app would compose the string itself —
  an allocation per key, and a second naming convention beside the one `api/input` owns.
- **Why not**: `press()` already took `shifted` for tab. `controlled` is the same argument for
  the same reason, and the app that saw the key is still what says.

## Consequences

### Positive
- Typing over a selected run replaces it, a click puts the caret where it landed, a drag and
  shift with the caret keys both select, and cut, copy, paste and select all work.
- Nothing in `api/ui` learned a platform type, and the library still tests with no window, no
  device and no font.
- `TextBox::insert()` did not change shape: a paste is a run of characters, which is what a
  composed character already was.

### Negative
- `ui::Cursor` now has a reason to hold a font callback, so an app that routes input and draws
  with two different measurements of the same font will put the caret in the wrong place. The
  fix is to hand both the same pair, which is what `ui::TextRenderer` gives out.
- `at()` costs one `Measure` call per character boundary of the line. That is a click rather
  than a frame, and a box holds one line, but it is quadratic in the length of that line.

### Risks
- The selection is drawn as a highlight behind a line that is still written whole, so selected
  text keeps its colour. A theme choosing an opaque `highlight` for its `textbox` class hides
  the run it is meant to mark. Drawing the line in three pieces would fix the colour and would
  measure the pieces separately, which is the thing the caret placement relies on not doing.
