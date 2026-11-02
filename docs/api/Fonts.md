# Fonts

This page is for someone writing an app against the api, or changing how text is drawn. It
covers `api/font`. Terms are defined in the [glossary](README.md#glossary).

`api/font` (`v3d::font`) opens typefaces with FreeType, shapes and lays out text, and rasterizes
its glyphs. Most apps do not use it directly: `ui::paint::TextRenderer` owns the faces, their
glyphs and the drawing of text. See [ui/](ui/README.md).

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
  - `metrics(size)` gives the face's `FaceMetrics` at a size in pixels, unhinted: its
    ascender, descender and line gap, and the position and thickness of an underline and a
    strikethrough, each as a distance from the baseline. A face whose OS/2 table asks for its
    typographic metrics is measured by them, and any other by its hhea table.
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

## Shaping

Text is shaped by HarfBuzz, over the faces a `FallbackChain` holds. Shaping applies a font's
kerning, mark positioning, ligatures and contextual forms, and any OpenType feature asked for.

- **`Shaper(chain)`** shapes for one chain. **`shape(utf8, style, size, features)`** shapes a
  UTF-8 string at a `FaceStyle` and a size in pixels, and returns its runs in the string's
  order. An empty string, a size that is not above 0 and at most `Shaper::LARGEST`, or a chain
  with no faces gives no runs.
- **`shape(utf8, from, to, bidi, style, size, features)`** shapes only the bytes from one
  offset to another, with a `Bidi` resolved over the whole string. The rest of the string is
  the context, so the characters either side shape the range's edges as they would inside one
  run. A paragraph shapes each of its style ranges this way.
- **Runs.** A character stays in the run before it when that run's face has it, or is the face
  the chain finds for it. Otherwise it starts a run in the face the chain finds. A run also ends
  where the script changes. A character with no script of its own, such as a space, a digit or
  a combining mark, takes the script of its run, or of the first character after it that has
  one. Text with no script at all is shaped as Latin.
- **Direction.** A run also ends where the embedding level changes, as [Bidi](#bidi) resolves
  it. A run's `level` is that level, and its direction is right to left when the level is odd,
  so a number inside Arabic text is a run of its own, shaped left to right. The last argument of
  `shape` is the `BaseDirection` each paragraph is read in, `Auto` unless given.
- **`ShapedRun`** holds its `face`, its `script` as an ISO 15924 code such as `"Latn"`, its
  `direction`, the byte range `start` to `end` it came from, its `glyphs`, and the sum of their
  advances. Its glyphs are in visual order, left to right, whatever its direction.
- **`ShapedGlyph`** holds the glyph's `index` in the run's face, its `advance`, the `offset` it
  is drawn at from the pen, and its `cluster`: the byte offset of the first character it came
  from. Every measure is in pixels at the size shaped, with y growing downwards. A combining
  mark takes no advance, and keeps its own character's cluster.
- Advances come from the face's metrics, unhinted, so they match a distance field glyph's.
- **`Feature`** turns an OpenType feature on or off across the whole string: `{ "tnum" }` for
  tabular figures, `{ "kern", 0 }` for no kerning. A feature that chooses among alternates takes
  the alternate's number as its value. A tag that is not four letters is left out.

Background: [ADR-0141](../adr/0141-text-opentype-shaping-with-harfbuzz.md)

## Segmentation

`Segmenter` divides UTF-8 text where Unicode divides it: into grapheme clusters and words as
UAX #29 defines them, and at the line break opportunities UAX #14 defines. ICU supplies the
rules, and a dictionary for the scripts that mark no word boundaries.

- **`Segmenter(locale, logger)`** loads the rules for a BCP 47 language tag, such as `"ja"`.
  An empty tag uses Unicode's rules untailored. A segmenter is used from one thread.
- **`valid()`** says whether the rules loaded. When they did not, which is logged, every code
  point is its own cluster, the whole string is one word segment, and a line ends only at its
  end.
- **`graphemes(utf8)`** gives the offsets a caret may stop at: 0, the start of every grapheme
  cluster, and the string's size. A cluster is what a reader sees as one character, such as a
  letter and its accents, a Hangul syllable, an emoji joined by zero width joiners, or a flag.
  An empty string gives only 0.
- **`words(utf8)`** divides the string at its word boundaries into `WordSegment`s, which cover
  it in order with no gap. A segment's `word` is true when it holds letters, digits or
  ideographs, and false for spaces and punctuation.
- **`lineBreaks(utf8)`** gives each place a line may end as a `LineBreak`: the `offset` where
  the next line would start, and whether the break is `mandatory`, after a newline or a
  paragraph separator. The last is the string's size. A line may break after a space or a
  hyphen, never before closing punctuation, and between the Thai, Lao, Khmer and Myanmar words
  a dictionary finds.
- Every offset is a byte offset at the start of a character. A malformed byte is a cluster of
  its own. An empty string gives no words and no line breaks.

Background: [ADR-0142](../adr/0142-text-bidi-and-segmentation-with-icu.md)

## Bidi

`Bidi` resolves a UTF-8 text's embedding levels as UAX #9 does, through ICU. Every character
gets a level: even reads left to right and odd right to left. Explicit embeddings, overrides and
isolates are applied. Every offset is a byte offset, and the text is not held after the `Bidi`
is made.

- **`Bidi(utf8, base)`** divides the text into paragraphs at each paragraph separator, such as
  a newline. Each paragraph is read in the `BaseDirection` given: `LeftToRight`, `RightToLeft`,
  or `Auto`, the direction of its first strong character, left to right when it has none.
- **`runs()`** gives the text's maximal `LevelRun`s of one level, in the text's order, covering
  it with no gap. A run's `rightToLeft()` says whether its level is odd.
- **`level(offset)`** gives a character's level. **`rightToLeft(offset)`** says whether the
  paragraph holding it reads right to left.
- **`line(start, end)`** gives one line's runs in the order they are drawn, left to right.
  Spaces at the line's end, and before a tab, take the paragraph's level, so a space where a
  line wraps sits at the paragraph's end edge. A line that is empty, runs past the text, or
  crosses from one paragraph into the next gives none.
- **`caret(offset, affinity)`** gives the `CaretEdge` a caret is drawn against: a character
  and whether it is that character's right edge. Where two runs of different direction meet,
  the characters either side of a caret are drawn apart, so the caret has an `Affinity`.
  `Upstream` goes with the character before it, at that character's end edge, and is what typing
  or moving forwards gives. `Downstream` goes with the character after it, at its start edge.
  At the text's start or end there is only one character, and that one is taken. An empty text
  gives the paragraph's start edge.

Background: [ADR-0142](../adr/0142-text-bidi-and-segmentation-with-icu.md)

## Paragraphs

A `Paragraph` is the text object the ui and the games hold: a text with styles over its
ranges, divided into lines at a width, and placed. It answers its size, its lines, where a caret
goes and what a point is over. It needs no GPU; a renderer draws it by walking its lines.

**Styles.**

- **`TextStyle`** names the `fonts` (a `FallbackChain`), the `face` (weight and italic), the
  `size` in pixels, the OpenType `features` and the `colour`. It also says whether it has an
  `underline` or a `strikethrough`, and lists the `effects` drawn beneath it, as
  [effect fields](#effect-fields) describes. Its `contrast`, from 0 to 1, strengthens the
  glyphs' coverage, as light text on a dark ground needs. The first four decide the glyphs;
  `shapesLike()` compares those alone. The colour, the decorations, the effects and the contrast
  only decide how the glyphs are drawn.
- **`ParagraphStyle`** names the `direction` each paragraph is read in, the `alignment`, and
  the `width` lines wrap at. At a width of 0 or less they never wrap. It also names
  `maxLines`, 0 for no limit, and whether a cut line ends in an `ellipsis`. Its
  `lineSpacing` is a line's height as a multiple of its faces' natural height.
- **`Alignment`** is `Start` or `End`, which follow the paragraph's direction, or `Left`,
  `Right` or `Center`.

**Making and changing one.**

- **`Paragraph(cache, text, style, paragraphStyle)`** holds a text in one style.
- **`setStyle(start, end, style)`** styles the bytes from one offset to another, clamped to the
  text. `styles()` gives the `StyledRange`s, which cover the text in order, with adjacent ranges
  in one style joined. `styleAt(offset)` gives the style of the character there.
- **`setText(text)`** replaces the text, which takes the style of the first range.
- **`setWidth(width)`** and **`setParagraphStyle(style)`** change how it is laid out.

**What a change costs.** A paragraph is shaped once and laid out once, when first asked.

| Change | Shapes again | Lays out again |
|---|---|---|
| The width, the alignment, the line limit, the ellipsis or the line spacing | No | Yes |
| A style whose colour, decorations, effects or contrast alone differ | No | No |
| A style that shapes differently, the text, or the direction | Through the cache | Yes |

**The shaping cache.** Every paragraph of an app shares one `ShapingCache`, made with the
locale whose rules divide words and lines. It keeps the `ShapedText` of the texts most recently
used: the string, its levels, its shaped runs, its clusters and its line breaks. A text is found
by its string, its direction and its ranges as they shape, with adjacent ranges that shape alike
taken as one. Text built again each frame, as immediate mode builds it, is found rather than
shaped. Past its capacity, 256 texts unless another is given, the text used longest ago is let
go. A paragraph keeps its own text whether or not the cache does. `shaped()` counts the texts
shaped rather than found. The cache and its paragraphs are used from one thread.

**Lines.**

- A line breaks after the last opportunity that fits, as `Segmenter` finds them. A line
  separator always ends one. A word wider than the width breaks between its grapheme clusters,
  with at least one cluster on each line.
- Space at a line's end hangs past its edge: it is not counted in the line's `width`, and it is
  not aligned. In a paragraph read right to left it hangs past the left edge. A no-break space,
  a figure space and a narrow no-break space hold words together, and do not hang.
- With `maxLines` above 0, the lines past it are dropped. The empty line after a closing line
  separator hides no text, so dropping it alone cuts nothing. Otherwise the last line shown is
  cut at a cluster so that it and an ellipsis, U+2026 in the style of the text before it, fit the
  width. The ellipsis is a run with `ellipsis` set, at the paragraph's end edge.
- Empty text is one empty line, and text ending in a line separator has an empty line after it.
  An empty line is as tall as its style's first face.
- A **`Line`** holds the byte range `start` to `end`, which includes the space and the line
  separator ending it. It holds its `top`, `height` and `baseline`, downwards from the
  paragraph's top, and the `x` and `width` of its text without the space at its end. A line is as
  tall as the ascender, descender and line gap of the tallest faces on it, times the line
  spacing, with the baseline centred in any extra height.
- A line's **`LineRun`s** are in the order they are drawn, left to right. Each holds its `face`,
  `size`, direction, byte range, `x` from the paragraph's left, `advance`, and `glyphs`. A glyph
  is drawn at its run's `x` plus the advances before it, plus its own offset. A run's colour and
  decorations are those of the style at its glyphs' clusters, as `styleAt()` gives it.
- Text is not shaped again where a line breaks it, so a ligature or a joining form across the
  break keeps its shape.

**Carets, hits and selections.** Every index is a byte offset, and a caret stops only at a
grapheme cluster's edge; `clusters()` gives those offsets. A glyph that came from several
clusters, such as a ligature, is shared between them evenly.

- **`caret(index, affinity)`** gives the `Caret`'s `x`, and the `top` and `height` of its line.
  Upstream it is the end edge of the cluster before the index, and downstream the start edge of
  the cluster after it, which differ where runs of two directions meet. At a wrap, upstream is
  the end of the line above and downstream the start of the line below. An index past a cut line
  is drawn at the end of the last line shown.
- **`hitTest(point)`** gives the `Hit` nearest a point: an index and its affinity. It takes the
  line the point is over, or the nearest line. Over a cluster's left half it is that cluster's
  left edge, and over its right half its right edge. Left or right of the line it is the line's
  edge on that side.
- **`selection(start, end)`** gives the `SelectionRect`s covering the clusters from one index to
  another: one for each stretch of a line they cover side by side, top to bottom.

Background: [ADR-0143](../adr/0143-text-styled-paragraphs-over-a-shared-shaping-cache.md)

## The glyph store

A `GlyphStore` holds the glyphs text is drawn from. A glyph is made the first time it is drawn
and packed into pages, which a renderer copies to the device. It needs no GPU.

- **`GlyphKey`** tells one glyph from another: its `face`, its `index` there, its `size` in
  pixels, the `subpixel` step its pen was at, and its `mode` (`Coverage`, `Distance` or
  `Colour`). It also holds a distance field's `spread` in pixels, a colour glyph's `palette` and
  `foreground`, and a variable font's normalized `coordinates`. A key names its face by address,
  so a face the store holds glyphs from stays open while the store is used.
- **`glyph(key, rasterize)`** gives the `StoredGlyph`: its `page`, the `position` of its top
  left texel there, its `size`, and its `bearing`, the top left pixel from the pen with y
  downwards. The first time a key is asked for, `rasterize` makes its `GlyphBitmap` and the
  store packs it. A glyph with no pixels, such as a space, is stored with no size. A glyph wider
  or taller than a plot with its padding is not stored, nor is a bitmap whose pixels do not match
  its size, nor one whose `channels` are not the pages'. `made()` counts the glyphs made rather
  than found.
- **Pages and plots.** `Spec` gives each page's `pageSize` (1024 unless given), each plot's
  `plotSize` (256), the `pages` kept (4), and the `padding` around each glyph (1). Its
  `channels` are the bytes in a pixel: 1, or 4 for a store of colour glyphs. A page is cut into
  square plots, and a plot holds glyphs on shelves, each as tall as its first glyph.
- **When it fills.** A glyph goes into a plot with room, then an empty plot, then a new page up
  to the limit. With none of those, the plot used longest ago is emptied and used again, and
  every glyph in it is made again the next time it is drawn.
- **Frames.** `beginFrame()` starts a frame. A plot holding a glyph asked for in the current
  frame is never emptied, because the frame being built draws from it. When every plot is in
  that use, a page is added past the limit. A page past the limit is let go at the start of the
  frame after one that used nothing in it.
- **Reaching the device.** `pixels(page)` gives a page's pixels. `regions()` gives the plots
  changed since it was last called, each as a `GlyphRegion`, and a page added is changed whole.
  A renderer copies them with `Textures::update()`. It compares `pages()` with the textures it
  holds to find a page added or let go.

Background: [ADR-0144](../adr/0144-text-glyph-pages-of-plots-reused-least-recently-used-first.md)

## Coverage glyphs

**`rasterize(key)`** makes a glyph's coverage for the glyph store, as a `GlyphStore::Rasterizer`.

- It rasterizes at the key's size in pixels, the glyph's pen moved right by the key's
  `subpixel` step, in steps of a quarter of a pixel (`SUBPIXEL_STEPS`).
- It hints the glyph by its face's own instructions vertically, so stems and the x-height sit
  on whole pixels. It does not hint horizontally, so the glyph's shape is the same at every step.
- It gives one byte a pixel, with the `bearing` of its top left pixel from the pen.
- A key whose mode is not `Coverage`, a face that cannot be sized, or a glyph that cannot be
  loaded gives an empty bitmap.

A renderer draws a glyph with its pen rounded down to a step: the whole pixel it is in, plus
the bearing, is where the bitmap's top left goes, and the step picks the rasterization. Each
line's baseline is rounded to a whole pixel.

Background: [ADR-0145](../adr/0145-text-coverage-on-the-pixel-grid-curves-when-scaled.md)

## Curve glyphs

Text that is scaled, turned or placed in the world is drawn from its glyphs' curves, as the
Slug algorithm draws it. **`outline(face, index)`** reads a glyph's `CurveGlyph`.

- **Curves.** The outline is read unscaled and unhinted, in em units with y upwards, as
  `QuadraticCurve`s from `p1` to `p3` pulled towards `p2`. A straight line has `p2` on `p3`. A
  cubic, as a CFF face holds, is split into four quadratics. A segment that goes nowhere is left
  out. The outline is closed, contour by contour, and filled by the nonzero rule. `min` and
  `max` bound it.
- **Bands.** Each axis is divided into strips of one thickness, about the square root of the
  number of curves of them, from 1 to 16. A `horizontal` band
  holds every curve whose height reaches into it, except a horizontal line, sorted from the
  largest x down. A `vertical` band holds the curves reaching into its strip, except a vertical
  line, sorted from the largest y down. Bands overlap their neighbours by `BAND_OVERLAP`, a
  1024th of an em, so a curve on the line between two is in both.
- **`bandTransform()`** gives the scale (x and y) and offset (z and w) that turn a point in em
  units into the band it is in. The reader clamps each axis to its bands.
- A glyph with no outline, such as a space, or one that cannot be loaded has no curves and no
  bands.

**The curve store.** A `CurveStore` keeps the curves text is drawn from, read the first time each
glyph is drawn. A glyph's curves are the same at every size, so it is found by its face and its
index, and kept as long as the store is.

- **`glyph(face, index)`** gives the `StoredCurves`: the value its `bands` start at, the `min`
  and `max` of its outline in em units, and whether it is `empty`, as a space is.
- **`curves()`** and **`bands()`** are the two arrays a renderer copies to the device, four floats
  a texel, in rows of `ROW` texels. Curve c is texels 2c and 2c + 1: p1 and p2, then p3.
- **Bands.** Band value k is float k of `bands()`. A glyph's bands start at its value g:
  - values g to g + 3 are its band transform, x and y scale, then x and y offset;
  - values g + 4 and g + 5 are how many vertical and horizontal bands it has;
  - from g + 8, each horizontal band has its curve count and the value its list starts at, and
    then each vertical band does;
  - each list holds curve indices, in the band's order.
- **Rows.** Each array grows by doubling its rows. `curveRows()` and `bandRows()` say how many
  it holds, and `changedCurves()` and `changedBands()` give the rows written since they were
  last asked for, once.
- A float holds an index exactly up to 2 to the 24th, so the store holds that many curves and
  values. A key names its face by address, so a face the store holds glyphs from stays open.
  `read()` counts the glyphs read rather than found.

## Colour glyphs

Emoji and other colour glyphs come from a face's colour tables. A face may hold several, and a
glyph is drawn from the first of COLR version 1, COLR version 0, SVG, then CBDT or sbix that has
it.

- **`Face::colour()`** gives the `ColourTables` a face holds: `layers` (COLR version 0), `paint`
  (COLR version 1), `svg`, `cbdt` and `sbix`. **`Face::coloured(index)`** says whether any of
  them draws a glyph.
- **`colour(face, index, palette, size)`** reads a `ColourGlyph` through HarfBuzz's paint
  interface, in a palette. A palette the face does not have is its first. A bitmap is taken
  from the strike nearest the size in pixels to the em. A glyph no colour table draws reads as
  empty.
- **SVG glyphs** are read through ThorVG, from the document element whose id is `glyph` and the
  glyph's index. A document compressed by gzip is inflated first. The document's first
  viewport is the em square, which a `viewBox` on its root maps onto, and nothing is clipped to
  it. Each shape's fill and stroke becomes a fill layer: inside the shape's outline, by its fill
  rule, or inside the outline FreeType's stroker draws around its path. Clip paths clip the
  layers beneath them, and opacity multiplies their colours' alpha. A masked element is a
  group that its mask's alpha keeps or removes. A luminance mask is read as an alpha mask, and
  a dashed stroke is drawn whole.

**The form.** A `ColourGlyph` is every format in one shape: a list of `ColourLayer`s, in em
units with y upwards, and the `min` and `max` of what it draws.

- A **`Fill`** layer is drawn inside every one of its `clips`. A `ColourClip` is a closed
  outline of `QuadraticCurve`s, filled by the nonzero rule, or by the even-odd rule when it says
  so. Its `ColourFill` is one colour or a gradient:
  linear, from one point to another; radial, from one circle to another; or a sweep, around a
  centre counter-clockwise from one angle to another. A gradient's points are in its own space,
  which its `transform` maps into em units, and its `ColourExtend` pads, repeats or reflects past
  its first and last stops.
- A **`ColourStop`** holds its offset and its colour, unpremultiplied, as the font gives it. A
  stop in the text's colour is `foreground`: it takes the text's red, green and blue, and its
  alpha times the text's. `foreground()` says whether any layer has one.
- An **`Image`** layer places one of the glyph's `ColourImage`s, a picture from a bitmap table,
  inside its clips. The picture fills a box in its own space, which its `transform` maps into
  em units. The box is on the strike's own pixels.
- **`BeginGroup`** and **`EndGroup`** gather the layers between them apart. The group is then
  put on what is beneath it by the end's `ColourComposite`, its colours times the end's
  `opacity`. The composite is one of Porter and Duff's modes, or a blend mode as CSS defines
  it. Layers outside a group, and within one, are drawn over each other in order.

**Painting.** **`paint(key)`** makes a colour glyph's pixels for the glyph store, as a
`GlyphStore::Rasterizer`, from the key's face, index, palette and size.

- It paints at the key's size, its pen moved right by the key's `subpixel` step, as a
  coverage glyph is.
- FreeType rasterizes each clip, antialiased. A fill's colours are found at each pixel's
  centre, and an image is sampled from its four nearest texels, or averaged over the pixel where
  it is shrunk.
- Layers are put down in the colours the font gives, blended as they are encoded, as browsers
  blend them. A gradient mixes its stops premultiplied, as COLR asks.
- A layer in the text's colour takes the key's `foreground`, packed by `packColour()`.
- It gives four bytes a pixel, red, green and blue unpremultiplied then alpha, with the
  `bearing` of its top left pixel from the pen. A glyph's edge within a 256th of a pixel of a
  whole pixel is taken to be on it.
- A key of another mode, a glyph with no colour, or a face with no library gives an empty
  bitmap of four channels.

`paint(glyph, library, size, subpixel, foreground)` paints a `ColourGlyph` already read.

Background: [ADR-0149](../adr/0149-text-colour-glyphs-from-every-opentype-colour-table.md),
[ADR-0150](../adr/0150-text-svg-glyphs-read-through-thorvg.md)

## Effect fields

A style's `effects` are drawn beneath its text from each glyph's signed distance field. A
`TextEffect` grows each glyph by a `width`, fades it to nothing over a `softness` beyond that,
and moves it by an `offset`, all in pixels at the text's size, in its `colour`. An outline has a
width, a shadow an offset, and a glow a softness. A width below 0 lies inside the glyph. The
effects are drawn in the order the style lists them, the first furthest back, and every glyph's
effects are drawn before any glyph.

- **`field(key)`** makes a glyph's field for the glyph store, as a `GlyphStore::Rasterizer`. It
  reads the unhinted outline at the key's size and `spread`. Each pixel is its distance from the
  edge, from the spread outside at 0 to the spread inside at 255, with the edge at the middle.
  The field reaches the spread past the outline on every side. A key of another mode or with no
  spread gives an empty bitmap.
- **`FIELD_SIZE`** is 64 pixels to the em, the size every field is made at. One field serves
  every size its glyph is drawn at.
- **`fieldSpread(reach)`** is the spread an effect reaching a distance past the edge needs, the
  reach in em units. It is the reach in the field's pixels with one more for the effect's edge,
  rounded up to a multiple of four so that nearby reaches share a field, and at most
  `MAX_SPREAD`, 32. An effect reaches at most half an em, and past that it is cut off.
- **`effectReach(effect, size)`** is how far an effect on text of a size reaches, in em units,
  and **`fieldKey(face, index, reach)`** the key of the field it is drawn from.
- **`effectRamp(effect, size, spread)`** gives the field values at which the effect is whole and
  at which it is gone. A renderer packs them into each quad with `canvas::fieldRamp()`, and the
  shaders fade the effect's coverage between the two.

A field drawn much larger than 64 pixels to the em softens the effect's edge. The glyph's own
edge stays sharp, because it is drawn as coverage or from its curves.

Background: [ADR-0146](../adr/0146-text-effects-drawn-from-a-distance-field.md)

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
