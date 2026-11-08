# ADR-0205: Offline: the hierarchy is eight wide at full precision

**Status**: accepted
**Date**: 2026-11-08
**Tags**: offline
**Documented in**: [roadmap/MoyaRebuild.md](../../roadmap/MoyaRebuild.md)

## Context

The CPU traverses moya's own hierarchy, testing a node's children at once with SIMD. AVX2 tests
eight boxes in one instruction and SSE four. A GPU traverses its vendor's nodes, since HIP RT
converts to four wide and OptiX takes none, so the node layout serves the CPU alone. Child boxes
quantized to a few bits against the node's own box take about half the memory, and cost speed to
decode.

## Decision

Each node of the hierarchy has up to eight children, with each child's box at full precision.
The hierarchy is built binary and collapsed to eight wide.

## Alternatives

### Four children per node
- **For**: Fills SSE's lanes exactly.
- **Against**: A deeper tree with more nodes, slower than eight wide from AVX2 up.
- **Rejected because**: AVX2 is the common case, and SSE tests eight children in two
  instructions.

### Four or eight, chosen by instruction set
- **For**: Each instruction set gets the width it fills, as Embree chooses.
- **Against**: Two node layouts and two traversals to keep fast and tested.
- **Rejected because**: Eight wide serves every instruction set moya runs, and Cycles dropped a
  second width of its own for the same cost.

### Child boxes quantized against the node's box
- **For**: About half the memory per node, as the compressed wide hierarchy and Embree's
  quantized nodes show.
- **Against**: Each traversal step decodes the boxes, and a quantized box is looser than the
  child it bounds.
- **Rejected because**: The traversal is held to Embree's speed, and Embree's default layout is
  at full precision.

## Consequences

- **Gains**:
  - One node layout and one traversal shape for every instruction set.
  - The tightest boxes and the fastest step per node.
  - The build is width-agnostic until its last pass.
- **Costs**:
  - A node takes about twice the memory of a quantized one, which matters most in the largest
    scenes.
  - On SSE a node's test takes two instructions.
- **Revisit when**: a scene's hierarchy is too large for memory, or a wider SIMD test makes a
  sixteen-wide node pay.
