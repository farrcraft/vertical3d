# Lines and world quads

The two primitives that draw in world space, through the camera of the pass: one-pixel lines,
and textured quads with corners anywhere in the world.

## Lines

`canvas::Line` draws one-pixel lines in **world space**, through the camera of the pass. The
editor's grid, axes, wireframes and manipulators are lines.

```cpp
canvas::Line lines;
lines.clear();
lines.line(from, to, colour);
lines.polyline(points, colour, /* closed */ true);
lines.box(min, max, colour);                                 // twelve edges
lines.circle(centre, axisU, axisV, radius, 32, colour);      // axisU and axisV need not be unit or perpendicular

lines.push();
lines.transform(modelMatrix);   // or translate(offset)
// ...
lines.pop();

renderer_->lines()->submit(lines, pass.get());
```

- A pass with no camera set draws the lines in clip space.
- **In a pass with depth, lines test and write depth**, so solid geometry in front hides them.
  To draw lines over a scene instead, submit them to a pass without depth.
- Lines are always one pixel wide.
- `canvas::Line::clip()` takes a rectangle in the pixels of the image, not in world space. The
  transform does not apply to it.

## World quads and sprites

`canvas::World` draws textured quads with four corners in **world space**, through the pass camera.
Use it for sprites standing on a ground plane, particles and tile highlights.

```cpp
canvas::World world;
world.clear();
world.tint(glm::vec4(1.0F, 0.8F, 0.6F, 1.0F));   // multiplies every quad added after it
canvas::World::Corners corners = {a, b, c, d};  // in order around the edge
world.quad(corners, colour);
world.quad(corners, uv0, uv1, glm::vec4(1.0F), textureHandle);   // uv0 at the first corner, uv1 at the third

renderer_->worldQuads()->submit(world, pass.get(), 0, renderer::World::Blend::Alpha);
```

- Corners go in perimeter order, as `grid::tileCorners` returns them. Any convex quad comes out
  whole.
- `tint()` lasts until it is set again. `clear()` resets it to white.
- **There are two tints.** `canvas::World::tint()` is multiplied into each quad as it is added,
  so it suits a canvas built every frame, or part of one. `submit()` takes a tint as its last
  argument, white by default, which the vertex stage multiplies into every quad as the canvas
  is drawn. Use it to light a canvas kept across frames, such as a ground of thousands of tiles
  under the time of day, without building it again. Each submission keeps its own tint, so one
  canvas can be drawn twice in a frame in two lights. The two tints multiply together.
- `Blend::Alpha` draws straight alpha over what is there. `Blend::Additive` adds the colour
  scaled by alpha and only ever lightens, so flames and sparks give the same result in any order.
  A canvas is drawn with one blend, so use one canvas per blend.
- A pass with no camera set draws world quads in clip space.
- World quads are not clipped by a scissor rectangle and are not turned to face the camera.
  For a billboard, build the corners from the camera's right and up vectors.
- Neither face of a world quad is culled, so its winding never hides it.
- A world canvas and a line canvas in the same pass are not sorted against each other by
  depth. The `layer` argument decides which is drawn first.

### Depth order

**Quads draw in the order they were added.** What "further away" means is the game's decision:
in an isometric view a sprite is behind another when its feet are further up the ground plane,
not when it is further from the camera. In a pass with depth, world quads test depth but do not
write it. Solid geometry hides a world quad, and one world quad never hides another.

`DepthOrder` sorts quads for you by a key you compute:

```cpp
DepthOrder order;
order.clear();
order.quad(key, corners, uv0, uv1, colour, texture);   // larger key = further away
order.into(&world);                                     // furthest first
```

Equal keys are grouped by texture and otherwise keep the order they were added. Rounding the
key to a tile row means fewer texture changes and so fewer draws. A key that is not a number is
drawn after every key that is.

### How the three primitives treat depth

| Primitive | In a pass with depth |
|---|---|
| `Canvas` quads | Neither test nor write. A UI stays on top of whatever the scene drew. |
| `canvas::Line` lines | Test and write. |
| `canvas::World` quads | Test, but do not write. |
