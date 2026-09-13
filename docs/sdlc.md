# Software Development Lifecycle

How work moves through this repository. This is a solo project, so the process exists for one
reason: work happens in bursts months apart, and the reasoning behind a change has to survive
the gap. Anything that does not serve that is ceremony and should be cut.

## 1. Plan

Phased plans live in [`plans/`](plans/). A workstream earns a plan document when it spans
several phases or several apps and the ordering between the pieces is not obvious — when the
interesting question is *what blocks what*, not *what needs doing*.

A plan is a living document: update its state notes as things land. When every phase is closed
it moves to [`plans/completed/`](plans/completed/), and its open items move to
[`TODO.md`](TODO.md) — the ones that are still work. A plan's open questions as often close as
*decided against* or *complete, and nothing needs it yet*; the first belongs in the ADR that
settled it and the second in the document that owns the subject. `TODO.md` holds what is missing
or unfinished, and neither of those is.

[`plans/completed/EmbeddingSeams.md`](plans/completed/EmbeddingSeams.md) closed on 2026-09-07
and took up the seams an app that brings its own ui, renderer and `main` needs from
`api/engine`, `api/ui`, `api/image`, `api/asset` and `api/render`.
[`plans/completed/ApiOrganisation.md`](plans/completed/ApiOrganisation.md) closed on 2026-09-08.
It settled the include convention every file in the tree had disagreed with `examples/starter`
about ([ADR-0048](adr/0048-an-api-header-is-named-from-the-repository-root.md)) and split the six
`api/` directories that held more than one thing.
[`plans/completed/OfflineRenderingPhase3.md`](plans/completed/OfflineRenderingPhase3.md)
closed on 2026-09-10. It took up phase 3 of the offline rendering roadmap and answered the
question that roadmap had left open since it was written: shading is a language
([ADR-0026](adr/0026-shading-is-a-language-over-a-batch.md)) rather than a fixed set of
shaders.
[`plans/completed/GameFoundations.md`](plans/completed/GameFoundations.md) closed on 2026-09-07
and took up what a game needs from `api/asset`, `api/engine`, `api/event`, `api/render`,
`api/ui` and `api/audio` that a demo does not.
[`plans/completed/UiConsolidation.md`](plans/completed/UiConsolidation.md) closed on 2026-09-07
and took up the shape of `api/ui` after four ADRs landed on it in a day: three defects that
shipped, a draw path that allocated per component per frame, and a widget set whose cursor half
had no consumer.
[`plans/completed/ExternalApiConsumption.md`](plans/completed/ExternalApiConsumption.md) closed
on 2026-09-05 and made the `api/` libraries buildable inside another repository's tree.
[`plans/completed/OfflineRenderingPhase2.md`](plans/completed/OfflineRenderingPhase2.md) and
[`plans/completed/OfflineRenderingPhase1.md`](plans/completed/OfflineRenderingPhase1.md) both
closed on 2026-09-05 and took up the first two phases of that roadmap.
[`plans/completed/Modernization.md`](plans/completed/Modernization.md) closed on 2026-09-04 and
covered the overlapping rewrites: SDL3, the legacy tree migration, Vulkan, the engine
consolidation, and the per-app ports. Unphased work is in [`TODO.md`](TODO.md).

A tree being folded into the current layout or deleted gets a survey or an audit instead: an
itemised list of what has to land elsewhere first, and the only account of that tree once it is
gone. Those live in [`audits/`](audits/), and move to
[`audits/completed/`](audits/completed/) when the list is worked off.

An area nobody has taken up gets a roadmap, in [`roadmap/`](roadmap/): what it would need and
in what order, written while the code is still fresh in someone's head. A roadmap is not
scheduled and does not close. **When one of its sections is taken up it earns a plan**, and the
roadmap points at it.

## 2. Decide

**Significant decisions get an ADR.** Significant means hard to reverse, constrains later
phases, or a future reader would otherwise ask "why on earth is it done this way".

Records live in [`adr/`](adr/), indexed in [`adr/README.md`](adr/README.md). The
`architecture-decision-records` skill in `.claude/skills/` covers the format and the mechanics
of adding one.

Write the ADR **before or alongside** the implementation. One written afterwards tends to
justify what was built rather than record what was weighed.

Anything smaller than an ADR — a naming choice, a local refactor, a constant whose reasoning
fits in two lines — belongs in a comment beside the thing it explains, not in a separate
document.

`adr/` is the only home for decisions. A plan may link to them, but must not restate them: a
decision recorded twice will eventually disagree with itself.

## 3. Build

Conventions — namespaces, file extensions, pointer types, logging, line endings — are in
[`Conventions.md`](Conventions.md) and are not repeated here. [`Build.md`](Build.md) covers
configuring and building the tree.

One concern per commit. The commit message should say *why* where the reason is not obvious
from the diff, since a diff already says what changed. Where a commit implements a decision,
say which ADR settles it rather than restating the ADR.

## 4. Verify

**Build.** `ninja -C out/build/x64-Debug`, or one target at a time. Everything in the tree
compiles and links, so a failure is the change's until shown otherwise.

**Lint and static analysis.** cpplint on every push, plus `/WX`, `/analyze` and clang-tidy
locally. The tree is clean at all four, so every finding is a real one. See
[`Linting.md`](Linting.md).

**Tests.** Boost.Test, one binary per api library plus one per app that has logic worth
covering, registered with ctest: `ctest --test-dir out/build/x64-Debug --output-on-failure`.
[`Testing.md`](Testing.md) records what is covered and what cannot be; everything in
`api/render` below the recorder needs a window and a GPU. A change with a testable cpu half is
expected to bring cases with it.

**Render verification.** The frame loop is in and the apps draw, but CI still renders nothing,
so a rendering change is verified by running the app and reading the log. The Khronos
validation layer is routed through the logger, and a silent run is the signal. Automating this
against a software Vulkan implementation is [ADR-0007](adr/0007-ci-rendering-tests.md), which
has not been done.

## 5. Record

A change is not finished when it compiles. Before moving on:

- Update the open plan's state notes if the change moved a workstream, or `docs/TODO.md` if the
  change falls outside what that plan covers.
- Update the document that owns whatever the change moved. [`Architecture.md`](Architecture.md),
  [`Build.md`](Build.md), [`Testing.md`](Testing.md) and the rest are each the single home for
  their subject, and [`README.md`](README.md) says which is which. Update `../CLAUDE.md` only if
  the change moved the routing or one of the rules it keeps in front of an agent.
- Re-read the comments the change added, against [`Conventions.md`](Conventions.md#comments).
  Comments are written while the reasoning is loudest and the surrounding decision is not yet
  recorded, so they collect provenance, ADR summaries and narration that a second pass removes
  in a minute.
- Set an ADR's status when a proposed decision is accepted, deprecated, or superseded.
  Superseding does not delete: leave the old file in place and point it at the new one.
