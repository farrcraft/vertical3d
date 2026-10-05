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
| [api/Types.md](api/Types.md) | Geometry, cameras, transforms, random numbers, animation and effect data |
| [api/Grid.md](api/Grid.md) | Tile grids: movement, line of sight, terrain maps |
| [api/Assets.md](api/Assets.md) | The asset manager and loaders, images, fonts, models and JSON documents |
| [../examples/README.md](../examples/README.md) | The example projects |

## Changing the libraries themselves

For anyone working inside a library rather than using it.

| Document | Covers |
|---|---|
| [internals/RealtimeRenderer.md](internals/RealtimeRenderer.md) | How `api/render/realtime` works: frames in flight, recording, memory, descriptor sets |
| [internals/UserInterface.md](internals/UserInterface.md) | How `api/ui` works: layout, drawing, hit testing and input routing |
| [OfflineRenderer.md](OfflineRenderer.md) | moya and `api/render/offline`: RIB, the hiders, the shading language, the film |

## The applications

| Document | Covers |
|---|---|
| [Editor.md](Editor.md) | The `vertical3d` editor and `api/brep` |
| [Games.md](Games.md) | pong, tetris, voxel and odyssey, plus imagetool and v3dshell |

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
