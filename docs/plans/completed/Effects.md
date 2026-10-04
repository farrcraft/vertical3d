# Effects — A Sprite Clip, Particles On The Step, Weather, And A Tint Over The World

Drafted 2026-10-03 against `425cda2`, and closed 2026-10-04. Eleven steps across `api/type`, `api/ecs`, `api/render`
and the documents, one of them held, taking up [milestone 6](../../roadmap/completed/m6-Effects.md) of
[the game engine roadmap](../../roadmap/GameEngine.md). Every step is in this tree. cozy and retcon
are the consumers it is written for, and each adopts after it ships. Neither is changed nor run
here.

The roadmap has four sections: particles, weather, tinting the world, and panning a voice. The
surveys keep that order and change four things about it. **The sprite clip is written first.**
[MotionAndQueries](MotionAndQueries.md#step-6--a-sprite-clip) held it for a second
consumer, and a particle's look is that consumer. **A random source is a step of its own**,
because the roadmap's headless test needs a fixed answer and nothing in this tree gives one.
**Each half of tinting is its own step**, because the 2D half is a canvas and the 3D half is a
uniform and a table. **Panning is held**, because neither game has asked, and the reasoning is at
[step 10](#step-10--a-panned-voice-held).

**Nothing is due.** Neither game has scheduled any of this. That is the largest difference from
milestones 1 and 2. cozy's M6 names time of day and weather in one line and no trigger. retcon
lists fourteen kinds of effect and no phase that builds them. The plan is drafted now, while
milestones 1 to 5 are fresh, so that what a game adopts is written against one shape rather than
two games' guesses at it.

## What the surveys found

cozy was read at `3217bc2` and retcon at `3d22935`. Both pin this tree at `13a9557`, nine commits
behind, so neither has `realtime::sprites()`, the renderable components, the lit tier or
playback. This tree was read at `425cda2`.

**In cozy:**

* **It asks for very little by name.** `docs/roadmap/M6-world-and-map.md` lists "time of day and
  weather" in one bullet. Its M7 and M8 list no effects. No document mentions particles, a light
  pool, a flash or positional audio.
* **Its act palette is a ui theme today.** `M2UiAndMenus.md` built two themes and
  `Engine::activeTheme(name)` so that Act 2 could swap them. The world has no tint beyond each
  quad's own colour. The brainstorming's "colour and silhouette do the work that lighting would"
  is what a tint over the world quads would carry.
* **Decoration is stepped in `tick()`.** `M2UiAndMenus.md` says drifting light, weather and a fire
  are decoration, "animates in `tick()`, not `simulate()`", because nothing about them needs to be
  reproducible. The roadmap puts particles on the fixed step. [Step 3](#step-3--the-record-where-effects-live)
  has to say how both hold.
* **Its clock is minutes.** `cozy::Clock` advances in `tick()` at twelve game minutes a real
  second, wraps at midnight, starts at 07:20, and keeps no day count. A tint over a day is a
  colour looked up at `minutes() / 1440`.
* **It draws its own billboards.** `AppEngine::drawSprite` spans a quad by the camera's `right()`
  and `up()` into its own `WorldCanvas`, with no depth order. That is what `realtime::sprites()`
  replaced in this tree.
* **The walk cycle is not written.** Its M5 plan describes one, "a list of region names, a
  duration and an index advanced on the fixed step", and deferred it until there is walking art.
  `component::Facing` is all that exists. That is the shape [step 1](#step-1--a-sprite-clip-and-a-keyed-track)
  writes.
* **It has no random source**, and plays its ambience as one unpanned looping bed.

**In retcon:**

* **Its list is long and 3D.** `docs/game/content/visual-effects.md` names muzzle flash, impacts,
  blood, melee hits, explosions and fire, rain, snow, fog and day-night, status particles, a
  detection arc and an alarm flash. Nothing schedules them: there is no phase 10 plan, and its
  phase 10 roadmap does not mention visual effects.
* **Its look is a grade per zone.** `docs/art-production.md` gives each environment zone its own
  LUT. `LutPipeline` loads one strip once and has no swap. `SceneRenderSettings` fixes the key
  light's direction and has no colour. That is also true of this tree's `LitSettings` and
  `Grade`, which came from it.
* **It has a deterministic random source.** `engine/core/Rng.hpp` is splitmix64, with `next()`,
  `below(bound)` and a `state()` that a save keeps, per its ADR-0023. That is engine-shaped, and
  [step 2](#step-2--a-seeded-random-source) moves it.
* **Its camera's basis is mirrored from cozy's.** retcon's profile uses
  `Hand::DirectionCrossUp`, the `glm::lookAt` basis, so its `right()` points the other way. The
  world pipeline culls nothing, so a billboard spanned by it is not lost, but its texture is drawn
  mirrored. That is the caller's to know, since the basis is handed in.
* **It keeps its audio on FMOD**, with events and parameters and no listener or position, by its
  own ADR-0013.
* **It keeps its look.** Its round-two handoff keeps `SceneRenderer` and the cel, outline, shadow
  and LUT passes its own. Milestone 4 wrote the counterpart here, which retcon has not adopted.
  Particles in a lit scene are written against this tree's tier, and reach retcon when it adopts
  that tier.

**In this tree:**

* **`WorldCanvas` and `DepthOrder` are headless**, and so is `realtime::sprites()`. A billboard is
  a quad spanned by a right and an up the caller hands in. Its key is a distance along an axis the
  caller hands in. A particle drawn through the same `DepthOrder` sorts among the sprites, which
  is what a fire beside a character needs.
* **The world pipeline blends straight alpha and nothing else.** `World` compiles a pair, with and
  without depth, against one colour format, and `Builder::Blend` already takes any factors. A
  flash or a flame wants additive blending, which does not darken what is behind it.
* **`World` can be built against any target.** The lit and depth device cases build one against
  their own formats, so world quads can be drawn into a lit scene target today. Nothing does.
* **`type::animation::Clock` is written** and holds no time of its own. Its comment already names
  a sprite's frames as a second owner.
* **A clip's channels interpolate keyed values, and only for a joint.** Nothing interpolates a
  colour or a size over a time, which a particle's life and a day both need.
* **There is no random source anywhere in `api/`.** The standard distributions are
  implementation-defined, so a test written against `std::uniform_real_distribution` would pin
  one standard library's answer.
* **The lit tier has no light colour.** `SceneUniforms` holds a direction, a fill, thresholds and
  band multipliers, and every band is grey. **`Grade`'s table is fixed when it is built.**
* **`audio::Engine` has no pan.** A voice has a bus, a gain and a fade.

---

## The steps

| | What | Where | ADR | State |
|---|---|---|---|---|
| [1](#step-1--a-sprite-clip-and-a-keyed-track) | A sprite clip over the clock, and a keyed track of colours or sizes | `api/type` | — | done |
| [2](#step-2--a-seeded-random-source) | retcon's splitmix64, as a random source a test can pin | `api/type` | — | done |
| [3](#step-3--the-record-where-effects-live) | The record: where effects live, and what is stepped when | `docs/adr` | **[0072](../../adr/0072-an-emitter-is-a-component-on-the-step-that-owns-its-particles.md)** | done; accepted |
| [4](#step-4--an-emitter-and-its-particles) | An emitter's description, its particles, and the step that moves them | `api/type`, `api/ecs` | 0072 | done |
| [5](#step-5--particles-drawn) | Particles drawn as billboards into a depth order | `api/render` | 0072 | done |
| [6](#step-6--weather) | An emitter over a region, wind, and a density that eases | `api/type` | 0072 | done |
| [7](#step-7--a-tint-over-the-world-quads) | A tint a canvas multiplies over every quad it is given | `api/render` | — | done |
| [8](#step-8--particles-in-a-lit-scene) | World quads drawn into a lit scene, and an additive pipeline | `api/render` | — | done |
| [9](#step-9--a-lit-scenes-colour-over-time) | A key light with a colour, and a grade whose table can be replaced | `api/render` | — | done |
| [10](#step-10--a-panned-voice-held) | A pan per voice | — | — | held; in [TODO.md](../../TODO.md#audio) |
| [11](#step-11--a-fire-and-rain-drawn-and-the-handoff) | A fire and rain drawn, and the handoff | `api/render`, `docs` | — | done |

Steps 1, 2, 3, 7 and 9 depend on nothing. Step 4 needs steps 1, 2 and 3. Step 5 needs steps 1
and 4. Step 6 needs step 4. Step 8 needs step 5. Step 11 needs everything else.

---

### Step 1 — A sprite clip and a keyed track

**The sprite clip**, in `api/type/animation/` beside `Clock`, glm only:

```cpp
class SpriteClip final {
 public:
    struct Frame final {
        glm::vec2 uv0;
        glm::vec2 uv1;
        float duration;    // seconds
    };

    SpriteClip(std::vector<Frame> frames, bool loops);

    const Clock& clock() const noexcept;           // the frames' summed duration
    const Frame& frame(float time) const noexcept; // the frame an unwrapped time stands in
};
```

**Regions are held resolved, not by name**, as `component::Sprite` already holds them. A game
turns names into uvs with `config::SpriteSheets::uv` when the sheet loads, and builds the clip
again when it reloads. That keeps the clip glm-only and in `api/type`, which was the open
question MotionAndQueries left: region names would have taken it into a library of its own under
[ADR-0033](../../adr/0033-a-consumer-selects-the-api-libraries-it-wants.md), and nothing here needs
them.

A walk cycle is a clip per facing and a time the game keeps. A particle is the same clip with
its age as the time.

**The keyed track** is a sorted list of `(time, value)` keys, for a `float`, a `glm::vec3` or a
`glm::vec4`, sampled linearly and clamped at either end. It optionally wraps over a period, so
that the last key leads back into the first. It is one template in a header. A particle's colour
and size over its life are tracks over `[0, 1]`. A day's tint is a track over 1440 minutes that
wraps. A clip channel is not reused for this, because its keys belong to a joint and a path.

**Tests**, headless:

* a clip of three frames of unequal length gives each frame across its own span, including at
  both edges;
* a looping clip wraps to its first frame, and a clamped one holds its last;
* an empty clip and a frame of zero length are refused at construction;
* a track gives each key at its time, the lerp between, and the end keys outside;
* a wrapping track leads from its last key into its first across the period.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
type suite. There are eight cases, four in `sprite_clip_test` and four in `track_test`. With the
frame lookup taken as a lower bound, a time on a boundary shows the frame before and the span
case fails. With the wrap's span taken as the whole period, every time across midnight is wrong.
Two things came out differently:

* **A refusal is `std::invalid_argument`**, from the constructor, for both the clip and the
  track. The track also refuses keys that do not rise and, when it wraps, a key outside its
  period.
* **A track built from one value is a constant**, through a constructor of its own, so that a
  description of an emitter can default its colour and size without a list of keys.

### Step 2 — A seeded random source

**retcon's splitmix64 moves here** as `type::Random`, in its own header. It keeps `next()`,
`below(bound)` and a `state()` a save can keep and restore, because retcon's ADR-0023 relies on
all three. It adds what an emitter needs: a float in `[0, 1)` made from the top 24 bits, a float
in a range, a point in a box, and a direction within a cone. Each is written here rather than
taken from `<random>`, whose distributions are implementation-defined.

**Tests**, headless:

* the first values from a known seed match splitmix64's published sequence;
* a restored state continues the sequence;
* every float is in its range over a million draws;
* every direction lies within its cone.

A first-values case pins the generator, so a change to it fails here rather than as a particle
at a new place.

**retcon's handoff** is to replace `engine/core/Rng.hpp` with this, which keeps every saved
state valid.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
type suite. There are seven cases in `random_test`. They carry retcon's own cases across, with
the published vector added, and `next()` and `below()` are retcon's arithmetic unchanged, so
its saved states resume here. With a cone's height drawn from only half its span, the draws
stop short of the rim and the cone case fails. Two things came out differently:

* **The point in a box is `inside(minimum, maximum)`**, and it draws x, then y, then z, in
  separate statements, because the order of a constructor's arguments is unspecified and a
  seed would otherwise give a different point under another compiler.
* **The cone spreads its draws evenly over the cap's area**, with the height drawn evenly
  between the rim and the pole. Drawing the angle evenly instead would bunch them at the pole.

### Step 3 — The record: where effects live

**ADR-0072: an emitter is a component stepped with the simulation, which owns its particles, and
whose look is a render component.** It goes in as `proposed` and is accepted when step 5 draws
through it. It follows the split [ADR-0070](../../adr/0070-animation-is-sampled-from-playback-on-the-step.md)
made for animation.

* **The description and the stepping are `api/type`'s.** An emitter's description is plain data:
  a rate and a burst, a lifetime range, a spawn shape, a velocity cone, an acceleration, and
  tracks for size and colour. Its state is its particles and a random source. `step(description,
  state, origin, seconds)` ages, moves, kills and spawns. It is glm and arithmetic, tested the way
  `Clock` is.
* **The emitter is a component in `api/ecs`**, `ecs::component::Emitter`, holding the description
  and the state. `advance(registry, step)` steps every one from the entity's `Transform`, so a
  torch carried by a character emits from the character.
* **What a particle looks like is a render component**, `realtime::component::Particles`, with a
  texture and a sprite clip. It is the counterpart of `Sprite`, and it is a component per kind of
  drawing, per [ADR-0063](../../adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md).
  `api/ecs` cannot hold a `TextureHandle`.
* **A particle keeps its own previous position.** [ADR-0060](../../adr/0060-a-moving-thing-keeps-its-previous-step.md)
  draws between steps from a `Previous<T>` snapshot of the whole component. For an emitter that
  copies every particle every step. One extra `glm::vec3` per particle holds the same information
  at a fraction of the cost, and a particle born this step has a previous position equal to its
  own.
* **The api offers emitters and nothing that decides when one fires.** A muzzle flash is a burst
  the game starts. Weather is an emitter whose rate the game sets.

**On cozy's rule that decoration steps in `tick()`.** `step()` takes seconds and does not know
where it is called from. A game that calls `advance` from `tick()` with the frame's time draws at
an alpha of one, and its effects are as reproducible as it wants them to be. The record states
the fixed step as the default, because that is what makes a seeded emitter give the same answer
twice, and records that a game which does not need that may step it per frame.

**Alternatives the record weighs:**

* **A particle per entity.** It reuses `Transform`, `Sprite` and the snapshot. It also creates
  and destroys hundreds of entities a second, and pays a snapshot and a component lookup for each.
* **A particle system the game holds outside the registry.** It would not follow an entity, and a
  game would write the walk that `advance` is.
* **A library of its own, `api/effects`.** As with ADR-0070, it has no dependency `api/type`,
  `api/ecs` and `api/render` do not already have.
* **Particles stepped on the device.** The roadmap rules out GPU particles: neither game's counts
  need a compute pass.

**State: done; ADR-0072 accepted.** The record follows the draft. It was accepted when the plan
closed rather than when step 5 drew through it, by its decider. It adds one risk the draft did not name: a component that owns its particles
copies them when it is moved, and the escape is holding them behind a pointer.

### Step 4 — An emitter and its particles

**The shape**, in `api/type/effect/` (namespace `v3d::type::effect`):

* **`Emitter`**, the description:
  * `rate` in particles a second, and `burst()` for a count at once;
  * a lifetime range;
  * a spawn shape, which is a point, a sphere or a box;
  * a velocity as a direction, a cone and a speed range;
  * an acceleration, such as gravity or a steady wind;
  * a drag;
  * size and colour as tracks over a particle's life;
  * a cap on how many live at once.
* **`Particle`**: a position, a previous position, a velocity, an age, a lifetime, and the phase
  a sway reads.
* **`State`**: the particles, the fraction of a particle carried between steps (so that a rate of
  ten at sixty steps a second spawns ten, not zero), and a `Random`.
* **`step(emitter, state, origin, seconds)`.** It ages each particle and removes the dead by
  swapping with the last. It moves each one by semi-implicit Euler, and spawns what the rate owes.

**In `api/ecs`**, `component::Emitter` holds an `effect::Emitter` and an `effect::State`, with
`advance(registry, step)` beside `Playback`'s.

**Tests**, headless and seeded:

* a rate of ten for one second at sixty steps a second holds ten particles, and the carried
  fraction is what makes it ten;
* a burst holds its count at once, and the cap is never passed;
* a particle at rest under gravity stands where `½gt²` puts it, to within a step's Euler error;
* every particle is removed when its life ends, and none is drawn older than its lifetime;
* two emitters from one seed are equal step for step, and a different seed differs;
* a particle born this step has its previous position equal to its position;
* an entity's `Transform` moves where its emitter spawns.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the type
and ecs suites. There are eight cases in `effect_test` and two in `emitter_test`. With the owed
fraction's slack taken out, the rate case spawns four in a second rather than five. With the
previous position left where a particle was born, the second step's case fails. Five things came
out differently:

* **The walk is `emit(registry, step)`, not `advance`.** `Playback` already declares
  `advance(entt::registry&, float)` in the same namespace.
* **A rate summed a step at a time falls short in float.** At sixty steps a second, rates of
  one, five, nine and eleven a second each spawn one too few in their first second, while ten
  happens to overshoot. The owed fraction is floored with a slack of a ten-thousandth, and the
  case runs at five, where the shortfall shows.
* **The emitter is turned.** `step()` and `burst()` take an orientation beside the origin, which
  turns the spawn shape and the direction, so a muzzle flash follows its gun. Acceleration stays
  in the world. The component passes its `Transform`'s rotation.
* **The description's lifetime and speed are pairs of floats**, `lifeMin` and `lifeMax`, and
  `speedMin` and `speedMax`. A sphere's radius is `extent.x`, and a box's half size is `extent`.
* **The component's constructor is not `noexcept`**, because a default description allocates
  its tracks.

### Step 5 — Particles drawn

**`realtime::particles(registry, alpha, right, up, depthAxis, order)`**, beside `sprites()` and
with the same arguments. It walks `view<const Transform, const ecs::component::Emitter, const
component::Particles>()`. Each particle:

* is placed `alpha` of the way from its previous position to its position;
* is sized and coloured by its tracks at age over lifetime;
* takes its region from the clip at its age;
* is keyed along `depthAxis`, into the same `DepthOrder` as the sprites.

**A particle faces the camera, or stretches along its velocity.** `component::Particles` has a
facing. A rain streak is spanned by its velocity projected onto the camera's plane and by the
perpendicular, at a length the component scales. A spark uses the same facing. A smoke puff
faces the camera.

**Tests**, headless, in the way `SpriteTest.cpp` reads a canvas:

* one particle mid-step is drawn at the interpolated position, with its corners spanned by
  `right` and `up`;
* its uvs are the clip's frame at its age, and its colour is the track's;
* a particle and a sprite at different depths come out of the order furthest first, whichever was
  added first;
* a stretched particle's long edge lies along its velocity's projection;
* an emitter on an entity with no `Particles` is not drawn.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
render suite. There are five cases in `particle_test`, and a particle sorts between two sprites
through one `DepthOrder`, which is the midpoint this step was. Each of these breaks a case: the
particle drawn at its position rather than between steps; a slow streak drawn shorter than its
width; and a clip played by age when it is meant to run over the particle's life. Four things
came out differently:

* **The walk reads no `Transform`.** A particle stands in the world where it was born, so the
  view is `Emitter` and `Particles` alone.
* **The clip is shared**, as a `boost::shared_ptr<const SpriteClip>`, since a clip has no empty
  state and every particle of every emitter plays it. With no clip, the component's own uv pair
  is drawn, as a `Sprite`'s is.
* **A clip plays by age or over a life**, by `overLife`. A flame flickers at its own rate,
  and a puff thins as it dies however long it lives.
* **A particle is centred where it stands**, rather than standing on it as a sprite does. A
  streak is never shorter than it is wide.

### Step 6 — Weather

**Weather is an emitter whose shape follows the view.** Rain and snow need three things the
point emitters of step 4 do not:

* **A spawn region the caller moves.** It is a box above the ground the camera sees, which the
  game takes from milestone 1's ground pick at the screen's corners and hands in every step. A
  particle that falls out of the region's bottom is removed.
* **Wind** is an acceleration the game changes.
* **Density eases toward a target.** `Weather` holds an intensity that moves toward a target at a
  rate. The emitter's rate is its base rate times the intensity, so a shower starts and stops
  rather than switching.

Snow's drift is the sway in `Particle`: a sideways offset of a per-particle phase, amplitude and
frequency, added to the drawn position rather than to the simulated one. Interpolation is then
unaffected.

**Fog is not built.** In a 2D world it is a tinted quad over the scene, which a game draws with
`WorldCanvas::quad` today. In a lit scene it is a pass in the chain, and no game has asked for
one. Step 11's handoff says so.

**Tests**, headless and seeded:

* at full intensity the count settles at rate times the fall time;
* a target of zero eases the rate to zero at the intensity's rate, and the count then falls to
  zero;
* every particle stays inside a region that moves;
* wind moves the mean horizontal velocity by its acceleration times the time;
* the sway leaves the simulated position alone.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
type, ecs and render suites. There are four cases in `weather_test` and one more in
`particle_test`. Each of these breaks a case: a wrapped particle whose previous position is left
behind; an intensity that jumps to its target; and weather that ignores its wind. Five things came
out differently:

* **Weather is `type::effect::Weather` and `fall()`, not an emitter shape.** `fall()` takes an
  `Emitter` for how a drop launches, lives and looks, and ignores its rate and shape. It spawns
  over the region's top face instead.
* **`step()` is three public pieces**, which `fall()` shares: `travel()` ages, moves and removes,
  `owing()` keeps the owed fraction, and `spawn()` adds one particle at a place. The draws are made
  in the order they were, so a seed gives the particles it gave. `travel` is not `move`, which
  cpplint reads as `std::move`.
* **Density is per square unit of ground**, so a camera that zooms out keeps the look of the
  rain rather than thinning it.
* **A particle leaving the region across a side comes in at the other**, with its previous
  position moved too. A region that follows the view then keeps its density, where removing
  such particles would leave the trailing edge dry.
* **The sway is on the emitter's description**, `sway` and `swayRate`, and `particles()` adds it
  along the camera's right. It reads the particle's age as of its last step, so it moves in
  steps of one simulation step.

An intensity eased by steps of a hundred-and-twentieth reaches its target a step late, because
the steps sum short of one in float. The clamp then lands it exactly.

### Step 7 — A tint over the world quads

**`WorldCanvas::tint(colour)`** sets a colour that every quad added after it is multiplied by,
until it is set again. `clear()` resets it to white. `DepthOrder::into()` and `sprites()` add
through the canvas, so they are tinted with nothing more. A game sets the tint once before it
fills the canvas, instead of passing it to every caller.

The ui's `Canvas` is not tinted. Dusk falls on the world and not on the menu.

**It multiplies on the cpu, rather than through a push constant.** The canvas already writes
every vertex, so the multiply costs nothing. It needs no pipeline change, and it is visible to a
headless test, which is what the roadmap's verification asks. A push constant would let one canvas
be submitted twice under two tints, which nothing does.

cozy's act palette is a tint, and its day is a wrapping track of tints from step 1, sampled at
its clock's minutes. Light pools are not here, as the roadmap says.

**Tests**, headless:

* every vertex of a quad added after `tint()` is its colour times the tint;
* quads added before are unchanged, and `clear()` returns the tint to white;
* a depth order handed to a tinted canvas is tinted.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over the
render suite. There are two cases in the canvas suite and one in the depth order suite. With the
multiply taken out, the tinted quad keeps its own colour, and with the reset taken out of
`clear()`, the next frame is drawn under the last one's tint. It landed as drafted, with
`tint()` also giving back the colour being multiplied by.

### Step 8 — Particles in a lit scene

**World quads are drawn into the lit scene target**, after the cel and outline passes and before
the grade. They are depth-tested against the meshes, and graded with them. A `World` is built
against the scene target's colour and depth formats, which the device cases already do. What this
step adds is the order: the world pass reads the scene target's depth, and
[ADR-0068](../../adr/0068-a-target-per-frame-a-checked-format-and-passes-placed-by-what-they-read.md)
places it by that read.

**`World` gains an additive pair of pipelines.** The blend is source alpha by one, and depth is
tested and not written. `submit()` gains a blend, which defaults to straight alpha. A canvas is one
blend, so a game fills a canvas of smoke and a canvas of flame. An additive particle does not need
to be sorted against another, because addition commutes.

**The colours are linear here.** The scene target lights in linear
([ADR-0066](../../adr/0066-the-lit-tier-lights-in-linear.md)), so a vertex colour is a linear colour,
as it already is for every quad drawn into an sRGB swapchain.

**Tests**, on the device:

* a quad behind a lit cube is hidden where the cube covers it, and seen where it does not. The
  case reads the depth case's layout;
* an additive quad of a colour over a known clear adds exactly that colour, where the sum does
  not saturate. That is a blend the specification determines, so the colour is asserted exactly;
* a lit frame with particles is silent.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over both
render suites. The whole device suite is silent with `VK_LAYER_VALIDATE_SYNC=1`. There are two
new device cases:

* **`world_quads_are_hidden_by_a_lit_scene`.** A green ground quad, submitted after a red cube in
  the lit pass, leaves the cube's top face red at the centre and is green at the corner. With the
  world pipelines built without a depth test, the centre is green.
* **`an_additive_quad_adds_to_what_is_there`.** It is exact on the Radeon. The clear and the
  quad's colour are chosen so that every sum, at full and at half alpha, is a whole multiple of
  51 on a 255 scale. With the additive destination factor set to straight alpha's, both sums
  are wrong.

Three things came out differently:

* **The quads go in the lit pass itself**, not in a pass of their own placed by what it reads.
  The recorder binds the scene set only for a pipeline that declares one, and the pass records
  in submission order, so the world quads submitted after `meshes()` are drawn after it against
  its depth. No second render pass is begun.
* **`World` keeps four pipelines**, built by one `createPipeline(name, colour, depth, blend)`.
  The additive ones keep the destination's alpha.
* **A game builds its own `World` for the scene target**, as it builds its `Lit`. Nothing was
  added to make one, because `Engine3D::worldQuads()` is the swapchain's.

A device filter in Boost lists suites with `:`, not `,`. With a comma, one case of the three ran
and the additive mutation looked harmless.

### Step 9 — A lit scene's colour over time

**`LitSettings` gains a key colour and a fill colour**, which multiply the key's and the fill's
contributions before banding. They default to white, which leaves today's pictures unchanged to
the byte. `SceneUniforms` gains one `vec4` at the end of the block. Earlier offsets do not move,
and a replacement shader that includes `lit.glsl` gets the member with nothing more, per
[ADR-0067](../../adr/0067-lit-shaders-are-embedded-and-replaceable.md). Dusk is an amber key, and
night is a dim blue one.

**`Grade` can be given a new table.** `Grade::table(texels)` uploads a new 16³ table, retires the
old one through the ring ([ADR-0061](../../adr/0061-a-resource-is-released-explicitly.md)), and
rewrites each source's descriptor. retcon's grade per zone is a swap on entering the zone. A slow
change between two grades is a lerp of the two tables' texels on the cpu, uploaded as often as the
game likes. A table is 16 KiB, so this needs no second texture and no shader change.

**Tests:**

* headless: `pack()` with white colours gives the uniforms it gave before, byte for byte;
* on the device: the existing lit cases are unchanged with white colours;
* on the device: a red key turns the lit band of a white cube red, by the band's multiplier;
* on the device: after a swap to the inverting table, a scene grades to its inverse, as
  `PostTest.cpp`'s inverting case asserts. A source made before the swap is regraded with the new
  table;
* on the device: a table replaced while a frame grades with the old one keeps the frame silent.

**State: done.** All 25 suites pass, cpplint is clean, and so is `out/build/verify` over both
render suites. The whole device suite is silent with `VK_LAYER_VALIDATE_SYNC=1`. Every one of the
29 pictures the device suite wrote before this step is byte-identical after it, the lit, shadowed
and skinned ones included, which is the white light leaving the tier unchanged. The new cases are:

* `the_default_light_is_white`, and the pack case reading both colours;
* `the_light_has_a_colour`: a white cube's lit face is exactly white under a white light and
  exactly red under a red one;
* `a_replaced_table_regrades_its_sources`, and `a_table_replaced_in_flight_keeps_the_frame_silent`.

Each of these breaks a case: the lit band ignoring the colour; and a replaced table that leaves
its sources on the old one, which is also a validation error once the old table is released.
Four things came out differently:

* **The colours are the light's and the shadow band's**, `colour` and `shadowColour`, not a
  key's and a fill's. The cel shader bands one scalar of key and fill together, so there is no
  separate contribution to colour before banding. A colour per band does what the draft wanted:
  an amber light with blue shadows.
* **The block is 160 bytes**, with two `vec4`s after the shadow terms, so no earlier offset moved.
* **The swap is `Grade::replace(texels)`**, since `table()` is already the static that turns a
  strip into texels. Writing into the old table in place was not an option: a frame in flight
  reads it. Each source is rebound to a new material, and the handle the caller holds is the key
  it is found by.
* **The grade's linear sampler is kept and shared** by every table it makes.

### Step 10 — A panned voice, held

**Held, and here is why.** The roadmap waits for cozy to ask, because retcon pans through FMOD
by its own ADR-0013 and will not use `audio::Engine`. cozy has not asked: it plays one
non-positional ambience bed, and nothing in its M6 to M8 mentions footsteps placed on screen or
positional sound.

The change is small when it comes. `Play` gains a pan, and `audio::Engine` gains `pan(voice,
value)`, through whatever SDL_mixer 3 offers per track, which is checked when this is taken up. A
position becomes a pan in the caller, because a fixed camera over a flat world makes that mapping
the game's.

**The trigger is cozy asking**, for a footstep or a creature heard from one side. This moves to
[TODO.md](../../TODO.md) with that trigger when the plan closes.

### Step 11 — A fire and rain drawn, and the handoff

**Two pictures for a person to look at**, since filtering and blending are what
[ADR-0054](../../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md) says a reference
cannot pin. Both are device cases that assert silence and that consecutive frames differ, and both
write to `data_out/`:

* **A fire in a 2D world.** It is an emitter of flame with a four-frame clip, and one of smoke
  above it. They are drawn through `DepthOrder` beside a sprite standing in front of the fire and
  one behind it, under cozy's camera profile, with a dusk tint.
* **Rain in a lit scene.** It is a weather emitter over the look-dev scene with stretched
  particles and an additive splash, under a blue key light.

The sprite texture is generated by a fixture script, as the rigged fixtures were. No third-party
art is committed.

**State: done.** All 25 suites pass, cpplint is clean over the tree, and so is `out/build/verify`
over the device suite. The whole device suite, now 60 cases, is silent with
`VK_LAYER_VALIDATE_SYNC=1`. The two cases are `a_fire_is_drawn_among_sprites`, in a new
`effects_test`, and `rain_falls_in_a_lit_scene` in `lit_scene_test`. Each draws three frames a
third or half a second apart and writes them to `data_out/fire_*.png` and `data_out/rain_*.png`.
Both were looked at: the flame's glow is drawn over the foot of the figure behind it, and the
figure in front covers the flame's side. The rain falls as streaks over a ground and a cube
lit blue, the red cube a dull red under it. Four things came out differently:

* **The texture is made in the case**, four soft puffs of shrinking radius in a strip, rather
  than by a fixture script. It is sixteen texels a frame and is never compared with anything.
* **The camera is `type::camera::Isometric`'s**, as the lit cases use, rather than cozy's
  profile. The figures stand either side of the fire's line of sight. Standing them on it hid
  the fire behind the front one, which the first picture showed.
* **The rain is drawn additively and has no splash.** The drops themselves show the additive
  blend in a lit pass, and a splash would be a burst the game fires where a drop lands, which
  nothing here knows.
* **Fire is drawn with straight alpha**, so that it sorts among the sprites. Additive flame
  would be a canvas of its own submitted after them, which cannot stand behind a figure.

#### The handoff, for cozy and retcon when they adopt

Written here for each game to read, not sent to it. **Both must move their pin past `425cda2`
first.** Neither has milestones 3 to 5, and step 5 draws through `sprites()`'s depth order.

| A game needs | here |
|---|---|
| a walk cycle, a flickering flame | `type::animation::SpriteClip`, its regions held as uvs |
| a colour or a size over time, a day's tint | `type::animation::Track`, which may wrap over a period |
| a deterministic random source | `type::Random`, which is retcon's `Rng` with its states intact |
| a fire, a burst, a muzzle flash | `ecs::component::Emitter`, stepped by `emit(registry, step)`, with `realtime::component::Particles` ([ADR-0072](../../adr/0072-an-emitter-is-a-component-on-the-step-that-owns-its-particles.md)); `type::effect::burst()` for a burst |
| particles among sprites | `realtime::particles()` into the depth order `sprites()` uses |
| rain and snow | `type::effect::Weather` and `fall()`, with its region from the ground pick, and `sway` for snow |
| dusk, night, an act's palette over the world | `WorldCanvas::tint()`, sampled from a track |
| flame and sparks that lighten | `World::submit(..., World::Blend::Additive)` |
| effects in a lit scene | a `World` built against the scene target's formats, submitted in the lit pass after `meshes()` |
| a lit day and night, a grade per zone | `LitSettings::colour` and `shadowColour`, and `Grade::replace()` |

What adopting involves:

* **cozy** sets one tint before it fills its world canvases, sampled from a track at its clock's
  minutes. Its ui themes stay as they are. It may step its emitters from `tick()`, per ADR-0072.
* **retcon** hands `particles()` the camera's own `right()`. Its basis is mirrored from cozy's, so
  a texture with a handedness needs its clip's uvs swapped. Its effects are drawn into the scene
  target only once it draws through this tree's lit tier. It replaces `engine/core/Rng.hpp` with
  `type::Random`, which resumes every saved state. A lit shader of its own that includes
  `lit.glsl` gets the two colours at the end of the `Scene` block with nothing more.
* **Fog** is a tinted quad in a 2D world. In a lit one it waits for a game to ask for a pass.

---

## Sequence

**Steps 1 and 2 first.** Each is headless, needs nothing, and is something a game can use on its
own: a walk cycle, and a random source.

**Then the record, then step 4.** Step 4 is written against the record, and the record is written
once steps 1 and 2 show what a particle's look and its seed are.

**Step 5 is the plan's midpoint.** If a particle cannot sort among sprites through `DepthOrder`,
it shows here, and the plan stops to rethink ADR-0072's draw rather than building weather on it.

**Steps 6 and 7 complete the 2D half**, which is what cozy's M6 would adopt. Step 7 can be taken
at any time.

**Steps 8 and 9 are the 3D half.** Step 9 does not need particles and can go any time.

**Step 11 last.**

## Verification

Per [sdlc.md](../../sdlc.md), every step that changes code: `ninja -C out/build/x64-Debug`,
`ctest`, cpplint, and the `/W4 /WX`, `/analyze` and clang-tidy gates. The tree is clean at all of
them, so every finding is the step's. Steps 8 and 9 change what a lit frame binds and draws, so
each also runs the device suite once with `VK_LAYER_VALIDATE_SYNC=1`.

**What can be pinned is pinned:**

* the clip's frames, the tracks, and the random sequence, by hand;
* every emitter, every weather case and every drawn particle, from a fixed seed, headless;
* the tint on every vertex;
* an additive blend over a known clear, and a grade swap to the inverting table, on the device;
* white light colours leaving every lit picture unchanged, byte for byte.

**What cannot be pinned is silence and a picture a person looks at**: a fire, and rain. No new
reference picture is blessed. Nothing is verified in another repository.

## What this does not do

* **No GPU particles**, per the roadmap.
* **No light pools.** A campfire's glow is a light map that
  [ADR-0031](../../adr/0031-a-pass-draws-into-a-target-it-names.md) already allows, and cozy's art
  direction has not asked for one.
* **No fog pass, and no screen flash.** A flash is a tinted quad in the ui or a grade swap for one
  frame, and both are the game's.
* **No damage numbers, selection highlight or detection arc.** The roadmap puts them in `api/ui`
  and the outline.
* **It does not decide when an effect fires**, or what the weather is.
* **No seasons.** cozy's recipes wait on its M6 to settle them. A season is the game's calendar,
  read through the same tracks.

## When a step lands

Update the state in the table above.

* **Step 1** removes the sprite-clip entry from [TODO.md](../../TODO.md#sprite-sheets), and adds the
  clip to [Architecture.md](../../Architecture.md)'s account of `api/type`. The plans index says this
  plan is open.
* **Step 2** records retcon's handoff line in this plan, and nowhere else.
* **Step 3** adds the ADR index row for 0072 as `proposed`.
* **Step 4** adds `Emitter` to [ECSDesign.md](../../ECSDesign.md) beside `Playback`.
* **Step 5** accepts ADR-0072, and adds `particles()` and `component::Particles` to
  [RenderingPipeline.md](../../RenderingPipeline.md) beside the sprites.
* **Step 7** adds the tint to RenderingPipeline.md's account of `WorldCanvas`.
* **Step 8** adds the additive pipeline and the world pass in a lit frame to RenderingPipeline.md.
* **Step 9** adds the colours to RenderingPipeline.md's binding table, and the table swap to its
  account of the grade.
* **Step 11** adds the new device cases to [Testing.md](../../Testing.md).
* **When the plan closes**, [m6](../../roadmap/completed/m6-Effects.md) moves to `roadmap/completed/` and points
  here as done. The panned voice moves to TODO.md with its trigger, the roadmap's table row says
  so, and this file moves to [completed/](.).
