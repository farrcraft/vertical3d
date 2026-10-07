# ADR-0083: Review: a changeset answers for what it introduces

**Status**: accepted
**Date**: 2026-10-06
**Documented in**: [contributing/Review.md](../contributing/Review.md)

## Context

A review that reads whole files reports every defect it finds, whether the changeset made it or
it was there before. In a tree with old code, that means a changeset that touches an old file can
never come back clean, and each fix round reads more of the file and finds more. Some defects
recur in the same few shapes, such as an unchecked conversion at an input boundary or a comment
that has drifted from the code. A tool finds those more reliably than a reader. A fix that
corrects one site of a shape and not its siblings is found again by the next review.

## Decision

A changeset's review is clean when the changeset introduces no finding, at any severity, and a
changeset merges only when its review is clean. A defect the changeset did not introduce is
recorded in the known-debt list and is not a finding against it. The classes a tool can find are
checked by a tool, and a reviewer reports one of them only when the tool missed it.

## Alternatives

### Judge a changeset on every defect in the files it touches
- **For**: old defects are found while the file is open, and nothing is left behind.
- **Against**: a change to an old file cannot pass until the whole file is clean, and the review
  grows with the file rather than with the change.
- **Rejected because**: it makes a clean review depend on history the change did not write.

### Block on major findings only, and record minor ones as debt
- **For**: reviews close quickly, and minor findings are still written down.
- **Against**: minor findings a change introduces become debt the moment it merges, so the debt
  grows with every change.
- **Rejected because**: a change should not add its own defects to the debt, and a new minor
  finding costs least to fix while the change is fresh.

## Consequences

- **Gains**:
  - A clean review is reachable for any changeset, however old the code around it.
  - The known-debt list is the one place old defects live, and a changeset never adds a defect of
    its own to it.
  - Reviewers spend their reading on what a tool cannot check.
- **Costs**:
  - Every finding has to be traced to whether the change introduced it, which takes judgement
    when a change moves or rewrites old code.
  - Old defects in a file a change touches stay until someone takes them on.
  - The gates have to be built, and kept, before the rule can hold.
- **Revisit when**: entries on the known-debt list stay open long after their files are next
  changed, or the gates miss a class often enough that reviewers are finding it again.
