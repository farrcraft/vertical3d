# ADR-0164: Tooling: commits are checked locally through pre-commit

**Status**: accepted
**Date**: 2026-11-03
**Documented in**: docs/contributing/Gates.md

## Context

CI runs cpplint and the review gates on a pull request, and a branch here can hold many commits
before it has one. A finding then surfaces far from the commit that added it. A local check has
to judge what is being committed, and the tree is often committed in part while other edits stay
unstaged. cpplint is pinned because a newer release adds checks, so a local run has to use the
version CI uses. Running a gate must still need only Node.

## Decision

A `.pre-commit-config.yaml` at the repository root runs cpplint, pinned to the commit of the
version CI installs, and each quick gate as a local hook that runs Node. pre-commit sets unstaged
changes aside while the hooks run. Installing it is a contributor's choice, and CI stays the final
check.

## Alternatives

### A plain git hook that runs the gates
- **For**: no tool to install beyond Node, and the gates could run in parallel, in about half the
  time.
- **Against**: the gates read the working tree, so the hook judges unstaged edits rather than the
  commit. Reading the index instead is a change to every gate. It cannot install cpplint at a
  pinned version.
- **Rejected because**: a hook that checks the wrong snapshot gets skipped.

### Lefthook from npm
- **For**: one fast binary, with hooks run in parallel.
- **Against**: npm installs it through an install script, which `.npmrc` turns off, and it would
  be the first npm package that is not a type-checking tool. It does not set unstaged changes
  aside.
- **Rejected because**: it breaks the rule that npm brings only the type checker, and does not
  solve the snapshot problem.

### CI only
- **For**: nothing to install, and one place where the checks run.
- **Against**: a finding waits for a pull request, which can be many commits later.
- **Rejected because**: the cost of a finding grows with the commits built on it.

## Consequences

- **Gains**: a finding fails the commit that adds it, against the staged text. cpplint runs
  locally at the version CI runs, without a manual install.
- **Costs**: pre-commit needs Python on the machine, which cpplint already needed. Each commit
  waits for the gates, longest when it touches C++. While the hooks run, unstaged edits are off
  the disk, so agents sharing a working tree must not commit while another is building or
  editing. Each gate added has a hook to add as well.
- **Revisit when**: the gates can read the index themselves, which would let a plain hook check
  the commit without a third-party tool.
