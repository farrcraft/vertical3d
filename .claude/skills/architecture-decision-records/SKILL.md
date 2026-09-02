---
name: architecture-decision-records
description: Write an ADR for a technical decision in this project, in the house format. Use when a choice is being made that is hard to reverse, constrains later phases, or a future reader would ask "why on earth is it done this way" — and when someone asks why an existing decision was made. Covers numbering, the index row, superseding, and what is too small to record.
---

# Architecture Decision Records

`docs/adr/` is this project's record of why the codebase is shaped the way it is: a
`template.md`, an index in `README.md`, and a numbered run of accepted records — the Vulkan
rewrite, then the editor. Read the index for the current count and add the next number; you
are extending the log, not bootstrapping it.

The process rule lives in [`docs/sdlc.md`](../../../docs/sdlc.md) §2 ("Decide"). This skill
is how to carry it out.

## When a decision needs an ADR

The test from `sdlc.md`, verbatim: **significant means hard to reverse, constrains later
phases, or a future reader would otherwise ask "why on earth is it done this way."**

Decisions made *during* a phase count. A choice that has to be settled before the code can
be written is exactly the kind that gets forgotten once the code exists.

Write the ADR **before or alongside** the implementation, not after it. An ADR written
afterwards tends to justify what was built instead of recording what was weighed.

### Signals

- Choosing between two libraries, two algorithms, or two places to put a rule
- A version, a limit, or a contract that later phases build on — the Vulkan version the
  renderer targets, the shape of a draw submission, what belongs in `api/` versus an app
- Closing a gap an earlier ADR or plan explicitly left open
- The user says "let's go with X", "record this", or asks "why did we choose X?"
- A plan lists something under its open questions and the answer arrives

### Too small for an ADR

`sdlc.md` and `CLAUDE.md` cover the rest. Do not write an ADR for naming, formatting, a
local refactor, or a constant whose reasoning fits beside it in the header. That header is
usually the right home for a number.

## Where decisions live

`docs/adr/` only. `docs/plans/Modernization.md` links to the records that shape it and does
not restate them; keep it that way. A decision recorded in two places is a decision that
will eventually disagree with itself.

## The house format

Copy [`docs/adr/template.md`](../../../docs/adr/template.md) and fill it in. Do not invent a
variant; consistency across the record is the point.

```markdown
# ADR-NNNN: [Decision Title]

**Date**: YYYY-MM-DD
**Status**: proposed | accepted | deprecated | superseded by ADR-NNNN
**Deciders**: Joshua Farr

## Context

[2-5 sentences: the situation, the constraints, the forces at play]

## Decision

[1-3 sentences stating the decision clearly]

## Alternatives Considered

### Alternative 1: [Name]
- **Pros**: [benefits]
- **Cons**: [drawbacks]
- **Why not**: [specific reason this was rejected]

## Consequences

### Positive
- [benefit]

### Negative
- [trade-off]

### Risks
- [risk and mitigation]
```

**Deciders is `Joshua Farr`.** This is a solo project; do not invent a committee.

### What a good one does

- **Context says what forced the decision, and when.** "The consolidation removed the
  `SDL_Renderer` software fallback, and free-tier CI runners have no GPU" beats a paragraph
  of background.
- **Alternatives are real.** A rejected option with a genuine Pro is worth recording; a
  straw man is not. Mark the chosen one `— **chosen**` where it is one of the listed
  alternatives.
- **Negative consequences are honest.** An ADR with an empty Negative section is not
  finished. ADR-0007 admits a green CI run is weaker evidence than it looks, because a
  software rasterizer exercises the API and not the driver; ADR-0003 admits the engine
  consolidation costs a software fallback.
- **Risks name the escape hatch.** Say what a future change would have to touch — "the
  escape hatch is the workflow file and the test target's guard, not the engine."
- **Cross-reference by number**, and link the plan section where a decision builds on one.
- **It reads in about two minutes.** If Context runs past ten lines, cut it.

## Writing one

1. **Number it.** `ls docs/adr/` and take the next number. Four digits, zero-padded.
2. **Name the file** `NNNN-kebab-case-title.md`. The title inside is
   `# ADR-NNNN: Subject — Short Statement Of The Decision`, using the em-dash form.
3. **Draft it from `template.md`.** Delete the placeholder prose; leave no bracketed hints.
4. **Add the index row** at the bottom of the table in `docs/adr/README.md`:
   ```
   | [NNNN](NNNN-slug.md) | Title — Short Statement | accepted | YYYY-MM-DD |
   ```
   The title in the row matches the title in the file.
5. **Show it before committing it.** The decision is the user's; the writing is yours. A
   record whose Status is `accepted` when the user has not accepted it is a lie in the file
   that outlives the conversation.
6. **Commit it with the code it governs** where the two land together — the commit message
   should say the ADR settles the choice, not restate the ADR.

## Reading and superseding

**When asked why a decision was made**, scan `docs/adr/README.md` for the row, then read
that ADR's Context and Decision. If nothing matches, say so and offer to record it — do not
reconstruct a rationale from the code, because the whole point of the record is that the
code cannot carry it.

**Superseding does not delete.** Set the old ADR's status to `superseded by ADR-NNNN`,
leave the file in place, update its row in the index, and have the new ADR say in its
Context what the old one got wrong and what changed. Same for `deprecated` when the thing
the decision governed no longer exists.

**Amending in place** is right for a small correction of fact. A change of decision is a
new ADR.

## Categories that come up in this project

| Category | Examples |
|---|---|
| Toolchain and build | MSVC-only, CMake, vcpkg manifest mode, the Vulkan version targeted, what stays out of the root `add_subdirectory` list |
| Rendering architecture | one engine rather than two, draw data over draw code, the batched quad primitive, the pass model |
| Library boundaries | what belongs in `api/` versus an app, which libraries are allowed a realtime dependency, where a legacy tree's contents land |
| Migration and removal | deleting a backend outright rather than keeping a fallback, retiring or keeping an app |
| Process and quality gates | cpplint, the test tiers, what CI runs and on what runner |

Library boundaries are the category most easily missed, because a misplacement looks like a
file-organisation question rather than a decision. It is not: `moya` and `talyn` are offline
renderers with no realtime dependency, and keeping that true is what stops `api/type` and
`api/image` from quietly acquiring one.
