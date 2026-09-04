# ADR-0020: Themes — A Theme Is Data, And The App Resolves The Images It Names

**Date**: 2026-09-04
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`api/ui/style/` — `Theme`, `Style`, `Property` and the colour, font and image properties —
migrated out of `luxa/` complete and was never reachable: nothing constructed a `Style`
anywhere in the tree, `prop::Color` had no way to hold a colour, and `ui::Engine::load` read
a theme's name and stopped. Every colour and metric the ui is drawn with was therefore a
hardcoded default on `ComponentRenderer::Style`, and `component::Icon` and `prop::Image`
each held a texture handle nothing could set — which is why the editor's toolbars are
labelled where `gui.xml` asks for icons. Closing this is the last item on
[LuxaAudit.md](../LuxaAudit.md) and what lets `luxa/` be deleted. Two things constrain it:
`v3dlib_ui` depends on `v3dlib_render` for the canvas and on nothing else, and
[ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md) already settled that the library
knows where things go while the app knows how to draw them.

## Decision

A theme is JSON read by `ui::Engine::load` — styles holding colours, numbers, fonts and
images — and `ComponentRenderer::theme()` reads the `ui` style into the colours and metrics
it draws with, overriding only what the theme names. The library never loads or uploads an
image: `Engine::resolveImages()` hands each source to a callback the app supplies and keeps
the `TextureHandle` that comes back, the way text measuring and writing are already
callbacks.

## Alternatives Considered

### Alternative 1: `v3dlib_ui` links `v3dlib_asset` and loads its own images
- **Pros**: one call for an app instead of a callback; the audit's item says "route through
  `asset::Manager`", which this does literally.
- **Cons**: the ui would need the asset manager *and* the quad renderer to turn an image
  into a texture, so it acquires the app's whole loading stack; the library stops being
  testable without one; `api/ui/tests` would need a device to cover drawing.
- **Why not**: the same argument that kept the font library out of `ComponentRenderer`.
  A callback is the seam that already exists.

### Alternative 2: Keep the colours and metrics on the renderer and theme nothing
- **Pros**: nothing to design; the values live next to the code that draws with them and can
  be derived from the font size, as the editor derives them today.
- **Cons**: `api/ui/style/` stays dead weight that looks live, and a second app wanting a
  different look has to edit the library.
- **Why not**: it leaves the audit item open and `luxa/` undeletable.

### Alternative 3: A theme replaces the whole style, rather than overriding what it names — **chosen against**
- **Pros**: what is drawn is exactly what the document says, with no invisible defaults.
- **Cons**: every existing ui config would have to name all ten values before it drew
  correctly, and a theme could not carry one colour.
- **Why not**: a theme that names nothing has to keep drawing what it drew.

## Consequences

### Positive
- `api/ui/style/` is reachable: the four property kinds are read, `Theme::getStyleSet` and
  `Style::property` have callers, and a `style::Button` is picked by its state.
- The ui is themable from data without a rebuild, and a theme carrying one colour changes
  one thing.
- `v3dlib_ui` gains no dependency, so the schema, the image pass and the drawing are all
  covered by `api/ui/tests` with no window and no device.
- `Button`, `Label` and `Icon` are constructible from a config and drawn, which is the rest
  of what the luxa audit's item 7 asked for.
- A toolbar can be iconic, which is what `gui.xml` asks for and what the editor's left strip
  now is. A button's icon replaces its label rather than joining it, and the strip is laid
  out from the icon the button *names* rather than the texture it holds, so the layout is the
  same before and after the image pass runs.

### Negative
- Two ways to set the same value: `ComponentRenderer::style()` in code and the `ui` style in
  the document, with the document winning wherever it speaks. The editor uses both — metrics
  from the font size, colours from the theme.
- A style property is addressed by two strings, its name and its class, which is luxa's
  model kept rather than replaced. A typo in either is a property silently not found.
- The nine-slice corner is a number on the style, because a `TextureHandle` is a slot id and
  carries no size to derive one from.

### Risks
- An app that never calls `resolveImages()` draws every skinned button flat and every icon
  component not at all, silently — an unset handle is skipped rather than logged, since that
  is also the state before the pass runs. A toolbar button is the one that says something: it
  falls back to its label. The escape hatch is that the pass returns how many sources it
  resolved, and logs each one it could not.
- The schema is now a compatibility surface: `vgui.json` in four apps is read by it. Adding
  a property kind is additive; renaming one is not.
