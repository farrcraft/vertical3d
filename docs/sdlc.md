# Software Development Lifecycle

How work moves through this repository. This is a solo project, so the process exists for
one reason: work happens in bursts months apart, and the reasoning behind a change has to
survive the gap. Anything that does not serve that is ceremony and should be cut.

Some of what follows describes practice that is not in place yet. Those parts are marked.

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

**Build.** The authoritative check today. `ninja -C out/build/x64-Debug`, or one target at a
time. `tetris`, `voxel` and `odyssey` are known broken for reasons recorded in `CLAUDE.md`;
check that list before assuming a failure is yours.

**Lint.** cpplint, per the command in `CLAUDE.md`, run in CI by
[`.github/workflows/cpplint.yml`](../.github/workflows/cpplint.yml). Every file reports
`whitespace/indent_namespace` because the filter in that workflow names the check by its old
identifier; ignore those and treat everything else as real.

**Tests.** *Not in place.* Three tiers are planned — unit tests per api library, an
integration suite across libraries, and per-app suites. See the testing section of the
modernization plan.

**Render verification.** Nothing renders until the Vulkan frame loop lands, so "it builds"
is currently the only available signal for the render layer. Once there is a frame loop, CI
runs render tests against a software Vulkan implementation per
[ADR-0001](adr/0001-ci-rendering-tests.md).

## 5. Record

A change is not finished when it compiles. Before moving on:

- Update the plan's state notes if the change moved a workstream.
- Update `CLAUDE.md` if it changed the architecture, the build, or a convention — that file
  is what a new session reads first, and a stale one actively misleads.
- Set an ADR's status when a proposed decision is accepted, deprecated, or superseded.
  Superseding does not delete: leave the old file in place and point it at the new one.
