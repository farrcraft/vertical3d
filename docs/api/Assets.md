# Assets, images, fonts and files

This page is for someone writing an app against the api. It covers loading files through the
asset manager, the loaders for pictures, models and sound, JSON documents, writing files
safely, upgrading old documents, `api/image`, `api/font` and glTF models. Terms are defined in
the [glossary](README.md#glossary).

- [The asset manager](#the-asset-manager)
- [Loaders](#loaders)
- [JSON documents](#json-documents)
- [Writing documents](#writing-documents)
- [Reading old documents forward](#reading-old-documents-forward)
- [Images](#images)
- [Texture atlas](#texture-atlas)
- [Fonts](#fonts)
- [Models and glTF](#models-and-gltf)

## The asset manager

`v3d::asset::Manager` ([api/asset/Manager.h](../../api/asset/Manager.h)) loads files from one
directory. The game engine builds one rooted at `<app path>/data/` and returns it from
`Engine::assets()`. Every relative name an app loads resolves there, not against the working
directory.

```cpp
auto doc = assets()->load<v3d::asset::kind::Json>("level.json");
auto png = assets()->load<v3d::asset::media::kind::Image>("units.png");
auto model = assets()->load<v3d::asset::media::kind::Model>("knight.glb");
```

- `load<T>(name)` picks the loader from the file's extension, which is matched in lower case,
  and returns the asset as `T`.
- `load<T>(name, type)` names the `asset::Type` instead of using the extension.
- `path(name)` returns the full path a name resolves to, for code that opens the file itself.
- **Every failure gives a null pointer and a line in the log.** A missing file, a file that does
  not decode, an extension nothing is registered for, and a file of a different kind than `T`
  all fail the same way. Check the one pointer.
- The manager does not cache. Each call reads the file again, so keep what you load.

To load from somewhere else, build a second manager on that directory, for example on
`engine::userPath()` for a saved game. Register the loaders it needs on it first.

## Loaders

A manager loads only file types that have a loader registered on it. Each loader is registered
for one `asset::Type` and a list of extensions.

| Registered by | Extensions | Returns |
|---|---|---|
| Every `Manager`, in its constructor | `.json` | `asset::kind::Json` |
| | `.txt` | `asset::kind::Text` |
| `asset::media::registerLoaders()` (`v3d::asset_media`) | `.png`, `.jpg`, `.jpeg`, `.tga`, `.bmp` | `asset::media::kind::Image` |
| | `.gltf`, `.glb` | `asset::media::kind::Model` |
| `audio::registerLoaders()` (`v3d::audio`) | `.wav` | `audio::kind::Sound` |

- **The game engine registers the media loaders on the manager it builds.** It does not register
  the audio loader; an app that plays sound calls `audio::registerLoaders()` itself. See
  [engine/Audio.md](engine/Audio.md#audio).
- **A manager you build yourself**, in a test or a tool, has only the JSON and text loaders.
  Call `registerLoaders()` for the others. Forgetting is a failed load at run time, not a link
  error.
- **A typeface is not an asset.** The size a face is rasterized at belongs to the caller, so
  `ui::paint::TextRenderer` opens the face itself at the path `Manager::path()` gives.

To add a format, derive from `asset::Loader` ([api/asset/Loader.h](../../api/asset/Loader.h)),
implement `load(name)`, and register it:

```cpp
manager.registerLoader(boost::make_shared<MyLoader>(logger), {".map"});
```

- Extensions include the dot and are lower case.
- Registering a type again replaces its loader.
- **`load()` returns null and logs on failure. It never throws.**
- **A loader holds no state between calls.** One instance serves everything that loads through
  the manager, so a setting left on it by one caller would be seen by the next.

Splitting the loaders this way keeps each library's dependencies small. `config` needs only the
JSON loader, so it does not pull in libpng, libjpeg or FreeType. Only an app that links `audio`
pulls in SDL3_mixer.

Background: [ADR-0079](../adr/0079-assets-loaders-are-registered.md)

## JSON documents

`asset::kind::Json::document()` returns the parsed `boost::json::object`. Read its members
through the checked reads in [api/asset/Json.h](../../api/asset/Json.h). `boost::json::object::at()`
throws on a missing key, and `boost::json::value_to` and the `as_` accessors throw on a value of
the wrong type. When the document is not what you expect, return false with a log line.

**A game's own text format keeps its parser in the game.** The api parses the formats its own
libraries read: JSON documents, glTF, and RIB for moya. A grammar added for one game's record or
script files would commit the api to keeping that format stable for games that do not use it.

Each checked read takes an object and a key:

| Read | Returns | Refuses |
|---|---|---|
| `readString(object, key)` | `std::optional<std::string>` | anything but a string |
| `readNumber(object, key)` | `std::optional<double>` | anything but a number |
| `readBool(object, key)` | `std::optional<bool>` | anything but `true` or `false` |
| `readObject(object, key)` | `const boost::json::object*` | anything but an object |
| `readArray(object, key)` | `const boost::json::array*` | anything but an array |

- **A missing member and a member of the wrong type are both refused.** The read returns an
  empty optional or a null pointer, and does not throw for either.
- **A caller that reports the two cases differently tests `contains()` first**, then reads.
- `readNumber` takes a signed integer, an unsigned integer or a double, and returns a double. An
  integer beyond 2^53 loses precision. The value is not tested for being finite.
- `readBool` does not take a number as a bool.
- `readObject` and `readArray` return a pointer into the object rather than a copy. The pointer
  is valid while the object is unchanged.
- An element of an array is not a member, so these reads do not apply to it. Test its kind with
  `is_string()` or `is_number()` before reading it.

A number that becomes an integer goes through the checked conversions in
[Types.md](Types.md#checked-conversions) after it is read.

`asset::readFile(path)` in [api/asset/File.h](../../api/asset/File.h) reads a whole file as
bytes, without a loader. It sizes the file, then reads it. It returns `std::nullopt` when the
file will not open, cannot be sized, or reads fewer bytes than its size. An empty file gives an
empty string.

## Writing documents

The write functions are in [api/asset/Writer.h](../../api/asset/Writer.h).

| Function | What it does |
|---|---|
| `writeDocument(path, value)` | Writes a JSON value as readable text, ending in a newline. |
| `writeFile(path, bytes)` | Replaces a file with bytes. Use it for any non-JSON format. |
| `serializeDocument(value)` | Returns the readable text without writing it, with no trailing newline. |

**Every write is atomic.** The bytes go to a temporary file beside the target, which is then
renamed over it. If anything fails before the rename, the old file is untouched, the
temporary is removed, and the function returns false.

- The process needs permission to create files in the target's directory, not only to write the
  target.
- On Windows, renaming over a file that another process holds open fails. The function returns
  false; report it so the player can close whatever holds the file.
- The write is not flushed to disk. It is safe against a crash, not against power loss: after a
  power cut the player finds either the old document or the new one, never half of one.
- A temporary left by a process killed during a write is not cleaned up, but the next successful
  write to the same path overwrites it.
- Other programs watching the directory see a file created and renamed, not modified.

**The output is always readable, and there is no compact mode.** Scalars, short vectors, and
objects of those stay on one line. Everything else is indented two spaces per level. A double
that a float holds exactly prints as that float, so `0.1f` prints as `0.1` rather than
`0.10000000149011612`. Any other double prints as a double, so `1e-50` and `123456789.123` keep
their values. JSON has no form for infinity or NaN, so `writeDocument()` returns false for a
document holding one and leaves the file untouched. Call `asset::serializeDocument()` by its full
name. An unqualified `serialize()` on a `boost::json::value` finds `boost::json::serialize` by
argument-dependent lookup, and that writes the whole document on one line.

What a document holds is the caller's decision. These functions decide only how it reaches the
disk. `engine::Settings`, the editor's project files and `config::SpriteSheets` all write
through them.

Background: [ADR-0041](../adr/0041-files-write-documents-atomically.md)

## Reading old documents forward

A document a game writes outlives the build that wrote it. `asset::readForward()` in
[api/asset/Migration.h](../../api/asset/Migration.h) upgrades an old JSON document to the
current version, one version at a time.

- **The document carries a whole number `"version"` at its root**, starting at 1.
- **The chain is a list of steps, oldest first.** `chain[0]` takes version 1 to 2, `chain[1]`
  takes 2 to 3, and so on. A build at version `n` has `n - 1` steps.
- A step is an `asset::Migration`, `std::function<bool(boost::json::object&)>`. It returns false
  if it cannot upgrade the document.

```cpp
const std::vector<v3d::asset::Migration> chain = {
    [](boost::json::object& doc) { doc["volume"] = doc["sound"]; doc.erase("sound"); return true; },
};
switch (v3d::asset::readForward(&doc, 2, chain)) {
    case v3d::asset::Reading::Current:  break;                 // already version 2
    case v3d::asset::Reading::Migrated: save(doc); break;      // may be written back
    case v3d::asset::Reading::Newer:    readOnly_ = true; break;
    case v3d::asset::Reading::Refused:  useDefaults(); break;
}
```

| Reading | Meaning |
|---|---|
| `Current` | Already at this build's version. Untouched. |
| `Migrated` | Upgraded to this build's version. It may be written back. |
| `Newer` | Written by a later build. Untouched. **Do not overwrite it.** |
| `Refused` | No version, a version that is not a whole number of at least 1, a missing step, or a failed step. Untouched. |

The rules:

- The upgrade runs on a copy and stamps the version after each step. The caller's document is
  replaced only if every step succeeds, so a document is never left half-upgraded.
- **A step must read and write only the document it is given.** `readForward()` cannot undo a
  change a step makes anywhere else.
- **Refusing to overwrite a `Newer` document is the caller's job.** `engine::Settings` does it
  through `writable()`.
- A step is plain code with no schema behind it. Write a test for each one.
- A step can never be removed while documents of the version before it may still exist.

Background: [ADR-0073](../adr/0073-files-migrate-old-documents-one-version-at-a-time.md)

## Images

`api/image` (`v3d::image`) reads, writes and compares images. It depends on libpng and libjpeg
and on no device.

### Image

`image::Image` ([api/image/Image.h](../../api/image/Image.h)) is a buffer of pixels with a width,
a height and a bit depth.

- **Row 0 is the top of the picture.** Every reader produces this order, and the canvas, the
  texture upload and the atlas packer all expect it.
- **`format()` is the channel count:** `Format::Grey` (1), `RGB` (3) or `RGBA` (4). It follows
  the depth.
- **An `Image` cannot be copied.** It owns its buffer. Hold one through
  `boost::shared_ptr<Image>`, and use `image::crop()` to make a copy of all or part of one.

### Reading and writing

`image::Factory` ([api/image/Factory.h](../../api/image/Factory.h)) reads and writes by file
extension: `png`, `jpg`, `jpeg`, `bmp` and `tga`.

```cpp
v3d::image::Factory factory(logger);
boost::shared_ptr<v3d::image::Image> picture = factory.read("sprite.png");
factory.write("out.tga", picture);
```

- `read(data, size, kind)` decodes from memory. Pass the format key, such as `"png"`, because a
  buffer has no extension. A glTF file's embedded images are read this way.
- Every reader returns rows top to bottom. A bmp is read at 8, 16, 24 or 32 bits, uncompressed
  or, at 16 and 32 bits, packed by the masks its header gives. 16 bits uncompressed is five bits
  a channel. 32 bits reads as RGBA, and as opaque when its alpha is zero everywhere, since most
  writers leave it unused. Every other depth comes back as RGB.
- Each read returns null when the file will not open or does not decode.
- Through the asset manager, an image file loads as `asset::media::kind::Image`, and `image()`
  returns the `image::Image`.

**The readers do not agree on greyscale.** Know which you are reading:

| Format | A greyscale file reads as |
|---|---|
| png | RGB. The reader has libpng expand grey to RGB. |
| bmp | RGB. An 8-bit file's indices are resolved through its palette. |
| jpeg | RGB. The reader always has the decoder output RGB. |
| tga | Grey, at the file's own depth. |

**Every writer can write greyscale**: png as a grey image, tga as type 3, jpeg as
`JCS_GRAYSCALE`, and bmp as 8-bit indices into a 256-entry grey ramp.

### Other functions

- `crop(source, x, y, width, height)` cuts a rectangle into a new image at the same depth. It
  returns null when the rectangle is empty or not wholly inside the source. Use it to cut a
  sprite back out of a packed sheet.
- `swapRedBlue(from, to, pixels, channels)` swaps the first and third bytes of each pixel, for
  converting between RGB and the BGR order BMP and TGA store. The buffers may be the same.
- `compare(a, b, tolerance)` compares two images channel by channel and returns a
  `Difference`. It records the worst pixel even when the images match. `description()` gives
  one line saying what differed and where. Keep the tolerance for float rounding between
  compilers, not for "close enough".

## Texture atlas

`image::TextureAtlas` ([api/image/TextureAtlas.h](../../api/image/TextureAtlas.h)) packs many
small images into one larger image, so they can share one texture.

```cpp
v3d::image::TextureAtlas atlas(512, 512, 4, logger);     // width, height, bytes per texel
glm::ivec4 r = atlas.region(32, 32);                     // the size you will blit
if (r.x < 0) { /* the atlas is full */ }
atlas.region(r.x, r.y, r.z, r.w, pixels, 32 * 4);        // blit into it
```

- **Ask for the size you will blit, and blit into exactly the rectangle you get back.** The
  atlas reserves a one-texel gutter on all four sides of every region itself. Do not add the
  gutter to what you ask for or subtract it from what you get.
- `region(width, height)` returns `x`, `y`, width and height of the usable rectangle, or an `x`
  and `y` of -1 when there is no room left.
- The sheet also keeps a one-texel border around its edge.
- **The gutter is left as zero:** black at depth 3, transparent black at depth 4. Edge texels
  are not copied into it. A linear sampler reading exactly at a region's edge therefore fades
  slightly towards the gutter. If that shows, inset the uvs by half a texel, as tetris does.
- Each region costs two texels more per axis than its size. The largest region a 16 × 16 atlas
  can hold is 12 × 12.
- `image()` returns the packed image for upload, and `write(filename)` saves it.

Without a gutter, a linear sampler at a region's edge reads the neighbouring region's texels.
The visible result is a thin line of the wrong colour along a sprite's edge, with nothing in the
log.

## Fonts

`api/font` (`v3d::font`) rasterizes typefaces with FreeType into a texture atlas and lays out
text as quads. Most apps do not use it directly: `ui::paint::TextRenderer` owns a font, its atlas
and the drawing of text. See [ui/](ui/README.md).

- **`TextureFont(filename, size, logger, spread)`** loads one face at one size.
  - With `spread` 0 the glyphs are plain coverage.
  - With a `spread` above 0 the glyphs are a signed distance field. `size` is then the base size,
    and text at any other size is drawn as a ratio of it. `spread` is how many pixels either side
    of an edge the field runs. A larger spread lets a glyph scale up further before its edge
    softens, and costs `2 × spread` extra texels per axis of every glyph.
- **`atlas(textureAtlas)`** sets the atlas the glyphs pack into, and **`loadGlyphs(charcodes)`**
  rasterizes and packs them. **Check its result.** It returns false when any glyph did not fit.
  Text drawn with a partly packed font is missing characters and measures short, so layout
  around it is wrong as well.
- `glyph(charcode)` returns a glyph's size, offset, advance and uvs, or null if it was not
  loaded. A charcode it has not seen is rasterized and packed then. `glyph(-1)` is an opaque
  white square, used to draw lines and backgrounds.
- `packed(charcode)` returns a glyph only if it is already packed, and never adds one. Use it
  once the atlas has been uploaded, since a glyph packed afterwards is not on the device.
- `ascender()`, `descender()`, `height()`, `linegap()`, `underlinePosition()` and
  `underlineThickness()` are the face's metrics.
- **`TextureFontCache(width, height, depth, logger)`** shares one atlas among several fonts.
  Depth is 1 for coverage or a distance field, and 3 for subpixel coverage.
- **`TextureTextBuffer::addText(&pen, markup, text)`** lays out a wide string as quads, moving
  the pen. A `Markup` sets the size, colours, gamma, underline, overline, strikethrough and the
  font. A size of zero or less, which is a new markup's, means the font's own size.

## Models and glTF

The glTF loader ([api/asset/media/loader/Gltf.h](../../api/asset/media/loader/Gltf.h)) reads
`.gltf` and `.glb` files into an `asset::media::kind::Model`. Its `model()` is a `type::Model`,
described in [Types.md](Types.md#models).

### What the loader builds

- **One model per file**, with one vertex array and one index list. The loader walks the file's
  scene from its root nodes and merges every mesh a node names, placed by the node's world
  matrix. Normals are turned by that matrix's inverse transpose.
- The scene is the file's default scene, or its first, or every root node when it has none.
- **A mesh named by two nodes is merged twice.** A file that reuses one mesh many times makes a
  larger upload than the file.
- **Primitives that share a material become one part**, in the order the loader reaches them.
  Parts are in the order their materials first appear. A primitive with no material uses a
  default one.
- Positions are required. Normals and uvs are read where a primitive has them, and are zero
  where it does not.
- A primitive with no indices gets a sequential run, so the model is always indexed.
- Triangle strips and fans become triangle lists, wound as glTF winds them. A primitive of
  points or lines is left out with a warning, because a model holds triangles.
- A file that does not read or parse gives no asset and a log line.
- **A file that fails validation gives no asset and a log line.** The loader validates before it
  reads an accessor, so an accessor that runs past its buffer view, or an index past the last
  vertex, rejects the whole file.

### Materials and textures

**Only the base colour is read**, from the metallic-roughness model.

- **A texture the file names arrives as a name**, in `Material::baseColourTexture`. The name is
  the file's uri with its percent-escapes decoded, so `my%20texture.png` arrives as
  `my texture.png`. It is relative to the model file. It is resolved through the asset
  manager and uploaded by whoever draws the model: the renderer's `MeshRegistry` does this for
  a model it loads. The model never holds the pixels of a named texture.
- **A texture the file embeds arrives decoded.** A `.glb` stores images in its own buffer, and a
  `.gltf` may inline one as a data uri, whose `data:image/png` or `data:image/jpeg` prefix
  states its type when the file gives no `mimeType`. Neither has a name to give, so the loader
  decodes it and `kind::Model::baseColourImage(material)` returns it. It is null for a named
  texture or no texture.
- An embedded image in a format glTF does not allow, or one that does not decode, costs the
  model that texture. The model still loads.

### Skins and animation

- **The first skin** any mesh in the scene is bound to becomes the model's skeleton. Its joints
  are reordered so that every parent comes before its children.
- **A skin with fewer inverse bind matrices than joints gives no asset** and a log line, and so
  does one whose inverse bind matrices are not 4x4 matrices. A skin with no inverse bind
  matrices binds each joint by the identity.
- Every vertex gets an influence. **Four influences per vertex are read**, and a second set of
  joints and weights is dropped.
- **A skinned mesh is placed by its joints, not by its node.**
- **An unskinned mesh in a skinned file follows the nearest joint above it rigidly**, or the
  first root joint if it is under none. It stands where its node puts it while that joint is
  at rest.
- **Every animation in the file becomes a clip**, keeping the channels that animate the
  skeleton's joints. A file with no skeleton has no clips.

### Drawing a model

A `type::Model` holds no device resources. To draw it, register it with the renderer's
`MeshRegistry`, which uploads it once per path. [rendering/](rendering/README.md) covers that.

Background: [ADR-0069](../adr/0069-models-material-parts-over-one-vertex-buffer.md)
