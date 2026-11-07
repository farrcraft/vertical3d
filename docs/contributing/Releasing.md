# Releasing

This document is for the maintainer who makes a release, and for a contributor whose change has
to reach one. It says what a release is, how its version and notes are kept, and how one is made.

- [What a release is](#what-a-release-is)
- [The version](#the-version)
- [The changelog](#the-changelog)
- [Making a release](#making-a-release)
- [A fix to a release](#a-fix-to-a-release)
- [What the workflow does](#what-the-workflow-does)
- [Signing](#signing)
- [The Flatpak](#the-flatpak)

## What a release is

A release is one version of every app: the editor, moya, imagetool and the five demos. They are
built from one commit, and ship together.

- **Windows** gets an installer, and the install tree as a zip.
  [Build.md](Build.md#the-installer) describes both. They are unsigned until SignPath
  Foundation accepts the project, and then signed through it.
- **Linux** gets a Flatpak, [described below](#the-flatpak). It is attached to the GitHub
  release, and is not published on Flathub.
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
   names `X.Y.Z`, and add a `<release>` for it to the Flatpak's metainfo. Merge the branch.
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
4. It calls [flatpak.yml](../../.github/workflows/flatpak.yml), which builds the Flatpak with
   the same release options, as `vertical3d-<version>-linux-x86_64.flatpak`.
5. Once every job has passed, the release job attests the three files, and publishes a GitHub
   release from the tag. Its notes are the changelog's section, and a candidate is marked as a
   pre-release.

## Signing

The Windows release is signed through SignPath Foundation, which signs open-source projects at
no cost and is named as the publisher. Until SignPath accepts the project, the release is
unsigned and the workflow has no signing step.

### What SignPath requires

SignPath's terms are at https://signpath.org/terms. The project already meets these:

- **Every component has an OSI licence**, provided the release is built without FMOD. A build
  with FMOD is never signed.
- **The project is released** in the form to be signed: the installer and the zip.
- **The installer has an uninstaller.**
- **Each app's ProductName is "Vertical3D"** and its version is the release's, as SignPath's
  metadata rules ask. [v3dInstall.cmake](../../cmake/v3dInstall.cmake) writes both.
- **No app sends anything over the network.**
- **[README.md](../../README.md) has a "Code signing policy" section**, under that exact heading.
  It names SignPath.io and SignPath Foundation, lists the roles and who holds each, and states
  that the apps send nothing over the network. When someone gains or loses write access, or a
  role changes hands, its table changes in the same step.
- **Each release's page links the policy.** The release job appends the link to the notes it
  takes from the changelog.
- **Every account with write access has two-factor login on GitHub.** SignPath requires it for
  everyone on the team, and on SignPath's own site as well. A new collaborator turns it on
  before they are given access.

To check two-factor login on an account, open GitHub's **Settings > Password and
authentication** while signed in to it. The "Two-factor authentication" section reads
**Enabled**, and lists at least one method, an authenticator app or a security key, with
recovery codes saved. From a terminal, `gh auth refresh -s user` gives the CLI's token the
`user` scope, and `gh api user --jq .two_factor_authentication` then prints `true`.

### Applying and setting up

1. **Apply at https://signpath.org/apply**, naming the repository and the installer the release
   publishes. Acceptance is at SignPath Foundation's discretion.
2. **Once accepted, in SignPath's web app:**
   - Add the predefined trusted build system **GitHub.com** to the organization, and link it
     to the project.
   - Make a signing policy for releases, with yourself as its approver, so each request waits
     for your approval.
   - Make an artifact configuration that signs the project's own executables (the apps' `.exe`
     files) and the installer, and nothing else. The DLLs vcpkg builds belong to other projects
     and ship unsigned. Restrict the product name to "Vertical3D".
   - Make a user with submitter rights on the project and the policy, and an API token for it.
3. **In the repository's settings on GitHub**, under Secrets and variables > Actions:
   - Add the token as the secret `SIGNPATH_API_TOKEN`.
   - Add the organization ID, the project slug and the signing policy slug as the variables
     `SIGNPATH_ORGANIZATION_ID`, `SIGNPATH_PROJECT_SLUG` and `SIGNPATH_POLICY_SLUG`.
4. Optionally, install the SignPath GitHub app on the repository. SignPath uses it only to read
   the audit log.

### The workflow's signing step

With those in place, a change to [ctest.yml](../../.github/workflows/ctest.yml) adds signing to
the Release job on a tag. It uploads the files to sign as an artifact with
`actions/upload-artifact`, and submits it with `signpath/github-action-submit-signing-request`,
waiting for the signed files.

- **The apps are signed before the installer is built**, because the installer holds copies of
  them. The job signs the executables, puts the signed copies in the install tree, builds the
  zip and the installer from it, and then signs the installer in a second request.
- **Each request waits for the approver.** A release's job sits at the signing step until the
  approver approves it on SignPath's site. The action's `wait-for-completion-timeout-in-seconds`
  is 600 by default, so the maintainer pushes a tag only when they can approve within it, or the
  step sets a longer one.

## The Flatpak

[flatpak/com.farrcraft.Vertical3D.yml](../../flatpak/com.farrcraft.Vertical3D.yml) builds every
app on the Freedesktop runtime, from source and with no network once its sources are fetched.
Its app ID is `com.farrcraft.Vertical3D`.

- **The runtime supplies the libraries it has**, where its version is one the tree accepts. The
  manifest's first comment lists them.
- **Each other dependency is a module**, pinned by URL and SHA-256 to the version vcpkg's
  baseline builds. thorvg and OpenUSD apply the patches of their overlay ports in `ports/`. When
  the baseline moves a package, its module moves with it, in the same change.
- **The runtime is the newest Freedesktop branch.** Flathub's linter warns when a newer one is out,
  and the manifest moves to it.
- **The apps are installed in `/app/lib/vertical3d`**, with `data/` beside them, and each has a
  link in `/app/bin`. [Build.md](Build.md#on-linux) gives the layout.
- **The editor is the Flatpak's command**, and the editor and each demo have a desktop entry and
  an icon. moya and imagetool run with `flatpak run --command=moya` or `--command=imagetool`.
- **The sandbox gets** a Wayland or X11 window, the GPU, PulseAudio, and the user's documents
  folder, where the editor keeps its projects.

[flatpak/com.farrcraft.Vertical3D.metainfo.xml](../../flatpak/com.farrcraft.Vertical3D.metainfo.xml)
is what software centres show. It holds a `<release>` for each version, so a release adds one in
its first step, beside the changelog's heading.

flatpak.yml builds the Flatpak on every pull request that changes more than documents, and keeps
the bundle with the run. To build it locally, run flatpak-builder in Flathub's image under Docker:

```
docker run --rm --privileged -v C:\code\vertical3d:/src:ro -v v3d-flatpak:/work ^
  ghcr.io/flathub-infra/flatpak-github-actions:freedesktop-26.08 bash -c ^
  "cd /work && flatpak-builder --disable-rofiles-fuse --state-dir=/work/state --force-clean /work/build /src/flatpak/com.farrcraft.Vertical3D.yml"
```

The `v3d-flatpak` volume keeps each module's build, so a second run rebuilds only what changed.
`flatpak build /work/build vertical3d --version` in the same image runs an app from the build.
`flatpak-builder-lint manifest` and `appstreamcli validate` check the manifest and the metainfo.

### How it ships

The bundle is the whole of the Linux release, as the installer is of the Windows one.

- **Each release attaches the bundle**, `vertical3d-<version>-linux-x86_64.flatpak`, and the
  project publishes no Flatpak repository. A user installs it from the download, and installs
  the next release the same way, because an installed bundle does not update itself.
- **The runtime comes from Flathub.** A user adds the Flathub remote once, and Flatpak takes
  `org.freedesktop.Platform` from it. [Install.md](../manual/Install.md#on-linux) gives users the
  commands.
- **The app is not on Flathub.** Flathub does not accept a manifest written with an AI
  assistant, and this one is.

Background: [ADR-0183](../adr/project/0183-releases-the-flatpak-ships-only-as-a-release-bundle.md),
[ADR-0182](../adr/project/0182-releases-the-flatpak-takes-the-runtimes-libraries-and-builds-the-rest.md)
