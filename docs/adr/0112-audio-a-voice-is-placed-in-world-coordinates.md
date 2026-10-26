# ADR-0112: Audio: a voice is placed in world coordinates

**Status**: accepted
**Date**: 2026-10-25
**Documented in**: [docs/api/engine/Audio.md](../api/engine/Audio.md)

## Context

A game places a sound at a point in its world, and hears it from wherever the camera or the
player stands. SDL_mixer places a track relative to a listener fixed at the origin, in right-handed
axes, and attenuates it with distance by a model of its own. It holds a track's stereo pan and
its position in one mode, so setting either clears the other. FMOD Studio places an event relative
to a listener it is told about, in left-handed axes. A game that plays events and clips together
hears both from one place.

## Decision

`audio::Engine` holds the listener, which the game sets once a frame in its own world
coordinates, and a voice's position is given in the same coordinates. The engine converts a
position into the listener's space for the mixer, and forwards the listener and each event's
position to the event backend, which converts to its own axes. A voice is either panned or
placed, and setting one clears the other.

## Alternatives

### The game hands over positions relative to the listener
- **For**: the mixer takes such a position as it is, and the engine holds no state for it.
- **Against**: every game writes the same conversion from world to listener space, once for
  clips and again in FMOD's axes for events.
- **Rejected because**: the conversion is the same in every game, and FMOD needs the listener
  itself, which a relative position does not carry.

### A pan applied on top of a position
- **For**: a game could widen or nudge a placed sound without moving it.
- **Against**: the mixer holds a track in one mode or the other, so the engine would have to
  compute the spatial gains itself and give up the mixer's model.
- **Rejected because**: it means writing a spatializer to keep a control no game in the tree
  uses.

### Events stay unplaced
- **For**: the event interface and its null backend are unchanged.
- **Against**: an event-based game pans its events by hand, and a 3D event authored in FMOD
  Studio plays as if at the listener.
- **Rejected because**: FMOD's placement and its authored attenuation are much of the reason to
  use events.

## Consequences

- **Gains**: a game sets one listener and places clips and events in the coordinates it already
  uses. Distance attenuation comes from the mixer and from the event's own authoring.
- **Costs**: the game has to set the listener every frame it moves, or placed sounds stay where
  they were heard from last. A clip's attenuation follows SDL_mixer's fixed model, which a game
  cannot tune. The FMOD conversion has no CI gate, as the rest of that backend has none. Whether
  a placed sound is audibly right is checked by ear, not by a test.
- **Revisit when**: a game needs its own attenuation curve for clips, needs more than one
  listener, or SDL_mixer lets a pan and a position combine.
