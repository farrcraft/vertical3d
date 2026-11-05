# ADR-0178: Releases: a tag on main starts a release

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases
**Documented in**: [Releases.md](../../roadmap/Releases.md), until a release process document states the rule

## Context

A release has to be built from a commit that CI has already checked, by a workflow rather than
by hand. Work happens on one branch at a time, which merges into `main` through a pull request.
A fix sometimes has to reach users of a release after `main` has moved on to changes that are
not ready.

## Decision

Pushing a tag `vX.Y.Z` on `main` starts the release workflow, and `vX.Y.Z-rc.N` starts a release
candidate first. A branch `release/X.Y` is cut from a release's tag only when a fix has to ship
after `main` has moved on. The fix lands on `main` first, and is cherry-picked onto the branch.

## Alternatives

### A release branch for every minor release
- **For**: each minor release has a place for its patches from the start, as Godot's stable
  branches and Blender's release branches do.
- **Against**: a branch that no fix ever needs is still created, and every fix has to be landed
  twice whether a patch release follows or not. Both projects have many maintainers and support
  several releases at once.
- **Rejected because**: with one working branch and one supported release, a branch made before
  it is needed is only upkeep.

### A release pull request that a tool keeps open
- **For**: release-please and similar tools keep a pull request that bumps the version and the
  changelog, and merging it tags the release.
- **Against**: the tools derive the next version from Conventional Commits, which this
  repository does not use, and the changelog is written by hand.
- **Rejected because**: a pushed tag says the same thing without a tool reading the commits.

## Consequences

- **Gains**: a release is a commit `main` already holds and CI already checked. Nothing is
  branched until a fix needs it, and a fix is never lost from `main`.
- **Costs**: a release cannot leave out a change that is already on `main`. A fix that does not
  apply to an older release has to be written again on its branch.
- **Revisit when**: more than one release is supported at a time.
