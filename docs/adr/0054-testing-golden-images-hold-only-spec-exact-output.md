# ADR-0054: Testing: golden images hold only spec-exact output

**Status**: accepted
**Date**: 2026-09-12
**Amends**: [ADR-0007](0007-ci-render-tests-on-software-vulkan.md)
**Documented in**: [Testing.md](../contributing/Testing.md#realtime-references)

## Context

`vulkan::frame::Capture` reads a presented or offscreen frame back into an image, so that it can
be compared against a committed reference. Render tests run on two conformant Vulkan
implementations: a developer's GPU and lavapipe in CI
([ADR-0007](0007-ci-render-tests-on-software-vulkan.md)). A reference made on one and compared on
the other is only stable if the specification leaves no room for the two to differ. Rasterizing
an axis-aligned shape on whole-pixel boundaries is fixed exactly. Blending, filtered sampling,
multisample resolve and most interpolation are specified only to a precision.

## Decision

A committed realtime reference may contain only output that the Vulkan specification fixes
pixel for pixel, and it is compared at a tolerance of zero. A case that draws anything else
asserts validation silence and checks chosen pixels, and has no reference.

## Alternatives

### One reference and a tuned tolerance
- **For**: Every case can have a picture, including blended and filtered ones. The offline
  renderer's tests work this way, at a tolerance of 1.
- **Against**: The offline tests compare two runs of the same CPU code, where the tolerance
  absorbs floating-point rounding. Here it would absorb the difference between two
  implementations, which has no known bound. Nobody can derive the right number, and a real
  regression smaller than it is invisible.
- **Rejected because**: A threshold chosen to make a test pass is what the test then asserts.

### A reference per implementation, selected by device name
- **For**: Every case can be pinned exactly on each device, including blending and filtering.
- **Against**: Two or more files per case, and a picture is checked only on the device that
  produced it. An unrecognised device asserts nothing, and the set grows with every GPU the
  project runs on.
- **Rejected because**: It multiplies what is meant to be one statement of what the renderer
  draws.

### Produce references on lavapipe and compare only on lavapipe
- **For**: Stable by construction, and a developer's GPU never fails a build.
- **Against**: It checks the API against a software implementation and says nothing about a
  driver. The suite would also assert different things locally and in CI.
- **Rejected because**: A picture is pinned to catch what validation cannot see, on the device
  that actually draws it.

## Consequences

- **Gains**:
  - A reference made on one implementation is a real check on every other.
  - A wrong colour, a wrong transform or a transposed texture fails a test. Validation catches
    none of these.
  - Whether a case may have a reference is settled by the rule, not argued per case.
- **Costs**:
  - Blending, filtering, multisampling and text have no reference coverage.
  - A case has to be designed around what may be pinned, which is not how the renderer is used
    anywhere else.
  - Every sampler in the tree filters linearly, so a textured reference must draw one texel per
    pixel.
  - Each reference is a binary file, and a deliberate renderer change means regenerating it.
  - The easiest response to a failure is to replace the reference with what was drawn. The test
    writes its output to `data_out/` and never to the committed file, but nothing else prevents
    that.
- **Revisit when**: a reference differs between two conformant implementations, which would mean
  the rule admits more than the specification fixes. The case then drops its reference and keeps
  its spot checks. A nearest-filtering sampler would also widen what can be pinned.
