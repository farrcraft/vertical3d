# Documentation

Everything here describes the tree as it stands. Dated narrative belongs in git, in the ADRs
and in the plans.

## Reference

What the tree is, and how to work in it. Each document is the single home for its subject: if
two of them say the same thing, one of them is wrong.

| | |
|---|---|
| [Architecture.md](Architecture.md) | The map: the libraries, the two Engines, the app shell, geometry, and the invariants that cross everything |
| [Build.md](Build.md) | Configuring and building, the CMake layout and options, shaders, linking rules, build traps |
| [Dependencies.md](Dependencies.md) | vcpkg and the manifest, the Vulkan SDK, the libnoise submodule, adding or updating a package |
| [Testing.md](Testing.md) | The Boost.Test suites, what is covered and what cannot be, and how a rendering change is verified |
| [Linting.md](Linting.md) | cpplint, `/WX`, `/analyze` and clang-tidy — the four gates the tree is clean at |
| [Conventions.md](Conventions.md) | House style: file layout, namespaces, pointers, logging, and what a comment is for |
| [RenderingPipeline.md](RenderingPipeline.md) | The realtime renderer from window to draw item, and what is not built yet |
| [OfflineRenderers.md](OfflineRenderers.md) | talyn and moya, `api/render/offline`, and RIB |
| [Editor.md](Editor.md) | `vertical3d/` — its layout, its meshes, and the records that settle it |
| [ECSDesign.md](ECSDesign.md) | entt notes. Still mostly open questions |
| [NewProject.md](NewProject.md) | Starting an application against the api from another repository |

## The record

Why things are as they are, what is planned, and what is loose.

| | |
|---|---|
| [sdlc.md](sdlc.md) | How work moves through the repo — plan, decide, build, verify, record |
| [adr/](adr/) | Architecture decision records, indexed in [adr/README.md](adr/README.md) |
| [plans/](plans/) | Phased workstreams. [OfflineRenderingPhase3.md](plans/OfflineRenderingPhase3.md) is open |
| [roadmap/](roadmap/) | What an area nobody has taken up would need, and in what order |
| [audits/](audits/) | The only account of the four legacy trees, and how to recover a file from each |
| [TODO.md](TODO.md) | Loose ends, and the clang-tidy backlog |

[../CLAUDE.md](../CLAUDE.md) is orientation for a coding agent. It routes into these documents
rather than repeating them.
