# Put an image on a surface

A shader reads an image with the `texture()` function. The built-in `paintedplastic` shader
does this for you: it multiplies the surface colour by the image its `texturename` names.

## Use paintedplastic

```
Format 160 120 1
Projection "perspective" "fov" [40]
Option "searchpath" "texture" ["C:/renders/textures:&"]
WorldBegin
LightSource "ambientlight" 1 "intensity" [1]
Surface "paintedplastic" "Kd" [0] "Ks" [0] "texturename" ["blocks.png"]
Polygon "P" [-1 -1 5  1 -1 5  1 1 5  -1 1 5] "st" [0 1  1 1  1 0  0 0]
WorldEnd
```

The quad shows `blocks.png`, upright. With `Kd` and `Ks` at zero and one ambient light of
intensity 1, the colour is the image's own. Add lights and raise `Kd` to light it.

## Where moya looks for the image

- moya looks in each directory on `Option "searchpath" "texture"`, then at the name as given.
  The path is written as the shader path is, as
  [Shaders.md](Shaders.md#2-tell-moya-where-the-shader-is) describes.
- A relative name is read from moya's own directory. Give a search path or a full path.
- The image can be a PNG, JPEG, TGA or BMP file. moya chooses the reader by the file's extension.
- `MakeTexture` does nothing. Name the image itself, not a converted texture file.
- An image moya cannot read is logged once, and reads as black.

## Map the image with st

The image is placed on a surface by its `s` and `t` coordinates. `s` runs across the image from
0 on the left to 1 on the right. `t` runs down it, from 0 at the top to 1 at the bottom.

Give a polygon `"st"`, two numbers for each corner. The example puts `(0, 0)`, the image's
upper left corner, on the quad's upper left corner. A coordinate outside 0 to 1 repeats the
image.

A polygon without `"st"` has coordinates of its own, which do not run across it as one image.
Under the reyes hider each piece a large polygon is split into runs from 0 to 1 by itself.

## Read a texture in your own shader

```
color c = texture("blocks.png");            /* the colour at s and t */
color d = texture("blocks.png", s * 4, t);  /* repeat four times across */
float g = float texture("blocks.png");      /* the first channel */
```

- `texture(name)` reads at the point's `s` and `t`. `texture(name, s, t)` reads at the
  coordinates you give.
- The result is a colour. A `float` cast reads the first channel only.
- moya blends the four texels nearest the point. It does not average the image over the area
  a pixel covers, so a texture far smaller on screen than its size can shimmer. Raise
  `PixelSamples` to smooth it ([Quality.md](Quality.md)).
- A grey image fills all three channels. An alpha channel is not read.
