# ADR-0258: CI: workflows are audited by zizmor and actionlint

**Status**: accepted
**Date**: 2026-11-21
**Tags**: ci, tooling

## Context

A workflow runs with a token, and the release and the documentation site's jobs hold one that can
publish. A mistake in a workflow is then a way into what the repository ships. It can be a token
left on disk, an expression expanded into a shell script, a cache a release restores, or a tag
moved after review. A workflow can also be wrong without being unsafe, through a misspelt job
in `needs:`, an expression of the wrong type or a shell script's bug, and that shows only when the
job runs. CodeQL reads the workflows for security in CI alone, and the pin check reads nothing else.

## Decision

zizmor audits every workflow and Dependabot's configuration for security, and actionlint checks
every workflow for correctness. Each runs in pre-commit and in a CI job that fails on a finding.
zizmor's audit of pins replaces the tree's own pin check, and CodeQL's queries for workflows go on
running.

## Alternatives

### zizmor alone
- **For**: one tool to pin and update, and it covers every security class the others do.
- **Against**: a workflow's correctness is checked only by running it, and a job that runs only for
  a release is run only then.
- **Rejected because**: a mistake in a release's job is found at the release, when it costs most.

### zizmor's findings in code scanning, not failing the job
- **For**: findings are kept as alerts with their history, beside CodeQL's, and a finding can be
  dismissed with a reason.
- **Against**: a finding never fails the run, so blocking it depends on a ruleset over code
  scanning, and pre-commit reports it differently from CI.
- **Rejected because**: a finding in a workflow is fixed before the change merges, as a finding in
  the code is, and a failing job says so in the place a contributor looks.

### CodeQL's queries for workflows alone
- **For**: they run already, and need nothing more to keep up to date.
- **Against**: they cover fewer classes, check nothing about correctness, and cannot run before a
  commit.
- **Rejected because**: a finding would reach a pull request every time, rather than being caught
  where the change is made.

## Consequences

- **Gains**: a workflow is checked before it is committed, as the code is. A release's job is
  checked for correctness on every change to it. The pin check is one tool's audit rather than a
  script the tree keeps.
- **Costs**: two more tools to pin and update. zizmor's online audits need a token, and run only in
  CI. actionlint checks shell scripts only where shellcheck is installed.
- **Revisit when**: either tool stops being maintained, or CodeQL's queries for workflows cover
  what zizmor's audits do and run locally.
