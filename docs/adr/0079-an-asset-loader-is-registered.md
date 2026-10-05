# ADR-0079: Asset Loading — A Loader Is Registered, And A Media Loader Lives Beside What It Builds

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`asset::Manager` built all nine of its loaders in its constructor and offered no way to add one,
so `v3dlib_asset` linked every library a loader builds a payload from: audio and the mixer for a
wav, font and FreeType for a typeface, image and libpng and libjpeg for a picture. Every library
above it inherited that closure, so `config`, which only reads documents, needed the mixer, and so
did the starter, which draws a quad.
[ADR-0033](0033-a-consumer-selects-the-api-libraries-it-wants.md) exists so that a consumer installs
only the closure of what it uses, and this undid it. The same constructor kept the extension table
as a chain of `if`s that had already drifted from `image::Factory`'s, and three of its loaders were
one loader with the reader class changed. The audit is [B5, A2, T1 and T2](../audits/ApiDesignReview.md).

Not every payload is needed by the same consumers. `render` loads models and their textures through
the manager, and `ui` loads typefaces, so whatever holds those loaders is in their closure anyway.
Nothing graphical loads a sound.

## Decision

**`Manager` keeps only the loaders that read a document** — Json and Text — and a loader is
registered with the extensions it answers to, so the extension table is the registrations. **A new
library, `v3dlib_asset_media`** in `api/asset/media`, holds the image, glTF and typeface loaders and
the kinds they build, and registers them with `media::registerLoaders()`. **The wav loader and the
sound kind move to `api/audio`**, which links the asset core and registers its own with
`audio::registerLoaders()`.

`engine::Engine` registers the media loaders on the manager it builds; an app that plays sound
registers the audio ones. This reverses the direction
[ADR-0021](0021-sdl3-mixer-replaces-soloud.md) gave the edge between asset and audio — audio now
links asset, not the other way — and keeps its `Resolve` seam.

## Alternatives Considered

### Alternative 1: Every media loader in one `asset_media`, sound included
- **Pros**: one library for an app to name, and audio stays ignorant of asset as ADR-0021 had it.
- **Cons**: `render` has to link it for its models, so the mixer comes back into `render`'s closure,
  and `engine`'s and the starter's through it. Only `config` gets lighter.
- **Why not**: it moves the problem rather than removing it. It was the first answer, and finding
  what `render` loads is what changed it.

### Alternative 2: Each loader in the library of its payload
- **Pros**: no new library; a loader sits beside the type it builds.
- **Cons**: `image`, `font` and `audio` all link asset, and `image` is a leaf that `render_offline`
  depends on precisely because it needs nothing else.
- **Why not**: it costs the leaves their independence to save one library. Sound gets this
  treatment because it is the one payload whose consumers are disjoint from the rest.

### Alternative 3: Open the registry and leave every loader in `api/asset`
- **Pros**: the smallest change; the extension table and the duplicated image loaders are fixed.
- **Cons**: the closure does not change at all.
- **Why not**: the closure is the finding.

### Alternative 4: An asset core, `asset_media` for graphics, sound in `audio` — **chosen**
- **Pros**: the mixer leaves every closure that does not play a sound. `render`'s closure gains
  nothing, since it already linked font and image. `config` needs only the core.
- **Cons**: see below.

## Consequences

### Positive
- The starter, `engine` and `render` configure without SDL3_mixer, and `config` without
  FreeType, libpng or libjpeg.
- An extension resolves to a format in one table, and `.bmp` and `.jpeg` load like the rest.
- An app with a format of its own registers a loader for it instead of loading around the manager.

### Negative
- A manager loads only what was registered on it. One built by hand — a test, a tool — has to call
  `registerLoaders()` itself, and forgetting is a failed load at run time rather than a link error.
- Audio links asset, the reverse of ADR-0021's direction. It is still one way, so there is no cycle.
- One more library for a consumer to name, and media kinds change their include path and namespace
  to `api/asset/media/kind/` and `v3d::asset::media::kind`.

### Risks
- A second payload whose consumers are disjoint from the rest would want the sound treatment too;
  the test is what it adds to `render`'s closure. The escape hatch is a library's own
  `registerLoaders()`, which is all either move needs.
