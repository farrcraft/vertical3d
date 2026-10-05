# ADR-0035: UI: immediate mode beside the retained tree

**Status**: accepted
**Date**: 2026-09-06
**Documented in**: [api/ui/ImmediateMode.md](../api/ui/ImmediateMode.md), [internals/UserInterface.md](../internals/UserInterface.md)

## Context

A HUD suits a retained tree: it is built once, from a JSON document, and its components are
found by name. A developer tool is different: its panels read live state every frame, such as a
unit's hit points or whose turn it is. Kept as a retained tree, each such panel has to be
updated by hand, and the usual defect is a readout that shows stale state. Both kinds of ui
should look alike and draw onto the same canvas with the same text callbacks.

## Decision

`v3d::ui::Immediate` is a second way to write a ui, drawing onto the same `realtime::Canvas` as
the retained components. A panel is a sequence of calls between `begin()` and `end()`, and each
widget is placed by a layout pen, hit-tested against the box it was just drawn in, and answers
the call at once. Between frames it keeps only what a widget cannot recompute: what the cursor
is over, what a press went down on, and small per-widget state such as a window's fold and
scroll.

## Alternatives

### Build the tools as retained components on the box model
- **For**: one way to write a ui, and no new concepts.
- **Against**: every panel becomes a tree to build once and update by hand, including rows that
  appear and disappear with entities. A stale readout is the defect a debugging tool can least
  afford.
- **Rejected because**: it is more code than the immediate version and gives up the property the
  tool needs.

### Keep Dear ImGui for tools, and use `api/ui` for game ui
- **For**: a mature, complete toolkit, and no new code.
- **Against**: a second frame model, a second font path and a second renderer backend, drawn in
  a different pass from the rest of the ui. The boundary between the two needs maintaining
  indefinitely.
- **Rejected because**: removing a second toolkit and its upkeep is part of what `api/ui` is
  for.

### Make the retained components immediate underneath, so there is one model
- **For**: one model throughout.
- **Against**: a theme, a ui document and a component found by name are retained by nature. The
  editor's menu bar is built once from a file and looked up by name. Rewriting that as calls per
  frame moves the tree into the app instead of removing it.
- **Rejected because**: it rewrites working code to merge two models that serve different jobs.

## Consequences

- **Gains**:
  - A panel that reads live state is one function, and cannot show stale values.
  - It shares the canvas, so it adds no render pass or draw call.
  - It shares the box drawing and the text callbacks with the retained side, so the two look
    like one ui. It reads its own `tools` theme class, because a tool panel and a HUD want
    different metrics.
  - It is testable with no window, device or font.
- **Costs**:
  - Two ways to write a ui in one library, and nothing in the code says which to use.
  - Hover is a frame behind: it is decided while a frame is drawn and used by the next, so a
    widget that appeared or moved is not hovered until the frame after.
  - A widget's id is its label hashed with the id stack. Two widgets with the same label in the
    same scope share hover and press state until `pushId` separates them.
  - Per-widget state is kept for a number of frames after a widget stops being drawn, so a
    panel that builds ids from changing text creates and discards entries.
- **Revisit when**: a tool needs state the immediate layer cannot recompute each frame, or the
  two models start to need different drawing.
