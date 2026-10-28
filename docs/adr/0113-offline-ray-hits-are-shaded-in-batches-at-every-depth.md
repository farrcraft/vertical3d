# ADR-0113: Offline: ray hits are shaded in batches at every depth

**Status**: accepted
**Date**: 2026-10-25
**Amends**: ADR-0093
**Documented in**: [offline/RayTracing.md](../offline/RayTracing.md)

## Context

The shading machine is most of a ray-traced render, and every run of it pays a fixed cost before
it shades a point. Run once for each hit and once for each light on it, that cost is paid many
times over for every pixel sample. A shader's `trace()` and a ray seen through a surface each
start further hits, and a scene that reflects or refracts has as many of those as of primary
hits. A batch has to keep each hit's derivatives, shadows, time and compositing order its own,
so that a picture does not change.

## Decision

The tracer traces and shades rays in batches. The hits of a batch that share a surface shader,
its placement, an object space and their lights are shaded by one run. The rays that run traces,
and the rays that continue through a surface, are the next batch. The ray hider casts
each row pass by pass, every unsettled pixel's samples as one batch.

## Alternatives

### A row's first hits as one batch, and every other hit alone
- **For**: the hider gathers its batch in one place, and the tracer's recursion is unchanged.
- **Against**: traced rays and surfaces seen through stay one hit to a run, and in a scene that
  reflects or refracts they are as many as the primary hits.
- **Rejected because**: it leaves the per-run cost on the rays that grow fastest with a scene's
  depth of tracing.

### One hit to a run, with a cheaper machine
- **For**: no change to the tracer or the hider, and a faster machine helps the reyes hider too.
- **Against**: a fixed cost per run remains however small it is made, and is still paid once per
  hit and light.
- **Rejected because**: only a batch spreads that cost, and a faster machine adds to a batch's
  gain rather than replacing it.

### Every ray of a bucket, sorted by shader, deferred until the bucket is traced
- **For**: the widest batches and the most coherent texture reads.
- **Against**: SL's `trace()` returns its colour within the shader's run, so a run cannot be
  deferred past the rays it traces.
- **Rejected because**: the language's synchronous `trace()` rules it out.

## Consequences

- **Gains**: the per-run cost is spread over up to 64 hits at every depth. A batch gives each ray
  the same answer, to the bit, as the ray alone, so the reference pictures do not change.
- **Costs**: a callback has no lane mask, so a run traces a ray for every hit, including a hit
  whose branch skipped the call. Primitives whose matrices differ only by rounding are shaded
  in separate runs. A batch holds the hits, poses and register files of a row of samples at
  once.
- **Revisit when**: a scene's batches are mostly of one hit, so the grouping costs more than it
  saves, or the machine gains a mask that a callback could read.
