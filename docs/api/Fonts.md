# Fonts

This page is for someone writing an app against the api, or changing how text is drawn. It
covers `api/font`. Terms are defined in the [glossary](README.md#glossary).

`api/font` (`v3d::font`) rasterizes typefaces with FreeType into a texture atlas and lays out
text as quads. Most apps do not use it directly: `ui::paint::TextRenderer` owns the faces, their
atlas and the drawing of text. See [ui/](ui/README.md).

- **`TextureFont(filename, size, logger, spread)`** loads one face at one size.
  - With `spread` 0 the glyphs are plain coverage.
  - With a `spread` above 0 the glyphs are a signed distance field. `size` is then the base size,
    and text at any other size is drawn as a ratio of it. `spread` is how many pixels either side
    of an edge the field runs. A larger spread lets a glyph scale up further before its edge
    softens, and costs `2 × spread` extra texels per axis of every glyph.
  - A distance field is built from the unhinted outline. A coverage glyph is hinted by the
    face's own instructions. A glyph's advance comes from the same load as its bitmap.
  - The face is opened by the constructor and stays open until the font is destroyed.
- **`atlas(textureAtlas)`** sets the atlas the glyphs pack into, and **`loadGlyphs(utf8)`**
  rasterizes and packs each character of a UTF-8 string. **Check its result.** It returns
  false when any glyph did not fit. Text drawn with a partly packed font is missing characters
  and measures short, so layout around it is wrong as well.
- `glyph(charcode)` returns a code point's glyph: its size, offset, advance and uvs, or null if
  it cannot be loaded. A code point it has not seen is rasterized and packed then.
  `glyph(TextureFont::SOLID)` is an opaque white square, used to draw lines and backgrounds.
- `packed(charcode)` returns a glyph only if it is already packed, and never adds one. Use it
  once the atlas has been uploaded, since a glyph packed afterwards is not on the device.
- **`fallback(font)`** gives a face another face for the code points it lacks. `loadGlyphs()`
  packs each code point the face has no glyph for into the fallback, and `packed()` looks there
  for it, so the text buffer lays out a string that mixes the two as one font. The fallback has
  to pack into the same atlas at the same size. Several faces may share one fallback, and a code
  point is packed into it once. A face with no fallback packs its own missing glyph for a code
  point it lacks.
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
