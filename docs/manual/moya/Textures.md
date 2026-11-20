# Put an image on a surface

RI's `paintedplastic` surface multiplies the surface colour by an image. A material of MaterialX
nodes reads an image through MaterialX's `image` node. Either way, moya turns the image into a
texture file the first time a render reads it.

## Use paintedplastic

```
Format 160 120 1
PixelSamples 8 8
Projection "perspective" "fov" [40]
Option "searchpath" "texture" ["C:/renders/textures:&"]
WorldBegin
LightSource "ambientlight" 1 "intensity" [1]
Surface "paintedplastic" "Kd" [1] "Ks" [0] "texturename" ["blocks.png"]
Polygon "P" [-1 -1 5  1 -1 5  1 1 5  -1 1 5] "st" [0 1  1 1  1 0  0 0]
WorldEnd
```

The quad shows `blocks.png`, upright. With `Kd` at 1, `Ks` at zero and one ambient light of
intensity 1, the colour is the image's own. Add lights to light it, and raise `Ks` for a shine.

## Map the image with st

The image is placed on a surface by its `s` and `t` coordinates. `s` runs across the image from
0 on the left to 1 on the right. For `paintedplastic`, `t` runs down it, from 0 at the top to 1
at the bottom, as RI places it.

- **Give a polygon `"st"`**, two numbers for each corner. The example puts `(0, 0)`, the image's
  upper left corner, on the quad's upper left corner.
- **A coordinate outside 0 to 1 repeats the image.**
- **A surface without `"st"` reads one point of the image.** Every sphere and other quadric is
  such a surface, so the image does not show on it.
- **MaterialX's `image` node runs `t` up the image**, from 0 at the bottom. A network that reads
  the coordinates through a `texcoord` node therefore gives `"st"` with `t` at 0 along the
  bottom edge. Its `uaddressmode` and `vaddressmode` choose what lies beyond the edge:
  `constant`, `clamp`, `periodic` or `mirror`.

## Where moya looks for the image

- **A full path is read as given.** Write `/` between directories.
- **A relative name is looked for in each directory on `Option "searchpath" "texture"`**, in
  order. The path is written as the shader search path is, as
  [Materials.md](Materials.md#1-tell-moya-where-the-documents-are) describes. Without a texture
  search path, a relative name is read from the folder you run moya from.
- **The image can be an EXR, PNG, JPEG, TGA or BMP file.**
- **An image moya cannot find or read is logged**, and the surface reads it as white for
  `paintedplastic`, or as the `image` node's default.
- **A name with `<UDIM>` in it** reads the tiles of a UDIM set, from 1001 up.

## Colour and data

moya converts a colour image into its working colour space, ACEScg, before it uses it.

- **A PNG, JPEG, TGA or BMP image is read as sRGB.**
- **An EXR image is read in the space its chromaticities name**, or as linear Rec. 709 when it
  names none.
- **An image that holds data**, such as heights or normals, is read as it is when the `image`
  node is not a colour, or when its `file` input names the colour space `Raw`.

## Make a texture yourself

A render makes each image into a tiled, mipmapped EXR texture, and keeps it in a folder named
`moya-textures` in the system's temporary directory. A later render of the same image reads it
from there. To make a texture yourself, with its own settings, run moya with `--maketexture`:

```
moya.exe --maketexture blocks.png blocks.exr --swrap periodic --twrap periodic
```

`MakeTexture` in a RIB file does the same, at the point the request is read:

```
MakeTexture "blocks.png" "blocks.exr" "periodic" "periodic" "box" 1 1
```

- Name the texture file, such as `blocks.exr`, where the scene named the image. moya reads a
  texture as it is.
- [CommandLine.md](CommandLine.md#options) lists the options of `--maketexture`.
- `"string colorspace"` on `MakeTexture` names the image's colour space, as `--colorspace` does.

## How a texture is filtered

moya reads a texture over the area of the surface that one sample covers, from the level of
the mipmap that fits it. A texture that is small on the screen is therefore smooth, and does not
shimmer. A surface seen at a glancing angle is read along its length at up to eight points.
