# ADR-0180: Releases: SignPath Foundation signs the Windows release

**Status**: accepted
**Date**: 2026-11-05
**Tags**: releases
**Documented in**: [Releasing.md](../../contributing/Releasing.md#what-a-release-is)

## Context

Windows warns before it runs an unsigned installer downloaded from the web, and many users stop
there. A signing certificate costs money every year, or depends on where its holder lives. Every
component of a release built without FMOD has an OSI licence. Signing does not remove
SmartScreen's warning at once: a file's reputation builds over weeks whatever signed it.

## Decision

The Windows installer and the apps are signed through SignPath Foundation, which signs
open-source projects at no cost. The release workflow sends each build to SignPath, and a
maintainer approves each signing request. Until SignPath accepts the project, releases ship
unsigned.

## Alternatives

### Azure Artifact Signing
- **For**: Microsoft's own service, about $10 a month, signing from the workflow.
- **Against**: it offers certificates to individuals only in the US and Canada, and limits
  onboarding while it is in preview.
- **Rejected because**: the project's maintainer cannot count on being eligible.

### A certificate from a commercial authority
- **For**: the project is the named publisher, with no third party's terms.
- **Against**: it costs $150 to $300 a year, and its key has to be kept safe and handed to CI.
- **Rejected because**: it costs every year for the same warning-free result a free service
  gives.

### Unsigned releases
- **For**: no cost, no application and no approval step.
- **Against**: Windows warns on every download, and a user cannot tell a release from a
  tampered copy.
- **Rejected because**: the first impression of every Windows release would be a warning.

## Consequences

- **Gains**: signed releases at no cost, with the signing key held by SignPath rather than in CI.
- **Costs**: SignPath Foundation, not the project, is named as the publisher. Every member with
  release rights needs two-factor login. A build with FMOD can never be signed this way. Releases
  stay unsigned until the application is accepted, which may need a release first.
- **Revisit when**: SignPath's terms change, the release ships proprietary code, or the project
  gets a certificate of its own.
