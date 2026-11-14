# ADR-0232: Offline: Cryptomatte coverage is merged on the host

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, compositing
**Documented in**: [offline/Film.md](../../offline/Film.md)

## Context

Cryptomatte fixes how an ID is hashed from a name, how ranks are written and what the manifest
holds, but not how a renderer gathers each ID's coverage of a pixel. A camera sample keeps one
surface, since a cut-out surface passes or stops it by a draw, so each sample gives one ID per
layer and one weight. Hair, particles, motion blur and defocus put many IDs in a pixel, and
those are the pixels a compositor mattes most. Kernel code allocates nothing, and each device has
to give the same picture on every run.

## Decision

Each camera sample records its IDs and its filter weight in a buffer of the pass. After each
pass, the host merges each pixel's records in sample order into a list of every ID's coverage,
which a file ranks. A surface seen through
glass is not recorded, and the glass takes its own ID.

## Alternatives

### A bounded table of IDs in each pixel
- **For**: The table stays on the device in fixed memory, as Cycles, Karma and Octane keep one.
- **Against**: Past its size, a table keeps the IDs that arrived first, not the largest. A table
  of ten IDs in three layers takes half a gigabyte for a 1080p frame for the whole render.
- **Rejected because**: It is approximate in the pixels compositors work on most.

### A queue of hits to the host
- **For**: The device holds nothing but the queue.
- **Against**: Appending to a queue needs atomics, and its order changes between runs, so the
  host has to sort it.
- **Rejected because**: The pass's buffer gives the same exactness in a fixed order.

## Consequences

- **Gains**:
  - Coverage is exact for any number of IDs in a pixel.
  - Device memory is bounded by a pass's samples, and a buffer can cover a band of rows.
  - Deep output can read the same records.
- **Costs**:
  - The host merges records after every pass.
  - A GPU copies each pass's records back to the host.
  - A checkpoint has to hold lists of different lengths for each pixel.
- **Revisit when**: A GPU's copy back costs a significant share of a pass.
