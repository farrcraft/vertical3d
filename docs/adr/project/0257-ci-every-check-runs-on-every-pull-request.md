# ADR-0257: CI: every check runs on every pull request

**Status**: accepted
**Date**: 2026-11-21
**Tags**: ci, testing, review
**Amends**: ADR-0174
**Documented in**: [CI.md](../../contributing/CI.md)

## Context

CI's checks include long ones: the suites under the sanitizers, moya's second colour build, the
whole tree with two Linux compilers, the Flatpak and the fuzzers. Together they take longer than
a pull request should wait, and longer than some jobs' time limits. A changeset answers for what it
introduces, so a check that runs only after a merge finds a defect that is already on main. Runners
cost nothing for a public repository, and a merge queue needs a repository an organisation owns.
Fuzzing finds more the longer it runs, where a suite finds the same on every run.

## Decision

Every check runs on every pull request that changes more than documents, and again on main.
A check that grows too long is made faster or split across runners, never moved off pull
requests. Fuzzing runs briefly on each pull request, and in long batches on a schedule.

## Alternatives

### The long checks on main and nightly
- **For**: a pull request learns of most failures in minutes rather than an hour, and the long
  checks run less often.
- **Against**: a defect only a long check finds is merged before it is found, and the change that
  caused it has to be found among the merges since.
- **Rejected because**: it breaks the rule that a change answers for what it introduces, to save
  runner time that costs nothing.

### The long checks on a label
- **For**: a pull request that touches nothing a long check reads is fast, and one that does can
  still be checked in full before it merges.
- **Against**: whether a change needs the long checks is judged by whoever labels it, and a wrong
  judgement passes silently. A rule that labels by path has to know what every check reads.
- **Rejected because**: the saving rests on a judgement made before the checks have run, which is
  the judgement the checks exist to replace.

## Consequences

- **Gains**: a pull request that passes has passed everything CI checks. A failure names the change
  that caused it. The fuzzers search for longer than a pull request could wait.
- **Costs**: the longest job sets how long every pull request waits. Each push takes a dozen or more
  runners, toward GitHub's limit of twenty at once. The suites have to be kept cheap, so a test's
  cost is reviewed like its coverage.
- **Revisit when**: a pull request still waits more than an hour once the jobs are split, or a push
  no longer fits the runners GitHub allows at once. Revisit too if the repository moves to an
  organisation, where a merge queue could run the long checks once per merge.
