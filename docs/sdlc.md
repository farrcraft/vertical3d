# Software Development Lifecycle

How work moves through this repository. This is a solo project, so the process exists for
one reason: work happens in bursts months apart, and the reasoning behind a change has to
survive the gap. Anything that does not serve that is ceremony and should be cut.

## 1. Plan

Phased plans live in [`plans/`](plans/). A workstream earns a plan document when it spans
several phases or several apps and the ordering between the pieces is not obvious — when
the interesting question is *what blocks what*, not *what needs doing*.

[`plans/Modernization.md`](plans/Modernization.md) is the current one, covering the
overlapping rewrites: SDL3, the legacy tree migration, Vulkan, the engine consolidation, and
the per-app ports.

A plan is a living document. When something lands, update the state notes in it rather than
leaving them to rot — a plan describing a tree that no longer exists is worse than no plan.

## 2. Decide

**Significant decisions get an ADR.** Significant means hard to reverse, constrains later
phases, or a future reader would otherwise ask "why on earth is it done this way".

Records live in [`adr/`](adr/), indexed in [`adr/README.md`](adr/README.md). The
`architecture-decision-records` skill in `.claude/skills/` covers the format and the
mechanics of adding one.

Write the ADR **before or alongside** the implementation. One written afterwards tends to
justify what was built rather than record what was weighed.

Anything smaller than an ADR — a naming choice, a local refactor, a constant whose
reasoning fits in two lines — belongs in a comment beside the thing it explains, not in a
separate document.

`adr/` is the only home for decisions. A plan may link to them, but must not restate them —
a decision recorded twice is a decision that will disagree with itself.

## 3. Build

Conventions — namespaces, file extensions, pointer types, logging, line endings — are in
[`../CLAUDE.md`](../CLAUDE.md) and are not repeated here.

One concern per commit. The commit message should say *why* where the reason is not obvious
from the diff; a diff already says what changed. Where a commit implements a decision, say
which ADR settles it rather than restating the ADR.

`.gitattributes` enforces LF. Editors that write CRLF turn a small change into a whole-file
diff, so strip the CRs rather than committing them.

## 4. Verify

**Build.** `ninja -C out/build/x64-Debug`, or one target at a time. Everything compiles and
links; `CLAUDE.md` keeps a build health list, so check it before assuming a failure is
yours.

**Lint.** cpplint, per the command in `CLAUDE.md`, run in CI by
[`.github/workflows/cpplint.yml`](../.github/workflows/cpplint.yml). Every file reports
`whitespace/indent_namespace` because the filter in that workflow names the check by its old
identifier; ignore those and treat everything else as real.

**Tests.** Boost.Test, one binary per api library plus one per app that has logic worth
covering, registered with ctest: `ctest --test-dir out/build/x64-Debug --output-on-failure`.
`CLAUDE.md` records what is covered and what is not — everything in `api/render` below the
recorder needs a window and a GPU and is not. A change with a testable cpu half is expected
to bring cases with it.

**Render verification.** The frame loop is in and the apps draw, but CI still renders
nothing, so a rendering change is verified by running the app and reading the log: the
Khronos validation layer is routed through the logger, and a silent run is the signal.
Automating it against a software Vulkan implementation is
[ADR-0007](adr/0007-ci-rendering-tests.md), which has not been done.

## 5. Record

A change is not finished when it compiles. Before moving on:

- Update the plan's state notes if the change moved a workstream.
- Update `CLAUDE.md` if it changed the architecture, the build, or a convention — that file
  is what a new session reads first, and a stale one actively misleads.
- Re-read the comments the change added, against the comment convention in `CLAUDE.md`.
  Comments are written while the reasoning is loudest and the surrounding decision is not yet
  recorded, so they collect provenance, ADR summaries and narration that a second pass
  removes in a minute.
- Set an ADR's status when a proposed decision is accepted, deprecated, or superseded.
  Superseding does not delete: leave the old file in place and point it at the new one.
