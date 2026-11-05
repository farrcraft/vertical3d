# Releasing

This document is for the maintainer who makes a release, and for a contributor whose change has
to reach one. It says what a release is, how its version and notes are kept, and how one is made.

- [What a release is](#what-a-release-is)
- [The version](#the-version)
- [The changelog](#the-changelog)
- [Making a release](#making-a-release)
- [A fix to a release](#a-fix-to-a-release)
- [What the workflow does](#what-the-workflow-does)

## What a release is

A release is one version of every app: the editor, moya, imagetool and the five demos. They are
built from one commit, and ship together.

- **Windows** gets an installer, and the install tree as a zip.
  [Build.md](Build.md#the-installer) describes both. They are unsigned until SignPath
  Foundation accepts the project, and then signed through it.
- **Linux** gets a Flatpak, which is still to come.
- **macOS** is not a release platform.
- **The api is not shipped as a binary.** A release's tag gives a project that uses the api a
  named commit to pin, as [UsingTheApi.md](../api/UsingTheApi.md#keeping-up-with-the-tree)
  says.

Background: [ADR-0165](../adr/project/0165-releases-the-apps-ship-as-one-versioned-product.md),
[ADR-0166](../adr/project/0166-releases-a-windows-installer-and-a-flatpak-on-linux.md),
[ADR-0180](../adr/project/0180-releases-signpath-foundation-signs-the-windows-release.md)

## The version

A release's version is a semantic version. The root's `project()` holds the number of the release
the tree builds toward, and [api/engine/README.md](../api/engine/README.md#the-version) says how
it reaches the apps.

- **While the major version is 0, a minor release may break the api, and a patch release never
  does.** 1.0.0 is a later decision that the api is stable.
- **A release candidate** adds `-rc.N` to the number it is a candidate for, such as `0.1.0-rc.1`.
- **After a release, the next change moves the number** to the release it builds toward: the
  minor number for new features or a break, the patch number for fixes alone.

Background: [ADR-0175](../adr/project/0175-releases-semantic-versions-from-0-1-0.md)

## The changelog

[CHANGELOG.md](../../CHANGELOG.md) is written by hand, in the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) form.

- **A change that a user of the apps, or a project that uses the api, would notice adds its
  entry under Unreleased**, in the same pull request as the code. The entry says what changed for
  that reader, not how the code changed.
- **A break in the api says what to change**, so a project that pins the api can update.
- **`scripts/changelog.ts` fails a pull request** that changes what a release ships and adds no
  entry. What a release ships is the sources under `api/`, the apps, the demos and the shared
  data, but not their tests or READMEs. [Gates.md](Gates.md) lists it.
- The changelog describes change, so the writing rules' ban on history does not apply to it.

Background: [ADR-0177](../adr/project/0177-releases-a-hand-written-changelog.md)

## Making a release

A release is made from `main`, after the pull request that holds its last change has merged.

1. On a branch, rename the changelog's `## [Unreleased]` heading to `## [X.Y.Z] - <date>`, and
   add an empty `## [Unreleased]` above it. Check that `project()` in the root `CMakeLists.txt`
   names `X.Y.Z`. Merge the branch.
2. For a candidate, push the tag `vX.Y.Z-rc.N` on that commit of `main`. The workflow publishes it
   as a pre-release.
3. When a candidate is good, or with no candidate, push the tag `vX.Y.Z` on the commit to release.
4. On the next branch, move `project()`'s number to the next release.

A tag starts the workflow, and only the maintainer pushes one. A published release cannot be
changed: a mistake is fixed by the next patch release.

Background: [ADR-0178](../adr/project/0178-releases-a-tag-on-main-starts-a-release.md)

## A fix to a release

A fix lands on `main` first, through a pull request, as any change does.

- **When `main` holds nothing that should not ship yet**, the next patch release is tagged on
  `main`.
- **When it does**, a branch `release/X.Y` is cut from the release's tag, and the fix is
  cherry-picked onto it from `main`. Its changelog section and its patch number are set on that
  branch, and the patch release is tagged there. The changelog on `main` gets the same section.

## What the workflow does

[ctest.yml](../../.github/workflows/ctest.yml) runs on a pushed `v*` tag as it runs on a pull
request, with these steps added:

1. It checks that the tag is `vX.Y.Z` or `vX.Y.Z-rc.N`, that `X.Y.Z` is the number `project()`
   holds, and that the changelog has a section for it.
2. It configures both builds with `V3D_RELEASE=ON`, and a candidate's `V3D_PRERELEASE`, so the
   apps show the release's version.
3. The Release build installs, runs every app from the install, and zips the install tree as
   `vertical3d-<version>-windows-x64.zip`. It builds the installer,
   `vertical3d-<version>-windows-x64-setup.exe`, and checks it installs and uninstalls.
4. Once every job has passed, the release job attests both files, and publishes a GitHub release
   from the tag. Its notes are the changelog's section, and a candidate is marked as a
   pre-release.
