# ADR-0006: App Scope — Both Pong And Tetris Are Kept

**Date**: 2026-08-30
**Status**: accepted
**Deciders**: Joshua Farr

## Context

Tetris does not compile: it includes `api/gl/GLFontRenderer.h`, which does not exist in the
repository, uses a `fonts_` member dropped from its header, and links `v3dlib_core`, a target
that is never built. Its renderer is also the last consumer of fixed-function OpenGL —
`glBegin`, `glTranslatef`, `glPushMatrix`, `glOrtho` — none of which survives
[ADR-0001](0001-vulkan-replaces-opengl.md). Pong is already on the `api/` framework and
builds. Whether tetris is worth the port had to be settled before planning the phase that
would do it.

## Decision

Both apps are kept. Tetris stays a first-class app and is ported alongside pong, which means
its fixed-function rendering is rewritten rather than abandoned.

## Alternatives Considered

### Alternative 1: Keep both — **chosen**
- **Pros**: Tetris exercises sprite rendering from multiple textures, which pong does not,
  so it is the app that forces the atlas and the textured quad path to actually work. Two
  consumers keep the api honest about what is app-specific.
- **Cons**: A full rewrite of its renderer, plus a config migration, plus fixing the build.
- **Why not**: n/a — chosen.

### Alternative 2: Retire tetris
- **Pros**: Removes the oldest rendering code in the repository and one of three broken
  targets. Pong alone is enough to drive the port.
- **Cons**: Loses the only app whose drawing needs multiple textures. An api validated by a
  single consumer tends to grow that consumer's assumptions.
- **Why not**: The work is a rewrite of one renderer, not a rescue of a large codebase, and
  the second consumer is worth more than the rewrite costs.

## Consequences

### Positive
- The textured quad path of [ADR-0005](0005-one-batched-quad-primitive.md) gets a real
  consumer early, rather than being validated first by odyssey several phases later.
- Two apps on the api surface assumptions that one would hide.

### Negative
- A phase of work that could have been deleted instead.
- Tetris carries other debts that now have to be paid: its `data/config.json` is still the
  old inline `keys`/`menu` format that `Config::load` rejects, and its debug text block is
  broken C++ rather than merely outdated — it does pointer arithmetic on string literals.

### Risks
- Tetris cannot be made to *run* until the Vulkan frame loop exists, so the interim goal is
  only a green build. There is a temptation to keep polishing it against a renderer that
  cannot draw; the phase boundary is what guards against that.
