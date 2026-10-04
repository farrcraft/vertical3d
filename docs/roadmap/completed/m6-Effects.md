# Effects

Milestone 6 of [the game engine roadmap](GameEngine.md). The things on screen that are neither
a sprite nor a mesh: particles, weather, and the colour of the world changing with the time of
day. Both games list them and neither has started them, so this milestone is drawn from their
design documents rather than from code that has hit a wall.

**It has a 2D half and a 3D half.** The 2D half is drawn through `WorldCanvas`, needs
[milestone 1](m1-MotionAndQueries.md)'s clip and nothing else, and is the half cozy wants from its
M6 on. The 3D half draws the same particles in a lit scene and waits on
[milestone 4](m4-LitScene.md).

**Done by [Effects](../../plans/completed/Effects.md)**, drafted 2026-10-03 and closed 2026-10-04,
the panned voice aside. An emitter is a component stepped with the simulation that owns its
particles, and its look is a render component
([ADR-0072](../../adr/0072-an-emitter-is-a-component-on-the-step-that-owns-its-particles.md)).
Weather is an emitter falling over a region that follows the view, a tint is the world canvas's,
and a lit scene takes a light colour and a grade that can be replaced. The panned voice waits in
[TODO.md](../../TODO.md#audio) for cozy to ask.

## What exists

* **World quads.** [`WorldCanvas`](../../../api/render/realtime/WorldCanvas.h) draws textured,
  tinted quads with four world corners, depth-tested without writing
  ([ADR-0042](../../adr/0042-a-textured-quad-in-world-space.md)). A particle drawn as a billboard
  is one of these, and so is a raindrop.
* **A tint per quad and per vertex.** Every quad carries a colour; nothing tints a whole pass.
* **Audio with buses and fades.** [`audio::Engine`](../../../api/audio/Engine.h) plays a voice on a
  named bus with a fade in, a fade out and a gain. It has no position and no pan. cozy uses it;
  retcon uses FMOD instead, by its own ADR-0013, and is not a consumer.
* **cozy keeps the time of day itself**, in its `Clock`, and its M6 roadmap names time of day
  and weather together.

What the games ask for:

* **cozy** — weather, and a palette that carries the act structure: its brainstorming has the
  wilderness in warm greens and amber, Act 2 desaturating and cooling it, and the town returning
  warmth at a different temperature. It also says colour and silhouette do the work lighting
  would, because this is not a lit 3D game.
* **retcon** — muzzle flash, impacts, blood, melee hits, explosions and fire in combat; rain, snow,
  fog and day-night transitions in the world; damage numbers, a selection highlight and status
  particles in the ui; a detection arc and an alarm flash for stealth
  (`docs/game/content/visual-effects.md`).

## What it needs

### Particles

An emitter that spawns particles at a rate or in a burst, a fixed-step update that moves and ages
them, and a draw that writes each as a billboard into a `WorldCanvas`. The update is on the fixed
step like everything else that simulates
([ADR-0032](../../adr/0032-the-loop-simulates-at-a-fixed-step.md)), and drawing between steps is
[milestone 1's interpolation](m1-MotionAndQueries.md#interpolation).

**A particle's look is a sprite clip.** Smoke that thins, a spark that fades, a flame that
flickers are each a sequence of regions over a lifetime, which is milestone 1's clip advanced by
the particle's age instead of the loop's clock. That is why the 2D half waits on milestone 1 and
nothing else.

The billboard needs the camera's right and up vectors to face the viewer. Under cozy's fixed
orthographic camera and retcon's four-way snapped one, those change rarely, and the emitter
should take them rather than reach for a camera.

### Weather

A preset over particles: an emitter covering the view rather than a point, wind as a velocity,
and a density that changes over time. Rain and snow are this; fog in a 2D world is a tinted quad
over the scene, and in a 3D one is a pass in milestone 4's chain. What weather is falling, and
when, is a game's.

### Tinting the world

A colour multiplied over everything a pass draws, changed over time — dusk is amber, night is
blue, an act's palette is a different set of the same. In a 2D world that is the tint every
quad already carries, applied once by the canvas rather than by every caller; in a lit one it is
the light's colour and milestone 4's LUT. cozy's palette per act is this and nothing more.

**Pools of light are not in this milestone, on purpose.** A campfire glowing in a dark camp is a
light map drawn into a target and multiplied over the world, which
[ADR-0031](../../adr/0031-a-pass-draws-into-a-target-it-names.md) already allows. cozy's art
direction has colour do lighting's work, so whether it wants one is cozy's to ask.

### Panning a voice

A pan per voice, and a position mapped to a pan by the caller. That is the whole of positional
audio for a fixed camera looking at a flat world, and it is what a footstep on the left side of
the screen needs. It waits on cozy asking, because retcon will not, and nothing in cozy's
roadmap has yet.

## Verification

The simulation is headless: an emitter stepped a known number of times holds a known number of
particles at known ages and positions, with a seeded random source so the answer is fixed.
Weather is the same with a different emitter. The tint is a colour on every vertex the canvas
emits, which a canvas test already knows how to read.

What a particle looks like is filtered sampling and blending, which no reference here can pin
([ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)); it is
validation silence and a look, per [Testing.md](../../Testing.md). A pan is a number handed to
SDL_mixer, and `audio::Engine::initialize()` is already one of the things no CI here can run.

## Not in this milestone

* **GPU particles.** Neither game's list is large enough to need a compute pass, and a cpu
  system writing into a canvas is the one both halves share.
* **Damage numbers and the selection highlight.** Text and outlines in world space are `api/ui`
  and milestone 4's outline respectively, rather than effects.
