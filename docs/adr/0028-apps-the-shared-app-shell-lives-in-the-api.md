# ADR-0028: Apps: the shared app shell lives in the api

**Status**: accepted
**Date**: 2026-09-05
**Documented in**: [api/engine/](../api/engine/README.md), [api/ui/](../api/ui/README.md)

## Context

Every app with a window needs the same shell around its game: a font and glyph atlas, a way to
draw text, an in-game menu that pauses the game, a `main` that logs what the renderer threw, and
a check that skips drawing while the window is minimized. None of it is game logic. When each app
writes its own copy, a fix has to be made in every copy, and the copies drift apart. Anything
shared here must keep `ComponentRenderer` free of the font library: it takes text measuring and
drawing as callbacks so that its tests run without a window
([ADR-0034](0034-ui-layout-is-resolved-while-drawing.md)).

## Decision

The shell every app repeats lives in the api: text drawing in `ui::paint::TextRenderer`, the menu
in `ui::shell::GameMenu`, `main` in `engine::run<T>`, and the minimized-window check in
`Engine3D::beginFrame`. `ui::shell::Screen` builds the ui's renderers over the `Engine3D` it is
handed and owns the canvas they draw into. An app keeps only what makes it that game.

## Alternatives

### A separate `api/app` library for the shell
- **For**: `v3dlib_ui` stays free of anything that needs a GPU, and the concept has its own name.
  A consumer that wants only the ui does not get an app framework with it.
- **Against**: it would link ui, render, engine, asset and font to hold a few classes, and every
  app already links all of them.
- **Rejected because**: no consumer would ever be on the far side of that library boundary, so it
  isolates nothing.

### Text rendering and the screen in `api/render/realtime`
- **For**: text is drawn onto a `Canvas`, and the canvas and `Engine3D` live there. `api/ui` would
  stay free of anything needing a device.
- **Against**: `v3dlib_ui` links `v3dlib_render`, so a class in render that builds ui renderers
  makes a cycle. `v3dlib_render` also links `v3dlib_font` privately so that no realtime header
  names a font type, and this would make that link public.
- **Rejected because**: the class exists to feed `ComponentRenderer`, and it cannot name
  `ComponentRenderer`'s types from the far side of that edge.

## Consequences

- **Gains**:
  - One copy of each piece of the shell, and an app's `main` is one line.
  - A missing font or a missing menu container leaves an app running without labels or without a
    menu, in every app, rather than faulting in some.
  - `GameMenu` has tests that run with no device.
  - A new app, such as [examples/starter](../../examples/starter/), starts from a working shell.
- **Costs**:
  - `TextRenderer` uploads its atlas in its constructor, so it needs a device and has no headless
    test.
  - Most of `v3dlib_ui` is device-free, but a consumer can no longer link it without a Vulkan
    device somewhere in the picture.
  - `GameMenu` expects default command and container names. An app that names them differently
    gets no menu rather than an error.
- **Revisit when**: an app needs a shell that the shared classes cannot be configured to give, or a
  consumer needs the ui without any renderer.
