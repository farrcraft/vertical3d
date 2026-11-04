# ADR-0167: UI: a chosen tab is marked as a place, not a toggle

**Status**: proposed
**Date**: 2026-11-03
**Documented in**: [docs/api/ui/Components.md](../api/ui/Components.md), [docs/api/ui/Themes.md](../api/ui/Themes.md), [docs/api/ui/ImmediateMode.md](../api/ui/ImmediateMode.md)

## Context

The ui marks two different things as chosen. A tab shows which page or mode is in view, while a
toggle shows a setting that is on. Both were filled with the accent's highlight, so a tab strip,
a strip of checked tools and the active area's accent outline all read alike. An editor gains
more of both over time: overlays, axis locks and snapping on one side, tabs in any area on the
other. Blender's theme draws a chosen tab as a lighter neutral and gives the blue only to a
checked toggle or radio button, and Material and Fluent mark a tab with a line.

## Decision

The accent fill marks a checked toggle and nothing else. A chosen tab is marked as the place
shown: in a strip, the page's colour with an accent line under it; in a set of choices, raised
from a recess to the control colour. Either is set semibold.

## Alternatives

### Fill a chosen tab with the accent, as a checked toggle is
- **For**: one rule for every chosen thing, and the tools and the workspaces mark a choice alike.
- **Against**: where you are and what is switched on look the same, and the accent loses its
  meaning as toggles and tabs multiply.
- **Rejected because**: the tools this one is modelled on keep the two apart, Blender in its
  own workspace tabs among them.

### One tab look for every set of tabs
- **For**: the fewest rules: every tab is a neutral lift with an accent line.
- **Against**: it loses the difference between a few modes to pick between and a strip of pages,
  which segmented controls and tabs draw differently.
- **Rejected because**: the two read better apart, and the design draws them apart.

## Consequences

- **Gains**: the accent fill means "on" wherever it appears, and a tab never competes with the
  active area's outline or a checked tool.
- **Costs**: the theme has a `chosen` role and a tab has two looks, so a component author picks
  between strip and choice. A row of tab-look buttons needs the app to draw the recess behind it.
- **Revisit when**: a theme or an app needs a chosen tab that stands out more than a lift and a
  line give.
