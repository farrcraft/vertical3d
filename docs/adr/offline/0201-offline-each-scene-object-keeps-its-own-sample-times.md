# ADR-0201: Offline: each scene object keeps its own sample times

**Status**: accepted
**Date**: 2026-11-07
**Tags**: offline, interchange
**Documented in**: [offline/Scene.md](../../offline/Scene.md)

## Context

A transformation or a geometry's points can carry several samples over the shutter. RIB's motion
blocks and USD's time samples give times as they were authored, which differ from object to
object, and Riley keeps each object's times as given. OptiX and Embree traverse motion with keys
spread evenly over a time range, and Cycles takes a count of even steps. Interpolating a matrix
element by element shears a rotation, so Cycles, pbrt and OptiX decompose a transformation before
interpolating it.

## Decision

Each transformation and each sampled geometry holds its own times, as its client gave them. A
consumer that needs even keys, as the BVH and a GPU's traversal do, resamples them when it builds.
A transformation is interpolated between samples by decomposing it into a translation, a rotation
taken the shortest way, and a scale.

## Alternatives

### The scene fixes one set of even times over the shutter
- **For**: Matches what OptiX and Embree traverse, so the BVH builds from the scene's times as
  they are. Every object has the same number of samples.
- **Against**: Every client resamples every motion, and a motion whose samples are not even is
  approximated before the renderer sees it. A scene of mostly still objects pays for as many
  samples as its fastest one needs. Giving each object a count of its own avoids that, and is
  then a time of its own in all but name.
- **Rejected because**: It moves resampling into every client and loses the given times, when
  the BVH can resample once.

### Interpolate the elements of the matrices
- **For**: Cheap, and exact for a translation.
- **Against**: A rotation between two samples shrinks and shears the object.
- **Rejected because**: A rotating object must keep its shape.

## Consequences

- **Gains**:
  - A client hands over motion exactly as it holds it, and nothing is lost before the renderer.
  - An object that does not move holds one sample, whatever its neighbours do.
  - Each consumer chooses its own resampling, so a GPU's even keys do not constrain the CPU's.
- **Costs**:
  - The BVH and each device resample motion when they build, and a resampled motion differs
    slightly from the given one between samples.
  - Two consumers that resample differently see slightly different motion.
  - A shear or a scale that is not uniform does not survive the decomposition exactly.
- **Revisit when**: every device moya targets traverses keys at arbitrary times, or clients turn
  out to give even times in every case.
