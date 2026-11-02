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
string index, the index under a point, and the rectangles of a selection. A renderer draws it.

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
| Text: how glyphs are drawn. Decided by ADR-0145, amending ADR-0036: coverage on the pixel grid, curves when scaled. | 9, 10 |
| Text: how effects are drawn. Decided by ADR-0146, amending ADR-0145: a distance field kept for them. | 11 |
| Text: the glyph atlas. Decided by ADR-0144: pages of plots, the plot used longest ago reused first. | 8 |
| Text: the shaping library. Decided by ADR-0141: HarfBuzz. | 3 |
| Text: the bidi and segmentation libraries. Decided by ADR-0142: ICU, with its data filtered. | 4, 5 |
| Text: the text object the ui and the games hold, and how it is cached. Decided by ADR-0143: a paragraph with style ranges over a shared shaping cache. | 6 |
| Text: how coverage is blended. Decided by ADR-0147: corrected to blend as in display space, with a contrast curve. | 12 |
| Text: which colour glyph formats are read. Decided by ADR-0149, amending ADR-0145: all five, COLR and SVG drawn both ways. | 14 |
| Text: how SVG glyphs are read. Decided by ADR-0150: ThorVG. | 14 |

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

**Closed.**

- `font::Library` is one FreeType library. `font::Face` holds a font file open, with its family,
  style, OS/2 weight and italic flag, and finds a code point's glyph index.
- `font::Family` chooses a face by weight and style as CSS does. `font::FallbackChain` tries its
  families in order, and a code point none has is the first face's missing glyph.
- `TextureFont` draws one face. It keeps glyphs by their index in the face, so code points drawn
  with one glyph pack it once. Its fallback is a chain of fonts, and a code point no font down
  it has is its own face's missing glyph.
- `ui::paint::TextRenderer` opens its faces through one library into a chain of two families.
  Each weight's fonts follow the chain, with one font for each face, so both weights share the
  fallback.
- A face's underline is read in its design units, so its distance below the baseline matches
  the face.
- The fallback chain suite covers a face's names, weight and glyphs, a face that will not open,
  CSS's choice of weight, the chain's order, and a code point no face has. The font suite
  covers code points sharing one glyph.

### Step 3 — Shaping

**Closed.** ADR-0141 decided it, after weighing kb_text_shape, FreeType's GPOS kerning and the
platforms' shapers.

- `font::Shaper` shapes a string through HarfBuzz over a fallback chain's faces. It splits the
  string into runs where the face or the script changes, keeping a space or a mark in the run
  before it. Each run's direction is its script's, until step 5 resolves embedding levels.
- A `font::ShapedRun` holds its face, script, direction, byte range and glyphs in visual order.
  A `font::ShapedGlyph` holds its index, advance, offset and source cluster.
- A `font::Feature` turns an OpenType feature on or off across a string.
- Both manifests name HarfBuzz with its FreeType feature. A face holds a HarfBuzz face over its
  FreeType face.
- Shaped text is not drawn yet. A substitution gives a glyph index no code point names, and the
  glyph store of step 8 packs glyphs by index.
- The shaper suite covers a kerned pair, tabular figures, a mark on its base, contextual
  alternates, runs split by face and script, and the sizes that shape nothing. The tree ships no
  Arabic face, so an Arabic word joined is checked in step 16 with the script faces.

### Step 4 — Unicode text boundaries

**Closed.** ADR-0142 decided it with step 5's library, after weighing SheenBidi with
libunibreak, Raqm and ICU4X.

- `font::Segmenter` splits UTF-8 text into grapheme clusters, words and line break
  opportunities through ICU, as UAX #29 and UAX #14 define them, in a locale given as a BCP 47
  tag. Every offset is a byte offset, because ICU reads the UTF-8 in place.
- Thai, Lao, Khmer and Myanmar lines break between the words ICU's dictionaries find.
- ICU comes from an overlay of the vcpkg port in `ports/icu/`, which builds its data from source
  filtered to the break rules and the properties they read, the dictionaries and
  normalization. Both manifests name it, and
  CI's vcpkg cache keys hash the overlay.
- The segmentation suite covers an accented letter, an emoji sequence and a flag as one cluster.
  It covers a break after a hyphen, no break before a closing bracket, a mandatory break after a
  newline, and Thai words split by the dictionary. It also covers words told from punctuation,
  and malformed and empty strings.

### Step 5 — Bidirectional text

**Closed.** ADR-0142 decided its library: ICU, with step 4's.

- `font::Bidi` resolves a text's embedding levels through ICU, as UAX #9 defines them, with each
  paragraph read in a base direction given or found from its first strong character.
- `line()` reorders one line's runs from logical to visual order, with trailing space at the
  paragraph's level.
- `caret()` takes an affinity, upstream or downstream, and gives the character and the edge the
  caret is drawn against. Step 6 turns that edge into a position, and step 13 sets the affinity
  from the way the caret moved.
- The shaper splits runs where the level changes, and a run's direction is its level's. A
  `ShapedRun` holds its level, by which step 6 orders a line's runs.
- The bidi suite covers Hebrew in an English sentence, a number in an Arabic sentence, the caret
  either side of each boundary, trailing space in a paragraph read right to left, and
  paragraphs resolved apart. The shaper suite covers a number in Arabic shaped as its own run.

### Step 6 — The text object and layout

**Closed.** ADR-0143 decided it, after weighing an immutable layout, a paragraph with no
shared cache, a cache with no object, and one style for each text.

- `font::Paragraph` holds a text with `TextStyle`s over its ranges and a `ParagraphStyle`. It
  keeps its shaping, so a new width, alignment or line limit only lays it out again, and a new
  colour or decoration does neither.
- `font::ShapingCache` shapes texts for every paragraph and keeps the 256 used most recently.
  It finds them by string, direction and the ranges as they shape, so immediate mode's text
  built again each frame is found.
- The shaper shapes one range of a string with the levels of the whole, and the rest as context.
- Lines break at step 4's opportunities, and a word wider than the width breaks at a cluster.
  Space at a line's end hangs. A line is aligned to its start, its end, the left, the right or
  the centre. A line limit cuts the last line shown and ends it in an ellipsis.
- `Face::metrics()` reads the ascender, descender, line gap, underline and strikethrough
  through HarfBuzz, honouring a font's choice of typographic metrics. A line is as tall as its
  tallest faces, times the line spacing.
- A paragraph answers its size, its lines and runs in drawing order, a caret for an index and
  an affinity, the hit under a point, and the rectangles of a selection, each by cluster. A
  ligature is shared between its clusters evenly.
- The font suite frees ICU's caches once its cases have run, so the debug runtime reports no
  leaks.
- The paragraph suite covers wrapping at words and at clusters, each alignment, and the
  ellipsis. It covers the caret through kerned and reordered text, a hit on each half of a glyph,
  and a selection and a caret across a wrap. It also covers empty lines, and shaping kept across
  a new width, a new colour and a second paragraph. It covers styles split and joined, and space
  hanging left in text read right to left. It covers a line limit with no ellipsis, a limit that
  hides only an empty line, a new direction shaping again, and the cache letting old text go.

### Step 7 — Partial texture updates

**Closed.**

- `Textures::update()` copies pixels into a region of a texture that frames may be drawing,
  and the texture keeps its slot. The copy waits on the device for the fragment shaders of the
  frames submitted before it, through a barrier out of `SHADER_READ_ONLY_OPTIMAL` that keeps the
  rest of the texture. The host never waits.
- It refuses the white texture, a render target's image, a request still waiting, a released
  handle, a region outside the texture, and pixels with other channels.
- The device suite covers a region updated while a frame drawing the texture is in flight: that
  frame draws the old texels and the next draws the new region beside the kept rest. It also
  covers each update that is refused.

### Step 8 — The glyph store

**Closed.** ADR-0144 decided the atlas, after weighing one atlas cleared when full, pages that
only grow, and evicting single glyphs.

- `font::GlyphStore` makes a glyph the first time it is drawn, keyed by face, glyph index,
  size, sub-pixel step, mode and variable font coordinates, and packs it into pages of plots.
- A full store adds a page up to its limit, then empties the plot used longest ago that the
  frame being built does not use. When that frame uses every plot, a page is added past the
  limit and let go once a frame uses nothing in it, so no text stops drawing for want of room.
- `ui::paint::GlyphPages` keeps a texture for each page, and copies each changed plot into it
  through step 7's updates.
- The charset belonged to the old atlas the ui drew from, and went with it in step 9.
- The glyph store suite covers a glyph made once, a full store emptying the plot used longest
  ago, a glyph drawn this frame kept, changed plots given once, and glyphs that cannot fit. The
  glyph pages suite covers a page made and its plots copied, and a page let go released.

### Step 9 — Coverage text

**Closed.** ADR-0145 decided how glyphs are drawn, amending ADR-0036, after weighing coverage
beside a multi-channel distance field, all three, and distance fields for all text.

- `font::rasterize()` rasterizes a glyph's coverage at its device size, hinted by its face
  vertically and not horizontally, with its pen moved by a quarter pixel step.
- `ui::paint::TextRenderer` lays text out as a `font::Paragraph` and draws each glyph from a
  `font::GlyphStore`, its pen rounded down to a quarter pixel and its line's baseline to a
  whole pixel. Its pages reach the device through `GlyphPages`. `Screen` begins the store's
  frame each frame.
- A change of scale draws the glyphs at the new size, since each size is its own glyph.
- A coverage glyph is a one channel mask whose view puts it in alpha, so the quad shader's
  ordinary textured path draws it, and needs no branch of its own.
- The charset option is gone, with the atlas it filled: any character the faces have is drawn.
  The ui no longer links the image library.
- The text renderer suite covers any character drawn, glyphs on whole pixels at each step, and
  a larger size rasterized at that size. The editor was photographed at 12 pixels and at a scale
  of 1.5, and magnified: stems are one pixel and words sit on one baseline. The validation layer
  was silent in the editor and in pong.

### Step 10 — Curve text

**Closed.** ADR-0145 decided it with step 9.

- `font::outline()` reads a glyph's outline as quadratic curves in em units and bands them, as
  the Slug algorithm reads them. `font::CurveStore` keeps them, the same at every size, in two
  float arrays.
- Float textures hold the arrays on the device, and `ui::paint::CurvePages` keeps them in step.
  The quad and world shaders find each pixel's coverage from the curves crossing it, through
  `curves.glsl`, at any size and angle.
- `Canvas::curves()` draws glyphs on the screen, and `canvas::World::glyph()` and
  `canvas::DepthOrder::glyph()` stand them in the world and in depth order.
  `ui::paint::CurveText` draws a paragraph through any 2D transform, or into the world.
- Antialiased text is not output the Vulkan specification fixes, so by ADR-0054 it has no
  reference image. The device suite checks chosen pixels of a square drawn from curves, upright,
  turned, in the world through a camera, and sorted by depth. A word drawn at 14 pixels and
  turned at three times that was looked at.
- The curve suite covers an outline enclosing its strokes, closed contours, sorted bands and the
  band transform. The curve store suite covers a glyph read once, stored as the shader reads it,
  and changed rows given once. The ui suite covers glyph quads, curves reaching the device once,
  a transform placing them, and a paragraph standing in the world.
- Diorama stands a sign over its well, drawn from its curves and facing the camera, and was
  photographed. The validation layer was silent.

### Step 11 — Effects

**Closed.** ADR-0146 decided it, amending ADR-0145, after weighing effects from the curves and
from a stroked glyph.

- A `font::TextEffect` grows each glyph by a width, fades it over a softness, and moves it by an
  offset, in a colour. A `TextStyle` lists its effects, drawn beneath its text in order.
- `font::field()` makes a glyph's signed distance field through FreeType, at 64 pixels to the em
  with a spread fitted to the effect, for the glyph store. One field serves every size.
- An effect is a quad over its field whose vertices carry a ramp, the field values at which it
  is whole and at which it is gone. The quad and world shaders fade between them through
  `field.glsl`. `canvas::World::field()` and `canvas::DepthOrder::field()` stand effects in the
  world, and depth order puts them beneath their glyphs.
- `ui::paint::TextRenderer` draws a paragraph with its effects beneath it, from the store it
  draws coverage from. `ui::paint::CurveText` keeps a store of fields with `GlyphPages`, and
  draws effects on the screen, in the world and in depth order.
- The field suite covers a field reaching its spread past the outline, its values across an edge,
  the spread an effect needs, and each effect's ramp. The ui suites cover effect quads beneath
  the glyphs, layered in order and moved by their offset, in the world and in depth order, and a
  field made once. The render suites cover a field batch and fields beneath glyphs in depth
  order. The device suite checks chosen pixels of a field drawn with no ramp, a soft ramp and a
  hard one, on the screen and in the world.
- Diorama's sign has an outline and a soft shadow, and was photographed with those and again with
  a glow. The validation layer was silent.

### Step 12 — Blending

**Closed.** ADR-0147 decided it, after weighing linear blending with stem darkening.

- A text vertex carries a contrast, -1 for anything that is not text. `text.glsl` strengthens
  text's coverage by it, and in an _SRGB target changes the coverage so the linear blend gives
  the display space colour over a ground of the opposite luminance. The quad and world shaders
  apply it to coverage, curve and distance field glyphs. `world.frag` has an _SRGB variant, as
  `quad.frag` has.
- `Canvas::glyphs()` draws coverage glyphs with their contrasts. `font::TextStyle` names a
  contrast, which `TextRenderer` and `CurveText` give each glyph. Effects are corrected with
  none.
- The device suite stores half covered text in a UNORM and an _SRGB target and finds one value,
  on the screen and in the world. It finds a picture uncorrected, and contrast lifting half
  coverage to three quarters. The render and ui suites cover the contrast reaching each vertex.
- Diorama's light sign and a dark one were photographed in its _SRGB scene, and the editor's
  light text in its UNORM swapchain. The validation layer was silent.

### Step 13 — The ui on text objects

**Closed.** ADR-0148 decided how the ui reaches text, after weighing an abstract text interface
and callbacks that return paragraphs.

- `paint::Type` is a text renderer at one size and weight. The retained components, the
  arranger, the menus and the editor measure, wrap and draw through one, and the immediate layer
  through the screen's `TextRenderer`. The `Measure` and `Write` callbacks are gone.
- `wrap()` is gone. A label's rows, and the immediate layer's wrapped text, break as a paragraph
  lays them out, through `TextRenderer::rows()`, so a word wider than its row breaks at the edge.
- `LineEdit` steps, erases and stands its caret at grapheme cluster edges, and keeps which
  character the caret goes with from the way it moved. Its `layout()` lays the line out as a
  paragraph and gives the caret, the selection's runs and the composition. The text box and the
  immediate field draw and hit test through it, exactly through kerned and right to left text.
- An input method's composition reaches the focused field from SDL's text editing events, and
  is drawn at the caret, underlined, until it is typed in.
- `Type::tabular()` sets the OpenType feature `tnum`. The editor's frame and frame rate, the
  statistics overlay and a hud's lines are set in it.
- `TextRenderer::trace()` tells a test each line drawn. The ui and editor suites draw through a
  monospaced fixture face at ten pixels a character in `api/ui/tests/fonts/`, beside a Hebrew
  face.
- The line edit suite covers clusters, the caret's affinity, a composition, kerning and text
  read right to left. The text box and field suites cover a composition drawn in place. The
  editor and every demo were photographed, and pong's pause menu with its emphasis. The
  validation layer was silent in each.

### Step 14 — Colour glyphs

**Closed.** ADR-0149 decided the formats and ADR-0150 the SVG reader, after weighing COLR alone,
COLR with bitmaps, bitmaps alone, plutosvg, lunasvg and nanosvg.

- A face reports its colour tables, and says whether a glyph is in colour. `font::colour()`
  walks a glyph through HarfBuzz's paint interface into one layered form. A fill is drawn inside
  its clips in a solid colour, the text's colour, or a linear, radial or sweep gradient, and a
  group is put down by a composite. A CBDT or sbix glyph is a picture from its nearest strike,
  put on the strike's own pixels.
- An SVG glyph's document is parsed by ThorVG, once for each face, with the em square as its
  first viewport. Its element is walked into the same form, a stroke outlined by FreeType's
  stroker. ThorVG joins both manifests, found by name.
- `font::paint()` paints the form at a device size into four channel glyph pages, with every
  COLR composite. The text renderer draws such a glyph as a picture in its own colours, faded by
  the text's alpha.
- The curve store keeps a colour glyph as a program of fills, clips, gradients and groups among
  its band values. The quad and world shaders run it through `colour.glsl`, so curve text draws
  a colour glyph sharp when scaled or in the world. A bitmap glyph there is a scaled picture.
- Noto Color Emoji's COLR version 1 build is the text renderer's emoji face, last in its chain.
  The shaper chooses a face a grapheme cluster at a time, so an emoji sequence or a keycap is
  one glyph, and a space after an emoji is set in the text's face.
- The colour suite covers a glyph from each table, over fixtures written by
  `api/font/tests/fonts/colourfonts.ts`. It covers palettes, the text's colour, each gradient, a
  transform, a stroke and the even-odd rule. It also covers a strike scaled, a stored program, a
  document parsed once, and emoji sequences shaped to one glyph. The device suite checks chosen
  pixels of a program's gradient and its layer in the text's colour. Noto's emoji test builds in
  COLR version 1, SVG and sbix were painted alike. Pong's menu and Diorama's sign were photographed
  with emoji, and the validation layer was silent.

### Step 15 — Variable fonts

**Open.**

- A style sets a variable font's axes, such as weight, by name. Shaping and the glyph store
  follow them.
- The font suite covers a weight between two named instances.

### Step 16 — Scripts covered by the shipped fonts

**Open.**

- The fallback chain ships faces for Arabic, Hebrew, Devanagari, Thai and CJK, under licences
  the tree can carry.
- The shaping suite covers an Arabic word joined and a Devanagari syllable reordered.
- The device suite compares a line of each script against a reference image.
