# ADR-0177: Releases: a hand-written changelog

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases
**Documented in**: [Releases.md](../../roadmap/Releases.md), until a release process document states the rule

## Context

A release's notes tell a user what changed, and tell a project that pins the api what it has to
change. Under 0.x versions a minor release may break the api, so the notes are where a break is
explained. Commits here are written as prose sentences, and a pull request merges one long
working branch that holds many pieces of work.

## Decision

The repository keeps a hand-written `CHANGELOG.md` in the Keep a Changelog form. A change that a
user or a project using the api would notice adds its entry under Unreleased, in the same change
as the code. A release renames that section to its version, and the section becomes the
release's notes.

## Alternatives

### Generated from commit messages
- **For**: git-cliff builds the changelog from the history, and nobody has to remember an entry.
- **Against**: it reads Conventional Commits prefixes, so every commit message would change form.
  It turns each commit into a line, which is the history rather than what changed for a user,
  and a breaking change still needs its explanation written by hand.
- **Rejected because**: the notes are for users, and commit messages are written for
  contributors.

### GitHub's generated release notes
- **For**: no file to keep. GitHub lists the merged pull requests and groups them by label.
- **Against**: one pull request here merges a whole working branch, so a release would be a
  line or two naming branches. Nothing would explain a break.
- **Rejected because**: the pull requests here are too coarse to describe a release.

## Consequences

- **Gains**: the notes say what changed for a user, in their terms, and explain each break where
  it lands. The changelog is in the tree, so a change and its entry are reviewed together.
- **Costs**: every user-facing change has to remember its entry, which a gate checks. Writing
  the entries is work a generator would not ask for.
- **Revisit when**: pull requests become small and one per change, which would make generated
  notes describe a release well.
