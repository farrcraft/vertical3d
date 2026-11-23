# ADR-0255: Offline: each picture is denoised once it holds enough samples

**Status**: accepted
**Date**: 2026-11-20
**Tags**: offline, editor
**Documented in**: [offline/Interactive.md](../../offline/Interactive.md)

## Context

An interactive picture is noisy for most of its life, and a denoised one shows the user what a
converged render will look like long before it converges. moya denoises through OIDN once a render
ends, filtering the albedo and normal first, and that costs a share of a pass. A denoise that runs
while sampling continues takes time from the samples, and a denoised picture of too few samples
smears detail. Cycles' viewport denoises every picture from a start sample the user sets, with a
prefilter that trades the cleaning of albedo and normal for speed. Arnold runs OIDN or OptiX as an
imager that refreshes while an interactive render runs.

## Decision

A session whose caller turns denoising on denoises every picture whose pixels hold at least a start
number of samples, which the caller sets, and gives it beside the raw picture. An interactive
picture is denoised at OIDN's fast quality from the albedo and normal as they are. The picture that
ends a render is denoised as a batch render's is, with the albedo and normal filtered first.

## Alternatives

### Denoised when sampling ends or pauses
- **For**: Sampling never gives up time to the denoiser, and every denoised picture is of a settled
  render.
- **Against**: The user sees noise for as long as the render improves, which in an editing session
  is nearly always. A pause has to be guessed from the edits that stopped.
- **Rejected because**: The denoised picture is most useful while the user is still deciding, which
  is while the render runs.

### Denoised picture by picture, when the caller asks
- **For**: moya holds no rule, and each client chooses its own.
- **Against**: Every client has to time the denoiser against the passes itself, and a request made
  during a pass waits for it. The editor's render view and a Hydra delegate would each write the
  same schedule.
- **Rejected because**: A picture is made, and its samples counted, in the session, so the rule
  belongs there.

## Consequences

- **Gains**:
  - A rendered edit looks like its converged result within a few passes.
  - The raw picture's bytes do not depend on whether denoising runs, so the oracle is unchanged.
  - The final picture is the batch render's denoised picture, byte for byte.
- **Costs**:
  - Each denoised picture takes time the samples would have had.
  - A picture at fast quality smears fine detail that the final picture keeps.
  - A session holds the denoiser's filters and buffers for as long as denoising is on.
- **Revisit when**: a denoiser runs on another device beside sampling, so that it costs the samples
  nothing.
