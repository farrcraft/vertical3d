# Plans

A workstream earns a plan document when it spans several phases or several apps and the
ordering between the pieces is not obvious — when the interesting question is *what blocks
what*, not *what needs doing*. See [sdlc.md](../sdlc.md).

A plan is a living document while it is open, so update its state notes as things land. When
every phase is closed it moves to [completed/](completed/), and any open item it was carrying
moves to [TODO.md](../TODO.md). The plan itself stays, because the reasoning behind an ordering
outlives the schedule.

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
[the offline rendering roadmap](../roadmap/OfflineRendering.md) — light and surface — and
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
