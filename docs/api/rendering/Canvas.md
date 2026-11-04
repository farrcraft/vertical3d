# The 2D canvas

`Canvas` collects 2D quads and text for one pass. It is what a game's sprites, a HUD and the
user interface draw onto. It is `realtime::canvas::Canvas`, in `api/render/realtime/canvas/`, beside
the world space canvases in [LinesAndWorldQuads.md](LinesAndWorldQuads.md).

## 2D: the quad canvas

Every 2D thing — a rectangle, a sprite, a glyph, a UI panel — is a textured quad. An app fills a
`Canvas` on the CPU and hands it to `quads()->submit(canvas, pass)`.

```cpp
Canvas canvas;
canvas.resize(glm::uvec2(width, height));    // the area drawn into, in pixels
canvas.clear();                  // at the start of every frame

canvas.rect(min, max, colour);                                  // untextured
canvas.rect(min, max, uv0, uv1, glm::vec4(1.0F), textureHandle); // textured; colour multiplies the texel
canvas.circle(centre, radius, 32, colour);
canvas.arc(centre, radius, 16, start, sweep, colour);           // a filled wedge
canvas.ring(centre, outer, inner, 16, start, sweep, colour);    // a band between two radii
canvas.segment({.from = a, .to = b, .width = 2.0F}, colour);  // a band between two points, at any angle
canvas.rect(min, max, {.topLeft = dark, .topRight = dark, .bottomRight = clear, .bottomLeft = clear});  // a blend
canvas.ring(shape, {.inner = dark, .outer = clear});           // a band blending from its inner radius out

canvas.push();
canvas.translate(glm::vec2(100.0F, 50.0F));
canvas.scale(glm::vec2(2.0F));
// ... drawn moved and scaled ...
canvas.pop();

renderer_->quads()->submit(canvas, pass.get());
```

- Coordinates are pixels with the origin at the top left and y down.
- Angles for `arc` and `ring` are radians. Zero points right and a quarter turn points down.
- A segment is one quad whose long sides run parallel to the line between its points, half the
  width either side. Its ends are square. It is untextured, so it joins the open untextured
  batch. A segment with no length or no width draws nothing. The transform stack has no
  rotation, so a segment is the way to draw a connector or a route at an angle.
- A segment given a `feather` smooths its long sides. Each side fades to nothing across the
  feather, centred on the side, so the band covers as much as a hard one of the same width. A
  line narrower than its feather has no solid core and fades from its middle. Its ends stay
  hard. A feathered segment is three quads across it.
- A rectangle given `Corners` takes one colour per corner and blends them. It is two
  triangles, so a blend along one axis is exact and four different colours show the diagonal
  where the triangles meet. A ring given a `Blend` blends from the inner radius to the outer,
  or from the centre to the rim when it has no hole. Together they draw a fading edge, such as
  a shadow's.
- An untextured quad is drawn against a 1×1 white texture, so it costs no extra draw.
- The canvas cuts a new draw only when the texture, the text flag or the clip changes. Group
  quads that share a texture to keep draws down.
- A canvas copies nothing to the GPU until `submit`. An empty canvas costs nothing.
- **Any number of canvases may be submitted in a frame**, into any passes. Each gets its own
  GPU buffers.
- A `Canvas` does not touch Vulkan, so it can be built and tested without a GPU.

### A canvas's own coordinate space

A canvas can draw in units of the game's own instead of pixels:

```cpp
canvas.space(glm::vec2(800.0F, 600.0F), Canvas::Fit::Contain);
```

- The space's origin is its top left and y grows down.
- `Fit::Stretch` fills the canvas and stretches the space where the aspect ratios differ.
- `Fit::Contain` keeps the space's aspect ratio, as large as fits, centred with empty bars on
  either side.
- `viewport()` returns where the space lands, in pixels. `toSpace(pixel)` converts a cursor
  position into the space's units.
- The space is kept across `clear()` and `resize()`. A size of zero goes back to pixels.

Each canvas carries its own projection, so a game in a space and a menu in pixels can be two
canvases in one pass. Pong draws its court and its menu this way.

### Clipping

```cpp
canvas.clip(glm::vec2(10.0F, 10.0F), glm::vec2(200.0F, 120.0F));
// ... everything here is cut to that rectangle ...
canvas.unclip();
```

- The rectangle is in the coordinates being drawn in. The current transform applies to it at
  the moment `clip()` is called, so translating afterwards moves the content, not the clip.
- A clip inside a clip is intersected with it, so an inner clip can only shrink the area.
- Clipping is done by the GPU per draw. A quad that crosses the edge is drawn whole and only
  the part inside appears.
- With a space set, the rectangle is in the space's units and is converted to pixels for you.
- The clip rectangle is placed relative to the image, not to the pass's viewport. A canvas
  drawn into a pass whose viewport is offset from the image's corner is clipped in the wrong
  place.

### Fading

```cpp
canvas.fade(0.5F);
// ... everything here is drawn at half its alpha ...
canvas.unfade();
```

- Each vertex's alpha is multiplied by the opacity as it is added, so text and textured quads
  fade with everything else.
- A fade inside a fade multiplies, and `unfade()` goes back to the opacity before the matching
  `fade()`. `clear()` drops any fade left open.
- Pieces that overlap under one fade are each faded, so where they overlap shows through more
  than either does alone. A shape drawn as pieces that never overlap fades evenly.

### Text

Text on the pixel grid is drawn as textured rectangles. `ui::paint::TextRenderer` lays it out
with `v3d::font` and places each glyph on whole pixels. `Canvas::glyphs(vertices, indices, page)`
copies the glyphs as quads sampling their coverage from a single-channel glyph page. The page's
view swizzles its channel into alpha, and the run joins pictures of the same page. `render` does
not depend on `font`.

`Canvas::curves(vertices, indices, curves, bands)` copies glyphs drawn from their quadratic curves,
as vertices and the triangles over them. Each vertex's `uv` is its place in em units, y upwards,
and its `glyph` is where its glyph's data starts among the bands. `curves` and `bands` are float
textures holding the curves and each glyph's bands, laid out as `api/render/shaders/curves.glsl`
describes. The quad shader finds each pixel's coverage from the curves that cross it, so the
glyphs stay sharp at any size and under any transform. A curve run never merges with one drawn
another way.

A vertex whose `glyph` has its high bit set names a colour glyph's program among the bands, as
`api/render/shaders/colour.glsl` describes, rather than a glyph. The program draws its layers
inside their clips, with their gradients and composites, in the colours the font gives. Its
colours are as they are displayed, as a quad's are. The vertex's colour gives its red, green
and blue to any layer in the text's colour, and its alpha fades the whole glyph. A world glyph
names a program the same way, and in an _SRGB target its colours are given in linear light.

`Canvas::text(vertices, indices, atlas)` copies text that has already been laid out against an
atlas of signed distances, as vertices and the triangles over them. A vertex whose `glyph` is 0
is a glyph: the quad shader turns the distance into a smooth edge about one pixel wide at any
scale. A vertex whose `glyph` is a ramp from `canvas::fieldRamp(whole, gone)` is an effect, such
as an outline, a shadow or a glow. The ramp holds the field value at which the effect covers the
whole pixel and the value at which it covers none of it. Coverage fades between the two, or rises
across one pixel when they are closer than that. Text and other quads that sample the same
atlas are drawn as separate batches.

### Text weight

A vertex's `contrast` marks it as text when it is 0 or more. It is -1 unless set, so nothing
else is touched. Text's coverage is changed in the shader in two steps:

- **Contrast.** Coverage `a` becomes `a + contrast * a * (1 - a)`, in every target. At 0 nothing
  changes, and at 1 half coverage becomes three quarters. Light text on a dark ground looks
  lighter than dark text on a light ground, and a style raises its contrast to make up for it.
- **The target.** A UNORM target blends in display space and keeps the coverage as it is. An
  _SRGB target blends in linear light, which would make dark text on a light ground thin and light
  text on a dark ground bold. There the coverage is changed so that the linear blend gives the
  colour a display space blend would, over a ground of the opposite luminance. Text then looks
  one weight in either target. Over a mid-tone or busy ground the match is close, not exact.

Coverage, curve and distance field glyphs are corrected alike, on the screen and in the world.

Background: [ADR-0147](../../adr/0147-text-coverage-corrected-to-blend-as-in-display-space.md)

### User interface

`api/ui` draws onto a `Canvas` too, so a game and its menus share the same uploads and draws.
How to build a UI is in [ui/](../ui/README.md).
