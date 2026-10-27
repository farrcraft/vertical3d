# Documentation

The documents here describe the tree as it is now. Each one is written for one kind of reader,
so start from the section that matches what you are doing.

## Working in this repository

For anyone building, testing or changing the tree.

| Document | Covers |
|---|---|
| [contributing/GettingStarted.md](contributing/GettingStarted.md) | From a fresh clone to a running app and a passing test run |
| [contributing/Build.md](contributing/Build.md) | Configuring, the CMake layout and options, shaders, linking rules, and traps |
| [contributing/Dependencies.md](contributing/Dependencies.md) | vcpkg, the Vulkan SDK, libnoise, and adding or updating a package |
| [contributing/Testing.md](contributing/Testing.md) | Running and writing tests, the GPU tests, reference images, and checking a rendering change |
| [contributing/Linting.md](contributing/Linting.md) | cpplint, warnings as errors, `/analyze` and clang-tidy |
| [contributing/Review.md](contributing/Review.md) | What counts as a review finding, what a reviewer reads, and what a fix has to do |
| [contributing/Conventions.md](contributing/Conventions.md) | Code style, and how to write comments and documents |
| [../CONTRIBUTING.md](../CONTRIBUTING.md) | Where engine changes are made when you found the problem from another project |

## Writing an app against the api

For anyone using the libraries under `api/`, inside this tree or from another repository.

| Document | Covers |
|---|---|
| [api/README.md](api/README.md) | What each library is for, how they depend on each other, and a glossary |
| [api/UsingTheApi.md](api/UsingTheApi.md) | Starting a project in another repository and choosing libraries |
| [api/engine/](api/engine/README.md) | The app lifecycle, the game loop, input, config files, settings, logging and audio |
| [api/rendering/](api/rendering/README.md) | Drawing: frames and passes, 2D and world primitives, meshes, lighting, capture |
| [api/ui/](api/ui/README.md) | Building a UI from documents or in immediate mode, themes, focus and input |
| [api/ECS.md](api/ECS.md) | The entity registry, shared components, interpolation, animation and particles |
| [api/Types.md](api/Types.md) | Geometry queries, random numbers, checked conversions, flags and transforms |
| [api/Cameras.md](api/Cameras.md) | The camera profile, its matrices and handedness, and the isometric orbit |
| [api/Models.md](api/Models.md) | `model::Model`, skeletons, and loading glTF |
| [api/Animation.md](api/Animation.md) | Clips, poses, curves, tracks and particle effects |
| [api/Grid.md](api/Grid.md) | Tile grids: movement, line of sight, terrain maps |
| [api/Assets.md](api/Assets.md) | The asset manager and loaders, loading on a worker, JSON documents, and writing files |
| [api/Images.md](api/Images.md) | Images, the texture atlas, and packing a sprite sheet |
| [api/Fonts.md](api/Fonts.md) | Typefaces, glyph atlases and text layout |
| [api/Database.md](api/Database.md) | SQLite connections, statements and transactions |
| [../examples/README.md](../examples/README.md) | The example projects |

## Changing the libraries themselves

For anyone working inside a library rather than using it.

| Document | Covers |
|---|---|
| [internals/realtime/](internals/realtime/README.md) | How `api/render/realtime` works: frames in flight, recording, memory, descriptor sets |
| [internals/ui/](internals/ui/README.md) | How `api/ui` works: layout, drawing, hit testing, input routing and the immediate layer |
| [offline/](offline/README.md) | moya and `api/render/offline`: RIB, the hiders, the shading language, the film |

## The applications

| Document | Covers |
|---|---|
| [editor/](editor/README.md) | The `vertical3d` editor and `api/brep` |
| [Demos.md](Demos.md) | The five demos in `demos/`: pong, tetris, voxel, odyssey and diorama |
| [Imagetool.md](Imagetool.md) | `imagetool`, the command-line image tool |

## The project record

Why things are as they are, and what is planned. These are background reading, not a guide to
how the code works.

| Document | Covers |
|---|---|
| [sdlc.md](sdlc.md) | How work moves through the repository: plans, decisions, verification |
| [adr/](adr/README.md) | Architecture decision records: what was decided, what was rejected, and why |
| [plans/](plans/README.md) | Work spanning several phases, open and completed |
| [roadmap/](roadmap/README.md) | What an area nobody has started would need |
| [audits/](audits/README.md) | The only record of the deleted legacy trees |
| [TODO.md](TODO.md) | Open work that has no plan |
