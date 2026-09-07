# ADR-0035: Immediate Mode — A Second Way To Draw A UI, Onto The Same Canvas

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0034](0034-a-component-has-children-and-a-box.md) gave `api/ui` a box model, which is
what a HUD needs: a tree built once and kept in step with the game. A developer tool is the
other shape. Its panels read live state every frame — a unit's hit points, whose turn it is,
what a scene's mission is — and a retained tree of those is a synchronisation problem whose
characteristic defect is a readout showing something that stopped being true two frames ago.

The tree has nothing for it. Every component in `api/ui` is retained, built by a loader from a
JSON document, and answered for by name. There is no way to write a panel as a function of the
state it reads, which is how every debugging overlay in every engine is written and is what
Dear ImGui is for.

The premise is nonetheless already recorded here as somebody else's decision.
[ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md) says drawing is what lays the ui out,
and `ComponentRenderer` says in its own docblock that "every component is left holding the
bounds it was drawn in, which is what the cursor is tested against". That is immediate mode,
written down — a widget's box is a fact about the frame that drew it.

## Decision

**`v3d::ui::Immediate` is a second way to write a ui, drawing onto the same
`realtime::Canvas` the retained components draw onto.** A panel is a sequence of calls between
`begin()` and `end()`; a widget is placed where a layout pen has got to, hit tested against the
box it was just drawn in, and answers on the spot. Nothing is retained between frames except
which widget the cursor is on, which one a press went down on, and the few bits a widget cannot
recompute — a tab bar's selected tab and a window's collapsed flag.

## Alternatives Considered

### Alternative 1: An immediate mode layer over the shared canvas — **chosen**
- **Pros**: A panel is a function of the state it reads, so it cannot show something stale.
  It shares the canvas, so it costs the frame no pass and no draw of its own, which keeps
  [ADR-0005](0005-one-batched-quad-primitive.md) whole. It shares the `Measure` and `Write`
  callbacks and the box drawing with the retained side, so the two look like one ui. And it
  makes `api/ui` a replacement for two third-party toolkits rather than one.
- **Cons**: Two ways to write a ui in one library, and no rule in the code says which to reach
  for. A widget answers the cursor with the box it had *last* frame in the one case that
  matters — deciding what is hovered when panels overlap — so a widget that moved is hovered a
  frame late.
- **Why not**: n/a — chosen.

### Alternative 2: Build the devtools as retained components on ADR-0034's box model
- **Pros**: One way to write a ui, and no new concepts at all. The box model is already there.
- **Cons**: Every panel becomes a tree to build once and update by hand, and the update is the
  hard part: a row per entity, appearing and disappearing as entities do. The characteristic
  defect of that code is a stale readout, which is precisely what a debugging tool must not
  have.
- **Why not**: It trades a real capability for a nominal simplification, and it is the more
  code of the two.

### Alternative 3: Keep Dear ImGui as a dependency for tools, and use `api/ui` for game ui
- **Pros**: A mature, complete toolkit, and no new code at all.
- **Cons**: A second ui with a second frame model, a second font path, a second backend to
  keep against the renderer, and its output in a different pass from everything else. The seam
  between the two is a thing to maintain forever.
- **Why not**: A consumer of this api removes it for exactly that reason, and re-adding it
  here would be adding the problem the consumer is leaving.

### Alternative 4: Make the retained components immediate underneath, so there is one model
- **Pros**: Genuinely one model, and the most elegant answer on paper.
- **Cons**: A theme, a JSON document and a component looked up by name are all retained by
  nature; the editor's menu bar is built once from a file and asked for by name at run time.
  Rewriting that as calls per frame would mean the app holding the menu tree instead, which is
  the loader moved rather than removed.
- **Why not**: It rewrites what works to unify two things that are wanted for different jobs.

## Consequences

### Positive
- A panel that reads live state is one function, and the devtools of a consuming app port call
  for call rather than being redesigned.
- Windows, a tab bar, a table with a header row, a disabled scope, a selectable row, a
  separator, a scrubbable int and a progress bar exist in the tree for the first time. The
  editor, `voxel` and `odyssey` can each have a debug panel for the cost of calling for one.
- `fillBox` and `plateBox` are the box drawing both ways share, so a rounded panel is the same
  rounded panel whichever side drew it, and a theme's "ui" style dresses both.
- The layer needs no window, no device and no font to be tested: it takes `Measure` and `Write`
  as callbacks like `ComponentRenderer`, so a case can drive a cursor over a button and assert
  what it answered.

### Negative
- **Hover is a frame behind.** Which widget the cursor is over is decided as the frame is
  drawn and used by the next one, which is what lets a window drawn later take the cursor from
  one drawn earlier. A widget that appeared or moved this frame is not hovered until the next.
- **There is no clipping**, so a table wider than its window draws over the edge of it and a
  long panel runs off the bottom. This is the same gap the box model has, and scrolling needs
  it first.
- Two ui models in one library is a thing to explain to every reader of it, and nothing
  enforces the split.

### Risks
- **The id of a widget is its label hashed with the id stack**, so two widgets with the same
  label in the same scope are one widget as far as the cursor is concerned — they share a hover
  and an active state. That is the standard hazard of the model and the standard escape hatch
  is `pushId`/`popId`; it will nonetheless be met the first time a table has two "Kill" buttons
  in it.
- A widget's box is stored per frame in a map keyed by id, and an id that stops being drawn
  leaves an entry behind. The map is cleared of anything not drawn for a frame, so the cost is
  bounded, but a panel that generates ids from changing text churns it.
