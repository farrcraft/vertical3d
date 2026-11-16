# The moya manual

moya is an offline renderer. It reads a scene file in the RenderMan format, renders it by path
tracing, and writes the picture to an image file. It opens no window and needs no graphics card.
It shades each surface with a MaterialX material. RI's standard surfaces, such as `matte` and
`plastic`, are materials moya defines.

This manual is for someone who renders with moya. You write or export scene files, run moya, and
look at the image it writes. The manual does not describe the source code.
[docs/offline/](../../offline/README.md) covers that, for someone who changes moya.

## Terms

- **RI** (RenderMan Interface): the set of calls a RenderMan renderer accepts, such as `Polygon`
  and `Surface`.
- **RIB** (RenderMan Interface Bytestream): a text file that holds RI calls, one per line or
  more. moya reads a scene from a RIB file. Each call in it is a **request**.
- **MaterialX**: an open file format for materials. A MaterialX document is an XML file, usually
  ending in `.mtlx`, that holds nodes and the graphs that join them.

[HowMoyaRenders.md](HowMoyaRenders.md) defines the rest of the words this manual uses.

## Getting moya

moya comes with every release of Vertical3D, and [Install.md](../Install.md) says how to install
one. On Windows the installer puts `moya.exe` in `C:\Program Files\Vertical3D`. In the Flatpak,
moya runs with `flatpak run --command=moya com.farrcraft.Vertical3D`. moya runs from a command
prompt or a terminal, and has no Start menu entry or desktop entry.

moya is also built with the rest of the repository, into `out/build/<preset>/moya/`. A Debug
build renders slowly. A release, or a Release build, is faster.

## The pages

Start with the tutorial if you have not used moya before.

| Page | Type | Go to it to |
|---|---|---|
| [Tutorial.md](Tutorial.md) | Tutorial | Render a first scene, from the RIB file to the image |
| [Materials.md](Materials.md) | How-to | Give a surface a material: RI's surfaces, a surface of your own in MaterialX, and RenderMan's `Bxdf` |
| [Lights.md](Lights.md) | How-to | Light a scene with RI's lights, area lights and RenderMan's lights |
| [Displacement.md](Displacement.md) | How-to | Displace a subdivision surface with a MaterialX graph |
| [Camera.md](Camera.md) | How-to | Place the camera, and add depth of field and motion blur |
| [Curves.md](Curves.md) | How-to | Draw hair, grass and lines with curves |
| [Textures.md](Textures.md) | How-to | Put an image on a surface |
| [Output.md](Output.md) | How-to | Choose the output file, its format and its size, render part of the picture, and add channels |
| [Quality.md](Quality.md) | How-to | Trade speed for quality: samples, adaptive sampling, filters, bounces and denoising |
| [CommandLine.md](CommandLine.md) | Reference | Every option moya takes, and what it prints and logs |
| [Rib.md](Rib.md) | Reference | Each RIB request moya reads, and what it does with it |
| [HowMoyaRenders.md](HowMoyaRenders.md) | Explanation | How a RIB file becomes an image: the scene, the path tracer, the film and the outputs |
