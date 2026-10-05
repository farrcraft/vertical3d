# Software Development Lifecycle

How work moves through this repository. This is a solo project, so the process exists for one
reason: work happens in bursts months apart, and the reasoning behind a change has to survive
the gap. Anything that does not serve that is ceremony and should be cut.

## 1. Plan

**A plan** lives in [plans/](plans/). A piece of work earns one when it spans several phases or
several apps, and the order of the pieces matters: when the question is *what blocks what*,
not *what needs doing*.

A plan changes as work lands, so keep its state notes current. When every phase is closed, the
plan moves to [plans/completed/](plans/completed/). Its unfinished items move to
[TODO.md](TODO.md). An open question that closed as "decided against" belongs in the ADR that
decided it. One that closed as "done, and nothing needs more yet" belongs in the document that
owns the subject. Neither goes in `TODO.md`, which holds only missing or unfinished work.

**An audit or a survey** lives in [audits/](audits/). It is written for a tree that is being
folded into the current layout or deleted. It lists what has to land elsewhere first, and it
is the only record of that tree once the tree is gone.

**A roadmap** lives in [roadmap/](roadmap/). It is written for an area nobody has started:
what the area would need, and in what order, written while the code is fresh in someone's
head. A roadmap is not a schedule. When one of its sections is started, that section gets a
plan and the roadmap links to it.

## 2. Decide

**A decision gets an ADR when all of these hold:**

- There was a real choice. At least one rejected option is one a competent engineer might have
  picked.
- The choice is hard to reverse, or it constrains other code.
- The reasons cannot be recovered by reading the code.

**These are not ADRs:**

- a bug fix
- the behaviour of one class or one widget
- a naming or file-layout choice
- a fact that is really reference material
- a choice whose only rival was leaving things as they were

Those belong in a comment beside the code, or in the reference document for the subject.

Records live in [adr/](adr/), indexed by area in [adr/README.md](adr/README.md). The
`architecture-decision-records` skill in `.claude/skills/` covers the format. Write the ADR
before or alongside the code. One written afterwards tends to justify what was built rather
than record what was weighed.

**An ADR and a reference document hold different things.** The ADR holds the reasoning: the
forces, the alternatives and the costs. The reference document holds the resulting rule, and
states it in full. When an ADR is accepted, the reference document is updated to state the
rule. It may link the ADR as background, but a reader must not need the ADR to follow the
rule.

## 3. Build

[Conventions.md](contributing/Conventions.md) has the house style, and [Build.md](contributing/Build.md) covers
configuring and building.

One concern per commit. The message says *why* where the diff does not make it obvious. Where a
commit implements a decision, the message names the ADR.

## 4. Verify

**Build.** Run `ninja -C out/build/x64-Debug`, or build one target by name. Everything in the
tree compiles and links, so a failure belongs to the change until shown otherwise.

**Lint and static analysis.** cpplint runs on every push. `/WX`, `/analyze` and clang-tidy run
locally. The tree is clean at all four, so every finding is a real one. See
[Linting.md](contributing/Linting.md).

**Tests.** Each api library has a Boost.Test binary, as does each app with logic worth
covering. Run them with `ctest --test-dir out/build/x64-Debug --output-on-failure`. A change
with a testable CPU half brings test cases with it. [Testing.md](contributing/Testing.md) records what is
covered and what cannot be.

**Rendering.** CI runs the device tests against lavapipe, a software Vulkan driver, and a few of
them compare against committed reference images. Locally, a rendering change is checked by
running the app and reading the log. The Khronos validation layer reports through the logger,
so a run with no validation messages is the signal. [Testing.md](contributing/Testing.md) has the detail.

## 5. Record

A change is not finished when it compiles. Before moving on:

- **Update the plan.** Update the open plan's state notes if the change moved its work, or
  [TODO.md](TODO.md) if the change falls outside any plan.
- **Update the reference document.** Update the document that owns whatever the change moved.
  [README.md](README.md) says which document owns which subject. Update `../CLAUDE.md` only if
  the change moved the routing, one of its rules, or the shape of the tree.
- **Re-read the comments the change added** against [Conventions.md](contributing/Conventions.md#writing).
  Comments are written while the reasoning is fresh, so they collect history, ADR citations and
  long sentences. A second pass removes them in a minute.
- **Set the ADR status.** Set a status when a proposed decision is accepted. When a later ADR
  changes an earlier one, add `Amended by` to the earlier one's header and set its status to
  `amended`. When the later one replaces it outright, set `Superseded by` and `superseded`.
  Change the header, not the body.
