# data

Assets that more than one app uses. The build copies them into the `data/` directory beside each
app that asks for them, together with the app's own assets from `<app>/data`.

- `fonts/` holds the typefaces the ui draws with. Each font's licence is the `-OFL.txt` file
  beside it.
- `ui/game.json` is the theme the game shell in [api/shell](../api/shell/) draws with.

[docs/contributing/Build.md](../docs/contributing/Build.md#data-directories) covers how data
directories are copied.
