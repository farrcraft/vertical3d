# ADR-0215: Offline: a pixel's noise is the difference of two halves

**Status**: accepted
**Date**: 2026-11-09
**Tags**: offline
**Documented in**: [offline/Integrator.md](../../offline/Integrator.md)

## Context

Adaptive sampling stops a pixel once its noise falls below a threshold. Each device gives the
same picture on every run, so the decision to stop can depend only on the samples, never on
which thread finished first. ZSobol gives a pixel's samples as runs of a power of two, and each
first half of a run is stratified as a whole run is. Stratified samples converge faster than
independent ones, so a variance computed as if they were independent overstates the error. An
artist sets the threshold, so it has to mean the same in a dark pixel as in a bright one.

## Decision

A second beauty takes the first half of each run of a pixel's samples, and a pixel's error is the
difference of its two estimates, normalised by brightness as Cycles measures it. A pixel is
checked only between passes, and stops when its error and its four neighbours' errors are all
below the threshold. The threshold is in the working space.

## Alternatives

### The variance of a pixel's mean, from its second moment
- **For**: No second beauty, since the film already holds the second moment. RenderMan's
  default metric and Arnold's threshold are of this kind.
- **Against**: Under ZSobol it overstates the error, so pixels that are clean keep sampling. Its
  estimate is poor while a pixel has few samples.
- **Rejected because**: It measures the noise of independent samples, which moya does not take.

### Each pixel stopped on its own error alone
- **For**: The simplest rule, and no pixel takes samples it does not need.
- **Against**: A pixel whose two halves agree by chance stops early, and leaves a speck of noise
  among converged neighbours.
- **Rejected because**: Cycles spreads its check to neighbours for that reason, and the cost is a
  few more samples near noise.

### A threshold after the display transform
- **For**: The error is measured as the viewer sees it.
- **Against**: The display transform is OpenColorIO's, which kernel code does not run, and it
  changes with the view an artist picks.
- **Rejected because**: A threshold has to mean the same under every view.

## Consequences

- **Gains**:
  - The error follows how far a pixel's estimate is from converging, under stratified samples.
  - The same pixels stop on every run and on every thread count.
  - A threshold means what it means in Cycles, which artists already know.
- **Costs**:
  - Each pixel holds three more floats.
  - A pixel is not checked until both halves hold a few samples, so a minimum count comes first.
  - Pixels beside noise take more samples than their own error needs.
- **Revisit when**: a denoiser's own estimate of its error is a better stopping rule than the
  noise of the samples it is given.
