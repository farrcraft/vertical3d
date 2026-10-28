# ADR-0120: Editor: a frame is evaluated, not posed into items

**Status**: accepted
**Date**: 2026-10-26
**Documented in**: [editor/Animation.md](../editor/Animation.md)

## Context

The views, the picker, the manipulators, the render window and the batch all need the scene's
values at some time. The batch, motion blur and a live render that keeps running while the user
scrubs each need several times at once. A document's save, its revisions and its history
describe what the user authored. An animator edits a keyed property at the
current frame and expects to see the edit before deciding whether to key it. Later the
evaluation also has to carry what constraints, drivers and a modelling history produce.

## Decision

An item holds only its authored values, and the scene evaluates a time into a value that holds
every item's placement and keyed properties at that time, which every reader takes. A curve
gives a keyed property its value, except that a keyed property edited since the frame was last
set shows the edit until the frame changes again or a key takes it. Evaluating changes nothing
in the document.

## Alternatives

### An evaluated copy of the scene
- **For**: readers keep reading items, and evaluated geometry has an obvious home.
- **Against**: an item's id is its identity and cannot be shared by a copy. Every writer has to
  find its way back from a copy to the original. Each time evaluated copies every mesh,
  whether anything changed it or not.
- **Rejected because**: two sets of items that look alike invite reading or writing the wrong
  one, and an evaluation that is a value carries geometry as well when that is needed.

### Pose the current frame into the items, evaluate other times apart
- **For**: interactive readers stay as they are, and an edit shows because it is in the item.
- **Against**: a save writes posed values as authored ones, playback moves revisions, and a
  value is read one way in the views and another in the batch.
- **Rejected because**: it keeps the document holding values it did not author.

### Every edit to a keyed property sets a key
- **For**: there is no pending state, and nothing is lost on a frame change.
- **Against**: every nudge made while looking at a frame adds a key to the animation.
- **Rejected because**: keying is a choice the animator makes, so it can be added later as a
  choice but cannot be the rule.

## Consequences

- **Gains**:
  - Any number of times can be evaluated at once without touching the document, so the batch,
    shutter samples and a background render read the same rule as the views.
  - A save, a revision and an undo describe only what the user did.
  - The evaluation is where constraints, drivers and evaluated geometry go.
- **Costs**:
  - Every reader of a placement, a lens or an emission takes it from an evaluation, and a
    manipulator starts a drag from evaluated values rather than the item's own.
  - The scene keeps a copy of each item's values as they were when the frame was last set, to
    tell a pending edit from an authored one.
  - A pending edit is not part of the animation, so the batch does not render it.
- **Revisit when**: keyed edits should be kept across frame changes, for example by animation
  layers, or evaluating every item on every read becomes too slow.
