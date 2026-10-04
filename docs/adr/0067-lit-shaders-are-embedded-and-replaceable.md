# ADR-0067: Lit Shaders — The api's Are Embedded, A Consumer May Hand In Its Own, And The Shared Blocks Are One Include

**Date**: 2026-10-03
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The tree embeds its shaders at build time with `v3d_add_shader`. A shader on disk beside an
executable goes stale silently, and CMake copies no engine data into an app's build tree.
retcon loads its lit shaders from a directory instead, so that a shader can be swapped without a
relink, and its look is its shaders as much as its parameters. Its handoff lists the other cost of
its arrangement: the camera block is written out in three shaders and the scene block in two, and
a member changed in one and not the others is a layout mismatch with no diagnostic.
[LitScene](../plans/completed/LitScene.md) brings the lit tier into the api, and retcon recorded that its
look stays its own.

## Decision

**The api's lit shaders are compiled and embedded like the rest. `renderer::Lit` takes its
shader modules as SPIR-V words, defaulting to the embedded ones**, so a game that loads its own
from disk hands over what it loaded. A replacement declares the same sets and push block, which
is the contract, and pipeline creation's validation is the check. **The blocks every lit shader
shares are one file, `shaders/lit/lit.glsl`, included by each**, and `v3d_add_shader` gives glslc
a dependency file so an edit to the include rebuilds the shaders that use it.

## Alternatives Considered

### Alternative 1: Load the api's shaders from a directory, as retcon does
- **Pros**: A shader is swapped without a relink.
- **Cons**: The directory has to be found at run time and kept beside every executable, and a
  stale file is a wrong picture with nothing to say why.
- **Why not**: It is the problem embedding was chosen to remove.

### Alternative 2: Embedded only, with nothing replaceable
- **Pros**: One way to get a shader.
- **Cons**: A game's look is its shaders, and retcon's would then be the api's.
- **Why not**: The tier would be retcon's look rather than a tier retcon's look runs on.

### Alternative 3: Embedded by default, handed in when wanted — **chosen**
- **Pros**: No consumer needs a directory, and one that wants one has it.
- **Cons**: A replacement's agreement with the layout is checked only when the pipeline is built.
- **Why not**: n/a — chosen.

## Consequences

### Positive
- The camera, scene and object blocks are written once.
- retcon keeps loading its shaders from its own directory.

### Negative
- `renderer::Lit` has a constructor argument that nothing in this tree passes.

### Risks
- A replacement that declares a block the api's do not, or reads a binding the layout lacks,
  fails at pipeline creation. One that only reorders members fails at nothing. The include is
  there for a replacement to use as well, which is the mitigation.
