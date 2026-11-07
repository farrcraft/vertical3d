# The moya manual

moya is an offline renderer. It reads a scene file in the RenderMan format, renders it, and writes
the picture to an image file. It opens no window and needs no graphics card. Its shaders are
written in the RenderMan Shading Language, and moya compiles them when a scene names them.

This manual is for someone who renders with moya. You write or export scene files and shaders,
run moya, and look at the image it writes. The manual does not describe the source code.
[docs/offline/](../../offline/README.md) covers that, for someone who changes moya.

## Terms

- **RI** (RenderMan Interface): the set of calls a RenderMan renderer accepts, such as `Polygon`
  and `Surface`.
- **RIB** (RenderMan Interface Bytestream): a text file that holds RI calls, one per line or
  more. moya reads a scene from a RIB file. Each call in it is a **request**.
- **Shading language (SL)**: RenderMan's language for shaders. A shader is a small program that
  works out the colour of a surface, the light a light source gives, or how a surface is moved.

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
| [Hiders.md](Hiders.md) | How-to | Choose between the reyes hider and the ray-trace hider |
| [Lights.md](Lights.md) | How-to | Light a scene with ambient, distant, point, spot and area lights, and cast shadows |
| [Shaders.md](Shaders.md) | How-to | Write a surface shader, put it where moya finds it, and pass it parameters |
| [Displacement.md](Displacement.md) | How-to | Move a surface with a displacement shader |
| [Camera.md](Camera.md) | How-to | Place the camera, and add depth of field and motion blur |
| [Curves.md](Curves.md) | How-to | Draw hair, grass and lines with curves |
| [Textures.md](Textures.md) | How-to | Put an image on a surface |
| [Output.md](Output.md) | How-to | Choose the output file, its format and its size, and render part of the picture |
| [Quality.md](Quality.md) | How-to | Trade speed for quality: pixel samples, filters, shading rate and trace depth |
| [CommandLine.md](CommandLine.md) | Reference | Every option moya takes, and what it prints and logs |
| [Rib.md](Rib.md) | Reference | Each RIB request moya reads, and what it does with it |
| [ShadingLanguage.md](ShadingLanguage.md) | Reference | The shading language as moya runs it: types, variables, functions and the built-in shaders |
| [HowMoyaRenders.md](HowMoyaRenders.md) | Explanation | How a RIB file becomes an image: hiders, buckets, grids and samples |
