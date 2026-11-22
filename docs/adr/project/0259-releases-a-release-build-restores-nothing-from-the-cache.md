# ADR-0259: Releases: a release build restores nothing from the cache

**Status**: accepted
**Date**: 2026-11-21
**Tags**: releases, ci
**Documented in**: [CI.md](../../contributing/CI.md)

## Context

A release publishes what its build produces: an install tree, an installer and a Flatpak. Any run
on main can write the Actions cache that a release's run reads, and a run that is compromised can
fill the cache to evict what is there and write entries of its own. A release that restores them
ships what that run wrote, and nothing in the release shows it. A build that reads a cache cannot
show that its output does not depend on an earlier run. A release happens a few times a year, and
a build with nothing cached fits inside a job's time limit.

## Decision

A run for a release's tag neither restores nor saves any cache. It builds every dependency from
source, and fetches every tool from its release, as a run with an empty cache does. Every other run
keeps its caches, but for a weekly run that builds as a release does.

## Alternatives

### Tools from the cache once their hashes are checked
- **For**: the SDK, the software Vulkan driver and the installer's compiler are restored in seconds,
  and a hash pinned in the workflow shows each is the release it names.
- **Against**: the time saved is a few minutes of downloads in a build of hours, and each tool needs
  a pinned hash kept up to date beside its version.
- **Rejected because**: fetching each tool from its release gives the same assurance with nothing
  read from the cache.

### Restoring the caches, as other runs do
- **For**: a release builds in under an hour, as a pull request does, from the same archives a pull
  request tested.
- **Against**: whatever can write a cache on main reaches what the release ships, and the release
  cannot show it did not.
- **Rejected because**: a release is the one build whose output leaves the repository, and saving
  its time is worth less than knowing what it was built from.

## Consequences

- **Gains**: a release depends only on the tagged tree and the releases of what it fetches. A
  poisoned cache can reach a pull request's run but not a shipped build.
- **Costs**: a release takes hours rather than under an hour. A dependency that stops building from
  source is found by the weekly run rather than by the pull request that changed it, and the
  weekly run spends hours of runner time each week.
- **Revisit when**: releases become frequent enough that their time matters, or GitHub offers a
  cache whose entries a release can verify.
