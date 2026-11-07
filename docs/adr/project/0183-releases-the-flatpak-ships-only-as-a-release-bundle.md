# ADR-0183: Releases: the Flatpak ships only as a release bundle

**Status**: accepted
**Date**: 2026-11-06
**Tags**: releases
**Amends**: ADR-0166
**Documented in**: [Releasing.md](../../contributing/Releasing.md#the-flatpak)

## Context

Flathub does not accept a manifest with AI-generated or AI-assisted content, and the project is
written with an AI assistant. Outside Flathub, a Flatpak reaches users as a single-file bundle,
or from a repository the project hosts and users add as a remote. A remote updates the app
through `flatpak update`, a bundle does not, and the Windows installer does not either. A
repository the project hosts needs a host whose bandwidth grows with every install, and a
remote's URL cannot move once users have added it.

## Decision

Each release attaches the Flatpak as a single-file bundle, as it attaches the Windows
installer, and that bundle is the whole of the Linux release. The project publishes no Flatpak
repository. Users add Flathub only for the runtime the apps run on.

## Alternatives

### Flathub
- **For**: the app is listed in the software centres with no setup, and Flathub builds, hosts
  and updates it.
- **Against**: its policy forbids an AI-assisted manifest and an AI-written pull request, and
  lets reviewers reject an app whose code is AI-generated.
- **Rejected because**: the project cannot meet the policy.

### A repository on the documentation site
- **For**: users get updates through `flatpak update` and their software centre, which is
  Flatpak's own preferred way to distribute an app.
- **Against**: GitHub Pages replaces the whole site on each deploy, so every deploy rebuilds
  and signs the repository. Its soft bandwidth limit is shared with the documents, and the
  project holds a signing key every user's remote depends on.
- **Rejected because**: it gives Linux updates that Windows does not have, at a cost that grows
  with every install.

### An OCI remote, with images on GitHub's container registry
- **For**: updates, with the downloads served by the registry rather than by Pages.
- **Against**: hosting Flatpak images there is experimental, and it signs with tooling of its
  own that would have to be proven with Flatpak first.
- **Rejected because**: updates are not wanted on one platform alone.

### AppImage or Snap instead of the Flatpak
- **For**: an AppImage needs no runtime and no remote. Snap's store is listed in Ubuntu's
  software centre and updates automatically.
- **Against**: an AppImage runs on the host's own libraries and graphics driver, so an older
  distribution may lack what Vulkan 1.3 needs, and it has no sandbox. Snap is mainly Ubuntu's,
  and its store's terms for AI-generated code are not published.
- **Rejected because**: the Flatpak's runtime brings a current Vulkan driver to every
  distribution, which matters more than either gain.

## Consequences

- **Gains**: Linux gets what Windows gets, a download from the release, with no host, no
  signing key and no limit on downloads.
- **Costs**: an installed Flatpak never updates itself, so a user installs each release by
  hand. The app is in no software centre. Users still add Flathub for the runtime.
- **Revisit when**: either platform's users need updates, which is a decision for both, or
  Flathub's policy admits the project.
