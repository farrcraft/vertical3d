# ADR-0216: Offline: a checkpoint is an EXR of the film's sums

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline, files
**Documented in**: [offline/Film.md](../../offline/Film.md)

## Context

A long render has to survive being stopped, and continue where it stopped on the same or another
machine. A resumed render has to give the same picture as one that never stopped, as every run of
a device does. The film holds weighted sums of floats, and resolving a pixel divides them by its
weight, which multiplying back does not undo exactly. An output may be written at half
precision. A resume against a scene that changed has to be refused, and a file's name and time
do not show whether its contents changed.

## Decision

A checkpoint is an OpenEXR file of its own that holds the film's unresolved sums as float
channels. The channels are the beauty, every channel, the weights, the sample counts and the half
beauty, with the pass count in the header. The header also holds a hash of the scene's description
and of the film's channels, and a resume whose hash differs is refused.

## Alternatives

### The output as the checkpoint
- **For**: One file per render, as RenderMan's output EXR carries the state it recovers from.
- **Against**: An output holds resolved values, and may hold halves. The sums rebuilt from them
  differ from the film's in their last bits.
- **Rejected because**: A resumed render would not be the same as one that never stopped.

### A file of moya's own
- **For**: The film's memory written as it is: the smallest file and the fastest write, as
  V-Ray's resumable files are.
- **Against**: No other tool opens it, so a stopped render cannot be looked at, and its layout
  is moya's to keep readable across versions.
- **Rejected because**: OpenEXR holds the same floats in a form every compositor opens.

### Ranges of samples rendered apart and merged
- **For**: No checkpoint at all. Cycles' start sample splits a render across machines, and a
  merge adds the pieces.
- **Against**: A render stopped part way through a range loses that range. A merge needs the
  sums and the weights, so each piece has to hold them anyway.
- **Rejected because**: It splits a render ahead of time, and does not resume one that stopped.

### The scene file's name and time
- **For**: Cheap to check, as Karma checks a USD file's time.
- **Against**: A file copied, touched or rewritten unchanged is refused. A scene built by the
  editor has no file.
- **Rejected because**: The scene's description shows what the render reads, from any client.

## Consequences

- **Gains**:
  - A resumed render is the same, byte for byte, as one that never stopped.
  - A stopped render opens in any EXR reader.
  - A resume against a changed scene, or a film of other channels, is refused from any client.
- **Costs**:
  - A checkpoint is a second file beside the outputs, of floats, larger than they are.
  - Hashing the scene's description reads the whole scene once when a render starts.
  - A description that prints a parameter in a new way refuses every checkpoint made before.
- **Revisit when**: deep images need a checkpoint of their own, or writing the sums
  takes a visible share of a render.
