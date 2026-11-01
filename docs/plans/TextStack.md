# Text Stack — Shaped, Hinted and Scalable Text for the Editor and Games

Drafted 2026-11-01 against `b3b5a4ec`. This plan's steps fall in `api/font`, `api/ui`,
`api/render/realtime`, the editor and the demos. When it closes, text is shaped by OpenType's
rules, laid out in lines in any direction, and drawn sharp at every size. Text on the screen's
pixel grid is rasterized at its exact size with hinting. Text that is scaled, turned or placed
in the world is drawn from the glyphs' curves. The editor and the games hold one kind of text
object, which measures, draws, places a caret and hit tests.

The text library is described in [Fonts.md](../api/Fonts.md), the ui's use of it in
[api/ui/](../api/ui/README.md), and the quad shader in
[Canvas.md](../api/rendering/Canvas.md#text).

A step with a testable CPU half brings test cases with it. A step that changes how text looks
is checked by running the editor and a game and reading a magnified screenshot, as
[Testing.md](../contributing/Testing.md) describes.

## What the survey found

The text library was read end to end at `b3b5a4ec`, with every caller in `api/ui`, the editor
and the demos. The editor's text was photographed and magnified at 12 pixels.

**How text is drawn.** `ui::paint::TextRenderer` opens Inter Regular, Inter SemiBold and Noto
Sans as one-channel signed distance fields. FreeType builds each field at 48 pixels with a
spread of 8. Every glyph of a charset given up front is packed into one 1024 by 1024 atlas,
which is uploaded once. Text at any size is drawn by scaling the field, so the editor's 12
pixel text is a quarter of the field's size. The quad shader turns a distance into coverage.

**What is wrong with what is drawn:**

* **Glyphs in one word sit on different rows.** `TextureTextBuffer::addText()` cuts each
  glyph's bottom and top to a whole pixel on its own
  (`api/font/TextureTextBuffer.cxx:106-107`). A word's letters can land a pixel apart, and a
  glyph's height changes by up to a pixel.
* **Edges are blurred across two pixels.** The shader blends from `0.5 - fwidth` to
  `0.5 + fwidth` (`api/render/shaders/quad.frag:50-52`), where one pixel is meant.
* **Small text has no hinting.** A field built at 48 pixels and drawn at 12 cannot be fitted to
  the pixel grid it lands on. Stems and the x-height fall between pixels.
* **The bitmap and the advance disagree.** Every glyph is loaded with
  `FT_LOAD_FORCE_AUTOHINT`, which overrides Inter's own hinting
  (`api/font/TextureFont.cxx:67-71`). Its advance comes from a second, unhinted load
  (`:391`).
* **A newline moves down too far.** It adds the descender to a height that already holds it
  (`api/font/TextureTextBuffer.cxx:31`).
* **Underlines and strikethroughs break at every glyph.** Each is drawn as a quad per glyph.

**What is missing:**

* **Kerning.** Inter and Noto Sans keep their kerning only in the GPOS table. Neither has a
  `kern` table, so `FT_Get_Kerning` would find none, and nothing reads GPOS.
* **Shaping.** No OpenType feature is applied. Inter's tabular figures, `case`, contextual
  alternates and stylistic sets cannot be reached. A combining mark is drawn as its own
  advancing glyph.
* **Lines in any direction.** There is no bidirectional reordering, no script itemization and
  no complex script.
* **Unicode text boundaries.** `ui::paint::wrap()` breaks at ASCII spaces only, and measures
  the whole line again for each word. `LineEdit` moves the caret by code point, not by
  grapheme cluster: a letter and its accents, or an emoji sequence, is several stops.
* **A text object.** The ui reaches text through `Measure` and `Write` callbacks that take a
  string. A text field finds its caret by measuring the string before it, which holds only
  while each glyph's advance is added to the last.
* **Glyphs on demand.** A character outside the charset is dropped with no width. A glyph that
  does not fit the atlas stops all text from drawing. The atlas never grows and never drops a
  glyph.
* **Text in the world.** `Canvas`'s world and depth-ordered quads have no text path.
* **Colour glyphs and variable fonts.** Neither is read.
* **Effects.** No outline, shadow or glow is drawn. The library's stroked outline path cannot be
  reached.

**How others do it.** Godot rasterizes ui text at its exact size with light hinting and quarter
pixel positions, and keeps a distance field mode for scaled text. Unity's TextMesh Pro has
hinted bitmap modes beside its distance fields. Unreal's Slate caches glyphs on demand, and
keeps distance field fonts for world text. Dear ImGui rasterizes any size on demand into an
atlas it updates in place. Godot, Slate and Unity's advanced text generator shape with
HarfBuzz. HarfBuzz does not reorder bidirectional text or find line breaks, so each pairs it
with a bidi library and a segmenter. Slug draws glyphs from their curves on the GPU, with no
atlas, and its patent is in the public domain.

## The design

Text passes through five layers. Each takes what the one above it gives.

| Layer | Takes | Gives |
|---|---|---|
| Fonts | Font files, and the axes of a variable font | Faces held open, a fallback chain, and each face's metrics |
| Shaping | A string, a style, and the OpenType features asked for | Runs of glyphs, each with its glyph index, advance, offset and source cluster |
| Layout | Shaped runs and a width | Lines, broken at Unicode's opportunities, reordered for direction, aligned, and cut with an ellipsis |
| Glyphs | A glyph, a size, a sub-pixel step and a mode | A rasterized or curve-encoded glyph in a GPU resource, made on demand |
| Drawing | A laid out text and a place on the screen or in the world | Quads, through the coverage shader or the curve shader |

**A text object** is what the ui and the games hold. It is shaped and laid out once, and kept
while its string, style and width stay the same. It answers its size, the caret position of a
string index, the index under a point, and the rectangles of a selection. It draws itself.

**Two ways to draw a glyph:**

| Mode | For | How |
|---|---|---|
| Coverage | Text on the screen's pixel grid: the editor, menus, HUDs | Rasterized by FreeType at its exact device size, hinted vertically, at a quarter pixel's horizontal step. Each line's baseline is on a whole pixel |
| Curves | Text that is scaled, turned, animated, or in the world | Drawn from the glyph's quadratic curves in the fragment shader, at any size and angle |

**What a style names:** the font family, the weight, the size, the colour, the OpenType
features, the variable font axes, the decorations, and the effects.

## Decisions

| Decision | Step |
|---|---|
| Text: how glyphs are drawn. Open: coverage beside curves, coverage beside a multi-channel distance field, or all three. It amends ADR-0036. | 9, 10 |
| Text: how effects are drawn. Open: from the curves, from a distance field kept for them, or from a coverage pass of a stroked glyph. | 11 |
| Text: the glyph atlas. Open: pages that grow and drop unused glyphs, or one fixed atlas cleared when it fills. | 8 |
| Text: the shaping library. Open: HarfBuzz, or kb_text_shape. | 3 |
| Text: the bidi and segmentation libraries. Open: SheenBidi with libunibreak, raqm, or ICU. | 4, 5 |
| Text: the text object the ui and the games hold, and how it is cached. Open. | 6 |
| Text: how coverage is blended. Open: as it lands in a display space target, with a contrast curve, or in linear space with stem darkening. | 12 |
| Text: which colour glyph formats are read. Open: COLR v0 and v1, CBDT, sbix and SVG. | 14 |

Each decision is researched against how established tools answer it before it is drafted as an
ADR.

## What blocks what

- **Step 1 blocks nothing.** It fixes today's path while the rest is built.
- **Step 2 blocks steps 3, 8 and 15.** Shaping, glyphs and variable fonts need faces held open.
- **Steps 3, 4 and 5 block step 6.** A text object holds shaped, segmented and reordered runs.
- **Step 7 blocks step 8.** New glyphs reach the device as region updates.
- **Steps 6 and 8 block steps 9 and 10.** Each mode draws a laid out text from the glyph store.
- **Steps 9 and 10 block step 11.** Effects are drawn through one of the modes.
- **Steps 6 and 9 block step 13.** The ui moves to the text object and to coverage together.
- **Steps 3 and 8 block step 14.** A colour glyph is found by shaping and stored as a glyph.

## Steps

### Step 1 — Today's path drawn correctly

**Closed.**

- Each line's baseline is the pen's y rounded to a whole pixel, once. Each glyph is placed at
  its exact scaled position and size from that baseline.
- The quad shader ramps coverage across one pixel. A pixel's width is the length of the
  distance's screen gradient, so a diagonal edge is no wider than a straight one.
- A glyph's bitmap and its advance come from one load. A distance field is built from the
  unhinted outline, and a coverage glyph is hinted by the face's own instructions.
- A newline moves down by the font's line height.
- A background is one quad behind each line, and an underline, an overline or a strikethrough
  one quad along it. They are measured downwards from the baseline, as glyphs are.
- Glyphs are looked up by a map. A face is opened by its font's constructor and stays open.
- The text buffer suite covers glyphs on one baseline, quad heights at a fractional size and
  pen, the newline, and the background and underline per line. The editor was photographed at
  12 pixels and magnified beside the same text before the change.

### Step 2 — Fonts held open, with a fallback chain

**Open.**

- A face holds its FreeType face open while it is in use, and one FreeType library serves
  every face.
- A font family names its faces by weight and style. A fallback chain lists the families to try
  in order, for each code point the ones before it lack.
- Glyphs are named by face and glyph index, not by code point.
- The font suite covers the chain's order, a code point no face has, and a face that will not
  open.

### Step 3 — Shaping

**Open.** Decide the shaping library.

- Text is split into runs by face, by script and by direction, and each run is shaped.
- A shaped run holds each glyph's index, advance, offset and source cluster. Kerning, marks,
  ligatures and contextual alternates are applied as the font defines them.
- A style turns OpenType features on or off: `tnum`, `case`, `ss01` and the rest.
- The manifests name the library's port.
- The shaping suite covers Inter's kerning of a known pair, `tnum` giving digits one width, a
  combining mark placed over its base, and an Arabic word joined.

### Step 4 — Unicode text boundaries

**Open.** Decide the segmentation library.

- Text is split into grapheme clusters, words and line break opportunities, as UAX #29 and
  UAX #14 define them.
- The segmentation suite covers an accented letter and an emoji sequence as one cluster, a break
  after a hyphen, and no break before a closing bracket.

### Step 5 — Bidirectional text

**Open.** Decide the bidi library, with step 4's.

- A paragraph's embedding levels are found as UAX #9 defines them. Each line's runs are
  reordered from logical to visual order.
- A caret at a direction boundary has an affinity, so it is drawn on the side it was moved to.
- The bidi suite covers Hebrew in an English sentence, a number in an Arabic sentence, and the
  caret at each boundary.

### Step 6 — The text object and layout

**Open.** Decide the text object and its cache.

- A text object is shaped and laid out from a string, a style and a width, and kept while
  those stay the same.
- Lines break at step 4's opportunities. A word longer than the width breaks at a cluster. A
  line is aligned left, right, centre or to the paragraph's direction, and may end in an
  ellipsis.
- Line height and the baseline come from the face's metrics and the style.
- It answers its size, the caret position of an index, the index under a point, and the
  rectangles of a selection, each by cluster.
- The layout suite covers wrapping, alignment, the ellipsis, the caret through kerned and
  reordered text, and a hit test on each half of a glyph.

### Step 7 — Partial texture updates

**Open.**

- `api/render/realtime` updates a region of a texture that is in use, after the frames reading
  it have finished, without waiting for the device to go idle.
- The device suite covers a region updated while the texture is drawn.

### Step 8 — The glyph store

**Open.** Decide the atlas.

- A glyph is rasterized or encoded the first time it is drawn. It is keyed by face, glyph index,
  size, sub-pixel step, mode and variable font axes.
- New glyphs reach the device through step 7's updates.
- The store follows the atlas decision when it fills, and no text stops drawing because one
  glyph did not fit.
- The charset option is removed.
- The glyph suite covers a glyph made once, a full store, and a glyph drawn after the store
  frees room.

### Step 9 — Coverage text

**Open.** Decide how glyphs are drawn.

- Text on the pixel grid is rasterized at its device size, the ui's size times its scale, with
  vertical hinting.
- A glyph's horizontal position is kept to a quarter pixel, and each quarter is its own
  rasterization.
- A change of scale draws the glyphs at the new size.
- The quad shader's coverage branch draws them.
- The coverage suite covers the sub-pixel steps and a change of scale. The editor is
  photographed at 12 pixels and at a scale of 1.5, and magnified.

### Step 10 — Curve text

**Open.** Decided with step 9.

- A glyph's quadratic curves are stored on the device, banded so a pixel reads only the curves
  that cross it.
- The curve shader finds each pixel's coverage from those curves, at any size and angle.
- `Canvas` draws text in world space and in depth order.
- The curve suite covers a glyph's bands. The device suite compares text drawn turned and at
  several sizes against reference images. A game is photographed with text in the world.

### Step 11 — Effects

**Open.** Decide how effects are drawn.

- A style may give text an outline, a shadow and a glow, each with a colour, a width and an
  offset.
- The effects suite covers each effect's geometry. A game is photographed with each.

### Step 12 — Blending

**Open.** Decide how coverage is blended.

- Text looks the same weight in a display space target and in an sRGB target.
- A style may adjust contrast for light text on a dark ground.
- Light and dark text are photographed in both targets.

### Step 13 — The ui on text objects

**Open.**

- `api/ui` measures, wraps and draws through text objects. The `Measure` and `Write` callbacks
  are removed.
- A text field places its caret, moves it by grapheme cluster, and draws its selection through
  its text object. An input method's composition is shaped and drawn in place.
- `wrap()` is replaced by layout.
- The editor and every demo move to the new path. Counters and frame rates use `tnum`.
- The ui suites cover the text field through kerned and right-to-left text. The editor and each
  demo are photographed.

### Step 14 — Colour glyphs

**Open.** Decide the colour glyph formats.

- A colour glyph is stored in an RGBA page and drawn in its own colours, with the text's alpha.
- An emoji font is in the fallback chain.
- The colour suite covers a COLR glyph and an emoji sequence shaped to one glyph.

### Step 15 — Variable fonts

**Open.**

- A style sets a variable font's axes, such as weight, by name. Shaping and the glyph store
  follow them.
- The font suite covers a weight between two named instances.

### Step 16 — Scripts covered by the shipped fonts

**Open.**

- The fallback chain ships faces for Arabic, Hebrew, Devanagari, Thai and CJK, under licences
  the tree can carry.
- The device suite compares a line of each script against a reference image.
