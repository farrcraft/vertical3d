# ADR-0028: App Shell — What Every App Repeats Belongs To The api, Not To Each App

**Date**: 2026-09-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

pong, tetris, voxel and odyssey were each ported to the current engine one at a time, by
copying the app that was already working. Four ports later the shell around the game is
duplicated rather than shared: `loadFont` is byte-identical in tetris and voxel apart from
the class name and near enough in pong and the editor, `drawText` is identical in all four,
`main` differs only in a type name and a string, and the show-the-menu-and-pause-the-game
logic exists three times — twice inside pong's own event handler.

None of it is game logic. A glyph atlas, a `main` that logs what a renderer threw, and a
menu the escape key puts up are the same in every app that has a window, and each copy is a
place a fix has to be made four times. The engine's own `Feature` mask, `Engine::quit()` and
`ComponentRenderer` were already shared; the seam simply stopped short of the app.

The constraint is [ADR-0019](0019-the-ui-is-laid-out-by-what-draws-it.md): `ComponentRenderer`
takes text measuring and writing as callbacks rather than depending on the font library, so
that its suite runs with no window. Anything shared here has to leave that seam intact.

## Decision

The shell an app repeats moves into the api: `v3d::ui::TextRenderer` owns the font, the
atlas and the glyph drawing; `v3d::ui::GameMenu` owns the menu a game puts up over itself;
`v3d::engine::run<T>` and `v3d::engine::appPath` are an app's `main`; and
`Engine3D::beginFrame` is the minimized-window rule every renderer opens with. An app keeps
what makes it that game and nothing else.

## Alternatives Considered

### Alternative 1: Shared classes in the api, and the ui library is their home — **chosen**
- **Pros**: One copy of each. `v3dlib_ui` already links render, asset and font, so
  `TextRenderer` and `GameMenu` need no new dependency edge, and `GameMenu` is testable with
  no device, which the code it replaces never was. A fifth app — and
  [docs/examples/starter](../examples/starter/) is one — starts from a working shell.
- **Cons**: `v3dlib_ui` grows a class that needs a GPU, in a library whose suite is otherwise
  device-free. `GameMenu` hard-codes three command names and two component names, which is
  app-facing policy sitting in a library.
- **Why not**: n/a — chosen. The names are defaults a constructor argument overrides, and
  `TextRenderer` forward declares the `QuadRenderer` it uploads through, so no ui header
  names Vulkan.

### Alternative 2: A separate `api/app` library for the shell
- **Pros**: Keeps `v3dlib_ui` device-free and names the concept outright. A consumer wanting
  only the ui does not get an app framework with it.
- **Cons**: It would link ui, render, engine, asset and font to hold four classes, and every
  app already links all five. A library boundary that no consumer can be on the far side of
  is a directory, not a boundary.
- **Why not**: The dependency it would isolate is one every app already has.

### Alternative 3: `TextRenderer` in `api/render/realtime` rather than `api/ui`
- **Pros**: Text is drawn onto a `Canvas`, and that is where the canvas lives. `api/ui` stays
  free of anything needing a device.
- **Cons**: `v3dlib_render` links `v3dlib_font` PRIVATE precisely because no realtime header
  names a font type, and this would make it PUBLIC. The class exists to supply
  `ComponentRenderer`'s two callbacks and could not name their types from there without
  render depending on ui.
- **Why not**: It puts the class on the far side of the boundary from the thing it feeds.

### Alternative 4: Leave it duplicated
- **Pros**: No api surface added, and each app stays free to diverge.
- **Cons**: The copies had already diverged in the ways that are bugs rather than choices —
  pong, tetris and voxel dereferenced a font that failed to load where the editor guarded it,
  and pong dereferenced a menu container it never checked for.
- **Why not**: Divergence between four copies of the same paragraph is not freedom.

## Consequences

### Positive
- The apps and the editor lose 827 lines and gain 111. The api gains 601, and 239 of tests
  against code that had none. Every renderer's font handling is one construction, and every
  `main` is one line.
- The guarded paths win: a missing font or a missing menu container now leaves an app running
  without labels or without a menu, in all four, rather than faulting in three.
- `GameMenu` has a suite. The submenu rule — going up out of one leaves the menu open, and only
  closing the top level resumes the game — was asserted nowhere and is now asserted six ways.
- `Window::focused()` removes the tree's last `window_->sdl()` reach-through, and no app
  includes an SDL header but the `SDL_main.h` a `main` is entered through - closing the
  "factor SDL out of the apps" item in [TODO.md](../TODO.md).

### Negative
- `TextRenderer` cannot be tested: its constructor uploads an atlas, so it needs a device, and
  it joins the list in `CLAUDE.md` of what waits on [ADR-0007](0007-ci-rendering-tests.md).
- An app wanting to draw text in two fonts now holds two `TextRenderer`s where it used to hold
  two markups against one atlas. Nothing in the tree does.
- `v3dlib_ui` is no longer a library a consumer can link without a Vulkan device somewhere in
  the picture, even though most of it still is.

### Risks
- `GameMenu` reads "menuPrevious", "menuNext" and "selectMenu" from an app's bindings and
  "game-menu"/"main-menu" from its ui config. An app that names them differently gets silence,
  not an error. The escape hatch is the constructor's two name arguments; the command names
  are not yet overridable, and would need to be if an app ever disagrees.
- The `Suspend` callback runs while the menu is toggled, so an app that tears something down
  inside it can invalidate what the toggle is holding. The three callers only set a flag and
  move the cursor.
