# ADR-0130: Shell: games share one designed shell with their own accent

**Status**: accepted
**Date**: 2026-10-30
**Documented in**: [api/ui/ShellComponents.md](../api/ui/ShellComponents.md)

## Context

Every game takes its `main`, its game menu and its text renderer from the api's shell. No game
names a style in its theme, so every game draws the library's defaults, which were chosen for the
editor. Each game picks its own text size, and the sizes in the tree run from 18 to 48 pixels. A
game shows a pause menu, settings, statistics and HUD text. A new game should look finished
before anyone designs its ui.

## Decision

The api ships a game theme, and the shell's screens are drawn with it. They have a dimmed
backdrop, a titled menu, settings rows that show their values, text sized from the window's
height, and a safe area. A game names its title and its accent colour, and may override any
role, while the editor keeps a theme of its own.

## Alternatives

### A designed ui for each game
- **For**: each game has its own palette, title treatment, HUD layout and button art.
- **Against**: every game's theme has to change with every change to the shell, and the games'
  menus drift apart.
- **Rejected because**: the shell belongs to the api, and a title, an accent and the game's own
  art give a game enough of its own look.

### Fix the defects only
- **For**: the least change to how the games look.
- **Against**: the games keep the editor's dense look, with no backdrop, no title and no values
  in their settings.
- **Rejected because**: the shell is the first thing every game shows.

## Consequences

- **Gains**:
  - Every game, including the next, opens with a finished menu.
  - A fix to the shell's look reaches every game at once.
- **Costs**:
  - The games' menus look alike apart from their colour and title.
  - A game that wants a different menu writes overrides against a theme it does not own.
- **Revisit when**: a game needs a menu laid out differently, not only coloured differently.
