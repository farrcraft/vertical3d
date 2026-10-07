# ADR-0079: Assets: loaders are registered

**Status**: accepted
**Date**: 2026-10-04
**Amends**: [ADR-0021](0021-audio-use-sdl3-mixer.md)
**Documented in**: [api/Assets.md](../api/Assets.md)

## Context

When `asset::Manager` builds every loader itself, `v3dlib_asset` links every library a loader
builds a payload from: the mixer for a sound, FreeType for a typeface, libpng and libjpeg for an
image. Every library above it inherits that closure, so a library that only reads documents, or a
starter app that draws one quad, needs the audio mixer. A consumer should install only the
closure of what it uses ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)). The
consumers of each payload differ: `render` loads models and textures and `ui` loads typefaces, but
nothing graphical loads a sound.

## Decision

`asset::Manager` keeps only the loaders that read a document, and every other loader is registered
on it, with the extensions it answers to, by a `registerLoaders()` function in the library that
holds it. The image and glTF loaders live in a new library, `v3dlib_asset_media`, and the wav
loader lives in `api/audio`, which links the asset core. A typeface is not an asset:
`ui::paint::TextRenderer` opens the face itself, at a path the manager resolves, because the size
to rasterize at is the caller's.

## Alternatives

### Every media loader in one `asset_media` library, sound included
- **For**: one library for an app to name, and audio stays independent of asset.
- **Against**: `render` has to link it for its models, so the mixer comes back into `render`'s
  closure, and into `engine`'s and the starter's through it.
- **Rejected because**: it moves the dependency rather than removing it. Only `config` would get
  lighter.

### Each loader in the library of its payload
- **For**: no new library, and a loader sits beside the type it builds.
- **Against**: `image`, `font` and `audio` would all link asset, and `image` is a leaf that the
  offline renderer depends on because it needs nothing else.
- **Rejected because**: it costs the leaf libraries their independence to save one library. Sound
  gets this treatment because its consumers share nothing with the others'.

### Open the registry and leave every loader in `api/asset`
- **For**: the smallest change. Registration alone gives one extension table and lets an app add
  a format.
- **Against**: the link closure does not change at all.
- **Rejected because**: the closure is the problem.

## Consequences

- **Gains**:
  - The mixer leaves every closure that does not play a sound, and `render`'s closure gains
    nothing, since it already linked font and image.
  - A library that only reads documents needs only the asset core.
  - Extensions resolve to formats in one table, built from the registrations.
  - An app with a format of its own registers a loader for it instead of loading around the
    manager.
- **Costs**:
  - A manager loads only what was registered on it. A manager built by hand, in a test or a tool,
    has to call `registerLoaders()`, and forgetting is a failed load at run time, not a link
    error.
  - `api/audio` links `api/asset`, the reverse of the edge ADR-0021 described. It still runs one
    way, so there is no cycle.
  - One more library for a consumer to name.
- **Revisit when**: another payload's consumers turn out to be disjoint from the rest. The test is
  what its loader adds to `render`'s closure.
