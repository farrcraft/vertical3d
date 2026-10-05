# CLAUDE.md

Orientation for coding agents. It routes into [docs/](docs/) rather than repeating what is
there, because a fact recorded in two places will eventually disagree with itself. Read the
document that owns a subject before working on it.

## What this is

A monorepo for the Vertical3D ecosystem: reusable C++ libraries under `api/` (targets named
`v3dlib_*`, one per subdirectory) plus the apps that consume them at the top level — pong,
tetris, voxel, odyssey, vertical3d, moya, imagetool, v3dshell. Every one of those
directories builds with the tree. [examples/](examples/) is the exception: it consumes the
repository rather than being part of it.

Much of this code traces back to the early 2000s and is being modernised incrementally: C++17
and newer, granular namespaces, CMake replacing autotools and VS solutions. How modern a given
file is varies widely. MSVC and Windows only in practice; rendering is Vulkan 1.3 and windowing
is SDL3.

## Where things are written down

| Working on | Read |
|---|---|
| Anything at all | [docs/Architecture.md](docs/Architecture.md) — the map, and the invariants that bite |
| Building, CMake, linking a target | [docs/Build.md](docs/contributing/Build.md) |
| A dependency, vcpkg, the Vulkan SDK | [docs/Dependencies.md](docs/contributing/Dependencies.md) |
| Tests, or verifying a rendering change | [docs/Testing.md](docs/contributing/Testing.md) |
| A lint or analyser finding | [docs/Linting.md](docs/contributing/Linting.md) |
| Style, and what a comment is for | [docs/Conventions.md](docs/contributing/Conventions.md) |
| `api/render/realtime` | [docs/RenderingPipeline.md](docs/api/Rendering.md) |
| `moya`, `api/render/offline`, RIB | [docs/OfflineRenderers.md](docs/OfflineRenderer.md) |
| `api/ui` | [docs/UserInterface.md](docs/api/UserInterface.md) |
| `vertical3d/`, `api/brep` | [docs/Editor.md](docs/Editor.md) |
| Why something is shaped as it is | [docs/adr/](docs/adr/), indexed in its README |
| What is planned, and what is loose | [docs/plans/](docs/plans/), [docs/TODO.md](docs/TODO.md) |

[docs/README.md](docs/README.md) is the full index. [docs/sdlc.md](docs/sdlc.md) describes how
work moves through the repo: where plans live, when a decision earns an ADR, and what
"verified" means.

The legacy trees — `vault/`, `rigel/`, `luxa/`, `v3dlibs/` — are deleted, and
[docs/audits/](docs/audits/) is the only account of what they held. Read
[RigelSurvey.md](docs/audits/completed/RigelSurvey.md) before writing off a rigel feature as
covered.

## The commands

```
ninja -C out/build/x64-Debug                       # build; one target by name
ctest --test-dir out/build/x64-Debug --output-on-failure
cpplint --linelength=180 --exclude=out --exclude=vendor --exclude=vcpkg_installed \
  --exclude=voxel/src/noise --recursive .
```

Neither `ninja` nor `ctest` is on a plain shell's PATH: those two lines need a developer
environment, and `scripts\build.cmd` and `scripts\test.cmd` are the same commands with one
entered. [docs/Build.md](docs/contributing/Build.md) covers configuring from a cold tree, the options, and
what has to be installed first. The tree is clean at cpplint, at `/W4` with `/WX`, at
`/analyze` and at the 186 clang-tidy checks left enabled, so **every finding is a new one**.

## Rules that cost the most when broken

- **Never delete `out/build/<config>/vcpkg_installed/`.** That directory is the dependency
  install, and a cold reinstall takes 45 minutes.
  [docs/Build.md](docs/contributing/Build.md#traps) says what to delete instead when a cache has to be
  reset.
- **An api library carries its own dependencies, including the other api libraries it uses.**
  An app names the `v3dlib_*` targets it uses and nothing else. There is no OpenGL in the tree,
  and glm and EnTT have to be linked rather than assumed.
  [docs/Build.md](docs/contributing/Build.md#linking-rules).
- **Simulation goes in `simulate(float step)`, not `tick(unsigned int delta)`.** The loop calls
  both. Nothing enforces the split, so simulation left in `tick()` compiles and then runs at a
  speed that depends on the frame rate. `tick` takes milliseconds and `simulate` takes seconds.
  [docs/api/Engine.md](docs/api/Engine.md) has the rest of the loop.
- **An app starts up in `start()` and tears down in `release()`.** The engine calls both, in an
  order it controls. Anything that presents to the window, a renderer above all, is released
  in `release()`. A quit command calls `Engine::quit()`. Only `run<T>` calls `shutdown()`, and an
  app cannot reach it.
- **The shell around a game belongs to the api.** `main`, the game menu, the text renderer and
  the minimized-window check are all provided. An app that writes its own has diverged from the
  others rather than customised them.
- **Comments and documents stand on their own.** A comment states the rule; it never cites an
  ADR, a document or a plan. A reference document states the rule in full, and may add an ADR
  only as a `Background:` link. Write plainly: short sentences, no aphorisms, no history.
  [docs/contributing/Conventions.md](docs/contributing/Conventions.md#writing) has the rules,
  and is worth re-reading against the comments a change added.

## Style in one screen

Use `boost::shared_ptr` and `boost::make_shared`, not the `std` equivalents. Headers are `.h`;
implementations are `.cpp` **or** `.cxx`, mixed even within a directory, so match the immediate
neighbours. Namespaces mirror the `api/` path and close with `};  // namespace <full name>`,
trailing semicolon included. A namespace body is not indented, and neither is a continuation
line at namespace scope. 4-space indent, with access specifiers one space into the class body.
Log through `logger_->get()->info("... {}", value)`, never the old `LOG_*` macros. LF line
endings. [docs/Conventions.md](docs/contributing/Conventions.md) has the full set.

## When a change lands

Per [docs/sdlc.md](docs/sdlc.md): update the open plan or `docs/TODO.md`, set an ADR's status
if one was decided, re-read the comments the change added, and update the document that owns
whatever the change moved. Update this file only if the change moved the routing, one of the
rules above, or the shape of the tree.
