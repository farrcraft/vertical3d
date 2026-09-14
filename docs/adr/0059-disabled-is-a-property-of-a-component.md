# ADR-0059: UI Components — Disabled Is A Property Of A Component, Not A Fourth Button State

**Date**: 2026-09-13
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`api/ui` had the word for a control that is present and cannot be used —
`component::Button::STATE_INACTIVE` — and nothing anywhere painted it, skipped it or kept it.
The enumerator has been in `Button` since the component was written and no app in this tree had
ever set it, so the first consumer to try found five things at once.

**Nothing draws it.** The flat draw path has two appearances: `lit`, which is a checked toggle
or a hover, and everything else. `STATE_INACTIVE` and `STATE_PRESS` both drew exactly as
`STATE_NORMAL` did, and `paint::Dressing` carried `text` and `activeText` and no third label
colour that a disabled one could have used.

**A theme could say it and nothing read it.** `Loader` parsed `{"class": "button", "state":
"inactive"}` correctly and `skin()` matched a style by state, so a theme author could write the
style and reasonably expect it to be honoured. Only a nine-image skin consumed one, and a skin
replaces the plate and not the label colour, which is chosen outside it. There was no route by
which a theme could express a disabled look.

**One field held two lifetimes and the transient one won.** `Button::state()` carried hover and
press, which last as long as the cursor is where it is, and inactive, which is a property of the
control. `Cursor::lit()` wrote `STATE_HOVER` or `STATE_NORMAL` straight over it, so hovering a
disabled button destroyed its disabled-ness permanently. It did not bite only because
`Container::pick` tests `pickable()` before the bound, which made "disabled" two flags that had
to be set together with nothing anywhere saying so.

**The keyboard did not know about it.** `Engine::tabOrder()` tested `visible()` and
`focusable()`, so `pickable(false)` disabled a control against the mouse alone and tab still
reached it and return still activated it.

**And only `Button` had the notion.** `CheckBox`, `RadioButton`, `SelectList`, `TextBox` and
`Scrollbar` had no disabled state and no way to acquire one, so a settings page wanting to grey
out a control that does not apply had no answer for four of its five control types.

The prompting case is a game built against this tree in another repository: its title screen
offers **Continue**, which means nothing until there is a saved game. It set `"pickable": false`
and `STATE_INACTIVE` on the reasonable assumption that a state called *inactive* is drawn
inactively, and for two milestones that entry was pixel for pixel identical to the working
entries beside it. A player who pressed it got silence — which reads as a game that has stopped
responding rather than as a control that is not available yet. The same line marked a *working*
entry inactive for as long, and nobody noticed either way, which is the clearest evidence the
mark did nothing.

## Decision

**Whether a component can be used is `Component::enabled()`, beside `pickable()`,
`focusable()` and `visible()`, and it inherits down the tree.** `ui::usable(component)` is the
derived answer — the component and everything holding it. `Button::STATE_INACTIVE` is deleted.

| Site | What it does now |
|---|---|
| `Container::pick` | skips a disabled subtree whole, the way it skips a hidden one |
| `Engine::tabOrder` | skips it, so tab and return cannot reach it |
| `Engine::focus` | refuses it, the answer a component that never asked to be focusable gets |
| `Engine::focusNext` | starts the walk again when what held the focus is no longer in the order |
| `Cursor::lit` | leaves a disabled button's state alone, in both directions |
| `Keys::press`, `Keys::text` | answer no key and no character, so both reach the app's bindings |
| `Toolbar::buttonAt` | does not offer it the cursor, so a strip neither lights it nor sends its command |
| `paint::Dressing` | carries `disabledText`, read from the theme's `ui` style as `disabled-text` |
| `ComponentRenderer` | a disabled component is never lit, writes its text in `disabledText`, tints an icon with it, and draws no focus ring |
| `Loader` | reads `"enabled": false` on any component, the way it reads `pickable`, and `"state": "disabled"` — or the older `"inactive"` — on a button style |

A theme's button styles are told apart by `style::Button::State`, its own enum, rather than by
the component's `ButtonState`: three of those are the transient state a cursor writes and the
fourth is a property nothing about the cursor touches, and one enum for both is what let them be
confused in the first place.

## Alternatives Considered

### Alternative 1: Paint `STATE_INACTIVE`, and change nothing else
- **Pros**: about thirty lines — a `Dressing::disabledText` and a branch in `draw(Button)` — and
  it delivers the picture the prompting case asked for
- **Cons**: leaves the transient state writing over the permanent one, leaves the tab order
  reaching a disabled button, and leaves four of five control types with no way to say it
- **Why not**: the drawing is the symptom. A fix that leaves `Cursor::lit` able to destroy the
  property silently and permanently is a trap set for the second consumer.

### Alternative 2: `enabled()` on `Component`, not inherited
- **Pros**: a flag and two accessors, no walk anywhere, and every call site reads one bool
- **Cons**: a disabled `VerticalBox` holding three enabled buttons would be a box nobody can
  click through to controls that look and behave as though they work
- **Why not**: greying out a group is most of what the notion is for, and per-control calls kept
  in step by hand is the arrangement this ADR exists to end. Inheriting costs a parent walk in
  the one place that cannot skip a subtree — drawing — and nothing at all in the two that can.

### Alternative 3: Keep `STATE_INACTIVE` beside `enabled()`
- **Pros**: no consumer has to change, and a theme naming `"inactive"` needs no new spelling
- **Cons**: two ways to say one thing, one of which a hover silently destroys
- **Why not**: that is the defect with a second way of reaching it. The style name is data and
  keeps working; the enumerator is the part that was a trap, so it goes.

### Alternative 4: A per-state button style carries the disabled colours
- **Pros**: the theme says everything about a disabled button in one place, and `skin()` already
  finds the style by state
- **Cons**: `skin()` reports a style with no images as no skin at all, so this means reworking
  what a style-by-state is for; and a label colour is read outside the skin by every other
  control, which would then have two places to name one colour
- **Why not**: `disabled-text` in `ui` is where every other shared label colour already lives,
  per [ADR-0020](0020-a-theme-is-data-and-the-app-resolves-its-images.md), and it dresses all
  five control types rather than buttons alone.

## Consequences

### Positive
- A control that is there and cannot be used says so, and says it the same way everywhere: a
  greyed label, no plate, no ring, no hover, and an icon tinted with the same colour.
- Disabled is one call rather than two kept in step. `pickable(false)` meant *the mouse only*
  and nothing said so; `enabled(false)` reaches the cursor, the tab order and the paint.
- Greying a group is the box around it, which is what a settings page wants and what no
  per-control flag gives.
- The five control types that had no notion of it have one, because it lives on `Component`.
- A component that held the focus and is no longer reachable — disabled, hidden, or taken out of
  the tree — no longer strands the focus: `focusNext` starts the walk again. That was reachable
  before this change through `visible(false)` and nothing had found it.

### Negative
- **`Button::STATE_INACTIVE` is gone, and a consumer setting it will not compile.** That is the
  intent — the alternative is a silent no-op — and the port is `enabled(false)`.
- Drawing asks `usable()` per component per frame, which walks the parent chain. Depth is small
  and the walk is pointer chasing, but it is work the draw did not do before.
- A per-state button style still carries images only. A theme that wants a disabled *plate*
  colour rather than a disabled plate *image* cannot name one, which is Alternative 4's cost
  and is unchanged by this.
- `style::Button::State` and `component::Button::ButtonState` are two enums a reader has to tell
  apart, mapped in one place — `look()` in `ComponentRenderer.cpp`.

### Risks
- **A component disabled while it holds the focus keeps `focused()` until something moves the
  focus.** It draws no ring and answers no key, so nothing reaches it and nothing shows it, but
  `Engine::focused()` reports it and a listener on `onFocus()` was not told. Moving the focus
  from inside a setter would mean `Component` knowing its engine, which nothing else here needs.
- **Inheriting is the answer a reader expects and is surprising in one direction**: a component
  reports `enabled()` true while `usable()` is false, because something above it is disabled.
  The two names are the seam, and every call site in the library reads `usable()`.
