# ADR-0155: Rendering: one device presents to every window

**Status**: accepted
**Date**: 2026-11-02
**Tags**: rendering
**Documented in**: [docs/internals/realtime/Frames.md](../../internals/realtime/Frames.md#other-windows), [docs/api/rendering/FramesAndTargets.md](../../api/rendering/FramesAndTargets.md#other-windows)

## Context

The editor opens a second window to show a render, and may later move any editor into one.
Textures, glyph pages, icons and pipelines are built on a device and cannot be used on another.
A window context makes a device of its own today, picked for its window's surface. A device
loss, a resize or a minimised window has to stay one window's concern where it can. bgfx, Godot
and Dear ImGui each draw every window with one device and give each window its own swapchain.

## Decision

Every window a program draws to is drawn by one device. Each window has its own surface and
swapchain, and one frame acquires an image from each window that has one, records them all into
one command buffer, and presents them together.

## Alternatives

### A device for each window
- **For**: nothing in the renderer changes, and each window paces and fails on its own.
- **Against**: every texture, glyph page and pipeline is built once for each window, and a
  picture made for one window reaches another only through the CPU.
- **Rejected because**: a window that shows what another draws needs the same resources.

### One device, a submission for each window
- **For**: each window is acquired, submitted and presented on its own, so a slow present holds
  back only its window.
- **Against**: each window needs frames in flight of its own, and every buffer kept for each frame
  in flight would be shared by two rings that advance apart.
- **Rejected because**: one frame of the renderer's buffers serves every window only when every
  window is drawn in the same frame.

## Consequences

- **Gains**: anything uploaded once can be drawn in any window, and a window costs a surface, a
  swapchain and its images.
- **Costs**: a frame waits for every window's image, so the slowest window paces them all. The
  device is picked for the first window's surface, and a later window whose surface it cannot
  present to is refused. A device loss ends every window.
- **Revisit when**: windows need different frame rates, such as a monitor that refreshes much
  more slowly than another.
