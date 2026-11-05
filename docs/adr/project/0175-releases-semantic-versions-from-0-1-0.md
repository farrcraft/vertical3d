# ADR-0175: Releases: semantic versions from 0.1.0

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases
**Documented in**: [Releases.md](../../roadmap/Releases.md), until a release process document states the rule

## Context

A release carries one version for every app and for the api it is built from. The number has to
tell a user and a project that pins the api what a new release may break. It is also read as
numbers by CMake's `project()`, by Windows version resources and by AppStream metadata. The api
still changes in every piece of work, and no part of it is held stable yet.

## Decision

A release's version is a semantic version, and the first release is 0.1.0. While the major
version is 0, a minor release may break the api and a patch release never does. 1.0.0 is a later
decision that the api is stable.

## Alternatives

### Semantic versions from 1.0.0
- **For**: a major version that moves on every break, from the first release.
- **Against**: the major version would move on almost every release while the api changes, and
  a 1.x number promises a stability nothing in the tree keeps yet.
- **Rejected because**: 0.x states the api's real state, and keeps 1.0.0 meaningful.

### A calendar version, year.month
- **For**: a user sees at once how recent a release is, as Magnum's year.month numbers show.
- **Against**: the number says nothing about what a release may break. O3DE, which shows a
  calendar version, keeps a semantic version beside it for that reason.
- **Rejected because**: a project that pins the api needs to know what an update breaks more
  than when it was made.

### Major.minor.patch read for an engine
- **For**: Godot's reading lets a minor release break narrow areas that few projects use.
- **Against**: the line between a narrow break and a real one is a judgement each release makes
  again, and a reader cannot see it from the number.
- **Rejected because**: under 0.x a minor release may break anything, which states the same
  freedom without a judgement.

## Consequences

- **Gains**: the version states plainly that the api has no compatibility promise yet. A patch
  release is always safe to take. Every tool that reads a version reads this one.
- **Costs**: a project that pins the api has to read the changelog for every minor release.
  Going to 1.0.0 commits the api to semantic versioning's rules from then on.
- **Revisit when**: the api is held stable across releases, which is the decision to release
  1.0.0.
