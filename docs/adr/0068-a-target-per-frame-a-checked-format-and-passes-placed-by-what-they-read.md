# ADR-0068: Render Targets For A Chain — A Target May Hold An Image Per Frame, A Pipeline Is Checked Against Its Target, And A Pass Is Placed By What It Reads

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0031](0031-a-pass-draws-into-a-target-it-names.md) gave a pass a target and listed three
things it left to the caller. A target is one image, so a pass cannot read what it drew last
frame. A pipeline drawn into a target of another format is not caught at submission. And a pass
is recorded in the order it was created, with `Frame::passBefore` as the way in front of
`Engine3D`'s colour pass. The fourth alternative that record rejected, an image per frame in
flight, was to be revisited "if a target is ever wanted for something a barrier cannot order".
A pass that reads its own previous frame is that: one image cannot be read as last frame and
written as this one in the same frame. [LitScene](../plans/completed/LitScene.md) step 10's post chain
is the first chain of more than two passes, and its order is the frame's to get right.

## Decision

**A `RenderTarget` is built with one image or one per frame in flight.** A pass draws into
the frame's own image. `current()` and `previous()` name the slots, and a reader registers
each slot once and names the one it wants per frame. A target with more than one image starts
each one cleared and readable, so `previous()` can be read on the first frame.

**The recorder checks a pipeline's formats against what it draws into** when it binds it, where
both sides state one, and throws naming the pass.

**A pass declares the targets it reads with `Pass::reads()`.** The frame records every pass
that writes a target before every pass that reads it, and otherwise keeps the order the passes
were created in. A cycle throws. `Frame::passBefore` is deleted.

This amends ADR-0031.

## Alternatives Considered

### Alternative 1: Two targets, swapped by the caller
- **Pros**: No change to `RenderTarget` at all. ADR-0031's own answer.
- **Cons**: Every reader of last frame writes the same swap. The pass's target changes every
  frame, so the recorder sees two unrelated targets and cannot say which one a pass meant.
- **Why not**: It is the same images with the bookkeeping moved into every consumer.

### Alternative 2: The order stays the caller's, with `passBefore` and friends
- **Pros**: Nothing to compute. Whoever builds the frame already knows the chain.
- **Cons**: The order is spread across whoever adds a pass, and `Engine3D` creates its pass
  before an app says anything. Getting it wrong reads stale contents and reports nothing.
- **Why not**: The recorder already finds a target's writers by identity. Adding readers
  means the frame can place passes, not just check them.

### Alternative 3: Readers found from the draw items' materials
- **Pros**: No declaration to forget. A material names a texture that names a target's image.
- **Cons**: A scene set at set 2 names its shadow map outside any material, and `Resources`
  would need to map an image back to its target.
- **Why not**: The one reader step 8 added would already be missed.

### Alternative 4: A slot count, a format check and declared reads — **chosen**
- **Pros**: The writer-and-reader scan works per image as it did per target. The check costs a
  comparison per pipeline bind. The order is stated once, where each pass is made.
- **Cons**: A reader that forgets `reads()` is recorded where it was created, which is today's
  behaviour and no worse. A two-slot target spends twice the memory.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- A pass can read what it drew last frame, which a temporal effect or a feedback buffer needs.
- A shadow map written per slot lets one frame's shadow pass overlap the previous frame's lit
  pass, where one shared image serialises them with a barrier.
- `Engine3D` creating its colour pass first no longer decides where an app's passes go.

### Negative
- A reader of a two-slot target holds one handle per slot and chooses between them each frame.
- Validation already reports a format mismatch when its layers are on. The check duplicates it
  so that a release build, or a test, sees the mistake by name rather than as a wrong picture.
- Creating a multi-slot target submits once and waits, to leave its images readable.

### Risks
- A reader that names no `reads()` still depends on creation order. Synchronization validation
  catches it only as a hazard; the escape hatch is that `reads()` is one line where the pass is
  made.
- A target's slot is chosen by the ring's frame index, so a frame built for one index and
  recorded under another would draw into the wrong slot. The index moves only after a submit,
  so a frame built during the tick and recorded at its end, as an engine does, keeps one.
