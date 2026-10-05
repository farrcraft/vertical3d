# ADR-0021: Audio: use SDL3_mixer

**Status**: amended
**Date**: 2026-09-04
**Amended by**: [ADR-0079](0079-assets-loaders-are-registered.md)
**Documented in**: [api/Engine.md](../api/Engine.md)

## Context

`api/audio` needs a backend that opens a playback device and plays short clips. The tree already
uses SDL3 for the window, input and events. SoLoud, the vendored backend, has no SDL3 backend and
no upstream activity, so it cannot open a device in an SDL3 build. The games play wav files as
one-shot effects, and nothing asks for positional audio.

## Decision

`api/audio` plays sound through SDL3_mixer, taken from vcpkg rather than vendored. `audio::Engine`
owns the mixer and one playback device, and a clip loads without an engine, so loading a file and
playing it stay separate.

## Alternatives

### miniaudio
- **For**: a single public-domain header with no library to link. It also carries spatial audio.
- **Against**: it is a second platform layer beside SDL, with its own device enumeration and its
  own audio thread.
- **Rejected because**: the link weight of SDL3_mixer is paid once, and a second platform layer is
  a cost on every later change to devices or threading.

### Rebuild the vendored SoLoud against a native backend
- **For**: the smallest change. Only the submodule's build flags move, and `winmm` or `wasapi`
  would open a device.
- **Against**: it keeps the tree on a project with no SDL3 support and no maintenance. It also
  keeps a prebuilt library committed under `vendor/` and a CI step to build it.
- **Rejected because**: it keeps the arrangement that let a backend built for SDL2 sit unnoticed in
  an SDL3 tree.

### OpenAL Soft
- **For**: mature, and built around 3D positional audio.
- **Against**: contexts, buffers and sources to play a few one-shot effects. It is LGPL.
- **Rejected because**: nothing in the tree needs positional audio.

## Consequences

- **Gains**:
  - Audio goes through the same platform layer as the window and input.
  - `vendor/soloud`, its committed library and its CI step are gone.
  - SDL3_mixer decodes wav, mp3 and ogg, and plays a one-shot clip in one call.
- **Costs**:
  - SDL3_mixer is a linked library, not a header, so every target that links `v3dlib_audio`
    carries it.
  - The port needed a newer vcpkg baseline than the tree had pinned, so every dependency moved
    with it.
  - Opening a device needs audio hardware, which a CI runner does not have. The tests cover
    loading and configuration but never open a device.
- **Revisit when**: a game needs positional audio, or SDL3_mixer stops being maintained.
