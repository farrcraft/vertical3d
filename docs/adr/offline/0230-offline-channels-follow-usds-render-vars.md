# ADR-0230: Offline: channels follow USD's render vars

**Status**: accepted
**Date**: 2026-11-13
**Tags**: offline, compositing, interchange
**Documented in**: [offline/Scene.md](../../offline/Scene.md)

## Context

A compositor asks a renderer for channels beside the beauty: values at the first hit, a primvar,
or the light of the paths an expression names. moya's scene is filled by three clients: RIB,
which names a channel by RenderMan's `DisplayChannel`, the editor, and a Hydra delegate, which
hands over USD's render vars. A depth, a position or an ID averaged by the pixel filter gives a
value that no surface holds. Every production renderer lets a channel choose how its samples are
combined, and USD's core schema leaves that to each renderer.

## Decision

A channel names a kind of source and a name, as UsdRender's `RenderVar` does: a value the
renderer computes, a primvar, or a light path expression. Each channel also names how its
samples are combined into a pixel: the filtered mean, the closest, the least, the most or the
sum. The default follows the source, so depth, position and IDs take the closest and every other
value the filtered mean.

## Alternatives

### RenderMan's display channels
- **For**: RIB reads as written, and one string names any source.
- **Against**: The kind of source is hidden inside the string, so a wrong one is found only when
  the render parses it. A Hydra delegate joins a kind and a name into a string, which the render
  then splits again.
- **Rejected because**: USD's richest client hands over a kind and a name, and RIB reduces to
  them as easily.

### A fixed set of passes
- **For**: Each pass is known ahead, cheap and simple to name.
- **Against**: No primvar and no light path expression can be asked for. Cycles' Hydra delegate
  skips every render var outside its set.
- **Rejected because**: A compositor's channels are open-ended.

## Consequences

- **Gains**:
  - A Hydra delegate passes USD's render vars through as they are.
  - A depth or ID channel is never averaged by accident.
  - A source of the wrong kind is refused when the scene is built.
- **Costs**:
  - The RIB client splits RenderMan's `lpe:` prefix and its built-in names into kinds.
  - The combine rule is moya's own, since USD holds it only in each renderer's namespace.
  - The closest sample needs a fixed tie-break, by sample index, to give one picture on every
    run.
- **Revisit when**: USD's render schema gains a combine rule of its own.
