# Plans

A workstream earns a plan document when it spans several phases or several apps and the
ordering between the pieces is not obvious — when the interesting question is *what blocks
what*, not *what needs doing*. See [sdlc.md](../sdlc.md).

A plan is a living document while it is open, so update its state notes as things land. When
every phase is closed it moves to [completed/](completed/), and any open item it was carrying
moves to [TODO.md](../TODO.md). The plan itself stays, because the reasoning behind an ordering
outlives the schedule.

[completed/ApiDesignDebt.md](completed/ApiDesignDebt.md) was drafted on 2026-10-04 and closed on
2026-10-05, working off [the api/ design review](../audits/completed/ApiDesignReview.md): twelve
defects first, then the asset↔audio cycle and link visibility, the engine's lifecycle
([ADR-0080](../adr/0080-the-engine-owns-its-lifecycle.md)), keys and commands as two events
([ADR-0081](../adr/0081-a-key-and-a-command-are-different-events.md)), textures on the context
([ADR-0082](../adr/0082-textures-belong-to-the-context.md)), and the rules the review found
written in several places by hand - one ui traversal, one set of shading globals, one geometry
ring reset, one transform value. The reyes hider's opacity went to
[TODO.md](../TODO.md#offline-rendering).

[completed/OfflineRenderingPhases4To6.md](completed/OfflineRenderingPhases4To6.md) was drafted and
closed on 2026-10-04, taking up the last three phases of [the offline rendering
roadmap](../roadmap/completed/OfflineRendering.md): a pixel as a filtered set of seeded samples in a
film both renderers share, depth of field, motion blur of a transform and adaptive sampling; a
trace that recurses, reflection, refraction, transparency, spheres, `texture()` and `noise()`;
and phase 6 answered as one ray tracer both renderers reach
([ADR-0077](../adr/0077-one-ray-tracer-both-renderers-reach.md)), which gives moya shadows. Area
lights, displacement and an acceleration structure went to
[TODO.md](../TODO.md#offline-rendering).

[completed/ShellAndShipping.md](completed/ShellAndShipping.md) was drafted and closed on
2026-10-04, taking up [milestone 7](../roadmap/completed/m7-ShellAndShipping.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md). A strip respects `pickable()`, a command can
be held, voxel looks in relative mouse mode, and a document is read forward through a chain
([ADR-0073](../adr/0073-a-document-is-read-forward-one-version-at-a-time.md)). One shell class
builds the ui's renderers over an `Engine3D`, and pong, tetris, voxel and the editor draw through
it ([ADR-0074](../adr/0074-the-shell-builds-the-uis-renderers.md)). A canvas may draw in a space of
its own, which pong's court now is
([ADR-0075](../adr/0075-a-canvas-may-draw-in-a-space-of-its-own.md)). A box can wrap, the editor
opens and saves as through a file chooser, every pass is timed on the device, and there is a
slider. Asynchronous loading went to [TODO.md](../TODO.md#loading) behind cozy's region streaming.
The three records were accepted when it closed.

Four things came out differently. **The renderer helper could not live on the realtime side**,
because `v3dlib_ui` links `v3dlib_render`. It is in `api/ui/shell`, with `Engine3D` declared rather
than included. **A game space is the canvas's, not set 0's**: set 0 is a camera per pass and pong
draws its court and its menu in one pass, and a clip has to be mapped out of the space because a
scissor is in pixels. **A menu bar read nothing from a document but its name**, so marking one
unpickable would have been silently dropped. **And the resolver counted its style classes by
hand**, so the slider's class indexed past the array; the count is now taken from the enum.

Running the apps found two defects the plan had not looked for, both now in TODO.md: a minimised
window spins and logs every frame, and voxel's mouselook turns most of the way round in ten
pixels. The analysis gates found three things a build alone would not, each now fixed: a nested
struct's member initializers in a default argument, which clang refuses and MSVC accepts; a
destructor that could allocate; and a function past the cognitive complexity threshold.

[completed/Effects.md](completed/Effects.md) was drafted on 2026-10-03 and closed on 2026-10-04,
taking up [milestone 6](../roadmap/completed/m6-Effects.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md). The sprite clip that
[MotionAndQueries](completed/MotionAndQueries.md#step-6--a-sprite-clip) held is written over
milestone 5's clock, and retcon's splitmix64 is `type::Random`, so a seed fixes every particle on
any standard library. An emitter is a component stepped with the simulation that owns its
particles, and its look is a render component
([ADR-0072](../adr/0072-an-emitter-is-a-component-on-the-step-that-owns-its-particles.md)).
Particles sort among sprites through one depth order. Weather falls over a region that follows
the view, the world canvas carries a tint, world quads go into a lit pass with an additive blend,
and a lit scene has a light colour and a grade whose table can be replaced. The panned voice went
to [TODO.md](../TODO.md#audio) until cozy asks. Neither game had scheduled any of this, so it was
drafted while milestones 1 to 5 were fresh rather than against a trigger.

Four things came out differently. **A rate summed a step at a time falls short in float**: at
sixty steps a second, rates of one, five, nine and eleven a second spawn one too few in their
first second, while ten happens to overshoot, which is why the first draft of the case caught
nothing. **World quads in a lit scene need no pass of their own**, because the recorder binds the
scene set only for a pipeline that declares one and a pass records in submission order. **The
lit tier has no separate key and fill to colour**, since it bands one scalar of both, so the
colours are the light's and the shadow band's. **And a grade's table cannot be rewritten in
place** under a frame in flight, so `Grade::replace()` makes a new one, rebinds every source,
and keeps each caller's handle as the key it is found by.

Two things about verifying it are worth knowing next time. A Boost filter lists suites with `:`,
and with `,` one case of three ran, so a mutation looked harmless. And every picture the device
suite wrote before step 9 was kept and compared byte for byte afterwards, which is what showed a
white light leaves the lit tier as it was.

[completed/SkeletalAnimation.md](completed/SkeletalAnimation.md) was drafted and closed on
2026-10-03, taking up [milestone 5](../roadmap/completed/m5-SkeletalAnimation.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md). A file is one model in parts, read through
its node hierarchy, and may carry a skin
([ADR-0069](../adr/0069-a-model-is-parts-over-one-array-and-may-carry-a-skin.md)). Clips are
sampled on the cpu from a playback component advanced on the step and drawn between steps, and
which clip plays is the game's ([ADR-0070](../adr/0070-animation-is-sampled-from-playback-on-the-step.md)).
A frame's palettes are one storage buffer in the scene set, so both lit passes draw the pose
([ADR-0071](../adr/0071-joint-palettes-are-a-storage-buffer-in-the-scene-set.md)). Instancing
was held, and went to [TODO.md](../TODO.md#lit-scenes) behind a count that nothing has reached.

Four things came out differently. **The loader had never read a node**, so any mesh a node
moved loaded at its own origin, which every file so far had been too simple to show. **A skeleton
needs a root matrix**: a Mixamo rig's hundredth scale stands above the root joint, which a clip
animating that joint would otherwise overwrite. **No rigged asset existed anywhere**, so the
fixtures are generated: a hand-written strip whose every value is exact, which lets a skin at rest
be compared with its unskinned mesh byte for byte, and the same strip exported by Blender, which
shows the exporter's matrices agree with this tree's arithmetic. **And the steps' records were
accepted as each step began** rather than when its code had proved them, because each was read
before anything was built on it.

[completed/LitScene.md](completed/LitScene.md) was drafted and closed on 2026-10-03, taking up
[milestone 4](../roadmap/completed/m4-LitScene.md) of [the game engine roadmap](../roadmap/completed/GameEngine.md).
It writes retcon's lit tier into the frame model the rest of the api draws through, rather than
moving retcon's hand-recorded passes: the recorder learns a scene set at set 2 and a depth bias
([ADR-0064](../adr/0064-a-pass-carries-a-scene-set-and-a-depth-bias.md)), which is the one thing
retcon's refusal of that model rested on. Images and samplers are classes, a model reaches the
device through a registry ([ADR-0065](../adr/0065-a-mesh-is-registered-by-path-and-released.md)),
the tier lights in linear ([ADR-0066](../adr/0066-the-lit-tier-lights-in-linear.md)) with
replaceable shaders ([ADR-0067](../adr/0067-lit-shaders-are-embedded-and-replaceable.md)), and a
frame places its passes by what they read
([ADR-0068](../adr/0068-a-target-per-frame-a-checked-format-and-passes-placed-by-what-they-read.md)).
retcon's look-dev scene is a device case, and the plan's last step is the handoff retcon reads
when it adopts.

Three things came out differently. **A format mismatch is a validation error after all**, so the
recorder's check is for a run without the layers, and for naming the pass. **Passes into one
target keep their creation order whatever the reads move**, which a sort by reads alone broke by
putting an overlay under the colour pass. **And retcon's light matrix could not come across as it
is**: it is built with `glm::lookAt`, mirrored from this tree's cameras, so a shadow pass culled
with the cel pass's winding would have drawn the back faces.

[completed/RenderableComponent.md](completed/RenderableComponent.md) was drafted and closed on
2026-10-03, taking up [milestone 3](../roadmap/completed/m3-RenderableComponent.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md). An entity is drawn from an
`ecs::component::Transform` and a component per kind of drawing, which the api walks
([ADR-0063](../adr/0063-an-entity-is-drawn-from-a-transform-and-a-component-per-kind.md)). The
transform holds a quaternion rather than retcon's yaw, because a yaw interpolated between steps
turns the long way round. `realtime::sprites()` draws every `Sprite` into one depth order. The
mesh component is shaped and handed to [milestone 4](../roadmap/completed/m4-LitScene.md), because nothing
gives a mesh a handle yet.

Three things came out differently. **The prototype is a test here, not a change in cozy.** The
roadmap asked for cozy's sprites to be written against the record before it was accepted, and a
case in `SpriteTest.cpp` rebuilds cozy's scene from its camera profile and its constants instead.
Consumers adopt a feature after it ships. **That case found the plan had cozy's order
backwards**: from cozy's eye the acorn is nearer than the player, not further. **And a test
cannot return a `WorldCanvas` by value**: clang-tidy holds its move to not throwing, and its
`std::deque` allocates when moved, so the test fills one in place, as the canvas's other tests
already did.

[completed/LargeWorlds.md](completed/LargeWorlds.md) was drafted and closed on 2026-10-03,
taking up [milestone 2](../roadmap/completed/m2-LargeWorlds.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md). A texture can be released, with a
generation that refuses a stale handle and destruction deferred for the frames in flight
([ADR-0061](../adr/0061-a-resource-is-released-explicitly.md)). World quads can be handed to a
canvas in an order the caller keys, voxel culls its chunks, and `api/grid` builds a grid from a
map's picture and terrain legend, which odyssey now uses
([ADR-0062](../adr/0062-a-map-picture-and-legend-are-the-grids.md)). Regions, remembered sight
and the movement filter went to [TODO.md](../TODO.md#tile-grids) behind their triggers.

Four things came out differently. **The retirement queue is the ring's**, collected by
`Ring::begin()`, because the draft's `DeviceContext` drives no frame: `Presenter` and the device
suite both begin frames themselves. **The device harness could not release anything in
flight**, because its one submit waited for the frame; a submit that does not wait was split out
of it, and the release cases were shown to fail with the fix taken out. **`Registry::clear()`
never did what its comment said**: slots restarted at zero, so an earlier handle reached
whatever was added next, and it now releases every slot instead. And **`Picture` holds a
`std::optional<TileGrid>` and a vector of unknown glyphs**, rather than a shared pointer and a
map. The first keeps boost out of `api/grid`. The second exists because clang-tidy holds a move
constructor to not throwing, and MSVC's `std::map` allocates when it is moved.

[completed/MotionAndQueries.md](completed/MotionAndQueries.md) was drafted and closed on
2026-10-03, taking up [milestone 1](../roadmap/completed/m1-MotionAndQueries.md) of
[the game engine roadmap](../roadmap/completed/GameEngine.md): moya's `Plane` and `Frustum` moved into
`api/type` with the frustum told its depth range, a ray meeting a plane, box overlap, and a
component's previous step kept so that `Engine::alpha()` has a reader
([ADR-0060](../adr/0060-a-moving-thing-keeps-its-previous-step.md)). The sprite clip went to
[TODO.md](../TODO.md#sprite-sheets) behind a second consumer.

Four things came out differently. **The wrong depth range is not the same bug for both
cameras**: read as `[-1, 1]`, an orthographic camera's near plane falls far behind the eye, as
the plan said, but a perspective one's falls at half the near distance, still in front. The
tests put a box in that gap for each. **Pong had a suite** — the plan twice said it had none —
and it was the full build failing to link it, not a person playing, that showed the scene now
needed `api/type`. Its collision and scoring cases carried step 4 unchanged. **The positions
could not be copied**: `Position1D` and `Position2D` declared moves and no copy, which a
snapshot needs, and the ADR records that they gained one. And **watching pong after step 5
found a gameplay fault the plan had not looked for**: a paddle return reversed both components
of the ball's direction and ignored where the ball struck, so every rally was a horizontal line.
That was fixed beside the plan rather than as a step of it.

[completed/RealtimeGoldenImage.md](completed/RealtimeGoldenImage.md) was drafted and closed on
2026-09-12, out of what [RenderTestsInCI](completed/RenderTestsInCI.md) left behind. Five steps
pinning what the device suite draws to committed pictures, on the rule that a reference may hold
only what the specification determines pixel-for-pixel
([ADR-0054](../adr/0054-a-realtime-reference-is-a-picture-the-spec-determines.md)) — which is
what answers [ADR-0007](../adr/0007-ci-rendering-tests.md)'s objection that a golden image is one
rasterizer's output and so says nothing about another's. Four pictures are pinned and every one
of them is byte-exact on a Radeon and on the runner's lavapipe.

Its ordering was built around a claim the authoring machine cannot test: there is one gpu here,
the second implementation is the runner's, and CI runs on a pull request rather than on a branch.
So the first picture was a probe — the dullest one the suite could draw — and the rest waited on
what CI said about it. That is the shape to reuse for anything else blessed here: one artefact
through the whole loop before four of them are.

Three things came out differently. **Step 5's own text asserted the opposite of the ADR it
cites**: it said two overlapping world quads should give the same picture in either submission
order, while [ADR-0042](../adr/0042-a-textured-quad-in-world-space.md) says the caller supplies
the order and the depth-tested pipeline tests without writing, so one quad never occludes
another. The drafted assertion would have failed a correct renderer, and on its first run it
did. What landed pins a picture per order and asserts that the two differ, which is the
assertion a pipeline that started writing depth would fail.

**ADR-0054 had to be amended before step 4 could be written at all.** It required nearest
filtering; every sampler in the tree is `VK_FILTER_LINEAR` and nothing can ask for another, so
as written the rule admitted no textured picture. It now states the principle by stage — no
partially covered pixel, a texel returned unchanged at one texel per pixel, and a blend that is
the identity — which also covers the fact that the quad pipeline blends and an opaque source
makes that blend exact. The first probe picture had been accepted under wording that excluded
it.

**And the one red CI run was not the picture.** The step that re-runs the suite to turn a skip
into a failure ran the executable from the workspace root rather than from beside itself, so the
reference resolved to nothing while ctest, in the same job, passed the case. Nothing in that
suite had read a file before, so no earlier run could have caught it.

[completed/RenderTestsInCI.md](completed/RenderTestsInCI.md) was drafted on 2026-09-11 against
`88711c0` and closed the next day. Six steps building what
[ADR-0007](../adr/0007-ci-rendering-tests.md) decided on 2026-08-30 and nothing had since
implemented: the device half of `api/render/realtime` asserted by something other than a person
running an app and reading the validation log. Its ordering mattered because only one of the six
blocked anything — a device could not be selected without a surface, and every headless object
needs one that can be — while the two smallest were independent of it and useful to an app on
their own. The last step was last on purpose: the suites run against a real driver before they
are asked to run against a software one, so that a failure is the test's fault or lavapipe's but
never both at once.

Four defects came out of it, none of them the thing the step was looking for, and every one
found by running the code rather than by reading it — the plan lists them. The one that changed
the shape of the work was step 4's survey finding `Presenter` to be two classes wearing one
name, which earned [ADR-0051](../adr/0051-the-in-flight-ring-is-not-the-swapchain.md).

The last step is the one worth reading before writing CI against a driver again. Lavapipe was
never the obstacle it was expected to be, and the four red runs said nothing about it: a GitHub
runner is elevated, the Vulkan loader ignores `VK_DRIVER_FILES` and `VK_LAYER_PATH` in an
elevated process, and the driver it was pointed at was never loaded to be judged. What made that
visible was giving the failure a voice — the probe reporting the exception it caught, and the
job printing the log and the loader's own account — which took three runs to build and one to
answer.

[completed/ApiOrganisation.md](completed/ApiOrganisation.md) was drafted and closed on
2026-09-08, out of a survey of `api/`. Eleven steps, none of which changed behaviour: three that
changed how a header is named and seven that moved files, plus the documents. Its ordering was
the point — the first three steps were a barrier rather than a preference, because the tree had
763 relative includes across 337 files and no use at all of the include root
[ADR-0027](../adr/0027-the-api-is-consumed-as-source.md) had created, so every move made before
the conversion would have rewritten `../` counts in libraries and apps that had nothing to do
with it. Afterwards a move was a `git mv`, a `CMakeLists.txt` edit and a namespace line.

Four things came out differently. Step 4 was drafted to wait for
[OfflineRenderingPhase3](completed/OfflineRenderingPhase3.md) and did not need to, because both are
sequential commits on one branch rather than concurrent ones. `ui/immediate/` was not made:
`Immediate` is one of the library's two entry points rather than a concern within it, which is
the same test that kept `Layout` and `Arranger` at the top. Steps 7 and 10 became moves *and*
renames, because a group segment on an already deep namespace gave
`vulkan::pipeline::PipelineBuilder`. And two of step 9's three stated reasons turned out to be
wrong — `Loader.h` above `loader/` is a base class above its implementations, the same shape
`api/image` uses — while the step justified itself on a reason the plan had not predicted: it
separated four pairs of same-named types that had been told apart by scoping alone.

The plan also carries what the renames cost, which is the part worth reading before attempting
the same shape again: three substring collisions, one of which rewrote Vulkan's own
`VkPipelineCache`, and a forward declaration in the wrong namespace at nearly every step.

[completed/EmbeddingSeams.md](completed/EmbeddingSeams.md) was drafted and closed on
2026-09-07. Seven steps over the places where an `api/` library assumed the app it was hosting
was one of the four in this tree: a loop that offered an app no event of its own
([ADR-0043](../adr/0043-an-app-sees-an-event-before-the-bindings-do.md)), a text renderer that
was device-free in every line but one, an immediate layer that could not be asked whether it
wanted the cursor, an image reader that could only be pointed at a path, and a depth target
that was written and could not be sampled
([ADR-0044](../adr/0044-a-sampled-depth-target-is-read-only.md)). Its ordering mattered because
only one of the seven blocked anything — the event seam — while a different one was the only
one whose cost was growing, since a copy of `ui::TextRenderer` existed downstream for the sake
of a single line. Two entries the list was drafted from had closed before it was written, in
[GameFoundations](completed/GameFoundations.md).

Four things came out differently. Step 6's decoded image went on `asset::Model` rather than on
`type::Model::Material`, because `v3dlib_type` links glm alone and a material holding an image
would take `api/image` into both offline renderers. That step also found what nothing had: the
png reader installed no libpng error handler, so a truncated file aborted the process — which
did not matter while every png came from a file the tree shipped, and does once a reader can be
pointed at arbitrary bytes. Step 5 turned out to be a wrong measurement as well as a missing
control, since a widget's room was measured from the row margin even on a shared row. And step
7 has no headless case after all: `chooseFormat` asks a physical device for format properties,
so it needs one like everything else below the recorder.

[completed/GameFoundations.md](completed/GameFoundations.md) was drafted on 2026-09-07 against
this tree from outside it, and staged and closed here the same day. Thirteen steps taking up
what a game needs from these libraries that a demo does not: a document written whole or not at
all ([ADR-0041](../adr/0041-a-document-is-written-whole-or-not-at-all.md)), a textured quad in
world space ([ADR-0042](../adr/0042-a-textured-quad-in-world-space.md)), and the halves of
`api/ui` and `api/audio` that stop short. Its ordering mattered because the groups were largely
independent — four of them, and only two with an order inside — so the one defect it carried, an
editor save that truncated the previous project before writing the new one, did not wait behind
the structural work.

Two things came out differently. Step 8 gave `Keys::press` an argument for whether shift is
held, because a key name carries no modifier and `api/ui` cannot ask `api/input` for one without
taking SDL into a library that needs no window to test. And step 12 landed wholly in
`api/config`, because what a sprite sheet resolves to is a texture handle the app already holds.

[completed/UiConsolidation.md](completed/UiConsolidation.md) was drafted on 2026-09-06 out of an
architecture review of `api/ui` and closed on 2026-09-07. Fourteen steps over a library that
grew four ADRs in a day and had not had a pass over its shape since. Its ordering mattered for
two reasons. Three of its steps were defects shipping today - an opaque "translucent" panel, an
`Immediate` state map that grew without bound against an ADR saying it did not, and a `window()`
alpha argument that did nothing - and those were separable fixes that should not wait behind the
structural work. And the structural work had a strict order the other way: the names and the
headers before the draw path, because the draw path would otherwise be written twice, and the
style resolver before the renderer was split, because the resolver *is* one half of that split.

Two things came out differently. Step 8 did not split the layout half out of the renderer:
layout and paint are mutually recursive by design there, and the defect the split was meant to
close - two implementations of the strip rule, which disagreed about a left toolbar's width on
the first frame - was closed without it. And step 12 found what nothing had: a game that owns
the mouse has no cursor to give the immediate layer, so voxel's debug window cannot be folded.

[completed/OfflineRenderingPhase3.md](completed/OfflineRenderingPhase3.md) was drafted on
2026-09-05 and closed on 2026-09-10. Twelve steps taking up phase 3 of
[the offline rendering roadmap](../roadmap/completed/OfflineRendering.md) — light and surface — and
answering the question that roadmap had left open since it was written: shading is a language
([ADR-0026](../adr/0026-shading-is-a-language-over-a-batch.md)) rather than a fixed set of
shaders. That answer made it a subsystem rather than a weekend, and the plan states the
ordering cost rather than hiding it: phases 4 and 5 sat behind it.

Its ordering was almost entirely forced. One step — the surface normal — blocked everything and
depended on nothing, and the five that are the language are strictly sequential, because there
is no type checking an AST that does not parse and no running a program that has not been
compiled. Only the last four had any slack in them.

Four things came out differently, and each was a step's own text being wrong rather than the
ordering. `ambient()` cannot be a function over `illuminance` as step 7 assumed — a light with
no direction is one an illuminance loop cannot reach, which is exactly what makes it ambient.
`Shader` was already the syntax node, so step 8's shader instance is `sl::Instance`. Step 8's C
API bodies moved to step 9, because a body for any of them calls a render context method step 9
wrote. And step 10 had to decide something the plan left open — who calls `transmission` — since
RI's own standard lights ask nothing, so the three directional ones here do.

Two things it found that nothing else had. The analysis gates had never covered an app at all:
`out/build/verify` was configured with `V3D_BUILD_APPS=OFF`, so `/analyze` and clang-tidy had
only ever seen `api/`. And neither renderer could tell a pixel nothing was drawn into from a
black one, which an imager needs and which phase 1's fix for a black png had quietly cemented.

The phase closed the way it was meant to: the two pictures the tree drew before there was a
language are the pictures the language draws, unchanged, through every pass of it.

[completed/UiFoundations.md](completed/UiFoundations.md) was drafted on 2026-09-06 against this
tree from outside it, and staged and closed here the same day. Nine steps taking up what a ui
needs from the engine before it can have more than one text size: signed distance field glyphs
([ADR-0036](../adr/0036-text-is-a-distinct-kind-of-quad.md), which amends
[0005](../adr/0005-one-batched-quad-primitive.md)), menu input capture, a user settings path, and
a window that can be told its size. Its ordering mattered because two of its steps were defects
that shipped — pong's rebinding menu did nothing when activated, and an overflowing glyph atlas
reported success — and because four apps each hardcoded a font size around a constraint that was
the library's rather than theirs.

Two things came out differently. Step 4 could not be the additive step it was drafted as, so it
landed with step 2; and step 5 gave its size argument a default, which meant the four call sites
it was expected to break did not break, and step 6 became four apps choosing a size rather than
four apps being repaired.

[completed/GameLoopFoundations.md](completed/GameLoopFoundations.md) was drafted on 2026-09-05
against this tree from outside it, and staged and closed here on 2026-09-06. It took up three
gaps in the game loop that every app subclassing `v3d::engine::Engine` had worked around
separately or had failed to: a fixed simulation step
([ADR-0032](../adr/0032-the-loop-simulates-at-a-fixed-step.md)), window focus as an event, and
the camera profile loader moving out of the editor into `api/config`. Its ordering mattered
because the capability and the two-app bug fix behind it are separate commits — pong and
odyssey were advancing their worlds one increment per frame and so ran faster on a faster
machine.

[completed/ExternalApiConsumption.md](completed/ExternalApiConsumption.md) was drafted and
closed on 2026-09-05. It made the `api/` libraries buildable inside another repository's tree,
by the route [ADR-0027](../adr/0027-the-api-is-consumed-as-source.md) settles: as source,
through a root that nests, rather than as an installed package. Most of it was a correction the
tree needed regardless, since a library now states its own include root and propagates its own
dependencies where both used to come off globals in the root `CMakeLists.txt`.

[completed/OfflineRenderingPhase2.md](completed/OfflineRenderingPhase2.md) and
[completed/OfflineRenderingPhase1.md](completed/OfflineRenderingPhase1.md) were both drafted
and closed on 2026-09-05, taking up the first two phases of that roadmap: the shared
`api/render/offline` library and each renderer computing a pixel that came from geometry, then
one RIB reader dispatching onto an interface both renderers implement, and the editor's export
to the same format.

[completed/Modernization.md](completed/Modernization.md) closed on 2026-09-04, covering the
overlapping rewrites: SDL3, the legacy tree migration, Vulkan, the engine consolidation, and
the per-app ports.
