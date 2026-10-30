# ADR-0131: UI: tools scale with the monitor, games with the window

**Status**: accepted
**Date**: 2026-10-30
**Documented in**: [api/ui/Setup.md](../api/ui/Setup.md)

## Context

On Windows a window and its events are measured in device pixels, and SDL reports the monitor's
scale separately. A ui drawn at fixed pixel sizes is therefore small on a monitor set to 150% or
200%. A tool is read at a desk, and its text should be the same physical size on any monitor. A
game's menu should fill the same share of its window whatever size the window is. Lines one pixel
wide and small text blur when a whole canvas is stretched by a factor that is not whole, and
players and users also ask for larger text than either rule gives.

## Decision

A screen takes a scaling rule: a tool's ui follows the monitor's scale, and a game's follows its
window's height against a reference height of 1080, and a user preference multiplies either one.
The text size and every dressing size are multiplied by the result, and the ui is redrawn at it.
An outline's width is rounded down to whole device pixels and is never less than one.

## Alternatives

### The monitor's scale for every app
- **For**: one rule, and the one Blender and Dear ImGui follow.
- **Against**: a game in a large window, or full screen on a large monitor, shows a menu sized
  for a small one.
- **Rejected because**: a game's menu has to fill its window.

### A transform on the whole canvas
- **For**: the ui is laid out at one size and stretched, as Unity's canvas scaler and Godot's
  canvas items stretch do, so nothing is rebuilt when the scale changes.
- **Against**: lines and text blur at fractional scales such as 125% and 150%, and every cursor
  point, clipping rectangle and app drawing on the canvas has to be converted between two
  coordinate spaces.
- **Rejected because**: the ui has to be sharp at the scales Windows laptops use, and one
  coordinate space keeps hit testing as it is.

## Consequences

- **Gains**:
  - The editor's text is the same physical size on any monitor, and follows the window when it
    moves to another one.
  - A game's menu fills the same share of its window at any size.
  - Text and lines are drawn sharp at any scale, and the cursor stays in device pixels.
- **Costs**:
  - A change of scale rebuilds the text callbacks and the dressings, and resolves the theme
    again.
  - A theme's sizes are logical pixels that the screen multiplies, so a theme cannot name an
    exact device pixel size other than an outline's.
  - A game's ui at a reference height of 1080 is smaller than a tool's in a small window.
- **Revisit when**: an app draws one ui into several windows on monitors of different scales.
