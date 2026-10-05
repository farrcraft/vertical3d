# Config documents

The JSON documents an app ships with in its `data/` directory, and how the engine reads them.
These are read-only. What the player changes is saved as described in [Files.md](Files.md).

## Config documents

With `Feature::Config`, the engine reads `data/config.json`. It must use the indirect form: a
list of typed entries, each naming another file in `data/`.

```json
{
  "configs": [
    { "type": "binding", "file": "mappings.json" },
    { "type": "window",  "file": "window.json" },
    { "type": "sound",   "file": "sounds.json" },
    { "type": "map",     "file": "map.json" }
  ]
}
```

pong's [data/](../../../pong/data) directory is a complete example.

The api recognises seven types, listed in `config::Type` in
[api/config/Type.h](../../../api/config/Type.h):

| Type | What reads it | Document shape |
|---|---|---|
| `window` | The engine, when it opens the window | `{"window": {"width": 1024, "height": 768}}`. Both must be whole numbers, or startup fails. |
| `binding` | The engine's bindings | See [Bindings](Input.md#bindings). |
| `ui` | `ui::Engine::load()` | Themes and containers. See [ui/](../ui/README.md). |
| `sound` | `audio::Engine::load()` | See [Audio](Audio.md#audio). |
| `camera` | `config::CameraProfiles` | `{"cameras": [...]}`, named camera profiles. See below. |
| `layout` | The editor's viewport layout | The editor's own format. See [editor/](../../editor/README.md). |
| `sprite` | `config::SpriteSheets` | See [Sprite sheets](#sprite-sheets). |

**Any other type is an app's own document.** The config loads it like the others and files it
under the type its entry names. odyssey's board is one: its entry has type `map`, and it reads
the document with `config()->get("map")`. Inside an engine subclass, `document("map")` returns
the same document as a `boost::json::object`, or null if the config lists none.

How failures are reported:

- `config.json` missing, or an entry without a `type` or `file`, or a listed file that does not
  load, is logged and stops startup.
- The window document and the bindings check every key they read. A document they do not
  understand is a line in the log and a failed startup, not an exception.
- A config document names images and never loads them. The app resolves a theme's images and a
  sprite sheet's image through its own asset manager and renderer.

Background: [ADR-0020](../../adr/0020-ui-themes-are-data-apps-load-the-images.md)

### Camera profiles

`config::CameraProfiles` ([api/config/CameraProfiles.h](../../../api/config/CameraProfiles.h))
reads named `type::camera::Profile`s. Each entry needs a `name`. The other fields have defaults:
`orthographic` (true), `eye` (`[0, 0, -10]`), `lookat` (`[0, 0, 0]`), `up` (`[0, 1, 0]`),
`zoom` (10), `fov` (60, in degrees), `aspect` (1.33), `near` (0.1), `far` (100), and `adaptive`
(`none`, `projection`, `position` or `both`). A profile is described by where the camera is and
what it looks at, because `Profile::lookat()` is the one call that sets the basis and the
rotation together. The editor's `data/cameras.json` is the example in the tree. Cameras are
covered in [Types.md](../Types.md#cameras).

### Sprite sheets

`config::SpriteSheets` ([api/config/SpriteSheets.h](../../../api/config/SpriteSheets.h)) reads a
table of named rectangles in an image:

```json
{
  "sheets": [
    {
      "name": "units", "image": "units.png", "width": 256, "height": 256,
      "sprites": [
        { "name": "knight", "x": 0, "y": 0, "width": 32, "height": 32 }
      ]
    }
  ]
}
```

- Rectangles are in pixels, and each sheet states its own size. `SpriteSheet::uv()` divides one
  by the other to give the uv pair a canvas takes.
- A sheet with no name, image or size is rejected and the others are kept. So is a sprite whose
  rectangle runs off the sheet.
- `get()` returns an empty sheet or region for a name it does not hold, and `uv()` returns
  false. A missing sprite draws nothing and logs nothing, so check `has()` if it matters.
- **This is the one config document the api also writes.** A sprite sheet is usually made by a
  packing tool. Build sheets with `SpriteSheet(name, image, width, height)` and `place()`, add
  them with `SpriteSheets::add()`, and write `document()` with
  [`asset::writeDocument()`](Files.md#writing-files). `add()` replaces a sheet of the same name. To keep
  the sheets you did not repack, `load()` the existing document first. Sheets and sprites are
  written in the order they were added, so a repack produces a readable diff.

A ui image can name one region of a sheet. [ui/](../ui/README.md) covers that.
