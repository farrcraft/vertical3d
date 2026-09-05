# ADR-0021: Audio Backend — SDL3_mixer Replaces SoLoud, And A Clip Resolves Through The Asset Manager

**Date**: 2026-09-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Nothing in the tree has ever made a sound, and covering `api/audio` on 2026-09-04 is what
finally said why. Three faults sat on top of each other: `audio::Engine` handed a config's
filename straight to the backend, so it resolved against the working directory rather than
the asset manager's path and no wav was ever found; `AudioClip::load` returned true whatever
the backend reported, so the miss was silent; and `Soloud::init()` had been failing all
along with `UNKNOWN_ERROR` and no backend, because the SoLoud prebuilt committed under
`vendor/soloud/contrib/Debug` is built against the SDL2 backend and this tree has shipped
SDL3 since the upgrade closed. Fixing the first two exposed the third as an access violation
on exit, since `play()` on a mixer that never started leaves voices `deinit()` then faults
tearing down. SoLoud's last upstream commit is August 2024 and it has no SDL3 backend, so
there is nothing to wait for.

## Decision

`api/audio` is built on SDL3_mixer 3.2.0, taken from vcpkg rather than vendored, and
`vendor/soloud` goes. `audio::Engine` owns `MIX_Init` and one `MIX_Mixer` over the default
playback device; `AudioClip` owns a `MIX_Audio` loaded with a null mixer, so a clip still
loads without an engine and `asset::loader::Wav` keeps working. `audio::Engine::load` takes
a `Resolve` callback and never reaches the asset manager itself, because `v3dlib_asset`
loads through `v3dlib_audio` and the dependency cannot run both ways — the seam
[ADR-0020](0020-a-theme-is-data-and-the-app-resolves-its-images.md) takes for the ui's
images.

## Alternatives Considered

### Alternative 1: SDL3_mixer — **chosen**
- **Pros**: audio goes through the same platform layer as the window, input and events, so
  there is one fewer vendor to reason about and no second opinion about what an audio device
  is; actively maintained by libsdl-org, and 3.0 was a rewrite rather than a patch; decodes
  wav, mp3 and ogg; `MIX_PlayAudio` is the fire-and-forget one-shot every caller in the tree
  wants; refcounted `MIX_Init` and a nullable mixer on `MIX_LoadAudio` let a clip load
  without an engine, which is what keeps the asset loader honest.
- **Cons**: it is a linked library rather than a header, so it rides along on every target
  that links `v3dlib_asset`; the port did not exist at the vcpkg baseline this project
  pinned, so taking it meant moving the baseline and every dependency with it.
- **Why not**: chosen.

### Alternative 2: miniaudio
- **Pros**: a single public-domain header with no libraries at all, which would make the
  `v3dlib_asset` → `v3dlib_audio` PUBLIC link nearly free; present at the old vcpkg
  baseline, so it needed no dependency movement; carries spatial audio the editor might
  eventually want.
- **Cons**: a second platform layer beside SDL for a tree that already has one, with its own
  device enumeration and its own opinion about the audio thread.
- **Why not**: the dependency weight is real but small, and it is paid once. Two platform
  layers is paid forever.

### Alternative 3: Rebuild the vendored SoLoud against a native backend
- **Pros**: cheapest possible change — the repository would not move at all, only the
  submodule's build flags, and `winmm` or `wasapi` would open a device.
- **Cons**: pins the tree to a project with no commits since August 2024 and no SDL3
  support; keeps the prebuilt `.lib` committed under `vendor/`, the `link_directories` entry
  and the CI step that builds SoLoud on its null backend.
- **Why not**: it fixes the symptom by keeping the arrangement that hid it. A library built
  against SDL2 sat in an SDL3 tree for as long as it did precisely because nothing rebuilt
  it and nothing checked what `init()` returned.

### Alternative 4: OpenAL Soft
- **Pros**: mature, and the only candidate with real 3D audio as its centre of gravity.
- **Cons**: contexts, buffers and sources to play three one-shot sound effects; LGPL.
- **Why not**: the tree plays wav files when a ball hits a paddle. Nothing asks for
  positional audio, and ADR-0010's rule about paying for what an app uses applies here too.

## Consequences

### Positive
- Pong makes a sound, which nothing in the repository had done before.
- `vendor/soloud` is gone, and with it a committed `.lib`, a `link_directories` entry, a
  submodule checkout in CI and the workflow step that built it on its null backend. The
  remaining vendored submodule is `libnoise`.
- A failed device is now a logged error and a silent app: `initialize()` reports what the
  backend said, `playClip` refuses when nothing is open, and teardown skips a mixer that
  never started.
- A clip resolves against the asset manager's path like every other asset, so an app's
  `sounds.json` names files the same way its other configs do.

### Negative
- Taking the port meant moving the vcpkg baseline from 2025-02-21 to 2026-05-09, so every
  dependency moved at once rather than one at a time — including SDL3 itself, 3.2.4 to
  3.4.8. SDL guarantees source compatibility in that direction, but the tree now carries a
  dependency bump it did not otherwise need.
- SDL3_mixer is a library rather than a header, and `v3dlib_asset` links `v3dlib_audio`
  PUBLIC, so every app that touches an asset links it whether or not it plays anything.

### Risks
- **The audio device is still untested in CI.** Opening one needs hardware a runner does not
  have, so the suite covers what a clip does with a file, what a config is rejected for and
  what the resolver is asked — and never calls `initialize()`. A device that fails to open
  on a developer's machine is caught by the log line, not by a red build. This is the same
  boundary [ADR-0007](0007-ci-rendering-tests.md) draws for rendering.
- **A baseline that moved once will have to move again.** The escape hatch is
  `vcpkg-configuration.json`, one commit hash; the thing that makes a future bump expensive
  is letting this one go stale for another eighteen months.
