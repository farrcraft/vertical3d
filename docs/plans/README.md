# Plans

A workstream earns a plan document when it spans several phases or several apps and the
ordering between the pieces is not obvious — when the interesting question is *what blocks
what*, not *what needs doing*. See [sdlc.md](../sdlc.md).

A plan is a living document while it is open, so update its state notes as things land. When
every phase is closed it moves to [completed/](completed/), and any open item it was carrying
moves to [TODO.md](../TODO.md). The plan itself stays, because the reasoning behind an ordering
outlives the schedule.

[completed/ApiOrganisation.md](completed/ApiOrganisation.md) was drafted and closed on
2026-09-08, out of a survey of `api/`. Eleven steps, none of which changed behaviour: three that
changed how a header is named and seven that moved files, plus the documents. Its ordering was
the point — the first three steps were a barrier rather than a preference, because the tree had
763 relative includes across 337 files and no use at all of the include root
[ADR-0027](../adr/0027-the-api-is-consumed-as-source.md) had created, so every move made before
the conversion would have rewritten `../` counts in libraries and apps that had nothing to do
with it. Afterwards a move was a `git mv`, a `CMakeLists.txt` edit and a namespace line.

Four things came out differently. Step 4 was drafted to wait for
[OfflineRenderingPhase3](OfflineRenderingPhase3.md) and did not need to, because both are
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

[OfflineRenderingPhase3.md](OfflineRenderingPhase3.md) **is open**, drafted on 2026-09-05. It
takes up phase 3 of [the offline rendering roadmap](../roadmap/OfflineRendering.md) — light and
surface — and answers the question that roadmap left open: shading is a language rather than a
fixed set of shaders. That answer makes it a subsystem rather than a weekend, and phases 4 and
5 sit behind it.

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
