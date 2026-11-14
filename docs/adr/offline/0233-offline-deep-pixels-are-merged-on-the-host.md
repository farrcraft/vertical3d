# ADR-0233: Offline: deep pixels are merged on the host

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, compositing
**Documented in**: [offline/Film.md](../../offline/Film.md)

## Context

A deep EXR file holds a list of samples for each pixel, each with its depth, colour, alpha and
IDs, and flattening the list has to give the flat picture. OpenEXR fixes the file, and not how a
renderer builds and merges its lists. A camera sample keeps one surface, so each sample gives one
depth, one radiance and one weight. Volumes will add a sample for each stretch of a ray inside
them, and Octane's bounded bins are documented to cut volumes off. Kernel code allocates nothing,
and each device has to give the same picture on every run.

## Decision

Each camera sample records its depth and radiance beside the IDs and weight that Cryptomatte
records, and after each pass the host merges each pixel's records, in sample order, into a list.
Two samples are joined when their IDs agree and their depths lie within a tolerance, as Arnold,
RenderMan and Redshift merge them. A surface is a point sample whose alpha is its weight over the
weight that reached its depth, its IDs are OpenEXR's Deep IDs, and the file is written tidy.

## Alternatives

### A bounded list of bins in each pixel
- **For**: The bins stay on the device in fixed memory, as Octane keeps them.
- **Against**: Bins of 32 samples take 2 GB for a 1080p frame and over 8 GB at 4K. A pixel of
  more surfaces or volume stretches than bins loses its depth.
- **Rejected because**: It fails on volumes and dense layers, which are what deep output is for.

### The first hit alone
- **For**: The smallest file, and the simplest to build.
- **Against**: Cut-out cards stacked along a ray flatten wrongly.
- **Rejected because**: Its flattening does not give the flat picture.

## Consequences

- **Gains**:
  - Flattening gives the flat picture, up to the tolerance.
  - A list grows with the surfaces and stretches in a pixel, not with its samples.
  - Deep output and Cryptomatte share one buffer of records.
- **Costs**:
  - The host merges records after every pass, and a GPU copies them back.
  - The tolerance decides what is kept apart, and a scene may need its own.
  - A checkpoint has to hold each pixel's list.
- **Revisit when**: A GPU's copy back costs a significant share of a pass.
