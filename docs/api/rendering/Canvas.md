# The 2D canvas

`Canvas` collects 2D quads and text for one pass. It is what a game's sprites, a HUD and the
user interface draw onto.

## 2D: the quad canvas

Every 2D thing — a rectangle, a sprite, a glyph, a UI panel — is a textured quad. An app fills a
`Canvas` on the CPU and hands it to `quads()->submit(canvas, pass)`.

```cpp
Canvas canvas;
canvas.resize(width, height);    // the area drawn into, in pixels
canvas.clear();                  // at the start of every frame

canvas.rect(min, max, colour);                                  // untextured
canvas.rect(min, max, uv0, uv1, glm::vec4(1.0f), textureHandle); // textured; colour multiplies the texel
canvas.circle(centre, radius, 32, colour);
canvas.arc(centre, radius, 16, start, sweep, colour);           // a filled wedge
canvas.ring(centre, outer, inner, 16, start, sweep, colour);    // a band between two radii

canvas.push();
canvas.translate(glm::vec2(100.0f, 50.0f));
canvas.scale(glm::vec2(2.0f));
// ... drawn moved and scaled ...
canvas.pop();

renderer_->quads()->submit(canvas, pass.get());
```

- Coordinates are pixels with the origin at the top left and y down.
- Angles for `arc` and `ring` are radians. Zero points right and a quarter turn points down.
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
canvas.space(glm::vec2(800.0f, 600.0f), Canvas::Fit::Contain);
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
canvas.clip(glm::vec2(10.0f, 10.0f), glm::vec2(200.0f, 120.0f));
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

### Text

`Canvas::text(textBuffer, atlas)` copies text that a `v3d::font` text buffer has already laid
out. The atlas is a single-channel texture of signed distances. The quad shader turns the
distance into a smooth edge about one pixel wide at any scale. Text and other quads that
sample the same atlas are drawn as separate batches.

### User interface

`api/ui` draws onto a `Canvas` too, so a game and its menus share the same uploads and draws.
How to build a UI is in [ui/](../ui/README.md).
