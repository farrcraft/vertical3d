# ADR-0234: Offline: denoised channels sit beside noisy ones

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, compositing, testing
**Documented in**: [offline/Output.md](../../offline/Output.md)

## Context

OIDN denoises a picture from its beauty, guided by an albedo and a normal, and can filter the
guides first so that they carry no noise of their own. A compositor rebalances light groups and
expression channels after rendering, and a sum of noisy channels is noisy whatever the beauty
is. OIDN does not promise the same output under different thread counts, though its own tests
check that repeated runs agree. Each device has to give the same picture on every run, and moya
already builds its CPU path for each instruction set.

## Decision

OIDN denoises the beauty, and any expression channel or light group that a scene marks, each into
a channel of its own beside its noisy original. Every denoised channel is guided by one albedo
and one normal that OIDN filters first, and moya gives the input's scale itself. A denoised
channel is held to the same picture on every run, and OIDN runs on a fixed count of threads of
its own if its output otherwise depends on how many it is given.

## Alternatives

### The beauty alone
- **For**: One filter run for each picture.
- **Against**: A light group rebalanced in a compositor is as noisy as it was rendered.
- **Rejected because**: Light groups exist to be rebalanced, as RenderMan, Arnold and V-Ray
  denoise them for.

### The beauty denoised in place
- **For**: One beauty, and less memory.
- **Against**: The noisy beauty is lost, and the beauty no longer equals the sum of its channels.
- **Rejected because**: It takes the exact record away from the picture.

## Consequences

- **Gains**:
  - A rebalanced composite stays clean.
  - The noisy channels remain the exact sums of the render.
  - One pair of filtered guides serves every denoised channel.
- **Costs**:
  - Each denoised channel is a filter run and a channel more in the file.
  - Whether OIDN's output depends on its thread count has to be tested, and a fixed count costs
    its speed on a large machine.
  - IDs and deep samples are not denoised.
- **Revisit when**: OIDN promises the same output under any thread count, or a GPU device
  denoises.
