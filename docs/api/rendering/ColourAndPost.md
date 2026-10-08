# Colour and post passes

The colour rules every app has to follow, and the full-screen passes that run after a scene is
drawn.

## Post passes and the colour grade

`vulkan::renderer::FullScreen` draws one triangle covering a target, with a fragment shader you
supply as SPIR-V. Use it for any pass over a whole image: a blur, a tone map, a vignette.

```cpp
vulkan::renderer::FullScreen::Spec spec;
spec.name = "vignette";
spec.fragment = vignetteSpirv;                // reads uv at location 0
spec.sources = 1;                             // images bound at set 1, bindings 0 upwards
spec.colour = swapchainFormat;                // what the pass draws into
vulkan::renderer::FullScreen post(device, cache, resources, ring, uniforms, spec);

MaterialHandle source = post.source({sceneTexture});   // once, and again after a resize
colour->reads(scene);
post.submit(source, colour.get());
```

- uv runs from 0 at the top left to 1 at the bottom right.
- The images are bound at set 1, one combined image sampler per binding. Set 0 is declared and
  need not be read.
- A source binds the images as they are now. After a target is resized, or a texture released,
  make a new source.
- Release every source with `post.release(source)` before the `FullScreen` is destroyed.

### Grade

`Grade` is a colour grade built on `FullScreen`: every pixel is looked up in a 16×16×16 colour
table.

```cpp
Grade grade(logger(), context, swapchainFormat, VK_FORMAT_UNDEFINED, strip);   // once
MaterialHandle source = grade.source(*scene);                                  // again after a resize

colour->reads(scene);
grade.submit(source, colour.get());
```

- The table comes from a 256×16 strip image: sixteen slices of blue side by side, red across each
  and green down it. No strip, or one of the wrong size, gives the identity table.
- The table is indexed by linear colour, so bake a strip for the linear light a lit scene holds.
- The scene is read one texel per pixel, so the grade must draw into a target the scene's size.
- `replace(texels)` changes the table from the next frame. Use it for a zone's own look, or lerp
  two tables yourself for a slow change. Call it before the frame's `submit()`. The texels are
  in the order `Grade::table()` returns. Sources keep their handles. Uploading the new table
  waits for the GPU queue to go idle, so a lerp that replaces the table every frame stalls every
  frame.
- `grade.release(source)` lets a source go. Destroying the grade releases its table and any
  source still held.

## Colour

**By default, colour is display space.** The swapchain uses a `UNORM` format, so the value a
shader writes is the value on screen. Every colour and texture in an unlit app is authored in
display space, and textures are uploaded as `UNORM` to match. Blending happens in display space.

**An app that writes linear light chooses the swapchain format.** Pass a format to the
`Engine3D` constructor:

```cpp
renderer_ = boost::make_shared<Engine3D>(logger(), assets(), VK_FORMAT_B8G8R8A8_SRGB);
```

- If the surface offers that format in a non-linear sRGB colour space, the swapchain uses it.
- Otherwise it falls back to the `UNORM` default and logs a warning.
- **Build pipelines against `renderer_->context()->colourFormat()`**, the format that was
  actually chosen, never against the format you asked for.
- **2D and ui colours stay display space on an `_SRGB` chain.** The quad renderer decodes a
  quad's colour to linear before an `_SRGB` target stores it, so the target's encode gives the
  authored colour back. A `#808080` quad shows as `#808080` on either kind of chain. Into any
  other format a quad is written as it is.

**A lit scene works in linear light and draws into an `_SRGB` target**, which encodes the result
when it is stored. Its albedo textures are uploaded with `TextureFactory::Encoding::Srgb`, so
they are decoded to linear when sampled. `MeshRegistry` does this for you. A glTF base colour
factor is already linear. Every other texture keeps the `Display` default.

Background: [ADR-0009](../../adr/0009-colour-display-space-unorm-swapchain.md),
[ADR-0066](../../adr/0066-lighting-light-in-linear-draw-to-srgb.md)
