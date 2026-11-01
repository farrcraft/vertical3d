# Fonts

This page is for someone writing an app against the api, or changing how text is drawn. It
covers `api/font`. Terms are defined in the [glossary](README.md#glossary).

`api/font` (`v3d::font`) rasterizes typefaces with FreeType into a texture atlas and lays out
text as quads. Most apps do not use it directly: `ui::paint::TextRenderer` owns the faces, their
atlas and the drawing of text. See [ui/](ui/README.md).

## Faces and the fallback chain

- **`Library`** is the FreeType library an app's faces are opened through. A face holds its
  library, so the library closes after its last face. A library and its faces are used from one
  thread.
- **`Face::open(library, filename, logger)`** opens a font file and holds it open. It returns
  null, and logs why, for a file that does not open or has no Unicode character map.
  - `family()` and `style()` are the names the file gives. `weight()` is the OS/2 weight class,
    from 100 for thin to 900 for black, with 400 regular and 700 bold. A file with no OS/2 table
    is 700 when it says it is bold, and 400 otherwise. `italic()` is the file's italic flag.
  - `glyphIndex(charcode)` is the index of a code point's glyph. Index 0 is the face's missing
    glyph, and `has(charcode)` is whether the index is not 0. A glyph is named by its face and
    its index, a `FaceGlyph`.
- **`FaceStyle`** is the weight and style a face is chosen by: `{ .weight = 600, .italic =
  false }`. The weight defaults to 400 and italic to false.
- **`Family(name)`** holds the faces of one typeface. `add(face)` leaves out a null face.
  `face(style)` chooses as CSS does:
  - Faces of the style asked for are tried first, and all faces when there are none.
  - The weight asked for is used when a face has it.
  - For a weight from 400 to 500, the nearest heavier face up to 500, then the nearest lighter
    face, then the nearest face heavier than 500.
  - For a weight below 400, the nearest lighter face, then the nearest heavier one.
  - For a weight above 500, the nearest heavier face, then the nearest lighter one.
- **`FallbackChain`** holds families in order. `add(family)` leaves out an empty family.
  `faces(style)` is each family's face for a style, in order. `find(charcode, style)` is the
  glyph from the first of those faces that has the code point. A code point no
  face has is the first face's missing glyph. A chain with no families finds a null face.

## Texture fonts

- **`TextureFont(face, size, logger, spread)`** draws one face at one size.
  `TextureFont(filename, size, logger, spread)` opens the file through a library of its own.
  - With `spread` 0 the glyphs are plain coverage.
  - With a `spread` above 0 the glyphs are a signed distance field. `size` is then the base size,
    and text at any other size is drawn as a ratio of it. `spread` is how many pixels either side
    of an edge the field runs. A larger spread lets a glyph scale up further before its edge
    softens, and costs `2 × spread` extra texels per axis of every glyph.
  - A distance field is built from the unhinted outline. A coverage glyph is hinted by the
    face's own instructions. A glyph's advance comes from the same load as its bitmap.
  - Glyphs are kept by their index in the face. Code points drawn with one glyph pack it once.
- **`atlas(textureAtlas)`** sets the atlas the glyphs pack into, and **`loadGlyphs(utf8)`**
  rasterizes and packs each character of a UTF-8 string. **Check its result.** It returns
  false when any glyph did not fit. Text drawn with a partly packed font is missing characters
  and measures short, so layout around it is wrong as well.
- `glyph(charcode)` returns a code point's glyph: its size, offset, advance and uvs, or null if
  it cannot be loaded. A code point it has not seen is rasterized and packed then.
  `glyph(TextureFont::SOLID)` is an opaque white square, used to draw lines and backgrounds.
- `packed(charcode)` returns a glyph only if it is already packed, and never adds one. Use it
  once the atlas has been uploaded, since a glyph packed afterwards is not on the device.
- **`fallback(font)`** gives a font another font for the code points it lacks, and that font
  may have a fallback of its own, as a `FallbackChain` orders its faces. `loadGlyphs()` packs
  each code point the face has no glyph for into the fallback, when some font down the chain
  has it. `packed()` looks there for it, so the text buffer lays out a string that mixes them
  as one font. The fallback has to pack into the same atlas at the same size. Several fonts may
  share one fallback, and a code point is packed into it once. A code point no font down the
  chain has is packed as this face's missing glyph.
- `ascender()`, `descender()`, `height()`, `linegap()`, `underlinePosition()` and
  `underlineThickness()` are the face's metrics.
- **`width(utf8, size)`** measures a UTF-8 string at a size from the packed glyphs alone. A
  code point with no packed glyph adds nothing.
- **`TextureFontCache(width, height, depth, logger)`** shares one atlas among several fonts.
  Depth is 1 for coverage or a distance field, and 3 for subpixel coverage.
- **`TextureTextBuffer::addText(&pen, markup, utf8)`** lays out a UTF-8 string as quads, moving
  the pen. **It draws only glyphs already packed**, so it never adds to an atlas that is on the
  device. A code point with no packed glyph draws nothing and moves the pen by nothing, and the
  lines and background need `SOLID` packed too. A `Markup` sets the size, colours, gamma,
  underline, overline, strikethrough and the font. A size of zero or less, which is a new
  markup's, means the font's own size.
  - y grows downwards and the pen sits on the baseline. Each line's baseline is the pen's y
    rounded to a whole pixel, once. Every glyph on the line is placed at its exact scaled
    position and size from it, so the line's glyphs share one baseline. x is not rounded.
  - A newline moves the pen down by the font's `height()` and back to the first pen's x.
  - A background is one quad behind each line, drawn before its glyphs. An underline, an
    overline and a strikethrough are each one quad along the line, drawn after them.
