# ADR-0165: Releases: the apps ship as one versioned product

**Status**: accepted
**Date**: 2026-11-03
**Tags**: releases
**Amends**: [ADR-0027](0027-build-consume-the-api-as-source.md)
**Documented in**: [Releasing.md](../../contributing/Releasing.md#what-a-release-is)

## Context

The tree holds an editor, a renderer, an image tool and five demos, built only from source. Users
of the apps need a build they can install without a compiler, vcpkg or the Vulkan SDK. Other
repositories consume the api libraries as source, and pin them by a submodule commit. The api
still changes often, and a binary of it would tie a consumer to the release's vcpkg baseline and
runtime library.

## Decision

A release ships the editor, moya, imagetool and the demos together, under one version number for
the whole repository. The api libraries are not shipped as a binary. A release's tag gives a
consumer of the api a named commit to pin, which amends the rule that the tree has no version.

## Alternatives

### The apps and a binary SDK of the api
- **For**: a consumer links installed libraries through `find_package`, with no source build.
- **Against**: the consumer must match the release's vcpkg baseline, triplet and runtime library
  exactly, and each release needs Debug and Release libraries. The api's types change between
  releases, so a binary gives no stability that a tag does not.
- **Rejected because**: the reasons ADR-0027 gives against a binary drop still hold.

### A version for each app
- **For**: an app that did not change does not get a new version.
- **Against**: the apps share the api, so one api change can alter all of them. A user cannot
  tell which versions of the editor and moya work together. None of the projects compared
  versions its parts separately.
- **Rejected because**: the apps are built from one tree at one commit, and one number says so.

### The editor and moya only
- **For**: the demos' assets and the LGPL code in voxel stay out of the releases.
- **Against**: the demos are the only apps that show the game engine running, and each would
  still have to be built from source to be seen.
- **Rejected because**: the demos are part of what the tree offers, and their licence and asset
  questions have answers.

## Consequences

- **Gains**: one download holds every app at one version, and moya always matches the editor
  that runs it. A consumer of the api can pin a release instead of a bare commit.
- **Costs**: every app must install and run outside its build directory, including the demos.
  Each release answers for every app's licences and assets. A change to one app moves the
  version of all of them.
- **Revisit when**: a consumer needs the api as a binary, or one app needs releases on a cadence
  the others do not share.
