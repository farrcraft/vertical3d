# ADR-0084: Audio: events and parameters behind an interface

**Status**: accepted
**Date**: 2026-10-07
**Amends**: [ADR-0021](0021-audio-use-sdl3-mixer.md)
**Documented in**: [api/engine/Audio.md](../api/engine/Audio.md)

## Context

`api/audio` plays clips by id on named buses through SDL3_mixer. A game whose sound is authored
in an audio tool plays named events from banks, and drives the mix with global parameters such
as intensity or time of day. SDL3_mixer has no banks, events or parameters, and a clip id does
not carry what an event carries. FMOD Studio provides all three, but its licence forbids
redistributing the SDK, and its downloads need an account, so neither the tree nor CI can hold
or fetch it. An app that plays events still has to build and run on a machine without it.

## Decision

`api/audio` gains an event interface beside the clip engine: load a bank, play a named event,
set a global parameter. A null backend is used when no event backend is built, and reports each
event as not played. An FMOD Studio backend is built only when a configure option names a local
SDK, and clips, buses and fades stay on SDL3_mixer either way.

## Alternatives

### Replace SDL3_mixer with FMOD
- **For**: one audio engine, and clips become events like any other sound.
- **Against**: every app, and CI, would need an SDK the tree cannot ship or fetch, to play a wav.
- **Rejected because**: the apps in the tree play clips, and they must build on any machine.

### Build events on SDL3_mixer
- **For**: no new dependency. An event could be a clip id and a parameter a bus gain.
- **Against**: a bank authored in an audio tool cannot be read, so the game's sound would have
  to be rebuilt by hand, and the mix its designer built would be lost.
- **Rejected because**: it gives the names of events without what an event is.

### Leave FMOD in each game
- **For**: nothing in the api changes, and the game keeps full control of its middleware.
- **Against**: each game writes its own bank loading and parameter plumbing, and a game whose
  machine lacks the SDK cannot build at all.
- **Rejected because**: the null backend lets a game that plays events build anywhere, which only
  an interface in the api can give it.

## Consequences

- **Gains**:
  - A game plays events and sets parameters through the api, and builds without FMOD.
  - Clips keep working exactly as before, with or without the event backend.
- **Costs**:
  - The FMOD backend has no CI gate, because CI has no SDK. It is checked by running an app
    against a real bank.
  - A developer who wants FMOD installs its SDK and points the build at it by hand.
  - Two audio engines may run in one process, each with its own device.
- **Revisit when**: a second middleware backend is wanted, or SDL3_mixer gains events, or the
  FMOD licence allows the SDK in CI.
