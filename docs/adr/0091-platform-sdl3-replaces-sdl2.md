# ADR-0091: Platform: SDL3 replaces SDL2

**Status**: accepted
**Date**: 2026-05-09
**Documented in**: [docs/contributing/Dependencies.md](../contributing/Dependencies.md)

## Context

The engine needs a window, a Vulkan surface, keyboard, mouse and text input, a user settings
directory and an event loop, on Windows first and other platforms later. SDL2 provides all of
them, but it receives only fixes, and new platform and API work goes into SDL3. The renderer is
moving from OpenGL to Vulkan, which changes the window and context layer whichever library holds
it. Several SDL2 calls return error codes and sizes in forms the engine has to translate.

## Decision

The api uses SDL3 for its windows, input, events and platform paths, and no SDL2 remains. SDL3
creates the Vulkan surface and loads the Vulkan library, built with its `vulkan` feature.

## Alternatives

### Stay on SDL2
- **For**: no port, and SDL2 can create a Vulkan surface too
- **Against**: it receives only fixes, so every later platform change lands in a library the
  tree does not use. Its APIs report failure as negative codes, have no event for a change of
  pixel size, and leave a path's encoding to the platform
- **Rejected because**: the window layer was being rewritten for Vulkan anyway, and moving then
  costs the least

### GLFW
- **For**: a small library made for a window and a Vulkan surface
- **Against**: it has no audio, no user settings path, and less input than SDL. The tree would
  take a second library for each
- **Rejected because**: SDL covers everything the engine needs in one dependency

## Consequences

- **Gains**: a maintained line. Calls that succeed or fail return a bool. A window reports a
  change of pixel size as its own event. SDL hands an app its arguments, its base path and its
  settings path in UTF-8
- **Costs**: every SDL call site was ported, and the event and key names changed. vcpkg's sdl3
  has to be built with its `vulkan` feature, or the Vulkan library fails to load at startup. The
  audio backend of the time had no SDL3 support and was replaced, by ADR-0021
- **Revisit when**: SDL3 stops being maintained, or a platform the tree targets is one SDL3
  does not support
