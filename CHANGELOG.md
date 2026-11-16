# Changelog

The changes in each release of Vertical3D that a user of the apps, or a project that uses the
api, would notice. The form is [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the
versions are [semantic versions](https://semver.org/spec/v2.0.0.html). While the major version is
0, a minor release may break the api, and a patch release never does.

A change adds its entry under Unreleased, in the same pull request as the code. A release renames
that section to its version, and the section becomes the release's notes.

## [Unreleased]

### Added

- moya's path tracer lights a scene by RI's standard and area lights, and by RenderMan's
  `PxrRectLight`, `PxrDiskLight`, `PxrSphereLight`, `PxrCylinderLight`, `PxrDistantLight`,
  `PxrDomeLight` and `PxrMeshLight` through the `Light` request, with cones and IES profiles.
  `Illuminate` links a light to the objects it lights.
- `PixelVariance` stops each pixel of a render once its noise falls below it.
- `moya --checkpoint <file>` writes a checkpoint after each pass, or at most once in the
  seconds `--checkpoint-interval` gives. `--resume` continues a stopped render from it, and gives
  the same picture as a render that never stopped.
- A render shades each surface by its material. RenderMan's `Pattern` and `Bxdf`
  name MaterialX nodes in RIB, and a `Bxdf` of type `materialx` reads a material from a
  MaterialX document. OpenPBR Surface, Standard Surface, UsdPreviewSurface and glTF PBR render,
  with MaterialX's pattern, image, normal map and bump nodes, and a material's displacement and
  opacity.
- RIB's `Sides` is read. A material that emits, such as RI's `constant`,
  emits from both faces of a two-sided surface, which RI's default is.
- `moya --maketexture` and RIB's `MakeTexture` make a tiled, mipmapped EXR texture from an
  image. A render makes any other image a material reads into one, and finds it along
  RIB's texture search path.
- A render writes outputs for compositing. `DisplayChannel` names a channel of a light
  path expression after `lpe:`, or of a value at the first hit such as `z`, `P` or `Nn`. A
  light's `lightGroup` gives an expression a group of lights to name. `PxrCryptomatte` writes
  Cryptomatte layers of objects and materials, and a `Display` of the type `deepexr` writes a deep
  EXR file. The beauty carries alpha.
- `moya --denoise`, and `Option "moya" "int denoise" [1]`, denoise the beauty through
  OIDN and write it beside the noisy one. A `DisplayChannel` whose `denoise` is 1 is denoised too.
- An RI `Displacement` reads `<name>.mtlx` on the shader search path, as a `Surface` does, and
  displaces a subdivision surface by the graph it defines.

### Changed

- `moya` renders a RIB file by path tracing, in place of the reyes and ray-trace hiders, and
  writes every file the scene's `Display` requests name, a `.png` or an EXR file. `--output`
  has to end in `.exr` or `.png`. A scene that names no `Display` writes its beauty to an EXR
  file named after the scene. `Hider`, `ShadingRate` and `Option "limits"` change nothing.
- `moya --width` without `--height`, or a side outside 1 to 65536, is refused, where the scene's
  `Format` was used.
- RIB is the api library `rib`, in `api/rib` and the namespace `v3d::rib`, where it was part of
  `render_offline` in `v3d::render::offline::rib`. A project links `v3d::rib`. A pixel filter's
  name and limits are `rib::Filter` and its neighbours, and its weight is moya's.

### Removed

- `moya --grid` and `--bucket`, which sized the reyes hider's grids and buckets.
- RI's C interface in libmoya, `RiBegin` and the other `Ri` entry points with their header
  `RenderMan.h`. A program builds moya's scene through the retained scene in C++, or writes RIB.
- The api library `render_offline`, with the shading language, the ray tracer, the old film, the
  textures and the old camera it held. RIB stays, as the library `rib`.

### Fixed

- The API reference describes each namespace, where it showed the copyright notice.

## [0.1.0] - 2026-11-06

### Added

- The editor, the offline renderer moya, imagetool and five demos (pong, tetris, voxel, odyssey
  and diorama), released together under one version.
- Every app answers `--version`, and the editor shows its version under Help > About.
- A Windows installer, with a Start menu entry for each windowed app and an uninstaller, and the
  same install tree as a zip. Both hold the notices of every library and font the apps ship.
- Each app's file properties on Windows show its name and version.
- A Flatpak for Linux holding every app, with a desktop entry for the editor and each demo.

### Changed

- Every app finds its data from the directory it is installed in, whatever the working directory.
  An app's own data is read from `data/<app>/`, before the shared `data/`.
- Every app writes `v3d.log` in its user directory, not beside the executable.
- The editor's default project is in a `Vertical3D` folder in the user's documents, and RIB export
  writes into the project's folder.
- `engine::appPath()` takes no argument.
