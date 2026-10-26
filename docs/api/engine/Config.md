# Config documents

The JSON documents an app ships with in its `data/` directory, and how the engine reads them.
These are read-only. What the player changes is saved as described in [Files.md](Files.md).

## Config documents

With `Feature::Config`, the engine reads `data/config.json` into an `asset::Manifest`
([api/asset/Manifest.h](../../../api/asset/Manifest.h)). The file is a list of entries, each
naming a kind and another file in `data/`.

```json
{
  "configs": [
    { "type": "binding", "file": "mappings.json" },
    { "type": "window",  "file": "window.json" },
    { "type": "sound",   "file": "sounds.json" },
    { "type": "world",   "file": "world.json" }
  ]
}
```

pong's [data/](../../../pong/data) directory is a complete example.

A kind is any name. The library that reads a kind declares its name as a constant, and the api
reads these:

| Kind | Declared by, and read by | Document shape |
|---|---|---|
| `window` | `engine::WINDOW_DOCUMENT`. The engine, when it opens the window | `{"window": {"width": 1024, "height": 768}}`. Both must be whole numbers an `int` can hold, or startup fails. |
| `binding` | `event::Bindings::DOCUMENT`. The engine's bindings | See [Bindings](Input.md#bindings). |
| `ui` | `ui::Engine::DOCUMENT`. `ui::Engine::load()`, from the shell | Themes and containers. See [ui/](../ui/README.md). |
| `sound` | `audio::Engine::DOCUMENT`. `audio::Engine::load()` | See [Audio](Audio.md#audio). |
| `camera` | `engine::CameraProfiles::DOCUMENT` | `{"cameras": [...]}`, named camera profiles. See below. |
| `layout` | The editor's `ViewLayout::DOCUMENT` | The editor's own format. See [editor/](../../editor/README.md). |
| `sprite` | `asset::media::SpriteSheets::DOCUMENT` | See [Sprite sheets](#sprite-sheets). |

**An app names kinds of its own the same way.** The manifest loads every entry alike. odyssey's
world is one: `tile::World::DOCUMENT` is `world`, and the game reads the document with
`manifest()->get(tile::World::DOCUMENT)`. Inside an engine subclass, `document(kind)` returns
the same document as a `boost::json::object`, or null if the config lists none. A misspelt kind
is a null document, not a compile error, so name a kind through its constant.

How failures are reported:

- Each of these is logged and stops startup: `config.json` missing, an entry without a `type`
  or `file`, and an entry whose `type` or `file` is not a string. An entry whose `type` is
  empty, and a listed file that does not load, are treated the same way. A failed load keeps
  nothing it read.
- The window document and the bindings check every key they read. A document they do not
  understand is a line in the log and a failed startup, not an exception.
- A config document names images and never loads them. The app resolves a theme's images and a
  sprite sheet's image through its own asset manager and renderer.

Background: [ADR-0020](../../adr/0020-ui-themes-are-data-apps-load-the-images.md),
[ADR-0119](../../adr/0119-assets-an-apps-document-list-is-an-asset-manifest.md)

A tool that does not run the engine reads the same list by building a manifest itself:

```cpp
v3d::asset::Manager assets("data", logger);
v3d::asset::Manifest manifest(logger);
if (manifest.load(assets, "config.json")) {
    auto window = manifest.get(v3d::engine::WINDOW_DOCUMENT);
}
```

### Camera profiles

`engine::CameraProfiles` ([api/engine/CameraProfiles.h](../../../api/engine/CameraProfiles.h))
reads named `type::camera::Profile`s. Each entry needs a `name`. The other fields have defaults:
`orthographic` (true), `eye` (`[0, 0, -10]`), `lookat` (`[0, 0, 0]`), `up` (`[0, 1, 0]`),
`zoom` (10), `fov` (60, in degrees), `aspect` (1.33), `near` (0.1), `far` (100), and `adaptive`
(`none`, `projection`, `position` or `both`). A `zoom` below `Camera::MINIMUM_ZOOM`, 0.01, fails
the load. A profile is described by where the camera is and
what it looks at, because `Profile::lookat()` is the one call that sets the basis and the
rotation together. The editor's `data/cameras.json` is the example in the tree. Cameras are
covered in [Types.md](../Types.md#cameras).

### Sprite sheets

`asset::media::SpriteSheets`
([api/asset/media/SpriteSheets.h](../../../api/asset/media/SpriteSheets.h)) reads a table of
named rectangles in an image. It is in `v3d::asset_media`, so a tool reads it without the engine:

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
- A sheet with no name, image or size is rejected and the others are kept. A sprite whose
  rectangle runs off the sheet is rejected too.
- A document that names a sheet more than once logs a warning, and the last sheet of that name
  is kept. It keeps the place in `names()` that the name first took.
- `get()` returns an empty sheet or region for a name it does not hold, and `uv()` returns
  false. A missing sprite draws nothing and logs nothing, so check `has()` if it matters.
- **This is the one config document the api also writes.** A sprite sheet is usually made by a
  packing tool. Build sheets with `SpriteSheet(name, image, width, height)` and `place()`, add
  them with `SpriteSheets::add()`, and write `document()` with
  [`asset::writeDocument()`](Files.md#writing-files). `add()` replaces a sheet of the same
  name. To keep the sheets you did not repack, `load()` the existing document first. Sheets and
  sprites are written in the order they were added, so a repack produces a readable diff.
  `imagetool --pack` does this, through [`image::pack()`](../Assets.md#packing-a-sprite-sheet).

A ui image can name one region of a sheet. [ui/](../ui/README.md) covers that.
