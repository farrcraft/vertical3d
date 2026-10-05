# ADR-0074: An App's Text And Ui Renderers — One Shell Class Builds Them Over The Renderer It Is Handed

**Date**: 2026-10-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

pong, tetris, voxel and the editor each build the same set, in their own order. Each makes a
`TextRenderer` with an upload lambda over `Engine3D::quads()`, a `ComponentRenderer` over its
measure and write, and usually a `StatisticsOverlay`. Each owns the canvas they draw into, and
checks every frame whether the window's size has moved it. cozy writes the same set, and also
rebuilds the renderers that close over a font size whenever its text scale changes.
[ADR-0028](0028-apps-the-shared-app-shell-lives-in-the-api.md) says what every game writes the same way is
the api's. The roadmap asked for a helper "on the realtime side", which the library edges rule
out: `v3dlib_ui` links `v3dlib_render`, so a helper in `api/render` that built a `TextRenderer`
would be a cycle.

## Decision

**`ui::shell::Screen` builds the text renderer, the component renderer, and optionally the
statistics overlay and an immediate layer, over the `Engine3D` it is handed. It owns the canvas
they draw into, and begins each frame.** It lives in `api/ui/shell` and declares `Engine3D`
rather than including it, so no `api/ui` header names a Vulkan type. The passes, the draw order
and what is submitted where stay the app's. The size, the line height and the rest of the
dressing are handed in.

## Alternatives Considered

### Alternative 1: A shell class in `api/ui`, over a declared `Engine3D` — **chosen**
- **Pros**: `v3dlib_ui` already links `v3dlib_render`, so no library gains an edge. The upload
  lambda is written once, in a `.cpp`. A rescale rebuilds what closes over the size in one place,
  rather than in each game.
- **Cons**: A consumer that draws through its own frame model and not `Engine3D`, as retcon does,
  gets none of it.
- **Why not**: n/a — chosen.

### Alternative 2: In `api/render`, beside `Engine3D`
- **Pros**: It sits beside the renderer it begins frames on, which is what the roadmap asked for.
- **Cons**: It would link `v3dlib_ui`, which links `v3dlib_render`.
- **Why not**: The cycle.

### Alternative 3: A library of its own above both, `api/shell`
- **Pros**: Neither library learns about the other, and a ui with no renderer stays exactly that.
- **Cons**: It has no dependency `v3dlib_ui` lacks, and a consumer selecting libraries
  ([ADR-0033](0033-build-select-api-libraries-through-a-manifest.md)) would select one more for
  one class.
- **Why not**: A library for one class, guarding against an edge that already exists.

### Alternative 4: A `TextRenderer` constructor that builds its own upload
- **Pros**: The smallest change, and the four apps would lose one statement each.
- **Cons**: It puts a Vulkan type back in an `api/ui` header, which
  [EmbeddingSeams](../plans/completed/EmbeddingSeams.md) removed. It also does nothing about the
  rest of the set or the resize.
- **Why not**: It undoes a seam to save one line.

### Alternative 5: On `engine::Engine`
- **Pros**: Every app subclasses it, so the set would be built with no call at all.
- **Cons**: It would take `v3dlib_ui` into the base class of odyssey and the starter, which draw
  no text, and the base class would then own a renderer it does not otherwise know about.
- **Why not**: The loop is not the renderer, and not every app has a ui.

## Consequences

### Positive
- Four apps stop writing the set, and the resize, and cozy's rescale, by hand.
- The editor begins its frames the way the others do, so a minimised window draws nothing.

### Negative
- The dressing differs between apps, so it is handed in as a function of the size, which is one
  more thing to learn than a list of fields.
- What the screen builds is rebuilt on a rescale. A caller that kept a reference to the old
  component renderer holds one that is no longer drawn with, so the screen hands out references
  to be used and not kept.

### Risks
- **An app that needs a part of the set the screen does not build** writes it beside the screen,
  as voxel's terrain is beside its overlay today. If two apps write the same thing beside it, that
  thing is the screen's.
