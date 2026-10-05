---
name: architecture-decision-records
description: Write an ADR for a technical decision in this project, in the house format. Use when a choice between real alternatives is being made that is hard to reverse or constrains other code, and when someone asks why an existing decision was made. Covers what qualifies, titles, numbering, the index, amending and superseding.
---

# Architecture Decision Records

`docs/adr/` records why the codebase is shaped as it is. It holds `template.md`, an index in
`README.md` grouped by area, and a numbered run of records. Read the index for the next free
number. Numbers are never reused, including the numbers of deleted records.

The process rule is in [`docs/sdlc.md`](../../../docs/sdlc.md) §2. This skill is how to carry
it out.

## Does it qualify?

Write an ADR only when **all three** hold:

1. **There was a real choice.** At least one rejected option is one a competent engineer might
   have picked. "Leave it as it is" does not count as a rival unless it was genuinely viable.
2. **It is hard to reverse, or it constrains other code.** A library boundary, a data format, a
   contract between modules, a version floor.
3. **The reasons cannot be recovered from the code.**

**Not an ADR**, whatever its length:

- a bug fix, even an interesting one (that goes in the commit message)
- the behaviour of one class or one widget (that goes in its header)
- a naming or file-layout choice (that goes in `docs/contributing/Conventions.md`, if anywhere)
- reference material such as a list of fields, a formula or a table of behaviours (that goes in
  the reference doc)

When a write-up runs long, the length is not evidence that it is a decision. A long account of
a fix is still a fix.

## Where the content goes

An ADR holds the **reasoning**: the forces, the alternatives and the costs. The **current
rule** is stated in full in a reference document or a header, which the ADR names in its
`Documented in` line. When you write an ADR, update that document in the same change. The
document may end a section with `Background: ADR-NNNN`, but it must make sense without it.

Code comments never cite an ADR. They state the rule.

## The format

Copy [`docs/adr/template.md`](../../../docs/adr/template.md). Do not invent a variant.

**Title.**

- The title is `Area: choice`, in sentence case, about eight words or fewer. Examples:
  `Loop: fixed-step simulation, variable-rate rendering`, `Files: write documents atomically`.
- A reader scanning the index should know what was decided without opening the file.
- One decision per record. If the title needs "and" to join two choices, write two records.
- Use plain words. No metaphors, and no code identifiers unless the identifier is the subject.

**File name.** `NNNN-` followed by the title in kebab case, for example
`0032-loop-fixed-step-simulation-variable-rate-rendering.md`.

**Header.** `Status` is one of proposed, accepted, amended or superseded. The header also
carries `Date`, `Amends`, `Amended by`, `Supersedes`, `Superseded by` and `Documented in`. Omit any link line
that has nothing to link.

**Context.** Five sentences at most, in the present tense. Give the constraints and forces that
make a choice necessary. Do not tell the story of how the problem was found. Do not name other
repositories, issue IDs or plan phases.

**Decision.** Three sentences at most. Leave out tables, field lists and implementation detail.

**Alternatives.** List rejected options only. The chosen option is the Decision; do not list it
again. Each rejected option gets For, Against and Rejected because.

**Consequences.** Gains, Costs and Revisit when. The Costs line must be honest; a record with
no costs is not finished. Leave out counts, run logs, test totals and migration steps, because
they stop being true. They belong in the commit message.

**Length.** The record should read in about two minutes.

## Writing one

1. Take the next number from the index.
2. Draft it from `template.md`, and leave no placeholder text.
3. Add a row to the right area table in `docs/adr/README.md`: `| [NNNN](file.md) | Title | status |`.
4. Update the document named in `Documented in` so that it states the rule.
5. **Show it to the user before committing.** The decision is theirs. A record marked
   `accepted` that the user never accepted is a false statement that outlives the
   conversation.
6. Commit it with the code it governs. The commit message names the ADR.

## Amending, superseding and retiring

- **A later decision changes part of an earlier one.**
  - Add `Amends: ADR-X` to the new record.
  - Add `Amended by: ADR-Y` to the old one, and set its status to `amended`.
  - Edit the old record's header, not its body.
- **A later decision replaces an earlier one.**
  - Use `Superseded by` and the status `superseded` instead.
  - The old file stays, so the reasoning that was replaced is still readable.
- **A correction of fact** (a renamed class, a wrong count) is edited in place.
- **A record that turns out not to be a decision is deleted.**
  - Its content moves to the doc or header where it belongs.
  - Its row in the index stays, saying where the content went.

## Answering "why was this done?"

Scan the index for the area, then read that record's Context and Decision. If no record
covers it, say so and offer to write one. Do not reconstruct a rationale from the code; the
point of the record is that the code cannot carry it.
